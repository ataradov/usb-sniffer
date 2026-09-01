param(
  [string]$DiamondRoot
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $DiamondRoot) {
  $DiamondRoot = Join-Path $repoRoot 'tools\lattice-diamond-3.14\install'
}

$diamondRootPath = (Resolve-Path -LiteralPath $DiamondRoot).Path
$buildScriptPath = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'build-fpga-wsl.sh')).Path

function Convert-ToWslPath([string]$Path) {
  if ($Path -notmatch '^([A-Za-z]):\\(.*)$') {
    throw "Cannot convert path to WSL: $Path"
  }
  return "/mnt/$($Matches[1].ToLowerInvariant())/$($Matches[2].Replace('\', '/'))"
}

$diamondWsl = Convert-ToWslPath $diamondRootPath
$buildScriptWsl = Convert-ToWslPath $buildScriptPath

wsl.exe -d Ubuntu -- bash $buildScriptWsl $diamondWsl
if ($LASTEXITCODE -ne 0) {
  throw "Lattice Diamond build failed with exit code $LASTEXITCODE"
}

$jedPath = Join-Path $repoRoot 'fpga\impl\usb_sniffer_impl.jed'
if (-not (Test-Path -LiteralPath $jedPath)) {
  throw "Diamond completed without producing $jedPath"
}

$outputPath = Join-Path $repoRoot 'bin\usb_sniffer_impl.jed'
Copy-Item -LiteralPath $jedPath -Destination $outputPath -Force
Get-FileHash -LiteralPath $outputPath -Algorithm SHA256
