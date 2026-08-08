@echo off

echo Stopping WeaselServer...
taskkill /f /im WeaselServer.exe 2>nul

set START_FILE=%TEMP%\weasel_build_start.txt
powershell -NoProfile -Command "(Get-Date).ToString('o') | Set-Content -Encoding ascii '%START_FILE%'"

set SCRIPT_DIR=%~dp0
if "%SCRIPT_DIR:~-1%"=="\" set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%
cd /d "%SCRIPT_DIR%" || ( echo Cannot cd to %SCRIPT_DIR% & call :show_elapsed & pause & exit /b 1 )

if not exist env.bat ( echo ERROR: env.bat not found & call :show_elapsed & pause & exit /b 1 )
call env.bat

call version.bat

echo ========================================
echo  Weasel Pack Script v%WEASEL_VERSION%
echo ========================================
echo.
echo Version: %WEASEL_VERSION%  Build: %WEASEL_BUILD% & echo.

if not defined BOOST_ROOT ( echo ERROR: BOOST_ROOT not set & call :show_elapsed & pause & exit /b 1 )
if not exist "%BOOST_ROOT%\boost" ( echo ERROR: Boost not found at %BOOST_ROOT% & call :show_elapsed & pause & exit /b 1 )
echo BOOST_ROOT=%BOOST_ROOT% & echo.

rem --- locate VS ---
set VS_WHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
if not exist "%VS_WHERE%" set VS_WHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe
if not exist "%VS_WHERE%" ( echo ERROR: vswhere.exe not found & call :show_elapsed & pause & exit /b 1 )
for /f "usebackq tokens=*" %%i in (`"%VS_WHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VS_INSTALL_DIR=%%i
if not defined VS_INSTALL_DIR ( echo ERROR: Visual Studio not found & call :show_elapsed & pause & exit /b 1 )
set VARS_BAT=%VS_INSTALL_DIR%\Common7\Tools\VsDevCmd.bat
if not exist "%VARS_BAT%" ( echo ERROR: VsDevCmd.bat not found & call :show_elapsed & pause & exit /b 1 )
echo Visual Studio: %VS_INSTALL_DIR% & echo.

setlocal enabledelayedexpansion

set CL=/FS

set STASH=build dist deps\glog\build deps\googletest\build deps\leveldb\build deps\marisa-trie\build deps\opencc\build deps\yaml-cpp\build

for %%d in (%STASH%) do (
  if exist "%SCRIPT_DIR%\librime\%%d_%1" (
    if exist "%SCRIPT_DIR%\librime\%%d" rmdir /s /q "%SCRIPT_DIR%\librime\%%d"
    move "%SCRIPT_DIR%\librime\%%d_%1" "%SCRIPT_DIR%\librime\%%d" >nul
  )
)
if exist "%SCRIPT_DIR%\librime\build" rmdir /s /q "%SCRIPT_DIR%\librime\build"
if exist "%SCRIPT_DIR%\librime\dist" rmdir /s /q "%SCRIPT_DIR%\librime\dist"

echo [1/4] librime x64 ...
cd /d "%SCRIPT_DIR%\librime"
for %%d in (%STASH%) do (
  if exist "%SCRIPT_DIR%\librime\%%d_x64" (
    if exist "%SCRIPT_DIR%\librime\%%d" rmdir /s /q "%SCRIPT_DIR%\librime\%%d"
    move "%SCRIPT_DIR%\librime\%%d_x64" "%SCRIPT_DIR%\librime\%%d" >nul
  )
)
if exist build rmdir /s /q build
if exist dist rmdir /s /q dist
if exist lib rmdir /s /q lib

call "%VARS_BAT%" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 ( echo VsDevCmd x64 failed & call :show_elapsed & pause & exit /b 1 )

call build.bat deps release
if errorlevel 1 ( echo librime deps x64 FAILED & call :show_elapsed & pause & exit /b 1 )
call build.bat release
if errorlevel 1 ( echo librime x64 build FAILED & call :show_elapsed & pause & exit /b 1 )

for %%d in (%STASH%) do (
  if exist "%SCRIPT_DIR%\librime\%%d" (
    if exist "%SCRIPT_DIR%\librime\%%d_x64" rmdir /s /q "%SCRIPT_DIR%\librime\%%d_x64"
    move "%SCRIPT_DIR%\librime\%%d" "%SCRIPT_DIR%\librime\%%d_x64" >nul
  )
)

copy /Y "%SCRIPT_DIR%\librime\dist_x64\include\rime_*.h" "%SCRIPT_DIR%\include\" >nul 2>nul
copy /Y "%SCRIPT_DIR%\librime\dist\include\rime_*.h" "%SCRIPT_DIR%\include\" >nul 2>nul
if not exist "%SCRIPT_DIR%\lib64" mkdir "%SCRIPT_DIR%\lib64"
copy /Y "%SCRIPT_DIR%\librime\dist_x64\lib\rime.lib" "%SCRIPT_DIR%\lib64\" >nul 2>nul || copy /Y "%SCRIPT_DIR%\librime\dist\lib\rime.lib" "%SCRIPT_DIR%\lib64\" >nul
copy /Y "%SCRIPT_DIR%\librime\dist_x64\lib\rime.dll" "%SCRIPT_DIR%\output\" >nul 2>nul || copy /Y "%SCRIPT_DIR%\librime\dist\lib\rime.dll" "%SCRIPT_DIR%\output\" >nul
echo [1/4] librime x64 done
echo.

echo [2/4] librime Win32 ...
cd /d "%SCRIPT_DIR%\librime"
for %%d in (%STASH%) do (
  if exist "%SCRIPT_DIR%\librime\%%d_Win32" (
    if exist "%SCRIPT_DIR%\librime\%%d" rmdir /s /q "%SCRIPT_DIR%\librime\%%d"
    move "%SCRIPT_DIR%\librime\%%d_Win32" "%SCRIPT_DIR%\librime\%%d" >nul
  )
)
if exist build rmdir /s /q build
if exist dist rmdir /s /q dist
if exist lib rmdir /s /q lib

call "%VARS_BAT%" -arch=x86 -host_arch=x64 >nul
if errorlevel 1 ( echo VsDevCmd x86 failed & call :show_elapsed & pause & exit /b 1 )

call build.bat deps release
if errorlevel 1 ( echo librime deps Win32 FAILED & call :show_elapsed & pause & exit /b 1 )
call build.bat release
if errorlevel 1 ( echo librime Win32 build FAILED & call :show_elapsed & pause & exit /b 1 )

for %%d in (%STASH%) do (
  if exist "%SCRIPT_DIR%\librime\%%d" (
    if exist "%SCRIPT_DIR%\librime\%%d_Win32" rmdir /s /q "%SCRIPT_DIR%\librime\%%d_Win32"
    move "%SCRIPT_DIR%\librime\%%d" "%SCRIPT_DIR%\librime\%%d_Win32" >nul
  )
)

if not exist "%SCRIPT_DIR%\lib" mkdir "%SCRIPT_DIR%\lib"
if not exist "%SCRIPT_DIR%\output\Win32" mkdir "%SCRIPT_DIR%\output\Win32"
copy /Y "%SCRIPT_DIR%\librime\dist_Win32\lib\rime.lib" "%SCRIPT_DIR%\lib\" >nul 2>nul || copy /Y "%SCRIPT_DIR%\librime\dist\lib\rime.lib" "%SCRIPT_DIR%\lib\" >nul
copy /Y "%SCRIPT_DIR%\librime\dist_Win32\lib\rime.dll" "%SCRIPT_DIR%\output\Win32\" >nul 2>nul || copy /Y "%SCRIPT_DIR%\librime\dist\lib\rime.dll" "%SCRIPT_DIR%\output\Win32\" >nul
echo [2/4] librime Win32 done
echo.

echo [3/4] Weasel solution ...

set CL=/FS

call "%VARS_BAT%" -arch=x64 -host_arch=x64 >nul
cd /d "%SCRIPT_DIR%"

cscript.exe render.js weasel.props BOOST_ROOT PLATFORM_TOOLSET VERSION_MAJOR VERSION_MINOR VERSION_PATCH PRODUCT_VERSION FILE_VERSION >nul

:: Kill WeaselServer before x64 build
:kill_x64
net stop WeaselInputService 2>nul
taskkill /f /im WeaselServer.exe 2>nul
timeout /t 1 /nobreak >nul
del /f /q output\WeaselServer.exe 2>nul
if exist output\WeaselServer.exe (
  powershell -Command "Stop-Process -Force -Name WeaselServer -ErrorAction SilentlyContinue; Start-Sleep 1; Remove-Item 'output\WeaselServer.exe' -Force -ErrorAction SilentlyContinue" >nul
  if exist output\WeaselServer.exe goto kill_x64
)

msbuild weasel.sln /t:Build /p:Configuration=Release /p:Platform=x64 /fl2 /m
if errorlevel 1 ( echo Weasel x64 build FAILED & call :show_elapsed & pause & exit /b 1 )

:: Kill WeaselServer before Win32 build
:kill_Win32
net stop WeaselInputService 2>nul
taskkill /f /im WeaselServer.exe 2>nul
timeout /t 1 /nobreak >nul
del /f /q output\Win32\WeaselServer.exe 2>nul
if exist output\Win32\WeaselServer.exe (
  powershell -Command "Stop-Process -Force -Name WeaselServer -ErrorAction SilentlyContinue; Start-Sleep 1; Remove-Item 'output\Win32\WeaselServer.exe' -Force -ErrorAction SilentlyContinue" >nul
  if exist output\Win32\WeaselServer.exe goto kill_Win32
)

msbuild weasel.sln /t:Build /p:Configuration=Release /p:Platform=Win32 /fl1 /m
if errorlevel 1 ( echo Weasel Win32 build FAILED & call :show_elapsed & pause & exit /b 1 )
echo [3/4] Weasel build done
echo.

echo [4/4] NSIS installer ...
cd /d "%SCRIPT_DIR%"
if not exist output\archives mkdir output\archives
copy /Y "%SCRIPT_DIR%\LICENSE.txt" "%SCRIPT_DIR%\output\" >nul
copy /Y "%SCRIPT_DIR%\README.md" "%SCRIPT_DIR%\output\README.txt" >nul
copy /Y "%SCRIPT_DIR%\plum\rime-install.bat" "%SCRIPT_DIR%\output\" >nul
if not exist "%SCRIPT_DIR%\output\data\opencc" mkdir "%SCRIPT_DIR%\output\data\opencc"
copy /Y "%SCRIPT_DIR%\librime\share\opencc\*.*" "%SCRIPT_DIR%\output\data\opencc\" >nul

if not defined PROGRAMFILES_X86 set PROGRAMFILES_X86=%ProgramFiles(x86)%
"%PROGRAMFILES_X86%\NSIS\Bin\makensis.exe" /DWEASEL_VERSION=%WEASEL_VERSION% /DWEASEL_BUILD=%WEASEL_BUILD% /DPRODUCT_VERSION=%PRODUCT_VERSION% output\install.nsi
if errorlevel 1 ( echo NSIS installer FAILED & call :show_elapsed & pause & exit /b 1 )

echo.
echo ========================================
echo  SUCCESS!
echo  Installer: output\archives\weasel-%PRODUCT_VERSION%-installer.exe
echo ========================================
call :show_elapsed
pause
exit /b 0

rem ---------------------------------------------------------------------------
:show_elapsed
  for /f "usebackq" %%i in (`powershell -NoProfile -Command "$s=[datetime](Get-Content '%START_FILE%'); $d=(Get-Date)-$s; if($d.TotalSeconds -lt 0){$d=$d.Add([timespan]::FromHours(24))}; '{0:hh\:mm\:ss}' -f $d"`) do set _ELAPSED=%%i
  echo.
  echo Build finished, total time: %_ELAPSED%
  exit /b
