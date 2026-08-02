@echo off

setlocal

set SCRIPT_DIR=%~dp0
if "%SCRIPT_DIR:~-1%"=="\" set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%

cd /d "%SCRIPT_DIR%"

if not exist env.bat copy env.bat.template env.bat

if exist env.bat call env.bat

if not defined WEASEL_ROOT set WEASEL_ROOT=%SCRIPT_DIR%

call version.bat

echo PRODUCT_VERSION=%PRODUCT_VERSION%
echo WEASEL_VERSION=%WEASEL_VERSION%
echo WEASEL_BUILD=%WEASEL_BUILD%
echo WEASEL_ROOT=%WEASEL_ROOT%
echo WEASEL_BUNDLED_RECIPES=%WEASEL_BUNDLED_RECIPES%
echo.

if defined GITHUB_ENV (
	setlocal enabledelayedexpansion
	echo git_ref_name=%PRODUCT_VERSION%>>!GITHUB_ENV!
)

if defined BOOST_ROOT (
  if exist "%BOOST_ROOT%\boost" goto boost_found
)
echo Error: Boost not found! Please set BOOST_ROOT in env.bat.
exit /b 1

:boost_found
echo BOOST_ROOT=%BOOST_ROOT%
echo.

if not defined BJAM_TOOLSET (
  rem the number actually means platform toolset, not %VisualStudioVersion%
  set BJAM_TOOLSET=msvc-14.5
)

if not defined PLATFORM_TOOLSET (
  rem VS 2026 = v145; VS 2022 = v143
  set PLATFORM_TOOLSET=v145
)

if defined DEVTOOLS_PATH set PATH=%DEVTOOLS_PATH%%PATH%

set MSBUILD_EXE=msbuild.exe
if exist "%ProgramFiles%\Microsoft Visual Studio\Community\MSBuild\Current\Bin\MSBuild.exe" set MSBUILD_EXE=%ProgramFiles%\Microsoft Visual Studio\Community\MSBuild\Current\Bin\MSBuild.exe

set build_config=Release
set build_option=/t:Build
set build_boost=0
set boost_build_variant=release
set build_data=0
set build_opencc=0
set build_rime=0
set rime_build_variant=release
set build_weasel=0
set build_installer=0
set build_arm64=0

rem parse the command line options
:parse_cmdline_options
  if "%1" == "" goto end_parsing_cmdline_options
  if "%1" == "debug" (
    set build_config=Debug
    set boost_build_variant=debug
    set rime_build_variant=debug
  )
  if "%1" == "release" (
    set build_config=Release
    set boost_build_variant=release
    set rime_build_variant=release
  )
  if "%1" == "rebuild" set build_option=/t:Rebuild
  if "%1" == "boost" set build_boost=1
  if "%1" == "data" set build_data=1
  if "%1" == "opencc" set build_opencc=1
  if "%1" == "rime" set build_rime=1
  if "%1" == "librime" set build_rime=1
  if "%1" == "weasel" set build_weasel=1
  if "%1" == "installer" set build_installer=1
  if "%1" == "arm64" set build_arm64=1
  if "%1" == "all" (
    set build_boost=1
    set build_data=1
    set build_opencc=1
    set build_rime=1
    set build_weasel=1
    set build_installer=1
    set build_arm64=1
  )
  shift
  goto parse_cmdline_options
:end_parsing_cmdline_options

if %build_weasel% == 0 (
if %build_boost% == 0 (
if %build_data% == 0 (
if %build_opencc% == 0 (
if %build_rime% == 0 (
  set build_weasel=1
)))))

if %build_weasel% == 1 (
  if not exist lib64\rime.lib (
    set build_rime=1
  )
  if not exist lib\rime.lib (
    set build_rime=1
  )
)

rem quit WeaselServer.exe before building
cd /d %WEASEL_ROOT%
if exist output\weaselserver.exe (
  output\weaselserver.exe /q
)

echo Stopping WeaselServer...
taskkill /f /im WeaselServer.exe 2>nul

rem build booost
if %build_boost% == 1 (
  call :setup_vs_env x86
  if errorlevel 1 exit /b 1
  call :build_boost
  if errorlevel 1 exit /b 1
  cd /d %WEASEL_ROOT%
)

rem -------------------------------------------------------------------------
rem build librime x64 and Win32
if %build_rime% == 1 (
  if not exist librime\build.bat (
    git submodule update --init --recursive
  )
  cd %WEASEL_ROOT%\librime
  rem clean cache before building
  for %%a in ( build dist lib ^
    build_x64 dist_x64 lib_x64 ^
    build_Win32 dist_Win32 lib_Win32 ^
    deps\glog\build ^
    deps\glog\build_x64 ^
    deps\glog\build_Win32 ^
    deps\googletest\build ^
    deps\googletest\build_x64 ^
    deps\googletest\build_Win32 ^
    deps\leveldb\build ^
    deps\leveldb\build_x64 ^
    deps\leveldb\build_Win32 ^
    deps\marisa-trie\build ^
    deps\marisa-trie\build_x64 ^
    deps\marisa-trie\build_Win32 ^
    deps\opencc\build ^
    deps\opencc\build_x64 ^
    deps\opencc\build_Win32 ^
    deps\yaml-cpp\build ^
    deps\yaml-cpp\build_x64 ^
    deps\yaml-cpp\build_Win32 ) do (
      if exist %%a rd /s /q %%a
  )

  rem build x64 librime
  set ARCH=x64
  call :build_librime_platform x64 %WEASEL_ROOT%\lib64 %WEASEL_ROOT%\output
  rem build Win32 librime
  set ARCH=Win32
  call :build_librime_platform Win32 %WEASEL_ROOT%\lib %WEASEL_ROOT%\output\Win32
  rem clean the modified file
  rem git checkout .
  rem git submodule foreach git checkout .
)

rem -------------------------------------------------------------------------
if %build_weasel% == 1 (
  if not exist output\data\essay.txt (
    set build_data=1
  )
  if not exist output\data\opencc\TSCharacters.ocd* (
    set build_opencc=1
  )
)
if %build_data% == 1 call :build_data
if %build_opencc% == 1 call :build_opencc_data

if %build_weasel% == 0 goto end

cd /d %WEASEL_ROOT%

set WEASEL_PROJECT_PROPERTIES=BOOST_ROOT^
  PLATFORM_TOOLSET^
  VERSION_MAJOR^
  VERSION_MINOR^
  VERSION_PATCH^
  PRODUCT_VERSION^
  FILE_VERSION

cscript.exe render.js weasel.props %WEASEL_PROJECT_PROPERTIES%

del msbuild*.log

if defined SDKVER set build_sdk_option=/p:WindowsTargetPlatformVersion=%SDKVER%
if not defined SDKVER set build_sdk_option=

if %build_arm64% == 1 (
  rem VS 2022 (v143) supports ARM/ARM64; VS 2026 (v145) does not
  if "%PLATFORM_TOOLSET%" geq "v145" (
    echo Platform toolset %PLATFORM_TOOLSET% does not support ARM/ARM64 builds, skipping.
  ) else (
    "%MSBUILD_EXE%" weasel.sln %build_option% /p:Configuration=%build_config% /p:Platform="ARM" /fl6 %build_sdk_option%
    if errorlevel 1 goto error
    "%MSBUILD_EXE%" weasel.sln %build_option% /p:Configuration=%build_config% /p:Platform="ARM64" /fl5 %build_sdk_option%
    if errorlevel 1 goto error
  )
)

"%MSBUILD_EXE%" weasel.sln %build_option% /p:Configuration=%build_config% /p:Platform="x64" /fl2 %build_sdk_option%
if errorlevel 1 goto error
"%MSBUILD_EXE%" weasel.sln %build_option% /p:Configuration=%build_config% /p:Platform="Win32" /fl1 %build_sdk_option%
if errorlevel 1 goto error

if %build_arm64% == 1 (
  rem VS 2022 (v143) supports ARM/ARM64; VS 2026 (v145) does not
  if "%PLATFORM_TOOLSET%" geq "v145" (
    echo Platform toolset %PLATFORM_TOOLSET% does not support ARM/ARM64 builds, skipping arm64x_wrapper.
  ) else (
    pushd arm64x_wrapper
    call build.bat
    if errorlevel 1 goto error
    popd

    copy arm64x_wrapper\weaselARM64X.dll output
    if errorlevel 1 goto error
  )
)

if %build_installer% == 1 (
  "%ProgramFiles(x86)%"\NSIS\Bin\makensis.exe ^
  /DWEASEL_VERSION=%WEASEL_VERSION% ^
  /DWEASEL_BUILD=%WEASEL_BUILD% ^
  /DPRODUCT_VERSION=%PRODUCT_VERSION% ^
  output\install.nsi
  if errorlevel 1 goto error
)

goto end

rem -------------------------------------------------------------------------
rem build boost
:build_boost
  set BJAM_OPTIONS_COMMON=-j%NUMBER_OF_PROCESSORS%^
    --with-filesystem^
    --with-json^
    --with-locale^
    --with-regex^
    --with-serialization^
    --with-thread^
    define=BOOST_USE_WINAPI_VERSION=0x0603^
    toolset=%BJAM_TOOLSET%^
    link=static^
    runtime-link=static^
    --build-type=complete
  
  set BJAM_OPTIONS_X86=%BJAM_OPTIONS_COMMON%^
    architecture=x86^
    address-model=32
  
  set BJAM_OPTIONS_X64=%BJAM_OPTIONS_COMMON%^
    architecture=x86^
    address-model=64
  
  set BJAM_OPTIONS_ARM32=%BJAM_OPTIONS_COMMON%^
    define=BOOST_USE_WINAPI_VERSION=0x0A00^
    architecture=arm^
    address-model=32
  
  set BJAM_OPTIONS_ARM64=%BJAM_OPTIONS_COMMON%^
    define=BOOST_USE_WINAPI_VERSION=0x0A00^
    architecture=arm^
    address-model=64
  
  cd /d %BOOST_ROOT%
  if not exist b2.exe call bootstrap.bat
  if errorlevel 1 goto error
  rem Delete any .obj left corrupted by previous failed 32-bit link.exe
  if exist "bin.v2\libs\serialization\build" (
    del /s /q "bin.v2\libs\serialization\build\*grammar*.obj" 2>nul
  )
  b2 %BJAM_OPTIONS_X86% stage %BOOST_COMPILED_LIBS%
  if errorlevel 1 (
    echo.
    echo Boost x86 build failed.  Cleaning corrupted intermediate files and retrying once...
    if exist "bin.v2\libs\serialization\build" (
      del /s /q "bin.v2\libs\serialization\build\*grammar*.obj" 2>nul
    )
    b2 %BJAM_OPTIONS_X86% stage %BOOST_COMPILED_LIBS%
    if errorlevel 1 goto error
  )
  b2 %BJAM_OPTIONS_X64% stage %BOOST_COMPILED_LIBS%
  if errorlevel 1 goto error
  
  if %build_arm64% == 1 (
    b2 %BJAM_OPTIONS_ARM32% stage %BOOST_COMPILED_LIBS%
    if errorlevel 1 goto error
    b2 %BJAM_OPTIONS_ARM64% stage %BOOST_COMPILED_LIBS%
    if errorlevel 1 goto error
  )
  exit /b

rem -------------------------------------------------------------------------
:setup_vs_env
  set VS_ARCH=%~1
  if "%VS_ARCH%"=="" set VS_ARCH=x64
  if exist "%ProgramFiles%\Microsoft Visual Studio\Community\Common7\Tools\VsDevCmd.bat" (
    call "%ProgramFiles%\Microsoft Visual Studio\Community\Common7\Tools\VsDevCmd.bat" -arch=%VS_ARCH% -host_arch=x64
    if errorlevel 1 exit /b 1
    exit /b 0
  )
  echo Error: VsDevCmd.bat not found.
  exit /b 1

rem ---------------------------------------------------------------------------
:build_data
  copy %WEASEL_ROOT%\LICENSE.txt output\
  copy %WEASEL_ROOT%\README.md output\README.txt
  copy %WEASEL_ROOT%\plum\rime-install.bat output\
  set plum_dir=plum
  set rime_dir=output/data
  set WSLENV=plum_dir:rime_dir
  bash plum/rime-install %WEASEL_BUNDLED_RECIPES%
  if errorlevel 1 goto error
  exit /b

rem ---------------------------------------------------------------------------
:build_opencc_data
  if not exist %WEASEL_ROOT%\librime\share\opencc\TSCharacters.ocd2 (
    cd %WEASEL_ROOT%\librime
    call build.bat deps %rime_build_variant%
    if errorlevel 1 goto error
  )
  cd %WEASEL_ROOT%
  if not exist output\data\opencc mkdir output\data\opencc
  copy %WEASEL_ROOT%\librime\share\opencc\*.* output\data\opencc\
  if errorlevel 1 goto error
  exit /b

rem ---------------------------------------------------------------------------
rem %1 : ARCH
rem %2 : push | pop , push to backup when pop to restore
:stash_build
  pushd %WEASEL_ROOT%\librime
  for %%a in ( build dist lib ^
    deps\glog\build ^
    deps\googletest\build ^
    deps\leveldb\build ^
    deps\marisa-trie\build ^
    deps\opencc\build ^
    deps\yaml-cpp\build ) do (
    if "%2"=="push" (
      if exist %%a  move %%a %%a_%1 
    )
    if "%2"=="pop" (
      if exist %%a_%1  move %%a_%1 %%a 
    )
  )
  popd
  exit /b

rem ---------------------------------------------------------------------------
rem %1 : ARCH
rem %2 : target_path of rime.lib, base %WEASEL_ROOT% or abs path
rem %3 : target_path of rime.dll, base %WEASEL_ROOT% or abs path
:build_librime_platform
  if /I "%1"=="x64" (
    call :setup_vs_env x64
  ) else (
    call :setup_vs_env x86
  )
  if errorlevel 1 goto error

  rem restore backuped %1 build
  call :stash_build %1 pop

  cd %WEASEL_ROOT%\librime
  set OLD_PLATFORM_TOOLSET=%PLATFORM_TOOLSET%
  set PLATFORM_TOOLSET=
  if not exist env.bat (
    copy %WEASEL_ROOT%\env.bat env.bat
  )
  if not exist lib\opencc.lib (
    call build.bat deps %rime_build_variant%
    if errorlevel 1 (
      set PLATFORM_TOOLSET=%OLD_PLATFORM_TOOLSET%
      call :stash_build %1 push
      goto error
    )
  )
  call build.bat %rime_build_variant%
  set PLATFORM_TOOLSET=%OLD_PLATFORM_TOOLSET%
  if errorlevel 1 (
    call :stash_build %1 push
    goto error
  )

  cd %WEASEL_ROOT%\librime
  call :stash_build %1 push

  copy /Y %WEASEL_ROOT%\librime\dist_%1\include\rime_*.h %WEASEL_ROOT%\include\
  if errorlevel 1 goto error
  copy /Y %WEASEL_ROOT%\librime\dist_%1\lib\rime.lib %2\
  if errorlevel 1 goto error
  copy /Y %WEASEL_ROOT%\librime\dist_%1\lib\rime.dll %3\
  if errorlevel 1 goto error

  exit /b
rem ---------------------------------------------------------------------------

:error

cd %WEASEL_ROOT%
echo error building weasel...
exit /b 1

:end
cd %WEASEL_ROOT%
