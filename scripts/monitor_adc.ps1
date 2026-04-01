param(
    [string]$Port,
    [int]$BaudRate = 115200,
    [double]$Vref = 3.0,
    [string]$CsvPath,
    [switch]$PassThru,
    [switch]$ListPorts
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-SerialPortDescriptions {
    $portDescriptions = @{}
    $ports = [System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object

    foreach ($portName in $ports) {
        $portDescriptions[$portName] = $null
    }

    try {
        $devices = Get-CimInstance Win32_PnPEntity | Where-Object { $_.Name -match '\(COM\d+\)' }
        foreach ($device in $devices) {
            if ($device.Name -match '\((COM\d+)\)') {
                $portDescriptions[$Matches[1]] = $device.Name
            }
        }
    } catch {
        # Leave descriptions empty if CIM lookup is unavailable.
    }

    return $portDescriptions
}

function Show-SerialPorts {
    $portDescriptions = Get-SerialPortDescriptions

    if ($portDescriptions.Count -eq 0) {
        Write-Host 'No serial ports detected.'
        return
    }

    foreach ($entry in $portDescriptions.GetEnumerator() | Sort-Object Name) {
        if ([string]::IsNullOrWhiteSpace($entry.Value)) {
            Write-Host $entry.Key
        } else {
            Write-Host ("{0}`t{1}" -f $entry.Key, $entry.Value)
        }
    }
}

function Resolve-SerialPort {
    param(
        [string]$RequestedPort
    )

    $portDescriptions = Get-SerialPortDescriptions
    $ports = @($portDescriptions.Keys | Sort-Object)

    if ($RequestedPort) {
        if ($ports -notcontains $RequestedPort) {
            throw "Serial port '$RequestedPort' was not found. Use -ListPorts to see available ports."
        }
        return $RequestedPort
    }

    if ($ports.Count -eq 0) {
        throw 'No serial ports detected. Connect the board and try again.'
    }

    if ($ports.Count -eq 1) {
        return $ports[0]
    }

    $preferredPattern = 'STM32|STLink|ST-LINK|USB Serial Device|Virtual COM Port'
    $preferredPorts = @(
        foreach ($portName in $ports) {
            $description = $portDescriptions[$portName]
            if (($null -ne $description) -and ($description -match $preferredPattern)) {
                $portName
            }
        }
    )

    if ($preferredPorts.Count -eq 1) {
        return $preferredPorts[0]
    }

    $availablePorts = ($ports -join ', ')
    throw "Multiple serial ports detected ($availablePorts). Specify one with -Port or use -ListPorts."
}

if ($ListPorts) {
    Show-SerialPorts
    exit 0
}

$selectedPort = Resolve-SerialPort -RequestedPort $Port

$serialPort = [System.IO.Ports.SerialPort]::new(
    $selectedPort,
    $BaudRate,
    [System.IO.Ports.Parity]::None,
    8,
    [System.IO.Ports.StopBits]::One
)
$serialPort.NewLine = "`r`n"
$serialPort.ReadTimeout = 500
$serialPort.DtrEnable = $false
$serialPort.RtsEnable = $false

$csvWriter = $null
if ($CsvPath) {
    $csvWriter = [System.IO.StreamWriter]::new($CsvPath, $true, [System.Text.Encoding]::ASCII)
    if ((Test-Path $CsvPath) -and ((Get-Item $CsvPath).Length -eq 0)) {
        $csvWriter.WriteLine('host_timestamp_utc,tick_ms,raw_sample,voltage_v')
        $csvWriter.Flush()
    }
}

$samplePattern = '^\[(?<tick>\d+)\]\s+[A-Z]\/ads7822:\s+sample=(?<sample>\d+)$'
$readFailPattern = '^\[(?<tick>\d+)\]\s+[A-Z]\/ads7822:\s+read failed$'

try {
    $serialPort.Open()
    $serialPort.DiscardInBuffer()

    Write-Host ("Monitoring ADS7822 output on {0} at {1} baud. Press Ctrl+C to stop." -f $selectedPort, $BaudRate)
    Write-Host ("Using Vref = {0:N3} V" -f $Vref)

    while ($true) {
        try {
            $line = $serialPort.ReadLine().TrimEnd()
        } catch [System.TimeoutException] {
            continue
        }

        if ([string]::IsNullOrWhiteSpace($line)) {
            continue
        }

        if ($PassThru) {
            Write-Host $line
        }

        if ($line -match $samplePattern) {
            $tickMs = [int]$Matches['tick']
            $rawSample = [int]$Matches['sample']
            $voltage = ($rawSample / 4095.0) * $Vref
            $timestamp = [DateTime]::UtcNow

            Write-Host (
                "{0:HH:mm:ss.fff}  tick={1,8} ms  raw={2,4}  voltage={3:N4} V" -f
                $timestamp.ToLocalTime(), $tickMs, $rawSample, $voltage
            )

            if ($null -ne $csvWriter) {
                $csvWriter.WriteLine(
                    "{0:o},{1},{2},{3:F6}" -f $timestamp, $tickMs, $rawSample, $voltage
                )
                $csvWriter.Flush()
            }

            continue
        }

        if ($line -match $readFailPattern) {
            Write-Warning ("ADS7822 read failed at tick {0} ms" -f [int]$Matches['tick'])
        }
    }
} finally {
    if ($null -ne $csvWriter) {
        $csvWriter.Dispose()
    }

    if ($serialPort.IsOpen) {
        $serialPort.Close()
    }

    $serialPort.Dispose()
}