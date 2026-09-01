/* Copyright (C) 2024 Nunuhara Cabbage <nunuhara@haniwa.technology>
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

// Adapted from GARbro
//
// Copyright (C) 2015 by morkt
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "nulib.h"
#include "nulib/buffer.h"
#include "nulib/little_endian.h"
#include "ai5/cg.h"
#include "ai5/lzss.h"

struct gcc_bitbuffer {
	uint8_t *buf;
	int index;
	int current;
	int mask;
};

void gcc_bitbuffer_seek_byte(struct gcc_bitbuffer *b, int idx)
{
	b->index = idx;
	b->mask = 0x80;
}

bool gcc_bitbuffer_read_bit(struct gcc_bitbuffer *b)
{
	b->mask <<= 1;
	if (0x100 == b->mask) {
		b->current = b->buf[b->index++];
		b->mask = 1;
	}
	return 0 != (b->current & b->mask);
}

static void decode_chunk(struct buffer *out, uint8_t *chunk, int chunk_size)
{
	static uint16_t chunk_next_index[0x10000];

	// get count of bytes in chunk
	uint16_t byte_count[256] = {0};
	for (int i = 0; i < chunk_size; i++) {
		++byte_count[chunk[2 + i]];
	}

	// get sum of bytes less than i for [0..i..255]
	uint16_t bytes_lt[256];
	for (uint16_t i = 0, count = 0; i < 256; i++) {
		bytes_lt[i] = count;
		count += byte_count[i];
		byte_count[i] = 0;
	}

	// initialize chunk_next_index
	for (int i = 0; i < chunk_size; i++) {
		int b = chunk[2 + i];
		int r = byte_count[b] + bytes_lt[b];
		chunk_next_index[r] = (uint16_t)i;
		byte_count[b]++;
	}

	// decode chunk
	int chunk_i = chunk_next_index[le_get16(chunk, 0)];
	for (int i = 0; i < chunk_size; i++) {
		buffer_write_u8(out, chunk[2 + chunk_i]);
		chunk_i = chunk_next_index[chunk_i];
	}
}

static int read_count(struct gcc_bitbuffer *b)
{
	int result = 1;
	int bit_count = 0;
	while (!gcc_bitbuffer_read_bit(b)) {
		bit_count++;
	}
	for (; bit_count > 0; bit_count--) {
		result <<= 1;
		if (gcc_bitbuffer_read_bit(b))
			result |= 1;
	}
	return result;
}

static void read_compressed_chunk(struct gcc_bitbuffer *control, struct buffer *in,
		struct buffer *out, uint8_t *chunk, int chunk_size)
{
	uint8_t buf_a[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
	uint8_t buf_b[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

	int chunk_i = 0;
	int8_t prev_b = -1;
	while (chunk_i < chunk_size + 2) {
		int b;
		uint8_t buf_b_i;
		if (!gcc_bitbuffer_read_bit(control)) {
			if (gcc_bitbuffer_read_bit(control)) {
				buf_b_i = read_count(control) & 0xf;
				b = buf_b[buf_b_i];
				chunk[chunk_i++] = b;
			} else {
				if (gcc_bitbuffer_read_bit(control)) {
					int n = read_count(control);
					if (gcc_bitbuffer_read_bit(control))
						b = (prev_b - n) & 0xff;
					else
						b = (prev_b + n) & 0xff;
				} else {
					b = buffer_read_u8(in);
				}
				chunk[chunk_i++] = b;
				for (buf_b_i = 0; buf_b_i < 0xf && buf_b[buf_b_i] != b; buf_b_i++)
					; // nothing
			}
		} else {
			int buf_a_i;
			int count = read_count(control);
			if (gcc_bitbuffer_read_bit(control)) {
				buf_a_i = 0;
				b = buf_a[0];
			} else if (gcc_bitbuffer_read_bit(control)) {
				buf_a_i = read_count(control) & 0xf;
				b = buf_a[buf_a_i];
			} else {
				if (gcc_bitbuffer_read_bit(control)) {
					int n = read_count(control);
					if (gcc_bitbuffer_read_bit(control))
						b = (prev_b - n) & 0xff;
					else
						b = (prev_b + n) & 0xff;
				} else {
					b = buffer_read_u8(in);
				}
				for (buf_a_i = 0; buf_a_i < 0xf && buf_a[buf_a_i] != b; buf_a_i++)
					; // nothing
			}
			if (buf_a_i != 0) {
				for (int i = buf_a_i & 0xf; i > 0; --i)
					buf_a[i] = buf_a[i - 1];
				buf_a[0] = b;
			}
			for (int n = 0; n < count; n++)
				chunk[chunk_i++] = b;

			for (buf_b_i = 0; buf_b_i < 0xf && buf_b[buf_b_i] != b; buf_b_i++)
				; // nothing
		}
		if (buf_b_i) {
			for (int i = buf_b_i & 0xf; i != 0; --i)
				buf_b[i] = buf_b[i - 1];
			buf_b[0] = b;
		}
		prev_b = b;
	}

	decode_chunk(out, chunk, chunk_size);
}

static void read_raw_chunk(struct gcc_bitbuffer *control, struct buffer *in,
		struct buffer *out, int chunk_size)
{
	int n = 0;
	while (n < chunk_size) {
		if (!gcc_bitbuffer_read_bit(control)) {
			// single pixel
			buffer_write_u8(out, buffer_read_u8(in));
			buffer_write_u8(out, buffer_read_u8(in));
			buffer_write_u8(out, buffer_read_u8(in));
			n += 3;
		} else {
			// RLE
			int count = read_count(control);
			uint8_t b = buffer_read_u8(in);
			uint8_t g = buffer_read_u8(in);
			uint8_t r = buffer_read_u8(in);
			for (int i = 0; i < count; ++i) {
				buffer_write_u8(out, b);
				buffer_write_u8(out, g);
				buffer_write_u8(out, r);
			}
			n += 3 * count;
		}
	}
}

static uint8_t *alt_unpack(struct buffer *data, int offset, size_t total)
{
	uint8_t chunk[0x10001] = {0};

	// control bitstream
	struct gcc_bitbuffer control = { .buf = data->buf };
	gcc_bitbuffer_seek_byte(&control, offset);

	// source data stream
	struct buffer in = *data;
	buffer_seek(&in, offset + le_get32(data->buf, 0x10));

	// output data stream
	struct buffer out;
	buffer_init(&out, xcalloc(1, total), total);

	int dst = 0;
	while (dst < total) {
		int chunk_size = min(total - dst, 0xffff);
		if (gcc_bitbuffer_read_bit(&control)) {
			read_compressed_chunk(&control, &in, &out, chunk, chunk_size);
		} else {
			read_raw_chunk(&control, &in, &out, chunk_size);
		}
		dst += chunk_size;
		if (dst != out.index)
			NOTICE("???");
	}
	return out.buf;
}

static uint8_t *lzss_unpack(struct buffer *data, int offset, size_t total)
{
	size_t unpacked_size = total;
	uint8_t *unpacked = lzss_decompress_with_limit(data->buf + offset,
			data->size - offset, &unpacked_size);
	if (unpacked_size != total) {
		WARNING("unexpected unpacked size: %u (expected %u)",
				(unsigned)unpacked_size, (unsigned)total);
		if (unpacked_size < total)
			unpacked = xrealloc(unpacked, total);
	}
	return unpacked;
}

uint8_t *unpack_alpha(struct buffer *data, unsigned *w, unsigned *h)
{
	// control bitstream
	struct gcc_bitbuffer control = { .buf = data->buf };
	int control_offset = 0x20 + le_get32(data->buf, 0x0c);
	gcc_bitbuffer_seek_byte(&control, control_offset);

	unsigned alpha_w = le_get16(data->buf, 0x18);
	unsigned alpha_h = le_get16(data->buf, 0x1a);
	unsigned total = alpha_w * alpha_h;

	uint8_t *alpha = xcalloc(1, total);

	struct buffer in = *data;
	buffer_seek(&in, control_offset + le_get32(data->buf, 0x1c));

	unsigned dst = 0;
	while (dst < total) {
		if (gcc_bitbuffer_read_bit(&control)) {
			// RLE
			int count = read_count(&control);
			uint8_t v = buffer_read_u8(&in);
			for (int i = 0; i < count; ++ i) {
				alpha[dst++] = v;
			}
		} else {
			// single pixel
			alpha[dst++] = buffer_read_u8(&in);
		}
	}
	*w = alpha_w;
	*h = alpha_h;
	return alpha;
}

struct cg *gcc_decode(uint8_t *data, size_t size)
{
	if (size < 12)
		return NULL;
	struct cg *cg = xcalloc(1, sizeof(struct cg));
	cg->metrics.x = le_get16(data, 4);
	cg->metrics.y = le_get16(data, 6);
	cg->metrics.w = le_get16(data, 8);
	cg->metrics.h = le_get16(data, 10);
	if (data[3] == 'm') {
		cg->metrics.bpp = 32;
		cg->metrics.has_alpha = true;
	} else {
		cg->metrics.bpp = 24;
		cg->metrics.has_alpha = false;
	}

	struct buffer data_buf;
	buffer_init(&data_buf, data, size);

	uint8_t *color;
	uint8_t *alpha = NULL;
	unsigned alpha_w = 0;
	unsigned alpha_h = 0;
	switch (le_get32(data, 0)) {
	case 0x6e343247: // G24n
		color = lzss_unpack(&data_buf, 0x14, cg->metrics.w * cg->metrics.h * 3);
		break;
	case 0x6d343247: // G24m
		color = lzss_unpack(&data_buf, 0x20, cg->metrics.w * cg->metrics.h * 3);
		alpha = unpack_alpha(&data_buf, &alpha_w, &alpha_h);
		break;
	case 0x6e343252: // R24n
		color = alt_unpack(&data_buf, 0x14, cg->metrics.w * cg->metrics.h * 3);
		break;
	case 0x6d343252: // R24m
		color = alt_unpack(&data_buf, 0x20, cg->metrics.w * cg->metrics.h * 3);
		alpha = unpack_alpha(&data_buf, &alpha_w, &alpha_h);
		break;
	default:
		WARNING("unsupported CGG image type");
		free(cg);
		return NULL;
	}

	cg->pixels = xmalloc(cg->metrics.w * cg->metrics.h * 4);

	for (int row = 0; row < cg->metrics.h; row++) {
		int dst_row = cg->metrics.h - (row + 1);
		uint8_t *src = color + row * cg->metrics.w * 3;
		uint8_t *dst = cg->pixels + dst_row * cg->metrics.w * 4;
		for (int col = 0; col < cg->metrics.w; col++, src += 3, dst += 4) {
			dst[0] = src[2];
			dst[1] = src[1];
			dst[2] = src[0];
			dst[3] = 255;
		}
	}

	if (alpha) {
		// XXX: mask is full size (including x/y offsets)
		if (cg->metrics.x + cg->metrics.w > alpha_w) {
			WARNING("alpha width is too small");
		} else if (cg->metrics.y + cg->metrics.h > alpha_h) {
			WARNING("alpha height is too small");
		} else {
			for (int row = 0; row < cg->metrics.h; row++) {
				int dst_row = cg->metrics.h - (row + 1);
				uint8_t *src = alpha + cg->metrics.x + row * alpha_w;
				uint8_t *dst = cg->pixels + dst_row * cg->metrics.w * 4;
				for (int col = 0; col < cg->metrics.w; col++, src++, dst += 4) {
					dst[3] = *src;
				}
			}
		}
	}

	free(color);
	free(alpha);
	return cg;
}
