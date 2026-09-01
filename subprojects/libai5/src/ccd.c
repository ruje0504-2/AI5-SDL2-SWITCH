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

#include <string.h>

#include "nulib.h"
#include "nulib/buffer.h"
#include "nulib/port.h"
#include "nulib/little_endian.h"

#include "ai5/ccd.h"

static bool sprite_list_end(struct buffer *in)
{
	size_t rem = buffer_remaining(in);
	if (rem >= 14)
		return false;
	if (rem > 1)
		WARNING("Junk at end of sprites section?");
	if (rem == 1 && buffer_peek_u8(in) != 0xff)
		WARNING("Unexpected value at end of sprite list: 0x%02x",
				buffer_peek_u8(in));
	return true;
}

struct ccd_sprite ccd_load_sprite(unsigned no, uint8_t *data)
{
	uint16_t sp_off = le_get16(data, 0) + 14 * no;
	struct ccd_sprite sp;
	sp.state = data[sp_off];
	if (sp.state == 0xff)
		return sp;
	sp.no = data[sp_off + 1];
	sp.w = data[sp_off + 2];
	sp.h = data[sp_off + 3];
	sp.x = le_get16(data, sp_off + 4);
	sp.y = le_get16(data, sp_off + 6);
	sp.frame = data[sp_off + 8];
	sp.script_index = data[sp_off + 9];
	sp.script_cmd = data[sp_off + 10];
	sp.script_repetitions = data[sp_off + 11];
	sp.script_ptr = le_get16(data, sp_off + 12);
	return sp;
}

static void parse_sprites(struct ccd *ccd, struct buffer *in)
{
	while (!sprite_list_end(in)) {
		struct ccd_sprite sp;
		sp.state = buffer_read_u8(in);
		sp.no = buffer_read_u8(in);
		sp.w = buffer_read_u8(in);
		sp.h = buffer_read_u8(in);
		sp.x = buffer_read_u16(in);
		sp.y = buffer_read_u16(in);
		sp.frame = buffer_read_u8(in);
		sp.script_index = buffer_read_u8(in);
		sp.script_cmd = buffer_read_u8(in);
		sp.script_repetitions = buffer_read_u8(in);
		sp.script_ptr = buffer_read_u16(in);
		vector_push(struct ccd_sprite, ccd->sprites, sp);
	}
}

static void parse_scripts(struct ccd *ccd, struct buffer *in)
{
	uint16_t script_start = buffer_peek_u16(in);

	while (!buffer_end(in) && in->index < script_start) {
		uint8_t *script;
		uint16_t script_off = buffer_read_u16(in);
		script_start = min(script_start, script_off);

		if (script_off >= in->size) {
			WARNING("Script offset beyond end of script section: 0x%04x", script_off);
			goto invalid_script;
		}

		uint8_t *start = in->buf + script_off;
		uint8_t *end = memchr(start, 0, in->size - script_off);
		if (!end) {
			WARNING("Script extends beyond end of script section");
			goto invalid_script;
		}

		script = xmalloc((end - start) + 1);
		memcpy(script, start, (end - start) + 1);
		vector_push(uint8_t*, ccd->scripts, script);
		continue;
invalid_script:
		script = xmalloc(1);
		*script = 0;
		vector_push(uint8_t*, ccd->scripts, script);
	}
}

static bool spawn_list_end(struct buffer *in)
{
	size_t rem = buffer_remaining(in);
	if (rem >= 4)
		return false;
	if (rem > 0)
		WARNING("Junk at end of spawns section?");
	return true;
}

struct ccd_spawn ccd_load_spawn(unsigned no, uint8_t *data)
{
	uint16_t off = le_get16(data, 4) + 4 * no;
	struct ccd_spawn spawn;
	spawn.screen_x = data[off];
	spawn.screen_y = data[off + 1];
	spawn.sprite_x = data[off + 2];
	spawn.sprite_y = data[off + 3];
	return spawn;
}

static void parse_spawns(struct ccd *ccd, struct buffer *in)
{
	while (!spawn_list_end(in)) {
		struct ccd_spawn spawn;
		spawn.screen_x = buffer_read_u8(in);
		spawn.screen_y = buffer_read_u8(in);
		spawn.sprite_x = buffer_read_u8(in);
		spawn.sprite_y = buffer_read_u8(in);
		vector_push(struct ccd_spawn, ccd->spawns, spawn);
	}
}

struct ccd *ccd_parse(uint8_t *data, size_t size)
{
	struct buffer in;
	buffer_init(&in, data, size);

	uint16_t sprite_offset = buffer_read_u16(&in);
	uint16_t script_offset = buffer_read_u16(&in);
	uint16_t spawn_offset = buffer_read_u16(&in);
	uint16_t tiles_offset = buffer_read_u16(&in);

	if (sprite_offset != 8)
		WARNING("Junk before sprite list? (offset=0x%04x", sprite_offset);

	if (script_offset >= size || sprite_offset >= size || spawn_offset >= size
			|| tiles_offset >= size) {
		WARNING("CCD section is beyond EOF "
				"(offsets=0x%04x;0x%04x;0x%04x;0x%04x)",
				sprite_offset, script_offset, spawn_offset, tiles_offset);
	}
	if (script_offset <= sprite_offset || spawn_offset <= script_offset
			|| tiles_offset <= spawn_offset) {
		WARNING("CCD sections in unexpected order "
				"(offsets=0x%04x;0x%04x;0x%04x;0x%04x)",
				sprite_offset, script_offset, spawn_offset, tiles_offset);
		return NULL;
	}

	struct ccd *ccd = xcalloc(1, sizeof(struct ccd));

	buffer_seek(&in, sprite_offset);
	in.size = script_offset;
	parse_sprites(ccd, &in);

	buffer_seek(&in, script_offset);
	in.size = spawn_offset;
	parse_scripts(ccd, &in);

	buffer_seek(&in, spawn_offset);
	in.size = tiles_offset;
	parse_spawns(ccd, &in);

	return ccd;
}

void ccd_free(struct ccd *ccd)
{
	uint8_t *p;
	vector_foreach(p, ccd->scripts) {
		free(p);
	}
	vector_destroy(ccd->sprites);
	vector_destroy(ccd->scripts);
	vector_destroy(ccd->spawns);
	free(ccd);
}

void ccd_print(struct port *out, struct ccd *ccd)
{
	unsigned i;
	struct ccd_sprite *sprite;
	uint8_t *script;
	struct ccd_spawn *spawn;

	i = 0;
	port_puts(out, "sprites = {\n");
	vector_foreach_p(sprite, ccd->sprites) {
		port_printf(out, "\t[%u] = {\n", i++);
		port_printf(out, "\t\t.state = 0x%02x,\n", sprite->state);
		port_printf(out, "\t\t.no = %u,\n", sprite->no);
		port_printf(out, "\t\t.w = %u,\n", sprite->w);
		port_printf(out, "\t\t.h = %u,\n", sprite->h);
		port_printf(out, "\t\t.x = %u,\n", sprite->x);
		port_printf(out, "\t\t.y = %u,\n", sprite->y);
		port_printf(out, "\t\t.frame = %u,\n", sprite->frame);
		port_printf(out, "\t\t.script_index = %u,\n", sprite->script_index);
		port_printf(out, "\t\t.script_cmd = %u,\n", sprite->script_cmd);
		port_printf(out, "\t\t.script_repetitions = %u,\n", sprite->script_repetitions);
		port_printf(out, "\t\t.script_ptr = 0x%04x,\n", sprite->script_ptr);
		port_puts(out, "\t},\n");
	}
	port_puts(out, "};\n\n");

	i = 0;
	port_puts(out, "scripts = {\n");
	vector_foreach(script, ccd->scripts) {
		port_printf(out, "\t[%u] = \"", i++);
		for (unsigned j = 0; script[j]; j++) {
			port_printf(out, "%s%x:%u", j ? " " : "", script[j] >> 4, script[j] & 0xf);
		}
		port_puts(out, "\",\n");
	}
	port_puts(out, "};\n\n");

	i = 0;
	port_puts(out, "spawns = {\n");
	vector_foreach_p(spawn, ccd->spawns) {
		port_printf(out, "\t[%2u] = { ", i++);
		port_printf(out, ".screen = { %2u, %2u }, ", spawn->screen_x, spawn->screen_y);
		port_printf(out, ".sprite = { %2u, %2u } },\n", spawn->sprite_x, spawn->sprite_y);
	}
	port_puts(out, "};\n");
}
