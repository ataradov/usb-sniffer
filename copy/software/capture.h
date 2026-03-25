// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

#ifndef _CAPTURE_H_
#define _CAPTURE_H_

/*- Includes ----------------------------------------------------------------*/
#include "os_common.h"

/*- Definitions -------------------------------------------------------------*/
enum
{
  CaptureCtrl_Reset  = 0,
  CaptureCtrl_Enable = 1,
  CaptureCtrl_Speed0 = 2,
  CaptureCtrl_Speed1 = 3,
  CaptureCtrl_Test   = 4,
};

enum
{
  CaptureSpeed_LS    = 0,
  CaptureSpeed_FS    = 1,
  CaptureSpeed_HS    = 2,
  CaptureSpeed_Reset = 3,
};

enum
{
  CaptureTrigger_Disabled,
  CaptureTrigger_Low,
  CaptureTrigger_High,
  CaptureTrigger_Falling,
  CaptureTrigger_Rising,
};

/*- Prototypes --------------------------------------------------------------*/
bool capture_extcap_request(void);
bool capture_start(void);

void capture_callback(u8 *data, int size);

#endif // _CAPTURE_H_


