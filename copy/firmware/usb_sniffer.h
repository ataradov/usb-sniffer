// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

#ifndef _USB_SNIFFER_H_
#define _USB_SNIFFER_H_

/*- Prototypes --------------------------------------------------------------*/
void debug(void);
void delay_1ms(void);
void delay_ms(uint8_t ms);
void reset_endpoints(void);
void setup_endpoints(void);

#endif // _USB_SNIFFER_H_

