#pragma once

#include <string>

// 造词对话框（TSF 进程内弹窗，输入法可用）
class CreateWordDialog {
 public:
  explicit CreateWordDialog(const std::wstring& code);

  bool DoModal(HWND owner);

  const std::wstring& code() const { return code_; }
  const std::wstring& text() const { return text_; }
  void set_code(const std::wstring& code) { code_ = code; }
  void set_text(const std::wstring& text) { text_ = text; }

 private:
  std::wstring code_;
  std::wstring text_;
};
