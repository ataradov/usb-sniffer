`timescale 1ns / 1ps

module usb_phy_tb;
  reg clk = 0;
  reg dir = 0;
  reg nxt = 1;
  wire stp;
  tri [7:0] ulpi_data;
  reg [7:0] phy_data = 0;
  reg phy_drive = 0;
  wire [7:0] rx_data;
  wire rx_active;
  wire rx_valid;
  wire rx_error;

  assign ulpi_data = phy_drive ? phy_data : 8'hzz;

  usb_phy dut (
    .ulpi_clk_i(clk),
    .ulpi_data_io(ulpi_data),
    .ulpi_dir_i(dir),
    .ulpi_nxt_i(nxt),
    .ulpi_stp_o(stp),
    .utmi_rx_data_o(rx_data),
    .utmi_rx_active_o(rx_active),
    .utmi_rx_valid_o(rx_valid),
    .utmi_rx_error_o(rx_error),
    .utmi_tx_data_i(8'h00),
    .utmi_tx_valid_i(1'b0),
    .utmi_tx_ready_o(),
    .utmi_xcvrselect_i(2'b00),
    .utmi_termselect_i(1'b0),
    .utmi_opmode_i(2'b01),
    .utmi_dppulldown_i(1'b0),
    .utmi_dmpulldown_i(1'b0),
    .utmi_linestate_o(),
    .utmi_vbus_o()
  );

  always #5 clk = !clk;

  initial begin
    repeat (20) @(posedge clk);

    @(negedge clk);
    dir <= 1;
    nxt <= 0;
    phy_drive <= 1;
    phy_data <= 8'h10; // RxEvent = active

    repeat (2) @(posedge clk);
    #1;
    if (!rx_active || rx_valid) $fatal(1, "RX command was not decoded");

    @(negedge clk);
    nxt <= 1;
    phy_data <= 8'h69;
    @(posedge clk);
    #1;
    if (!rx_valid || rx_data !== 8'h69) $fatal(1, "First RX byte was lost");

    @(negedge clk);
    phy_data <= 8'ha5;
    @(posedge clk);
    #1;
    if (!rx_valid || rx_data !== 8'ha5) $fatal(1, "Second RX byte was lost");

    @(negedge clk);
    nxt <= 0;
    phy_data <= 8'h00; // RxEvent = inactive
    @(posedge clk);
    #1;
    if (rx_active || rx_valid || rx_error) $fatal(1, "RX stop was not decoded");

    $display("usb_phy_tb: PASS");
    $finish;
  end
endmodule
