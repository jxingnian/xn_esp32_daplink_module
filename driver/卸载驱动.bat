@echo off
echo ========================================
echo  ESP32-S3 CMSIS-DAP v2 Driver Uninstaller
echo ========================================
echo.
echo Requesting administrator privileges...
echo.

PowerShell -NoProfile -ExecutionPolicy Bypass -Command "& {Start-Process PowerShell -ArgumentList '-NoProfile -ExecutionPolicy Bypass -NoExit -File ""%~dp0uninstall_driver.ps1""' -Verb RunAs -Wait}"

echo.
echo Uninstallation script completed.
echo.
pause
