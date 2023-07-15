// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

/*- Includes ----------------------------------------------------------------*/
#include <libusb.h>
#include "os_common.h"
#include "capture.h"
#include "usb_sniffer.h"
#include "usb.h"

/*- Definitions -------------------------------------------------------------*/
#define CPUCS_ADDR     0xe600
#define TIMEOUT        250
#define CTRL_REG_SIZE  4

enum
{
  CMD_FX2LP_REQUEST    = 0xa0,

  CMD_I2C_READ         = 0xb0,
  CMD_I2C_WRITE        = 0xb1,

  CMD_JTAG_ENABLE      = 0xc0,
  CMD_JTAG_REQUEST     = 0xc1,
  CMD_JTAG_RESPONSE    = 0xc2,

  CMD_CTRL             = 0xd0,
};

#define MAX_COUNT_IN_REQUEST   255

#define DATA_ENDPOINT          0x82
#define DATA_ENDPOINT_SIZE     512
#define TRANSFER_SIZE          (DATA_ENDPOINT_SIZE * 2000)
#define TRANSFER_COUNT         4
#define TRANSFER_TIMEOUT       250 // ms

/*- Variables ---------------------------------------------------------------*/
static libusb_device_handle *g_usb_handle = NULL;
static struct libusb_transfer *g_transfers[TRANSFER_COUNT];
static u8 *g_buffers[TRANSFER_COUNT];
static bool g_speed_test = false;
static u64 g_speed_test_time;
static int g_speed_test_size;
static u64 g_speed_test_count;
static u64 g_logged_delta = 0;

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
static void usb_check_error(int error, const char *text)
{
  if (error < 0)
    os_error("%s: %s\n", text, libusb_error_name(error));
}

//-----------------------------------------------------------------------------
void usb_init(void)
{
  int rc = libusb_init(NULL);
  usb_check_error(rc, "libusb_init()");
}

//-----------------------------------------------------------------------------
bool usb_open(int vid, int pid)
{
  libusb_device **usb_devices;
  int rc, count;

  count = libusb_get_device_list(NULL, &usb_devices);
  usb_check_error(count, "libusb_get_device_list()");

  for (int i = 0; i < count; i++)
  {
    libusb_device *dev = usb_devices[i];
    struct libusb_device_descriptor desc;

    rc = libusb_get_device_descriptor(dev, &desc);
    usb_check_error(rc, "libusb_get_device_descriptor()");

    if (desc.idVendor == vid && desc.idProduct == pid)
    {
      libusb_device_handle *handle;

      rc = libusb_open(dev, &handle);
      usb_check_error(rc, "libusb_open()");

      g_usb_handle = handle;
    }
  }

  libusb_free_device_list(usb_devices, 1);

  if (!g_usb_handle)
    return false;

  libusb_set_auto_detach_kernel_driver(g_usb_handle, 1);

  rc = libusb_claim_interface(g_usb_handle, 0);
  usb_check_error(rc, "libusb_claim_interface()");

  return true;
}

//-----------------------------------------------------------------------------
void usb_close(void)
{
  if (g_usb_handle)
    libusb_close(g_usb_handle);

  libusb_exit(NULL);
}

//-----------------------------------------------------------------------------
void usb_fx2lp_reset(bool reset)
{
  int rc;
  u8 cpucs = reset;

  rc = libusb_control_transfer(g_usb_handle,
    LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
    CMD_FX2LP_REQUEST, CPUCS_ADDR, 0/*wIndex*/, &cpucs, sizeof(cpucs), TIMEOUT);

  usb_check_error(rc, "usb_fx2lp_reset()");
}

//-----------------------------------------------------------------------------
void usb_fx2lp_sram_read(int addr, u8 *data, int size)
{
  int rc;

  rc = libusb_control_transfer(g_usb_handle,
    LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
    CMD_FX2LP_REQUEST, addr, 0/*wIndex*/, data, size, TIMEOUT);

  usb_check_error(rc, "usb_fx2lp_sram_read()");
}

//-----------------------------------------------------------------------------
void usb_fx2lp_sram_write(int addr, u8 *data, int size)
{
  int rc;

  rc = libusb_control_transfer(g_usb_handle,
    LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
    CMD_FX2LP_REQUEST, addr, 0/*wIndex*/, data, size, TIMEOUT);

  usb_check_error(rc, "usb_fx2lp_sram_write()");
}

//-----------------------------------------------------------------------------
void usb_i2c_read(int addr, u8 *data, int size)
{
  int rc;

  rc = libusb_control_transfer(g_usb_handle,
    LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
    CMD_I2C_READ, addr | 1, 0/*wIndex*/, data, size, TIMEOUT);

  usb_check_error(rc, "usb_i2c_read()");
}

//-----------------------------------------------------------------------------
void usb_i2c_write(int addr, u8 *data, int size)
{
  int rc;

  rc = libusb_control_transfer(g_usb_handle,
    LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
    CMD_I2C_WRITE, addr, 0/*wIndex*/, data, size, TIMEOUT);

  usb_check_error(rc, "usb_i2c_write()");
}

//-----------------------------------------------------------------------------
void usb_jtag_enable(bool enable)
{
  int rc;

  rc = libusb_control_transfer(g_usb_handle,
    LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
    CMD_JTAG_ENABLE, enable/*wValue*/, 0/*wIndex*/, NULL, 0, TIMEOUT);

  usb_check_error(rc, "usb_jtag_enable()");
}

//-----------------------------------------------------------------------------
void usb_jtag_request(u8 *data, int count)
{
  int rc;
  u8 buf[64];

  os_assert(0 < count && count <= MAX_COUNT_IN_REQUEST);

  memset(buf, 0, sizeof(buf));

  for (int i = 0; i < count; i++)
    buf[i / 4] |= (data[i] << ((i % 4) * 2));

  rc = libusb_control_transfer(g_usb_handle,
    LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
    CMD_JTAG_REQUEST, count/*wValue*/, 0/*wIndex*/, buf, (count + 3) / 4, TIMEOUT);

  usb_check_error(rc, "usb_jtag_request()");
}

//-----------------------------------------------------------------------------
void usb_jtag_response(u8 *data, int count)
{
  int rc;
  u8 buf[64];

  os_assert(count <= MAX_COUNT_IN_REQUEST);

  memset(data, 0, (count + 7) / 8);

  rc = libusb_control_transfer(g_usb_handle,
    LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
    CMD_JTAG_RESPONSE, 0/*wValue*/, 0/*wIndex*/, buf, (count + 3) / 4, TIMEOUT);

  usb_check_error(rc, "usb_jtag_response()");

  for (int i = 0; i < (count + 3) / 4; i++)
    data[i / 2] |= ((buf[i] & 0x0f) << ((i % 2) * 4));
}

//-----------------------------------------------------------------------------
void usb_ctrl(int index, int value)
{
  int rc;

  value = index | ((value ? 1 : 0) << CTRL_REG_SIZE);

  rc = libusb_control_transfer(g_usb_handle,
    LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
    CMD_CTRL, value/*wValue*/, 0/*wIndex*/, NULL, 0, TIMEOUT);

  usb_check_error(rc, "usb_ctrl()");
}

//-----------------------------------------------------------------------------
void usb_flush_data(void)
{
  int rc, size;
  u8 buf[DATA_ENDPOINT_SIZE];

  for (int k = 0; k < 100; k++)
  {
    rc = libusb_bulk_transfer(g_usb_handle, DATA_ENDPOINT, buf, sizeof(buf), &size, 20);

    if (rc == LIBUSB_ERROR_TIMEOUT)
      break;
    else
      usb_check_error(rc, "libusb_bulk_transfer()");
  }
}

//-----------------------------------------------------------------------------
static void LIBUSB_CALL usb_capture_callback(struct libusb_transfer *transfer)
{
  int rc;

  if (LIBUSB_TRANSFER_TIMED_OUT == transfer->status)
    {}
  else if (LIBUSB_TRANSFER_COMPLETED != transfer->status)
    os_error("usb_capture_callback(): %d\n", transfer->status);

  if (g_speed_test)
  {
    u16 *data = (u16 *)transfer->buffer;
    int count = transfer->actual_length / sizeof(u16);
    u64 time, delta;

    for (int i = 0; i < count; i++)
      os_check(data[i] == os_rand16(0), "data error during the speed test on count %d", g_speed_test_count + i);

    g_speed_test_count += count;
    g_speed_test_size  += transfer->actual_length;

    time = os_get_time();
    delta = time - g_speed_test_time;

    if (delta > 1000)
    {
      f64 speed = (f64)g_speed_test_size / ((f64)delta / 1000.0) / 1000000.0;

      printf("Transfer rate: %5.2f MB/s\n", speed);

      g_speed_test_size = 0;
      g_speed_test_time = time;
    }
  }
  else
  {
    u64 delta = os_get_time();

    capture_callback(transfer->buffer, transfer->actual_length);

    delta = os_get_time() - delta;

    if (delta > g_logged_delta)
    {
      g_logged_delta = delta;
      log_print("Processing time = %d ms (size = %d bytes)", (int)delta, (int)transfer->actual_length);
    }
  }

  rc = libusb_submit_transfer(transfer);
  usb_check_error(rc, "libusb_submit_transfer() in usb_capture_callback()");
}

//-----------------------------------------------------------------------------
void usb_data_transfer(void)
{
  for (int i = 0; i < TRANSFER_COUNT; i++)
  {
    int rc;

    g_buffers[i]   = os_alloc(TRANSFER_SIZE);
    g_transfers[i] = libusb_alloc_transfer(0);
    os_check(g_transfers[i], "libusb_alloc_transfer()");

    libusb_fill_bulk_transfer(g_transfers[i], g_usb_handle, DATA_ENDPOINT,
        g_buffers[i], TRANSFER_SIZE, usb_capture_callback, NULL, TRANSFER_TIMEOUT);

    rc = libusb_submit_transfer(g_transfers[i]);
    usb_check_error(rc, "libusb_submit_transfer()");
  }

  while (1)
  {
    libusb_handle_events(NULL);
  }
}

//-----------------------------------------------------------------------------
void usb_ctrl_init(void)
{
  usb_ctrl(CaptureCtrl_Reset,  1);
  usb_ctrl(CaptureCtrl_Enable, 0);
  usb_ctrl(CaptureCtrl_Test,   0);
  usb_ctrl(CaptureCtrl_Speed0, 1);
  usb_ctrl(CaptureCtrl_Speed0, 0);
  usb_ctrl(CaptureCtrl_Speed1, 1);
  usb_ctrl(CaptureCtrl_Speed1, 0);
}

//-----------------------------------------------------------------------------
void usb_speed_test(void)
{
  usb_ctrl_init();

  usb_ctrl(CaptureCtrl_Reset, 1);
  usb_ctrl(CaptureCtrl_Test, 1);

  usb_flush_data();

  usb_ctrl(CaptureCtrl_Reset, 0);

  g_speed_test = true;
  g_speed_test_time  = os_get_time();
  g_speed_test_size  = 0;
  g_speed_test_count = 0;

  usb_data_transfer();
}


