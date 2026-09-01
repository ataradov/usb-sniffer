`timescale 1ns / 1ps

module usb_capture_burst_tb;
  localparam PACKET_COUNT = 8;
  localparam PACKET_SIZE = 515;

  reg clk = 0;
  reg reset = 1;
  reg dir = 0;
  reg nxt = 0;
  reg [7:0] phy_data = 0;
  reg phy_drive = 0;
  tri [7:0] ulpi_data;
  wire [7:0] int_data;
  wire int_valid;

  integer captured_bytes = 0;
  integer captured_packets = 0;
  integer overflow_events = 0;
  integer packet;
  integer byte_index;

  assign ulpi_data = phy_drive ? phy_data : 8'hzz;

  usb_capture dut (
    .reset_i(reset),
    .usb_clk_i(clk),
    .usb_stp_o(),
    .usb_dir_i(dir),
    .usb_nxt_i(nxt),
    .usb_d_io(ulpi_data),
    .usb_dm_i(2'b00),
    .usb_dp_i(2'b00),
    .int_data_o(int_data),
    .int_valid_o(int_valid),
    .int_ack_i(1'b1),
    .ctrl_enable_i(1'b1),
    .ctrl_speed_i(2'b10),
    .ctrl_compact_i(1'b1),
    .ctrl_fold_i(1'b0),
    .trigger_i(1'b0)
  );

  always #5 clk = !clk;

  always @(posedge clk) begin
    if (dut.wr_data_w)
      captured_bytes <= captured_bytes + 1;
    if (dut.commit_data_w)
      captured_packets <= captured_packets + 1;
    if (dut.raw_pop_w && dut.raw_type_w == 2'd3)
      overflow_events <= overflow_events + 1;
  end

  task rx_command(input [1:0] event_code);
  begin
    @(negedge clk);
    dir <= 1'b1;
    nxt <= 1'b0;
    phy_drive <= 1'b1;
    phy_data <= { 2'b00, event_code, 4'b0000 };
    @(posedge clk);
  end
  endtask

  task rx_byte(input [7:0] value);
  begin
    @(negedge clk);
    nxt <= 1'b1;
    phy_data <= value;
    @(posedge clk);
  end
  endtask

  initial begin
    repeat (4) @(posedge clk);
    reset <= 0;

    // Allow ULPI setup and the initial status record to complete.
    repeat (30) @(posedge clk);
    rx_command(2'b00);
    rx_command(2'b00);

    // Eight maximum-size packets with only one ULPI clock between them model a
    // dense HS burst. Header formatting must not make the raw queue overflow.
    for (packet = 0; packet < PACKET_COUNT; packet = packet + 1) begin
      rx_command(2'b01);
      for (byte_index = 0; byte_index < PACKET_SIZE; byte_index = byte_index + 1)
        rx_byte((byte_index == 0) ? (packet[0] ? 8'h4b : 8'hc3) : byte_index[7:0]);
      rx_command(2'b00);
    end

    repeat (2000) @(posedge clk);

    if (overflow_events !== 0)
      $fatal(1, "Expected no raw overflows, observed %0d", overflow_events);
    if (captured_packets !== PACKET_COUNT)
      $fatal(1, "Expected %0d packets, captured %0d", PACKET_COUNT, captured_packets);
    if (captured_bytes !== PACKET_COUNT * PACKET_SIZE)
      $fatal(1, "Expected %0d bytes, captured %0d", PACKET_COUNT * PACKET_SIZE, captured_bytes);

    $display("usb_capture_burst_tb: PASS");
    $finish;
  end
endmodule
