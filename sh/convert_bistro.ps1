# sh/convert_bistro.ps1
# Convierte las texturas DDS del Bistro a PNG, que es lo unico que lee stb_image.
#
# Dos pasadas porque los normales son BC5 (solo guardan X e Y) y necesitan
# -reconstructz: sin eso el azul sale en 0, pbr.frag lee n.z = -1 y la
# iluminacion se rompe entera.
#
# Uso:
#   .\sh\convert_bistro.ps1
#   .\sh\convert_bistro.ps1 -Size 2048

param(
    [int]$Size = 1024
)

. "$PSScriptRoot\_config.ps1"

$texconv = Get-Command texconv.exe -ErrorAction SilentlyContinue
if (-not $texconv) {
    Write-Host ">> No se encontro texconv.exe en el PATH." -ForegroundColor Red
    Write-Host "   Bajalo de https://github.com/microsoft/DirectXTex/releases" -ForegroundColor Yellow
    exit 1
}

$dir = Join-Path $RepoRoot "assets\bistro\Textures"
if (-not (Test-Path $dir)) {
    Write-Host ">> No existe '$dir'." -ForegroundColor Red
    exit 1
}

# Los .tga quedan afuera: el FBX no referencia ninguno (las 405 rutas que
# declara son todas .dds), 8 de los 11 son duplicados de un .dds que ya existe,
# y stbi_load lee TGA nativo.
$normales = @(Get-ChildItem -Path $dir -Filter "*_Normal.dds")
$resto    = @(Get-ChildItem -Path $dir -Filter "*.dds" | Where-Object { $_.Name -notlike "*_Normal.dds" })

Write-Host ">> $($resto.Count) mapas de color/specular/emissive y $($normales.Count) normales a ${Size}px" -ForegroundColor Cyan

# Pasada 1: BaseColor, Specular, Emissive.
if ($resto.Count -gt 0) {
    & texconv -f R8G8B8A8_UNORM -ft png -w $Size -h $Size -y -o $dir $resto.FullName
    if ($LASTEXITCODE -ne 0) { Write-Host ">> Fallo la pasada 1" -ForegroundColor Red; exit $LASTEXITCODE }
}

# Pasada 2: normales BC5, con Z reconstruida.
if ($normales.Count -gt 0) {
    & texconv -f R8G8B8A8_UNORM -ft png -w $Size -h $Size -y -reconstructz -o $dir $normales.FullName
    if ($LASTEXITCODE -ne 0) { Write-Host ">> Fallo la pasada 2" -ForegroundColor Red; exit $LASTEXITCODE }
}

$png = @(Get-ChildItem -Path $dir -Filter "*.png").Count
Write-Host ">> Listo: $png PNG en '$dir'" -ForegroundColor Green
