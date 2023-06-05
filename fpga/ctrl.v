// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

`timescale 1ns / 1ps

//-----------------------------------------------------------------------------
module ctrl (
  input          clk_i,
  input          ctrl_clk_i,
  input          ctrl_data_i,
  output  [15:0] ctrl_o
);

//-----------------------------------------------------------------------------
reg [2:0] clk_sync_r  = 3'b000;
reg [2:0] data_sync_r = 3'b000;

always @(posedge clk_i) begin
  clk_sync_r  <= { ctrl_clk_i, clk_sync_r[2:1] };
  data_sync_r <= { ctrl_data_i, data_sync_r[2:1] };
end

wire start_w = clk_sync_r[0] && !data_sync_r[1] && data_sync_r[0];
wire stop_w  = clk_sync_r[0] && data_sync_r[1] && !data_sync_r[0];
wire bit_w   = clk_sync_r[1] && !clk_sync_r[0];

//-----------------------------------------------------------------------------
reg [4:0] data_r  = 5'b0;
reg [2:0] count_r = 3'd0;
reg [15:0] ctrl_r = 16'h0000;
reg error_r       = 1'b0;
reg active_r      = 1'b0;

wire count_ok_w = count_r == 3'd5;
wire done_w     = stop_w && count_ok_w && !error_r;

always @(posedge clk_i) begin
  if (start_w && !active_r) begin
    data_r   <= 5'b0;
    count_r  <= 3'd0;
    error_r  <= 1'b0;
    active_r <= 1'b1;
  end else if (done_w) begin
    ctrl_r[data_r[3:0]] <= data_r[4];
    active_r <= 1'b0;
  end else if (stop_w) begin
    active_r <= 1'b0;
  end else if (bit_w) begin
    if (count_ok_w) begin
      error_r <= 1'b1;
    end else begin
      data_r  <= { data_sync_r[1], data_r[4:1] };
      count_r <= count_r + 1'd1;
    end
  end
end

//-----------------------------------------------------------------------------
assign ctrl_o = ctrl_r;

endmodule


