// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

#ifndef _FPGA_H_
#define _FPGA_H_

/*- Includes ----------------------------------------------------------------*/

/*- Definitions -------------------------------------------------------------*/

/*- Prototypes --------------------------------------------------------------*/
void fpga_enable(void);
void fpga_disable(void);
u32 fpga_read_idcode(void);
u64 fpga_read_traceid(void);
void fpga_program_sram(u8 *data, int size);
void fpga_erase_flash(void);
void fpga_program_flash(u8 *data, int size);

#endif // _FPGA_H_


