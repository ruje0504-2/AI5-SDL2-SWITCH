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

#include "nulib.h"
#include "nulib/buffer.h"
#include "nulib/port.h"
#include "nulib/vector.h"

#include "ai5/a6.h"

a6_array a6_parse(uint8_t *data, size_t data_size)
{
	struct buffer in;
	buffer_init(&in, data, data_size);

	a6_array a = vector_initializer;
	while (true) {
		if (buffer_end(&in)) {
			WARNING("Unterminated A6 file");
			return a;
		}
		struct a6_entry e;
		e.id = buffer_read_u16(&in);
		if (e.id == 0xffff)
			break;
		e.x_left = buffer_read_u16(&in);
		e.y_top = buffer_read_u16(&in);
		e.x_right = buffer_read_u16(&in);
		e.y_bot = buffer_read_u16(&in);
		vector_push(struct a6_entry, a, e);
	}

	return a;
}

void a6_free(a6_array a)
{
	vector_destroy(a);
}

void a6_print(struct port *out, a6_array a)
{
	struct a6_entry *e;
	vector_foreach_p(e, a) {
		port_printf(out, "%u:\t[%u,%u] - [%u,%u]\n", e->id, e->x_left, e->y_top,
				e->x_right, e->y_bot);
	}
}
