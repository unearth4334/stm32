param(
    [string]$PortName = "COM18",
    [int]$BaudRate = 115200,
    [string]$InaAddr = "0x41",
    [double]$ShuntOhms = 0.005,
    [double]$CurrentLimitA = 0.5
)

function Send-Ina232Command {
    param(
        [Parameter(Mandatory=$true)][System.IO.Ports.SerialPort]$Port,
        [Parameter(Mandatory=$true)][string]$Command,
        [int]$DelayMs = 450
    )

    $Port.WriteLine($Command)
    Start-Sleep -Milliseconds $DelayMs
    $raw = $Port.ReadExisting()
    return $raw
}

function Get-HexRegisterFromOutput {
    param(
        [string]$Output,
        [string]$FieldName
    )
    $pattern = [regex]::Escape($FieldName) + "=0x([0-9A-Fa-f]{1,4})"
    $m = [regex]::Match($Output, $pattern)
    if (-not $m.Success) {
        throw "Could not parse $FieldName from output:`n$Output"
    }
    return [Convert]::ToInt32($m.Groups[1].Value, 16)
}

$port = $null
try {
    $port = New-Object System.IO.Ports.SerialPort $PortName,$BaudRate,None,8,one
    $port.ReadTimeout = 1500
    $port.WriteTimeout = 1000
    $port.NewLine = "`r`n"
    $port.Open()

    Start-Sleep -Milliseconds 500
    $null = $port.ReadExisting()

    Write-Host "Connecting to INA232 at $InaAddr..."
    $out = Send-Ina232Command -Port $port -Command ("ina232 -connect " + $InaAddr)
    Write-Host $out

    Write-Host "Reading calibration register..."
    $outCal = Send-Ina232Command -Port $port -Command "ina232 -r calibration"
    Write-Host $outCal
    $cal = Get-HexRegisterFromOutput -Output $outCal -FieldName "calibration"

    # Current LSB from calibration and shunt:
    # I_LSB = 0.00512 / (cal * Rshunt)
    $currentLsbA = 0.00512 / ($cal * $ShuntOhms)

    # Expected current register code for target current (informational):
    $currentRaw = [math]::Round($CurrentLimitA / $currentLsbA)

    # For shunt-over alert, alert_limit is in shunt-voltage LSBs (2.5 uV/bit):
    # Vshunt_limit = I_limit * Rshunt
    # alert_raw = Vshunt_limit / 2.5uV = I_limit * Rshunt / 2.5e-6
    $alertRaw = [int][math]::Round(($CurrentLimitA * $ShuntOhms) / 2.5e-6)

    if ($alertRaw -lt 0 -or $alertRaw -gt 0xFFFF) {
        throw "Computed alert_limit out of range: $alertRaw"
    }

    $alertHex = ("0x{0:X4}" -f $alertRaw)

    Write-Host ""
    Write-Host "Computed values:"
    Write-Host ("  calibration     : 0x{0:X4} ({1})" -f $cal, $cal)
    Write-Host ("  current_lsb     : {0:F9} A/bit" -f $currentLsbA)
    Write-Host ("  current_raw@0.5A: 0x{0:X4} ({1}) [informational]" -f [int]$currentRaw, [int]$currentRaw)
    Write-Host ("  alert_limit raw : {0} ({1})" -f $alertHex, $alertRaw)
    Write-Host ""

    Write-Host "Programming alert_limit..."
    $out = Send-Ina232Command -Port $port -Command ("ina232 -w alert_limit " + $alertHex)
    Write-Host $out

    # 0x8001 = SOL (shunt over limit) + LEN (latch enable)
    Write-Host "Programming mask_enable (SOL + LEN)..."
    $out = Send-Ina232Command -Port $port -Command "ina232 -w mask_enable 0x8001"
    Write-Host $out

    Write-Host "Verifying registers..."
    $outAlert = Send-Ina232Command -Port $port -Command "ina232 -r alert_limit"
    $outMask  = Send-Ina232Command -Port $port -Command "ina232 -r mask_enable"
    Write-Host $outAlert
    Write-Host $outMask

    Write-Host "Optional live read:"
    Write-Host (Send-Ina232Command -Port $port -Command "ina232 -r current")
    Write-Host (Send-Ina232Command -Port $port -Command "ina232 -r shunt_voltage")

    Write-Host "Disconnecting..."
    Write-Host (Send-Ina232Command -Port $port -Command "ina232 -disconnect")
    Write-Host "Done."
}
catch {
    Write-Error $_.Exception.Message
}
finally {
    if ($port -and $port.IsOpen) { $port.Close() }
}