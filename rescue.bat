@echo off
chcp 65001 >nul
:: 急救卸载：即使 DLL 已被删除 / 损坏 / 被占用，也能把注册表痕迹清干净。
:: 与 uninstall.bat 的区别：不依赖 DLL 能正常加载。
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0rescue.ps1"
echo.
pause
