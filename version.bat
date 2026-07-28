@echo off
rem ============================================================
rem Single source of truth for Weasel version numbers.
rem Call this from build.bat / xbuild.bat / pack.bat.
rem Define VERSION_MAJOR / VERSION_MINOR / VERSION_PATCH
rem before calling to override defaults.
rem ============================================================

if not defined VERSION_MAJOR set VERSION_MAJOR=1
if not defined VERSION_MINOR set VERSION_MINOR=0
if not defined VERSION_PATCH set VERSION_PATCH=2
if not defined WEASEL_VERSION set WEASEL_VERSION=%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%
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
