# sh/rebuild.ps1
# Limpia build/, reconfigura y compila desde cero.
#
# Uso:
#   .\sh\rebuild.ps1
#   .\sh\rebuild.ps1 -Config Release

param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Debug"
)

. "$PSScriptRoot\_config.ps1"

& "$PSScriptRoot\clean.ps1"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& "$PSScriptRoot\configure.ps1"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& "$PSScriptRoot\build.ps1" -Config $Config
exit $LASTEXITCODE
