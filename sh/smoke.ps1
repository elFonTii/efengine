# sh/smoke.ps1
# Arranca el sandbox N segundos, lo mata y busca errores en el log. Cubre lo que
# los tests headless no ven: shaders que no compilan, pases que no se crean,
# asserts al arrancar.
#
# Uso:
#   .\sh\smoke.ps1 -SaveBaseline                      # guarda los errores que ya existen
#   .\sh\smoke.ps1                                    # Debug, 60 s, arranque normal
#   .\sh\smoke.ps1 -SandboxArgs "--estres","4096"
#   .\sh\smoke.ps1 -SandboxArgs "--efe","assets/scenes/sandbox.efe"

param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Debug",
    [int]$Seconds = 60,
    [string[]]$SandboxArgs = @(),
    [switch]$SaveBaseline
)

. "$PSScriptRoot\_config.ps1"

$exe = Get-ExePath $Config
if (-not (Test-Path $exe)) {
    Write-Host ">> No se encontro el ejecutable: $exe (compilar antes)" -ForegroundColor Red
    exit 1
}

$out      = Join-Path $BuildDir "smoke_out.txt"
$err      = Join-Path $BuildDir "smoke_err.txt"
$baseline = Join-Path $BuildDir "smoke_baseline.txt"

Write-Host ">> Smoke: $exe $($SandboxArgs -join ' ') durante $Seconds s" -ForegroundColor Cyan
$startArgs = @{
    FilePath               = $exe
    WorkingDirectory       = $RepoRoot
    PassThru               = $true
    RedirectStandardOutput = $out
    RedirectStandardError  = $err
}
if ($SandboxArgs.Count -gt 0) { $startArgs.ArgumentList = $SandboxArgs }
$p = Start-Process @startArgs

Start-Sleep -Seconds $Seconds
$murio = $p.HasExited
if (-not $murio) { Stop-Process -Id $p.Id -Force }

$log = @()
if (Test-Path $out) { $log += Get-Content $out }
if (Test-Path $err) { $log += Get-Content $err }
$errores = @($log | Where-Object { $_ -match "ERROR|Assertion|ASSERT" })

if ($SaveBaseline) {
    Set-Content -Encoding utf8 -Path $baseline -Value $errores
    Write-Host ">> Baseline guardada: $($errores.Count) lineas en $baseline" -ForegroundColor Yellow
    exit 0
}

if (Test-Path $baseline) {
    $conocidos = @(Get-Content $baseline)
    $errores = @($errores | Where-Object { $conocidos -notcontains $_ })
}

if ($murio) { Write-Host ">> El sandbox termino solo antes de tiempo (exit $($p.ExitCode))" -ForegroundColor Red }
if ($errores.Count -gt 0) {
    Write-Host ">> Errores nuevos en el log:" -ForegroundColor Red
    $errores | ForEach-Object { Write-Host "   $_" }
}
if ($murio -or $errores.Count -gt 0) { exit 1 }
Write-Host ">> Smoke OK: sin errores nuevos en $Seconds s ($($log.Count) lineas)" -ForegroundColor Green
exit 0
