`timescale 1ns / 1ps

module trigger_filter_tb;
  reg clk = 0;
  reg reset = 1;
  reg trigger = 0;
  wire [7:0] int_data;
  wire int_valid;
  tri [7:0] usb_data;

  usb_capture dut (
    .reset_i(reset),
    .usb_clk_i(clk),
    .usb_stp_o(),
    .usb_dir_i(1'b0),
    .usb_nxt_i(1'b0),
    .usb_d_io(usb_data),
    .usb_dm_i(2'b00),
    .usb_dp_i(2'b00),
    .int_data_o(int_data),
    .int_valid_o(int_valid),
    .int_ack_i(1'b0),
    .ctrl_enable_i(1'b0),
    .ctrl_speed_i(2'b10),
    .ctrl_compact_i(1'b1),
    .trigger_i(trigger)
  );

  always #5 clk = !clk;

  initial begin
    repeat (4) @(posedge clk);
    reset <= 0;
    trigger <= 1;

    repeat (59) @(posedge clk);
    if (dut.trigger_r !== 1'b0) begin
      $display("Trigger changed before 60 stable samples");
      $fatal(1);
    end

    repeat (3) @(posedge clk);
    if (dut.trigger_r !== 1'b1) begin
      $display("Trigger did not change after 60 stable samples");
      $fatal(1);
    end

    $display("trigger_filter_tb: PASS");
    $finish;
  end
endmodule
