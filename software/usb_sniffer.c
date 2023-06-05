// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

/*- Includes ----------------------------------------------------------------*/
#include "os_common.h"
#include "fx2lp.h"
#include "fpga.h"
#include "usb.h"
#include "capture.h"
#include "usb_sniffer.h"

/*- Definitions -------------------------------------------------------------*/
#define FX2LP_VID      0x04b4
#define FX2LP_PID      0x8613

#define CAPTURE_VID    0x6666
#define CAPTURE_PID    0x6620

/*- Variables ---------------------------------------------------------------*/
Options g_opt;
static FILE *g_log_fd = NULL;

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
static void log_open(char *name)
{
  if (!name)
  {
    g_log_fd = stdout;
    return;
  }

  g_log_fd = fopen(name, "a+");

  if (!g_log_fd)
  {
    g_log_fd = stdout;
    return;
  }

  struct timeval tv;
  gettimeofday(&tv, NULL);

  time_t t = tv.tv_sec;
  struct tm *info = localtime(&t);

  char buf[64];
  strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M:%S", info);
  fprintf(g_log_fd, "\nLog started on %s.%03d\n", buf, (int)(tv.tv_usec / 1000));
}

//-----------------------------------------------------------------------------
void log_print(char *fmt, ...)
{
  if (g_log_fd != stdout)
  {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    time_t t = tv.tv_sec;
    struct tm *info = localtime(&t);

    char buf[64];
    strftime(buf, sizeof(buf), "%H:%M:%S", info);
    fprintf(g_log_fd, "%s.%03d  ", buf, (int)(tv.tv_usec / 1000));
  }

  va_list args;
  va_start(args, fmt);
  vfprintf(g_log_fd, fmt, args);
  va_end(args);

  fprintf(g_log_fd, "\n");

  fflush(g_log_fd);
}

//-----------------------------------------------------------------------------
static void print_help(const char *name, const OsOption *options)
{
  printf("USB Sniffer, built " __DATE__ " " __TIME__ "\n\n");
  printf("Usage: %s [options]\n", name);
  os_opt_print_help(options);
  printf("\n");
  printf("PID list is a comma-separated list consisting of:\n");
  printf("  sof, in, out, setup, ping, ack, nak, nyet, stall,\n");
  printf("  data0, data1, data2, mdata, split, pre/err, reserved, all\n");
  printf("\n");
  printf("All PIDs are enabled by default. Disable list is applied first,\n");
  printf("followed by the enable list.\n");
  printf("\n");

  exit(0);
}

//-----------------------------------------------------------------------------
static void parse_command_line(int argc, char *argv[])
{
  static const OsOption options[] =
  {
    {  1, "General:", NULL, NULL, NULL },
    { 'h', "help",     NULL,     &g_opt.help,     "print this help message and exit" },

    {  1, "Capture:", NULL, NULL, NULL },
    { 's', "speed",    "speed",  &g_opt.speed,      "select USB speed: 'ls', 'fs' (default) or 'hs'" },
    { 'l', "fold",     NULL,     &g_opt.fold_empty, "fold empty frames" },
    { 'n', "limit",    "number", &g_opt.limit,      "limit the number of captured packets" },
    { 't', "trigger",  "type",   &g_opt.trigger,    "capture trigger: 'disabled' (default), 'low', 'high', 'falling' or 'rising'" },
    {  0 , "test",     NULL,     &g_opt.test,       "perform a transfer rate test" },

    {  1, "Wireshark extcap:",  NULL, NULL, NULL },
    {  0 , "extcap-version",    "version", &g_opt.extcap_version,    "show the version of this utility" },
    {  0 , "extcap-dlts",       NULL,      &g_opt.extcap_dlts,       "provide a list of dlts for the given interface" },
    {  0 , "extcap-interfaces", NULL,      &g_opt.extcap_interfaces, "provide a list of interfaces to capture from" },
    {  0 , "extcap-interface",  "name",    &g_opt.extcap_interface,  "provide the interface to capture from" },
    {  0 , "extcap-config",     NULL,      &g_opt.extcap_config,     "provide a list of configurations for the given interface" },
    { 'c', "capture",           NULL,      &g_opt.extcap_capture,    "start capture" },
    { 'f', "fifo",              "name",    &g_opt.extcap_fifo,       "output fifo or file name" },

    {  1, "Firmware update:", NULL, NULL, NULL },
    {  0 , "mcu-sram",   "name", &g_opt.mcu_sram,   "upload FX2LP firmware into the SRAM and run it" },
    {  0 , "mcu-eeprom", "name", &g_opt.mcu_eeprom, "program FX2LP firmware into the EEPROM" },
    {  0 , "fpga-sram",  "name", &g_opt.fpga_sram,  "upload BIT file into the FPGA SRAM" },
    {  0 , "fpga-flash", "name", &g_opt.fpga_flash, "program JED file into the FPGA flash" },
    {  0 , "fpga-erase", NULL,   &g_opt.fpga_erase, "erase FPGA flash" },
    {  0 },
  };
  int last = os_opt_parse(options, argc, argv);

  if (g_opt.help)
    print_help(argv[0], options);

  os_check(last == argc, "malformed command line, use '-h' for more information");
}

//-----------------------------------------------------------------------------
void open_capture_device(void)
{
  if (!usb_open(CAPTURE_VID, CAPTURE_PID))
    os_error("could not open a capture device");
}

//-----------------------------------------------------------------------------
static void mcu_sram(const char *name)
{
  u8 *data;
  int size;

  if (!usb_open(FX2LP_VID, FX2LP_PID))
    os_error("could not open unconfigured FX2LP device");

  size = os_file_read_all(name, &data);

  printf("Uploading %d bytes into the FX2LP SRAM\n", size);
  fx2lp_sram_upload(data, size);
  printf("...done\n");

  os_free(data);
  exit(0);
}

//-----------------------------------------------------------------------------
static void mcu_eeprom(const char *name)
{
  u64 traceid;
  u8 *sn;
  u8 *data;
  int size;

  open_capture_device();

  fpga_enable();
  traceid = fpga_read_traceid() & 0x00ffffffffffffff;
  fpga_disable();

  size = os_file_read_all(name, &data);

  sn = memmem(data, size, "[-----SN-----]", strlen("[-----SN-----]"));
  os_check(sn, "provided binary does not include a placeholder for the serial number");

  sprintf((char *)sn, "%014lx", traceid);

  printf("Programming %d bytes into the FX2LP EEPROM (SN: %s)\n", size, sn);
  fx2lp_eeprom_upload(data, size);
  printf("...done\n");

  exit(0);
}

//-----------------------------------------------------------------------------
static void fpga_sram(const char *name)
{
  u8 *data;
  int size = os_file_read_all(name, &data);

  printf("Uploading FPGA SRAM\n");
  open_capture_device();
  fpga_enable();
  fpga_program_sram(data, size);
  fpga_disable();
  printf("...done\n");

  exit(0);
}

//-----------------------------------------------------------------------------
static void fpga_flash(const char *name)
{
  u8 *data;
  int size = os_file_read_all(name, &data);

  printf("Programming FPGA flash\n");
  open_capture_device();
  fpga_enable();
  fpga_program_flash(data, size);
  fpga_disable();
  printf("...done\n");

  exit(0);
}

//-----------------------------------------------------------------------------
static void fpga_erase(void)
{
  printf("Erasing FPGA flash\n");
  open_capture_device();
  fpga_enable();
  fpga_erase_flash();
  fpga_disable();
  printf("... done\n");

  exit(0);
}

//-----------------------------------------------------------------------------
static int get_capture_speed(void)
{
  if (!g_opt.speed)
    return CaptureSpeed_FS;
  else if (0 == strcmp(g_opt.speed, "ls"))
    return CaptureSpeed_LS;
  else if (0 == strcmp(g_opt.speed, "fs"))
    return CaptureSpeed_FS;
  else if (0 == strcmp(g_opt.speed, "hs"))
    return CaptureSpeed_HS;

  os_error("unrecognized capture speed setting: '%s'", g_opt.speed);

  return 0;
}

//-----------------------------------------------------------------------------
static int get_capture_trigger(void)
{
  if (!g_opt.trigger)
    return CaptureTrigger_Disabled;
  else if (0 == strcmp(g_opt.trigger, "disabled"))
    return CaptureTrigger_Disabled;
  else if (0 == strcmp(g_opt.trigger, "low"))
    return CaptureTrigger_Low;
  else if (0 == strcmp(g_opt.trigger, "high"))
    return CaptureTrigger_High;
  else if (0 == strcmp(g_opt.trigger, "falling"))
    return CaptureTrigger_Falling;
  else if (0 == strcmp(g_opt.trigger, "rising"))
    return CaptureTrigger_Rising;

  os_error("unrecognized capture trigger setting: '%s'", g_opt.trigger);

  return 0;
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  char *log_name = getenv("USB_SNIFFER_LOG");

  log_open(log_name);

  if (log_name)
  {
    log_print("Arguments:");
    for (int i = 0; i < argc; i++)
      log_print("%d: '%s'", i, argv[i]);
  }

  parse_command_line(argc, argv);

  g_opt.capture_speed   = get_capture_speed();
  g_opt.capture_trigger = get_capture_trigger();
  g_opt.capture_limit   = g_opt.limit ? strtoll(g_opt.limit, NULL, 10) : -1;

  if (capture_extcap_request())
    return 0;

  usb_init();

  if (capture_start())
    return 0;

  if (g_opt.test)
  {
    log_print("Starting speed test");
    open_capture_device();
    usb_speed_test();
    return 0;
  }

  if (g_opt.mcu_sram)
    mcu_sram(g_opt.mcu_sram);

  if (g_opt.mcu_eeprom)
    mcu_eeprom(g_opt.mcu_eeprom);

  if (g_opt.fpga_sram)
    fpga_sram(g_opt.fpga_sram);

  if (g_opt.fpga_flash)
    fpga_flash(g_opt.fpga_flash);

  if (g_opt.fpga_erase)
    fpga_erase();

  return 0;
}


