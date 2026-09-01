/* Copyright (C) 2025 Nunuhara Cabbage <nunuhara@haniwa.technology>
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

#include "nulib.h"
#include "nulib/buffer.h"
#include "ai5/cg.h"

/*
 * GPX images are compressed using a 2D variant of LZSS.
 * The data can be encoded horizontally or vertically (depending
 * presumably on whichever achieves the best compression ratio).
 */

/*
 * Decode the "length" portion of an (offset,length) pair.
 */
int gpx_decode_run_length(struct bitbuffer *b)
{
	int len = 0;
	switch (bitbuffer_read_zeros(b, 5)) {
	case 0: len = 0x2 + bitbuffer_read_bit(b); break;
	case 1: len = 0x4 + bitbuffer_read_number(b, 2); break;
	case 2: len = 0x8 + bitbuffer_read_number(b, 3); break;
	case 3: len = 0x10 + bitbuffer_read_number(b, 6); break;
	case 4: len = 0x50 + bitbuffer_read_number(b, 8); break;
	case 5: len = 0x150 + bitbuffer_read_number(b, 10); break;
	}
	return len;
}

/*
 * Decode the "offset" portion of an (offset,length) pair. This is
 * 2D offset.
 */
void gpx_decode_offset(struct bitbuffer *b, int *x_off, int *y_off)
{
	// Offset when reading from the current line. Always negative.
	static int same_line_offsets[8] = {
		-1, -2, -4, -6, -8, -12, -16, -20
	};
	// Offset when reading from an earlier (completed) line. Can be positive.
	static int prev_line_offsets[16] = {
		-20, -16, -12, -8, -6, -4, -2, -1, 0, 1, 2, 4, 6, 8, 12, 16
	};

	if (!bitbuffer_read_bit(b)) {
		// source is more than 1 line distant
		if (!bitbuffer_read_bit(b)) {
			// 2-bit(+4) offset
			*y_off = -(bitbuffer_read_number(b, 2) + 4);
		} else {
			// 1-bit(+2) offset
			*y_off = -(bitbuffer_read_bit(b) + 2);
		}
		*x_off = prev_line_offsets[bitbuffer_read_number(b, 4)];
	} else {
		if (bitbuffer_read_bit(b)) {
			// previous line
			*y_off = -1;
			*x_off = prev_line_offsets[bitbuffer_read_number(b, 4)];
		} else {
			// current line
			*y_off = 0;
			*x_off = same_line_offsets[bitbuffer_read_number(b, 3)];
		}
	}
}

/*
 * Calculate the offset to a given x,y coordinate in an indexed CG.
 */
static uint8_t *pixel_offset(struct cg *cg, int col, int row)
{
	return cg->pixels + row * cg->metrics.w + col;
}

/*
 * Decode a horizontally encoded GPX image.
 */
static void gpx_decode_horizontal(struct cg *cg, uint8_t *data, size_t size)
{
	struct bitbuffer b;
	bitbuffer_init(&b, data, size);

	for (int row = 0; row < cg->metrics.h; row++) {
		for (int col = 0; col < cg->metrics.w;) {
			uint8_t *dst = pixel_offset(cg, col, row);
			if (!bitbuffer_read_bit(&b)) {
				// copy previously decoded bytes
				int x_off, y_off;
				gpx_decode_offset(&b, &x_off, &y_off);
				uint8_t *src = pixel_offset(cg, col + x_off, row + y_off);
				int len = gpx_decode_run_length(&b);
				for (int i = 0; i < len; i++) {
					*dst++ = *src++;
				}
				// XXX: can't use memcpy or memmove here due to overlap. E.g.
				//      a literal byte followed by a long run at that byte
				//      (reading past the original `dst` location) is common.
				//memcpy(dst, src, len);
				col += len;
			} else {
				// literal byte
				*dst = bitbuffer_read_number(&b, 8);
				col++;
			}
		}
	}
}

/*
 * Decode a vertically encoded GPX image.
 */
static void gpx_decode_vertical(struct cg *cg, uint8_t *data, size_t size)
{
	struct bitbuffer b;
	bitbuffer_init(&b, data, size);

	for (int col = 0; col < cg->metrics.w; col++) {
		for (int row = 0; row < cg->metrics.h;) {
			uint8_t *dst = pixel_offset(cg, col, row);
			if (!bitbuffer_read_bit(&b)) {
				// copy previously decoded bytes
				int x_off, y_off;
				gpx_decode_offset(&b, &y_off, &x_off);
				uint8_t *src = pixel_offset(cg, col + x_off, row + y_off);
				int len = gpx_decode_run_length(&b);
				for (int i = 0; i < len; i++) {
					*dst = *src;
					dst += cg->metrics.w;
					src += cg->metrics.w;
				}
				row += len;
			} else {
				// literal byte
				*dst = bitbuffer_read_number(&b, 8);
				row++;
			}
		}
	}
}

/*
 * Decode a GPX image.
 */
struct cg *gpx_decode(uint8_t *data, size_t size)
{
	if (size < 0x2ce)
		return NULL;
	struct cg *cg = xcalloc(1, sizeof(struct cg));
	cg->metrics.x = le_get16(data, 0);
	cg->metrics.y = le_get16(data, 2);
	cg->metrics.w = le_get16(data, 4);
	cg->metrics.h = le_get16(data, 6);
	bool rotated = le_get16(data, 8);

	cg->palette = xcalloc(4, 256);
	int c = 10; // why?
	for (int i = 0; i < 236; i++) {
		cg->palette[(i+c)*4 + 2] = data[10 + i*3 + 0];
		cg->palette[(i+c)*4 + 1] = data[10 + i*3 + 1];
		cg->palette[(i+c)*4 + 0] = data[10 + i*3 + 2];
		cg->palette[(i+c)*4 + 3] = 255;
	}

	unsigned stride = cg->metrics.w;
	cg->pixels = xcalloc(stride, cg->metrics.h);
	if (rotated)
		gpx_decode_vertical(cg, data + 0x2ce, size - 0x2ce);
	else
		gpx_decode_horizontal(cg, data + 0x2ce, size - 0x2ce);

	return cg;
}
