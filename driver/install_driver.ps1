# ESP32-S3 CMSIS-DAP v2 Auto Driver Installation Script
# Requires Administrator privileges

param(
    [switch]$Silent
)

$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " ESP32-S3 CMSIS-DAP v2 Driver Installer" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check Administrator privileges
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "ERROR: Administrator privileges required!" -ForegroundColor Red
    Write-Host "Please right-click this script and select 'Run as Administrator'" -ForegroundColor Yellow
    if (-not $Silent) {
        Read-Host "Press Enter to exit"
    }
    exit 1
}

# 获取脚本目录
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$infFile = Join-Path $scriptDir "esp32s3_daplink.inf"

# Check if .inf file exists
if (-not (Test-Path $infFile)) {
    Write-Host "ERROR: esp32s3_daplink.inf file not found!" -ForegroundColor Red
    if (-not $Silent) {
        Read-Host "Press Enter to exit"
    }
    exit 1
}

Write-Host "Step 1/4: Installing WinUSB driver..." -ForegroundColor Green

try {
    $result = & pnputil.exe /add-driver $infFile /install 2>&1
    Write-Host "Driver installed successfully!" -ForegroundColor Green
} catch {
    Write-Host "Warning: Driver installation failed or already installed" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Step 2/4: Configuring device interface GUID..." -ForegroundColor Green

# Add device interface GUID to registry
$guid = "{CDB3B5AD-293B-4663-AA36-1AAE46463776}"
$devices = Get-PnpDevice | Where-Object { $_.InstanceId -like "*VID_0D28&PID_0204*" }

if ($devices) {
    foreach ($device in $devices) {
        $instanceId = $device.InstanceId
        $regPath = "HKLM:\SYSTEM\CurrentControlSet\Enum\$instanceId\Device Parameters"
        
        try {
            if (Test-Path $regPath) {
                Set-ItemProperty -Path $regPath -Name "DeviceInterfaceGUIDs" -Value $guid -Type MultiString -Force
                Write-Host "Registry configured for: $($device.FriendlyName)" -ForegroundColor Green
            }
        } catch {
            Write-Host "Warning: Failed to configure registry for $($device.FriendlyName)" -ForegroundColor Yellow
        }
    }
} else {
    Write-Host "Warning: Device not detected, please plug in ESP32-S3 CMSIS-DAP" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Step 3/4: Restarting device..." -ForegroundColor Green

if ($devices) {
    foreach ($device in $devices) {
        try {
            Disable-PnpDevice -InstanceId $device.InstanceId -Confirm:$false -ErrorAction SilentlyContinue
            Start-Sleep -Seconds 1
            Enable-PnpDevice -InstanceId $device.InstanceId -Confirm:$false -ErrorAction SilentlyContinue
            Write-Host "Device restarted: $($device.FriendlyName)" -ForegroundColor Green
        } catch {
            Write-Host "Warning: Failed to restart device" -ForegroundColor Yellow
        }
    }
}

Write-Host ""
Write-Host "Step 4/4: Verifying installation..." -ForegroundColor Green

Start-Sleep -Seconds 2
$devices = Get-PnpDevice | Where-Object { $_.InstanceId -like "*VID_0D28&PID_0204*" }

if ($devices) {
    Write-Host "Device found:" -ForegroundColor Green
    foreach ($device in $devices) {
        Write-Host "  - $($device.FriendlyName)" -ForegroundColor Cyan
        Write-Host "    Status: $($device.Status)" -ForegroundColor Gray
    }
} else {
    Write-Host "Warning: Device not detected" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Installation Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. Replug ESP32-S3 CMSIS-DAP device" -ForegroundColor White
Write-Host "2. Open Keil MDK" -ForegroundColor White
Write-Host "3. Select CMSIS-DAP Debugger in debug settings" -ForegroundColor White
Write-Host ""

if (-not $Silent) {
    Read-Host "Press Enter to exit"
}
