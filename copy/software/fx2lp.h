// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

#ifndef _FX2LP_H_
#define _FX2LP_H_

/*- Includes ----------------------------------------------------------------*/

/*- Definitions -------------------------------------------------------------*/

/*- Prototypes --------------------------------------------------------------*/
void fx2lp_sram_upload(u8 *data, int size);
void fx2lp_eeprom_upload(u8 *data, int size);

#endif // _FX2LP_H_


