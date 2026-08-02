@echo off
rem ============================================================
rem Single source of truth for Weasel version numbers.
rem Call this from build.bat / xbuild.bat / pack.bat.
rem VERSION_MAJOR / VERSION_MINOR / VERSION_PATCH must be
rem defined in env.bat before calling.
rem ============================================================

if not defined VERSION_MAJOR goto error
if not defined VERSION_MINOR goto error
if not defined VERSION_PATCH goto error
set WEASEL_VERSION=%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%
if not defined WEASEL_BUILD set WEASEL_BUILD=0

set PRODUCT_VERSION=
if not defined RELEASE_BUILD (
  git --version >nul 2>&1
  if not errorlevel 1 (
    for /f "delims=" %%i in ('git tag --sort=-creatordate ^| findstr /r "%WEASEL_VERSION%"') do (
      set LAST_TAG=%%i
      goto found_tag
    )
    :found_tag
    if defined LAST_TAG (
      for /f "delims=" %%i in ('git rev-list %LAST_TAG%..HEAD --count') do set WEASEL_BUILD=%%i
    )
    for /F %%i in ('git rev-parse --short HEAD') do set PRODUCT_VERSION=%WEASEL_VERSION%.%WEASEL_BUILD%.%%i
  )
)
if not defined PRODUCT_VERSION set PRODUCT_VERSION=%WEASEL_VERSION%.%WEASEL_BUILD%
if not defined FILE_VERSION set FILE_VERSION=%WEASEL_VERSION%.%WEASEL_BUILD%
goto :eof

:error
echo ERROR: VERSION_MAJOR / VERSION_MINOR / VERSION_PATCH not set.
echo Please define them in env.bat.
exit /b 1
