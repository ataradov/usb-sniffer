`timescale 1ns / 1ps

module speed_detect_tb;
  reg clk = 0;
  reg reset = 1;
  reg [1:0] dm = 0;
  reg [1:0] dp = 0;
  reg vbus = 0;
  reg rx_active = 0;
  wire [1:0] speed;

  speed_detect dut (
    .clk_i(clk),
    .reset_i(reset),
    .dm_i(dm),
    .dp_i(dp),
    .vbus_i(vbus),
    .rx_active_i(rx_active),
    .speed_o(speed)
  );

  always #5 clk = !clk;

  initial begin
    repeat (4) @(posedge clk);
    reset <= 0;
    vbus <= 1;
    dp <= 2'd3;

    repeat (80) @(posedge clk);
    if (speed !== 2'b01) begin
      $display("Expected Full-Speed, got %b", speed);
      $fatal(1);
    end

    reset <= 1;
    repeat (2) @(posedge clk);
    if (speed !== 2'b11) begin
      $display("Reset did not restore unknown speed, got %b", speed);
      $fatal(1);
    end

    $display("speed_detect_tb: PASS");
    $finish;
  end
endmodule
