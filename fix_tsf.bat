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

set "RIME_DIR=C:\Program Files\Rime\weasel-1.0.2"

echo 正在复制新 WeaselSetup.exe...
copy /Y output\WeaselSetup.exe "%RIME_DIR%\WeaselSetup.exe"

echo 正在注册 TSF 输入法...
"%RIME_DIR%\WeaselSetup.exe" /s

echo 正在启动 WeaselServer...
start "" "%RIME_DIR%\WeaselServer.exe"

echo 完成！请切换到小狼毫输入法测试。
pause
