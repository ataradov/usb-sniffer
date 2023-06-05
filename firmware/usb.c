// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "usb_sniffer.h"
#include "usb_descriptors.h"

/*- Definitions -------------------------------------------------------------*/
#define	bmRequestType  SETUPDAT[0]
#define	bRequest       SETUPDAT[1]
#define	wValueL        SETUPDAT[2]
#define	wValueH        SETUPDAT[3]
#define	wIndexL        SETUPDAT[4]
#define	wIndexH        SETUPDAT[5]
#define	wLengthL       SETUPDAT[6]
#define	wLengthH       SETUPDAT[7]

#define EP0_SIZE       64

/*- Variables ---------------------------------------------------------------*/
static uint8_t usb_config = 0;
static uint8_t usb_interface = 0;

/*- Prototypes --------------------------------------------------------------*/
static bool handle_vendor_request(void);

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
static void usb_renumerate(void)
{
  USBCS |= USBCS_DISCON | USBCS_RENUM;

  delay_ms(250);

  USBIRQ = 0xff;
  EPIRQ  = 0xff;

  USBCS &= ~USBCS_DISCON;
}

//-----------------------------------------------------------------------------
static inline void usb_control_stall(void)
{
  EP0CS |= EPCS_STALL;
}

//-----------------------------------------------------------------------------
static void usb_reset_toggle(uint8_t ep)
{
  uint8_t v = TOGCTL_EP(ep & 0x0f);

  if (USB_IN_ENDPOINT == (ep & USB_DIRECTION_MASK))
    v |= TOGCTL_IO;

  TOGCTL = v;
  TOGCTL = v | TOGCTL_R;
}

//-----------------------------------------------------------------------------
static volatile uint8_t *get_epcs(uint8_t endpoint)
{
  uint8_t ep  = endpoint & USB_INDEX_MASK;
  uint8_t dir = endpoint & USB_DIRECTION_MASK;
  volatile uint8_t *cfg = NULL;

  if (ep == 0)
    return &EP0CS;

  if (ep == 1)
  {
    if (dir == USB_IN_ENDPOINT)
    {
      if (EP1INCFG & EPCFG_VALID)
        return &EP1INCS;
    }
    else
    {
      if (EP1OUTCFG & EPCFG_VALID)
        return &EP1OUTCS;
    }
  }

  if (ep > 8 || (ep & 1))
   return NULL;

  ep >>= 1;
  cfg = &EP2CFG + ep;

  if (*cfg & EPCFG_VALID)
    return &EP2CS + ep;

  return NULL;
}

//-----------------------------------------------------------------------------
static void usb_control_send_buf(uint8_t *data, uint8_t size)
{
  uint8_t i;

  if (size > wLengthL)
    size = wLengthL;

  for (i = 0; i < size; i++)
    EP0BUF[i] = data[i];

  EP0BCL = size;
  SYNCDELAY;

  while (EP0CS & EPCS_BUSY);
}

//-----------------------------------------------------------------------------
static void usb_control_send(uint8_t size)
{
  if (size > wLengthL)
    size = wLengthL;

  EP0BCL = size;
  SYNCDELAY;

  while (EP0CS & EPCS_BUSY);
}

//-----------------------------------------------------------------------------
static int usb_control_recv(void)
{
  EP0BCL = 0;
  SYNCDELAY;
  while (EP0CS & EPCS_BUSY);

  return EP0BCL;
}

//-----------------------------------------------------------------------------
static void usb_handle_standard_request(void)
{
  if (handle_vendor_request())
  {
  }

  else if (USB_CMD(IN, DEVICE, STANDARD) == bmRequestType && USB_GET_DESCRIPTOR == bRequest)
  {
    uint8_t type = wValueH;
    uint8_t index = wValueL;

    if (USB_DEVICE_DESCRIPTOR == type)
    {
      usb_control_send_buf((uint8_t *)&usb_device_descriptor, usb_device_descriptor.bLength);
    }
    else if (USB_CONFIGURATION_DESCRIPTOR == type)
    {
      usb_control_send_buf((uint8_t *)&usb_configuration_hierarchy, usb_configuration_hierarchy.configuration.wTotalLength);
    }
    else if (USB_STRING_DESCRIPTOR == type)
    {
      if (0 == index)
      {
        usb_control_send_buf((uint8_t *)&usb_string_descriptor_zero, usb_string_descriptor_zero.bLength);
      }
      else if (index < USB_STR_COUNT)
      {
        const char *str = usb_strings[index];
        uint8_t buf[64];
        uint8_t size = 2;
        uint8_t i;

        for (i = 0; str[i]; i++)
        {
          buf[size]   = str[i];
          buf[size+1] = 0;
          size += 2;
        }

        buf[0] = size;
        buf[1] = USB_STRING_DESCRIPTOR;

        usb_control_send_buf(buf, size);
      }
      else
      {
        usb_control_stall();
      }
    }
    else
      usb_control_stall();
  }

  else if (USB_CMD(OUT, DEVICE, STANDARD) == bmRequestType && USB_SET_CONFIGURATION == bRequest)
  {
    usb_config = wValueL;
    setup_endpoints();
  }

  else if (USB_CMD(IN, DEVICE, STANDARD) == bmRequestType && USB_GET_CONFIGURATION == bRequest)
  {
    usb_control_send_buf(&usb_config, sizeof(usb_config));
  }

  else if (USB_CMD(OUT, DEVICE, STANDARD) == bmRequestType && USB_SET_INTERFACE == bRequest)
  {
    usb_interface = wValueL;
  }

  else if (USB_CMD(IN, DEVICE, STANDARD) == bmRequestType && USB_GET_INTERFACE == bRequest)
  {
    usb_control_send_buf(&usb_interface, sizeof(usb_interface));
  }

  else if (USB_CMD(IN, DEVICE, STANDARD) == bmRequestType && USB_GET_STATUS == bRequest)
  {
    uint16_t status = 0;
    usb_control_send_buf((uint8_t *)&status, sizeof(status));
  }

  else if (USB_CMD(IN, INTERFACE, STANDARD) == bmRequestType && USB_GET_STATUS == bRequest)
  {
    uint16_t status = 0;
    usb_control_send_buf((uint8_t *)&status, sizeof(status));
  }

  else if (USB_CMD(IN, ENDPOINT, STANDARD) == bmRequestType && USB_GET_STATUS == bRequest)
  {
    volatile uint8_t *epcs = get_epcs(wIndexL);

    if (epcs)
    {
      uint16_t status = (*epcs & EPCS_STALL) ? 1 : 0;
      usb_control_send_buf((uint8_t *)&status, sizeof(status));
    }
    else
    {
      usb_control_stall();
    }
  }

  else if (USB_CMD(OUT, DEVICE, STANDARD) == bmRequestType && USB_SET_FEATURE == bRequest)
  {
    if (USB_FEATURE_DEVICE_TEST_MODE == wValueL)
    {
      // Will be handled by the hardware
    }
    else
    {
      usb_control_stall();
    }
  }

  else if (USB_CMD(OUT, ENDPOINT, STANDARD) == bmRequestType && USB_SET_FEATURE == bRequest)
  {
    volatile uint8_t *epcs = get_epcs(wIndexL);

    if (USB_FEATURE_ENDPOINT_HALT == wValueL && epcs)
    {
      *epcs |= EPCS_STALL;
    }
    else
    {
      usb_control_stall();
    }
  }

  else if (USB_CMD(OUT, ENDPOINT, STANDARD) == bmRequestType && USB_CLEAR_FEATURE == bRequest)
  {
    volatile uint8_t *epcs = get_epcs(wIndexL);

    if (USB_FEATURE_ENDPOINT_HALT == wValueL && epcs)
    {
      *epcs &= ~EPCS_STALL;
      usb_reset_toggle(wIndexL);
    }
    else
    {
      usb_control_stall();
    }
  }

  else
  {
    usb_control_stall();
  }

  EP0CS |= EPCS_HSNAK;
}

//-----------------------------------------------------------------------------
static void usb_task(void)
{
  uint8_t irq = USBIRQ;

  if (irq & USBIRQ_URES)
  {
    USBIRQ = USBIRQ_URES;
    reset_endpoints();
  }

  if (irq & USBIRQ_SUDAV)
  {
    USBIRQ = USBIRQ_SUDAV;
    usb_handle_standard_request();
  }
}


