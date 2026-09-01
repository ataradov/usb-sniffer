param([switch]$Hardware)

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$sdccBin = Join-Path $projectRoot 'tools\sdcc-4.6.0\bin'
$makeBin = 'C:\msys64\usr\bin'
$gcc = 'C:\msys64\mingw64\bin\gcc.exe'
$iverilog = 'C:\iverilog\bin\iverilog.exe'
$vvp = 'C:\iverilog\bin\vvp.exe'

if (!(Test-Path (Join-Path $sdccBin 'sdcc.exe'))) {
  throw 'SDCC is not installed under tools\sdcc-4.6.0'
}

$env:Path = "$sdccBin;$makeBin;$env:Path"

Push-Location (Join-Path $projectRoot 'firmware')
try {
  & make clean
  if ($LASTEXITCODE) { throw 'Firmware clean failed' }
  & make
  if ($LASTEXITCODE) { throw 'Firmware build failed' }
} finally {
  Pop-Location
}

$firmware = Join-Path $projectRoot 'firmware\usb_sniffer.bin'
$firmwareBytes = [IO.File]::ReadAllBytes($firmware)
if ($firmwareBytes.Length -gt 0x1800) { throw 'Firmware exceeds the 0x1800-byte code region' }
$firmwareText = [Text.Encoding]::ASCII.GetString($firmwareBytes)
if (!$firmwareText.Contains('[-----SN-----]')) { throw 'Firmware serial-number placeholder is missing' }

& $iverilog -g2012 -Wall -Wimplicit -s usb_sniffer -o (Join-Path $PSScriptRoot 'fpga_lint.vvp') `
  (Join-Path $projectRoot 'fpga\usb_sniffer.v') `
  (Join-Path $projectRoot 'fpga\fifo_sync.v') `
  (Join-Path $projectRoot 'fpga\usb_phy.v') `
  (Join-Path $projectRoot 'fpga\usb_capture.v') `
  (Join-Path $projectRoot 'fpga\ctrl.v') `
  (Join-Path $projectRoot 'fpga\speed_detect.v')
if ($LASTEXITCODE) { throw 'FPGA lint failed' }

& $iverilog -g2012 -Wall -s speed_detect_tb -o (Join-Path $PSScriptRoot 'speed_detect_tb.vvp') `
  (Join-Path $PSScriptRoot 'speed_detect_tb.v') (Join-Path $projectRoot 'fpga\speed_detect.v')
if ($LASTEXITCODE) { throw 'speed_detect simulation build failed' }
& $vvp (Join-Path $PSScriptRoot 'speed_detect_tb.vvp')
if ($LASTEXITCODE) { throw 'speed_detect simulation failed' }

& $iverilog -g2012 -Wall -s trigger_filter_tb -o (Join-Path $PSScriptRoot 'trigger_filter_tb.vvp') `
  (Join-Path $PSScriptRoot 'trigger_filter_tb.v') `
  (Join-Path $projectRoot 'fpga\usb_capture.v') `
  (Join-Path $projectRoot 'fpga\usb_phy.v') `
  (Join-Path $projectRoot 'fpga\speed_detect.v')
if ($LASTEXITCODE) { throw 'trigger filter simulation build failed' }
& $vvp (Join-Path $PSScriptRoot 'trigger_filter_tb.vvp')
if ($LASTEXITCODE) { throw 'trigger filter simulation failed' }

& $iverilog -g2012 -Wall -s usb_phy_tb -o (Join-Path $PSScriptRoot 'usb_phy_tb.vvp') `
  (Join-Path $PSScriptRoot 'usb_phy_tb.v') (Join-Path $projectRoot 'fpga\usb_phy.v')
if ($LASTEXITCODE) { throw 'USB PHY simulation build failed' }
& $vvp (Join-Path $PSScriptRoot 'usb_phy_tb.vvp')
if ($LASTEXITCODE) { throw 'USB PHY simulation failed' }

& $iverilog -g2012 -Wall -s usb_capture_gap_tb -o (Join-Path $PSScriptRoot 'usb_capture_gap_tb.vvp') `
  (Join-Path $PSScriptRoot 'usb_capture_gap_tb.v') `
  (Join-Path $projectRoot 'fpga\usb_capture.v') `
  (Join-Path $projectRoot 'fpga\usb_phy.v') `
  (Join-Path $projectRoot 'fpga\speed_detect.v')
if ($LASTEXITCODE) { throw 'USB capture gap simulation build failed' }
& $vvp (Join-Path $PSScriptRoot 'usb_capture_gap_tb.vvp')
if ($LASTEXITCODE) { throw 'USB capture gap simulation failed' }

& $iverilog -g2012 -Wall -s usb_capture_burst_tb -o (Join-Path $PSScriptRoot 'usb_capture_burst_tb.vvp') `
  (Join-Path $PSScriptRoot 'usb_capture_burst_tb.v') `
  (Join-Path $projectRoot 'fpga\usb_capture.v') `
  (Join-Path $projectRoot 'fpga\usb_phy.v') `
  (Join-Path $projectRoot 'fpga\speed_detect.v')
if ($LASTEXITCODE) { throw 'USB capture burst simulation build failed' }
& $vvp (Join-Path $PSScriptRoot 'usb_capture_burst_tb.vvp')
if ($LASTEXITCODE) { throw 'USB capture burst simulation failed' }

& $iverilog -g2012 -Wall -s fifo_sync_tb -o (Join-Path $PSScriptRoot 'fifo_sync_tb.vvp') `
  (Join-Path $PSScriptRoot 'fifo_sync_tb.v') (Join-Path $projectRoot 'fpga\fifo_sync.v')
if ($LASTEXITCODE) { throw 'FIFO simulation build failed' }
& $vvp (Join-Path $PSScriptRoot 'fifo_sync_tb.vvp')
if ($LASTEXITCODE) { throw 'FIFO simulation failed' }

$osTest = Join-Path $PSScriptRoot 'test_os_common.exe'
& $gcc -DOS_WINDOWS -D_GNU_SOURCE -std=gnu11 -O2 -Wall -Wextra -Werror `
  (Join-Path $PSScriptRoot 'test_os_common.c') (Join-Path $projectRoot 'software\os_common.c') -o $osTest
if ($LASTEXITCODE) { throw 'os_common test build failed' }
& $osTest (Join-Path $PSScriptRoot 'file_io_test.bin')
if ($LASTEXITCODE) { throw 'os_common test failed' }
Remove-Item -LiteralPath (Join-Path $PSScriptRoot 'file_io_test.bin') -Force

$captureTest = Join-Path $PSScriptRoot 'test_capture.exe'
& $gcc -DOS_WINDOWS -D_GNU_SOURCE -std=gnu11 -O1 -g -Wall -Wextra -Werror `
  -ffunction-sections -fdata-sections (Join-Path $PSScriptRoot 'test_capture.c') `
  '-Wl,--gc-sections' -o $captureTest
if ($LASTEXITCODE) { throw 'capture test build failed' }
& $captureTest
if ($LASTEXITCODE) { throw 'capture test failed' }

if ($Hardware) {
  $probe = Join-Path $PSScriptRoot 'probe_firmware.exe'
  & $gcc -std=gnu11 -O2 -Wall -Wextra -Werror `
    '-IC:\msys64\mingw64\include\libusb-1.0' (Join-Path $PSScriptRoot 'probe_firmware.c') `
    'C:\msys64\mingw64\lib\libusb-1.0.a' 'C:\msys64\mingw64\lib\libwinpthread.a' `
    '-Wl,--subsystem,console' -o $probe
  if ($LASTEXITCODE) { throw 'firmware probe build failed' }
  & $probe
  if ($LASTEXITCODE) { throw 'firmware probe failed; reconnect the sniffer to load the updated EEPROM image' }
}

Write-Host "All tests passed. Firmware size: $($firmwareBytes.Length) bytes"
