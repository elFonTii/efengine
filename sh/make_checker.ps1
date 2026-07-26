# sh/make_checker.ps1
# Genera assets/textures/default/checker.png: cuadricula en dos grises, usada
# como albedo del material "default"
#
# Uso:
#   .\sh\make_checker.ps1
#   .\sh\make_checker.ps1 -Size 1024 -Cells 16

param(
    [int]$Size  = 512,
    [int]$Cells = 8
)

. "$PSScriptRoot\_config.ps1"

Add-Type -AssemblyName System.Drawing

$outDir = Join-Path $RepoRoot "assets\textures\default"
if (-not (Test-Path $outDir)) { New-Item -ItemType Directory -Force $outDir | Out-Null }
$out = Join-Path $outDir "checker.png"

$bmp = New-Object System.Drawing.Bitmap($Size, $Size)
$g   = [System.Drawing.Graphics]::FromImage($bmp)

$claro = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(200, 200, 200))
$medio = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(110, 110, 110))
$cell  = [int]($Size / $Cells)

for ($y = 0; $y -lt $Cells; $y++) {
    for ($x = 0; $x -lt $Cells; $x++) {
        $brush = if ((($x + $y) % 2) -eq 0) { $claro } else { $medio }
        $g.FillRectangle($brush, $x * $cell, $y * $cell, $cell, $cell)
    }
}

$g.Dispose()
$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
$claro.Dispose()
$medio.Dispose()

Write-Host ">> Escrito $out ($Size x $Size, $Cells x $Cells celdas)" -ForegroundColor Green
