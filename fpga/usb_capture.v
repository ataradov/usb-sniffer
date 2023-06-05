// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

`timescale 1ns / 1ps

module usb_capture (
  input         reset_i,

  input         usb_clk_i,
  output        usb_stp_o,
  input         usb_dir_i,
  input         usb_nxt_i,
  inout   [7:0] usb_d_io,

  input   [1:0] usb_dm_i,
  input   [1:0] usb_dp_i,

  output  [7:0] int_data_o,
  output        int_valid_o,
  input         int_ack_i,

  input         ctrl_enable_i,
  input   [1:0] ctrl_speed_i,

  input         trigger_i
);

//-----------------------------------------------------------------------------
localparam PTR_WIDTH = 13;
localparam FIFO_SIZE = 1 << PTR_WIDTH;

localparam
  DATA_HEADER_SIZE   = 3'd7,
  STATUS_HEADER_SIZE = 3'd4;

localparam
  USB_SPEED_LS      = 2'b00,
  USB_SPEED_FS      = 2'b01,
  USB_SPEED_HS      = 2'b10,
  USB_SPEED_AUTO    = 2'b11,
  USB_SPEED_UNKNOWN = 2'b11;

//-----------------------------------------------------------------------------
reg utmi_rx_active_r = 1'b0;

wire utmi_rx_active_w;

always @(posedge usb_clk_i) begin
  utmi_rx_active_r <= utmi_rx_active_w;
end

wire rx_start_w = (!utmi_rx_active_r && utmi_rx_active_w);
wire rx_stop_w  = (utmi_rx_active_r && !utmi_rx_active_w);

//-----------------------------------------------------------------------------
wire [1:0] speed_detect_w;

speed_detect speed_detect_inst (
  .clk_i       (usb_clk_i),
  .dm_i        (usb_dm_i),
  .dp_i        (usb_dp_i),
  .vbus_i      (utmi_vbus_w[1]),
  .rx_active_i (utmi_rx_active_r),
  .speed_o     (speed_detect_w)
);

wire [1:0] auto_speed_w = (speed_detect_w == USB_SPEED_UNKNOWN) ? USB_SPEED_HS : speed_detect_w;
wire [1:0] speed_w = (ctrl_speed_i == USB_SPEED_AUTO) ? auto_speed_w : ctrl_speed_i;

wire [1:0] utmi_xcvrselect_w =
  (speed_w == USB_SPEED_LS) ? 2'b10 :
  (speed_w == USB_SPEED_FS) ? 2'b01 : 2'b00;

wire utmi_termselect_w = (speed_w == USB_SPEED_HS) ? 1'b0 : 1'b1;

//-----------------------------------------------------------------------------
wire [7:0] utmi_rx_data_w;
wire       utmi_rx_valid_w;
wire       utmi_rx_error_w;
wire [1:0] utmi_vbus_w;

usb_phy usb_phy_inst (
  .ulpi_clk_i        (usb_clk_i),
  .ulpi_data_io      (usb_d_io),
  .ulpi_dir_i        (usb_dir_i),
  .ulpi_nxt_i        (usb_nxt_i),
  .ulpi_stp_o        (usb_stp_o),

  .utmi_rx_data_o    (utmi_rx_data_w),
  .utmi_rx_active_o  (utmi_rx_active_w),
  .utmi_rx_valid_o   (utmi_rx_valid_w),
  .utmi_rx_error_o   (utmi_rx_error_w),
  .utmi_tx_data_i    (8'h00),
  .utmi_tx_valid_i   (1'b0),
  .utmi_tx_ready_o   (),

  .utmi_xcvrselect_i (utmi_xcvrselect_w),
  .utmi_termselect_i (utmi_termselect_w),
  .utmi_opmode_i     (2'b01),
  .utmi_dppulldown_i (1'b0),
  .utmi_dmpulldown_i (1'b0),
  .utmi_linestate_o  (),
  .utmi_vbus_o       (utmi_vbus_w)
);

//-----------------------------------------------------------------------------
localparam
  ST_IDLE     = 3'd0,
  ST_DATA     = 3'd1,
  ST_DATA_OVF = 3'd2,
  ST_HEADER   = 3'd3,
  ST_OVERFLOW = 3'd4;

reg  [2:0] state_r;
reg [10:0] data_size_r;
reg        data_error_r;
reg        overflow_r;
reg        pid_rx_r;
reg        pid_ok_r;
reg  [3:0] pid_r;

always @(posedge usb_clk_i) begin
  if (reset_i) begin
    data_size_r  <= STATUS_HEADER_SIZE;
    data_error_r <= 1'b0;
    overflow_r   <= 1'b0;
    pid_rx_r     <= 1'b0;
    pid_ok_r     <= 1'b0;
    pid_r        <= 4'h0;
    state_r      <= ST_IDLE;
  end else case (state_r)
    ST_IDLE: begin
      data_size_r <= STATUS_HEADER_SIZE;

      if (rx_start_w && ctrl_enable_i) begin
        data_size_r  <= DATA_HEADER_SIZE;
        data_error_r <= 1'b0;
        pid_rx_r     <= 1'b0;
        state_r      <= fifo_ready_w ? ST_DATA : ST_OVERFLOW;
      end
    end

    ST_DATA: begin
      if (!utmi_rx_active_w) begin
        state_r <= ST_HEADER;
      end else if (fifo_overflow_w && utmi_rx_valid_w) begin
        state_r <= ST_OVERFLOW;
      end else if (data_size_r == 11'd1280) begin
        state_r <= ST_DATA_OVF;
      end else if (utmi_rx_valid_w)
        data_size_r <= data_size_r + 11'd1;

      if (utmi_rx_valid_w && !pid_rx_r) begin
        pid_rx_r <= 1'b1;
        pid_ok_r <= (utmi_rx_data_w[3:0] == ~utmi_rx_data_w[7:4]);
        pid_r    <= utmi_rx_data_w[3:0];
      end

      if (utmi_rx_error_w)
        data_error_r <= 1'b1;
    end

    ST_DATA_OVF: begin
      if (!utmi_rx_active_w) begin
        state_r <= ST_HEADER;
      end
    end

    ST_HEADER: begin
      if (commit_data_w) begin
        overflow_r <= 1'b0;
        state_r <= ST_IDLE;
      end
    end

    ST_OVERFLOW: begin
      overflow_r <= 1'b1;

      if (!utmi_rx_active_w)
        state_r <= ST_IDLE;
    end
  endcase
end

//-----------------------------------------------------------------------------
// Switching hysteresis because comparators are not symmetric
reg dp_r;
reg dm_r;

always @(posedge usb_clk_i) begin
  dp_r <= dp_r ? usb_dp_i[1] : (usb_dp_i == 2'd3);
  dm_r <= dm_r ? usb_dm_i[1] : (usb_dm_i == 2'd3);
end

//-----------------------------------------------------------------------------
localparam
  SYNC_ST_IDLE     = 2'd0,
  SYNC_ST_COUNT    = 2'd1,
  SYNC_ST_WAIT     = 2'd2,
  SYNC_ST_DETECTED = 2'd3;

localparam SYNC_COUNT_TIME   = 8'd30;
localparam SYNC_WAIT_TIME    = 8'd20;
localparam SYNC_DETECT_TIME  = 8'd160;
localparam SYNC_DETECT_COUNT = 3'd6;

reg [1:0] sync_state_r;
reg [7:0] sync_cnt_r;
reg [2:0] sync_bit_r;

wire ls_j_w = (!dp_r &&  dm_r);
wire ls_k_w = ( dp_r && !dm_r);
wire sync_cond_w = sync_bit_r[0] ? ls_j_w : ls_k_w;
wire sync_detected_w = (sync_state_r == SYNC_ST_DETECTED);

always @(posedge usb_clk_i) begin
  if (reset_i || utmi_rx_active_w || speed_w != USB_SPEED_LS) begin
    sync_state_r <= SYNC_ST_IDLE;
  end else case (sync_state_r)
    SYNC_ST_IDLE: begin
      sync_cnt_r <= 8'd0;
      sync_bit_r <= 3'd0;

      if (ls_k_w)
        sync_state_r <= SYNC_ST_COUNT;
    end

    SYNC_ST_COUNT: begin
      if (!sync_cond_w)
        sync_state_r <= SYNC_ST_IDLE;
      else if (sync_cnt_r == SYNC_COUNT_TIME) begin
        sync_cnt_r <= 8'd0;
        sync_bit_r <= sync_bit_r + 3'd1;

        if (sync_bit_r == SYNC_DETECT_COUNT)
          sync_state_r <= SYNC_ST_DETECTED;
        else
          sync_state_r <= SYNC_ST_WAIT;
      end else
        sync_cnt_r <= sync_cnt_r + 8'd1;
    end

    SYNC_ST_WAIT: begin
      if (sync_cond_w) begin
        sync_cnt_r <= 8'd0;
        sync_state_r <= SYNC_ST_COUNT;
      end else if (sync_cnt_r == SYNC_WAIT_TIME)
        sync_state_r <= SYNC_ST_IDLE;
      else
        sync_cnt_r <= sync_cnt_r + 8'd1;
    end

    SYNC_ST_DETECTED: begin
      if (sync_cnt_r == SYNC_DETECT_TIME)
        sync_state_r <= SYNC_ST_IDLE;
      else
        sync_cnt_r <= sync_cnt_r + 8'd1;
    end
  endcase
end

//-----------------------------------------------------------------------------
reg trigger_r;
reg [5:0] trigger_filter_r;

wire trigger_stable_w  = (trigger_i == trigger_r);
wire trigger_changed_w = (trigger_filter_r == 6'h60); // 1 us

always @(posedge usb_clk_i) begin
  if (reset_i || trigger_stable_w || trigger_changed_w)
    trigger_filter_r <= 6'h0;
  else if (!trigger_changed_w)
    trigger_filter_r <= trigger_filter_r + 6'd1;
end

always @(posedge usb_clk_i) begin
  if (reset_i || trigger_changed_w)
    trigger_r <= trigger_i;
end

//-----------------------------------------------------------------------------
reg vbus_r;
reg [15:0] vbus_filter_r;

wire vbus_w = utmi_vbus_w[1]; // utmi_vbus_w == 2 or 3

wire vbus_stable_w  = (vbus_w == vbus_r);
wire vbus_changed_w = (vbus_filter_r == 16'hffff); // 1 ms

always @(posedge usb_clk_i) begin
  if (reset_i || vbus_stable_w || vbus_changed_w)
    vbus_filter_r <= 16'd0;
  else if (!vbus_changed_w)
    vbus_filter_r <= vbus_filter_r + 16'd1;
end

always @(posedge usb_clk_i) begin
  if (reset_i || vbus_changed_w)
    vbus_r <= vbus_w;
end

//-----------------------------------------------------------------------------
reg [3:0] ls_r;
reg [5:0] ls_filter_r;

wire [3:0] ls_w = { usb_dm_i, usb_dp_i };

wire ls_stable_w  = (ls_w == ls_r);
wire ls_changed_w = (ls_filter_r == 6'd60); // 1 us

always @(posedge usb_clk_i) begin
  if (reset_i || sync_detected_w || utmi_rx_active_w || ls_stable_w || ls_changed_w)
    ls_filter_r <= 6'd0;
  else if (!ls_changed_w)
    ls_filter_r <= ls_filter_r + 6'd1;
end

always @(posedge usb_clk_i) begin
  if (reset_i || ls_changed_w)
    ls_r <= ls_w;
end

//-----------------------------------------------------------------------------
reg [1:0] speed_r;

always @(posedge usb_clk_i) begin
  speed_r <= speed_detect_w;
end

//-----------------------------------------------------------------------------
reg [7:0] status_r;
reg       status_pending_r;

wire [7:0] status_w = { speed_r, trigger_r, vbus_r, ls_r };
wire update_status_w = (status_w != status_r && !status_pending_r);

always @(posedge usb_clk_i) begin
  if (reset_i) begin
    status_r <= 8'h00;
    status_pending_r <= 1'b1;
  end else if (update_status_w) begin
    status_pending_r <= 1'b1;
  end else if (commit_status_w) begin
    status_r <= status_w;
    status_pending_r <= 1'b0;
  end
end

//-----------------------------------------------------------------------------
reg [2:0] header_cnt_r;

always @(posedge usb_clk_i) begin
  if (wr_status_w || wr_header_w)
    header_cnt_r <= header_cnt_r + 3'd1;
  else
    header_cnt_r <= 3'd0;
end

//-----------------------------------------------------------------------------
reg toggle_r;

always @(posedge usb_clk_i) begin
  if (reset_i || !ctrl_enable_i)
    toggle_r <= 1'b0;
  else if (fifo_commit_w)
    toggle_r <= !toggle_r;
end

//-----------------------------------------------------------------------------
reg [19:0] timestamp_r;

always @(posedge usb_clk_i) begin
  if (reset_i || !ctrl_enable_i)
    timestamp_r <= 20'd0;
  else
    timestamp_r <= timestamp_r + 20'd1;
end

wire ts_overflow_w = (timestamp_r == 20'hfffff);

//-----------------------------------------------------------------------------
reg ts_overflow_r;

always @(posedge usb_clk_i) begin
  if (reset_i || !ctrl_enable_i)
    ts_overflow_r <= 1'b0;
  else if (ts_overflow_w)
    ts_overflow_r <= 1'b1;
  else if (fifo_commit_w && header_ts_ovf_r)
    ts_overflow_r <= 1'b0;
end

//-----------------------------------------------------------------------------
reg ts_pending_r;

always @(posedge usb_clk_i) begin
  if (reset_i || !ctrl_enable_i || fifo_commit_w)
    ts_pending_r <= 1'b0;
  else if (20'hf0000 == timestamp_r && ts_overflow_r)
    ts_pending_r <= 1'b1;
end

//-----------------------------------------------------------------------------
reg [19:0] header_ts_r;
reg        header_ts_ovf_r;

always @(posedge usb_clk_i) begin
  if (reset_i)
    { header_ts_r, header_ts_ovf_r } <= { 20'h0, 1'b0 };
  else if (!wr_status_w && ST_IDLE == state_r)
    { header_ts_r, header_ts_ovf_r } <= { timestamp_r, ts_overflow_r };
end

//-----------------------------------------------------------------------------
reg [15:0] header_duration_r;

always @(posedge usb_clk_i) begin
  if (reset_i || rx_start_w)
    header_duration_r <= 16'h0;
  else if (utmi_rx_active_r && (header_duration_r != 16'hffff))
    header_duration_r <= header_duration_r + 16'd1;
end

//-----------------------------------------------------------------------------
localparam
  STATUS = 1'b0,
  DATA   = 1'b1;

wire [7:0] status_data_w =
  (3'd0 == header_cnt_r) ? { STATUS, toggle_r, 1'b0, header_ts_ovf_r, header_ts_r[19:16] } :
  (3'd1 == header_cnt_r) ? header_ts_r[15:8] :
  (3'd2 == header_cnt_r) ? header_ts_r[7:0] : status_w;

wire [7:0] header_data_w =
  (3'd0 == header_cnt_r) ? { DATA, toggle_r, 1'b0, header_ts_ovf_r, header_ts_r[19:16] } :
  (3'd1 == header_cnt_r) ? header_ts_r[15:8] :
  (3'd2 == header_cnt_r) ? header_ts_r[7:0] :
  (3'd3 == header_cnt_r) ? { 2'b00, data_error_r, crc_error_w, overflow_r, data_size_r[10:8] } :
  (3'd4 == header_cnt_r) ? data_size_r[7:0] :
  (3'd5 == header_cnt_r) ? header_duration_r[15:8] : header_duration_r[7:0];

//-----------------------------------------------------------------------------
wire commit_data_w   = wr_header_w && (header_cnt_r == (DATA_HEADER_SIZE-1'd1));
wire commit_status_w = wr_status_w && (header_cnt_r == (STATUS_HEADER_SIZE-1'd1));

//-----------------------------------------------------------------------------
wire wr_status_w = (ST_IDLE == state_r && (status_pending_r || ts_pending_r) && fifo_ready_w && ctrl_enable_i);
wire wr_header_w = (ST_HEADER == state_r || (ST_DATA == state_r && !utmi_rx_active_w));
wire wr_data_w   = (ST_DATA == state_r && utmi_rx_active_w && utmi_rx_valid_w);

wire [PTR_WIDTH-1:0] fifo_wr_ptr_w = fifo_wr_ptr_r + (wr_data_w ? data_size_r : header_cnt_r);
wire fifo_wr_w = (wr_status_w || wr_data_w || wr_header_w) && !fifo_overflow_w;
wire fifo_commit_w = commit_status_w || commit_data_w;

wire [7:0] fifo_data_w =
  wr_status_w ? status_data_w :
  wr_header_w ? header_data_w : utmi_rx_data_w;

wire fifo_ready_w = (fifo_size_r > 5'd31);

//-----------------------------------------------------------------------------
reg           [7:0] fifo_mem_r [0:FIFO_SIZE-1];
reg           [7:0] fifo_data_r;
reg [PTR_WIDTH-1:0] fifo_wr_ptr_r;
reg [PTR_WIDTH-1:0] fifo_rd_ptr_r;
reg   [PTR_WIDTH:0] fifo_size_r;

wire fifo_overflow_w = (fifo_wr_ptr_w == fifo_rd_ptr_r) && !fifo_empty_w;

always @(posedge usb_clk_i) begin
  if (reset_i)
    fifo_size_r <= FIFO_SIZE;
  else if (fifo_commit_w)
    fifo_size_r <= fifo_size_r - data_size_r;
  else if (int_valid_o && int_ack_i)
    fifo_size_r <= fifo_size_r + 1'd1;
end

always @(posedge usb_clk_i) begin
  if (reset_i)
    fifo_wr_ptr_r <= 1'd0;
  else if (fifo_commit_w)
    fifo_wr_ptr_r <= fifo_wr_ptr_r + data_size_r;
end

always @(posedge usb_clk_i) begin
  if (fifo_wr_w)
    fifo_mem_r[fifo_wr_ptr_w] <= fifo_data_w;
end

//-----------------------------------------------------------------------------
always @(posedge usb_clk_i) begin
  if (reset_i)
    fifo_rd_ptr_r <= 1'd0;
  else
    fifo_rd_ptr_r <= fifo_rd_ptr_w;
end

wire [PTR_WIDTH-1:0] fifo_rd_ptr_w = (int_valid_o && int_ack_i) ? fifo_rd_ptr_r + 1'd1 : fifo_rd_ptr_r;

always @(posedge usb_clk_i) begin
  fifo_data_r <= fifo_mem_r[fifo_rd_ptr_w];
end

wire fifo_empty_w = (fifo_size_r == FIFO_SIZE);

assign int_data_o  = fifo_data_r;
assign int_valid_o = !fifo_empty_w && !fifo_commit_w;

//-----------------------------------------------------------------------------
reg   [4:0] crc5_r;
reg  [15:0] crc16_r;
reg         crc5_ok_r;
reg         crc16_ok_r;

wire  [4:0] crc5_w  = usb_crc5(crc5_r, utmi_rx_data_w);
wire [15:0] crc16_w = usb_crc16(crc16_r, utmi_rx_data_w);

always @(posedge usb_clk_i) begin
  if (reset_i || ST_IDLE == state_r) begin
    crc5_r     <= 5'h1f;
    crc16_r    <= 16'hffff;
    crc5_ok_r  <= 1'b0;
    crc16_ok_r <= 1'b0;
  end else if (utmi_rx_active_w && utmi_rx_valid_w && pid_rx_r) begin
    crc5_r     <= crc5_w;
    crc16_r    <= crc16_w;
    crc5_ok_r  <= (5'h0c == crc5_w);
    crc16_ok_r <= (16'h800d == crc16_w);
  end
end

//-----------------------------------------------------------------------------
wire [15:0] pid_oh_w = one_hot(pid_r);

wire crc_pid12_w = (data_size_r == (DATA_HEADER_SIZE+1'd1)) ? pid_ok_r : crc5_ok_r;

wire [15:0] crc_w = { crc16_ok_r, pid_ok_r, crc5_ok_r, crc_pid12_w, crc16_ok_r,
    pid_ok_r, crc5_ok_r, crc5_ok_r, crc16_ok_r, pid_ok_r, crc5_ok_r, crc5_ok_r,
    crc16_ok_r, pid_ok_r, crc5_ok_r, 1'b0 };

wire [15:0] crc_ok_oh_w = pid_oh_w & crc_w;

wire crc_error_w = !(|crc_ok_oh_w);

//-----------------------------------------------------------------------------
function [4:0] usb_crc5(input [4:0] crc, input [7:0] data);
begin
  usb_crc5[0] = crc[0] ^ crc[2] ^ crc[3] ^ data[1] ^ data[2] ^ data[4] ^ data[7];
  usb_crc5[1] = crc[1] ^ crc[3] ^ crc[4] ^ data[0] ^ data[1] ^ data[3] ^ data[6];
  usb_crc5[2] = crc[0] ^ crc[3] ^ crc[4] ^ data[0] ^ data[1] ^ data[4] ^ data[5] ^ data[7];
  usb_crc5[3] = crc[0] ^ crc[1] ^ crc[4] ^ data[0] ^ data[3] ^ data[4] ^ data[6];
  usb_crc5[4] = crc[1] ^ crc[2] ^ data[2] ^ data[3] ^ data[5];
end
endfunction

//-----------------------------------------------------------------------------
function [15:0] usb_crc16(input [15:0] crc, input [7:0] data);
begin
  usb_crc16[0] = crc[8] ^ crc[9] ^ crc[10] ^ crc[11] ^ crc[12] ^
      crc[13] ^ crc[14] ^ crc[15] ^ data[0] ^ data[1] ^ data[2] ^
      data[3] ^ data[4] ^ data[5] ^ data[6] ^ data[7];
  usb_crc16[1] = crc[9] ^ crc[10] ^ crc[11] ^ crc[12] ^ crc[13] ^
      crc[14] ^ crc[15] ^ data[0] ^ data[1] ^ data[2] ^ data[3] ^
      data[4] ^ data[5] ^ data[6];
  usb_crc16[2] = crc[8] ^ crc[9] ^ data[6] ^ data[7];
  usb_crc16[3] = crc[9] ^ crc[10] ^ data[5] ^ data[6];
  usb_crc16[4] = crc[10] ^ crc[11] ^ data[4] ^ data[5];
  usb_crc16[5] = crc[11] ^ crc[12] ^ data[3] ^ data[4];
  usb_crc16[6] = crc[12] ^ crc[13] ^ data[2] ^ data[3];
  usb_crc16[7] = crc[13] ^ crc[14] ^ data[1] ^ data[2];
  usb_crc16[8] = crc[0] ^ crc[14] ^ crc[15] ^ data[0] ^ data[1];
  usb_crc16[9] = crc[1] ^ crc[15] ^ data[0];
  usb_crc16[10] = crc[2];
  usb_crc16[11] = crc[3];
  usb_crc16[12] = crc[4];
  usb_crc16[13] = crc[5];
  usb_crc16[14] = crc[6];
  usb_crc16[15] = crc[7] ^ crc[8] ^ crc[9] ^ crc[10] ^ crc[11] ^
      crc[12] ^ crc[13] ^ crc[14] ^ crc[15] ^ data[0] ^ data[1] ^
      data[2] ^ data[3] ^ data[4] ^ data[5] ^ data[6] ^ data[7];
end
endfunction

//-----------------------------------------------------------------------------
function [15:0] one_hot(input [3:0] in);
  integer i;
begin
  for (i = 0; i < 16; i = i + 1)
    one_hot[i] = (i == in);
end
endfunction

endmodule


