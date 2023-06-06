// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

#ifndef _USB_H_
#define _USB_H_

// WinUSB device information is stored in the Windows registry at:
// HKEY_LOCAL_MACHINE\System\CurrentControlSet\Enum\USB\<Device>\<Instance>\Device Parameters

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/*- Definitions -------------------------------------------------------------*/
enum
{
  USB_GET_STATUS        = 0,
  USB_CLEAR_FEATURE     = 1,
  USB_SET_FEATURE       = 3,
  USB_SET_ADDRESS       = 5,
  USB_GET_DESCRIPTOR    = 6,
  USB_SET_DESCRIPTOR    = 7,
  USB_GET_CONFIGURATION = 8,
  USB_SET_CONFIGURATION = 9,
  USB_GET_INTERFACE     = 10,
  USB_SET_INTERFACE     = 11,
  USB_SYNCH_FRAME       = 12,
};

enum
{
  USB_DEVICE_DESCRIPTOR                    = 1,
  USB_CONFIGURATION_DESCRIPTOR             = 2,
  USB_STRING_DESCRIPTOR                    = 3,
  USB_INTERFACE_DESCRIPTOR                 = 4,
  USB_ENDPOINT_DESCRIPTOR                  = 5,
  USB_DEVICE_QUALIFIER_DESCRIPTOR          = 6,
  USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR = 7,
  USB_INTERFACE_POWER_DESCRIPTOR           = 8,
  USB_OTG_DESCRIPTOR                       = 9,
  USB_DEBUG_DESCRIPTOR                     = 10,
  USB_INTERFACE_ASSOCIATION_DESCRIPTOR     = 11,
  USB_BINARY_OBJECT_STORE_DESCRIPTOR       = 15,
  USB_DEVICE_CAPABILITY_DESCRIPTOR         = 16,
};

enum
{
  USB_RECIPIENT_MASK       = 0x1f,
  USB_RECIPIENT_DEVICE     = 0x00,
  USB_RECIPIENT_INTERFACE  = 0x01,
  USB_RECIPIENT_ENDPOINT   = 0x02,
  USB_RECIPIENT_OTHER      = 0x03,
};

enum
{
  USB_REQUEST_MASK         = 0x60,
  USB_REQUEST_STANDARD     = 0x00,
  USB_REQUEST_CLASS        = 0x20,
  USB_REQUEST_VENDOR       = 0x40,
};

enum
{
  USB_TRANSFER_MASK        = 0x80,
  USB_TRANSFER_OUT         = 0x00,
  USB_TRANSFER_IN          = 0x80,
};

enum
{
  USB_IN_ENDPOINT          = 0x80,
  USB_OUT_ENDPOINT         = 0x00,
  USB_INDEX_MASK           = 0x7f,
  USB_DIRECTION_MASK       = 0x80,
};

enum
{
  USB_CONTROL_ENDPOINT     = 0 << 0,
  USB_ISOCHRONOUS_ENDPOINT = 1 << 0,
  USB_BULK_ENDPOINT        = 2 << 0,
  USB_INTERRUPT_ENDPOINT   = 3 << 0,

  USB_NO_SYNCHRONIZATION   = 0 << 2,
  USB_ASYNCHRONOUS         = 1 << 2,
  USB_ADAPTIVE             = 2 << 2,
  USB_SYNCHRONOUS          = 3 << 2,

  USB_DATA_ENDPOINT        = 0 << 4,
  USB_FEEDBACK_ENDPOINT    = 1 << 4,
  USB_IMP_FB_DATA_ENDPOINT = 2 << 4,
};

enum
{
  USB_FEATURE_ENDPOINT_HALT        = 0,
  USB_FEATURE_DEVICE_REMOTE_WAKEUP = 1,
  USB_FEATURE_DEVICE_TEST_MODE     = 2,
};

enum
{
  USB_STATUS_SELF_POWERED  = (1 << 0),
  USB_STATUS_REMOTE_WAKEUP = (1 << 1),
};

enum
{
  USB_DEVICE_CAPABILITY_WIRELESS_USB               = 1,
  USB_DEVICE_CAPABILITY_USB_2_0_EXTENSION          = 2,
  USB_DEVICE_CAPABILITY_SUPERSPEED_USB             = 3,
  USB_DEVICE_CAPABILITY_CONTAINER_ID               = 4,
  USB_DEVICE_CAPABILITY_PLATFORM                   = 5,
  USB_DEVICE_CAPABILITY_POWER_DELIVERY             = 6,
  USB_DEVICE_CAPABILITY_BATTERY_INFO               = 7,
  USB_DEVICE_CAPABILITY_PD_CONSUMER_PORT           = 8,
  USB_DEVICE_CAPABILITY_PD_PROVIDER_PORT           = 9,
  USB_DEVICE_CAPABILITY_SUPERSPEED_PLUS            = 10,
  USB_DEVICE_CAPABILITY_PRECISION_TIME_MEASUREMENT = 11,
  USB_DEVICE_CAPABILITY_WIRELESS_USB_EXT           = 12,
};

#define USB_WINUSB_VENDOR_CODE     0x20

#define USB_WINUSB_WINDOWS_VERSION 0x06030000 // Windows 8.1

#define USB_WINUSB_PLATFORM_CAPABILITY_ID \
    { 0xdf, 0x60, 0xdd, 0xd8, 0x89, 0x45, 0xc7, 0x4c, \
      0x9c, 0xd2, 0x65, 0x9d, 0x9e, 0x64, 0x8a, 0x9f }

enum // WinUSB Microsoft OS 2.0 descriptor request codes
{
  USB_WINUSB_DESCRIPTOR_INDEX    = 0x07,
  USB_WINUSB_SET_ALT_ENUMERATION = 0x08,
};

enum // wDescriptorType
{
  USB_WINUSB_SET_HEADER_DESCRIPTOR       = 0x00,
  USB_WINUSB_SUBSET_HEADER_CONFIGURATION = 0x01,
  USB_WINUSB_SUBSET_HEADER_FUNCTION      = 0x02,
  USB_WINUSB_FEATURE_COMPATBLE_ID        = 0x03,
  USB_WINUSB_FEATURE_REG_PROPERTY        = 0x04,
  USB_WINUSB_FEATURE_MIN_RESUME_TIME     = 0x05,
  USB_WINUSB_FEATURE_MODEL_ID            = 0x06,
  USB_WINUSB_FEATURE_CCGP_DEVICE         = 0x07,
  USB_WINUSB_FEATURE_VENDOR_REVISION     = 0x08,
};

enum // wPropertyDataType
{
  USB_WINUSB_PROPERTY_DATA_TYPE_SZ                  = 1,
  USB_WINUSB_PROPERTY_DATA_TYPE_EXPAND_SZ           = 2,
  USB_WINUSB_PROPERTY_DATA_TYPE_BINARY              = 3,
  USB_WINUSB_PROPERTY_DATA_TYPE_DWORD_LITTLE_ENDIAN = 4,
  USB_WINUSB_PROPERTY_DATA_TYPE_DWORD_BIG_ENDIAN    = 5,
  USB_WINUSB_PROPERTY_DATA_TYPE_LINK                = 6,
  USB_WINUSB_PROPERTY_DATA_TYPE_MULTI_SZ            = 7,
};

enum
{
  USB_HID_DESCRIPTOR          = 0x21,
  USB_HID_REPORT_DESCRIPTOR   = 0x22,
  USB_HID_PHYSICAL_DESCRIPTOR = 0x23,
};

#define USB_CMD(dir, rcpt, type) \
    (USB_TRANSFER_##dir | USB_REQUEST_##type | USB_RECIPIENT_##rcpt)

/*- Types -------------------------------------------------------------------*/
typedef struct
{
  uint8_t   bLength;
  uint8_t   bDescriptorType;
} usb_descriptor_header_t;

typedef struct
{
  uint8_t   bLength;
  uint8_t   bDescriptorType;
  uint16_t  bcdUSB;
  uint8_t   bDeviceClass;
  uint8_t   bDeviceSubClass;
  uint8_t   bDeviceProtocol;
  uint8_t   bMaxPacketSize0;
  uint16_t  idVendor;
  uint16_t  idProduct;
  uint16_t  bcdDevice;
  uint8_t   iManufacturer;
  uint8_t   iProduct;
  uint8_t   iSerialNumber;
  uint8_t   bNumConfigurations;
} usb_device_descriptor_t;

typedef struct
{
  uint8_t   bLength;
  uint8_t   bDescriptorType;
  uint16_t  wTotalLength;
  uint8_t   bNumInterfaces;
  uint8_t   bConfigurationValue;
  uint8_t   iConfiguration;
  uint8_t   bmAttributes;
  uint8_t   bMaxPower;
} usb_configuration_descriptor_t;

typedef struct
{
  uint8_t   bLength;
  uint8_t   bDescriptorType;
  uint8_t   bInterfaceNumber;
  uint8_t   bAlternateSetting;
  uint8_t   bNumEndpoints;
  uint8_t   bInterfaceClass;
  uint8_t   bInterfaceSubClass;
  uint8_t   bInterfaceProtocol;
  uint8_t   iInterface;
} usb_interface_descriptor_t;

typedef struct
{
  uint8_t   bLength;
  uint8_t   bDescriptorType;
  uint8_t   bEndpointAddress;
  uint8_t   bmAttributes;
  uint16_t  wMaxPacketSize;
  uint8_t   bInterval;
} usb_endpoint_descriptor_t;

typedef struct
{
  uint8_t   bLength;
  uint8_t   bDescriptorType;
  uint16_t  wLANGID;
} usb_string_descriptor_zero_t;

typedef struct
{
  uint8_t   bLength;
  uint8_t   bDescriptorType;
  uint16_t  bString;
} usb_string_descriptor_t;

typedef struct
{
  uint8_t   bLength;
  uint8_t   bDescriptorType;
  uint16_t  bcdHID;
  uint8_t   bCountryCode;
  uint8_t   bNumDescriptors;
  uint8_t   bDescriptorType1;
  uint16_t  wDescriptorLength;
} usb_hid_descriptor_t;

typedef struct
{
  uint8_t   bLength;
  uint8_t   bDescriptorType;
  uint16_t  wTotalLength;
  uint8_t   bNumDeviceCaps;
} usb_binary_object_store_descriptor_t;

typedef struct
{
  uint8_t   bLength;
  uint8_t   bDescriptorType;
  uint8_t   bDevCapabilityType;
  uint8_t   bReserved;
  uint8_t   PlatformCapabilityUUID[16];
  uint32_t  dwWindowsVersion;
  uint16_t  wMSOSDescriptorSetTotalLength;
  uint8_t   bMS_VendorCode;
  uint8_t   bAltEnumCode;
} usb_winusb_capability_descriptor_t;

typedef struct
{
  uint16_t  wLength;
  uint16_t  wDescriptorType;
  uint32_t  dwWindowsVersion;
  uint16_t  wDescriptorSetTotalLength;
} usb_winusb_set_header_descriptor_t;

typedef struct
{
  uint16_t  wLength;
  uint16_t  wDescriptorType;
  uint8_t   bFirstInterface;
  uint8_t   bReserved;
  uint16_t  wSubsetLength;
} usb_winusb_subset_header_function_t;

typedef struct
{
  uint16_t  wLength;
  uint16_t  wDescriptorType;
  uint8_t   CompatibleID[8];
  uint8_t   SubCompatibleID[8];
} usb_winusb_feature_compatble_id_t;

typedef struct
{
  uint16_t  wLength;
  uint16_t  wDescriptorType;
  uint16_t  wPropertyDataType;
  uint16_t  wPropertyNameLength;
  uint8_t   PropertyName[40];
  uint16_t  wPropertyDataLength;
  uint8_t   PropertyData[78];
} usb_winusb_feature_reg_property_guids_t;

#endif // _USB_H_

