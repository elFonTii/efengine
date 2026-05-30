# sh/run.ps1
# Compila y ejecuta el sandbox.
#
# Uso:
#   .\sh\run.ps1
#   .\sh\run.ps1 -Config Release

param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Debug"
)

. "$PSScriptRoot\_config.ps1"

# Compilar primero
& "$PSScriptRoot\build.ps1" -Config $Config
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$exe = Get-ExePath $Config
if (-not (Test-Path $exe)) {
    Write-Host ">> No se encontró el ejecutable: $exe" -ForegroundColor Red
    exit 1
}

Write-Host ">> Ejecutando $exe" -ForegroundColor Cyan
Write-Host "----------------------------------------" -ForegroundColor DarkGray
& $exe
$code = $LASTEXITCODE
Write-Host "----------------------------------------" -ForegroundColor DarkGray
Write-Host ">> sandbox terminó con código $code" -ForegroundColor Green
exit $code
