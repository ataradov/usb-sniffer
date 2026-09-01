$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$toolsDir = Join-Path $projectRoot 'tools'
$downloadsDir = Join-Path $toolsDir 'downloads'
$targetDir = Join-Path $toolsDir 'sdcc-4.6.0'
$installer = Join-Path $downloadsDir 'sdcc-4.6.0-x64-setup.exe'
$downloadUrl = 'https://downloads.sourceforge.net/project/sdcc/sdcc-win64/4.6.0/sdcc-4.6.0-x64-setup.exe'
$expectedSha256 = '0A165E155A052FCF7C29EA703EE77D5A8EB578EBA58279E79E885618DC4B2E1A'
$sevenZip = 'C:\Program Files\7-Zip\7z.exe'

if (Test-Path (Join-Path $targetDir 'bin\sdcc.exe')) {
  Write-Host "SDCC is already installed at $targetDir"
  exit 0
}

if (Test-Path -LiteralPath $targetDir) {
  throw "Refusing to overwrite existing directory: $targetDir"
}
if (!(Test-Path -LiteralPath $sevenZip)) {
  throw '7-Zip is required to extract the portable SDCC toolchain'
}

New-Item -ItemType Directory -Force -Path $downloadsDir | Out-Null
& curl.exe -L --fail --retry 3 --output $installer $downloadUrl
if ($LASTEXITCODE) { throw 'SDCC download failed' }

$actualSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $installer).Hash
if ($actualSha256 -ne $expectedSha256) {
  throw "Unexpected SDCC installer SHA-256: $actualSha256"
}

New-Item -ItemType Directory -Path $targetDir | Out-Null
& $sevenZip x -y "-o$targetDir" $installer | Out-Null
if ($LASTEXITCODE) { throw 'SDCC extraction failed' }

& (Join-Path $targetDir 'bin\sdcc.exe') --version
if ($LASTEXITCODE) { throw 'Extracted SDCC failed to run' }
