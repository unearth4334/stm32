$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$pyScript = Join-Path $scriptDir "stm32_console.py"

$pythonCmd = $null
if (Get-Command python -ErrorAction SilentlyContinue) {
    $pythonCmd = "python"
} elseif (Get-Command py -ErrorAction SilentlyContinue) {
    $pythonCmd = "py -3"
} else {
    throw "Python interpreter not found (expected 'python' or 'py')."
}

if ($pythonCmd -eq "python") {
    & python $pyScript @args
} else {
    & py -3 $pyScript @args
}
