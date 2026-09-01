# USB Sniffer code review

Review target: commit `0d26e7e`, covering the Windows host/extcap program, FX2LP
firmware and FPGA RTL.

## Correctness fixes

- Fixed EP2 register lookup in the FX2 firmware. Endpoint 2 previously mapped to
  EP4CFG/EP4CS, which prevented standard endpoint halt status and recovery.
- Added direction validation to endpoint lookup.
- Reset USB configuration/interface state on bus reset and validate
  SET_CONFIGURATION and SET_INTERFACE requests.
- Reset the EP2 FIFO whenever its configuration is enabled.
- Validate all vendor control request lengths and actual EP0 OUT payload sizes.
- Detect short libusb control transfers in the host program.
- Corrected file descriptor and read/write error handling, including descriptor
  zero and partial whole-file reads.
- Fixed non-monotonic PCAPNG timestamps produced by empty-frame folding.
- Clamped formatted capture messages to their destination buffer.
- Fixed the final-position off-by-one error in `find_str()`.
- Corrected the file-size function declaration/definition mismatch.
- Fixed the FPGA trigger filter constant (`6'h60` was truncated to 6'h20).
- Added deterministic reset behavior to the FPGA speed detector and comparator
  hysteresis state.
- Exposed the FPGA automatic speed mode in the CLI and Wireshark extcap UI.
  Auto mode uses separate Low-, Full- and High-Speed PCAPNG interfaces so the
  Wireshark USB link-layer dissector receives the detected speed per packet.
- Made speed detection recover from transient comparator levels and recognize
  the High-Speed device chirp after an initial Full-Speed classification.
  A real phone capture now reports `Detected speed: High-Speed` in auto mode.

## High-Speed capture investigation

The Air 3S capture contains host ACK packets without the preceding device DATA
packet. Since the host can only ACK a DATA packet it received, this proves that
the sniffer missed traffic on the target bus. The capture contains no FPGA FIFO
overflow or ULPI RX error notification.

The host empty-frame folding code does not discard DATA packets. The throughput
test validates the FPGA-to-FX2-to-PC data path, but does not exercise reception
through the USB3343 target PHY. Remaining likely causes are therefore the
USB3343/ULPI receive path or High-Speed electrical signal integrity. No
unproven change to that receive state machine is included in this patch set.

The repaired auto-speed FPGA image was built with the open-source
Yosys/nextpnr/Project Trellis flow and loaded into SRAM. A 20,000-packet phone
capture decoded enumeration descriptors and reproduced the remaining issue:
391 sequence warnings were ACK handshakes without the corresponding device DATA
packet. This further separates the receive-path problem from speed selection.

USB 2.0 permits an opposite-direction High-Speed response after only eight bit
times, which is one 60 MHz ULPI clock. The original capture state machine spent
seven ULPI clocks writing the previous packet header and could not accept the
next packet during that interval. Receive events are now buffered in a small
distributed-RAM queue and formatted independently. The FIFO free-space counter
also now handles a packet commit and host-side byte read in the same clock.
`usb_capture_gap_tb.v` reproduces the minimum-gap case and confirms that both
packets are retained.

The missing device DATA problem was also tested on the physical board with a
direct phone connection, through a USB hub, with another cable, with the
USB3343 VariSense threshold set to 50 percent, and with experimental passive HS
termination. The PHY experiments did not restore DATA and were therefore
removed from the final image.

Changing the sniffer's connection to a different USB port on the capture PC did
restore reliable device DATA reception. A 40,000-packet Auto-mode verification
capture contained DATA0/DATA1 payloads (including complete USB Mass Storage CBW
and CSW records), with zero CRC5 errors, zero CRC16 errors, and zero invalid PID
sequences. Forced High-Speed also worked; forced Full-Speed decoded noise, as
expected for this High-Speed link. This points to port-dependent power, ground,
or signal-integrity sensitivity in the hardware rather than a remaining packet
filter in the host software. Auto mode selected High-Speed correctly.

## Sustained High-Speed throughput

A 68 MB USB Mass Storage copy exposed a separate sustained-throughput limit.
The unoptimized stream reported 4,910 formatted-buffer overflows while moving
the file in about 1.7 seconds. Instrumented headers confirmed that the raw ULPI
event queue did not overflow; all losses occurred after formatting, when short
High-Speed bursts temporarily exceeded the approximately 44.4 MB/s FX2-to-PC
bulk transfer rate.

The final transport keeps the full 8 KiB formatted FIFO and uses compact
three-byte delta-timestamp headers for packets separated by no more than 255
ULPI clocks. Longer gaps and old host utilities retain the original seven-byte
header format. The FX2 slave FIFO clock was raised from 30 to 48 MHz and passes
FPGA timing, although the measured bulk endpoint rate remains USB-limited at
44.3-44.5 MB/s. Overflow telemetry now distinguishes the raw event queue from
the formatted output FIFO.

On the same 68 MB workload, the current best image reduced formatted-buffer
overflows from 4,910 to 2,137, with zero raw-queue overflows and zero CRC5/CRC16
errors. This is a material improvement but is not yet a mathematically lossless
capture at the target device's peak burst rate. Eliminating the remaining loss
will require more aggressive on-FPGA record compression or external capture
memory; it cannot be solved by increasing the already fully allocated eight
MachXO2 EBR blocks.

## Automated checks

Run on Windows from the repository root:

```powershell
.\tests\run.ps1
```

The suite builds the FX2 firmware, checks its code-size limit and serial-number
placeholder, lints all FPGA RTL, simulates speed detection, trigger filtering,
minimum-gap packet reception, and dense maximum-packet bursts, and runs host
file-I/O, compact-header parsing, and capture-folding regression tests.

After programming and physically reconnecting the FX2LP, verify that the fixed
EP2 standard-request path is running on the hardware with:

```powershell
.\tests\run.ps1 -Hardware
```
