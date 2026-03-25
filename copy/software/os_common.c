// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

/*- Includes ----------------------------------------------------------------*/
#include <time.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __linux__
#include <signal.h>
#endif
#include "os_common.h"

/*- Definitions -------------------------------------------------------------*/
#define OPT_DESC_OFFSET        32

#define FILE_ALLOC_FOOTER      8192

#ifndef O_BINARY
#define O_BINARY 0
#endif

/*- Variables ---------------------------------------------------------------*/
static void (*g_os_sig_handler)(void) = NULL;

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void *os_alloc(int size)
{
  void *data = malloc(size);

  os_check(data != NULL, "out of memory trying to allocate %d bytes", size);

  memset(data, 0, size);

  return data;
}

//-----------------------------------------------------------------------------
void *os_alloc_no_init(int size)
{
  void *data = malloc(size);

  os_check(data != NULL, "out of memory trying to allocate %d bytes", size);

  return data;
}

//-----------------------------------------------------------------------------
void *os_realloc(void *data, int size)
{
  data = realloc(data, size);

  os_check(data != NULL, "out of memory trying to re-allocate %d bytes", size);

  return data;
}

//-----------------------------------------------------------------------------
void os_free(void *data)
{
  //os_check(data != NULL, "free(NULL)");
  free(data);
}

//-----------------------------------------------------------------------------
void os_sleep(int ms)
{
  struct timespec ts;

  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000;

  nanosleep(&ts, NULL);
}

//-----------------------------------------------------------------------------
void os_check(bool cond, const char *fmt, ...)
{
  va_list args;

  if (cond)
    return;

  va_start(args, fmt);
  fprintf(stderr, "Error: ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);

  exit(1);
}

//-----------------------------------------------------------------------------
void os_error(const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  fprintf(stderr, "Error: ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);

  exit(1);
}

//-----------------------------------------------------------------------------
char *os_strdup(const char *str)
{
  char *res = strdup(str);
  os_assert(res);
  return res;
}

//-----------------------------------------------------------------------------
s64 os_get_time(void)
{
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return (s64)tv.tv_sec * 1000 + (s64)tv.tv_usec / 1000;
}

//-----------------------------------------------------------------------------
s64 os_stopwatch(void)
{
  static struct timeval last;
  struct timeval current;
  u64 res;

  gettimeofday(&current, NULL);

  res  = (current.tv_sec - last.tv_sec) * 1000;
  res += (current.tv_usec - last.tv_usec) / 1000;

  last = current;

  return res;
}

//-----------------------------------------------------------------------------
u16 os_rand16(u16 seed)
{
  static u16 state = 0x6c41;

  if (seed)
    state = seed;

  state ^= state << 7;
  state ^= state >> 9;
  state ^= state << 8;

  return state;
}

//-----------------------------------------------------------------------------
u32 os_rand32(u32 seed)
{
  static u32 state = 0x78656c41;

  if (seed)
    state = seed;

  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;

  return state;
}

//-----------------------------------------------------------------------------
u64 os_rand64(u64 seed)
{
  static u64 state = 0x78656c4178656c41;

  if (seed)
    state = seed;

  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;

  return state;
}

//-----------------------------------------------------------------------------
int os_file_read_all(const char *name, u8 **data)
{
  struct stat stat;
  int fd, rsize;

  fd = open(name, O_RDONLY | O_BINARY);
  os_check(fd, "os_file_read_all(): %s", strerror(errno));

  fstat(fd, &stat);

  *data = os_alloc(stat.st_size + FILE_ALLOC_FOOTER);

  rsize = read(fd, *data, stat.st_size);
  os_check(fd, "os_file_read_all(): %s", strerror(errno));

  os_check(rsize == stat.st_size, "os_file_read_all(): failed to read entire file");

  close(fd);

  return rsize;
}

//-----------------------------------------------------------------------------
int os_file_open_for_read(const char *name)
{
  int fd = open(name, O_RDONLY | O_BINARY);
  os_check(fd, "os_file_open_for_read(): %s", strerror(errno));
  return fd;
}

//-----------------------------------------------------------------------------
int os_file_open_for_write(const char *name)
{
  int fd = open(name, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, 0644);
  os_check(fd, "os_file_open_for_write(): %s", strerror(errno));
  return fd;
}

//-----------------------------------------------------------------------------
int os_file_read(int fd, u8 *data, int size)
{
  int res = read(fd, data, size);
  os_check(res, "os_file_read(): %s", strerror(errno));
  return res;
}

//-----------------------------------------------------------------------------
int os_file_write(int fd, u8 *data, int size)
{
  int res = write(fd, data, size);
  os_check(res, "os_file_write(): %s", strerror(errno));
  return res;
}

//-----------------------------------------------------------------------------
void os_file_close(int fd)
{
  close(fd);
}

//-----------------------------------------------------------------------------
int os_file_get_size(const char *name)
{
  struct stat stat;
  int fd;

  fd = open(name, O_RDONLY | O_BINARY);
  os_check(fd, "os_file_get_size(): %s", strerror(errno));
  fstat(fd, &stat);
  close(fd);

  return stat.st_size;
}

//-----------------------------------------------------------------------------
#ifdef __linux__
static void os_sig_handler(int signum)
{
  os_assert(g_os_sig_handler);

  if (SIGINT == signum)
    g_os_sig_handler();
}
#endif

//-----------------------------------------------------------------------------
#ifdef _WIN32
static BOOL WINAPI os_sig_handler(DWORD signal)
{
  os_assert(g_os_sig_handler);

  if (CTRL_C_EVENT == signal)
  {
    g_os_sig_handler();
    return TRUE;
  }

  return FALSE;
}
#endif

//-----------------------------------------------------------------------------
void os_set_sig_handler(void (*handler)(void))
{
  g_os_sig_handler = handler;

#ifdef __linux__
  static struct sigaction sigact;

  sigact.sa_handler = os_sig_handler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction(SIGINT, &sigact, NULL);
#endif

#ifdef _WIN32
  SetConsoleCtrlHandler(os_sig_handler, TRUE);
#endif
}

//-----------------------------------------------------------------------------
static const OsOption *find_long_option(const OsOption *options, char *text)
{
  for (const OsOption *opt = options; opt->value || opt->short_name; opt++)
  {
    if (opt->long_name && (0 == strcmp(opt->long_name, text)))
      return opt;
  }

  return NULL;
}

//-----------------------------------------------------------------------------
static const OsOption *find_short_option(const OsOption *options, char chr)
{
  for (const OsOption *opt = options; opt->value || opt->short_name; opt++)
  {
    if (opt->short_name == chr)
      return opt;
  }

  return NULL;
}

//-----------------------------------------------------------------------------
int os_opt_parse(const OsOption *options, int argc, char *argv[])
{
  const OsOption *arg_opt = NULL;
  const OsOption *opt;
  bool short_opt = false;

  for (int i = 1; i < argc; i++)
  {
    char *arg = argv[i];

    if (arg_opt)
    {
      if (arg[0] == '-')
        break;

      *(char **)arg_opt->value = arg;
      arg_opt = NULL;
      continue;
    }

    if (arg[0] != '-')
      return i;

    if (arg[1] == '-')
    {
      char *value = strchr(arg, '=');

      if (value)
        *value = 0;

      opt = find_long_option(options, &arg[2]);
      os_check(opt, "unrecognized option: %s", arg);

      if (value)
        *(char **)opt->value = &value[1];
      else if (opt->arg_name)
        arg_opt = opt;
      else
        *(bool *)opt->value = true;

      short_opt = false;
    }
    else if (arg[1] == 0)
    {
      os_error("expected option name");
    }
    else
    {
      char *ptr = &arg[1];

      while (*ptr)
      {
        if (arg_opt)
          os_error("option -%c requires an argument", arg_opt->short_name);

        opt = find_short_option(options, *ptr);
        os_check(opt, "unrecognized option: -%c", *ptr);

        if (opt->arg_name)
          arg_opt = opt;
        else
          *(bool *)opt->value = true;

        ptr++;
      }

      short_opt = true;
    }
  }

  if (arg_opt)
  {
    if (short_opt)
      os_error("option -%c requires an argument", arg_opt->short_name);
    else
      os_error("option --%s requires an argument", arg_opt->long_name);
  }

  return argc;
}

//-----------------------------------------------------------------------------
void os_opt_print_help(const OsOption *options)
{
  char buf[1024];

  for (const OsOption *opt = options; opt->value || opt->short_name; opt++)
  {
    if (1 == opt->short_name)
    {
      puts("");
      puts(opt->long_name);
      continue;
    }

    char *ptr = buf;

    ptr += sprintf(ptr, "  ");

    if (opt->short_name)
      ptr += sprintf(ptr, "-%c", opt->short_name);

    if (opt->short_name && opt->long_name)
      ptr += sprintf(ptr, ", ");

    if (opt->long_name)
      ptr += sprintf(ptr, "--%s", opt->long_name);

    if (opt->arg_name)
      ptr += sprintf(ptr, " <%s>", opt->arg_name);

    while ((ptr - buf) < OPT_DESC_OFFSET)
      *ptr++ = ' ';
    *ptr++ = ' ';
    *ptr = 0;

    strcat(ptr, opt->description);

    puts(buf);
  }
}


