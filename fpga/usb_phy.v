// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

`timescale 1ns / 1ps

module usb_phy (
  input         ulpi_clk_i,
  inout   [7:0] ulpi_data_io,
  input         ulpi_dir_i,
  input         ulpi_nxt_i,
  output        ulpi_stp_o,

  output  [7:0] utmi_rx_data_o,
  output        utmi_rx_active_o,
  output        utmi_rx_valid_o,
  output        utmi_rx_error_o,
  input   [7:0] utmi_tx_data_i,
  input         utmi_tx_valid_i,
  output        utmi_tx_ready_o,

  input   [1:0] utmi_xcvrselect_i,
  input         utmi_termselect_i,
  input   [1:0] utmi_opmode_i,
  input         utmi_dppulldown_i,
  input         utmi_dmpulldown_i,
  output  [1:0] utmi_linestate_o,
  output  [1:0] utmi_vbus_o
);

//-----------------------------------------------------------------------------
localparam
  ST_IDLE         = 3'd0,
  ST_WR_FUNC_CTRL = 3'd1,
  ST_WR_OTG_CTRL  = 3'd2,
  ST_WR_REG_STOP  = 3'd3,
  ST_TX_DATA      = 3'd4,
  ST_CLEAR_STP    = 3'd5;

localparam
  CMD_IDLE        = 2'b00,
  CMD_TX          = 2'b01,
  CMD_REG_WRITE   = 2'b10,
  CMD_REG_READ    = 2'b11;

localparam
  REG_FUNC_CTRL   = 6'h4,
  REG_OTG_CTRL    = 6'ha;

//-----------------------------------------------------------------------------
reg [2:0] state_r = ST_IDLE;

//-----------------------------------------------------------------------------
reg dir_r;

always @(posedge ulpi_clk_i) begin
  dir_r <= ulpi_dir_i;
end

wire turnaround_w = dir_r ^ ulpi_dir_i;

//-----------------------------------------------------------------------------
reg [1:0] xcvrselect_r = 2'b00;
reg       termselect_r = 1'b0;
reg [1:0] opmode_r     = 2'b11;
reg       dppulldown_r = 1'b1;
reg       dmpulldown_r = 1'b1;

wire func_ctrl_update_w = (opmode_r != utmi_opmode_i || termselect_r != utmi_termselect_i ||
    xcvrselect_r != utmi_xcvrselect_i);

wire otg_ctrl_update_w = (dppulldown_r != utmi_dppulldown_i || dmpulldown_r != utmi_dmpulldown_i);

always @(posedge ulpi_clk_i) begin
  if (ST_WR_OTG_CTRL == state_r && ulpi_nxt_i) begin
    dppulldown_r <= utmi_dppulldown_i;
    dmpulldown_r <= utmi_dmpulldown_i;
  end else if (ST_WR_FUNC_CTRL == state_r && ulpi_nxt_i) begin
    xcvrselect_r <= utmi_xcvrselect_i;
    termselect_r <= utmi_termselect_i;
    opmode_r     <= utmi_opmode_i;
  end
end

//-----------------------------------------------------------------------------
reg       rx_error_r  = 1'b0;
reg       rx_active_r = 1'b0;
reg [1:0] linestate_r = 2'b00;
reg [1:0] vbus_r      = 2'b00;

always @(posedge ulpi_clk_i) begin
  if (turnaround_w && ulpi_dir_i && ulpi_nxt_i) begin
    rx_active_r <= 1'b1;
  end else if (!turnaround_w && ulpi_dir_i && !ulpi_nxt_i) begin
    linestate_r <= ulpi_data_io[1:0];
    vbus_r <= ulpi_data_io[3:2];

    case (ulpi_data_io[5:4])
      2'b00: begin
        rx_active_r <= 1'b0;
        rx_error_r  <= 1'b0;
      end

      2'b01: begin
        rx_active_r <= 1'b1;
        rx_error_r  <= 1'b0;
      end

      2'b11: begin
        rx_active_r <= 1'b1;
        rx_error_r  <= 1'b1;
      end

      default: begin
        // Host disconnected
      end
    endcase
  end else if (!ulpi_dir_i) begin
    rx_active_r <= 1'b0;
  end
end

//-----------------------------------------------------------------------------
reg       rx_valid_r = 1'b0;
reg [7:0] rx_data_r  = 8'h00;

always @(posedge ulpi_clk_i) begin
  if (!turnaround_w && ulpi_dir_i) begin
    rx_valid_r <= ulpi_nxt_i;
    rx_data_r  <= ulpi_data_io;
  end else begin
    rx_valid_r <= 1'b0;
  end
end

//-----------------------------------------------------------------------------
reg [7:0] ulpi_data_r = 8'h00;
reg       ulpi_stp_r  = 1'b0;

always @(posedge ulpi_clk_i) begin
  if (!turnaround_w && !ulpi_dir_i) case (state_r)
    ST_IDLE: begin
      if (func_ctrl_update_w) begin
        ulpi_data_r <= { CMD_REG_WRITE, REG_FUNC_CTRL };
        state_r     <= ST_WR_FUNC_CTRL;
      end else if (otg_ctrl_update_w) begin
        ulpi_data_r <= { CMD_REG_WRITE, REG_OTG_CTRL };
        state_r     <= ST_WR_OTG_CTRL;
      end else if (utmi_tx_valid_i) begin
        ulpi_data_r <= { CMD_TX, 2'b00, utmi_tx_data_i[3:0] };
        state_r     <= ST_TX_DATA;
      end
    end

    ST_WR_FUNC_CTRL: begin
      if (ulpi_nxt_i) begin
        ulpi_data_r <= { 3'b010, utmi_opmode_i, utmi_termselect_i, utmi_xcvrselect_i };
        state_r     <= ST_WR_REG_STOP;
      end
    end

    ST_WR_OTG_CTRL: begin
      if (ulpi_nxt_i) begin
        ulpi_data_r <= { 5'b00000, utmi_dmpulldown_i, utmi_dppulldown_i, 1'b0 };
        state_r     <= ST_WR_REG_STOP;
      end
    end

    ST_WR_REG_STOP: begin
      if (ulpi_nxt_i) begin
        ulpi_data_r <= 8'h00;
        ulpi_stp_r  <= 1'b1;
        state_r     <= ST_CLEAR_STP;
      end
    end

    ST_TX_DATA: begin
      if (ulpi_nxt_i) begin
        if (!utmi_tx_valid_i) begin
          ulpi_data_r <= 8'h00;
          ulpi_stp_r  <= 1'b1;
          state_r     <= ST_CLEAR_STP;
        end else begin
          ulpi_data_r <= utmi_tx_data_i;
        end
      end
    end

    ST_CLEAR_STP: begin
      ulpi_stp_r <= 1'b0;
      state_r    <= ST_IDLE;
    end

  endcase
end

//-----------------------------------------------------------------------------
assign ulpi_data_io     = (turnaround_w || ulpi_dir_i) ? 8'hzz : ulpi_data_r;
assign ulpi_stp_o       = ulpi_stp_r;

assign utmi_rx_data_o   = rx_data_r;
assign utmi_rx_error_o  = rx_error_r;
assign utmi_rx_active_o = rx_active_r;
assign utmi_rx_valid_o  = rx_valid_r;
assign utmi_tx_ready_o  = (ST_TX_DATA == state_r && ulpi_nxt_i) || (ST_IDLE == state_r && utmi_tx_valid_i);

assign utmi_linestate_o = linestate_r;
assign utmi_vbus_o      = vbus_r;

endmodule

