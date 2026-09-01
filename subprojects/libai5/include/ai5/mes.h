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

#ifndef AI5_MES_H
#define AI5_MES_H

#include <stdint.h>
#include <stdio.h>

#include "nulib.h"
#include "nulib/vector.h"

typedef char *string;

struct port;
enum ai5_game_id;

#define MES_ADDRESS_SYNTHETIC 0xFFFFFFFF

/*
 * XXX: These opcode values don't necessarily correspond with those
 *      used in any particular game. They should be considered
 *      virtual opcodes for internal use. When parsing or compiling,
 *      the correct opcode should be looked up in the appropriate
 *      table for the target game.
 */
enum mes_statement_op {
	MES_STMT_END = 0x00,
	MES_STMT_ZENKAKU = 0x01,
	MES_STMT_HANKAKU = 0x02,
	MES_STMT_SET_FLAG_CONST = 0x03,
	MES_STMT_SET_VAR16 = 0x04,
	MES_STMT_SET_FLAG_EXPR = 0x05,
	MES_STMT_PTR16_SET8 = 0x06,
	MES_STMT_PTR16_SET16 = 0x07,
	MES_STMT_PTR32_SET32 = 0x08,
	MES_STMT_PTR32_SET16 = 0x09,
	MES_STMT_PTR32_SET8 = 0x0A,
	MES_STMT_JZ = 0x0B,
	MES_STMT_JMP = 0x0C,
	MES_STMT_SYS = 0x0D,
	MES_STMT_JMP_MES = 0x0E,
	MES_STMT_CALL_MES = 0x0F,
	MES_STMT_DEF_MENU = 0x10,
	MES_STMT_CALL_PROC = 0x11,
	MES_STMT_UTIL = 0x12,
	MES_STMT_LINE = 0x13,
	MES_STMT_DEF_PROC = 0x14,
	MES_STMT_MENU_EXEC = 0x15,
	MES_STMT_SET_VAR32 = 0x16,
	MES_STMT_17 = 0x17,
	MES_STMT_18 = 0x18,
	MES_STMT_19 = 0x19,
	MES_STMT_1A = 0x1a,
	MES_STMT_1B = 0x1b,
	MES_STMT_DEF_SUB = 0x1c,
	MES_STMT_CALL_SUB = 0x1d,
	MES_STMT_1F = 0x1f,
	MES_STMT_SET_ARG_CONST,
	MES_STMT_SET_ARG_EXPR,
};
#define MES_STMT_OP_MAX (MES_STMT_SET_ARG_EXPR+1)

#define MES_CODE_INVALID 0xFF

enum mes_expression_op {
	MES_EXPR_IMM = 0x00, // not a real op
	MES_EXPR_GET_VAR16 = 0x80,
	MES_EXPR_PTR16_GET16 = 0xA0,
	MES_EXPR_PTR16_GET8 = 0xC0,
	// XXX: codes E0 -> F2 match AIWIN codes
	MES_EXPR_PLUS = 0xE0,
	MES_EXPR_MINUS = 0xE1,
	MES_EXPR_MUL = 0xE2,
	MES_EXPR_DIV = 0xE3,
	MES_EXPR_MOD = 0xE4,
	MES_EXPR_RAND = 0xE5,
	MES_EXPR_AND = 0xE6,
	MES_EXPR_OR = 0xE7,
	MES_EXPR_BITAND = 0xE8,
	MES_EXPR_BITIOR = 0xE9,
	MES_EXPR_BITXOR = 0xEA,
	MES_EXPR_LT = 0xEB,
	MES_EXPR_GT = 0xEC,
	MES_EXPR_LTE = 0xED,
	MES_EXPR_GTE = 0xEE,
	MES_EXPR_EQ = 0xEF,
	MES_EXPR_NEQ = 0xF0,
	MES_EXPR_IMM16 = 0xF1,
	MES_EXPR_IMM32 = 0xF2,
	MES_EXPR_GET_FLAG_CONST = 0xF3,
	MES_EXPR_GET_FLAG_EXPR = 0xF4,
	MES_EXPR_PTR32_GET32 = 0xF5,
	MES_EXPR_PTR32_GET16 = 0xF6,
	MES_EXPR_PTR32_GET8 = 0xF7,
	MES_EXPR_GET_VAR32 = 0xF8,
	MES_EXPR_GET_ARG_CONST,
	MES_EXPR_GET_ARG_EXPR,
	MES_EXPR_END = 0xFF
};
#define MES_EXPR_OP_MAX (MES_EXPR_END+1)

enum aiw_mes_statement_op {
	AIW_MES_STMT_TXT = 0x00,
	AIW_MES_STMT_JMP = 0x01,
	AIW_MES_STMT_UTIL = 0x02,
	AIW_MES_STMT_JMP_MES = 0x03,
	AIW_MES_STMT_CALL_MES = 0x04,
	AIW_MES_STMT_SET_FLAG_CONST = 0x05,
	AIW_MES_STMT_SET_FLAG_EXPR = 0x06,
	AIW_MES_STMT_SET_VAR32 = 0x07,
	AIW_MES_STMT_PTR_SET8 = 0x08,
	AIW_MES_STMT_PTR_SET16 = 0x09,
	AIW_MES_STMT_SET_VAR16_CONST = 0x0a,
	AIW_MES_STMT_SET_VAR16_EXPR = 0x0b,
	AIW_MES_STMT_SET_SYSVAR_CONST = 0x0c,
	AIW_MES_STMT_SET_SYSVAR_EXPR = 0x0d,
	AIW_MES_STMT_LOAD = 0x0e,
	AIW_MES_STMT_SAVE = 0x0f,
	AIW_MES_STMT_JZ = 0x10,
	AIW_MES_STMT_DEF_PROC = 0x11,
	AIW_MES_STMT_CALL_PROC = 0x12,
	AIW_MES_STMT_DEF_MENU = 0x13,
	AIW_MES_STMT_MENU_EXEC = 0x14,
	AIW_MES_STMT_NUM = 0x15,
	AIW_MES_STMT_SET_TEXT_COLOR = 0x16,
	AIW_MES_STMT_WAIT = 0x20,
	AIW_MES_STMT_21 = 0x21,
	AIW_MES_STMT_COMMIT_MESSAGE = 0x22,
	AIW_MES_STMT_LOAD_IMAGE = 0x23,
	AIW_MES_STMT_SURF_COPY = 0x24,
	AIW_MES_STMT_SURF_COPY_MASKED = 0x25,
	AIW_MES_STMT_SURF_SWAP = 0x26,
	AIW_MES_STMT_SURF_FILL = 0x27,
	AIW_MES_STMT_SURF_INVERT = 0x28,
	AIW_MES_STMT_29 = 0x29,
	AIW_MES_STMT_SHOW_HIDE = 0x2a,
	AIW_MES_STMT_CROSSFADE = 0x2b,
	AIW_MES_STMT_CROSSFADE2 = 0x2c,
	AIW_MES_STMT_CURSOR = 0x2d,
	AIW_MES_STMT_ANIM = 0x2e,
	AIW_MES_STMT_LOAD_AUDIO = 0x2f,
	AIW_MES_STMT_LOAD_EFFECT = 0x30,
	AIW_MES_STMT_LOAD_VOICE = 0x31,
	AIW_MES_STMT_AUDIO = 0x32,
	AIW_MES_STMT_PLAY_MOVIE = 0x33,
	AIW_MES_STMT_34 = 0x34,
	AIW_MES_STMT_35 = 0x35,
	AIW_MES_STMT_37 = 0x37,
	AIW_MES_STMT_FE = 0xFE,
	AIW_MES_STMT_END = 0xFF,
};

enum aiw_mes_expression_op {
	AIW_MES_EXPR_IMM = 0x00, // 0x00 - 0x7f
	AIW_MES_EXPR_VAR32 = 0x80, // 0x80 - 0x9f
	AIW_MES_EXPR_PTR_GET8 = 0xa0, // 0xa0 - 0xdf
	// XXX: codes E0 -> F2 match AI5 codes
	AIW_MES_EXPR_PLUS = 0xe0,
	AIW_MES_EXPR_MINUS = 0xe1,
	AIW_MES_EXPR_MUL = 0xe2,
	AIW_MES_EXPR_DIV = 0xe3,
	AIW_MES_EXPR_MOD = 0xe4,
	AIW_MES_EXPR_RAND = 0xe5,
	AIW_MES_EXPR_AND = 0xe6,
	AIW_MES_EXPR_OR = 0xe7,
	AIW_MES_EXPR_BITAND = 0xe8,
	AIW_MES_EXPR_BITIOR = 0xe9,
	AIW_MES_EXPR_BITXOR = 0xea,
	AIW_MES_EXPR_LT = 0xeb,
	AIW_MES_EXPR_GT = 0xec,
	AIW_MES_EXPR_LTE = 0xed,
	AIW_MES_EXPR_GTE = 0xee,
	AIW_MES_EXPR_EQ = 0xef,
	AIW_MES_EXPR_NEQ = 0xf0,
	AIW_MES_EXPR_IMM16 = 0xf1,
	AIW_MES_EXPR_IMM32 = 0xf2,
	AIW_MES_EXPR_GET_FLAG_CONST = 0xf3,
	AIW_MES_EXPR_GET_FLAG_EXPR = 0xf4,
	// XXX: 0xf5 reserved for string parameter marker
	AIW_MES_EXPR_GET_VAR16_CONST = 0xf6,
	AIW_MES_EXPR_GET_VAR16_EXPR = 0xf7,
	AIW_MES_EXPR_GET_SYSVAR_CONST = 0xf8,
	AIW_MES_EXPR_GET_SYSVAR_EXPR = 0xf9,
	AIW_MES_EXPR_END = 0xFF
};

enum mes_system_var16 {
	// offset to usable heap memory
	MES_SYS_VAR_HEAP,
	// destination surface for graphics operations
	MES_SYS_VAR_DST_SURFACE,
	// VM flags
	MES_SYS_VAR_FLAGS,
	// cursor location
	MES_SYS_VAR_CURSOR_X,
	MES_SYS_VAR_CURSOR_Y,
	// text area
	MES_SYS_VAR_TEXT_START_X,
	MES_SYS_VAR_TEXT_START_Y,
	MES_SYS_VAR_TEXT_END_X,
	MES_SYS_VAR_TEXT_END_Y,
	// text cursor location
	MES_SYS_VAR_TEXT_CURSOR_X,
	MES_SYS_VAR_TEXT_CURSOR_Y,
	// text colors (bgr555)
	MES_SYS_VAR_BG_COLOR,
	MES_SYS_VAR_FG_COLOR,
	// System.display_number flags
	MES_SYS_VAR_DISPLAY_NUMBER_FLAGS,
	// font parameters
	MES_SYS_VAR_FONT_WIDTH,
	MES_SYS_VAR_FONT_HEIGHT,
	MES_SYS_VAR_FONT_WEIGHT,
	MES_SYS_VAR_CHAR_SPACE,
	MES_SYS_VAR_LINE_SPACE,
	// CG rectangle
	MES_SYS_VAR_CG_X,
	MES_SYS_VAR_CG_Y,
	MES_SYS_VAR_CG_W,
	MES_SYS_VAR_CG_H,
	// number of menu entries
	MES_SYS_VAR_NR_MENU_ENTRIES,
	// return value of System.menu_get_no
	MES_SYS_VAR_MENU_NO,
	// mask color for graphics operations
	MES_SYS_VAR_MASK_COLOR,
};

enum mes_system_var32 {
	// pointer to memory
	MES_SYS_VAR_MEMORY,
	// offset to CG file in file_data
	MES_SYS_VAR_CG_OFFSET,
	// offset to data file in file_data
	MES_SYS_VAR_DATA_OFFSET,
	// offsets to map files
	MES_SYS_VAR_MPX_OFFSET,
	MES_SYS_VAR_CCD_OFFSET,
	MES_SYS_VAR_EVE_OFFSET,
	// pointer to palette
	MES_SYS_VAR_PALETTE,
	// offset to A6 file in file_data
	MES_SYS_VAR_A6_OFFSET,
	// pointer to file_data
	MES_SYS_VAR_FILE_DATA,
	// pointer to menu_entry_addresses
	MES_SYS_VAR_MENU_ENTRY_ADDRESSES,
	// pointer to menu_entry_numbers
	MES_SYS_VAR_MENU_ENTRY_NUMBERS,
	// offset to map file (dungeon)
	MES_SYS_VAR_MAP_DATA,
	// 24-bit mask color
	MES_SYS_VAR_MASK_COLOR24,
};

#define MES_NR_SYSTEM_VARIABLES 28

typedef struct mes_path_component *mes_namespace_t;

struct mes_code_tables {
	uint8_t stmt_op_to_int[MES_STMT_OP_MAX];
	enum mes_statement_op int_to_stmt_op[MES_STMT_OP_MAX];

	uint8_t expr_op_to_int[MES_EXPR_OP_MAX];
	enum mes_expression_op int_to_expr_op[MES_EXPR_OP_MAX];

	uint8_t sysvar16_to_int[MES_NR_SYSTEM_VARIABLES];
	enum mes_system_var16 int_to_sysvar16[MES_NR_SYSTEM_VARIABLES];

	uint8_t sysvar32_to_int[MES_NR_SYSTEM_VARIABLES];
	enum mes_system_var32 int_to_sysvar32[MES_NR_SYSTEM_VARIABLES];

	mes_namespace_t system;
	mes_namespace_t util;
};
extern struct mes_code_tables mes_code_tables;

// aliases for system variable codes
#define mes_sysvar16_dst_surface (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_DST_SURFACE])
#define mes_sysvar16_flags (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_FLAGS])
#define mes_sysvar16_cursor_x (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_CURSOR_X])
#define mes_sysvar16_cursor_y (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_CURSOR_Y])
#define mes_sysvar16_text_start_x (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_TEXT_START_X])
#define mes_sysvar16_text_start_y (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_TEXT_START_Y])
#define mes_sysvar16_text_end_x (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_TEXT_END_X])
#define mes_sysvar16_text_end_y (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_TEXT_END_Y])
#define mes_sysvar16_text_cursor_x (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_TEXT_CURSOR_X])
#define mes_sysvar16_text_cursor_y (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_TEXT_CURSOR_Y])
#define mes_sysvar16_bg_color (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_BG_COLOR])
#define mes_sysvar16_fg_color (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_FG_COLOR])
#define mes_sysvar16_display_number_flags (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_DISPLAY_NUMBER_FLAGS])
#define mes_sysvar16_font_width (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_FONT_WIDTH])
#define mes_sysvar16_font_height (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_FONT_HEIGHT])
#define mes_sysvar16_font_weight (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_FONT_WEIGHT])
#define mes_sysvar16_char_space (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_CHAR_SPACE])
#define mes_sysvar16_line_space (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_LINE_SPACE])
#define mes_sysvar16_cg_x (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_CG_X])
#define mes_sysvar16_cg_y (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_CG_Y])
#define mes_sysvar16_cg_w (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_CG_W])
#define mes_sysvar16_cg_h (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_CG_H])
#define mes_sysvar16_nr_menu_entries (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_NR_MENU_ENTRIES])
#define mes_sysvar16_menu_no (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_MENU_NO])
#define mes_sysvar16_mask_color (mes_code_tables.sysvar16_to_int[MES_SYS_VAR_MASK_COLOR])
#define mes_sysvar32_memory (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_MEMORY])
#define mes_sysvar32_cg_offset (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_CG_OFFSET])
#define mes_sysvar32_data_offset (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_DATA_OFFSET])
#define mes_sysvar32_mpx_offset (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_MPX_OFFSET])
#define mes_sysvar32_ccd_offset (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_CCD_OFFSET])
#define mes_sysvar32_eve_offset (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_EVE_OFFSET])
#define mes_sysvar32_palette (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_PALETTE])
#define mes_sysvar32_a6_offset (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_A6_OFFSET])
#define mes_sysvar32_file_data (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_FILE_DATA])
#define mes_sysvar32_menu_entry_addresses (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_MENU_ENTRY_ADDRESSES])
#define mes_sysvar32_menu_entry_numbers (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_MENU_ENTRY_NUMBERS])
#define mes_sysvar32_map_data (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_MAP_DATA])
#define mes_sysvar32_mask_color (mes_code_tables.sysvar32_to_int[MES_SYS_VAR_MASK_COLOR24])

/*
 * Convert a game-specific statement opcode to a virtual opcode.
 */
static inline enum mes_statement_op mes_opcode_to_stmt(uint8_t op)
{
	if (unlikely(op >= ARRAY_SIZE(mes_code_tables.int_to_stmt_op)))
		return (enum mes_statement_op)MES_CODE_INVALID;
	return mes_code_tables.int_to_stmt_op[op];
}

/*
 * Convert a game-specific expression opcode to a virtual expression opcode.
 */
static inline enum mes_expression_op mes_opcode_to_expr(uint8_t op)
{
	return mes_code_tables.int_to_expr_op[op];
}

/*
 * Convert a game-specific index to a mes_system_var16.
 */
static inline enum mes_system_var16 mes_index_to_sysvar16(uint8_t i)
{
	if (unlikely(i >= ARRAY_SIZE(mes_code_tables.int_to_sysvar16)))
		return (enum mes_system_var16)MES_CODE_INVALID;
	return mes_code_tables.int_to_sysvar16[i];
}

/*
 * Convert a game-specific index to a mes_system_var16.
 */
static inline enum mes_system_var32 mes_index_to_sysvar32(uint8_t i)
{
	if (unlikely(i >= ARRAY_SIZE(mes_code_tables.int_to_sysvar32)))
		return (enum mes_system_var32)MES_CODE_INVALID;
	return mes_code_tables.int_to_sysvar32[i];
}
/*
 * Convert a virtual statement opcode to a game-specific opcode.
 */
static inline uint8_t mes_stmt_opcode(enum mes_statement_op op)
{
	return mes_code_tables.stmt_op_to_int[op];
}

/*
 * Convert a virtual expression opcode to a game-specific opcode.
 */
static inline uint8_t mes_expr_opcode(enum mes_expression_op op)
{
	return mes_code_tables.expr_op_to_int[op];
}

/*
 * Convert a mes_system_var16 to a game-specific index.
 */
static inline uint8_t mes_sysvar16_index(enum mes_system_var16 v)
{
	return mes_code_tables.sysvar16_to_int[v];
}

/*
 * Convert a mes_system_var32 to a game-specific index.
 */
static inline uint8_t mes_sysvar32_index(enum mes_system_var32 v)
{
	return mes_code_tables.sysvar32_to_int[v];
}

extern const char *mes_system_var16_names[MES_NR_SYSTEM_VARIABLES];
extern const char *mes_system_var32_names[MES_NR_SYSTEM_VARIABLES];

enum mes_parameter_type {
	MES_PARAM_STRING = 1,
	MES_PARAM_EXPRESSION = 2,
};

struct mes_expression {
	union {
		enum mes_expression_op op;
		enum aiw_mes_expression_op aiw_op;
	};
	union {
		uint8_t arg8;
		uint16_t arg16;
		uint32_t arg32;
	};
	// unary operand / binary LHS operand
	struct mes_expression *sub_a;
	// binary RHS operand
	struct mes_expression *sub_b;
};

struct mes_parameter {
	enum mes_parameter_type type;
	union {
		string str;
		struct mes_expression *expr;
	};
};

typedef vector_t(struct mes_expression*) mes_expression_list;
typedef vector_t(struct mes_parameter) mes_parameter_list;

struct mes_qname_part {
	enum { MES_QNAME_IDENT, MES_QNAME_NUMBER } type;
	union { string ident; unsigned number; };
};

typedef vector_t(struct mes_qname_part) mes_qname;

struct mes_statement;
typedef vector_t(struct mes_statement*) mes_statement_list;

struct aiw_mes_menu_case {
	struct mes_expression *cond;
	mes_statement_list body;
};
typedef vector_t(struct aiw_mes_menu_case) aiw_menu_table;

struct mes_statement {
	union {
		enum mes_statement_op op;
		enum aiw_mes_statement_op aiw_op;
	};
	uint32_t address;
	uint32_t next_address;
	bool is_jump_target;
	union {
		struct {
			string text;
			bool terminated;
			bool unprefixed;
		} TXT;
		// SET_FLAG_CONST, SET_VAR16, SET_VAR32
		struct {
			uint16_t var_no;
			mes_expression_list val_exprs;
		} SET_VAR_CONST;
		// SET_FLAG_EXPR
		struct {
			struct mes_expression *var_expr;
			mes_expression_list val_exprs;
		} SET_VAR_EXPR;
		// PTR{16,32}_SET{8,16,32}
		struct {
			uint8_t var_no;
			struct mes_expression *off_expr;
			mes_expression_list val_exprs;
		} PTR_SET;
		// JZ
		struct {
			uint32_t addr;
			struct mes_expression *expr;
		} JZ;
		// JMP
		struct {
			uint32_t addr;
		} JMP;
		// SYS
		struct {
			struct mes_expression *expr;
			mes_parameter_list params;
		} SYS;
		// JMP_MES, CALL_MES, CALL_PROC, UTIL
		struct {
			mes_parameter_list params;
		} CALL;
		// DEF_MENU
		struct {
			uint32_t skip_addr;
			mes_parameter_list params;
		} DEF_MENU;
		// LINE
		struct {
			uint8_t arg;
		} LINE;
		// DEF_PROC
		struct {
			uint32_t skip_addr;
			struct mes_expression *no_expr;
		} DEF_PROC;
		struct {
			uint32_t table_addr;
			uint32_t skip_addr;
			struct mes_expression *expr;
			aiw_menu_table cases;
		} AIW_DEF_MENU;
		struct {
			mes_expression_list exprs;
		} AIW_MENU_EXEC;
		struct {
			uint16_t a;
			uint16_t b;
		} AIW_0x35;
	};
};

/* parse.c */
bool mes_char_is_hankaku(uint8_t b);
bool mes_char_is_zenkaku(uint8_t b);

struct mes_expression *mes_parse_expression(uint8_t *data, size_t data_size);
struct mes_statement *mes_parse_statement(uint8_t *data, size_t data_size);
bool mes_parse_statements(uint8_t *data, size_t data_size, mes_statement_list *statements);
void mes_qname_free(mes_qname name);
void mes_expression_free(struct mes_expression *expr);
void mes_expression_list_free(mes_expression_list list);
void mes_parameter_list_free(mes_parameter_list list);
void mes_statement_free(struct mes_statement *stmt);
void mes_statement_list_free(mes_statement_list list);

enum ai5_game_id;
void mes_set_game(enum ai5_game_id id);

/* print.c */
void mes_expression_print(struct mes_expression *expr, struct port *out);
void mes_expression_list_print(mes_expression_list list, struct port *out);
void mes_parameter_print(struct mes_parameter *param, struct port *out);
void mes_parameter_list_print(mes_parameter_list list, struct port *out);
void mes_parameter_list_print_from(mes_parameter_list list, unsigned start, struct port *out);
void _mes_statement_print(struct mes_statement *stmt, struct port *out, int indent);
void mes_statement_print(struct mes_statement *stmt, struct port *out);
void _mes_statement_list_print(mes_statement_list statements, struct port *out, int indent);
void mes_asm_statement_list_print(mes_statement_list statements, struct port *out);
void mes_flat_statement_list_print(mes_statement_list statements, struct port *out);
void mes_statement_list_print(mes_statement_list statements, struct port *out);
void mes_label_print(uint32_t addr, const char *suffix, struct port *out);
void mes_clear_labels(void);

void _aiw_mes_expression_print(struct mes_expression *expr, struct port *out, bool bitwise);
void _aiw_mes_statement_print(struct mes_statement *stmt, struct port *out, int indent);

/* system.c */
mes_parameter_list mes_resolve_syscall(mes_qname name, int *no);
mes_parameter_list mes_resolve_util(mes_qname name);
string mes_get_syscall_name(unsigned no, mes_parameter_list params, unsigned *skip_params,
		const char *ns);
string mes_get_util_name(mes_parameter_list params, unsigned *skip_params);
int mes_resolve_sysvar(string name, bool *dword);

#endif // AI5_MES_H
