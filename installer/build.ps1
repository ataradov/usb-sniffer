$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$sourceExe = Join-Path $projectRoot 'software\usb_sniffer.exe'
$payloadExe = Join-Path $PSScriptRoot 'usb_sniffer.exe'
$distDir = Join-Path $projectRoot 'dist'
$outputExe = Join-Path $distDir 'USB-Sniffer-Wireshark-Plugin-Setup.exe'
$sedFile = Join-Path $distDir 'usb_sniffer_installer.sed'

if (!(Test-Path -LiteralPath $sourceExe)) {
  throw 'Build software\usb_sniffer.exe before creating the installer'
}

New-Item -ItemType Directory -Force -Path $distDir | Out-Null
Copy-Item -LiteralPath $sourceExe -Destination $payloadExe -Force
if (Test-Path -LiteralPath $outputExe) {
  Remove-Item -LiteralPath $outputExe -Force
}

$sed = @"
[Version]
Class=IEXPRESS
SEDVersion=3

[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=1
HideExtractAnimation=1
UseLongFileName=1
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=0
RebootMode=N
InstallPrompt=
DisplayLicense=
FinishMessage=USB Sniffer plugin installed. Restart or refresh interfaces in Wireshark.
TargetName=$outputExe
FriendlyName=USB Sniffer Wireshark Plugin Setup
AppLaunched=install.cmd
PostInstallCmd=<None>
AdminQuietInstCmd=install.cmd
UserQuietInstCmd=install.cmd
SourceFiles=SourceFiles

[SourceFiles]
SourceFiles0=$PSScriptRoot\

[SourceFiles0]
%FILE0%=
%FILE1%=

[Strings]
FILE0=install.cmd
FILE1=usb_sniffer.exe
"@

[IO.File]::WriteAllText($sedFile, $sed, [Text.Encoding]::ASCII)

$iexpress = "$env:SystemRoot\System32\iexpress.exe"
$process = Start-Process -FilePath $iexpress -ArgumentList '/N', '/Q', $sedFile -WindowStyle Hidden -Wait -PassThru
if ($process.ExitCode) { throw "IExpress failed with exit code $($process.ExitCode)" }
if (!(Test-Path -LiteralPath $outputExe)) { throw 'Installer was not created' }

Get-Item -LiteralPath $outputExe
