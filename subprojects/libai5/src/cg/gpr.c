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

#include <stdlib.h>
#include <string.h>

#include "nulib.h"
#include "nulib/buffer.h"
#include "ai5/cg.h"

/*
 * GPR images are similar to GPX images, but with 16-bit color and an
 * optional alpha channel.
 */

static struct { int x, y; } point_offset[] = {
	{ -1,  0 }, { -2,  0 }, { -3,  0 }, { -4,  0 },
	// XXX: pattern changes
	{  4, -1 }, {  3, -1 }, {  2, -1 }, {  1, -1 },
	{  0, -1 }, { -1, -1 }, { -2, -1 }, { -3, -1 }, { -4, -1 },
	{  4, -2 }, {  3, -2 }, {  2, -2 }, {  1, -2 },
	{  0, -2 }, { -1, -2 }, { -2, -2 }, { -3, -2 }, { -4, -2 },
	{  4, -3 }, {  3, -3 }, {  2, -3 }, {  1, -3 },
	{  0, -3 }, { -1, -3 }, { -2, -3 }, { -3, -3 }, { -4, -3 },
	{  4, -4 }, {  3, -4 }, {  2, -4 }, {  1, -4 },
	{  0, -4 }, { -1, -4 }, { -2, -4 }, { -3, -4 }, { -4, -4 },
	{  4, -5 }, {  3, -5 }, {  2, -5 }, {  1, -5 },
	{  0, -5 }, { -1, -5 }, { -2, -5 }, { -3, -5 }, { -4, -5 },
	{  4, -6 }, {  3, -6 }, {  2, -6 }, {  1, -6 },
	{  0, -6 }, { -1, -6 }, { -2, -6 }, { -3, -6 }, { -4, -6 },
	// XXX: pattern changes
	{ -1, -7 }, {  0, -7 }, {  1, -7 }, {  2, -7 }, {  3, -7 }, {  4, -7 },
};

static struct { int x, y; } point_offset_v[] = {
	{ -1,  0 }, { -2,  0 }, { -3,  0 }, { -4,  0 },
	// XXX: pattern changes
	{  4, -1 }, {  3, -1 }, {  2, -1 }, {  1, -1 },
	{  0, -1 }, { -1, -1 }, { -2, -1 }, { -3, -1 }, { -4, -1 },
	{  4, -2 }, {  3, -2 }, {  2, -2 }, {  1, -2 },
	{  0, -2 }, { -1, -2 }, { -2, -2 }, { -3, -2 }, { -4, -2 },
	{  4, -3 }, {  3, -3 }, {  2, -3 }, {  1, -3 },
	{  0, -3 }, { -1, -3 }, { -2, -3 }, { -3, -3 }, { -4, -3 },
	{  4, -4 }, {  3, -4 }, {  2, -4 }, {  1, -4 },
	{  0, -4 }, { -1, -4 }, { -2, -4 }, { -3, -4 }, { -4, -4 },
	{  4, -5 }, {  3, -5 }, {  2, -5 }, {  1, -5 },
	{  0, -5 }, { -1, -5 }, { -2, -5 }, { -3, -5 }, { -4, -5 },
	{  4, -6 }, {  3, -6 }, {  2, -6 }, {  1, -6 },
	{  0, -6 }, { -1, -6 }, { -2, -6 }, { -3, -6 }, { -4, -6 },
	// XXX: pattern changes
	{  1, -7 }, {  0, -7 }, {  1, -7 }, { -2, -7 }, { -3, -7 }, { -4, -7 },
};

static uint8_t *pixel_offset(struct cg *cg, int col, int row)
{
	return cg->pixels + row * cg->metrics.w * 4 + col * 4;
}

static void write_bgr555(uint8_t *dst, uint16_t px)
{
	dst[0] = (((px >> 10) & 0x1f) / 31.f) * 255;
	dst[1] = (((px >> 5) & 0x1f) / 31.f) * 255;
	dst[2] = ((px & 0x1f) / 31.f) * 255;
	dst[3] = 255;
}

static void gpr_decode_pixels_horizontal(struct cg *cg, uint8_t *data, size_t size)
{
	struct bitbuffer b;
	bitbuffer_init(&b, data, size);

	for (int row = 0; row < cg->metrics.h; row++) {
		for (int col = 0; col < cg->metrics.w;) {
			uint8_t *dst = pixel_offset(cg, col, row);
			switch (bitbuffer_read_ones(&b, 2)) {
			case 0:
				// literal pixel
				write_bgr555(dst, bitbuffer_read_number(&b, 15));
				col++;
				break;
			case 1: {
				// single pixel at (x,y) offset from dst
				unsigned i = bitbuffer_read_number(&b, 6);
				uint8_t *src = pixel_offset(cg, col + point_offset[i].x,
						row + point_offset[i].y);
				memcpy(dst, src, 4);
				col++;
				break;
			}
			case 2: {
				// copy previously decoded bytes
				int x, y;
				gpx_decode_offset(&b, &x, &y);
				uint8_t *src = pixel_offset(cg, col + x, row + y);
				int len = gpx_decode_run_length(&b);
				for (int i = 0; i < len; i++, dst+=4, src+=4) {
					memcpy(dst, src, 4);
				}
				col += len;
				break;
			}
			}
		}
	}
}

static void gpr_decode_pixels_vertical(struct cg *cg, uint8_t *data, size_t size)
{
	struct bitbuffer b;
	bitbuffer_init(&b, data, size);

	for (int col = 0; col < cg->metrics.w; col++) {
		for (int row = 0; row < cg->metrics.h;) {
			uint8_t *dst = pixel_offset(cg, col, row);
			switch (bitbuffer_read_ones(&b, 2)) {
			case 0:
				write_bgr555(dst, bitbuffer_read_number(&b, 15));
				row++;
				break;
			case 1: {
				unsigned i = bitbuffer_read_number(&b, 6);
				uint8_t *src = pixel_offset(cg, col + point_offset_v[i].y,
						row + point_offset_v[i].x);
				memcpy(dst, src, 4);
				row++;
				break;
			}
			case 2: {
				int x, y;
				gpx_decode_offset(&b, &y, &x);
				uint8_t *src = pixel_offset(cg, col + x, row + y);
				int len = gpx_decode_run_length(&b);
				for (int i = 0; i < len; i++) {
					memcpy(dst, src, 4);
					dst += cg->metrics.w * 4;
					src += cg->metrics.w * 4;
				}
				row += len;
				break;
			}
			}
		}
	}
}

// XXX: This is identical to gpx_decode_horizontal, except for how we write to the
//      destination cg.
static void gpr_decode_mask_horizontal(struct cg *cg, uint8_t *data, size_t size)
{
	struct bitbuffer b;
	bitbuffer_init(&b, data, size);

	for (int row = 0; row < cg->metrics.h; row++) {
		for (int col = 0; col < cg->metrics.w;) {
			uint8_t *dst = pixel_offset(cg, col, row);
			if (!bitbuffer_read_bit(&b)) {
				// copy previously decoded bytes
				int x, y;
				gpx_decode_offset(&b, &x, &y);
				uint8_t *src = pixel_offset(cg, col + x, row + y);
				int len = gpx_decode_run_length(&b);
				for (int i = 0; i < len; i++, dst+=4, src+=4) {
					dst[3] = src[3];
				}
				col += len;
			} else {
				// literal byte
				uint8_t a = bitbuffer_read_number(&b, 8);
				a = a == 0x20 ? 255 : a * 8;
				dst[3] = 255 - a;
				col++;
			}
		}
	}
}

// XXX: This is identical to gpx_decode_vertical, except for how we write to the
//      destination cg.
static void gpr_decode_mask_vertical(struct cg *cg, uint8_t *data, size_t size)
{
	struct bitbuffer b;
	bitbuffer_init(&b, data, size);

	for (int col = 0; col < cg->metrics.w; col++) {
		for (int row = 0; row < cg->metrics.h;) {
			uint8_t *dst = pixel_offset(cg, col, row);
			if (!bitbuffer_read_bit(&b)) {
				// copy previously decoded bytes
				int x, y;
				gpx_decode_offset(&b, &y, &x);
				uint8_t *src = pixel_offset(cg, col + x, row + y);
				int len = gpx_decode_run_length(&b);
				for (int i = 0; i < len; i++) {
					dst[3] = src[3];
					dst += cg->metrics.w * 4;
					src += cg->metrics.w * 4;
				}
				row += len;
			} else {
				// literal byte
				uint8_t a = bitbuffer_read_number(&b, 8);
				a = a == 0x20 ? 255 : a * 8;
				dst[3] = 255 - a;
				row++;
			}
		}
	}
}

struct cg *gpr_decode(uint8_t *data, size_t size)
{
	if (size < 12)
		return NULL;

	bool mask;
	if (!strncmp((char*)data, "R15n", 4))
		mask = false;
	else if (!strncmp((char*)data, "R15m", 4))
		mask = true;
	else
		return NULL;

	struct cg *cg = xcalloc(1, sizeof(struct cg));
	cg->metrics.x = le_get16(data, 4);
	cg->metrics.y = le_get16(data, 6);
	cg->metrics.w = le_get16(data, 8);
	cg->metrics.h = le_get16(data, 10);
	uint16_t vertical = le_get16(data, 12);

	cg->pixels = xcalloc(cg->metrics.w * 4, cg->metrics.h);
	if (mask) {
		uint32_t mask_ptr = le_get32(data, 14);
		if (mask_ptr >= size) {
			free(cg->pixels);
			free(cg);
			return NULL;
		}
		if (vertical & 1)
			gpr_decode_pixels_vertical(cg, data + 18, size - 18);
		else
			gpr_decode_pixels_horizontal(cg, data + 18, size - 18);
		if (vertical & 2)
			gpr_decode_mask_vertical(cg, data + mask_ptr, size - mask_ptr);
		else
			gpr_decode_mask_horizontal(cg, data + mask_ptr, size - mask_ptr);
	} else {
		if (vertical & 1)
			gpr_decode_pixels_vertical(cg, data + 14, size - 14);
		else
			gpr_decode_pixels_horizontal(cg, data + 14, size - 14);
	}

	return cg;
}
