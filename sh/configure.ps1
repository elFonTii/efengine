# sh/configure.ps1
# Genera los proyectos de Visual Studio en build/ (out-of-source).
#
# Uso:
#   .\sh\configure.ps1

. "$PSScriptRoot\_config.ps1"

Write-Host ">> Configurando en '$BuildDir' con '$Generator' ($Arch)" -ForegroundColor Cyan

cmake -G $Generator -A $Arch -S $RepoRoot -B $BuildDir
if ($LASTEXITCODE -ne 0) {
    Write-Host ">> Falló la configuración (exit $LASTEXITCODE)" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host ">> Configuración lista." -ForegroundColor Green
