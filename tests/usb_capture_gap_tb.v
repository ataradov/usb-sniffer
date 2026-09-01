`timescale 1ns / 1ps

module usb_capture_gap_tb;
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

  task rx_packet3(input [7:0] b0, input [7:0] b1, input [7:0] b2);
  begin
    rx_command(2'b01);
    rx_byte(b0);
    rx_byte(b1);
    rx_byte(b2);
    rx_command(2'b00);
  end
  endtask

  initial begin
    repeat (4) @(posedge clk);
    reset <= 0;

    // Allow the ULPI control-register writes and initial status record to finish.
    repeat (30) @(posedge clk);

    // Turn the ULPI bus around and establish an inactive RxCmd first.
    rx_command(2'b00);
    rx_command(2'b00);

    // An IN token followed by a device DATA packet after the minimum legal
    // HS opposite-direction inter-packet delay (8 bit times = one ULPI clock).
    rx_packet3(8'h69, 8'h00, 8'h00);
    rx_packet3(8'hc3, 8'h11, 8'h22);

    repeat (80) @(posedge clk);

    if (captured_packets !== 2)
      $fatal(1, "Expected two packets, captured %0d", captured_packets);
    if (captured_bytes !== 6)
      $fatal(1, "Expected six packet bytes, captured %0d", captured_bytes);

    $display("usb_capture_gap_tb: PASS");
    $finish;
  end
endmodule
