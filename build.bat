@echo off
setlocal

set "CFLAGS=-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow"
if not "%CFLAGS_OVERRIDE%"=="" set "CFLAGS=%CFLAGS_OVERRIDE%"
set "LDFLAGS=%LDFLAGS_OVERRIDE%"

set "CC=gcc"
if not "%CC_OVERRIDE%"=="" (
    set "CC=%CC_OVERRIDE%"
) else (
    where gcc >nul 2>&1
    if errorlevel 1 (
        if exist "%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\gcc.exe" (
            set "CC=%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\gcc.exe"
        ) else (
            if not "%MSYS2_ROOT_OVERRIDE%"=="" (
                set "CC=%MSYS2_ROOT_OVERRIDE%\mingw64\bin\gcc.exe"
            ) else (
                set "CC=C:\msys64\mingw64\bin\gcc.exe"
            )
        )
    )
)

"%CC%" --version >nul 2>&1
if errorlevel 1 (
    echo C11 compiler not found. Run: scripts\install_windows_toolchain.cmd
    echo Or set CC_OVERRIDE to the full path of gcc.exe.
    exit /b 9009
)

for %%I in ("%CC%") do set "CC_DIR=%%~dpI"
set "PATH=%CC_DIR%;%PATH%"

if not exist build mkdir build

if /I "%~1"=="test" goto test

%CC% %CFLAGS% -Iinclude main.c src\ml_error.c src\app_cli.c src\app_commands.c src\matrix.c src\linear_regression.c src\dataset.c src\logistic_regression.c src\classification_metrics.c src\cross_validation.c src\model_serialization.c src\feature_scaling.c src\regression_metrics.c %LDFLAGS% -lm -o build\cml-lab.exe
if errorlevel 1 exit /b %errorlevel%
echo Built build\cml-lab.exe
exit /b 0

:test
%CC% %CFLAGS% -Iinclude main.c src\ml_error.c src\app_cli.c src\app_commands.c src\matrix.c src\linear_regression.c src\dataset.c src\logistic_regression.c src\classification_metrics.c src\cross_validation.c src\model_serialization.c src\feature_scaling.c src\regression_metrics.c %LDFLAGS% -lm -o build\cml-lab.exe
if errorlevel 1 exit /b %errorlevel%
%CC% %CFLAGS% -Iinclude tests\test_main.c tests\test_ml_error.c tests\test_app_cli.c tests\test_matrix.c tests\test_linear_regression.c tests\test_dataset.c tests\test_logistic_regression.c tests\test_classification_metrics.c tests\test_model_serialization.c tests\test_feature_scaling.c tests\test_regression_metrics.c tests\test_cross_validation.c tests\test_regularization.c src\ml_error.c src\app_cli.c src\app_commands.c src\matrix.c src\linear_regression.c src\dataset.c src\logistic_regression.c src\classification_metrics.c src\cross_validation.c src\model_serialization.c src\feature_scaling.c src\regression_metrics.c %LDFLAGS% -lm -o build\test-foundation.exe
if errorlevel 1 exit /b %errorlevel%
build\test-foundation.exe
if errorlevel 1 exit /b %errorlevel%
python scripts\test_cli.py build\cml-lab.exe
if errorlevel 1 exit /b %errorlevel%
python scripts\validate_infrastructure.py
if errorlevel 1 exit /b %errorlevel%
exit /b 0
