# ESP32-S3 CMSIS-DAP v2 Driver Uninstaller
# Requires Administrator privileges

$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " ESP32-S3 CMSIS-DAP v2 Driver Uninstaller" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check Administrator privileges
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "ERROR: Administrator privileges required!" -ForegroundColor Red
    Write-Host "Please right-click this script and select 'Run as Administrator'" -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host "Step 1/3: Finding devices..." -ForegroundColor Green

$devices = Get-PnpDevice | Where-Object { $_.InstanceId -like "*VID_0D28&PID_0204*" }

if ($devices) {
    Write-Host "Found devices:" -ForegroundColor Green
    foreach ($device in $devices) {
        Write-Host "  - $($device.FriendlyName)" -ForegroundColor Cyan
        Write-Host "    Instance ID: $($device.InstanceId)" -ForegroundColor Gray
    }
} else {
    Write-Host "No devices found" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Step 2/3: Removing devices..." -ForegroundColor Green

if ($devices) {
    foreach ($device in $devices) {
        try {
            # Remove device
            $instanceId = $device.InstanceId
            & pnputil.exe /remove-device $instanceId 2>&1 | Out-Null
            Write-Host "Device removed: $($device.FriendlyName)" -ForegroundColor Green
        } catch {
            Write-Host "Warning: Failed to remove device" -ForegroundColor Yellow
        }
    }
}

Write-Host ""
Write-Host "Step 3/3: Removing driver package..." -ForegroundColor Green

# Find and remove driver package
$driverPackages = & pnputil.exe /enum-drivers | Select-String -Pattern "esp32s3_daplink.inf" -Context 1,0

if ($driverPackages) {
    foreach ($match in $driverPackages) {
        $oem = $match.Context.PreContext | Select-String -Pattern "oem\d+\.inf"
        if ($oem) {
            $oemFile = $oem.Matches[0].Value
            try {
                & pnputil.exe /delete-driver $oemFile /uninstall /force 2>&1 | Out-Null
                Write-Host "Driver package removed: $oemFile" -ForegroundColor Green
            } catch {
                Write-Host "Warning: Failed to remove driver package" -ForegroundColor Yellow
            }
        }
    }
} else {
    Write-Host "No driver package found" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Step 4/3: Cleaning registry..." -ForegroundColor Green

# Clean registry GUID
$guid = "{CDB3B5AD-293B-4663-AA36-1AAE46463776}"
$regBasePath = "HKLM:\SYSTEM\CurrentControlSet\Enum\USB\VID_0D28&PID_0204"

if (Test-Path $regBasePath) {
    Get-ChildItem -Path $regBasePath | ForEach-Object {
        $deviceParamPath = Join-Path $_.PSPath "Device Parameters"
        if (Test-Path $deviceParamPath) {
            try {
                Remove-ItemProperty -Path $deviceParamPath -Name "DeviceInterfaceGUIDs" -ErrorAction SilentlyContinue
                Write-Host "Registry cleaned for: $($_.PSChildName)" -ForegroundColor Green
            } catch {
                # Ignore errors
            }
        }
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Uninstallation Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. Replug ESP32-S3 CMSIS-DAP device" -ForegroundColor White
Write-Host "2. Test automatic driver installation (Scheme 1)" -ForegroundColor White
Write-Host ""

Read-Host "Press Enter to exit"
