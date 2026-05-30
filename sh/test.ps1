# sh/test.ps1
# Compila y ejecuta los tests unitarios (doctest vía CTest).
#
# Uso:
#   .\sh\test.ps1
#   .\sh\test.ps1 -Config Release

param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Debug"
)

. "$PSScriptRoot\_config.ps1"

# Compilar el target de tests (configura solo si build/ no existe).
& "$PSScriptRoot\build.ps1" -Config $Config -Target efengine_tests
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ">> Ejecutando tests ($Config)" -ForegroundColor Cyan
Write-Host "----------------------------------------" -ForegroundColor DarkGray
ctest --test-dir $BuildDir -C $Config --output-on-failure
$code = $LASTEXITCODE
Write-Host "----------------------------------------" -ForegroundColor DarkGray
if ($code -eq 0) {
    Write-Host ">> Todos los tests pasaron." -ForegroundColor Green
} else {
    Write-Host ">> Tests fallaron (exit $code)" -ForegroundColor Red
}
exit $code
