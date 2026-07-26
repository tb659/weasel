@echo off
call "C:\Program Files\Microsoft Visual Studio\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /d C:\dev\workspace\android\ime\weasel

:: Build x64 Release (skip rime - already built)
msbuild weasel.sln /t:Rebuild /p:Configuration=Release /p:Platform=x64 /p:PreferredToolArchitecture=x64 /p:SolutionDir=C:\dev\workspace\android\ime\weasel\
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo ALL_BUILD_OK
exit /b 0
