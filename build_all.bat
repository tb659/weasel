@echo off

:: Find Visual Studio installation via vswhere
set VS_WHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
if not exist "%VS_WHERE%" set VS_WHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe
if not exist "%VS_WHERE%" (
  echo Error: vswhere.exe not found. Cannot locate Visual Studio.
  exit /b 1
)

for /f "tokens=*" %%i in ('"%VS_WHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath') do set VS_INSTALL_DIR=%%i
if not defined VS_INSTALL_DIR (
  echo Error: Visual Studio installation not found.
  exit /b 1
)

set VCCMD=%VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat
if not exist "%VCCMD%" (
  echo Error: vcvars64.bat not found at %VCCMD%
  exit /b 1
)

call "%VCCMD%"
cd /d %~dp0

:: Kill WeaselServer before building to avoid file lock
echo Stopping WeaselServer...
taskkill /f /im WeaselServer.exe 2>nul

:: Build x64 Release (skip rime - already built)
msbuild weasel.sln /t:Rebuild /p:Configuration=Release /p:Platform=x64 /p:PreferredToolArchitecture=x64
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo ALL_BUILD_OK
exit /b 0