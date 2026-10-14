@echo off
setlocal

rem Run ESP-IDF without requiring PowerShell script execution.
if not defined C61_IDF_PATH set "C61_IDF_PATH=%USERPROFILE%\esp\v5.5.5\esp-idf"
if not defined C61_TOOLS_PATH set "C61_TOOLS_PATH=%USERPROFILE%\.espressif-c61"
set "IDF_PATH=%C61_IDF_PATH%"
set "IDF_TOOLS_PATH=%C61_TOOLS_PATH%"
set "IDF_PYTHON_ENV_PATH=%IDF_TOOLS_PATH%\python_env\idf5.5_py3.11_env"

if not exist "%IDF_PATH%\export.bat" (
    echo ESP-IDF 5.5.5 was not found at "%IDF_PATH%".
    exit /b 1
)
if not exist "%IDF_PYTHON_ENV_PATH%\Scripts\python.exe" (
    echo ESP-IDF's Python environment was not found at "%IDF_PYTHON_ENV_PATH%".
    exit /b 1
)

set "PATH=%IDF_PYTHON_ENV_PATH%\Scripts;%PATH%"
call "%IDF_PATH%\export.bat"
if errorlevel 1 exit /b %errorlevel%

pushd "%~dp0"
if errorlevel 1 exit /b %errorlevel%
if /I "%~1"=="ports" (
    python -m serial.tools.list_ports -v
) else (
    python "%IDF_PATH%\tools\idf.py" %*
)
set "IDF_RESULT=%errorlevel%"
popd
exit /b %IDF_RESULT%
