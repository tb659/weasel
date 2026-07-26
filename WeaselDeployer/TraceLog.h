#pragma once

#include <fstream>
#include <string>
#include <windows.h>

inline void AppendWeaselDeployerTrace(const std::wstring& message) {
  wchar_t temp_path[MAX_PATH] = {0};
  const DWORD length = GetTempPathW(_countof(temp_path), temp_path);
  if (length == 0 || length >= _countof(temp_path)) {
    return;
  }
  std::wstring path = std::wstring(temp_path) + L"weasel_deployer_trace.log";
  std::wofstream out(path, std::ios::app);
  if (!out.is_open()) {
    return;
  }
  SYSTEMTIME st = {0};
  GetLocalTime(&st);
  out << L'[' << st.wHour << L':' << st.wMinute << L':' << st.wSecond << L'.'
      << st.wMilliseconds << L"] " << message << std::endl;
}
