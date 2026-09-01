@echo off
setlocal
set "DEST=%APPDATA%\Wireshark\extcap"

if not exist "%DEST%" mkdir "%DEST%"
if errorlevel 1 goto :error

copy /Y "%~dp0usb_sniffer.exe" "%DEST%\usb_sniffer.exe" >nul
if errorlevel 1 goto :error

"%DEST%\usb_sniffer.exe" --extcap-interfaces >nul
if errorlevel 1 goto :error

echo USB Sniffer Wireshark extcap plugin installed to:
echo %DEST%\usb_sniffer.exe
exit /b 0

:error
echo Failed to install USB Sniffer Wireshark extcap plugin. 1>&2
exit /b 1
