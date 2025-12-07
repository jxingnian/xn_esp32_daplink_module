@echo off
echo ========================================
echo  ESP32-S3 CMSIS-DAP v2 Driver Installer
echo ========================================
echo.
echo Requesting administrator privileges...
echo.

PowerShell -NoProfile -ExecutionPolicy Bypass -Command "& {Start-Process PowerShell -ArgumentList '-NoProfile -ExecutionPolicy Bypass -NoExit -File ""%~dp0install_driver.ps1""' -Verb RunAs -Wait}"

echo.
echo Installation script completed.
echo.
pause
