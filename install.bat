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
    echo [X] 没找到 MinimalTSFIME.dll。
    echo     先编译工程，或直接把 DLL 完整路径作为第一个参数传给本脚本。
    pause
    exit /b 1
)

echo 注册: %DLL%
echo.
echo 注意：如果这是 32 位 DLL，请改用下面这条命令：
echo     %windir%\SysWOW64\regsvr32.exe "%DLL%"
echo.
regsvr32 "%DLL%"
echo.
echo 完成后请到"设置 - 时间和语言 - 语言和区域 - 中文(简体) - 语言选项 - 键盘"查看，
echo 必要时点"添加键盘"勾选"极简拼音 (TSF 示例)"。
pause
