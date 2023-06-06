// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

/*- Includes ----------------------------------------------------------------*/
#include "os_common.h"
#include "usb_sniffer.h"
#include "usb.h"
#include "fpga.h"

/*- Definitions -------------------------------------------------------------*/
#define LCMXO2_2000HC_IDCODE   0x012bb043

#define BITSTREAM_SIGNATURE    "LCMXO2-2000HC"
#define FPGA_IDCODE            LCMXO2_2000HC_IDCODE
#define MAX_CONFIG_SIZE        (512*1024)

enum
{
  CMD_IDCODE_PUB               = 0xe0,
  CMD_ISC_ENABLE_X             = 0x74,
  CMD_ISC_ENABLE               = 0xc6,
  CMD_LSC_CHECK_BUSY           = 0xf0,
  CMD_LSC_READ_STATUS          = 0x3c,
  CMD_ISC_ERASE                = 0x0e,
  CMD_LSC_ERASE_TAG            = 0xcb,
  CMD_LSC_INIT_ADDRESS         = 0x46,
  CMD_LSC_WRITE_ADDRESS        = 0xb4,
  CMD_LSC_PROG_INCR_NV         = 0x70,
  CMD_LSC_INIT_ADDR_UFM        = 0x47,
  CMD_LSC_PROG_TAG             = 0xc9,
  CMD_ISC_PROGRAM_USERCODE     = 0xc2,
  CMD_USERCODE                 = 0xc0,
  CMD_LSC_PROG_FEATURE         = 0xe4,
  CMD_LSC_READ_FEATURE         = 0xe7,
  CMD_LSC_PROG_FEABITS         = 0xf8,
  CMD_LSC_READ_FEABITS         = 0xfb,
  CMD_LSC_READ_INCR_NV         = 0x73,
  CMD_LSC_READ_UFM             = 0xca,
  CMD_ISC_PROGRAM_DONE         = 0x5e,
  CMD_LSC_PROG_OTP             = 0xf9,
  CMD_LSC_READ_OTP             = 0xfa,
  CMD_ISC_DISABLE              = 0x26,
  CMD_ISC_NOOP                 = 0xff,
  CMD_LSC_REFRESH              = 0x79,
  CMD_ISC_PROGRAM_SECURITY     = 0xce,
  CMD_ISC_PROGRAM_SECPLUS      = 0xcf,
  CMD_UIDCODE_PUB              = 0x19,
  CMD_LSC_BITSTREAM_BURST      = 0x7a,
};

#define ISC_ENABLE_SRAM        0x00
#define ISC_ENABLE_FLASH       0x08

#define ISC_ERASE_SRAM         (1 << 0)
#define ISC_ERASE_FEATURE      (1 << 1)
#define ISC_ERASE_CFG          (1 << 2)
#define ISC_ERASE_UFM          (1 << 3)
#define ISC_ERASE_ALL          (ISC_ERASE_SRAM | ISC_ERASE_FEATURE | ISC_ERASE_CFG | ISC_ERASE_UFM)
#define ISC_ERASE_ALL_NV       (ISC_ERASE_FEATURE | ISC_ERASE_CFG | ISC_ERASE_UFM)

#define STATUS_BUSY            (1 << 12)
#define STATUS_FAIL            (1 << 13)

#define FLASH_ROW_SIZE         128 // bits

#define MAX_COUNT_IN_REQUEST   255

/*- Variables ---------------------------------------------------------------*/
static u8 g_jtag_buf[MAX_COUNT_IN_REQUEST];
static int g_jtag_count = 0;

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
static void jtag_sync(void)
{
  if (g_jtag_count)
  {
    usb_jtag_request(g_jtag_buf, g_jtag_count);
    g_jtag_count = 0;
  }
}

//-----------------------------------------------------------------------------
static void jtag_clk(int tdi, int tms)
{
  g_jtag_buf[g_jtag_count++] = (tdi << 1) | tms;

  if (MAX_COUNT_IN_REQUEST == g_jtag_count)
    jtag_sync();
}

//-----------------------------------------------------------------------------
static void jtag_reset(void)
{
  for (int i = 0; i < 16; i++)
    jtag_clk(0, 1);

  jtag_clk(0, 0);
}

//-----------------------------------------------------------------------------
static void jtag_write_ir(int ir)
{
  jtag_clk(0, 1);
  jtag_clk(0, 1);
  jtag_clk(0, 0);
  jtag_clk(0, 0);

  for (int i = 0; i < 8; i++)
    jtag_clk((ir >> i) & 1, i == 7);

  jtag_clk(0, 1);
  jtag_clk(0, 0);
}

//-----------------------------------------------------------------------------
static void jtag_write_dr(u8 *data, int size)
{
  jtag_clk(0, 1);
  jtag_clk(0, 0);
  jtag_clk(0, 0);

  for (int i = 0; i < size; i++)
    jtag_clk((data[i / 8] >> (i % 8)) & 1, i == (size-1));

  jtag_clk(0, 1);
  jtag_clk(0, 0);
}

//-----------------------------------------------------------------------------
static void jtag_read_dr(u8 *data, int size)
{
  jtag_clk(0, 1);
  jtag_clk(0, 0);
  jtag_clk(0, 0);
  jtag_sync();

  for (int i = 0; i < size; i++)
    jtag_clk(0, i == (size-1));

  jtag_sync();
  usb_jtag_response(data, size);

  jtag_clk(0, 1);
  jtag_clk(0, 0);
}

//-----------------------------------------------------------------------------
static void jtag_run(int count)
{
  for (int i = 0; i < count; i++)
    jtag_clk(0, 0);
}

//-----------------------------------------------------------------------------
void fpga_enable(void)
{
  u32 idcode;

  usb_jtag_enable(true);
  jtag_reset();
  jtag_sync();

  idcode = fpga_read_idcode();

  if (FPGA_IDCODE != idcode)
    os_error("incorrect FPGA IDCODE (0x%08x)", idcode);
}

//-----------------------------------------------------------------------------
void fpga_disable(void)
{
  jtag_reset();
  jtag_sync();
  usb_jtag_enable(false);
}

//-----------------------------------------------------------------------------
u32 fpga_read_idcode(void)
{
  u32 idcode = 0;

  jtag_write_ir(CMD_IDCODE_PUB);
  jtag_read_dr((u8 *)&idcode, 32);
  jtag_sync();

  return idcode;
}

//-----------------------------------------------------------------------------
u64 fpga_read_traceid(void)
{
  u64 traceid = 0;

  jtag_write_ir(CMD_UIDCODE_PUB);
  jtag_read_dr((u8 *)&traceid, 64);

  return traceid;
}

//-----------------------------------------------------------------------------
static bool bitstream_valid(u8 *data, int size)
{
  if (size < 1024)
    return false;

  if (NULL == find_str(data, 1024, BITSTREAM_SIGNATURE))
    return false;

  return true;
}

//-----------------------------------------------------------------------------
void fpga_program_sram(u8 *data, int size)
{
  if (!bitstream_valid(data, size))
    os_error("malformed BIT file: device signature not found");

  jtag_write_ir(CMD_ISC_ENABLE);
  jtag_write_dr((u8[]){ ISC_ENABLE_SRAM }, 8);
  jtag_run(8);

  jtag_write_ir(CMD_ISC_ERASE);
  jtag_write_dr((u8[]){ ISC_ERASE_SRAM }, 8);
  jtag_run(8);

  jtag_write_ir(CMD_LSC_BITSTREAM_BURST);
  jtag_run(8);

  jtag_clk(0, 1);
  jtag_clk(0, 0);
  jtag_clk(0, 0);

  for (int i = 0; i < size; i++)
  {
    for (int j = 7; j >= 0; j--)
      jtag_clk((data[i] >> j) & 1, (i == (size-1)) && (j == 0));
  }

  jtag_clk(0, 1);
  jtag_clk(0, 0);

  jtag_run(100);

  jtag_write_ir(CMD_ISC_DISABLE);
  jtag_run(8);

  jtag_write_ir(CMD_ISC_NOOP);
  jtag_run(100);
  jtag_sync();
}

//-----------------------------------------------------------------------------
static int parse_jed_file(u8 *data, int size, u8 *config, u16 *feabits, u64 *feature)
{
  static char *start_text = "L000000";
  static char *fr_text = "NOTE FEATURE_ROW*";
  u8  *ptr;
  int bit_count = 0;
  int fr_bit_count = 0;
  int offset;

  // This is a very primitive parser. It expects a fixed format and will fail
  // if it finds something unexpected. It will also ignore EBR initialization data.

  if (!bitstream_valid(data, size))
    os_error("malformed JED file: device signature not found");

  ptr = find_str(data, size, start_text);

  if (NULL == ptr)
    os_error("malformed JED file: no 'L000000' found");

  offset = ptr - data + strlen(start_text);

  memset(config, 0, MAX_CONFIG_SIZE);

  for (; offset < size; offset++)
  {
    if (data[offset] == '*')
      break;

    if (data[offset] == '0' || data[offset] == '1')
    {
      config[bit_count / 8] |= ((data[offset] - '0') << (bit_count % 8));
      bit_count++;
      os_check(bit_count < (MAX_CONFIG_SIZE*8), "malformed JED file: configuration data is too big");
    }
  }

  if (offset == size)
    os_error("malformed JED file: no field terminator found");

  if (bit_count % FLASH_ROW_SIZE)
    os_error("malformed JED file: size of the configuration data must be a multiple of 128");

  ptr = find_str(data, size, fr_text);

  if (NULL == ptr)
    os_error("malformed JED file: no feature row found");

  offset = ptr - data + strlen(fr_text);

  *feature = 0;
  *feabits = 0;

  for (; offset < size; offset++)
  {
    if (data[offset] == '*')
      break;

    if (data[offset] == '0' || data[offset] == '1')
    {
      int bit = data[offset] - '0';

      os_check(fr_bit_count < (64 + 16), "malformed JED file: feature row data is too big");

      if (fr_bit_count < 64)
        *feature |= ((u64)bit << fr_bit_count);
      else
        *feabits |= (bit << (fr_bit_count-64));

      fr_bit_count++;
    }
  }

  if (offset == size)
    os_error("malformed JED file: no field terminator found");

  if (fr_bit_count != (64 + 16))
    os_error("malformed JED file: invalid feature row size");

  return bit_count;
}

//-----------------------------------------------------------------------------
static void poll_busy_flag(void)
{
  u32 status = 0;
  u8 busy = 1;

  while (busy)
  {
    jtag_write_ir(CMD_LSC_CHECK_BUSY);
    jtag_read_dr(&busy, 1);
  }

  jtag_write_ir(CMD_LSC_READ_STATUS);
  jtag_read_dr((u8 *)&status, 32);
  jtag_run(8);

  if (status & STATUS_BUSY)
    os_error("poll_busy_flag(): busy");

  if (status & STATUS_FAIL)
    os_error("poll_busy_flag(): fail");
}

//-----------------------------------------------------------------------------
void fpga_erase_flash(void)
{
  // Erase the SRAM
  jtag_write_ir(CMD_ISC_ENABLE);
  jtag_write_dr((u8[]){ ISC_ENABLE_SRAM }, 8);
  jtag_run(8);

  jtag_write_ir(CMD_ISC_ERASE);
  jtag_write_dr((u8[]){ ISC_ERASE_SRAM }, 8);
  jtag_run(8);

  jtag_write_ir(CMD_ISC_NOOP);

  // Erase the flash
  jtag_write_ir(CMD_ISC_ENABLE);
  jtag_write_dr((u8[]){ ISC_ENABLE_FLASH }, 8);
  jtag_run(8);

  jtag_write_ir(CMD_ISC_ERASE);
  jtag_write_dr((u8[]){ ISC_ERASE_ALL_NV }, 8);
  jtag_run(8);

  poll_busy_flag();
}

//-----------------------------------------------------------------------------
void fpga_program_flash(u8 *data, int size)
{
  u64 feature, feature_verify;
  u16 feabits, feabits_verify;
  u8  config[MAX_CONFIG_SIZE];
  int config_size, row_count;

  config_size = parse_jed_file(data, size, config, &feabits, &feature);
  row_count = config_size / FLASH_ROW_SIZE;

  printf("Erasing flash\n");

  fpga_erase_flash();

  // Program configuration data
  printf("Programming configuration data ");

  jtag_write_ir(CMD_LSC_INIT_ADDRESS);
  jtag_run(8);

  for (int row = 0; row < row_count; row++)
  {
    jtag_write_ir(CMD_LSC_PROG_INCR_NV);
    jtag_write_dr(&config[row * 16], FLASH_ROW_SIZE);
    jtag_run(1000);
    poll_busy_flag();

    if (row % 256 == 0)
    {
      printf(".");
      fflush(stdout);
    }
  }

  printf("\n");

  // Verify configuration data
  printf("Verifying configuration data\n");

  jtag_write_ir(CMD_LSC_INIT_ADDRESS);
  jtag_run(8);

  jtag_write_ir(CMD_LSC_READ_INCR_NV);
  jtag_run(8);

  for (int row = 0; row < row_count; row++)
  {
    u8 tmp[16];
    jtag_read_dr(tmp, FLASH_ROW_SIZE);
    jtag_run(8);

    if (memcmp(tmp, &config[row * 16], 16))
      os_error("configuration verification failed");
  }

  // Program and verify Feature Row
  printf("Programming and verifying Feature Row\n");

  jtag_write_ir(CMD_LSC_INIT_ADDRESS);
  jtag_run(8);

  jtag_write_ir(CMD_LSC_PROG_FEATURE);
  jtag_write_dr((u8 *)&feature, 64);
  jtag_run(8);

  poll_busy_flag();

  jtag_write_ir(CMD_LSC_READ_FEATURE);
  jtag_read_dr((u8 *)&feature_verify, 64);
  jtag_run(8);

  os_check(feature_verify == feature, "Feature Row verification failed");

  // Program and verify FEABITS
  printf("Programming and verifying FEABITS\n");

  jtag_write_ir(CMD_LSC_PROG_FEABITS);
  jtag_write_dr((u8 *)&feabits, 16);
  jtag_run(8);

  poll_busy_flag();

  jtag_write_ir(CMD_LSC_READ_FEABITS);
  jtag_run(8);
  jtag_read_dr((u8 *)&feabits_verify, 16);

  os_check(feabits_verify == feabits, "FEABITS verification failed");

  // Exit programming mode
  printf("Exiting programming mode\n");

  jtag_write_ir(CMD_ISC_PROGRAM_DONE);
  jtag_run(1000);
  poll_busy_flag();

  jtag_write_ir(CMD_ISC_DISABLE);
  jtag_run(8);

  jtag_write_ir(CMD_ISC_NOOP);
  jtag_run(100);

  jtag_write_ir(CMD_LSC_REFRESH);
  jtag_run(8);

  jtag_write_ir(CMD_ISC_NOOP);
  jtag_run(100);
  jtag_sync();
}


