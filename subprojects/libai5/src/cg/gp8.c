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

#include <stdlib.h>
#include <string.h>

#include "nulib.h"
#include "nulib/little_endian.h"
#include "ai5/cg.h"
#include "ai5/lzss.h"

struct cg *gp8_decode(uint8_t *data, size_t size)
{
	struct cg *cg = xcalloc(1, sizeof(struct cg));
	cg->metrics.x = le_get16(data, 0);
	cg->metrics.y = le_get16(data, 2);
	cg->metrics.w = le_get16(data, 4);
	cg->metrics.h = le_get16(data, 6);
	cg->metrics.bpp = 8;

	uint16_t stride = cg->metrics.w;
	if (stride & 3)
		stride = (stride & 0xfffc) + 4;

	cg->palette = xmalloc(256 * 4);
	memcpy(cg->palette, data + 8, 256 * 4);

	size_t pos = 8 + 256 * 4;
	size_t px_size;
	uint8_t *px = lzss_decompress(data + pos, size - pos, &px_size);
	if (px_size != stride * cg->metrics.h) {
		WARNING("Unexpected size for GP8 pixel data (expected %u; got %u)",
				(unsigned)stride * cg->metrics.h,
				(unsigned)px_size);
		if (px_size < stride * cg->metrics.h) {
			free(cg->palette);
			free(cg->pixels);
			free(cg);
			return NULL;
		}
	}

	cg->pixels = xmalloc(cg->metrics.w * cg->metrics.h);
	for (int i = 0; i < cg->metrics.h; i++) {
		uint8_t *dst = cg->pixels + cg->metrics.w * i;
		uint8_t *src = px + stride * (cg->metrics.h - (i + 1));
		memcpy(dst, src, cg->metrics.w);
	}
	free(px);

	return cg;
}
