// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

/*- Includes ----------------------------------------------------------------*/
#include "os_common.h"
#include "usb.h"
#include "usb_sniffer.h"
#include "capture.h"

/*- Definitions -------------------------------------------------------------*/
#define TIME_US                        1000
#define TIME_MS                        (1000 * TIME_US)

#define LINKTYPE_USB_2_0               288
#define LINKTYPE_USB_2_0_LOW_SPEED     293
#define LINKTYPE_USB_2_0_FULL_SPEED    294
#define LINKTYPE_USB_2_0_HIGH_SPEED    295
#define LINKTYPE_WIRESHARK_UPPER_PDU   252

#define INTERFACE_NAME                 "usb_sniffer"

#define UPDATE_INTERVAL                (2000 * TIME_MS)

#define DATA_HEADER_SIZE               7
#define STATUS_HEADER_SIZE             4
#define DATA_BUF_SIZE                  2048
#define FOLD_BUF_SIZE                  128
#define MAX_DATA_SIZE                  1280

// Byte 0
#define HEADER_STATUS                  0x80
#define HEADER_TOGGLE                  0x40
#define HEADER_ZERO                    0x20
#define HEADER_TS_OVERFLOW             0x10

// Byte 3 in data frames
#define HEADER_OVERFLOW                0x08
#define HEADER_CRC_ERROR               0x10
#define HEADER_DATA_ERROR              0x20

// Byte 3 in status frames
#define HEADER_LS_OFFS                 0
#define HEADER_LS_MASK                 0x0f
#define HEADER_VBUS                    0x10
#define HEADER_TRIGGER                 0x20
#define HEADER_SPEED_OFFS              6
#define HEADER_SPEED_MASK              0x03

#define PID_SOF                        0xa5
#define PID_IN                         0x69
#define PID_NAK                        0x5a

#define FOLD_LIMIT_LS_FS               1000
#define FOLD_LIMIT_HS                  8000

#define MIN_KEEPALIVE_DURATION         1000 // 1 us
#define MAX_KEEPALIVE_DURATION         2000 // 2 us

// J & K states are for Low-Speed mode
#define LS_INVALID                     -1
#define LS_SE0                         0
#define LS_J3                          12

#define LS_DELTA_THRESHOLD             (10 * TIME_MS)

/*- Types -------------------------------------------------------------------*/
typedef struct
{
  u64      ts;
  int      size;
  u8       data[DATA_BUF_SIZE];
} CaptureFrame;

/*- Variables ---------------------------------------------------------------*/
static u8   capture_data[DATA_BUF_SIZE];
static int  capture_data_ptr   = 0;
static int  capture_size       = 0;
static bool capture_header     = true;
static bool capture_status     = false;
static int  capture_toggle     = 0;
static int  capture_ls         = -1;
static int  capture_vbus       = -1;
static int  capture_trigger    = -1;
static int  capture_speed      = -1;
static bool capture_enabled    = false;
static u64  capture_ts_int     = 0;
static u64  capture_ts         = 0;
static u64  capture_last_ts    = 0;
static bool capture_overflow   = false;
static bool capture_crc_error  = false;
static bool capture_data_error = false;
static int  capture_duration   = 0;
static FILE *capture_fd        = NULL;
static u8   capture_buf[4096];
static int  capture_buf_ptr    = 0;
static CaptureFrame capture_fold_buf[FOLD_BUF_SIZE];
static int  capture_fold_buf_ptr = 0;
static int  capture_fold_count = 0;
static int  capture_saved_ls   = LS_INVALID;
static u64  capture_saved_ts   = 0;

/*- Prototypes --------------------------------------------------------------*/
static void line_state_event(void);
static void keepalive_event(u64 ts, int delta);
static void stop_folding(void);

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
bool capture_extcap_request(void)
{
  if (g_opt.extcap_version)
  {
    if (strcmp(g_opt.extcap_version, "4.0"))
      log_print("unsupported extcap version");
    else
      printf("extcap {version=1.0}{help=https://github.com/ataradov/usb-sniffer}{display=USB Sniffer}\n");
  }

  if (g_opt.extcap_interfaces)
  {
    printf("interface {value="INTERFACE_NAME"}{display=USB Sniffer}\n");
    return true;
  }

  if (g_opt.extcap_interface && strcmp(g_opt.extcap_interface, INTERFACE_NAME))
  {
    log_print("invalid interface, expected %s", INTERFACE_NAME);
    return true;
  }

  if (g_opt.extcap_dlts)
  {
    printf("dlt {number=%d}{name=USB}{display=USB}\n", LINKTYPE_USB_2_0);
    return true;
  }

  if (g_opt.extcap_config)
  {
    printf("arg {number=0}{call=--speed}{display=Capture Speed}{tooltip=USB capture speed}{type=selector}\n");
    printf("value {arg=0}{value=ls}{display=Low-Speed}{default=false}\n");
    printf("value {arg=0}{value=fs}{display=Full-Speed}{default=true}\n");
    printf("value {arg=0}{value=hs}{display=High-Speed}{default=false}\n");
    printf("arg {number=1}{call=--fold}{display=Fold empty frames}{tooltip=Fold frames that have no data or errors}{type=boolflag}\n");
    printf("arg {number=2}{call=--trigger}{display=Capture Trigger}{tooltip=Condition used to start the capture}{type=selector}\n");
    printf("value {arg=2}{value=disabled}{display=Disabled}{default=true}\n");
    printf("value {arg=2}{value=low}{display=Low}{default=false}\n");
    printf("value {arg=2}{value=high}{display=High}{default=false}\n");
    printf("value {arg=2}{value=falling}{display=Falling}{default=false}\n");
    printf("value {arg=2}{value=rising}{display=Rising}{default=false}\n");
    printf("arg {number=3}{call=--limit}{display=Capture Limit}{tooltip=Limit the number of captured packets (0 for unlimited)}{type=integer}{range=0,10000000}{default=0}\n");
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
static void put_pad(void)
{
  while (capture_buf_ptr % sizeof(u32))
    capture_buf[capture_buf_ptr++] = 0;
}

//-----------------------------------------------------------------------------
static void put_half(u16 value)
{
  capture_buf[capture_buf_ptr+0] = value;
  capture_buf[capture_buf_ptr+1] = value >> 8;
  capture_buf_ptr += sizeof(u16);
}

//-----------------------------------------------------------------------------
static void put_word(u32 value)
{
  capture_buf[capture_buf_ptr+0] = value;
  capture_buf[capture_buf_ptr+1] = value >> 8;
  capture_buf[capture_buf_ptr+2] = value >> 16;
  capture_buf[capture_buf_ptr+3] = value >> 24;
  capture_buf_ptr += sizeof(u32);
}

//-----------------------------------------------------------------------------
static void put_data(u8 *data, int size)
{
  memcpy(&capture_buf[capture_buf_ptr], data, size);
  capture_buf_ptr += size;
}

//-----------------------------------------------------------------------------
static void put_option(int index, char *str)
{
  int len = strlen(str);
  put_half(index);
  put_half(len);
  put_data((u8 *)str, len);
  put_pad();
}

//-----------------------------------------------------------------------------
static void send_buffer(void)
{
  int size = capture_buf_ptr + sizeof(u32);

  put_word(size);

  capture_buf[4] = size;
  capture_buf[5] = size >> 8;
  capture_buf[6] = size >> 16;
  capture_buf[7] = size >> 24;

  int res = fwrite(capture_buf, 1, capture_buf_ptr, capture_fd);
  os_check(capture_buf_ptr == res, "write() error");

  capture_buf_ptr = 0;
}

//-----------------------------------------------------------------------------
static void write_file_header(void)
{
  put_word(0x0a0d0d0a); // Block Type (SHB)
  put_word(0); // Block Length (placeholder)
  put_word(0x1a2b3c4d); // Section Byte Order
  put_half(1); // Major Version
  put_half(0); // Minor Version
  put_word(0xffffffff); // Section Length (unknown)
  put_word(0xffffffff); // Section Length (unknown)
  put_option(0x0002, "USB Sniffer by Alex Taradov"); // shb_hardware
  put_option(0x0000, "");
  send_buffer();
}

//-----------------------------------------------------------------------------
static void write_usb_header(void)
{
  int link_type;

  if (CaptureSpeed_LS == g_opt.capture_speed)
    link_type = LINKTYPE_USB_2_0_LOW_SPEED;
  else if (CaptureSpeed_FS == g_opt.capture_speed)
    link_type = LINKTYPE_USB_2_0_FULL_SPEED;
  else if (CaptureSpeed_HS == g_opt.capture_speed)
    link_type = LINKTYPE_USB_2_0_HIGH_SPEED;
  else
    os_assert(false);

  put_word(1); // Block Type (IDB)
  put_word(0); // Block Length (placeholder)
  put_half(link_type);
  put_half(0); // Reserved
  put_word(0xffff); // Snap Length
  put_option(0x0002, "usb"); // if_name
  put_option(0x0003, "Hardware USB interface"); // if_description
  put_half(9); // if_tsresol
  put_half(1); // Time resolution length data is 1 byte
  put_word(9); // Time resolution nanoseconds (10^-9)
  put_option(0x0000, "");
  send_buffer();
}

//-----------------------------------------------------------------------------
static void write_info_header(void)
{
  put_word(1); // Block Type (IDB)
  put_word(0); // Block Length (placeholder)
  put_half(LINKTYPE_WIRESHARK_UPPER_PDU); // Link Type
  put_half(0); // Reserved
  put_word(0xffff); // Snap Length
  put_option(0x0002, "info"); // if_name
  put_option(0x0003, "Out of band information"); // if_description
  put_half(9); // if_tsresol
  put_half(1); // Time resolution length data is 1 byte
  put_word(9); // Time resolution nanoseconds (10^-9)
  put_option(0x0000, "");
  send_buffer();
}

//-----------------------------------------------------------------------------
static void write_packet(u64 ts, u8 *data, int size)
{
  put_word(6); // Block Type (EPB)
  put_word(0); // Block Total Length (placeholder)
  put_word(0); // Interface ID
  put_word(ts >> 32); // Timestamp Upper
  put_word(ts); // Timestamp Lower
  put_word(size); // Captured Packet Length
  put_word(size); // Original Packet Length
  put_data(data, size);
  put_pad();
  put_option(0x0000, "");
  send_buffer();

  capture_last_ts = ts;
}

//-----------------------------------------------------------------------------
static void write_str(u64 ts, u8 *data, int size)
{
  static u8 hdr[] = { 0, 12, 0, 6, 's', 'y', 's', 'l', 'o', 'g', 0, 0, 0, 0 };

  put_word(6); // Block Type (EPB)
  put_word(0); // Block Total Length (placeholder)
  put_word(1); // Interface ID
  put_word(ts >> 32); // Timestamp Upper
  put_word(ts); // Timestamp Lower
  put_word(sizeof(hdr) + size); // Captured Packet Length
  put_word(sizeof(hdr) + size); // Original Packet Length
  put_data(hdr, sizeof(hdr));
  put_data(data, size);
  put_pad();
  send_buffer();

  capture_last_ts = ts;
}

//-----------------------------------------------------------------------------
static void capture_info(u64 ts, char *fmt, ...)
{
  char str[512];
  va_list args;

  va_start(args, fmt);
  int len = vsnprintf(str, sizeof(str), fmt, args);
  va_end(args);

  line_state_event();
  stop_folding();

  write_str(ts, (u8 *)str, len);

  fflush(capture_fd);
}

//-----------------------------------------------------------------------------
static void write_keepalive(u64 ts)
{
  capture_info(ts, "Keep-alive");
}

//-----------------------------------------------------------------------------
static void timeout_event(void)
{
  if (capture_enabled)
    capture_info(capture_ts, "Periodic update");
}

//-----------------------------------------------------------------------------
static void line_state_event(void)
{
  int dp = (capture_saved_ls >> 0) & 3;
  int dm = (capture_saved_ls >> 2) & 3;
  int delta = capture_ts - capture_saved_ts;
  int level = 0;
  char str[256];

  if (LS_INVALID == capture_saved_ls)
    return;

  capture_saved_ls = LS_INVALID;

  sprintf(str, "Line state: ");

  if (dp == 0 && dm == 0)
  {
    strcat(str, "SE0");
  }
  else if (dp == 0)
  {
    strcat(str, (CaptureSpeed_LS == g_opt.capture_speed) ? "J" : "K");
    level = dm;
  }
  else if (dm == 0)
  {
    strcat(str, (CaptureSpeed_LS == g_opt.capture_speed) ? "K" : "J");
    level = dp;
  }
  else
  {
    char buf[64];
    snprintf(buf, sizeof(buf), "Undefined (DP=%d / DM=%d)", dp, dm);
    strcat(str, buf);
  }

  if (level == 1)
    strcat(str, " [both]");
  else if (level == 2)
    strcat(str, " [single]");

  if (delta < LS_DELTA_THRESHOLD)
  {
    char buf[64];

    if (delta < TIME_US)
      snprintf(buf, sizeof(buf), " (%.2f ns)", (float)delta);
    else if (delta < TIME_MS)
      snprintf(buf, sizeof(buf), " (%.2f us)", (float)delta / TIME_US);
    else
      snprintf(buf, sizeof(buf), " (%.2f ms)", (float)delta / TIME_MS);

    strcat(str, buf);
  }

  capture_info(capture_saved_ts, str);
}

//-----------------------------------------------------------------------------
static void status_event(int ls, int vbus, int trigger, int speed)
{
  if (capture_trigger != trigger)
  {
    bool was_enabled = capture_enabled;

    if (CaptureTrigger_Disabled == g_opt.capture_trigger)
      capture_enabled = true;
    else if (CaptureTrigger_Low == g_opt.capture_trigger)
      capture_enabled = (trigger == 0);
    else if (CaptureTrigger_High == g_opt.capture_trigger)
      capture_enabled = (trigger == 1);
    else if (CaptureTrigger_Falling == g_opt.capture_trigger)
      capture_enabled = capture_enabled || (trigger == 0 && capture_trigger == 1);
    else if (CaptureTrigger_Rising == g_opt.capture_trigger)
      capture_enabled = capture_enabled || (trigger == 1 && capture_trigger == 0);
    else
      os_assert(false);

    capture_trigger = trigger;

    capture_info(capture_ts, "Trigger input = %d", capture_trigger);

    if (capture_enabled && !was_enabled)
      capture_info(capture_ts, "Starting capture");
    else if (was_enabled && !capture_enabled)
      capture_info(capture_ts, "Waiting for a trigger");
  }

  if (capture_vbus != vbus)
  {
    capture_vbus = vbus;
    capture_info(capture_ts, "VBUS %s", capture_vbus ? "ON" : "OFF");
  }

  if (capture_speed != speed)
  {
    static const char *str[] = { "Low-Speed", "Full-Speed", "High-Speed", "" };
    capture_speed = speed;

    if (capture_enabled)
    {
      if (CaptureSpeed_Reset == speed)
        capture_info(capture_ts, "--- Bus Reset ---");
      else
        capture_info(capture_ts, "Detected speed: %s", str[capture_speed]);
    }
  }

  if (capture_ls != ls)
  {
    u64 delta = capture_ts - capture_saved_ts;
    bool handle = true;

    capture_ls = ls;

    if (CaptureSpeed_LS == g_opt.capture_speed && LS_SE0 == capture_saved_ls && LS_J3 == ls &&
        (MIN_KEEPALIVE_DURATION < delta && delta < MAX_KEEPALIVE_DURATION))
    {
      capture_saved_ls = LS_INVALID;
      keepalive_event(capture_saved_ts, delta);
      handle = false;
    }

    if (handle)
    {
      line_state_event();
      capture_saved_ls = ls;
      capture_saved_ts = capture_ts;
    }
  }
}

//-----------------------------------------------------------------------------
static void stop_folding(void)
{
  int count = capture_fold_count;
  int ptr   = capture_fold_buf_ptr;

  if (0 == count && 0 == ptr)
    return;

  capture_fold_count   = 0;
  capture_fold_buf_ptr = 0;

  if (count == 1)
    capture_info(capture_ts, "Folded empty frame");
  else if (count > 1)
    capture_info(capture_ts, "Folded %d empty frames", count);

  for (int i = 0; i < ptr; i++)
  {
    if (capture_fold_buf[i].size < 0)
      write_keepalive(capture_fold_buf[i].ts);
    else
      write_packet(capture_fold_buf[i].ts, capture_fold_buf[i].data, capture_fold_buf[i].size);
  }
}

//-----------------------------------------------------------------------------
static void fold_packet(u64 ts, u8 *data, int size)
{
  capture_fold_buf[capture_fold_buf_ptr].ts = ts;
  capture_fold_buf[capture_fold_buf_ptr].size = size;
  memcpy(capture_fold_buf[capture_fold_buf_ptr].data, data, size);
  capture_fold_buf_ptr++;
}

//-----------------------------------------------------------------------------
static void fold_keepalive(u64 ts, int delta)
{
  capture_fold_buf[capture_fold_buf_ptr].ts = ts;
  capture_fold_buf[capture_fold_buf_ptr].size = -delta;
  capture_fold_buf_ptr++;
}

//-----------------------------------------------------------------------------
static void check_capture_limit(void)
{
  g_opt.capture_limit--;

  if (g_opt.capture_limit == 0)
  {
    capture_info(capture_ts, "Capture limit reached");
    exit(0);
  }
}

//-----------------------------------------------------------------------------
static void keepalive_event(u64 ts, int delta)
{
  if (!capture_enabled)
    return;

  if (!g_opt.fold_empty)
  {
    write_keepalive(ts);
  }
  else if (capture_fold_buf_ptr)
  {
    capture_fold_count++;
    capture_fold_buf_ptr = 0;

    if (capture_fold_count == FOLD_LIMIT_LS_FS)
      stop_folding();

    fold_keepalive(ts, delta);
  }
  else
  {
    fold_keepalive(ts, delta);
  }

  check_capture_limit();
}

//-----------------------------------------------------------------------------
static void data_event(void)
{
  bool data_error = capture_crc_error || capture_data_error;
  bool allow_sof  = (CaptureSpeed_LS != g_opt.capture_speed);
  int pid = capture_data[0];

  if (!capture_enabled)
    return;

  line_state_event();

  if (capture_overflow || data_error || FOLD_BUF_SIZE == capture_fold_buf_ptr)
    stop_folding();

  if (capture_overflow)
    capture_info(capture_ts, "Hardware buffer overflow");

  if (capture_data_error)
    capture_info(capture_ts, "USB PHY error");

  if (data_error || !g_opt.fold_empty)
  {
    write_packet(capture_ts, capture_data, capture_size);
  }
  else if (capture_fold_buf_ptr)
  {
    if (PID_IN == pid || PID_NAK == pid)
    {
      fold_packet(capture_ts, capture_data, capture_size);
    }
    else if (PID_SOF == pid && allow_sof)
    {
      capture_fold_count++;
      capture_fold_buf_ptr = 0;

      if (capture_fold_count == ((CaptureSpeed_HS == g_opt.capture_speed) ? FOLD_LIMIT_HS : FOLD_LIMIT_LS_FS))
        stop_folding();

      fold_packet(capture_ts, capture_data, capture_size);
    }
    else
    {
      stop_folding();
      write_packet(capture_ts, capture_data, capture_size);
    }
  }
  else
  {
    if (PID_SOF == pid && allow_sof)
      fold_packet(capture_ts, capture_data, capture_size);
    else
      write_packet(capture_ts, capture_data, capture_size);
  }

  check_capture_limit();
}

//-----------------------------------------------------------------------------
static void desync_error(void)
{
  capture_info(capture_ts, "Error: protocol desynchronization, stopping the capture");

  char header[256] = {0};

  for (int i = 0; i < capture_size; i++)
  {
    char tmp[16];
    snprintf(tmp, sizeof(tmp), "%02x ", capture_data[i]);
    strcat(header, tmp);
  }

  capture_info(capture_ts, "Packet header: %s", header);
  exit(0);
}

//-----------------------------------------------------------------------------
static void check_header(int toggle, int zero)
{
  if (toggle == capture_toggle && 0 == zero)
    return;

  if (toggle != capture_toggle)
    capture_info(capture_ts, "Error: received toggle value %d, expected %d", toggle, capture_toggle);

  if (zero)
    capture_info(capture_ts, "Error: zero bit in the header is not zero");

  desync_error();
}

//-----------------------------------------------------------------------------
static void check_data_size(int size)
{
  if (DATA_HEADER_SIZE <= size && size <= MAX_DATA_SIZE)
    return;

  capture_info(capture_ts, "Error: invalid data size (%d)", size);
  desync_error();
}

//-----------------------------------------------------------------------------
static inline void capture_sm(u8 byte)
{
  if (capture_header && 0 == capture_data_ptr)
  {
    capture_status = (0 == (byte & HEADER_STATUS));
    capture_size   = capture_status ? STATUS_HEADER_SIZE : DATA_HEADER_SIZE;
  }

  capture_data[capture_data_ptr++] = byte;

  if (capture_data_ptr < capture_size)
    return;

  if (capture_header)
  {
    int ts     = ((capture_data[0] & 0xf) << 16) | (capture_data[1] << 8) | capture_data[2];
    int toggle = (capture_data[0] & HEADER_TOGGLE) ? 1 : 0;
    int zero   = (capture_data[0] & HEADER_ZERO) ? 1 : 0;

    check_header(toggle, zero);

    if (capture_data[0] & HEADER_TS_OVERFLOW)
      capture_ts_int += 0x100000;

    capture_ts = ((capture_ts_int | ts) * 100) / 6; // convert to ns
    capture_toggle = 1 - toggle;

    if ((capture_ts - capture_last_ts) > UPDATE_INTERVAL)
      timeout_event();

    if (capture_status)
    {
      int ls      = (capture_data[3] >> HEADER_LS_OFFS) & HEADER_LS_MASK;
      int vbus    = (capture_data[3] & HEADER_VBUS) ? 1 : 0;
      int trigger = (capture_data[3] & HEADER_TRIGGER) ? 1 : 0;
      int speed   = (capture_data[3] >> HEADER_SPEED_OFFS) & HEADER_SPEED_MASK;

      status_event(ls, vbus, trigger, speed);
    }
    else // data
    {
      int size = (((int)capture_data[3] & 0x7) << 8) | capture_data[4];

      check_data_size(size);

      capture_size       = size - DATA_HEADER_SIZE;
      capture_overflow   = (capture_data[3] & HEADER_OVERFLOW)   ? true : false;
      capture_crc_error  = (capture_data[3] & HEADER_CRC_ERROR)  ? true : false;
      capture_data_error = (capture_data[3] & HEADER_DATA_ERROR) ? true : false;
      capture_duration   = ((int)capture_data[5] << 8) | capture_data[6];
      capture_header     = (0 == capture_size);
    }
  }
  else // payload
  {
    capture_header = true;
    data_event();
  }

  capture_data_ptr = 0;
}

//-----------------------------------------------------------------------------
void capture_callback(u8 *data, int size)
{
  for (int i = 0; i < size; i++)
    capture_sm(data[i]);
}

//-----------------------------------------------------------------------------
bool capture_start(void)
{
  if (!g_opt.extcap_capture || !g_opt.extcap_fifo)
    return false;

  log_print("Opening file '%s'", g_opt.extcap_fifo);

  capture_fd = fopen(g_opt.extcap_fifo, "wb");
  os_check(capture_fd, "could not open FIFO pipe");

  log_print("Opening capture device");

  open_capture_device();

  usb_ctrl_init();

  usb_ctrl(CaptureCtrl_Enable, 0);
  usb_ctrl(CaptureCtrl_Reset, 1);

  usb_flush_data();

  usb_ctrl(CaptureCtrl_Speed0, g_opt.capture_speed & 1);
  usb_ctrl(CaptureCtrl_Speed1, g_opt.capture_speed & 2);

  usb_ctrl(CaptureCtrl_Reset, 0);
  usb_ctrl(CaptureCtrl_Enable, 1);

  log_print("Starting capture");

  write_file_header();
  write_usb_header();
  write_info_header();

  if (CaptureTrigger_Disabled == g_opt.capture_trigger)
  {
    capture_info(0, "Starting capture");
    capture_enabled = true;
  }
  else
    capture_info(capture_ts, "Waiting for a trigger");

  usb_data_transfer();

  return true;
}

