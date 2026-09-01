param(
  [string]$ToolchainRoot,
  [string]$WslDistribution = 'Ubuntu'
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$fpgaRoot = Join-Path $repoRoot 'fpga'
if (-not $ToolchainRoot) {
  $ToolchainRoot = Join-Path $repoRoot 'tools\oss-cad-suite\oss-cad-suite'
}

$toolchainRootPath = (Resolve-Path -LiteralPath $ToolchainRoot).Path
$environmentScript = Join-Path $toolchainRootPath 'environment.ps1'
if (-not (Test-Path -LiteralPath $environmentScript)) {
  throw "OSS CAD Suite environment was not found: $environmentScript"
}
. $environmentScript

$binPath = Join-Path $toolchainRootPath 'bin'
$yosys = Join-Path $binPath 'yosys.exe'
$wslToolRoot = Join-Path $repoRoot 'tools\nextpnr-machxo2-2000-wsl'
$nextpnr = Join-Path $wslToolRoot 'nextpnr-machxo2-2000'
$ecppack = Join-Path $wslToolRoot 'ecppack'
$libtrellis = Join-Path $wslToolRoot 'libtrellis.so'

foreach ($tool in @($yosys, $nextpnr, $ecppack, $libtrellis)) {
  if (-not (Test-Path -LiteralPath $tool)) {
    throw "Required FPGA tool was not found: $tool"
  }
}

$buildRoot = Join-Path $fpgaRoot 'oss-build'
New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null

$jsonPath = Join-Path $buildRoot 'usb_sniffer.json'
$configPath = Join-Path $buildRoot 'usb_sniffer.config'
$bitPath = Join-Path $buildRoot 'usb_sniffer.bit'
$logPath = Join-Path $buildRoot 'timing.log'
$reportPath = Join-Path $buildRoot 'report.json'
$ossLpfPath = Join-Path $buildRoot 'usb_sniffer-oss.lpf'

$sources = @(
  'usb_sniffer.v',
  'fifo_sync.v',
  'usb_phy.v',
  'usb_capture.v',
  'ctrl.v',
  'speed_detect.v'
) | ForEach-Object { Join-Path $fpgaRoot $_ }
$sourcesYosys = $sources | ForEach-Object { $_.Replace('\', '/') }
$jsonYosys = $jsonPath.Replace('\', '/')

& $yosys -p "read_verilog $($sourcesYosys -join ' '); synth_lattice -family xo2 -top usb_sniffer -json $jsonYosys"
if ($LASTEXITCODE -ne 0) { throw "Yosys failed with exit code $LASTEXITCODE" }

$lpfLines = Get-Content -LiteralPath (Join-Path $fpgaRoot 'usb_sniffer.lpf') |
  Where-Object {
    $_ -notmatch '^SYSCONFIG (JTAG_PORT|MUX_CONFIGURATION_PORTS)=' -and
    $_ -notmatch '^IOBUF ALLPORTS '
  }
[IO.File]::WriteAllLines($ossLpfPath, $lpfLines)

function Convert-ToWslPath([string]$Path) {
  $resolved = (Resolve-Path -LiteralPath $Path).Path
  if ($resolved -notmatch '^([A-Za-z]):\\(.*)$') {
    throw "Cannot convert path to WSL: $resolved"
  }
  return "/mnt/$($Matches[1].ToLowerInvariant())/$($Matches[2].Replace('\', '/'))"
}

$nextpnrWsl = Convert-ToWslPath $nextpnr
$ecppackWsl = Convert-ToWslPath $ecppack
$wslToolRootPath = Convert-ToWslPath $wslToolRoot
$trellisDbWsl = Convert-ToWslPath (Join-Path $toolchainRootPath 'share\trellis\database')
$jsonWsl = Convert-ToWslPath $jsonPath
$lpfWsl = Convert-ToWslPath $ossLpfPath
$configWsl = "/mnt/$($configPath.Substring(0, 1).ToLowerInvariant())/$($configPath.Substring(3).Replace('\', '/'))"
$bitWsl = "/mnt/$($bitPath.Substring(0, 1).ToLowerInvariant())/$($bitPath.Substring(3).Replace('\', '/'))"
$logWsl = "/mnt/$($logPath.Substring(0, 1).ToLowerInvariant())/$($logPath.Substring(3).Replace('\', '/'))"
$reportWsl = "/mnt/$($reportPath.Substring(0, 1).ToLowerInvariant())/$($reportPath.Substring(3).Replace('\', '/'))"

& wsl.exe -d $WslDistribution -- $nextpnrWsl `
  --device LCMXO2-2000HC-5TG100C --freq 60 --json $jsonWsl `
  --lpf $lpfWsl --textcfg $configWsl --report $reportWsl --log $logWsl
if ($LASTEXITCODE -ne 0) { throw "nextpnr-machxo2 failed with exit code $LASTEXITCODE" }

& wsl.exe -d $WslDistribution -- env `
  "LD_LIBRARY_PATH=$wslToolRootPath" `
  $ecppackWsl --db $trellisDbWsl --input $configWsl --bit $bitWsl
if ($LASTEXITCODE -ne 0) { throw "ecppack failed with exit code $LASTEXITCODE" }

$outputPath = Join-Path $repoRoot 'bin\usb_sniffer_impl.bit'
Copy-Item -LiteralPath $bitPath -Destination $outputPath -Force
Get-FileHash -LiteralPath $outputPath -Algorithm SHA256
