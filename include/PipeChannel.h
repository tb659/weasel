#pragma once
#include <string>
#include <sstream>
#include <memory>
#include <windows.h>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <boost/thread.hpp>
#include <boost/thread/tss.hpp>

namespace weasel {

class PipeChannelBase {
 public:
  using Stream = boost::interprocess::wbufferstream;

  struct ChannelContext {
    std::unique_ptr<char[]> buffer;
    std::unique_ptr<Stream> write_stream;
    bool has_body;
    size_t body_size;  // body 实际字节数（WriteBody 设置）

    ChannelContext(size_t bs)
        : buffer(std::make_unique<char[]>(bs)),
          has_body(false),
          body_size(0) {}
  };

  PipeChannelBase(std::wstring&& pn_cmd, size_t bs, SECURITY_ATTRIBUTES* s);
  ~PipeChannelBase();

 protected:
  /* To ensure connection before operation */
  bool _Ensure();
  /* Connect pipe as client */
  HANDLE _Connect(const wchar_t* name);
  /* To reconnect message pipe */
  void _Reconnect();
  /* Try to connect for one time */
  HANDLE _TryConnect();
  size_t _WritePipe(HANDLE p, size_t s, char* b);
  void _FinalizePipe(HANDLE& p);
  void _Receive(HANDLE pipe, LPVOID msg, size_t rec_len);
  /* Try to get a connection from client */
  HANDLE _ConnectServerPipe(std::wstring& pn);
  inline bool _Invalid(HANDLE p) const { return p == INVALID_HANDLE_VALUE; }

  HANDLE* _GetPipeHandle() const {
    if (!hpipe_ptr.get()) {
      hpipe_ptr.reset(new HANDLE(INVALID_HANDLE_VALUE));
    }
    return hpipe_ptr.get();
  }

  ChannelContext* _GetContext() const {
    if (!context.get()) {
      context.reset(new ChannelContext(buff_size));
    }
    return context.get();
  }

 protected:
  std::wstring pname;
  // Thread-local pipe handle for isolation
  mutable boost::thread_specific_ptr<HANDLE> hpipe_ptr;
  const size_t buff_size;
  // Thread-local context for buffer and state
  mutable boost::thread_specific_ptr<ChannelContext> context;

 private:
  /* Security attributes */
  SECURITY_ATTRIBUTES* sa;
};

/* Pipe based IPC channel */
template <typename _TyMsg,
          typename _TyRes = DWORD,
          size_t _MsgSize = sizeof(_TyMsg),
          size_t _ResSize = sizeof(_TyRes)>
class PipeChannel : public PipeChannelBase {
 public:
  /* Type definitions */

  using Ptr = std::shared_ptr<PipeChannel>;
  using UPtr = std::unique_ptr<PipeChannel>;
  using Msg = _TyMsg;
  using Res = _TyRes;

  enum class ChannalCommand { NEW_MSG_PIPE, REFRESH };

 public:
  PipeChannel(std::wstring&& pn_cmd,
              SECURITY_ATTRIBUTES* s = NULL,
              size_t bs = 64 * 1024)
      : PipeChannelBase(std::move(pn_cmd), bs, s) {}

 public:
  /* Common pipe operations */

  bool Connect() { return _Ensure(); }
  bool Connected() const {
    HANDLE* phandle = _GetPipeHandle();
    return !_Invalid(*phandle);
  }
  void Disconnect() {
    HANDLE* phandle = _GetPipeHandle();
    _FinalizePipe(*phandle);
  }

  /* Write data to buffer (append semantics, multiple << calls concatenate) */
  template <typename _TyWrite>
  void Write(_TyWrite cnt) {
    std::wstringstream ss;
    ss << cnt;
    const std::wstring& s = ss.str();
    if (s.empty())
      return;
    auto ctx = _GetContext();
    size_t offset = ctx->body_size / sizeof(wchar_t);
    char* pbuff = ctx->buffer.get() + _MsgSize;
    if (offset == 0)
      memset(pbuff, 0, buff_size - _MsgSize);
    size_t bytes = s.size() * sizeof(wchar_t);
    if (offset * sizeof(wchar_t) + bytes <= buff_size - _MsgSize) {
      memcpy(pbuff + offset * sizeof(wchar_t), s.c_str(), bytes);
      ctx->body_size += bytes;
    }
    ctx->has_body = true;
  }

  /* Write data to buffer */
  template <typename _TyWrite>
  PipeChannel& operator<<(_TyWrite cnt) {
    Write(cnt);
    return *this;
  }

  /* 手动写入 body 数据（替换式，从 body 区起始位置写入） */
  void WriteBody(const wchar_t* data, size_t wchar_count) {
    auto ctx = _GetContext();
    char* pbuff = ctx->buffer.get() + _MsgSize;
    memset(pbuff, 0, buff_size - _MsgSize);
    if (data && wchar_count > 0) {
      memcpy(pbuff, data, wchar_count * sizeof(wchar_t));
    }
    ctx->has_body = true;
    ctx->body_size = wchar_count * sizeof(wchar_t);
  }

  _TyRes Transact(Msg& msg) {
    _Ensure();
    HANDLE* phandle = _GetPipeHandle();
    _Send(*phandle, msg);
    return _ReceiveResponse();
  }

  void ClearBufferStream() {
    auto ctx = _GetContext();
    ctx->has_body = false;
    ctx->body_size = 0;
    if (ctx->write_stream != nullptr) {
      ctx->write_stream.reset(nullptr);
    }
  }

  char* SendBuffer() const { return _GetContext()->buffer.get() + _MsgSize; }

  // body 区起点：_Receive 在消息模式下缓冲不足时会先部分读取（头进入
  // 调用方提供的 msg/result），第二次 ReadFile 把剩余 body 读入 buffer[0]
  char* ReceiveBuffer() const { return _GetContext()->buffer.get(); }

  template <typename _TyHandler>
  bool HandleResponseData(_TyHandler const& handler) {
    if (!handler) {
      return false;
    }

    // Use whole buffer to receive data in client
    return handler((LPWSTR)_GetContext()->buffer.get(),
                   (UINT)(buff_size * sizeof(char) / sizeof(wchar_t)));
  }

 protected:
  void _Send(HANDLE pipe, Msg& msg) {
    auto ctx = _GetContext();
    char* pbuff = ctx->buffer.get();
    DWORD lwritten = 0;

    *reinterpret_cast<Msg*>(pbuff) = msg;
    size_t body_bytes = ctx->has_body ? ctx->body_size : 0;
    size_t data_sz = ctx->has_body ? (_MsgSize + body_bytes) : _MsgSize;
    if (data_sz > buff_size)
      data_sz = buff_size;

    try {
      _WritePipe(pipe, data_sz, pbuff);
    } catch (...) {
      _Reconnect();
      _WritePipe(pipe, data_sz, pbuff);
    }
    ClearBufferStream();
  }

  _TyRes _ReceiveResponse() {
    HANDLE* phandle = _GetPipeHandle();
    _TyRes result;
    _Receive(*phandle, &result, sizeof(result));
    return result;
  }

  Stream& _BufferWriteStream() {
    auto ctx = _GetContext();
    if (ctx->write_stream == nullptr) {
      char* pbuff = (char*)ctx->buffer.get() + _MsgSize;
      memset(pbuff, 0, buff_size - _MsgSize);
      ctx->write_stream =
          std::make_unique<Stream>((wchar_t*)pbuff, _SendBufferSizeW());
    }
    return *ctx->write_stream;
  }

 private:
  inline size_t _SendBufferSizeW() const {
    return (buff_size - _MsgSize) * sizeof(char) / sizeof(wchar_t);
  }

  inline size_t _ReceiveBufferSizeW() const {
    return (buff_size - _ResSize) * sizeof(char) / sizeof(wchar_t);
  }
};
};  // namespace weasel
