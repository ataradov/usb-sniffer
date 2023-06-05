// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

#ifndef _OS_COMMON_H_
#define _OS_COMMON_H_

/*- Includes ----------------------------------------------------------------*/
#include <err.h>
#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <assert.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

/*- Definitions -------------------------------------------------------------*/
#define ARRAY_SIZE(x)  ((int)(sizeof(x) / sizeof(0[x])))

#define os_assert(x) \
  do \
  { \
    if (!(x)) \
    { \
      fprintf(stderr, "%s:%d: assertion '%s' failed\n", __FILE__, __LINE__, #x); \
      exit(1); \
    } \
  } while (0)

#define os_max(a, b) \
  ({ typeof(a) _a = (a); \
     typeof(b) _b = (b); \
     _a > _b ? _a : _b; \
  })

#define os_min(a, b) \
  ({ typeof(a) _a = (a); \
     typeof(b) _b = (b); \
     _a < _b ? _a : _b; \
  })

#define LIMIT(a, b) os_min(a, b)

/*- Types -------------------------------------------------------------------*/
typedef uint8_t    u8;
typedef int8_t     s8;
typedef uint16_t   u16;
typedef int16_t    s16;
typedef uint32_t   u32;
typedef int32_t    s32;
typedef uint64_t   u64;
typedef int64_t    s64;
typedef float      f32;
typedef double     f64;

typedef struct
{
  int      short_name;
  char     *long_name;
  char     *arg_name;
  void     *value;
  char     *description;
} OsOption;

/*- Prototypes --------------------------------------------------------------*/
void *os_alloc(int size);
void *os_alloc_no_init(int size);
void *os_realloc(void *data, int size);
void os_free(void *data);
void os_sleep(int ms);
void os_check(bool cond, const char *fmt, ...);
void os_error(const char *fmt, ...);
char *os_strdup(const char *str);

s64 os_get_time(void);
s64 os_stopwatch(void);

u16 os_rand16(u16 seed);
u32 os_rand32(u32 seed);
u64 os_rand64(u64 seed);

int os_file_read_all(const char *name, u8 **data);
int os_file_open_for_read(const char *name);
int os_file_open_for_write(const char *name);
int os_file_read(int fd, u8 *data, int size);
int os_file_write(int fd, u8 *data, int size);
void os_file_close(int fd);
int os_file_size(const char *name);

void os_set_sig_handler(void (*handler)(void));

int os_opt_parse(const OsOption *options, int argc, char *argv[]);
void os_opt_print_help(const OsOption *options);

#endif // _OS_COMMON_H_


