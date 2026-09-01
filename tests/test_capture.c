#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../software/usb_sniffer.h"

Options g_opt;

void log_print(char *fmt, ...) { (void)fmt; }
void open_capture_device(void) {}
void usb_ctrl_init(void) {}
void usb_ctrl(int index, int value) { (void)index; (void)value; }
void usb_flush_data(void) {}
void usb_data_transfer(void) {}

void os_check(bool cond, const char *fmt, ...)
{
  (void)fmt;
  assert(cond);
}

#include "../software/capture.c"

int main(void)
{
  char long_message[700];

  capture_fd = tmpfile();
  assert(capture_fd != NULL);

  g_opt.capture_speed = CaptureSpeed_Auto;
  write_file_header();
  write_usb_header(CaptureSpeed_LS);
  write_usb_header(CaptureSpeed_FS);
  write_usb_header(CaptureSpeed_HS);
  write_info_header();

  capture_speed = CaptureSpeed_FS;
  assert(effective_capture_speed() == CaptureSpeed_FS);
  assert(usb_interface_id() == CaptureSpeed_FS);
  assert(info_interface_id() == 3);

  capture_speed = CaptureSpeed_Reset;
  assert(effective_capture_speed() == CaptureSpeed_HS);
  assert(usb_interface_id() == CaptureSpeed_HS);

  // A full status header establishes the absolute timestamp. The following
  // compact one-byte ACK record advances it by ten 60 MHz ticks.
  capture_header = true;
  capture_data_ptr = 0;
  capture_toggle = 0;
  capture_ts_ticks = 0;
  const u8 full_status[] = { 0x00, 0x00, 0x64, 0x00 };
  const u8 compact_ack[] = { 0xe0, 0x50, 0x04, 0xd2 };
  capture_callback((u8 *)full_status, sizeof(full_status));
  assert(capture_ts_ticks == 100);
  capture_callback((u8 *)compact_ack, sizeof(compact_ack));
  assert(capture_ts_ticks == 110);
  assert(capture_toggle == 0);
  assert(capture_header == true);

  capture_fold_count = 1;
  capture_fold_buf_ptr = 2;
  capture_fold_buf[0].ts = 100;
  capture_fold_buf[0].size = 1;
  capture_fold_buf[0].data[0] = PID_SOF;
  capture_fold_buf[1].ts = 110;
  capture_fold_buf[1].size = 1;
  capture_fold_buf[1].data[0] = PID_IN;
  capture_ts = 200;

  stop_folding();
  assert(capture_last_ts == 200);

  memset(long_message, 'x', sizeof(long_message));
  long_message[sizeof(long_message)-1] = 0;
  capture_info(300, "%s", long_message);
  assert(capture_last_ts == 300);

  fclose(capture_fd);
  return 0;
}
