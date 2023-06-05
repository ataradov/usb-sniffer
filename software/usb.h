// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

#ifndef _USB_H_
#define _USB_H_

/*- Includes ----------------------------------------------------------------*/
#include "os_common.h"

/*- Definitions -------------------------------------------------------------*/
#define USB_EP0_SIZE   64

/*- Prototypes --------------------------------------------------------------*/
void usb_init(void);
bool usb_open(int vid, int pid);
void usb_close(void);

void usb_fx2lp_reset(bool reset);
void usb_fx2lp_sram_read(int addr, u8 *data, int size);
void usb_fx2lp_sram_write(int addr, u8 *data, int size);

void usb_i2c_read(int addr, u8 *data, int size);
void usb_i2c_write(int addr, u8 *data, int size);

void usb_jtag_enable(bool enable);
void usb_jtag_request(u8 *data, int count);
void usb_jtag_response(u8 *data, int count);

void usb_ctrl(int index, int value);

void usb_flush_data(void);
void usb_data_transfer(void);

void usb_ctrl_init(void);

void usb_speed_test(void);

#endif // _USB_H_


