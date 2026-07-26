// WeaselDeployer.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include <WeaselUtility.h>
#include <fstream>
#include <vector>
#include "WeaselDeployer.h"
#include "Configurator.h"

CAppModule _Module;

static int Run(LPTSTR lpCmdLine);

static std::vector<std::wstring> GetCommandLineArgs() {
  int argc = 0;
  LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  std::vector<std::wstring> args;
  if (!argv)
    return args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }
  LocalFree(argv);
  return args;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR lpCmdLine,
                       int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);

  LANGID langId = get_language_id();
  SetThreadUILanguage(langId);
  SetThreadLocale(langId);

  HRESULT hRes = ::CoInitialize(NULL);
  // If you are running on NT 4.0 or higher you can use the following call
  // instead to make the EXE free threaded. This means that calls come in on a
  // random RPC thread.
  // HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
  ATLASSERT(SUCCEEDED(hRes));

  // this resolves ATL window thunking problem when Microsoft Layer for Unicode
  // (MSLU) is used
  ::DefWindowProc(NULL, 0, 0, 0L);

  AtlInitCommonControls(
      ICC_BAR_CLASSES);  // add flags to support other controls

  hRes = _Module.Init(NULL, hInstance);
  ATLASSERT(SUCCEEDED(hRes));

  CreateDirectory(WeaselUserDataPath().c_str(), NULL);

  int ret = 0;
  HANDLE hMutex = CreateMutex(NULL, TRUE, L"WeaselDeployerExclusiveMutex");
  if (!hMutex) {
    ret = 1;
  } else if (GetLastError() == ERROR_ALREADY_EXISTS) {
    ret = 1;
  } else {
    ret = Run(lpCmdLine);
  }

  if (hMutex) {
    CloseHandle(hMutex);
  }
  _Module.Term();
  ::CoUninitialize();

  return ret;
}

static int Run(LPTSTR lpCmdLine) {
  Configurator configurator;
  configurator.Initialize();
  auto args = GetCommandLineArgs();

  if (args.size() == 1 && (args[0] == L"/?" || args[0] == L"/help")) {
    WCHAR msg[1024] = {0};
    if (LoadString(GetModuleHandle(NULL), IDS_STR_HELP, msg,
                   sizeof(msg) / sizeof(TCHAR))) {
      wcscat_s(msg,
               L"\n/word /add <schema_id> <code> <text>"
               L"\n/word /remove <schema_id> <code> <text>");
      MessageBox(NULL, msg, L"Weasel Deployer", MB_ICONINFORMATION | MB_OK);
    } else {
      MessageBox(NULL,
                 L"Usage: WeaselDeployer.exe [options]\n"
                 L"/? or /help		- Show this help message\n"
                 L"/deploy		- Update Workspace\n"
                 L"/dict		- Manage dictionary\n"
                 L"/sync		- Sync user data\n"
                 L"/install		- Install Weasel (Initial deployment)\n"
                 L"/word /add <schema_id> <code> <text>\n"
                 L"/word /remove <schema_id> <code> <text>",
                  L"Weasel Deployer", MB_ICONINFORMATION | MB_OK);
    }
    return 0;
  }

  if (args.size() == 1 && args[0] == L"/word") {
    return configurator.CreateWord();
  }

  if (args.size() == 5 && args[0] == L"/word" &&
      (args[1] == L"/add" || args[1] == L"/remove")) {
    return configurator.UpdateUserPhrase(wtou8(args[2]), wtou8(args[3]),
                                         wtou8(args[4]),
                                         args[1] == L"/remove");
  }

  if (!args.empty() && args[0] == L"/word") {
    MessageBox(NULL,
               L"用法：\n"
               L"WeaselDeployer.exe /word /add <schema_id> <code> <text>\n"
               L"WeaselDeployer.exe /word /remove <schema_id> <code> <text>",
               L"Weasel Deployer", MB_OK | MB_ICONINFORMATION);
    return 1;
  }

  if (args.size() == 1 && args[0] == L"/deploy") {
    return configurator.UpdateWorkspace();
  }

  if (args.size() == 1 && args[0] == L"/dict") {
    return configurator.DictManagement();
  }

  if (args.size() == 1 && args[0] == L"/sync") {
    return configurator.SyncUserData();
  }

  const bool installing = args.size() == 1 && args[0] == L"/install";
  return configurator.Run(installing);
}
