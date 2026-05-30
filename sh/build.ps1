# sh/build.ps1
# Compila el proyecto. Configura automáticamente si build/ no existe.
#
# Uso:
#   .\sh\build.ps1              # Debug por defecto
#   .\sh\build.ps1 -Config Release
#   .\sh\build.ps1 -Target sandbox

param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Debug",
    [string]$Target = ""
)

. "$PSScriptRoot\_config.ps1"

# Configurar si hace falta
if (-not (Test-Path (Join-Path $BuildDir "CMakeCache.txt"))) {
    Write-Host ">> build/ sin configurar, ejecutando configure..." -ForegroundColor Yellow
    & "$PSScriptRoot\configure.ps1"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

Write-Host ">> Compilando ($Config)" -ForegroundColor Cyan

$buildArgs = @("--build", $BuildDir, "--config", $Config)
if ($Target -ne "") { $buildArgs += @("--target", $Target) }

cmake @buildArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host ">> Falló la compilación (exit $LASTEXITCODE)" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host ">> Compilación lista ($Config)." -ForegroundColor Green
