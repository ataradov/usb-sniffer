#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../software/os_common.h"

int main(int argc, char **argv)
{
  static const u8 expected[] = { 0x00, 0x11, 0x22, 0x33, 0xff };
  u8 buffer[sizeof(expected)];
  u8 *all = NULL;
  int fd;

  assert(argc == 2);

  close(0);
  fd = os_file_open_for_write(argv[1]);
  assert(fd == 0);
  assert(os_file_write(fd, (u8 *)expected, sizeof(expected)) == sizeof(expected));
  os_file_close(fd);

  assert(os_file_size(argv[1]) == sizeof(expected));
  assert(os_file_read_all(argv[1], &all) == sizeof(expected));
  assert(memcmp(all, expected, sizeof(expected)) == 0);
  os_free(all);

  fd = os_file_open_for_read(argv[1]);
  assert(os_file_read(fd, buffer, sizeof(buffer)) == sizeof(buffer));
  assert(memcmp(buffer, expected, sizeof(expected)) == 0);
  assert(os_file_read(fd, buffer, sizeof(buffer)) == 0);
  os_file_close(fd);

  return 0;
}
