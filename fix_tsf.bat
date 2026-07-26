@echo off
chcp 65001 >nul
cd /d "%~dp0"

echo 正在停止 WeaselServer...
taskkill /f /im WeaselServer.exe 2>nul

echo 正在复制新 WeaselSetup.exe...
copy /Y output\WeaselSetup.exe "C:\Program Files\Rime\weasel-1.0.0\WeaselSetup.exe"

echo 正在注册 TSF 输入法...
"C:\Program Files\Rime\weasel-1.0.0\WeaselSetup.exe" /s

echo 正在启动 WeaselServer...
start "" "C:\Program Files\Rime\weasel-1.0.0\WeaselServer.exe"

echo 完成！请切换到小狼毫输入法测试。
pause
