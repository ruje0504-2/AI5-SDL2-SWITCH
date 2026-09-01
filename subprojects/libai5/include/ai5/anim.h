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

#ifndef LIBAI5_ANIM_H
#define LIBAI5_ANIM_H

#include <stdint.h>
#include "nulib/vector.h"
#include "ai5/game.h"

enum anim_type {
	ANIM_S4,
	ANIM_A8, // 8-bit .A
	ANIM_A,  // 16-bit .A (typical)
};

struct port;

extern unsigned anim_draw_call_size;
extern enum anim_type anim_type;

enum anim_opcode {
#define ANIM_OP_DRAW (-1)
	// stall 1 cycle
	ANIM_OP_NOOP = 0,
	// check if VM requested stop
	ANIM_OP_CHECK_STOP = 1,
	// stall N cycles
	ANIM_OP_STALL = 2,
	// reset IP to 0 (loop from beginning)
	ANIM_OP_RESET = 3,
	// stop
	ANIM_OP_HALT = 4,
	// loop N times
	ANIM_OP_LOOP_START = 5,
	// go to start of loop (or continue if loop finished)
	ANIM_OP_LOOP_END = 6,
	// same as ANIM_LOOP1_START (for nested loop)
	ANIM_OP_LOOP2_START = 7,
	// save as ANIM_LOOP2_END (for nested loop)
	ANIM_OP_LOOP2_END = 8,
	// load palette
	ANIM_OP_LOAD_PALETTE = 0x96,
};

enum anim_s4_draw_opcode {
	ANIM_S4_DRAW_OP_COPY = 0x10,
	ANIM_S4_DRAW_OP_COPY_MASKED = 0x20,
	ANIM_S4_DRAW_OP_SWAP = 0x30,
	ANIM_S4_DRAW_OP_SET_COLOR = 0x40,
	ANIM_S4_DRAW_OP_COMPOSE = 0x50,
	ANIM_S4_DRAW_OP_FILL = 0x60,
	ANIM_S4_DRAW_OP_SET_PALETTE = 0x80,
};

enum anim_a_draw_opcode {
	ANIM_A_DRAW_OP_COPY = 0x10,
	ANIM_A_DRAW_OP_COPY_MASKED = 0x20,
	ANIM_A_DRAW_OP_SWAP = 0x30,
	ANIM_A_DRAW_OP_COMPOSE = 0x40,
};

enum anim_draw_opcode {
	ANIM_DRAW_OP_COPY,
	ANIM_DRAW_OP_COPY_MASKED,
	ANIM_DRAW_OP_COPY_MASKED2,
	ANIM_DRAW_OP_SWAP,
	ANIM_DRAW_OP_SET_COLOR,
	ANIM_DRAW_OP_COMPOSE,
	ANIM_DRAW_OP_COMPOSE_WITH_OFFSET,
	ANIM_DRAW_OP_FILL,
	ANIM_DRAW_OP_SET_PALETTE,
	// BE-YOND
	ANIM_DRAW_OP_0x60_COPY_MASKED,
	ANIM_DRAW_OP_0x61_COMPOSE,
	ANIM_DRAW_OP_0x62,
	ANIM_DRAW_OP_0x63_COPY_MASKED_WITH_XOFFSET,
	ANIM_DRAW_OP_0x64_COMPOSE_MASKED,
	ANIM_DRAW_OP_0x65_COMPOSE,
	ANIM_DRAW_OP_0x66,
};

struct anim_target {
	uint8_t i;
	int x, y;
};

struct anim_size {
	int w, h;
};

struct anim_fill_args {
	struct anim_target dst;
	struct anim_size dim;
};

struct anim_copy_args {
	struct anim_target src;
	struct anim_target dst;
	struct anim_size dim;
};

struct anim_compose_args {
	struct anim_target fg;
	struct anim_target bg;
	struct anim_target dst;
	struct anim_size dim;
};

struct anim_color {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct anim_set_color_args {
	uint8_t i;
	struct anim_color color;
};

struct anim_set_palette_args {
	struct anim_color colors[16];
};

struct anim_draw_call {
	enum anim_draw_opcode op;
	union {
		struct anim_fill_args fill;
		struct anim_copy_args copy;
		struct anim_compose_args compose;
		struct anim_set_color_args set_color;
		struct anim_set_palette_args set_palette;
	};
};
typedef vector_t(struct anim_draw_call) anim_draw_call_list;

struct anim_palette {
	uint16_t addr;
	struct anim_color colors[256];
};

typedef vector_t(struct anim_palette) anim_palette_list;

struct anim_instruction {
	int op; // anim_opcode or -1 for draw call
	uint16_t arg;
};
typedef vector_t(struct anim_instruction) anim_stream;

#define ANIM_MAX_STREAMS 100

struct anim {
	anim_stream streams[ANIM_MAX_STREAMS];
	anim_draw_call_list draw_calls;
	anim_palette_list palettes;
};

void anim_set_game(enum ai5_game_id game);
struct anim *anim_parse(uint8_t *data, size_t data_size);
bool anim_parse_draw_call(uint8_t *data, struct anim_draw_call *out, unsigned src_i);
bool anim_print(struct port *out, struct anim *anim);
void anim_free(struct anim *anim);

#endif // LIBAI5_ANIM_H
