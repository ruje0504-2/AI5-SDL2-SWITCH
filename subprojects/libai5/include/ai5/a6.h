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

#ifndef AI5_A6_H
#define AI5_A6_H

#include "nulib/vector.h"

struct port;

struct a6_entry {
	unsigned id;
	unsigned x_left;
	unsigned y_top;
	unsigned x_right;
	unsigned y_bot;
};

typedef vector_t(struct a6_entry) a6_array;

a6_array a6_parse(uint8_t *data, size_t data_size);
void a6_free(a6_array a);
void a6_print(struct port *out, a6_array a);

#endif // AI5_A6_H
