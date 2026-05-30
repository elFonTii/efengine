# sh/clean.ps1
# Elimina la carpeta build/ y los artefactos in-source que pudieran
# haber quedado de compilaciones viejas (CMakeCache.txt, _deps, *.vcxproj, etc).
#
# Uso:
#   .\sh\clean.ps1            # borra build/
#   .\sh\clean.ps1 -InSource  # además limpia artefactos regados en el árbol fuente

param(
    [switch]$InSource
)

. "$PSScriptRoot\_config.ps1"

if (Test-Path $BuildDir) {
    Write-Host ">> Borrando '$BuildDir'" -ForegroundColor Cyan
    Remove-Item -Recurse -Force $BuildDir
} else {
    Write-Host ">> No existe '$BuildDir', nada que borrar." -ForegroundColor DarkGray
}

if ($InSource) {
    Write-Host ">> Limpiando artefactos in-source en '$RepoRoot'" -ForegroundColor Cyan
    $targets = @(
        "CMakeCache.txt", "CMakeFiles", "_deps",
        "*.sln", "*.vcxproj", "*.vcxproj.filters", "*.vcxproj.user",
        "x64", "Debug", "Release", "ALL_BUILD.dir", "ZERO_CHECK.dir"
    )
    foreach ($t in $targets) {
        Get-ChildItem -Path $RepoRoot -Filter $t -Force -ErrorAction SilentlyContinue |
            ForEach-Object {
                Write-Host "   - $($_.Name)" -ForegroundColor DarkGray
                Remove-Item -Recurse -Force $_.FullName -ErrorAction SilentlyContinue
            }
    }
}

Write-Host ">> Limpieza lista." -ForegroundColor Green
