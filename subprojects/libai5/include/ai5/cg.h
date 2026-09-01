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

#ifndef AI5_CG_H
#define AI5_CG_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct archive_data;

enum cg_type {
	CG_TYPE_AKB,
	CG_TYPE_GP4,
	CG_TYPE_GP8,
	CG_TYPE_G16,
	CG_TYPE_G24,
	CG_TYPE_G32,
	CG_TYPE_GCC,
	CG_TYPE_GPR,
	CG_TYPE_GPX,
	CG_TYPE_PNG,
	//CG_TYPE_BMP,
};

struct cg_metrics {
	unsigned x;
	unsigned y;
	unsigned w;
	unsigned h;
	unsigned bpp;
	bool has_alpha;
};

struct cg {
	struct cg_metrics metrics;
	// XXX: if `palette` is non-NULL, it's a 256-color BGRx palette
	//      and `pixels` is 8-bit indexed. Otherwise `pixels` is
	//      RGBA.
	uint8_t *pixels;
	uint8_t *palette;
	unsigned ref;
};

enum cg_type cg_type_from_name(const char *name);

struct cg *cg_load(uint8_t *data, size_t size, enum cg_type type);
struct cg *cg_load_arcdata(struct archive_data *data);
struct cg *cg_copy(struct cg *cg);
void cg_depalettize(struct cg *cg);
struct cg *cg_depalettize_copy(struct cg *cg);

bool cg_write(struct cg *cg, FILE *out, enum cg_type type);

struct cg *cg_alloc(void);
struct cg *cg_alloc_indexed(unsigned w, unsigned h);
struct cg *cg_alloc_direct(unsigned w, unsigned h);
void cg_free(struct cg *cg);

struct cg *akb_decode(uint8_t *data, size_t size);
struct cg *gp4_decode(uint8_t *data, size_t size);
struct cg *gp8_decode(uint8_t *data, size_t size);
struct cg *gxx_decode(uint8_t *data, size_t size, unsigned bpp);
struct cg *png_decode(uint8_t *data, size_t size);
struct cg *gcc_decode(uint8_t *data, size_t size);
struct cg *gpx_decode(uint8_t *data, size_t size);
struct cg *gpr_decode(uint8_t *data, size_t size);

bool gxx_write(struct cg *cg, FILE *out, unsigned bpp);
bool png_write(struct cg *cg, FILE *out);

struct bitbuffer;
int gpx_decode_run_length(struct bitbuffer *b);
void gpx_decode_offset(struct bitbuffer *b, int *x_off, int *y_off);

#endif // AI5_CG_H
