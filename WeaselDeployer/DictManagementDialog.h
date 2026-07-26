#pragma once

#include "resource.h"

#include <rime_levers_api.h>
#include <string>

class DictManagementDialog : public CDialogImpl<DictManagementDialog> {
 public:
  enum { IDD = IDD_DICT_MANAGEMENT };

  DictManagementDialog();
  ~DictManagementDialog();

 protected:
  BEGIN_MSG_MAP(DictManagementDialog)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  MESSAGE_HANDLER(WM_CLOSE, OnClose)
  MESSAGE_HANDLER(WM_COMMAND, OnCommandTrace)
  COMMAND_ID_HANDLER(IDC_BACKUP, OnBackup)
  COMMAND_ID_HANDLER(IDC_RESTORE, OnRestore)
  COMMAND_ID_HANDLER(IDC_EXPORT, OnExport)
  COMMAND_ID_HANDLER(IDC_IMPORT, OnImport)
  COMMAND_HANDLER(IDC_CREATE_WORD_BUTTON, BN_CLICKED, OnCreateWord)
  COMMAND_HANDLER(IDC_SYNC_USER_DATA_BUTTON, BN_CLICKED, OnSyncUserData)
  COMMAND_HANDLER(IDC_USER_DICT_LIST, LBN_SELCHANGE, OnUserDictListSelChange)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnCommandTrace(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnBackup(WORD, WORD code, HWND, BOOL&);
  LRESULT OnRestore(WORD, WORD code, HWND, BOOL&);
  LRESULT OnExport(WORD, WORD code, HWND, BOOL&);
  LRESULT OnImport(WORD, WORD code, HWND, BOOL&);
  LRESULT OnCreateWord(WORD, WORD code, HWND, BOOL&);
  LRESULT OnSyncUserData(WORD, WORD code, HWND, BOOL&);
  LRESULT OnUserDictListSelChange(WORD, WORD, HWND, BOOL&);

  void Populate();
  bool UpdateUserPhraseInPlace(const std::string& schema_id,
                               const std::string& code,
                               const std::string& text,
                               bool remove);
  bool SyncUserDataInPlace();

  CListBox user_dict_list_;
  CButton backup_;
  CButton restore_;
  CButton export_;
  CButton import_;
  CButton create_word_;
  CButton sync_user_data_;

  RimeLeversApi* api_;
};
