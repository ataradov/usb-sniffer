// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

#ifndef _USB_DESCRIPTORS_H_
#define _USB_DESCRIPTORS_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "usb.h"

/*- Definitions -------------------------------------------------------------*/
#define USB_CONTROL_EP_SIZE    64

enum
{
  USB_STR_ZERO,
  USB_STR_MANUFACTURER,
  USB_STR_PRODUCT,
  USB_STR_SERIAL_NUMBER,
  USB_STR_COUNT,
};

/*- Types -------------------------------------------------------------------*/
typedef struct
{
  usb_configuration_descriptor_t  configuration;
  usb_interface_descriptor_t      interface;
  usb_endpoint_descriptor_t       ep_in;
} usb_configuration_hierarchy_t;

typedef struct
{
  usb_binary_object_store_descriptor_t             bos;
  usb_winusb_capability_descriptor_t               winusb;
} usb_bos_hierarchy_t;

typedef struct
{
  usb_winusb_set_header_descriptor_t               header;
  usb_winusb_feature_compatble_id_t                comp_id;
  usb_winusb_feature_reg_property_guids_t          property;
} usb_msos_descriptor_set_t;

/*- Variables ---------------------------------------------------------------*/
const usb_device_descriptor_t usb_device_descriptor =
{
  .bLength            = sizeof(usb_device_descriptor_t),
  .bDescriptorType    = USB_DEVICE_DESCRIPTOR,
  .bcdUSB             = 0x0210,
  .bDeviceClass       = 0,
  .bDeviceSubClass    = 0,
  .bDeviceProtocol    = 0,
  .bMaxPacketSize0    = USB_CONTROL_EP_SIZE,
  .idVendor           = 0x6666,
  .idProduct          = 0x6620,
  .bcdDevice          = 0x0100,
  .iManufacturer      = USB_STR_MANUFACTURER,
  .iProduct           = USB_STR_PRODUCT,
  .iSerialNumber      = USB_STR_SERIAL_NUMBER,
  .bNumConfigurations = 1,
};

const usb_configuration_hierarchy_t usb_configuration_hierarchy =
{
  .configuration =
  {
    .bLength             = sizeof(usb_configuration_descriptor_t),
    .bDescriptorType     = USB_CONFIGURATION_DESCRIPTOR,
    .wTotalLength        = sizeof(usb_configuration_hierarchy_t),
    .bNumInterfaces      = 1,
    .bConfigurationValue = 1,
    .iConfiguration      = 0,
    .bmAttributes        = 0x80,
    .bMaxPower           = 250,
  },

  .interface =
  {
    .bLength             = sizeof(usb_interface_descriptor_t),
    .bDescriptorType     = USB_INTERFACE_DESCRIPTOR,
    .bInterfaceNumber    = 0,
    .bAlternateSetting   = 0,
    .bNumEndpoints       = 1,
    .bInterfaceClass     = 0xff,
    .bInterfaceSubClass  = 0,
    .bInterfaceProtocol  = 0,
    .iInterface          = 0,
  },

  .ep_in =
  {
    .bLength             = sizeof(usb_endpoint_descriptor_t),
    .bDescriptorType     = USB_ENDPOINT_DESCRIPTOR,
    .bEndpointAddress    = USB_IN_ENDPOINT | 2,
    .bmAttributes        = USB_BULK_ENDPOINT,
    .wMaxPacketSize      = 512,
    .bInterval           = 0,
  },
};

const usb_bos_hierarchy_t usb_bos_hierarchy =
{
  .bos =
  {
    .bLength             = sizeof(usb_binary_object_store_descriptor_t),
    .bDescriptorType     = USB_BINARY_OBJECT_STORE_DESCRIPTOR,
    .wTotalLength        = sizeof(usb_bos_hierarchy_t),
    .bNumDeviceCaps      = 1,
  },

  .winusb =
  {
    .bLength                = sizeof(usb_winusb_capability_descriptor_t),
    .bDescriptorType        = USB_DEVICE_CAPABILITY_DESCRIPTOR,
    .bDevCapabilityType     = USB_DEVICE_CAPABILITY_PLATFORM,
    .bReserved              = 0,
    .PlatformCapabilityUUID = USB_WINUSB_PLATFORM_CAPABILITY_ID,
    .dwWindowsVersion       = USB_WINUSB_WINDOWS_VERSION,
    .wMSOSDescriptorSetTotalLength = sizeof(usb_msos_descriptor_set_t),
    .bMS_VendorCode         = USB_WINUSB_VENDOR_CODE,
    .bAltEnumCode           = 0,
  },
};

const usb_msos_descriptor_set_t usb_msos_descriptor_set =
{
  .header =
  {
    .wLength             = sizeof(usb_winusb_set_header_descriptor_t),
    .wDescriptorType     = USB_WINUSB_SET_HEADER_DESCRIPTOR,
    .dwWindowsVersion    = USB_WINUSB_WINDOWS_VERSION,
    .wDescriptorSetTotalLength = sizeof(usb_msos_descriptor_set_t),
  },

  .comp_id =
  {
    .wLength           = sizeof(usb_winusb_feature_compatble_id_t),
    .wDescriptorType   = USB_WINUSB_FEATURE_COMPATBLE_ID,
    .CompatibleID      = "WINUSB",
    .SubCompatibleID   = { 0 },
  },

  .property =
  {
    .wLength             = sizeof(usb_winusb_feature_reg_property_guids_t),
    .wDescriptorType     = USB_WINUSB_FEATURE_REG_PROPERTY,
    .wPropertyDataType   = USB_WINUSB_PROPERTY_DATA_TYPE_SZ,
    .wPropertyNameLength = sizeof(usb_msos_descriptor_set.property.PropertyName),
    .PropertyName        = {
        'D',0,'e',0,'v',0,'i',0,'c',0,'e',0,'I',0,'n',0,'t',0,'e',0,'r',0,'f',0,'a',0,'c',0,'e',0,
        'G',0,'U',0,'I',0,'D',0, 0, 0 },
    .wPropertyDataLength = sizeof(usb_msos_descriptor_set.property.PropertyData),
    .PropertyData        = {
        '{',0,'8',0,'8',0,'B',0,'A',0,'E',0,'0',0,'3',0,'2',0,'-',0,'5',0,'A',0,'8',0,'1',0,'-',0,
        '4',0,'9',0,'f',0,'0',0,'-',0,'B',0,'C',0,'3',0,'D',0,'-',0,'A',0,'4',0,'F',0,'F',0,'1',0,
        '3',0,'8',0,'2',0,'1',0,'6',0,'D',0,'6',0,'}',0, 0, 0 },
  },
};

const usb_string_descriptor_zero_t usb_string_descriptor_zero =
{
  .bLength               = sizeof(usb_string_descriptor_zero_t),
  .bDescriptorType       = USB_STRING_DESCRIPTOR,
  .wLANGID               = 0x0409, // English (United States)
};

const char *const usb_strings[] =
{
  [USB_STR_MANUFACTURER]  = "Alex Taradov",
  [USB_STR_PRODUCT]       = "USB Sniffer",
  [USB_STR_SERIAL_NUMBER] = "[-----SN-----]",
};

#endif // _USB_DESCRIPTORS_H_

