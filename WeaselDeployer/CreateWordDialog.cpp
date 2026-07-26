#include "stdafx.h"

#include "CreateWordDialog.h"

#include "TraceLog.h"

#include <WeaselUtility.h>

CreateWordDialog::CreateWordDialog() {
  auto* module = rime_get_api()->find_module("levers");
  api_ = module ? reinterpret_cast<RimeLeversApi*>(module->get_api()) : nullptr;
}

CreateWordDialog::~CreateWordDialog() {}

void CreateWordDialog::PopulateSchemas() {
  if (!api_)
    return;
  RimeSwitcherSettings* settings = api_->switcher_settings_init();
  if (!settings)
    return;
  if (!api_->load_settings(reinterpret_cast<RimeCustomSettings*>(settings))) {
    api_->custom_settings_destroy(reinterpret_cast<RimeCustomSettings*>(settings));
    return;
  }
  RimeSchemaList available = {0};
  if (api_->get_available_schema_list(settings, &available)) {
    for (size_t i = 0; i < available.size; ++i) {
      const char* schema_id = available.list[i].schema_id;
      if (!schema_id || !*schema_id)
        continue;
      std::wstring text = u8tow(schema_id);
      int index = schema_.AddString(text.c_str());
      schema_.SetItemData(index, i);
    }
    api_->schema_list_destroy(&available);
  }
  api_->custom_settings_destroy(reinterpret_cast<RimeCustomSettings*>(settings));
  if (schema_.GetCount() > 0) {
    schema_.SetCurSel(0);
  }
}

void CreateWordDialog::UpdateActionHint() {
  CString hint = remove_mode_ ? L"删词会按方案与编码精确删除对应词条。"
                              : L"造词会按方案与编码写入用户词典。";
  hint_.SetWindowTextW(hint);
}

LRESULT CreateWordDialog::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
  AppendWeaselDeployerTrace(L"CreateWordDialog::OnInitDialog");
  action_.Attach(GetDlgItem(IDC_WORD_ACTION));
  schema_.Attach(GetDlgItem(IDC_WORD_SCHEMA));
  code_edit_.Attach(GetDlgItem(IDC_WORD_CODE));
  text_edit_.Attach(GetDlgItem(IDC_WORD_TEXT));
  hint_.Attach(GetDlgItem(IDC_WORD_HINT));

  action_.AddString(L"造词");
  action_.AddString(L"删词");
  action_.SetCurSel(0);
  remove_mode_ = false;

  PopulateSchemas();
  UpdateActionHint();

  CenterWindow();
  BringWindowToTop();
  return TRUE;
}

LRESULT CreateWordDialog::OnClose(UINT, WPARAM, LPARAM, BOOL&) {
  EndDialog(IDCANCEL);
  return 0;
}

LRESULT CreateWordDialog::OnCancel(WORD, WORD, HWND, BOOL&) {
  AppendWeaselDeployerTrace(L"CreateWordDialog::OnCancel");
  EndDialog(IDCANCEL);
  return 0;
}

LRESULT CreateWordDialog::OnActionSelChange(WORD, WORD, HWND, BOOL&) {
  remove_mode_ = action_.GetCurSel() == 1;
  UpdateActionHint();
  return 0;
}

LRESULT CreateWordDialog::OnOK(WORD, WORD, HWND, BOOL&) {
  AppendWeaselDeployerTrace(L"CreateWordDialog::OnOK begin");
  int schema_index = schema_.GetCurSel();
  if (schema_index < 0) {
    MessageBox(L"请先选择输入方案。", L"Weasel Deployer",
               MB_OK | MB_ICONINFORMATION);
    return 0;
  }

  CString schema_text;
  CString code_text;
  CString word_text;
  schema_.GetLBText(schema_index, schema_text);
  code_edit_.GetWindowTextW(code_text);
  text_edit_.GetWindowTextW(word_text);

  schema_id_ = wtou8(std::wstring(schema_text));
  code_ = wtou8(std::wstring(code_text));
  text_ = wtou8(std::wstring(word_text));

  if (schema_id_.empty() || code_.empty() || text_.empty()) {
    MessageBox(L"方案、编码和词条都不能为空。", L"Weasel Deployer",
               MB_OK | MB_ICONINFORMATION);
    return 0;
  }

  remove_mode_ = action_.GetCurSel() == 1;
  AppendWeaselDeployerTrace(L"CreateWordDialog::OnOK end -> IDOK");
  EndDialog(IDOK);
  return 0;
}
