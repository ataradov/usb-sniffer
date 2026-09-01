param(
  [string]$WslDistribution = 'Ubuntu'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$scriptPath = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'build-nextpnr-machxo2-2000-wsl.sh')).Path

function Convert-ToWslPath([string]$Path) {
  $resolved = (Resolve-Path -LiteralPath $Path).Path
  if ($resolved -notmatch '^([A-Za-z]):\\(.*)$') {
    throw "Cannot convert path to WSL: $resolved"
  }
  return "/mnt/$($Matches[1].ToLowerInvariant())/$($Matches[2].Replace('\', '/'))"
}

$repoWsl = Convert-ToWslPath $repoRoot
$scriptWsl = Convert-ToWslPath $scriptPath
& wsl.exe -d $WslDistribution -u root -- bash $scriptWsl $repoWsl
if ($LASTEXITCODE -ne 0) {
  throw "nextpnr MachXO2-2000 build failed with exit code $LASTEXITCODE"
}
