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

#ifndef AI5_CCD_H
#define AI5_CCD_H

#include <stdint.h>
#include "nulib/vector.h"

struct port;

struct ccd_sprite {
	uint8_t state;
	uint8_t no;
	uint8_t w;
	uint8_t h;
	uint16_t x;
	uint16_t y;
	uint8_t frame; // internal use
	uint8_t script_index;
	// XXX: These are for internal use by the VM. It modifies these fields
	//      in-place, even though the game doesn't appear to access them
	//      ever (Doukyuusei and All Stars at least).
	uint8_t script_cmd;
	uint8_t script_repetitions;
	uint16_t script_ptr;
};

struct ccd_spawn {
	uint8_t screen_x;
	uint8_t screen_y;
	uint8_t sprite_x;
	uint8_t sprite_y;
};

struct ccd {
	vector_t(struct ccd_sprite) sprites;
	vector_t(uint8_t*) scripts;
	vector_t(struct ccd_spawn) spawns;
};

// runtime API (unsafe)
struct ccd_sprite ccd_load_sprite(unsigned no, uint8_t *data);
struct ccd_spawn ccd_load_spawn(unsigned no, uint8_t *data);

struct ccd *ccd_parse(uint8_t *data, size_t size);
void ccd_free(struct ccd *ccd);

void ccd_print(struct port *out, struct ccd *ccd);

#endif // AI5_CCD_H
