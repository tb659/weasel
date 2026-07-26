#pragma once

#include "resource.h"
#include <rime_levers_api.h>
#include <string>

class CreateWordDialog : public CDialogImpl<CreateWordDialog> {
 public:
  enum { IDD = IDD_CREATE_WORD };

  CreateWordDialog();
  ~CreateWordDialog();

  bool remove_mode() const { return remove_mode_; }
  const std::string& schema_id() const { return schema_id_; }
  const std::string& code() const { return code_; }
  const std::string& text() const { return text_; }

 protected:
  BEGIN_MSG_MAP(CreateWordDialog)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  MESSAGE_HANDLER(WM_CLOSE, OnClose)
  COMMAND_HANDLER(IDOK, BN_CLICKED, OnOK)
  COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnCancel)
  COMMAND_HANDLER(IDC_WORD_ACTION, CBN_SELCHANGE, OnActionSelChange)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnOK(WORD, WORD, HWND, BOOL&);
  LRESULT OnCancel(WORD, WORD, HWND, BOOL&);
  LRESULT OnActionSelChange(WORD, WORD, HWND, BOOL&);

  void PopulateSchemas();
  void UpdateActionHint();

  RimeLeversApi* api_;
  CComboBox action_;
  CComboBox schema_;
  CEdit code_edit_;
  CEdit text_edit_;
  CStatic hint_;

  bool remove_mode_ = false;
  std::string schema_id_;
  std::string code_;
  std::string text_;
};
