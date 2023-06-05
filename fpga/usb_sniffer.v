// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

`timescale 1ns / 1ps

module usb_sniffer (
  input         t_usb_clk_i,
  output        t_usb_stp_o,
  input         t_usb_dir_i,
  input         t_usb_nxt_i,
  inout   [7:0] t_usb_d_io,

  input   [2:0] cmp_dm_i,
  input   [2:0] cmp_dp_i,

  input         ifclk_i,
  output        slrd_o,
  output        slwr_o,
  output        sloe_o,
  output        pktend_o,
  input         flaga_i,
  input         flagb_i,
  input         flagc_i,
  output  [1:0] fifoaddr_o,
  output [15:0] fd_o,

  input         ctrl_clk_i,
  input         ctrl_data_i,

  input         trigger_i,

  input         jtagen_i,

  output  [6:0] spare_o,
  output  [3:0] dbg_o
);

//-----------------------------------------------------------------------------
wire clk_i = t_usb_clk_i;

//-----------------------------------------------------------------------------
reg [1:0] trigger_r;

wire trigger_w = trigger_r[1];

always @(posedge clk_i) begin
  trigger_r <= { trigger_r[0], trigger_i };
end

//-----------------------------------------------------------------------------
reg [2:0] cmp_dm_r [0:1];
reg [2:0] cmp_dp_r [0:1];

always @(posedge clk_i) begin
  cmp_dm_r[0] <= cmp_dm_i;
  cmp_dm_r[1] <= cmp_dm_r[0];

  cmp_dp_r[0] <= cmp_dp_i;
  cmp_dp_r[1] <= cmp_dp_r[0];
end

wire [2:0] cmp_dm_sync_w = cmp_dm_r[1];
wire [2:0] cmp_dp_sync_w = cmp_dp_r[1];

wire [1:0] cmp_dm_w =
  cmp_dm_sync_w[2] ? 2'b11 :
  cmp_dm_sync_w[1] ? 2'b10 :
  cmp_dm_sync_w[0] ? 2'b01 : 2'b00;

wire [1:0] cmp_dp_w =
  cmp_dp_sync_w[2] ? 2'b11 :
  cmp_dp_sync_w[1] ? 2'b10 :
  cmp_dp_sync_w[0] ? 2'b01 : 2'b00;

//-----------------------------------------------------------------------------
wire [15:0] ctrl_w;

ctrl ctrl_inst (
  .clk_i(clk_i),
  .ctrl_clk_i(ctrl_clk_i),
  .ctrl_data_i(ctrl_data_i),
  .ctrl_o(ctrl_w)
);

wire reset_w  = ctrl_w[0];
wire enable_w = ctrl_w[1];
wire [1:0] speed_w = { ctrl_w[3], ctrl_w[2] };
wire test_w   = ctrl_w[4];

//-----------------------------------------------------------------------------
wire [7:0] capture_data_w;
wire       capture_valid_w;
wire       capture_ack_w;

usb_capture usb_capture_inst (
  .reset_i         (reset_w),

  .usb_clk_i       (t_usb_clk_i),
  .usb_stp_o       (t_usb_stp_o),
  .usb_dir_i       (t_usb_dir_i),
  .usb_nxt_i       (t_usb_nxt_i),
  .usb_d_io        (t_usb_d_io),

  .usb_dm_i        (cmp_dm_w),
  .usb_dp_i        (cmp_dp_w),

  .int_data_o      (capture_data_w),
  .int_valid_o     (capture_valid_w),
  .int_ack_i       (capture_ack_w),

  .ctrl_enable_i   (enable_w),
  .ctrl_speed_i    (speed_w),

  .trigger_i       (trigger_w)
);

//-----------------------------------------------------------------------------
wire if_ready_w = !flagb_i;
wire wr_ready_w;

//-----------------------------------------------------------------------------
reg [7:0] buf_r = 8'h00;
reg buf_valid_r = 1'b0;

wire wr_en_w = wr_ready_w && buf_valid_r && capture_valid_w;
wire [15:0] wr_data_w = { capture_data_w, buf_r };

assign capture_ack_w = capture_valid_w && (wr_ready_w || !buf_valid_r);

always @(posedge clk_i) begin
  if (reset_w) begin
    buf_valid_r <= 1'b0;
  end else if (!buf_valid_r && capture_valid_w) begin
    buf_r <= capture_data_w;
    buf_valid_r <= 1'b1;
  end else if (wr_en_w) begin
    buf_valid_r <= 1'b0;
  end
end

//----------------------------------------------------------------------------
wire [15:0] rd_data_w;
wire rd_valid_w;

//----------------------------------------------------------------------------
fifo_sync #(
  .W(16)
) fifo_sync_i (
  .reset_i(reset_w),

  .wr_clk_i(clk_i),
  .wr_data_i(wr_data_w),
  .wr_en_i(wr_en_w),
  .wr_ready_o(wr_ready_w),

  .rd_clk_i(ifclk_i),
  .rd_data_o(rd_data_w),
  .rd_en_i(rd_valid_w && if_ready_w),
  .rd_valid_o(rd_valid_w)
);

//-----------------------------------------------------------------------------
reg [2:0] reset_sync_r = 3'b000;

always @(posedge ifclk_i) begin
  reset_sync_r <= { reset_w, reset_sync_r[2:1] };
end

//-----------------------------------------------------------------------------
reg pktend_r = 1'b0;

always @(posedge ifclk_i) begin
  pktend_r <= if_ready_w && reset_sync_r[1] && !reset_sync_r[0];
end

//-----------------------------------------------------------------------------
reg [1:0] test_sync_r = 2'b00;

wire test_sync_w = test_sync_r[1];

always @(posedge ifclk_i) begin
  test_sync_r <= { test_sync_r[0], test_w && !reset_w };
end

function [15:0] rng_next(input [15:0] state);
reg [15:0] tmp1, tmp2;
begin
  tmp1 = state ^ { state[8:0], 7'h0 };
  tmp2 = tmp1 ^ { 9'h0, tmp1[15:9] };
  rng_next = tmp2 ^ { tmp2[7:0], 8'h0 };
end
endfunction

reg [15:0] rng_r;

always @(posedge ifclk_i) begin
  if (!test_sync_w)
    rng_r <= rng_next(16'h6c41);
  else if (if_ready_w)
    rng_r <= rng_next(rng_r);
end

//-----------------------------------------------------------------------------
assign slwr_o     = if_ready_w && (test_sync_w ? 1'b1 : rd_valid_w);
assign pktend_o   = test_sync_w ? 1'b0 : pktend_r;
assign fd_o       = jtagen_i ? 16'hzzzz : (test_sync_w ? rng_r : rd_data_w);
assign slrd_o     = 1'b0;
assign sloe_o     = 1'b0;
assign fifoaddr_o = 2'b00;

//-----------------------------------------------------------------------------
assign dbg_o   = 4'h0;
assign spare_o = 7'h0;

endmodule

