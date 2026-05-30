# sh/_config.ps1
# Configuración compartida por todos los scripts.
# Se incluye con: . "$PSScriptRoot\_config.ps1"

# Raíz del repo (un nivel arriba de sh/)
$script:RepoRoot  = (Resolve-Path "$PSScriptRoot\..").Path

# Carpeta de build out-of-source
$script:BuildDir  = Join-Path $RepoRoot "build"

# Generador y arquitectura
$script:Generator = "Visual Studio 18 2026"
$script:Arch      = "x64"

# Target ejecutable y dónde queda el binario (CMAKE_RUNTIME_OUTPUT_DIRECTORY)
$script:ExeName   = "sandbox"

function Get-ExePath([string]$Config) {
    return Join-Path $BuildDir "bin\$Config\$ExeName.exe"
}
