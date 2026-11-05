@echo off
chcp 65001 >nul
setlocal

net session >nul 2>&1
if errorlevel 1 (
    echo [X] 需要管理员权限：请右键本文件，选择"以管理员身份运行"。
    pause
    exit /b 1
)

set "DLL=%~1"
if "%DLL%"=="" set "DLL=%~dp0x64\Release\MinimalTSFIME.dll"
if not exist "%DLL%" set "DLL=%~dp0Win32\Release\MinimalTSFIME.dll"
if not exist "%DLL%" set "DLL=%~dp0MinimalTSFIME.dll"
if not exist "%DLL%" (
    echo [X] 没找到 MinimalTSFIME.dll，请把 DLL 完整路径作为参数传进来。
    pause
    exit /b 1
)

echo 反注册: %DLL%
regsvr32 /u "%DLL%"
echo.
echo 如仍有残留，注销或重启一次 Windows 即可。
pause
