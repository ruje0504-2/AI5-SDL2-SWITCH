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
#include "ai5/lzss.h"

#define FLAG_NO_ALPHA 0x40000000

struct cg *akb_decode(uint8_t *data, size_t size)
{
	if (size < 32)
		return NULL;
	if (strncmp((char*)data, "AKB ", 4))
		return NULL;

	struct cg *cg = xcalloc(1, sizeof(struct cg));
	cg->metrics.x = le_get32(data, 16);
	cg->metrics.y = le_get32(data, 20);
	cg->metrics.w = le_get16(data, 24) - cg->metrics.x;
	cg->metrics.h = le_get16(data, 28) - cg->metrics.y;
	uint32_t flags = le_get32(data, 8);

	if (cg->metrics.w == 0 || cg->metrics.h == 0) {
		WARNING("CG dimension is 0: %dx%d", cg->metrics.w, cg->metrics.h);
		free(cg);
		return NULL;
	}

	// decompress
	size_t decomp_size;
	uint8_t *decomp = lzss_decompress(data + 32, size - 32, &decomp_size);
	if (flags & FLAG_NO_ALPHA) {
		// RGB
		if (decomp_size < cg->metrics.w * cg->metrics.h * 3) {
			WARNING("Unexpected decompressed size: %u (expected %u)",
					(unsigned)decomp_size,
					(unsigned)cg->metrics.w * cg->metrics.h * 3);
			free(decomp);
			free(cg);
			return NULL;
		}
		cg->pixels = xmalloc(cg->metrics.w * cg->metrics.h * 4);
		for (int src_row = 0, row = cg->metrics.h - 1; row >= 0; src_row++, row--) {
			uint8_t *src = decomp + src_row * cg->metrics.w * 3;
			uint8_t *dst = cg->pixels + row * cg->metrics.w * 4;
			for (int col = 0; col < cg->metrics.w; col++, src += 3, dst += 4) {
				dst[0] = src[2];
				dst[1] = src[1];
				dst[2] = src[0];
				dst[3] = 0; // XXX: alpha is corrected in color decode step
			}
		}
	} else {
		// RGBA
		if (decomp_size < cg->metrics.w * cg->metrics.h * 4) {
			WARNING("Unexpected decoded size: %u (expected %u)",
					(unsigned)decomp_size,
					(unsigned)cg->metrics.w * cg->metrics.h * 4);
			free(decomp);
			free(cg);
			return NULL;
		}
		cg->pixels = xmalloc(cg->metrics.w * cg->metrics.h * 4);
		for (int src_row = 0, row = cg->metrics.h - 1; row >= 0; src_row++, row--) {
			uint8_t *src = decomp + src_row * cg->metrics.w * 4;
			uint8_t *dst = cg->pixels + row * cg->metrics.w * 4;
			for (int col = 0; col < cg->metrics.w; col++, src += 4, dst += 4) {
				dst[0] = src[2];
				dst[1] = src[1];
				dst[2] = src[0];
				dst[3] = src[3];
			}
		}
	}
	free(decomp);

	// decode colors
	uint8_t *px = cg->pixels;
	if (flags & FLAG_NO_ALPHA)
		px[3] = 255;
	for (int col = 1; col < cg->metrics.w; col++) {
		int prev = col - 1;
		px[col * 4 + 0] = px[col * 4 + 0] + px[prev * 4 + 0];
		px[col * 4 + 1] = px[col * 4 + 1] + px[prev * 4 + 1];
		px[col * 4 + 2] = px[col * 4 + 2] + px[prev * 4 + 2];
		if (flags & FLAG_NO_ALPHA)
			px[col * 4 + 3] = 255;
		else
			px[col * 4 + 3] = px[col * 4 + 3] + px[prev * 4 + 3];
	}

	for (int row = 1; row < cg->metrics.h; row++) {
		uint8_t *prev = px + (row-1) * cg->metrics.w * 4;
		uint8_t *dst = px + row * cg->metrics.w * 4;
		for (int col = 0; col < cg->metrics.w; col++) {
			dst[col * 4 + 0] = dst[col * 4 + 0] + prev[col * 4 + 0];
			dst[col * 4 + 1] = dst[col * 4 + 1] + prev[col * 4 + 1];
			dst[col * 4 + 2] = dst[col * 4 + 2] + prev[col * 4 + 2];
			dst[col * 4 + 3] = dst[col * 4 + 3] + prev[col * 4 + 3];
		}
	}

	return cg;
}
