
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>, Patrick Dussud. All rights reserved.
#ifndef _IO_FIFO_H_
#define _IO_FIFO_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>

void *fifo_create();
size_t fifo_write(void *fifo, const uint8_t *array, size_t length);
size_t fifo_read(void *fifo, uint8_t *array, size_t length);
size_t fifo_available_to_read(void *fifo);
void fifo_delete(void *fifo);

#ifdef __cplusplus
}
#endif

#endif //_IO_FIFO_H_