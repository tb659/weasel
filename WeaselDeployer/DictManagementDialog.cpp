#include "stdafx.h"
#include "DictManagementDialog.h"
#include "CreateWordDialog.h"
#include "Configurator.h"
#include "TraceLog.h"
#include <WeaselUtility.h>
#include <rime_api.h>
#include "WeaselDeployer.h"

void static OpenFolderAndSelectItem(std::wstring filepath) {
  filepath = std::filesystem::path(filepath).make_preferred().wstring();
  std::wstring directory = std::filesystem::path(filepath).parent_path();

  HRESULT hr;
  hr = CoInitializeEx(0, COINIT_MULTITHREADED);

  auto folder = ILCreateFromPath(directory.c_str());
  std::vector<LPITEMIDLIST> v;
  v.push_back(ILCreateFromPath(filepath.c_str()));

  SHOpenFolderAndSelectItems(folder, v.size(), (LPCITEMIDLIST*)v.data(), 0);

  for (auto idl : v) {
    ILFree(idl);
  }
  ILFree(folder);
  CoUninitialize();
}

template <typename T, typename U>
inline static std::wstring DoFileDialog(HWND hwndOwner,
                                        LPCWSTR title,
                                        UINT filterSize,
                                        COMDLG_FILTERSPEC filter[],
                                        LPCWSTR filename,
                                        LPCWSTR defExt) {
  std::wstring path;
  CoInitialize(NULL);
  CComPtr<T> spFileDialog;
  if (SUCCEEDED(spFileDialog.CoCreateInstance(__uuidof(U)))) {
    spFileDialog->SetFileTypes(filterSize, filter);
    spFileDialog->SetTitle(title);
    if (filename)
      spFileDialog->SetFileName(filename);

    spFileDialog->SetDefaultExtension(defExt);
    if (SUCCEEDED(spFileDialog->Show(hwndOwner))) {
      CComPtr<IShellItem> spResult;
      if (SUCCEEDED(spFileDialog->GetResult(&spResult))) {
        wchar_t* name;
        if (SUCCEEDED(spResult->GetDisplayName(SIGDN_FILESYSPATH, &name))) {
          path = name;
          CoTaskMemFree(name);
        }
      }
    }
  }
  CoUninitialize();
  return path;
}

DictManagementDialog::DictManagementDialog() {
  api_ = (RimeLeversApi*)rime_get_api()->find_module("levers")->get_api();
}

DictManagementDialog::~DictManagementDialog() {}

void DictManagementDialog::Populate() {
  RimeUserDictIterator iter = {0};
  api_->user_dict_iterator_init(&iter);
  while (const char* dict = api_->next_user_dict(&iter)) {
    std::wstring txt = u8tow(dict);
    user_dict_list_.AddString(txt.c_str());
  }
  api_->user_dict_iterator_destroy(&iter);
  user_dict_list_.SetCurSel(-1);
}

LRESULT DictManagementDialog::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
  AppendWeaselDeployerTrace(L"DictManagementDialog::OnInitDialog");
  user_dict_list_.Attach(GetDlgItem(IDC_USER_DICT_LIST));
  backup_.Attach(GetDlgItem(IDC_BACKUP));
  backup_.EnableWindow(FALSE);
  restore_.Attach(GetDlgItem(IDC_RESTORE));
  restore_.EnableWindow(TRUE);
  export_.Attach(GetDlgItem(IDC_EXPORT));
  export_.EnableWindow(FALSE);
  import_.Attach(GetDlgItem(IDC_IMPORT));
  import_.EnableWindow(FALSE);
  create_word_.Attach(GetDlgItem(IDC_CREATE_WORD_BUTTON));
  sync_user_data_.Attach(GetDlgItem(IDC_SYNC_USER_DATA_BUTTON));

  Populate();

  CenterWindow();
  BringWindowToTop();
  return TRUE;
}

LRESULT DictManagementDialog::OnClose(UINT, WPARAM, LPARAM, BOOL&) {
  EndDialog(IDCANCEL);
  return 0;
}

LRESULT DictManagementDialog::OnCommandTrace(UINT,
                                             WPARAM wParam,
                                             LPARAM,
                                             BOOL& bHandled) {
  const WORD id = LOWORD(wParam);
  const WORD code = HIWORD(wParam);
  AppendWeaselDeployerTrace(std::wstring(L"DictManagementDialog::WM_COMMAND id=") +
                            std::to_wstring(id) + L" code=" +
                            std::to_wstring(code));
  bHandled = FALSE;
  return 0;
}

bool DictManagementDialog::UpdateUserPhraseInPlace(const std::string& schema_id,
                                                   const std::string& code,
                                                   const std::string& text,
                                                   bool remove) {
  AppendWeaselDeployerTrace(L"DictManagementDialog::UpdateUserPhraseInPlace begin");
  if (!api_ || schema_id.empty() || code.empty() || text.empty()) {
    AppendWeaselDeployerTrace(L"DictManagementDialog::UpdateUserPhraseInPlace invalid args");
    return false;
  }
  const Bool ok = remove ? api_->remove_user_phrase(schema_id.c_str(), code.c_str(),
                                                      text.c_str())
                         : api_->add_user_phrase(schema_id.c_str(), code.c_str(),
                                                text.c_str());
  AppendWeaselDeployerTrace(std::wstring(L"DictManagementDialog::UpdateUserPhraseInPlace ok=") + (ok ? L"true" : L"false"));
  return !!ok;
}

bool DictManagementDialog::SyncUserDataInPlace() {
  RimeApi* rime = rime_get_api();
  if (!rime || !rime->sync_user_data()) {
    return false;
  }
  rime->join_maintenance_thread();
  return true;
}

LRESULT DictManagementDialog::OnCreateWord(WORD, WORD, HWND, BOOL&) {
  AppendWeaselDeployerTrace(L"DictManagementDialog::OnCreateWord begin");
  CreateWordDialog dialog;
  const INT_PTR result = dialog.DoModal();
  AppendWeaselDeployerTrace(std::wstring(L"DictManagementDialog::OnCreateWord DoModal=") + std::to_wstring(result));
  if (result != IDOK) {
    return 0;
  }
  if (!UpdateUserPhraseInPlace(dialog.schema_id(), dialog.code(),
                               dialog.text(), dialog.remove_mode())) {
    AppendWeaselDeployerTrace(L"DictManagementDialog::OnCreateWord update failed");
    const wchar_t* action = dialog.remove_mode() ? L"删词" : L"造词";
    std::wstring message = std::wstring(L"执行") + action +
                           L"失败。\n请检查方案、编码和词条是否正确。";
    MessageBox(message.c_str(), L"Weasel Deployer",
               MB_OK | MB_ICONERROR);
    return 0;
  }
  const wchar_t* action = dialog.remove_mode() ? L"删词" : L"造词";
  AppendWeaselDeployerTrace(L"DictManagementDialog::OnCreateWord update ok");
  std::wstring prompt = std::wstring(action) +
                        L"完成。\n是否立即同步用户资料？";
  if (MessageBox(prompt.c_str(), L"Weasel Deployer",
                 MB_YESNO | MB_ICONQUESTION) == IDYES) {
    if (!SyncUserDataInPlace()) {
      MessageBox(L"同步用户资料失败。", L"Weasel Deployer",
                 MB_OK | MB_ICONERROR);
      return 0;
    }
    MessageBox(L"同步用户资料完成。", L"Weasel Deployer",
               MB_OK | MB_ICONINFORMATION);
    return 0;
  }
  std::wstring message = std::wstring(action) +
                         L"完成。\n如需跨端同步，请点击“同步用户资料”。";
  MessageBox(message.c_str(), L"Weasel Deployer",
             MB_OK | MB_ICONINFORMATION);
  return 0;
}

LRESULT DictManagementDialog::OnSyncUserData(WORD, WORD, HWND, BOOL&) {
  if (!SyncUserDataInPlace()) {
    MessageBox(L"同步用户资料失败。", L"Weasel Deployer",
               MB_OK | MB_ICONERROR);
    return 0;
  }
  MessageBox(L"同步用户资料完成。", L"Weasel Deployer",
             MB_OK | MB_ICONINFORMATION);
  return 0;
}

LRESULT DictManagementDialog::OnBackup(WORD, WORD code, HWND, BOOL&) {
  int sel = user_dict_list_.GetCurSel();
  if (sel < 0 || sel >= user_dict_list_.GetCount()) {
    MSG_BY_IDS(IDS_STR_SEL_EXPORT_DICT_NAME, IDS_STR_SAD,
               MB_OK | MB_ICONINFORMATION);
    return 0;
  }
  std::wstring path;
  {
    char dir[MAX_PATH] = {0};
    rime_get_api()->get_user_data_sync_dir(dir, _countof(dir));
    WCHAR wdir[MAX_PATH] = {0};
    MultiByteToWideChar(CP_ACP, 0, dir, -1, wdir, _countof(wdir));
    path = wdir;
  }
  if (_waccess_s(path.c_str(), 0) != 0 &&
      !CreateDirectoryW(path.c_str(), NULL) &&
      GetLastError() == ERROR_PATH_NOT_FOUND) {
    MSG_BY_IDS(IDS_STR_ERREXPORT_SYNC_UV, IDS_STR_SAD, MB_OK | MB_ICONERROR);
    return 0;
  }
  WCHAR dict_name[100] = {0};
  user_dict_list_.GetText(sel, dict_name);
  path += std::wstring(L"\\") + dict_name + L".userdb.txt";
  std::string dict_name_str = wtou8(dict_name);
  if (!api_->backup_user_dict(dict_name_str.c_str())) {
    MSG_BY_IDS(IDS_STR_ERR_EXPORT_UNKNOWN, IDS_STR_SAD, MB_OK | MB_ICONERROR);
    return 0;
  } else if (_waccess(path.c_str(), 0) != 0) {
    MSG_BY_IDS(IDS_STR_ERR_EXPORT_SNAP_LOST, IDS_STR_SAD, MB_OK | MB_ICONERROR);
    return 0;
  }
  OpenFolderAndSelectItem(path);
  return 0;
}

LRESULT DictManagementDialog::OnRestore(WORD, WORD code, HWND, BOOL&) {
  CString open_str, dict_snapshot_str, kcss_dict_snapshot_str, all_files_str;
  open_str.LoadStringW(IDS_STR_OPEN);
  dict_snapshot_str.LoadStringW(IDS_STR_DICT_SNAPSHOT);
  kcss_dict_snapshot_str.LoadStringW(IDS_STR_KCSS_DICT_SNAPSHOT);
  all_files_str.LoadStringW(IDS_STR_ALL_FILES);

  const std::wstring dict_snapshot_name =
      dict_snapshot_str + L" (*.userdb.txt)";
  const std::wstring kcss_dict_snapshot_name =
      kcss_dict_snapshot_str + L" (*.userdb.kct.snapshot)";
  const std::wstring all_files_name = all_files_str;

  COMDLG_FILTERSPEC filter[3] = {
      {dict_snapshot_name.c_str(), L"*.userdb.txt"},
      {kcss_dict_snapshot_name.c_str(), L"*.userdb.kct.snapshot"},
      {all_files_name.c_str(), L"*.*"}};

  std::wstring selected_path = DoFileDialog<IFileOpenDialog, FileOpenDialog>(
      m_hWnd, open_str, ARRAYSIZE(filter), filter, NULL, L"snapshot");
  if (!selected_path.empty()) {
    char path[MAX_PATH] = {0};
    WideCharToMultiByte(CP_UTF8, 0, selected_path.c_str(), -1, path,
                        _countof(path), NULL, NULL);
    if (!api_->restore_user_dict(path)) {
      MSG_BY_IDS(IDS_STR_ERR_UNKNOWN, IDS_STR_SAD, MB_OK | MB_ICONERROR);
    } else {
      MSG_BY_IDS(IDS_STR_ERR_SUCCESS, IDS_STR_HAPPY,
                 MB_OK | MB_ICONINFORMATION);
    }
  }
  return 0;
}

LRESULT DictManagementDialog::OnExport(WORD, WORD code, HWND, BOOL&) {
  CString save_as_str, exported_str, record_count_str, all_files_str,
      txt_files_str;
  save_as_str.LoadStringW(IDS_STR_SAVE_AS);
  exported_str.LoadStringW(IDS_STR_EXPORTED);
  record_count_str.LoadStringW(IDS_STR_RECORD_COUNT);
  txt_files_str.LoadStringW(IDS_STR_TXT_FILES);
  all_files_str.LoadStringW(IDS_STR_ALL_FILES);
  const std::wstring txt_files_name = txt_files_str + L" (*.txt)";
  const std::wstring all_files_name = all_files_str;

  int sel = user_dict_list_.GetCurSel();
  if (sel < 0 || sel >= user_dict_list_.GetCount()) {
    MSG_BY_IDS(IDS_STR_SEL_EXPORT_DICT_NAME, IDS_STR_SAD,
               MB_OK | MB_ICONINFORMATION);
    return 0;
  }
  WCHAR dict_name[MAX_PATH] = {0};
  user_dict_list_.GetText(sel, dict_name);
  std::wstring file_name(dict_name);
  file_name += L"_export.txt";

  COMDLG_FILTERSPEC filter[2] = {{txt_files_name.c_str(), L"*.txt"},
                                 {all_files_name.c_str(), L"*.*"}};

  OutputDebugString(filter[0].pszName);
  std::wstring selected_path = DoFileDialog<IFileSaveDialog, FileSaveDialog>(
      m_hWnd, save_as_str, ARRAYSIZE(filter), filter, file_name.c_str(),
      L"txt");
  if (!selected_path.empty()) {
    char path[MAX_PATH] = {0};
    WideCharToMultiByte(CP_UTF8, 0, selected_path.c_str(), -1, path,
                        _countof(path), NULL, NULL);
    std::string dict_name_str = wtou8(dict_name);
    int result = api_->export_user_dict(dict_name_str.c_str(), path);
    if (result < 0) {
      MSG_BY_IDS(IDS_STR_ERR_UNKNOWN, IDS_STR_SAD, MB_OK | MB_ICONERROR);
    } else if (_waccess(selected_path.c_str(), 0) != 0) {
      MSG_BY_IDS(IDS_STR_ERR_EXPORT_FILE_LOST, IDS_STR_SAD,
                 MB_OK | MB_ICONERROR);
    } else {
      std::wstring report(std::wstring(exported_str) + L" " +
                          std::to_wstring(result) + L" " +
                          std::wstring(record_count_str));
      MSG_ID_CAP(report.c_str(), IDS_STR_HAPPY, MB_OK | MB_ICONINFORMATION);
      OpenFolderAndSelectItem(selected_path);
    }
  }
  return 0;
}

LRESULT DictManagementDialog::OnImport(WORD, WORD code, HWND, BOOL&) {
  CString open_str, imported_str, record_count_str, all_files_str,
      txt_files_str;
  open_str.LoadStringW(IDS_STR_OPEN);
  imported_str.LoadStringW(IDS_STR_IMPORTED);
  record_count_str.LoadStringW(IDS_STR_RECORD_COUNT);
  txt_files_str.LoadStringW(IDS_STR_TXT_FILES);
  all_files_str.LoadStringW(IDS_STR_ALL_FILES);
  const std::wstring txt_files_name = txt_files_str + L" (*.txt)";
  const std::wstring all_files_name = all_files_str;

  int sel = user_dict_list_.GetCurSel();
  if (sel < 0 || sel >= user_dict_list_.GetCount()) {
    MSG_BY_IDS(IDS_STR_SEL_IMPORT_DICT_NAME, IDS_STR_SAD,
               MB_OK | MB_ICONINFORMATION);
    return 0;
  }
  WCHAR dict_name[MAX_PATH] = {0};
  user_dict_list_.GetText(sel, dict_name);
  std::wstring file_name(dict_name);
  file_name += L"_export.txt";

  COMDLG_FILTERSPEC filter[2] = {{txt_files_name.c_str(), L"*.txt"},
                                 {all_files_name.c_str(), L"*.*"}};

  OutputDebugString(filter[0].pszName);
  std::wstring selected_path = DoFileDialog<IFileOpenDialog, FileOpenDialog>(
      m_hWnd, open_str, ARRAYSIZE(filter), filter, file_name.c_str(), L"txt");
  if (!selected_path.empty()) {
    char path[MAX_PATH] = {0};
    WideCharToMultiByte(CP_UTF8, 0, selected_path.c_str(), -1, path,
                        _countof(path), NULL, NULL);
    int result = api_->import_user_dict(wtou8(dict_name).c_str(), path);
    if (result < 0) {
      MSG_BY_IDS(IDS_STR_ERR_UNKNOWN, IDS_STR_SAD, MB_OK | MB_ICONERROR);
    } else {
      std::wstring report(std::wstring(imported_str) + L" " +
                          std::to_wstring(result) + L" " +
                          std::wstring(record_count_str));
      MSG_ID_CAP(report.c_str(), IDS_STR_HAPPY, MB_OK | MB_ICONINFORMATION);
    }
  }
  return 0;
}

LRESULT DictManagementDialog::OnUserDictListSelChange(WORD, WORD, HWND, BOOL&) {
  int index = user_dict_list_.GetCurSel();
  BOOL enabled = index < 0 ? FALSE : TRUE;
  backup_.EnableWindow(enabled);
  export_.EnableWindow(enabled);
  import_.EnableWindow(enabled);
  return 0;
}
