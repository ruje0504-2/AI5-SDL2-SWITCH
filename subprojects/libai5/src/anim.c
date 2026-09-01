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
#include "ai5/game.h"
#include "ai5/anim.h"

static unsigned anim_a_src = 1;
unsigned anim_draw_call_size = 33;
enum anim_type anim_type = ANIM_S4;

void anim_set_game(enum ai5_game_id game)
{
	switch (game) {
	case GAME_ISAKU:
	case GAME_DOUKYUUSEI:
	case GAME_AI_SHIMAI:
	case GAME_ALLSTARS:
	case GAME_BEYOND:
		anim_draw_call_size = 17;
		anim_type = ANIM_A;
		break;
	case GAME_DOUKYUUSEI2:
	case GAME_KAKYUUSEI:
	case GAME_SHUUSAKU:
		anim_draw_call_size = 17;
		anim_type = ANIM_A8;
		break;
	case GAME_SHANGRLIA:
		anim_draw_call_size = 10;
		anim_type = ANIM_S4;
		break;
	default:
		anim_draw_call_size = 33;
		anim_type = ANIM_S4;
		break;
	}
	if (game == GAME_DOUKYUUSEI) {
		anim_a_src = 9;
	}
}

static void parse_color(struct buffer *in, struct anim_color *out)
{
	uint8_t b1 = buffer_read_u8(in);
	uint8_t b2 = buffer_read_u8(in);
	out->r = (b2 & 0xf0) | (b2 >> 4);
	out->g = (b2 & 0x0f) | (b2 << 4);
	out->b = (b1 & 0x0f) | (b1 << 4);
}

static enum anim_draw_opcode parse_s4_opcode(uint8_t op)
{
	switch ((enum anim_s4_draw_opcode)(op & 0xf0)) {
	case ANIM_S4_DRAW_OP_COPY: return ANIM_DRAW_OP_COPY;
	case ANIM_S4_DRAW_OP_COPY_MASKED: return ANIM_DRAW_OP_COPY_MASKED;
	case ANIM_S4_DRAW_OP_SWAP: return ANIM_DRAW_OP_SWAP;
	case ANIM_S4_DRAW_OP_SET_COLOR: return ANIM_DRAW_OP_SET_COLOR;
	case ANIM_S4_DRAW_OP_COMPOSE: return ANIM_DRAW_OP_COMPOSE;
	case ANIM_S4_DRAW_OP_FILL: return ANIM_DRAW_OP_FILL;
	case ANIM_S4_DRAW_OP_SET_PALETTE: return ANIM_DRAW_OP_SET_PALETTE;
	}
	return -1;
}

static bool parse_s4_draw_call(struct buffer *in, struct anim_draw_call *out, unsigned src_i)
{
	size_t start = in->index;

	uint8_t op = buffer_read_u8(in);
	switch ((out->op = parse_s4_opcode(op))) {
	case ANIM_DRAW_OP_FILL:
		out->fill.dst.i = ((op >> 1) & 1) ? src_i : 0;
		out->fill.dst.x = buffer_read_u8(in) * 8;
		out->fill.dst.y = buffer_read_u16(in);
		out->fill.dim.w = (buffer_read_u8(in) + 1) * 8 - out->fill.dst.x;
		out->fill.dim.h = (buffer_read_u16(in) + 1) - out->fill.dst.y;
		break;
	case ANIM_DRAW_OP_COPY:
	case ANIM_DRAW_OP_COPY_MASKED:
	case ANIM_DRAW_OP_SWAP:
		out->copy.src.i = ((op >> 1) & 1) ? src_i : 0;
		out->copy.dst.i = (op & 1) ? src_i : 0;
		out->copy.src.x = buffer_read_u8(in) * 8;
		out->copy.src.y = buffer_read_u16(in);
		out->copy.dim.w = (buffer_read_u8(in) + 1) * 8 - out->copy.src.x;
		out->copy.dim.h = (buffer_read_u16(in) + 1) - out->copy.src.y;
		out->copy.dst.x = buffer_read_u8(in) * 8;
		out->copy.dst.y = buffer_read_u16(in);
		break;
	case ANIM_DRAW_OP_COMPOSE:
		out->compose.bg.i = (op & 1) ? src_i : 0;
		out->compose.fg.i = ((op >> 1) & 1) ? src_i : 0;
		out->compose.dst.i = ((op >> 2) & 1) ? src_i : 0;
		out->compose.fg.x = buffer_read_u8(in) * 8;
		out->compose.fg.y = buffer_read_u16(in);
		out->compose.dim.w = (buffer_read_u8(in) + 1) * 8 - out->compose.fg.x;
		out->compose.dim.h = (buffer_read_u16(in) + 1) - out->compose.fg.y;
		out->compose.bg.x = buffer_read_u8(in) * 8;
		out->compose.bg.y = buffer_read_u16(in);
		out->compose.dst.x = buffer_read_u8(in) * 8;
		out->compose.dst.y = buffer_read_u16(in);
		break;
	case ANIM_DRAW_OP_SET_COLOR:
		// XXX: index is also blue value...
		out->set_color.i = buffer_peek_u8(in);
		parse_color(in, &out->set_color.color);
		break;
	case ANIM_DRAW_OP_SET_PALETTE:
		for (int i = 0; i < 16; i++) {
			parse_color(in, &out->set_palette.colors[i]);
		}
		break;
	default:
		WARNING("Invalid draw call opcode: %02x", op);
		return false;
	}

	buffer_seek(in, start + anim_draw_call_size);
	return true;
}

static enum anim_draw_opcode parse_a8_draw_opcode(uint8_t op)
{
	switch (op & 0xf0) {
	case 0x10: return ANIM_DRAW_OP_COPY;
	case 0x20: return ANIM_DRAW_OP_COPY_MASKED;
	case 0x30: return ANIM_DRAW_OP_SWAP;
	// XXX: 0x40 is COMPOSE in Shuusaku, unused in Kakyuusei
	case 0x40: return ANIM_DRAW_OP_COMPOSE;
	// XXX: 0x50 is COPY_MASKED (redundant) in Kakyuusei, unused in Shuusaku
	case 0x50: return ANIM_DRAW_OP_COPY_MASKED2;
	case 0x60: return ANIM_DRAW_OP_FILL;
	}
	return -1;
}

static void parse_copy_args(struct buffer *in, struct anim_copy_args *out)
{
	out->src.x = buffer_read_u16(in);
	out->src.y = buffer_read_u16(in);
	out->dim.w = buffer_read_u16(in);
	out->dim.h = buffer_read_u16(in);
	out->dst.x = buffer_read_u16(in);
	out->dst.y = buffer_read_u16(in);
}

static void parse_compose_args(struct buffer *in, struct anim_compose_args *out)
{
	out->fg.x = buffer_read_u16(in);
	out->fg.y = buffer_read_u16(in);
	out->dim.w = buffer_read_u16(in);
	out->dim.h = buffer_read_u16(in);
	out->bg.x = buffer_read_u16(in);
	out->bg.y = buffer_read_u16(in);
	out->dst.x = out->bg.x;
	out->dst.y = out->bg.y;
}

static bool parse_a8_draw_call(struct buffer *in, struct anim_draw_call *out, unsigned src_i)
{
	size_t start = in->index;
	uint8_t op = buffer_read_u8(in);
	switch ((out->op = parse_a8_draw_opcode(op))) {
	case ANIM_DRAW_OP_COPY:
	case ANIM_DRAW_OP_COPY_MASKED:
	case ANIM_DRAW_OP_SWAP:
		out->copy.dst.i = (op & 1) ? src_i : 0;
		out->copy.src.i = ((op >> 1) & 1) ? src_i : 0;
		parse_copy_args(in, &out->copy);
		break;
	case ANIM_DRAW_OP_FILL:
		out->fill.dst.i = ((op >> 1) & 1) ? src_i : 0;
		out->fill.dst.x = buffer_read_u16(in);
		out->fill.dst.y = buffer_read_u16(in);
		out->fill.dim.w = buffer_read_u16(in);
		out->fill.dim.h = buffer_read_u16(in);
		break;
	case ANIM_DRAW_OP_COMPOSE:
		out->compose.dst.i = (op & 1) ? src_i : 0;
		out->compose.fg.i = ((op >> 1) & 1) ? src_i : 0;
		out->compose.bg.i = 3; // hardcoded in Shuusaku
		parse_compose_args(in, &out->compose);
		break;
	case ANIM_DRAW_OP_COPY_MASKED2:
		out->compose.dst.i = (op & 1) ? src_i : 0;
		out->compose.fg.i = ((op >> 1) & 1) ? src_i : 0;
		out->compose.bg.i = ((op >> 2) & 1) ? src_i : 0;
		parse_compose_args(in, &out->compose);
		break;
	default:
		WARNING("Invalid draw call opcode: %02x", op);
		return false;
	}
	buffer_seek(in, start + anim_draw_call_size);
	return true;
}

static enum anim_draw_opcode parse_a_draw_opcode(uint8_t op)
{
	switch ((enum anim_a_draw_opcode)(op & 0xf0)) {
	case ANIM_A_DRAW_OP_COPY:
		return ANIM_DRAW_OP_COPY;
	case ANIM_A_DRAW_OP_COPY_MASKED:
		return ANIM_DRAW_OP_COPY_MASKED;
	case ANIM_A_DRAW_OP_SWAP:
		if (ai5_target_game == GAME_BEYOND)
			return ANIM_DRAW_OP_COMPOSE_WITH_OFFSET;
		return ANIM_DRAW_OP_SWAP;
	case ANIM_A_DRAW_OP_COMPOSE:
		return ANIM_DRAW_OP_COMPOSE;
	default:
		// BE-YOND
		switch (op) {
		case 0x60: return ANIM_DRAW_OP_0x60_COPY_MASKED;
		case 0x61: return ANIM_DRAW_OP_0x61_COMPOSE;
		case 0x62: return ANIM_DRAW_OP_0x62;
		case 0x63: return ANIM_DRAW_OP_0x63_COPY_MASKED_WITH_XOFFSET;
		case 0x64: return ANIM_DRAW_OP_0x64_COMPOSE_MASKED;
		case 0x65: return ANIM_DRAW_OP_0x65_COMPOSE;
		case 0x66: return ANIM_DRAW_OP_0x66;
		}
		break;
	}
	return -1;
}

static bool parse_a_draw_call(struct buffer *in, struct anim_draw_call *out, unsigned src_i)
{
	size_t start = in->index;

	uint16_t op = buffer_read_u8(in);
	switch ((out->op = parse_a_draw_opcode(op))) {
	case ANIM_DRAW_OP_COPY:
	case ANIM_DRAW_OP_COPY_MASKED:
	case ANIM_DRAW_OP_SWAP:
	case ANIM_DRAW_OP_0x62:
	case ANIM_DRAW_OP_0x66:
		out->copy.src.i = src_i;
		out->copy.dst.i = 0;
		parse_copy_args(in, &out->copy);
		break;
	case ANIM_DRAW_OP_COMPOSE:
	case ANIM_DRAW_OP_COMPOSE_WITH_OFFSET:
		out->compose.bg.i = 2;
		out->compose.fg.i = src_i;
		out->compose.dst.i = 0;
		parse_compose_args(in, &out->compose);
		break;
	case ANIM_DRAW_OP_0x60_COPY_MASKED:
		out->copy.src.i = 4;
		out->copy.dst.i = 11;
		parse_copy_args(in, &out->copy);
		out->copy.dst.y -= 8;
		break;
	case ANIM_DRAW_OP_0x61_COMPOSE:
		out->compose.bg.i = 10;
		out->compose.fg.i = 10;
		out->compose.dst.i = 4;
		parse_compose_args(in, &out->compose);
		out->compose.bg.x = 200;
		out->compose.bg.y = 320;
		break;
	case ANIM_DRAW_OP_0x63_COPY_MASKED_WITH_XOFFSET:
		out->copy.src.i = 4;
		out->copy.dst.i = 11;
		parse_copy_args(in, &out->copy);
		out->copy.dst.y += 20;
		break;
	case ANIM_DRAW_OP_0x64_COMPOSE_MASKED:
		out->compose.bg.i = 8;
		out->compose.fg.i = 1;
		out->compose.dst.i = 0;
		parse_compose_args(in, &out->compose);
		break;
	case ANIM_DRAW_OP_0x65_COMPOSE:
		out->compose.bg.i = 2;
		out->compose.fg.i = 3;
		out->compose.dst.i = 0;
		parse_compose_args(in, &out->compose);
		break;
	default:
		WARNING("Invalid draw call opcode: %02x", op);
		return false;
	}
	buffer_seek(in, start + anim_draw_call_size);
	return true;
}

bool anim_parse_draw_call(uint8_t *data, struct anim_draw_call *out, unsigned src_i)
{
	struct buffer b;
	buffer_init(&b, data, anim_draw_call_size);
	switch (anim_type) {
	case ANIM_S4: return parse_s4_draw_call(&b, out, src_i);
	case ANIM_A8: return parse_a8_draw_call(&b, out, src_i);
	case ANIM_A:  return parse_a_draw_call(&b, out, src_i);
	}
	ERROR("Invalid anim_type");
}

static bool anim_parse_s4_instruction(struct buffer *in, struct anim *anim,
		struct anim_instruction *out)
{
	uint8_t op = buffer_read_u8(in);
	switch (op) {
	case ANIM_OP_NOOP:
	case ANIM_OP_CHECK_STOP:
	case ANIM_OP_RESET:
	case ANIM_OP_HALT:
	case ANIM_OP_LOOP_END:
	case ANIM_OP_LOOP2_END:
		out->op = op;
		break;
	case ANIM_OP_STALL:
	case ANIM_OP_LOOP_START:
	case ANIM_OP_LOOP2_START:
		out->op = op;
		out->arg = buffer_read_u8(in);
		break;
	case ANIM_OP_LOAD_PALETTE:
		out->op = op;
		out->arg = buffer_read_u16(in);
		break;
	default:
		if (op < 20 || op - 20 >= vector_length(anim->draw_calls)) {
			WARNING("at %x", (unsigned)in->index-1);
			WARNING("Invalid draw call index: %d", (int)op - 20);
			out->op = ANIM_OP_NOOP;
			break;
		}
		out->op = ANIM_OP_DRAW;
		out->arg = op - 20;
		break;
	}
	return true;
}

static bool anim_parse_a_instruction(struct buffer *in, struct anim *anim,
		struct anim_instruction *out)
{
	uint8_t op = buffer_read_u16(in);
	switch (op) {
	case ANIM_OP_NOOP:
	case ANIM_OP_CHECK_STOP:
	case ANIM_OP_RESET:
	case ANIM_OP_HALT:
	case ANIM_OP_LOOP_END:
	case ANIM_OP_LOOP2_END:
		out->op = op;
		break;
	case ANIM_OP_STALL:
	case ANIM_OP_LOOP_START:
	case ANIM_OP_LOOP2_START:
		out->op = op;
		out->arg = buffer_read_u16(in);
		break;
	case ANIM_OP_LOAD_PALETTE:
		out->op = op;
		out->arg = buffer_read_u16(in);
		break;
	default:
		if (op < 20 || op - 20 >= vector_length(anim->draw_calls)) {
			WARNING("at %x", (unsigned)in->index-1);
			WARNING("Invalid draw call index: %d", (int)op - 20);
			out->op = ANIM_OP_NOOP;
			break;
		}
		out->op = ANIM_OP_DRAW;
		out->arg = op - 20;
		break;
	}
	return true;
}

void anim_free(struct anim *anim)
{
	for (int i = 0; i < ANIM_MAX_STREAMS; i++) {
		vector_destroy(anim->streams[i]);
	}
	vector_destroy(anim->draw_calls);
	vector_destroy(anim->palettes);
	free(anim);
}

struct anim *anim_s4_parse(struct buffer *in)
{
	uint16_t stream_ptr[ANIM_MAX_STREAMS];
	uint16_t stream_end[ANIM_MAX_STREAMS];

	uint8_t nr_streams = buffer_read_u8(in);
	if (nr_streams > ANIM_MAX_STREAMS) {
		WARNING("Too many streams in animation file");
		return NULL;
	}
	if (!nr_streams) {
		WARNING("No streams in animation file");
		return NULL;
	}

	for (unsigned i = 0; i < nr_streams; i++) {
		stream_ptr[i] = buffer_read_u16(in);
	}

	// determine start of stream bytecode section
	unsigned stream_start = stream_ptr[0];
	for (unsigned i = 1; i < nr_streams; i++) {
		if (stream_ptr[i] < stream_start)
			stream_start = stream_ptr[i];
	}

	struct anim *anim = xcalloc(1, sizeof(struct anim));

	// read draw calls
	while (!buffer_end(in) && in->index < stream_start) {
		struct anim_draw_call call;
		if (!parse_s4_draw_call(in, &call, 1))
			goto err;
		vector_push(struct anim_draw_call, anim->draw_calls, call);
	}

	// read bytecode for each stream
	for (unsigned i = 0; i < nr_streams; i++) {
		buffer_seek(in, stream_ptr[i]);
		// XXX: Many S4 files are broken in that they contain stream pointers which
		//      point into previous streams. We fix these broken files by treating
		//      such streams as if they are empty.
		//      (The cause of the breakage seems to be the LOOP2_START
		//      instruction: the start of stream position is off by the number of
		//      occurrences of this instruction.)
		for (unsigned j = 0; j < i; j++) {
			if (stream_ptr[j] < stream_ptr[i] && stream_end[j] > stream_ptr[i]) {
				sys_warning("Streams %u and %u overlap (possibly broken file)\n", i, j);
				goto skip;
			}
		}
		while (!buffer_end(in) && buffer_peek_u8(in) != 0xff) {
			struct anim_instruction instr;
			if (!anim_parse_s4_instruction(in, anim, &instr))
				goto err;
			vector_push(struct anim_instruction, anim->streams[i], instr);
		}
skip:
		stream_end[i] = in->index;
	}

	return anim;
err:
	anim_free(anim);
	return NULL;

}

static struct anim *anim_a8_parse(struct buffer *in)
{
	uint32_t stream_ptr[10];
	uint8_t nr_draw_calls = buffer_read_u8(in);
	for (int i = 0; i < 10; i++) {
		stream_ptr[i] = buffer_read_u16(in);
	}

	// determine start of stream bytecode section
	unsigned stream_start = stream_ptr[0];
	for (int i = 0; i < 10; i++) {
		if (stream_ptr[i] < stream_start)
			stream_start = stream_ptr[i];
	}

	struct anim *anim = xcalloc(1, sizeof(struct anim));

	// read draw calls
	while (!buffer_end(in) && in->index < stream_start) {
		struct anim_draw_call call;
		if (ai5_target_game == GAME_SHUUSAKU || ai5_target_game == GAME_DOUKYUUSEI2) {
			// XXX: Palettes are counted as draw calls, but they have a different
			//      length. There is no way to detect whether we are looking at a
			//      draw call or a palette without reading the bytecode first.
			//      However, in practice palettes always start with 0xff or 0x00,
			//      which is the hack we use here.
			uint8_t c = buffer_peek_u8(in);
			if (c == 0 || c == 0xff) {
				// palette
				struct anim_palette *pal = vector_pushp(struct anim_palette,
						anim->palettes);
				pal->addr = in->index;
				for (int i = 0; i < 256; i++) {
					pal->colors[i].r = buffer_read_u8(in);
					pal->colors[i].g = buffer_read_u8(in);
					pal->colors[i].b = buffer_read_u8(in);
				}
				nr_draw_calls--;
				continue;
			}
		}
		if (!parse_a8_draw_call(in, &call, 1))
			goto err;
		vector_push(struct anim_draw_call, anim->draw_calls, call);
	}
	if (vector_length(anim->draw_calls) != nr_draw_calls)
		WARNING("Declared draw call count doesn't match number of parsed calls");

	// read bytecode for each stream
	for (int i = 0; i < 10; i++) {
		buffer_seek(in, stream_ptr[i]);
		while (!buffer_end(in) && buffer_peek_u8(in) != 0xff) {
			struct anim_instruction instr;
			if (!anim_parse_s4_instruction(in, anim, &instr))
				goto err;
			vector_push(struct anim_instruction, anim->streams[i], instr);
		}
	}

	return anim;
err:
	anim_free(anim);
	return NULL;
}

static struct anim *anim_a_parse(struct buffer *in)
{
	uint32_t stream_ptr[ANIM_MAX_STREAMS];

	uint16_t nr_draw_calls = buffer_read_u16(in);

	for (unsigned i = 0; i < ANIM_MAX_STREAMS; i++) {
		stream_ptr[i] = buffer_read_u32(in);
	}

	// determine start of stream bytecode section
	unsigned stream_start = stream_ptr[0];
	for (unsigned i = 0; i < ANIM_MAX_STREAMS; i++) {
		if (stream_ptr[i] < stream_start)
			stream_start = stream_ptr[i];
	}

	struct anim *anim = xcalloc(1, sizeof(struct anim));

	// read draw calls
	while (!buffer_end(in) && in->index < stream_start) {
		struct anim_draw_call call;
		if (!parse_a_draw_call(in, &call, 1))
			goto err;
		vector_push(struct anim_draw_call, anim->draw_calls, call);
	}
	if (vector_length(anim->draw_calls) != nr_draw_calls)
		WARNING("Declared draw call count doesn't match number of parsed calls");

	// read bytecode for each stream
	for (unsigned i = 0; i < ANIM_MAX_STREAMS; i++) {
		buffer_seek(in, stream_ptr[i]);
		while (!buffer_end(in) && buffer_peek_u16(in) != 0xffff) {
			struct anim_instruction instr;
			if (!anim_parse_a_instruction(in, anim, &instr))
				goto err;
			vector_push(struct anim_instruction, anim->streams[i], instr);
		}
	}

	return anim;
err:
	anim_free(anim);
	return NULL;
}

struct anim *anim_parse(uint8_t *data, size_t data_size)
{
	struct buffer in;
	buffer_init(&in, data, data_size);
	switch (anim_type) {
	case ANIM_S4: return anim_s4_parse(&in);
	case ANIM_A8: return anim_a8_parse(&in);
	case ANIM_A:  return anim_a_parse(&in);
	}
	ERROR("Invalid anim_type");
}

static void print_draw_args(struct port *out, struct anim_draw_call *call)
{
	switch (call->op) {
	case ANIM_DRAW_OP_COPY:
	case ANIM_DRAW_OP_COPY_MASKED:
	case ANIM_DRAW_OP_SWAP:
	case ANIM_DRAW_OP_0x60_COPY_MASKED:
	case ANIM_DRAW_OP_0x62:
	case ANIM_DRAW_OP_0x63_COPY_MASKED_WITH_XOFFSET:
	case ANIM_DRAW_OP_0x66:
		port_printf(out, " %u(%u, %u) -> %u(%u, %u) @ (%u, %u);\n",
				call->copy.src.i, call->copy.src.x, call->copy.src.y,
				call->copy.dst.i, call->copy.dst.x, call->copy.dst.y,
				call->copy.dim.w, call->copy.dim.h);
		break;
	case ANIM_DRAW_OP_COMPOSE:
	case ANIM_DRAW_OP_COMPOSE_WITH_OFFSET:
	case ANIM_DRAW_OP_COPY_MASKED2:
	case ANIM_DRAW_OP_0x61_COMPOSE:
	case ANIM_DRAW_OP_0x64_COMPOSE_MASKED:
	case ANIM_DRAW_OP_0x65_COMPOSE:
		port_printf(out, " %u(%u, %u) + %u(%u, %u) -> %u(%u, %u) @ (%u, %u);\n",
				call->compose.bg.i, call->compose.bg.x, call->compose.bg.y,
				call->compose.fg.i, call->compose.fg.x, call->compose.fg.y,
				call->compose.dst.i, call->compose.dst.x, call->compose.dst.y,
				call->compose.dim.w, call->compose.dim.h);
		break;
	case ANIM_DRAW_OP_FILL:
		port_printf(out, " %u(%u, %u) @ (%u, %u);\n", call->fill.dst,
				call->fill.dst.x, call->fill.dst.y, call->fill.dim.w,
				call->fill.dim.h);
		break;
	case ANIM_DRAW_OP_SET_COLOR:
		port_printf(out, " %u -> (%u,%u,%u);\n", call->set_color.i,
				call->set_color.color.r & 0xf, call->set_color.color.g & 0xf,
				call->set_color.color.b & 0xf);
		break;
	case ANIM_DRAW_OP_SET_PALETTE:
		for (int i = 0; i < 16; i++) {
			port_printf(out, " (%u,%u,%u)", call->set_palette.colors[i].r & 0xf,
					call->set_palette.colors[i].g & 0xf,
					call->set_palette.colors[i].b & 0xf);
		}
		port_puts(out, ";\n");
		break;
	}
}

static bool anim_print_draw_call(struct port *out, struct anim_draw_call *call, int indent)
{
	for (int i = 0; i < indent; i++) {
		port_puts(out, "\t");
	}
	switch (call->op) {
	case ANIM_DRAW_OP_COPY:
		port_puts(out, "COPY");
		break;
	case ANIM_DRAW_OP_COPY_MASKED:
		port_puts(out, "COPY_MASKED");
		break;
	case ANIM_DRAW_OP_SWAP:
		port_puts(out, "SWAP");
		break;
	case ANIM_DRAW_OP_SET_COLOR:
		port_puts(out, "SET_COLOR");
		break;
	case ANIM_DRAW_OP_COMPOSE:
		port_puts(out, "COMPOSE");
		break;
	case ANIM_DRAW_OP_COMPOSE_WITH_OFFSET:
		port_puts(out, "COMPOSE_WITH_OFFSET");
		break;
	case ANIM_DRAW_OP_COPY_MASKED2:
		port_puts(out, "COPY_MASKED2");
		break;
	case ANIM_DRAW_OP_FILL:
		port_puts(out, "FILL");
		break;
	case ANIM_DRAW_OP_SET_PALETTE:
		port_puts(out, "SET_PALETTE");
		break;
	case ANIM_DRAW_OP_0x60_COPY_MASKED:
		port_puts(out, "COPY_MASKED_0x60");
		break;
	case ANIM_DRAW_OP_0x61_COMPOSE:
		port_puts(out, "COMPOSE_0x61");
		break;
	case ANIM_DRAW_OP_0x62:
		port_puts(out, "0x62");
		break;
	case ANIM_DRAW_OP_0x63_COPY_MASKED_WITH_XOFFSET:
		port_puts(out, "COPY_MASKED_WITH_XOFFSET_0x63");
		break;
	case ANIM_DRAW_OP_0x64_COMPOSE_MASKED:
		port_puts(out, "COMPOSE_MASKED_0x64");
		break;
	case ANIM_DRAW_OP_0x65_COMPOSE:
		port_puts(out, "COMPOSE_0x65");
		break;
	case ANIM_DRAW_OP_0x66:
		port_puts(out, "0x66");
		break;
	default:
		sys_warning("Invalid draw call: %d", call->op);
		return false;
	}
	print_draw_args(out, call);
	return true;
}

static int get_palette_no(struct anim *anim, uint16_t addr)
{
	int no = 0;
	struct anim_palette *p;
	vector_foreach_p(p, anim->palettes) {
		if (p->addr == addr)
			return no;
		no++;
	}
	// XXX: OP3.A from Shuusaku is broken and triggers this
	//      (2 calls with palette address of 0)
	WARNING("Couldn't locate palette @ 0x%x", addr);
	return 0;
}

static void anim_print_instruction(struct port *out, struct anim *anim,
		struct anim_instruction *instr, int indent)
{
	for (int i = 0; i < indent; i++) {
		port_puts(out, "\t");
	}
	switch (instr->op) {
	case ANIM_OP_NOOP:
		port_puts(out, "NOOP;\n");
		break;
	case ANIM_OP_CHECK_STOP:
		port_puts(out, "CHECK_STOP;\n");
		break;
	case ANIM_OP_STALL:
		port_printf(out, "STALL %u;\n", instr->arg);
		break;
	case ANIM_OP_RESET:
		port_puts(out, "RESET;\n");
		break;
	case ANIM_OP_HALT:
		port_puts(out, "HALT;\n");
		break;
	case ANIM_OP_LOOP_START:
		port_printf(out, "LOOP_START %u;\n", instr->arg);
		break;
	case ANIM_OP_LOOP_END:
		port_puts(out, "LOOP_END;\n");
		break;
	case ANIM_OP_LOOP2_START:
		port_printf(out, "LOOP2_START %u;\n", instr->arg);
		break;
	case ANIM_OP_LOOP2_END:
		port_puts(out, "LOOP2_END;\n");
		break;
	case ANIM_OP_LOAD_PALETTE:
		port_printf(out, "LOAD_PALETTE %u;\n", get_palette_no(anim, instr->arg));
		break;
	default:
		ERROR("Invalid opcode: %d", instr->op);
	}
}

bool anim_print(struct port *out, struct anim *anim)
{
	int pal_no = 0;
	struct anim_palette *p;
	vector_foreach_p(p, anim->palettes) {
		port_printf(out, "PALETTE %u {", pal_no++);
		for (int i = 0; i < 256; i += 8) {
			port_puts(out, "\n\t");
			for (int j = i; j < i + 8; j++) {
				if (j > i)
					port_putc(out, ' ');
				port_printf(out, "#%02x%02x%02x", p->colors[j].r,
						p->colors[j].g, p->colors[j].b);
			}
		}
		port_puts(out, "\n};\n\n");
	}
	for (int i = 0; i < ANIM_MAX_STREAMS; i++) {
		if (vector_empty(anim->streams[i]))
			continue;
		port_printf(out, "STREAM:\n", i);
		int indent = 0;
		struct anim_instruction *p;
		vector_foreach_p(p, anim->streams[i]) {
			if (p->op == ANIM_OP_LOOP_END || p->op == ANIM_OP_LOOP2_END)
				indent--;
			if (p->op == ANIM_OP_DRAW) {
				struct anim_draw_call *call = &vector_A(anim->draw_calls, p->arg);
				if (!anim_print_draw_call(out, call, indent))
					return false;
			} else {
				anim_print_instruction(out, anim, p, indent);
			}
			if (p->op == ANIM_OP_LOOP_START || p->op == ANIM_OP_LOOP2_START)
				indent++;
		}
	}
	return true;
}
