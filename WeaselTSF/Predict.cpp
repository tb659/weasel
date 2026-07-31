#include "stdafx.h"
#include "WeaselTSF.h"
#include "CandidateList.h"
#include <KeyEvent.h>

// Backspace prediction — 回删/移动光标后，根据光标前文本触发上屏预测。
// 与 user_predict.lua 的"外部请求文件"桥接机制配合：
//   1. OnEndEdit（非组合态、光标前文本变化）→ 读光标前最多 3 个汉字
//   2. 经 IPC（PredictRequest）交由 WeaselServer 写入
//      %APPDATA%\Rime\user_predict_request.txt（revision = GetTickCount()）
//   3. 向引擎模拟发送占位符按键 zpredictz
//   4. lua 端消费请求文件 → activate_external_prediction → 显示预测候选

namespace {
const wchar_t* kPredictPlaceholder = L"zpredictz";
const DWORD kSuppressAfterCommit = 800;          // ms
const size_t kMaxAnchorCjkChars = 3;
const size_t kMaxAnchorUtf16Units = kMaxAnchorCjkChars * 2;

bool IsCjkChar(wchar_t ch) {
  return (ch >= 0x4E00 && ch <= 0x9FFF) ||   // CJK Unified Ideographs
         (ch >= 0x3400 && ch <= 0x4DBF) ||   // Extension A
         (ch >= 0x20000 && ch <= 0x2A6DF);   // Extension B (surrogate pairs)
}

// 从选区 range 提取光标前最多 max_cjk_chars 个汉字。
// 不做编辑会话请求，使用 OnEndEdit 提供的只读 cookie。
std::wstring GetTextBeforeCaret(TfEditCookie ec,
                                ITfContext* pContext,
                                size_t max_cjk_chars) {
  TF_SELECTION selection;
  ULONG fetched = 0;
  HRESULT hr = pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &selection,
                                      &fetched);
  if (hr != S_OK || fetched != 1 || !selection.range) {
    return L"";
  }

  com_ptr<ITfRange> range;
  hr = selection.range->Clone(&range);
  if (hr != S_OK) {
    selection.range->Release();
    return L"";
  }
  selection.range->Release();

  // start 向光标位置之前移动，限定读取窗口
  const LONG max_backward = static_cast<LONG>(max_cjk_chars * 2);
  LONG moved = 0;
  hr = range->ShiftStart(ec, -max_backward, &moved, NULL);
  if (hr != S_OK) {
    return L"";
  }

  wchar_t buffer[kMaxAnchorUtf16Units + 2];
  fetched = 0;
  hr = range->GetText(ec, 0, buffer, kMaxAnchorUtf16Units + 1, &fetched);
  if (hr != S_OK || fetched == 0) {
    return L"";
  }
  std::wstring raw(buffer, fetched);

  // 从末尾提取最多 max_cjk_chars 个汉字
  std::wstring result;
  for (auto it = raw.rbegin(); it != raw.rend(); ++it) {
    wchar_t ch = *it;
    if (IsCjkChar(ch)) {
      result.insert(result.begin(), ch);
      if (result.size() >= max_cjk_chars)
        break;
    } else if (!result.empty()) {
      break;  // 非汉字且已有候选 → 停止（只取连续的汉字前缀）
    }
  }
  return result;
}
}  // namespace

void WeaselTSF::_SetLastCommitTick() {
  _last_commit_tick = GetTickCount();
}

void WeaselTSF::_TriggerBackspacePredict(ITfContext* pContext,
                                         TfEditCookie ec) {
  // 组合态：由 lua 侧正常预测链处理，不干预。
  // 此时记录"组合期光标前文本"，用于组合退出时判断文档是否真的变了。
  if (_IsComposing()) {
    _composing_anchor = _GetCaretPrecedingText(pContext, ec, kMaxAnchorCjkChars);
    return;
  }
  // 英文模式不预测
  if (_status.ascii_mode)
    return;
  // 刚上屏后的短暂窗口：上屏联想由 lua commit_cb 自行注入，避免抢占
  DWORD now = GetTickCount();
  if (_last_commit_tick &&
      now - _last_commit_tick < kSuppressAfterCommit) {
    return;
  }

  std::wstring anchor = _GetCaretPrecedingText(pContext, ec, kMaxAnchorCjkChars);
  if (anchor.empty())
    return;
  // 组合刚退出（如退格/Enter 清掉占位符）且文档文本未变化 → 不触发，
  // 否则"删除占位符"会被误当成"删除上屏文本"，导致预测锚点滞后一个字。
  if (!_composing_anchor.empty() && anchor == _composing_anchor) {
    _composing_anchor.clear();
    return;
  }
  _composing_anchor.clear();
  // 光标前文本未变化（例如仅仅是自身的 UI 更新）→ 跳过
  if (anchor == _last_predict_anchor)
    return;
  _last_predict_anchor = anchor;
  _last_predict_tick = now;

  _WritePredictRequestFile(anchor);
  _SendPredictPlaceholderKeys();
}

std::wstring WeaselTSF::_GetCaretPrecedingText(ITfContext* pContext,
                                               TfEditCookie ec,
                                               size_t max_cjk_chars) {
  return GetTextBeforeCaret(ec, pContext, max_cjk_chars);
}

void WeaselTSF::_WritePredictRequestFile(const std::wstring& anchor) {
  if (anchor.empty())
    return;
  // 通过 IPC 交由 WeaselServer 写入请求文件：
  // TSF 客户端可能运行在 UWP/浏览器沙箱中，无法直接写 %APPDATA%。
  m_client.PredictRequest(anchor);
}

void WeaselTSF::_SendPredictPlaceholderKeys() {
  // 向引擎模拟输入占位符 zpredictz（形码方案中任意前缀都不是有效编码，不会顶字）
  for (const wchar_t* p = kPredictPlaceholder; *p; ++p) {
    m_client.ProcessKeyEvent(weasel::KeyEvent(static_cast<UINT>(*p), 0));
  }
}
