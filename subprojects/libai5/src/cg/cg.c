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
#include "nulib/file.h"
#include "ai5/arc.h"
#include "ai5/cg.h"

static struct cg *_cg_load(uint8_t *data, size_t size, enum cg_type type)
{
	switch (type) {
	case CG_TYPE_AKB: return akb_decode(data, size);
	case CG_TYPE_GP4: return gp4_decode(data, size);
	case CG_TYPE_GP8:  return gp8_decode(data, size);
	case CG_TYPE_G16: return gxx_decode(data, size, 16);
	case CG_TYPE_G24: return gxx_decode(data, size, 24);
	case CG_TYPE_G32: return gxx_decode(data, size, 32);
	case CG_TYPE_GCC: return gcc_decode(data, size);
	case CG_TYPE_PNG: return png_decode(data, size);
	case CG_TYPE_GPX: return gpx_decode(data, size);
	case CG_TYPE_GPR: return gpr_decode(data, size);
	}
	ERROR("invalid CG type: %d", type);
}

struct cg *cg_load(uint8_t *data, size_t size, enum cg_type type)
{
	struct cg *cg = _cg_load(data, size, type);
	if (cg)
		cg->ref = 1;
	return cg;
}

enum cg_type cg_type_from_name(const char *name)
{
	const char *ext = file_extension(name);
	if (!strcasecmp(ext, "akb")) return CG_TYPE_AKB;
	if (!strcasecmp(ext, "gp4")) return CG_TYPE_GP4;
	if (!strcasecmp(ext, "gp8")) return CG_TYPE_GP8;
	if (!strcasecmp(ext, "g16")) return CG_TYPE_G16;
	if (!strcasecmp(ext, "g24")) return CG_TYPE_G24;
	if (!strcasecmp(ext, "g32")) return CG_TYPE_G32;
	if (!strcasecmp(ext, "gcc")) return CG_TYPE_GCC;
	if (!strcasecmp(ext, "gpx")) return CG_TYPE_GPX;
	if (!strcasecmp(ext, "gpr")) return CG_TYPE_GPR;
	if (!strcasecmp(ext, "png")) return CG_TYPE_PNG;
	return -1;
}

struct cg *cg_load_arcdata(struct archive_data *data)
{
	enum cg_type type = cg_type_from_name(data->name);
	if (type < 0) {
		WARNING("Unrecognized CG type: %s", data->name);
		return NULL;
	}
	return cg_load(data->data, data->size, type);
}

struct cg *cg_copy(struct cg *cg)
{
	struct cg *copy = xmalloc(sizeof(struct cg));
	*copy = *cg;
	if (cg->palette) {
		copy->palette = xmalloc(256 * 4);
		memcpy(copy->palette, cg->palette, 256 * 4);
		copy->pixels = xmalloc(cg->metrics.w * cg->metrics.h);
		memcpy(copy->pixels, cg->pixels, cg->metrics.w * cg->metrics.h);
	} else {
		copy->pixels = xmalloc(cg->metrics.w * cg->metrics.h * 4);
		memcpy(copy->pixels, cg->pixels, cg->metrics.w * cg->metrics.h * 4);
	}
	copy->ref = 1;
	return copy;
}

uint8_t *_cg_depalettize(struct cg *cg)
{
	assert(cg->palette);
	uint8_t *px = xmalloc(cg->metrics.w * cg->metrics.h * 4);
	uint8_t *dst = px;
	uint8_t *src = cg->pixels;
	for (int i = 0; i < cg->metrics.w * cg->metrics.h; i++) {
		uint8_t *color = &cg->palette[*src++ * 4];
		*dst++ = color[2];
		*dst++ = color[1];
		*dst++ = color[0];
		*dst++ = 255;
	}
	return px;
}

void cg_depalettize(struct cg *cg)
{
	if (!cg->palette)
		return;

	uint8_t *px = _cg_depalettize(cg);
	free(cg->pixels);
	free(cg->palette);
	cg->palette = NULL;
	cg->pixels = px;
}

struct cg *cg_depalettize_copy(struct cg *cg)
{
	struct cg *copy = xmalloc(sizeof(struct cg));
	*copy = *cg;
	copy->ref = 1;

	copy->pixels = _cg_depalettize(cg);
	copy->palette = NULL;
	return copy;
}

bool _cg_write(struct cg *cg, FILE *out, enum cg_type type)
{
	switch (type) {
	case CG_TYPE_AKB: sys_warning("AKB write not supported"); return false;
	case CG_TYPE_GP4: sys_warning("GP4 write not supported"); return false;
	case CG_TYPE_GP8: sys_warning("GP8 write not supported"); return false;
	case CG_TYPE_G16: return gxx_write(cg, out, 16);
	case CG_TYPE_G24: return gxx_write(cg, out, 24);
	case CG_TYPE_G32: return gxx_write(cg, out, 32);
	case CG_TYPE_GCC: sys_warning("GCC write not supported"); return false;
	case CG_TYPE_GPX: sys_warning("GPX write not supported"); return false;
	case CG_TYPE_GPR: sys_warning("GPR write not supported"); return false;
	case CG_TYPE_PNG: return png_write(cg, out);
	}
	ERROR("Invalid CG type: %d", type);
}

bool cg_write(struct cg *cg, FILE *out, enum cg_type type)
{
	if (cg->palette) {
		struct cg *copy = cg_depalettize_copy(cg);
		bool r = _cg_write(copy, out, type);
		cg_free(copy);
		return r;
	} else {
		return _cg_write(cg, out, type);
	}
}

struct cg *cg_alloc(void)
{
	struct cg *cg = xcalloc(1, sizeof(struct cg));
	cg->ref = 1;
	return cg;
}

struct cg *cg_alloc_indexed(unsigned w, unsigned h)
{
	struct cg *cg = cg_alloc();
	cg->metrics.w = w;
	cg->metrics.h = h;
	cg->metrics.bpp = 8;
	cg->pixels = xcalloc(w, h);
	cg->palette = xcalloc(256, 4);
	return cg;
}

struct cg *cg_alloc_direct(unsigned w, unsigned h)
{
	struct cg *cg = cg_alloc();
	cg->metrics.w = w;
	cg->metrics.h = h;
	cg->metrics.bpp = 24;
	cg->pixels = xcalloc(w * 4, h);
	return cg;
}

void cg_free(struct cg *cg)
{
	if (!cg)
		return;
	if (cg->ref == 0)
		ERROR("double-free of CG");
	if (--cg->ref == 0) {
		free(cg->pixels);
		free(cg->palette);
		free(cg);
	}
}
