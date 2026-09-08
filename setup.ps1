# Run from PowerShell: .\setup.ps1 --qt-prefix C:/Qt/6.8.3/msvc2022_64 --package
$ErrorActionPreference = "Stop"
if (Get-Command py -ErrorAction SilentlyContinue) {
    & py -3 -B (Join-Path $PSScriptRoot "tools/setup.py") @args
} else {
    & python -B (Join-Path $PSScriptRoot "tools/setup.py") @args
}
exit $LASTEXITCODE
