// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

`timescale 1ns / 1ps

//-----------------------------------------------------------------------------
module fifo_sync #(
  parameter W = 8
)(
  input          reset_i, // Must be longer than both clocks

  input          wr_clk_i,
  input  [W-1:0] wr_data_i,
  input          wr_en_i,
  output         wr_ready_o,

  input          rd_clk_i,
  output [W-1:0] rd_data_o,
  input          rd_en_i,
  output         rd_valid_o
);

//-----------------------------------------------------------------------------
// The design of this synchronizer is based on the paper by Clifford E. Cummings
// titled "Simulation and Synthesis Techniques for Asynchronous FIFO Design".
// It is a fairly standard FIFO-based synchronizer, but a special attention
// must be paid to the way the signal wr_ready_o is generated.

//-----------------------------------------------------------------------------
reg [3:0] wr_ptr_r   = 4'h0;
reg [3:0] rd_ptr_0_r = 4'h0;
reg [3:0] rd_ptr_1_r = 4'h0;

reg [3:0] rd_ptr_r   = 4'h0;
reg [3:0] wr_ptr_0_r = 4'h0;
reg [3:0] wr_ptr_1_r = 4'h0;

wire [3:0] wr_ptr_bin_w = g2b(wr_ptr_r);
wire [3:0] rd_ptr_bin_w = g2b(rd_ptr_r);

//-----------------------------------------------------------------------------
always @(posedge wr_clk_i) begin
  if (reset_i)
    { rd_ptr_1_r, rd_ptr_0_r } <= 8'h00;
  else
    { rd_ptr_1_r, rd_ptr_0_r } <= { rd_ptr_0_r, rd_ptr_r };
end

assign wr_ready_o = (wr_ptr_r != { ~rd_ptr_1_r[3:2], rd_ptr_1_r[1:0] });

always @(posedge wr_clk_i) begin
  if (reset_i)
    wr_ptr_r <= 1'h0;
  else if (wr_en_i && wr_ready_o)
    wr_ptr_r <= b2g(wr_ptr_bin_w + 4'h1);
end

//-----------------------------------------------------------------------------
always @(posedge rd_clk_i) begin
  if (reset_i)
    { wr_ptr_1_r, wr_ptr_0_r } <= 8'h00;
  else
    { wr_ptr_1_r, wr_ptr_0_r } <= { wr_ptr_0_r, wr_ptr_r };
end

assign rd_valid_o = (rd_ptr_r != wr_ptr_1_r);

always @(posedge rd_clk_i) begin
  if (reset_i)
    rd_ptr_r <= 1'h0;
  else if (rd_en_i && rd_valid_o)
    rd_ptr_r <= b2g(rd_ptr_bin_w + 4'h1);
end

//-----------------------------------------------------------------------------
reg [W-1:0] buf_r [0:7];

always @(posedge wr_clk_i) begin
  if (wr_en_i && wr_ready_o)
    buf_r[wr_ptr_bin_w[2:0]] <= wr_data_i;
end

assign rd_data_o = buf_r[rd_ptr_bin_w[2:0]];

//-----------------------------------------------------------------------------
function [3:0] g2b(input [3:0] g);
begin
  g2b[0] = g[3] ^ g[2] ^ g[1] ^ g[0];
  g2b[1] = g[3] ^ g[2] ^ g[1];
  g2b[2] = g[3] ^ g[2];
  g2b[3] = g[3];
end
endfunction

function [3:0] b2g(input [3:0] b);
begin
  b2g[0] = b[0] ^ b[1];
  b2g[1] = b[1] ^ b[2];
  b2g[2] = b[2] ^ b[3];
  b2g[3] = b[3];
end
endfunction

endmodule

