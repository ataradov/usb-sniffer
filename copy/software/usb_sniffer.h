// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

#ifndef _USB_SNIFFER_H_
#define _USB_SNIFFER_H_

/*- Includes ----------------------------------------------------------------*/

/*- Definitions -------------------------------------------------------------*/

/*- Types -------------------------------------------------------------------*/
typedef struct
{
  bool     help;
  char     *file;

  char     *speed;
  bool     fold_empty;
  char     *limit;
  char     *trigger;
  bool     test;

  char     *extcap_version;
  bool     extcap_dlts;
  bool     extcap_interfaces;
  char     *extcap_interface;
  bool     extcap_config;
  bool     extcap_capture;
  char     *extcap_fifo;

  char     *mcu_sram;
  char     *mcu_eeprom;
  char     *fpga_sram;
  char     *fpga_flash;
  char     *fpga_erase;

  int      capture_speed;
  int      capture_trigger;
  s64      capture_limit;
} Options;

/*- Variables ---------------------------------------------------------------*/
extern Options g_opt;

/*- Prototypes --------------------------------------------------------------*/
void log_print(char *fmt, ...);
u8 *find_str(u8 *buf, int size, char *str);
void open_capture_device(void);

#endif // _USB_SNIFFER_H_


