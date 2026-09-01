/* Copyright (C) 2023 Nunuhara Cabbage <nunuhara@haniwa.technology>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://gnu.org/licenses/>.
 */

#ifndef NULIB_BUFFER_H
#define NULIB_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "nulib.h"
#include "nulib/little_endian.h"

typedef char *string; // nulib string

struct buffer {
	uint8_t *buf;
	size_t size;
	size_t index;
};

static inline uint8_t buffer_peek_u8_uc(struct buffer *r)
{
	return r->buf[r->index];
}

static inline uint8_t buffer_peek_u8(struct buffer *r)
{
	if (unlikely(r->index + 1 > r->size))
		ERROR("Out of bounds buffer read");
	return buffer_peek_u8_uc(r);
}

static inline uint8_t buffer_read_u8_uc(struct buffer *r)
{
	return r->buf[r->index++];
}

static inline uint8_t buffer_read_u8(struct buffer *r)
{
	uint8_t v = buffer_peek_u8(r);
	r->index++;
	return v;
}

static inline uint16_t buffer_peek_u16_uc(struct buffer *r)
{
	return le_get16(r->buf, r->index);
}

static inline uint16_t buffer_peek_u16(struct buffer *r)
{
	if (unlikely(r->index + 2 > r->size))
		ERROR("Out of bounds buffer read");
	return buffer_peek_u16_uc(r);
}

static inline uint16_t buffer_read_u16_uc(struct buffer *r)
{
	uint16_t v = buffer_peek_u16_uc(r);
	r->index += 2;
	return v;
}

static inline uint16_t buffer_read_u16(struct buffer *r)
{
	uint16_t v = buffer_peek_u16(r);
	r->index += 2;
	return v;
}

static inline uint32_t buffer_peek_u32_uc(struct buffer *r)
{
	return le_get32(r->buf, r->index);
}

static inline uint32_t buffer_peek_u32(struct buffer *r)
{
	if (unlikely(r->index + 4 > r->size))
		ERROR("Out of bounds buffer read");
	return buffer_peek_u32_uc(r);
}

static inline uint32_t buffer_read_u32_uc(struct buffer *r)
{
	uint32_t v = buffer_peek_u32_uc(r);
	r->index += 4;
	return v;
}

static inline uint32_t buffer_read_u32(struct buffer *r)
{
	uint32_t v = buffer_peek_u32(r);
	r->index += 4;
	return v;
}

static inline float buffer_read_float_uc(struct buffer *r)
{
	union { uint32_t i; float f; } v;
	v.i = buffer_read_u32_uc(r);
	return v.f;
}

static inline float buffer_read_float(struct buffer *r)
{
	union { uint32_t i; float f; } v;
	v.i = buffer_read_u32(r);
	return v.f;
}

void buffer_init(struct buffer *r, uint8_t *buf, size_t size) attr_nonnull_arg(1);
/* Read a null-terminated string. */
string buffer_read_string(struct buffer *r);
/* Skip a null-terminated string and returns a pointer to the (C) string. */
char *buffer_skip_string(struct buffer *r);
string buffer_read_pascal_string(struct buffer *r);
void buffer_read_bytes(struct buffer *r, uint8_t *dst, size_t n) attr_nonnull;
void buffer_skip(struct buffer *r, size_t off) attr_nonnull;
bool buffer_check_bytes(struct buffer *r, const char *data, size_t n) attr_nonnull;

/* Ensure at least `size` bytes are allocated at end of buffer. */
void _buffer_reserve(struct buffer *b, size_t size);

static inline void buffer_reserve(struct buffer *b, size_t size)
{
	if (unlikely(b->index + size >= b->size)) {
		_buffer_reserve(b, size);
	}
}

static inline void buffer_write_u32_uc(struct buffer *b, uint32_t v)
{
	b->buf[b->index++] = (v & 0x000000FF);
	b->buf[b->index++] = (v & 0x0000FF00) >> 8;
	b->buf[b->index++] = (v & 0x00FF0000) >> 16;
	b->buf[b->index++] = (v & 0xFF000000) >> 24;
}

static inline void buffer_write_u32(struct buffer *b, uint32_t v)
{
	buffer_reserve(b, 4);
	buffer_write_u32_uc(b, v);
}

static inline void buffer_write_u32_at(struct buffer *buf, size_t index, uint32_t v)
{
	size_t tmp = buf->index;
	buf->index = index;
	buffer_write_u32(buf, v);
	buf->index = tmp;
}

static inline void buffer_write_u16_uc(struct buffer *b, uint16_t v)
{
	b->buf[b->index++] = (v & 0x00FF);
	b->buf[b->index++] = (v & 0xFF00) >> 8;
}

static inline void buffer_write_u16(struct buffer *b, uint16_t v)
{
	buffer_reserve(b, 2);
	buffer_write_u16_uc(b, v);
}

static inline void buffer_write_u8_uc(struct buffer *b, uint8_t v)
{
	b->buf[b->index++] = v;
}

static inline void buffer_write_u8(struct buffer *b, uint8_t v)
{
	buffer_reserve(b, 1);
	buffer_write_u8_uc(b, v);
}

static inline void buffer_write_float_uc(struct buffer *b, float f)
{
	union { float f; uint32_t u; } v;
	v.f = f;
	buffer_write_u32_uc(b, v.u);
}

static inline void buffer_write_float(struct buffer *b, float f)
{
	union { float f; uint32_t u; } v;
	v.f = f;
	buffer_write_u32(b, v.u);
}

void buffer_write_bytes(struct buffer *b, const uint8_t *bytes, size_t len) attr_nonnull;

/* Write a null-terminated string. */
void buffer_write_string(struct buffer *b, string s);
/* Write a string without a null terminator. */
void buffer_write_cstring(struct buffer *b, const char *s);
/* Write a null-terminated string. */
void buffer_write_cstringz(struct buffer *b, const char *s);
/* Write a length-prefixed string (nulib string as argument). */
void buffer_write_pascal_string(struct buffer *b, string s);
/* Write a length-prefixed string (cstring as argument). */
void buffer_write_pascal_cstring(struct buffer *b, const char *s);

static inline bool buffer_end(struct buffer *b)
{
	return b->index >= b->size;
}

static inline size_t buffer_remaining(struct buffer *b)
{
	return buffer_end(b) ? 0 : b->size - b->index;
}

static inline char *buffer_strdata(struct buffer *r)
{
	return (char*)r->buf + r->index;
}

static inline void buffer_seek(struct buffer *r, size_t off)
{
	r->index = off;
}

static inline void buffer_align(struct buffer *r, int p)
{
	r->index = (r->index + (p-1)) & ~(p-1);
}

struct bitbuffer {
	uint8_t *buf;
	size_t buf_size;
	unsigned index;
	unsigned current;
	unsigned mask;
};

static inline void bitbuffer_init(struct bitbuffer *buf, uint8_t *data, size_t data_size)
{
	buf->buf = data;
	buf->buf_size = data_size;
	buf->index = 0;
	buf->current = 0;
	buf->mask = 0;
}

static inline bool bitbuffer_read_bit(struct bitbuffer *b)
{
	if (b->mask == 0) {
		if (b->index >= b->buf_size)
			ERROR("bitbuffer overflowed");
		b->current = b->buf[b->index++];
		b->mask = 0x80;
	}
	bool bit = b->current & b->mask;
	b->mask >>= 1;
	return !!bit;
}

static inline unsigned bitbuffer_read_number(struct bitbuffer *b, int nr_bits)
{
	unsigned r = 0;
	for (int i = 0; i < nr_bits; i++) {
		r = (r << 1) | bitbuffer_read_bit(b);
	}
	return r;
}

static inline unsigned bitbuffer_read_zeros(struct bitbuffer *b, int limit)
{
	unsigned zeros = 0;
	for (int i = 0; i < limit && !bitbuffer_read_bit(b); i++) {
		zeros++;
	}
	return zeros;
}

static inline unsigned bitbuffer_read_ones(struct bitbuffer *b, int limit)
{
	unsigned ones = 0;
	for (int i = 0; i < limit && bitbuffer_read_bit(b); i++) {
		ones++;
	}
	return ones;
}

#endif // NULIB_BUFFER_H
