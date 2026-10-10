@echo off
setlocal

winget install --id BrechtSanders.WinLibs.POSIX.UCRT --exact --accept-source-agreements --accept-package-agreements
if errorlevel 1 exit /b %errorlevel%

where gcc >nul 2>&1
if not errorlevel 1 (
    gcc --version
    exit /b %errorlevel%
)

set "GCC_PATH=%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\gcc.exe"
if not exist "%GCC_PATH%" (
    echo WinLibs installed, but gcc.exe was not found on PATH or at the expected WinGet package path.
    exit /b 1
)

"%GCC_PATH%" --version
if errorlevel 1 exit /b %errorlevel%
echo GCC is ready. Run build.bat test from the repository root.
exit /b 0
