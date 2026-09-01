`timescale 1ns / 1ps

module usb_capture_fold_tb;
  reg clk = 0;
  reg reset = 1;
  reg dir = 0;
  reg nxt = 0;
  reg [7:0] phy_data = 0;
  reg phy_drive = 0;
  tri [7:0] ulpi_data;

  integer data_commits = 0;
  integer fold_commits = 0;
  integer folded_sof = 0;
  integer folded_nak = 0;
  integer retained_records = 0;
  integer i;

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
    .int_data_o(),
    .int_valid_o(),
    .int_ack_i(1'b1),
    .ctrl_enable_i(1'b1),
    .ctrl_speed_i(2'b10),
    .ctrl_compact_i(1'b1),
    .ctrl_fold_i(1'b1),
    .trigger_i(1'b0)
  );

  always #5 clk = !clk;

  always @(posedge clk) begin
    if (dut.commit_data_w) begin
      data_commits <= data_commits + 1;
      retained_records <= retained_records + (dut.pending_valid_r ? 2 : 1);
    end
    if (dut.commit_fold_w) begin
      fold_commits <= fold_commits + 1;
      folded_sof <= folded_sof + dut.fold_sof_count_r;
      folded_nak <= folded_nak + dut.fold_nak_count_r;
    end
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

  task rx_packet1(input [7:0] b0);
  begin
    rx_command(2'b01);
    rx_byte(b0);
    rx_command(2'b00);
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
    repeat (30) @(posedge clk);
    rx_command(2'b00);
    rx_command(2'b00);

    // Empty traffic is accumulated so a summary is not emitted for every
    // individual SOF or IN/NAK transaction.
    for (i = 0; i < 16; i = i + 1)
      rx_packet3(8'ha5, 8'h93, 8'h00);
    for (i = 0; i < 16; i = i + 1) begin
      rx_packet3(8'h69, 8'h52, 8'h39);
      rx_packet1(8'h5a);
    end

    // A pending IN followed by anything except NAK must retain both records.
    rx_packet3(8'h69, 8'h52, 8'h39);
    rx_packet1(8'hd2);

    // A non-empty token remains a normal USB record.
    rx_packet3(8'he1, 8'h52, 8'h39);
    repeat (200) @(posedge clk);

    if (fold_commits == 0)
      $fatal(1, "Expected at least one fold summary");
    if (folded_sof !== 16)
      $fatal(1, "Expected 16 folded SOFs, observed %0d", folded_sof);
    if (folded_nak !== 16)
      $fatal(1, "Expected 16 folded IN/NAK transactions, observed %0d", folded_nak);
    if (data_commits !== 2)
      $fatal(1, "Expected two data commits, observed %0d", data_commits);
    if (retained_records !== 3)
      $fatal(1, "Expected three retained USB records, observed %0d", retained_records);

    $display("usb_capture_fold_tb: PASS");
    $finish;
  end
endmodule
