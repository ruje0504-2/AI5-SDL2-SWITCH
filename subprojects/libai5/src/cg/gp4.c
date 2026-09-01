/* Copyright (C) 2023 Nunuhara Cabbage <nunuhara@haniwa.technology>
 *
 * Adapted from Project ANISE
 * Copyright (C) 2002-2004 Yoon, Kyung Dam (tomyun)
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

#include <stdlib.h>
#include <string.h>

#include "nulib.h"
#include "nulib/little_endian.h"
#include "ai5/cg.h"

#define VIDEO_COLOR 16
#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 400

#define DECODE_PIXEL 0
#define DECODE_RLE   1

struct bitstream {
	uint8_t *data;
	unsigned size;
	unsigned byte_index;
	uint8_t bit_index;
	bool warned;
};

uint8_t bitstream_read_bit(struct bitstream *b)
{
	if (b->byte_index >= b->size) {
		if (!b->warned) {
			WARNING("Attempted to read beyond end of file");
			b->warned = true;
		}
		return 0;
	}
	uint8_t bit = (b->data[b->byte_index] >> b->bit_index) & 1;
	if (b->bit_index == 0) {
		b->bit_index = 7;
		b->byte_index++;
	} else {
		b->bit_index--;
	}
	return bit;
}

uint16_t bitstream_read_bits(struct bitstream *b, uint8_t n)
{
	uint16_t bits = 0;
	for (int i = 0; i < n; i++) {
		bits = (bits << 1) | bitstream_read_bit(b);
	}
	return bits;
}

void cg_write_px(struct cg *cg, unsigned x, unsigned y, uint8_t color)
{
	if (x >= cg->metrics.w || y >= cg->metrics.h) {
		WARNING("Attempted to write to invalid pixel index: %u,%u", x, y);
		return;
	}
	cg->pixels[y * cg->metrics.w + x] = color;
}

uint8_t cg_get_px(struct cg *cg, unsigned x, unsigned y)
{
	if (x >= cg->metrics.w || y >= cg->metrics.h) {
		WARNING("Attempted to read from invalid pixel index: %u,%u", x, y);
		return 0;
	}
	return cg->pixels[y * cg->metrics.w + x];
}

static void decode_rle_pos(struct bitstream *b, int *x, int *y)
{
	int hori, vert;
	if (bitstream_read_bit(b) == 0) {
		hori = 1;
		vert = (int)(bitstream_read_bits(b, 4) - 8);
	} else if (bitstream_read_bit(b) == 0) {
		hori = 0;
		vert = (int)(bitstream_read_bits(b, 3) - 8);
		if (vert <= -7) {
			if (vert == -7) {
				vert = 0;
			}
			vert = vert - 8;
		}
	} else {
		hori = 1;
		do {
			hori++;
		} while (bitstream_read_bit(b) == 1);
		vert = (int) (bitstream_read_bits(b, 4) - 8);
	}
	*x = *x - (hori * 4);
	*y = *y + vert;
}

static uint16_t decode_rle_length(struct bitstream *b)
{
	uint16_t length = 0;
	if (bitstream_read_bit(b) == 0) {
		length = bitstream_read_bit(b) + 2;
	} else if (bitstream_read_bit(b) == 0) {
		length = bitstream_read_bits(b, 2) + 4;
	} else if (bitstream_read_bit(b) == 0) {
		length = bitstream_read_bits(b, 3) + 8;
	} else {
		length = bitstream_read_bits(b, 6) + 16;
		if (length >= 79) {
			length = bitstream_read_bits(b, 10) + 79;
		}
	}
	return length;
}

static void decode(struct bitstream *b, unsigned dst_x, uint8_t table[17][16], struct cg *cg)
{
	unsigned dst_y = 0;
	int table_index = VIDEO_COLOR;

	while (dst_y < cg->metrics.h) {
		switch (bitstream_read_bit(b)) {
		case DECODE_PIXEL:
			for (unsigned x = 0; x < 4; x++) {
				uint8_t color = table[table_index][0];
				int color_index = 0;

				while (bitstream_read_bit(b) == 1) {
					color_index++;
					table[table_index][0] = table[table_index][color_index];
					table[table_index][color_index] = color;
					color = table[table_index][0];
				}

				table_index = color;
				cg_write_px(cg, dst_x + x, dst_y, color);
			}
			dst_y++;
			break;
		case DECODE_RLE: {
			// decode offset of pixel to copy
			int replica_x = dst_x, replica_y = dst_y;
			decode_rle_pos(b, &replica_x, &replica_y);

			// decode number of pixels to copy
			uint16_t length = decode_rle_length(b);

			// copy pixels
			for (uint16_t y = 0; y < length; y++, dst_y++) {
				for (uint16_t x = 0; x < 4; x++) {
					uint8_t color = cg_get_px(cg, replica_x + x, replica_y + y);
					cg_write_px(cg, dst_x + x, dst_y, color);
				}
			}
			break;
		}
		}
	}
}

struct cg *gp4_decode(uint8_t *data, size_t size)
{
	// read header
	struct cg *cg = xcalloc(1, sizeof(struct cg));
	cg->metrics.x = be_get16(data, 0);
	cg->metrics.y = be_get16(data, 2);
	cg->metrics.w = be_get16(data, 4) + 1;
	cg->metrics.h = be_get16(data, 6) + 1;
	cg->metrics.bpp = 8;

	cg->pixels = xcalloc(cg->metrics.w, cg->metrics.h);
	cg->palette = xcalloc(4, 256);

	uint8_t table[VIDEO_COLOR+1][VIDEO_COLOR];
	for (int i = 0; i < VIDEO_COLOR+1; i++) {
		for (int j = 0; j < VIDEO_COLOR; j++) {
			table[i][j] = (i + j) & 0xf;
		}
	}

	// decode palette
	for (int i = 0; i < VIDEO_COLOR; i++) {
		uint16_t c = be_get16(data, 8 + i*2);
		uint8_t g = c >> 12 & 0xf;
		uint8_t r = c >>  7 & 0xf;
		uint8_t b = c >>  2 & 0xf;
		r |= r << 4;
		g |= g << 4;
		b |= b << 4;
		cg->palette[i*4 + 0] = b;
		cg->palette[i*4 + 1] = g;
		cg->palette[i*4 + 2] = r;
		cg->palette[i*4 + 3] = 0;
	}

	struct bitstream b = {
		.data = data,
		.size = size,
		.byte_index = 40,
		.bit_index = 7,
		.warned = false
	};

	// decode pixels
	unsigned dst_x = 0;
	for (unsigned x = 0; x < (cg->metrics.w / 4); x++, dst_x += 4) {
		decode(&b, dst_x, table, cg);
	}

	return cg;
}
