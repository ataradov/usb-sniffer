// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>, Patrick Dussud. All rights reserved.
#include "os_common.h"
#include "io_fifo.h"
typedef struct RingBuff
{
    size_t _size;       /**< Data size*/
    volatile size_t _r; /**< Read index */
    volatile size_t _w; /**< Write index */
    uint8_t _data[];    /**< Data array */
} RingBuff;

static inline size_t rb_free(const size_t w, const size_t r, const size_t size)
{
    if (r > w)
    {
        return (r - w) - 1U;
    }
    else
    {
        return (size - (w - r)) - 1U;
    }
}

static inline size_t rb_available(const size_t w, const size_t r, const size_t size)
{
    if (w >= r)
    {
        return w - r;
    }
    else
    {
        return size - (r - w);
    }
}

static bool rb_write(RingBuff *rb, const uint8_t *data, const size_t cnt)
{
    size_t w = rb->_w;
    const size_t r = rb->_r;

    if (rb_free(w, r, rb->_size) < cnt)
    {
        return false;
    }

    if (w + cnt <= rb->_size)
    {
        /* Copy in the linear region */
        memcpy(&(rb->_data[w]), &data[0], cnt);
        /* Update the write index */
        w += cnt;
        if (w == rb->_size)
        {
            w = 0U;
        }
    }
    else
    {
        /* Copy in the linear region */
        const size_t linear_free = rb->_size - w;
        memcpy(&(rb->_data[w]), &data[0], linear_free);
        /* Copy remaining to the beginning of the buffer */
        const size_t remaining = cnt - linear_free;
        memcpy(&(rb->_data[0]), &data[linear_free], remaining);
        /* Update the write index */
        w = remaining;
    }

    rb->_w = w;

    return true;
}

static size_t rb_read(RingBuff *rb, uint8_t *data, size_t cnt)
{
    const size_t w = rb->_w;
    size_t r = rb->_r;
    size_t avail = rb_available(w, r, rb->_size);
    if (!avail)
        return 0;
    if (avail < cnt)
    {
        cnt = avail;
    }

    if (r + cnt <= rb->_size)
    {
        /* Copy in the linear region */
        memcpy(&data[0], &rb->_data[r], cnt);
        /* Update the read index */
        r += cnt;
        if (r == rb->_size)
        {
            r = 0U;
        }
    }
    else
    {
        /* Copy in the linear region */
        const size_t linear_available = rb->_size - r;
        memcpy(&data[0], &rb->_data[r], linear_available);
        /* Copy remaining from the beginning of the buffer */
        const size_t remaining = cnt - linear_available;
        memcpy(&data[linear_available], &rb->_data[0], remaining);
        /* Update the read index */
        r = remaining;
    }

    rb->_r = r;

    return cnt;
}

void* fifo_create()
{
    size_t size = 80 * 1024 * 1024;
    RingBuff *rb = (RingBuff *)os_alloc_no_init(sizeof(RingBuff) + size);
    rb->_size = size;
    rb->_r = 0;
    rb->_w = 0;
    return rb;
}

size_t fifo_write(void* fifo, const uint8_t *array, size_t length)
{
    RingBuff *rb = (RingBuff*)fifo;
    if (rb_free(rb->_w, rb->_r, rb->_size) >= length)
    {
        rb_write(rb, array, length);
        return length;
    }
    else
        return 0;
}

size_t fifo_read(void* fifo, uint8_t *array, size_t length)
{
    RingBuff *rb = (RingBuff *)fifo;
    return rb_read(rb, array, length);
}

size_t fifo_available_to_read(void* fifo)
{
    RingBuff *rb = (RingBuff *)fifo;
    return rb_available(rb->_w, rb->_r, rb->_size);
}

void fifo_delete(void *fifo)
{
    os_free(fifo);
}

