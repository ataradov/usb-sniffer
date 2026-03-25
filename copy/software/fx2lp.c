// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

/*- Includes ----------------------------------------------------------------*/
#include "os_common.h"
#include "fx2lp.h"
#include "usb.h"

/*- Definitions -------------------------------------------------------------*/
#define EEPROM_ADDR            0xa2
#define EEPROM_PAGE_SIZE       32 // Actually 64, but our protocol won't allow that
#define FX2LP_SIZE             16384
#define FX2LP_HEADER           12
#define FX2LP_FOOTER           5

/*- Variables ---------------------------------------------------------------*/

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void fx2lp_sram_upload(u8 *data, int size)
{
  u8 buf[USB_EP0_SIZE];
  int addr = 0;

  os_check(size <= FX2LP_SIZE, "fx2lp_sram_upload(): file is too big");

  usb_fx2lp_reset(true);

  addr = 0;

  while (size)
  {
    int sz = (size < USB_EP0_SIZE) ? size: USB_EP0_SIZE;

    usb_fx2lp_sram_write(addr, data, sz);
    usb_fx2lp_sram_read(addr, buf, sz);

    if (0 != memcmp(data, buf, sz))
      os_error("fx2lp_sram_upload(): verification failed");

    addr += sz;
    data += sz;
    size -= sz;
  }

  usb_fx2lp_reset(false);
}

//-----------------------------------------------------------------------------
static bool eeprom_request_valid(int addr, int size)
{
  return (addr < FX2LP_SIZE) && ((addr % EEPROM_PAGE_SIZE) == 0) && (size <= EEPROM_PAGE_SIZE);
}

//-----------------------------------------------------------------------------
static void fx2lp_eeprom_read(int addr, u8 *data, int size)
{
  u8 buf[sizeof(u16)];

  os_check(eeprom_request_valid(addr, size), "fx2lp_eeprom_read(): invalid request");

  buf[0] = addr >> 8;
  buf[1] = addr;

  usb_i2c_write(EEPROM_ADDR, buf, sizeof(u16));
  usb_i2c_read(EEPROM_ADDR, data, size);
}

//-----------------------------------------------------------------------------
static void fx2lp_eeprom_write(int addr, u8 *data, int size)
{
  u8 buf[sizeof(u16) + EEPROM_PAGE_SIZE];

  os_check(eeprom_request_valid(addr, size), "fx2lp_eeprom_write(): invalid request");

  buf[0] = addr >> 8;
  buf[1] = addr;

  memcpy(buf + sizeof(u16), data, size);

  usb_i2c_write(EEPROM_ADDR, buf, sizeof(u16) + size);

  os_sleep(7);
}

//-----------------------------------------------------------------------------
void fx2lp_eeprom_upload(u8 *data, int size)
{
  u8 buf[FX2LP_SIZE];
  int data_size = size;
  u32 addr = 0;

  size = ((FX2LP_HEADER + size + FX2LP_FOOTER) + (EEPROM_PAGE_SIZE-1)) & ~(EEPROM_PAGE_SIZE-1);

  os_check(size <= FX2LP_SIZE, "fx2lp_eeprom_upload(): file is too big");

  memset(buf, 0xff, sizeof(buf));

  buf[0]  = 0xc2;
  buf[7]  = 1; // 400 kHz I2C
  buf[8]  = data_size >> 8;
  buf[9]  = data_size;
  buf[10] = 0;
  buf[11] = 0;

  memcpy(buf + FX2LP_HEADER, data, data_size);

  buf[FX2LP_HEADER + data_size + 0] = 0x80;
  buf[FX2LP_HEADER + data_size + 1] = 0x01;
  buf[FX2LP_HEADER + data_size + 2] = 0xe6;
  buf[FX2LP_HEADER + data_size + 3] = 0x00;
  buf[FX2LP_HEADER + data_size + 4] = 0x00;

  while (size)
  {
    u8 tmp[EEPROM_PAGE_SIZE];

    fx2lp_eeprom_write(addr, &buf[addr], EEPROM_PAGE_SIZE);
    fx2lp_eeprom_read(addr, tmp, EEPROM_PAGE_SIZE);

    if (0 != memcmp(&buf[addr], tmp, EEPROM_PAGE_SIZE))
      os_error("fx2lp_eeprom_upload(): verification failed");

    addr += EEPROM_PAGE_SIZE;
    size -= EEPROM_PAGE_SIZE;
  }
}


