`timescale 1ns / 1ps

module fifo_sync_tb;
  reg reset = 1;
  reg wr_clk = 0;
  reg rd_clk = 0;
  reg [15:0] wr_data = 0;
  reg wr_en = 0;
  wire wr_ready;
  wire [15:0] rd_data;
  reg rd_en = 0;
  wire rd_valid;
  integer i;

  fifo_sync #(.W(16)) dut (
    .reset_i(reset),
    .wr_clk_i(wr_clk),
    .wr_data_i(wr_data),
    .wr_en_i(wr_en),
    .wr_ready_o(wr_ready),
    .rd_clk_i(rd_clk),
    .rd_data_o(rd_data),
    .rd_en_i(rd_en),
    .rd_valid_o(rd_valid)
  );

  always #5 wr_clk = !wr_clk;
  always #7 rd_clk = !rd_clk;

  initial begin
    repeat (5) @(posedge wr_clk);
    reset <= 0;

    for (i = 0; i < 8; i = i + 1) begin
      while (!wr_ready) @(posedge wr_clk);
      @(negedge wr_clk);
      wr_data <= 16'h1200 + i;
      wr_en <= 1;
      @(negedge wr_clk);
      wr_en <= 0;
    end

    for (i = 0; i < 8; i = i + 1) begin
      while (!rd_valid) @(posedge rd_clk);
      @(negedge rd_clk);
      if (rd_data !== (16'h1200 + i))
        $fatal(1, "FIFO mismatch at %0d: %h", i, rd_data);
      rd_en <= 1;
      @(negedge rd_clk);
      rd_en <= 0;
    end

    repeat (4) @(posedge rd_clk);
    if (rd_valid) $fatal(1, "FIFO did not become empty");

    $display("fifo_sync_tb: PASS");
    $finish;
  end
endmodule
