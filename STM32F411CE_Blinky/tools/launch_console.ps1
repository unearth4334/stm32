$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$pyScript = Join-Path $scriptDir "stm32_console.py"

$executed = $false

if (Get-Command py -ErrorAction SilentlyContinue) {
    & py -3 $pyScript @args
    if ($LASTEXITCODE -ne 9009) {
        $executed = $true
    }
}

if (-not $executed -and (Get-Command python3 -ErrorAction SilentlyContinue)) {
    & python3 $pyScript @args
    $executed = $true
}

if (-not $executed -and (Get-Command python -ErrorAction SilentlyContinue)) {
    & python $pyScript @args
    $executed = $true
}

if (-not $executed) {
    throw "Python interpreter not found (expected 'py', 'python3', or 'python')."
}
