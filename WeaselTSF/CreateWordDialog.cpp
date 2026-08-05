#include "stdafx.h"

#include "CreateWordDialog.h"

#include <resource.h>

namespace {

// 取当前 DLL 模块句柄（对话框资源位于 weasel*.dll 中）
HMODULE GetCurrentModule() {
  HMODULE hModule = NULL;
  GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                     (LPCWSTR)GetCurrentModule, &hModule);
  return hModule;
}

void GetDlgItemText(HWND hwnd, int id, std::wstring& out) {
  HWND edit = GetDlgItem(hwnd, id);
  if (!edit)
    return;
  int len = GetWindowTextLengthW(edit);
  out.resize(len);
  GetWindowTextW(edit, &out[0], len + 1);
}

INT_PTR CALLBACK WordDialogProc(HWND hwnd,
                                UINT uMsg,
                                WPARAM wParam,
                                LPARAM lParam) {
  if (uMsg == WM_INITDIALOG) {
    CreateWordDialog* dlg = reinterpret_cast<CreateWordDialog*>(lParam);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dlg));
    if (dlg) {
      SetDlgItemTextW(hwnd, IDC_WORD_CODE, dlg->code().c_str());
      HWND edit = GetDlgItem(hwnd, IDC_WORD_TEXT);
      if (edit)
        SetFocus(edit);
      return FALSE;
    }
    return TRUE;
  }

  if (uMsg == WM_COMMAND) {
    switch (LOWORD(wParam)) {
      case IDOK: {
        CreateWordDialog* dlg = reinterpret_cast<CreateWordDialog*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (dlg) {
          std::wstring code, text;
          GetDlgItemText(hwnd, IDC_WORD_CODE, code);
          GetDlgItemText(hwnd, IDC_WORD_TEXT, text);
          if (code.empty() || text.empty()) {
            MessageBoxW(hwnd, L"编码和词语都不能为空。", L"Weasel",
                        MB_OK | MB_ICONINFORMATION);
            return TRUE;
          }
          dlg->set_code(code);
          dlg->set_text(text);
        }
        EndDialog(hwnd, IDOK);
        return TRUE;
      }
      case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        return TRUE;
    }
  }
  return FALSE;
}

}  // namespace

CreateWordDialog::CreateWordDialog(const std::wstring& code) : code_(code) {}

bool CreateWordDialog::DoModal(HWND owner) {
  INT_PTR ret = DialogBoxParamW(GetCurrentModule(), MAKEINTRESOURCE(IDD_CREATE_WORD),
                                owner, WordDialogProc,
                                reinterpret_cast<LPARAM>(this));
  return ret == IDOK;
}
