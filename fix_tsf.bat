@echo off
chcp 65001 >nul
cd /d "%~dp0"

net session >nul 2>&1
if %errorlevel% neq 0 (
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

echo 正在停止 WeaselServer...
taskkill /f /im WeaselServer.exe 2>nul

:: Auto-detect install dir from registry (written by WeaselSetup)
set RIME_DIR=
for /f "skip=1 tokens=2*" %%a in ('reg query "HKLM\SOFTWARE\Rime\Weasel" /v "WeaselRoot" 2^>nul') do set "RIME_DIR=%%b"
if not defined RIME_DIR (
  for /f "skip=1 tokens=2*" %%a in ('reg query "HKLM\SOFTWARE\WOW6432Node\Rime\Weasel" /v "WeaselRoot" 2^>nul') do set "RIME_DIR=%%b"
)
if not defined RIME_DIR (
  echo 错误: 未找到注册表 HKLM\Software\Rime\Weasel\WeaselRoot
  echo 请确认已安装小狼毫，或手动设置 RIME_DIR。
  pause
  exit /b 1
)

echo 安装目录: %RIME_DIR%

echo 正在复制新 WeaselSetup.exe...
copy /Y output\WeaselSetup.exe "%RIME_DIR%\WeaselSetup.exe"

echo 正在注册 TSF 输入法...
"%RIME_DIR%\WeaselSetup.exe" /s

echo 正在启动 WeaselServer...
start "" "%RIME_DIR%\WeaselServer.exe"

echo 完成！请切换到小狼毫输入法测试。
pause
