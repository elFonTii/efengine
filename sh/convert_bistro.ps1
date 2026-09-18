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

# Primero tools/ del repo, despues el PATH. Asi no hace falta instalar nada a
# nivel sistema: tools/texconv.exe esta gitignored por la regla *.exe.
$local = Join-Path $RepoRoot "tools\texconv.exe"
if (Test-Path $local) {
    $texconv = $local
} else {
    $cmd = Get-Command texconv.exe -ErrorAction SilentlyContinue
    if (-not $cmd) {
        Write-Host ">> No se encontro texconv.exe ni en tools/ ni en el PATH." -ForegroundColor Red
        Write-Host "   Bajalo de https://github.com/microsoft/DirectXTex/releases" -ForegroundColor Yellow
        Write-Host "   y dejalo en '$local'." -ForegroundColor Yellow
        exit 1
    }
    $texconv = $cmd.Source
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
$specular = @(Get-ChildItem -Path $dir -Filter "*_Specular.dds")
$color    = @(Get-ChildItem -Path $dir -Filter "*.dds" |
              Where-Object { $_.Name -notlike "*_Normal.dds" -and $_.Name -notlike "*_Specular.dds" })

Write-Host ">> $($color.Count) color/emissive, $($specular.Count) specular (x2) y $($normales.Count) normales a ${Size}px" -ForegroundColor Cyan

# En lotes: 400 rutas completas pasan del limite de linea de comandos de Windows
# (~32 KB) y texconv ni arranca.
function Invoke-Texconv {
    param(
        [string]   $Titulo,
        [object[]] $Archivos,
        [string[]] $Extra = @()
    )

    if ($Archivos.Count -eq 0) { return }

    $lote = 40
    for ($i = 0; $i -lt $Archivos.Count; $i += $lote) {
        $fin   = [Math]::Min($i + $lote, $Archivos.Count) - 1
        $rutas = $Archivos[$i..$fin].FullName

        & $texconv -f R8G8B8A8_UNORM -ft png -w $Size -h $Size -y @Extra -o $dir $rutas | Out-Null
        if ($LASTEXITCODE -ne 0) {
            Write-Host ">> Fallo $Titulo en el lote que arranca en $i" -ForegroundColor Red
            exit $LASTEXITCODE
        }
        Write-Host ("   $Titulo {0}/{1}" -f ($fin + 1), $Archivos.Count) -ForegroundColor DarkGray
    }
}

# Pasada 1: BaseColor y Emissive, tal cual.
Invoke-Texconv -Titulo "color" -Archivos $color

# Pasadas 2 y 3: el Specular del Bistro es ORM empaquetado (R=AO sin usar,
# G=roughness, B=metallic), pero pbr.frag lee .r en los tres slots. Asi que
# sale desempaquetado en dos archivos, cada canal llevado al rojo.
#   X_Specular.dds -> X_Specular.png          (roughness, desde G)
#                  -> X_Specular_Metallic.png (metallic,  desde B)
Invoke-Texconv -Titulo "roughness" -Archivos $specular -Extra @("--swizzle", "ggg1")
Invoke-Texconv -Titulo "metallic"  -Archivos $specular -Extra @("--swizzle", "bbb1", "-sx", "_Metallic")

# Pasada 4: normales BC5, con Z reconstruida.
Invoke-Texconv -Titulo "normales" -Archivos $normales -Extra @("--reconstruct-z")

$png = @(Get-ChildItem -Path $dir -Filter "*.png").Count
Write-Host ">> Listo: $png PNG en '$dir'" -ForegroundColor Green
