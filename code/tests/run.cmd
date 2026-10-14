@echo off
setlocal
rem Host parser tests using Visual Studio's C compiler and AddressSanitizer.
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo Visual Studio C tools were not found.
    exit /b 1
)
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_PATH=%%i"
if not defined VS_PATH exit /b 1
call "%VS_PATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b %errorlevel%
pushd "%~dp0.."
if not exist "build\host-tests" mkdir "build\host-tests"
cl /nologo /std:c11 /W3 /Zi /fsanitize=address /D_CRT_SECURE_NO_WARNINGS /I main /I tests /I components\opendroneid /FI msvc_compat.h tests\test_rid.c main\rid_protocol.c components\opendroneid\opendroneid.c /Fe:build\host-tests\test_rid.exe /Fo:build\host-tests\ /Fd:build\host-tests\test_rid.pdb /link /INCREMENTAL:NO
if errorlevel 1 (
    popd
    exit /b 1
)
build\host-tests\test_rid.exe
set "TEST_RESULT=%errorlevel%"
popd
exit /b %TEST_RESULT%
