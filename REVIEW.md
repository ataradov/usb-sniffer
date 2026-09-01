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

## Automated checks

Run on Windows from the repository root:

```powershell
.\tests\run.ps1
```

The suite builds the FX2 firmware, checks its code-size limit and serial-number
placeholder, lints all FPGA RTL, simulates speed detection and trigger filtering,
and runs host file-I/O and capture-folding regression tests.

After programming and physically reconnecting the FX2LP, verify that the fixed
EP2 standard-request path is running on the hardware with:

```powershell
.\tests\run.ps1 -Hardware
```
