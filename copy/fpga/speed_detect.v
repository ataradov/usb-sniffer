// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

`timescale 1ns / 1ps

module speed_detect (
  input         clk_i,

  input   [1:0] dm_i,
  input   [1:0] dp_i,
  input         vbus_i,
  input         rx_active_i,

  output  [1:0] speed_o
);

//-----------------------------------------------------------------------------
// This is a qick and dirty bus speed detector for switching the transciever
// to the right speed. The actual protocol verification and decoding happens
// on the host side. This algorithm may get confused by non-compliant behaviour,
// but it is fine, since when debugging a misbehaving target device, speed
// negotiation must be debugged first and the correct reception of the frames
// does not matter as much. There is always an option for manual speed
// selection as well.

//-----------------------------------------------------------------------------
localparam
  USB_SPEED_LS    = 2'b00,
  USB_SPEED_FS    = 2'b01,
  USB_SPEED_HS    = 2'b10,
  USB_SPEED_RESET = 2'b11;

localparam
  ST_IDLE    = 2'd0,
  ST_WAIT    = 2'd1,
  ST_DETECT  = 2'd2,
  ST_HS_WAIT = 2'd3;

//-----------------------------------------------------------------------------
reg [1:0] dmp_r, dpp_r;
reg [1:0] dm_r, dp_r;
reg [5:0] holdoff_r;

always @(posedge clk_i) begin
  { dmp_r, dpp_r } <= { dm_i, dp_i };
end

always @(posedge clk_i) begin
  if ({ dmp_r, dpp_r } != { dm_i, dp_i })
    holdoff_r <= 6'd0;
  else if (holdoff_r == 6'd63) // ~1 us
    { dm_r, dp_r } <= { dm_i, dp_i };
  else
    holdoff_r <= holdoff_r + 6'd1;
end

//-----------------------------------------------------------------------------
reg [17:0] reset_timer_r;

//wire reset_w = (reset_timer_r == 18'd195000); // 3.25 ms
wire reset_w = (reset_timer_r == 18'd65535); // 1.09 ms

always @(posedge clk_i) begin
  if ((dm_i != 2'd0) || (dp_i != 2'd0) || rx_active_i)
    reset_timer_r <= 18'd0;
  else if (!reset_w)
    reset_timer_r <= reset_timer_r + 18'd1;
end

//-----------------------------------------------------------------------------
reg [1:0] state_r = ST_IDLE;
reg [1:0] speed_r = USB_SPEED_RESET;
reg [5:0] delay_r;

always @(posedge clk_i) begin
  if (reset_w || !vbus_i) begin
    speed_r <= USB_SPEED_RESET;
    state_r <= ST_WAIT;
    delay_r <= 6'd0;
  end else if (ST_WAIT == state_r) begin
    if ((dm_r != 2'd0) || (dp_r != 2'd0))
      state_r <= ST_DETECT;
  end else if (ST_DETECT == state_r) begin
    if ((dm_r == 2'd3) && (dp_r == 2'd0)) begin
      speed_r <= USB_SPEED_LS;
      state_r <= ST_IDLE;
    end else if ((dm_r == 2'd0) && (dp_r == 2'd3)) begin
      speed_r <= USB_SPEED_FS;
      state_r <= ST_IDLE;
    end else if ((dm_r == 2'd2) && (dp_r == 2'd0)) begin
      state_r <= ST_HS_WAIT;
    end else begin
      state_r <= ST_IDLE;
    end
  end else if (ST_HS_WAIT == state_r) begin
    if ((dm_r == 2'd1) || (dp_r == 2'd1)) begin
      speed_r <= USB_SPEED_HS;
      state_r <= ST_IDLE;
    end
  end
end

//-----------------------------------------------------------------------------
assign speed_o = speed_r;

endmodule


