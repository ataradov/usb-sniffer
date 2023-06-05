// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

/*- Includes ----------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "fx2_regs.h"
#include "usb_sniffer.h"
#include "usb.h"
#include "usb_descriptors.h"
#include "usb.c"

/*- Definitions -------------------------------------------------------------*/
enum
{
  CMD_I2C_READ         = 0xb0,
  CMD_I2C_WRITE        = 0xb1,

  CMD_JTAG_ENABLE      = 0xc0,
  CMD_JTAG_REQUEST     = 0xc1,
  CMD_JTAG_RESPONSE    = 0xc2,

  CMD_CTRL             = 0xd0,
};

#define CTRL_CLK       IOA_0_b
#define CTRL_DATA      IOA_3_b

#define CTRL_CLK_OE    (1ul << 0) // PA
#define CTRL_DATA_OE   (1ul << 3) // PA

#define GPIO           IOA_7_b
#define GPIO_OE        (1ul << 7) // PA

#define JTAG_EN        IOA_1_b
#define JTAG_TMS       IOB_0_b
#define JTAG_TCK       IOB_1_b
#define JTAG_TDI       IOB_2_b
#define JTAG_TDO       IOB_3_b

#define JTAG_EN_OE     (1ul << 1) // PA
#define JTAG_TMS_OE    (1ul << 0) // PB
#define JTAG_TCK_OE    (1ul << 1) // PB
#define JTAG_TDI_OE    (1ul << 2) // PB
#define JTAG_TDO_OE    (1ul << 3) // PB, input

#define OEA_VALUE      JTAG_EN_OE | CTRL_CLK_OE | CTRL_DATA_OE | GPIO_OE
#define OEB_VALUE      JTAG_TMS_OE | JTAG_TCK_OE | JTAG_TDI_OE

/*- Variables ---------------------------------------------------------------*/

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void delay_1ms(void) __naked
{
  __asm
  mov dptr, #(-1200 & 0xffff)
  1$:
  inc dptr
  mov a, dpl
  orl a, dph
  jnz 1$
  ret
  __endasm;
}

//-----------------------------------------------------------------------------
void delay_ms(uint8_t ms)
{
  do
  {
    delay_1ms();
  } while (--ms > 0);
}

//-----------------------------------------------------------------------------
static inline bool i2c_stop(void)
{
  I2CS = I2CS_STOP;
  while (I2CS & I2CS_STOP);
  return false;
}

//-----------------------------------------------------------------------------
static bool i2c_read(uint8_t addr, uint8_t size)
{
  uint8_t i;

  if (0 == size)
  {
    I2CS = I2CS_START | I2CS_STOP;
    I2DAT = addr;
    while (0 == (I2CS & I2CS_DONE));
    while (I2CS & I2CS_STOP);
    return (I2CS & I2CS_ACK) ? true : false;
  }
  else if (1 == size)
  {
    I2CS = I2CS_START;

    I2DAT = addr;
    while (0 == (I2CS & I2CS_DONE));
    if (0 == (I2CS & I2CS_ACK))
      return i2c_stop();

    I2CS = I2CS_LASTRD;
    (void)I2DAT;
    while (0 == (I2CS & I2CS_DONE));

    I2CS = I2CS_STOP;
    EP0BUF[0] = I2DAT;
    while (I2CS & I2CS_STOP);
  }
  else
  {
    I2CS = I2CS_START;

    I2DAT = addr;
    while (0 == (I2CS & I2CS_DONE));
    if (0 == (I2CS & I2CS_ACK))
      return i2c_stop();

    (void)I2DAT;
    while (0 == (I2CS & I2CS_DONE));

    for (i = 0; i < (size-2); i++)
    {
      EP0BUF[i] = I2DAT;
      while (0 == (I2CS & I2CS_DONE));
    }

    I2CS = I2CS_LASTRD;
    EP0BUF[size-2] = I2DAT;
    while (0 == (I2CS & I2CS_DONE));

    I2CS = I2CS_STOP;
    EP0BUF[size-1] = I2DAT;
    while (I2CS & I2CS_STOP);
  }

  return true;
}

//-----------------------------------------------------------------------------
static bool i2c_write(uint8_t addr, uint8_t size)
{
  uint8_t i;

  I2CS = I2CS_START;

  I2DAT = addr;
  while (0 == (I2CS & I2CS_DONE));
  if (0 == (I2CS & I2CS_ACK))
    return i2c_stop();

  for (i = 0; i < size; i++)
  {
    I2DAT = EP0BUF[i];
    while (0 == (I2CS & I2CS_DONE));
    if (0 == (I2CS & I2CS_ACK))
      return i2c_stop();
  }

  i2c_stop();

  return true;
}

//-----------------------------------------------------------------------------
static void jtag_transfer(uint8_t count)
{
  uint8_t full = count >> 2;
  uint8_t i;

  for (i = 0; i < full; i++)
  {
    B = EP0BUF[i];

    JTAG_TMS = B_0_b;
    JTAG_TDI = B_1_b;
    JTAG_TCK = 1;
    B_0_b = JTAG_TDO;
    JTAG_TCK = 0;

    JTAG_TMS = B_2_b;
    JTAG_TDI = B_3_b;
    JTAG_TCK = 1;
    B_1_b = JTAG_TDO;
    JTAG_TCK = 0;

    JTAG_TMS = B_4_b;
    JTAG_TDI = B_5_b;
    JTAG_TCK = 1;
    B_2_b = JTAG_TDO;
    JTAG_TCK = 0;

    JTAG_TMS = B_6_b;
    JTAG_TDI = B_7_b;
    JTAG_TCK = 1;
    B_3_b = JTAG_TDO;
    JTAG_TCK = 0;

    EP0BUF[i] = B;
  }

  if (count & 3)
  {
    B = EP0BUF[full];

    JTAG_TMS = B_0_b;
    JTAG_TDI = B_1_b;
    JTAG_TCK = 1;
    B_0_b = JTAG_TDO;
    B_1_b = 0;
    JTAG_TCK = 0;

    if (count & 2)
    {
      JTAG_TMS = B_2_b;
      JTAG_TDI = B_3_b;
      JTAG_TCK = 1;
      B_1_b = JTAG_TDO;
      B_2_b = 0;
      JTAG_TCK = 0;

      if (count & 1)
      {
        JTAG_TMS = B_4_b;
        JTAG_TDI = B_5_b;
        JTAG_TCK = 1;
        B_2_b = JTAG_TDO;
        JTAG_TCK = 0;
      }
    }
    else
    {
      B_2_b = 0;
    }

    B_3_b = 0;
    EP0BUF[full] = B;
  }
}

//-----------------------------------------------------------------------------
static void ctrl_transfer(uint8_t value)
{
  B = value;

  // Start
  CTRL_DATA = 0;

  CTRL_CLK  = 0;
  CTRL_DATA = B_0_b;
  CTRL_CLK  = 1;

  CTRL_CLK  = 0;
  CTRL_DATA = B_1_b;
  CTRL_CLK  = 1;

  CTRL_CLK  = 0;
  CTRL_DATA = B_2_b;
  CTRL_CLK  = 1;

  CTRL_CLK  = 0;
  CTRL_DATA = B_3_b;
  CTRL_CLK  = 1;

  CTRL_CLK  = 0;
  CTRL_DATA = B_4_b;
  CTRL_CLK  = 1;

  // Stop
  CTRL_DATA = 0;
  CTRL_DATA = 1;
}

//-----------------------------------------------------------------------------
static inline void jtag_enable(void)
{
  IFCONFIG = IFCONFIG_IFCLKSRC | IFCONFIG_IFCLKOE | IFCONFIG_IFCFG_PORTS;
  SYNCDELAY;

  JTAG_EN = 1;
}

//-----------------------------------------------------------------------------
static inline void jtag_disable(void)
{
  JTAG_EN = 0;

  IFCONFIG = IFCONFIG_IFCLKSRC | IFCONFIG_IFCLKOE | IFCONFIG_IFCFG_FIFO; // IFCONFIG_IFCLKPOL
  SYNCDELAY;
}

//-----------------------------------------------------------------------------
static bool handle_vendor_request(void)
{
  if (USB_CMD(IN, DEVICE, VENDOR) == bmRequestType && CMD_I2C_READ == bRequest)
  {
    if (i2c_read(wValueL, wLengthL))
      usb_control_send(wLengthL);

    return true;
  }
  else if (USB_CMD(OUT, DEVICE, VENDOR) == bmRequestType && CMD_I2C_WRITE == bRequest)
  {
    usb_control_recv();

    if (!i2c_write(wValueL, wLengthL))
      usb_control_stall();

    return true;
  }

  else if (USB_CMD(OUT, DEVICE, VENDOR) == bmRequestType && CMD_JTAG_ENABLE == bRequest)
  {
    if (wValueL)
      jtag_enable();
    else
      jtag_disable();

    return true;
  }
  else if (USB_CMD(OUT, DEVICE, VENDOR) == bmRequestType && CMD_JTAG_REQUEST == bRequest)
  {
    usb_control_recv();
    jtag_transfer(wValueL);

    return true;
  }
  else if (USB_CMD(IN, DEVICE, VENDOR) == bmRequestType && CMD_JTAG_RESPONSE == bRequest)
  {
    usb_control_send(wLengthL);

    return true;
  }

  else if (USB_CMD(OUT, DEVICE, VENDOR) == bmRequestType && CMD_CTRL == bRequest)
  {
    ctrl_transfer(wValueL);

    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
void reset_endpoints(void)
{
  EP1OUTCFG = 0;
  EP1INCFG  = 0;
  EP2CFG    = 0;
  EP4CFG    = 0;
  EP6CFG    = 0;
  EP8CFG    = 0;
}

//-----------------------------------------------------------------------------
void setup_endpoints(void)
{
  EP2CFG = EPCFG_VALID | EPCFG_TYPE_BULK | EPCFG_DIR_IN | EPCFG_SIZE_512 | EPCFG_BUF_QUAD;
  SYNCDELAY;
}

//-----------------------------------------------------------------------------
static inline void sys_init(void)
{
  CPUCS = CPUCS_CLKSPD_48_MHZ;
  SYNCDELAY;

  REVCTL = REVCTL_DYN_OUT | REVCTL_ENH_PKT;
  SYNCDELAY;

  EP0BCH = 0;
  SYNCDELAY;

  I2CTL = I2CTL_400KHZ;

  reset_endpoints();

  OEA = OEA_VALUE;
  OEB = OEB_VALUE;

  JTAG_EN  = 0;
  JTAG_TMS = 0;
  JTAG_TDI = 0;
  JTAG_TCK = 0;

  //---------------
  // Reset the control interface
  CTRL_CLK  = 1;
  CTRL_DATA = 0;
  CTRL_DATA = 1;

  //---------------
  FIFORESET = FIFORESET_NAKALL | 0;
  SYNCDELAY;
  FIFORESET = FIFORESET_NAKALL | 2;
  SYNCDELAY;
  FIFORESET = FIFORESET_NAKALL | 4;
  SYNCDELAY;
  FIFORESET = FIFORESET_NAKALL | 6;
  SYNCDELAY;
  FIFORESET = FIFORESET_NAKALL | 8;
  SYNCDELAY;
  FIFORESET = 0;
  SYNCDELAY;

  //EP2CFG = EPCFG_VALID | EPCFG_TYPE_BULK | EPCFG_DIR_IN | EPCFG_SIZE_512 | EPCFG_BUF_QUAD;
  //SYNCDELAY;

  EP2FIFOCFG = EPFIFOCFG_WORDWIDE | EPFIFOCFG_AUTOIN;
  SYNCDELAY;

  PINFLAGSAB = PINFLAGSAB_FLAGA_EP2EF | PINFLAGSAB_FLAGB_EP2FF;
  SYNCDELAY;
  PINFLAGSCD = PINFLAGSCD_FLAGC_EP2PF;
  SYNCDELAY;

  PORTACFG = 0x00;
  SYNCDELAY;

  FIFOPINPOLAR = FIFOPINPOLAR_FF | FIFOPINPOLAR_EF | FIFOPINPOLAR_SLWR | FIFOPINPOLAR_SLRD |
      FIFOPINPOLAR_SLOE | FIFOPINPOLAR_PKTEND;
  SYNCDELAY;

  EP2AUTOINLENH = 0x02; // 512-bytes
  SYNCDELAY;
  EP2AUTOINLENL = 0x00;
  SYNCDELAY;

  EP2FIFOPFL = 0x00;
  SYNCDELAY;
  EP2FIFOPFH = 0x80;
  SYNCDELAY;

  jtag_disable();
}

//-----------------------------------------------------------------------------
void main(void)
{
  sys_init();

  usb_renumerate();

  while (1)
  {
    usb_task();
  }
}


