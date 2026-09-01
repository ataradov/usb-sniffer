#include <stdio.h>
#include <libusb.h>

#define CAPTURE_VID 0x6666
#define CAPTURE_PID 0x6620

int main(void)
{
  libusb_device_handle *handle;
  unsigned char status[2] = { 0xff, 0xff };
  int rc;

  rc = libusb_init(NULL);
  if (rc < 0) {
    fprintf(stderr, "libusb_init: %s\n", libusb_error_name(rc));
    return 1;
  }

  handle = libusb_open_device_with_vid_pid(NULL, CAPTURE_VID, CAPTURE_PID);
  if (!handle) {
    fprintf(stderr, "USB Sniffer not found\n");
    libusb_exit(NULL);
    return 1;
  }

  libusb_set_auto_detach_kernel_driver(handle, 1);
  rc = libusb_claim_interface(handle, 0);
  if (rc < 0) {
    fprintf(stderr, "claim interface: %s\n", libusb_error_name(rc));
    libusb_close(handle);
    libusb_exit(NULL);
    return 1;
  }

  rc = libusb_control_transfer(handle,
      LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_ENDPOINT,
      LIBUSB_REQUEST_GET_STATUS, 0, 0x82, status, sizeof(status), 1000);

  libusb_release_interface(handle, 0);
  libusb_close(handle);
  libusb_exit(NULL);

  if (rc != sizeof(status)) {
    fprintf(stderr, "EP2 GET_STATUS failed: %s (%d)\n", rc < 0 ? libusb_error_name(rc) : "short transfer", rc);
    return 2;
  }

  printf("EP2 GET_STATUS passed, halt=%d\n", status[0] & 1);
  return 0;
}
