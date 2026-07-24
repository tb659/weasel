@echo off
setlocal

set REPO_DIR=C:\dev\workspace\android\ime\weasel
set SRC_DIR=%REPO_DIR%\output
set DST_DIR=C:\Program Files\Rime\weasel-0.17.4
set X64_DLL=%SRC_DIR%\weaselx64.dll

if exist "%REPO_DIR%\output_alt5\weaselx64_zwsp5.dll" set X64_DLL=%REPO_DIR%\output_alt5\weaselx64_zwsp5.dll
if exist "%REPO_DIR%\output_alt4\weaselx64_pkg4.dll" set X64_DLL=%REPO_DIR%\output_alt4\weaselx64_pkg4.dll
if exist "%REPO_DIR%\output_alt3\weaselx64_predict3.dll" set X64_DLL=%REPO_DIR%\output_alt3\weaselx64_predict3.dll

net session >nul 2>nul
if not %errorlevel% == 0 (
  echo Please run this script as Administrator.
  pause
  exit /b 1
)

if not exist "%SRC_DIR%" (
  echo Source directory not found: %SRC_DIR%
  pause
  exit /b 1
)

if not exist "%DST_DIR%" (
  echo Destination directory not found: %DST_DIR%
  pause
  exit /b 1
)

echo Stopping WeaselServer...
taskkill /F /IM WeaselServer.exe >nul 2>nul

echo Copying updated files...
xcopy /E /I /Y "%SRC_DIR%\*" "%DST_DIR%\"
if errorlevel 1 (
  echo Copy failed.
  pause
  exit /b 1
)

if exist "%X64_DLL%" (
  echo Copying x64 TSF DLL from: %X64_DLL%
  copy /Y "%X64_DLL%" "%DST_DIR%\weaselx64.dll" >nul
  if errorlevel 1 (
    echo Failed to copy x64 TSF DLL.
    pause
    exit /b 1
  )
)

echo Running installer script...
cd /d "%DST_DIR%"
call install.bat /s
if errorlevel 1 (
  echo install.bat failed.
  pause
  exit /b 1
)

echo.
echo Checking installed file timestamps...
powershell -NoProfile -Command "Get-Item '%DST_DIR%\WeaselServer.exe','%DST_DIR%\rime.dll','%DST_DIR%\weaselx64.dll' | Select-Object Name,Length,LastWriteTime | Format-Table -AutoSize"
if errorlevel 1 (
  echo Failed to read installed files.
  pause
  exit /b 1
)

echo.
echo Checking WeaselServer process...
powershell -NoProfile -Command "Get-Process WeaselServer -ErrorAction SilentlyContinue | Select-Object Id,ProcessName,StartTime | Format-Table -AutoSize"

echo.
echo Done.
pause
exit /b 0
