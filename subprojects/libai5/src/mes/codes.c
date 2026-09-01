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
#include "ai5/mes.h"
#include "ai5/game.h"

// system.c
extern struct mes_path_component mes_sys_none;
extern struct mes_path_component mes_sys_isaku;
extern struct mes_path_component mes_sys_doukyuusei;
extern struct mes_path_component mes_sys_doukyuusei2;
extern struct mes_path_component mes_sys_kakyuusei;
extern struct mes_path_component mes_sys_allstars;
extern struct mes_path_component mes_sys_ai_shimai;
extern struct mes_path_component mes_sys_beyond;
extern struct mes_path_component mes_sys_classics;
extern struct mes_path_component mes_sys_shuusaku;
#define mes_sys_kawarazakike mes_sys_shuusaku
extern struct mes_path_component mes_util_none;
extern struct mes_path_component mes_util_isaku;
extern struct mes_path_component mes_util_aishimai;
extern struct mes_path_component mes_util_beyond;
extern struct mes_path_component mes_util_shangrlia;
extern struct mes_path_component mes_util_yuno;

// default opcode tables {{{
#define DEFAULT_STMT_OP_TO_INT { \
	[MES_STMT_END]            = 0x00, \
	[MES_STMT_ZENKAKU]        = 0x01, \
	[MES_STMT_HANKAKU]        = 0x02, \
	[MES_STMT_SET_FLAG_CONST] = 0x03, \
	[MES_STMT_SET_VAR16]      = 0x04, \
	[MES_STMT_SET_FLAG_EXPR]  = 0x05, \
	[MES_STMT_PTR16_SET8]     = 0x06, \
	[MES_STMT_PTR16_SET16]    = 0x07, \
	[MES_STMT_PTR32_SET32]    = 0x08, \
	[MES_STMT_PTR32_SET16]    = MES_CODE_INVALID, \
	[MES_STMT_PTR32_SET8]     = MES_CODE_INVALID, \
	[MES_STMT_SET_ARG_CONST]  = MES_CODE_INVALID, \
	[MES_STMT_SET_ARG_EXPR]   = MES_CODE_INVALID, \
	[MES_STMT_JZ]             = 0x09, \
	[MES_STMT_JMP]            = 0x0A, \
	[MES_STMT_SYS]            = 0x0B, \
	[MES_STMT_JMP_MES]        = 0x0C, \
	[MES_STMT_CALL_MES]       = 0x0D, \
	[MES_STMT_DEF_MENU]       = 0x0E, \
	[MES_STMT_CALL_PROC]      = 0x0F, \
	[MES_STMT_UTIL]           = 0x10, \
	[MES_STMT_LINE]           = 0x11, \
	[MES_STMT_DEF_PROC]       = 0x12, \
	[MES_STMT_MENU_EXEC]      = 0x13, \
	[MES_STMT_SET_VAR32]      = 0x14, \
	[MES_STMT_17]             = MES_CODE_INVALID, \
	[MES_STMT_18]             = MES_CODE_INVALID, \
	[MES_STMT_19]             = MES_CODE_INVALID, \
	[MES_STMT_1A]             = MES_CODE_INVALID, \
	[MES_STMT_1B]             = MES_CODE_INVALID, \
	[MES_STMT_DEF_SUB]        = MES_CODE_INVALID, \
	[MES_STMT_CALL_SUB]       = MES_CODE_INVALID, \
	[MES_STMT_1F]             = MES_CODE_INVALID, \
}

#define DEFAULT_INT_TO_STMT_OP { \
	[0x00] = MES_STMT_END, \
	[0x01] = MES_STMT_ZENKAKU, \
	[0x02] = MES_STMT_HANKAKU, \
	[0x03] = MES_STMT_SET_FLAG_CONST, \
	[0x04] = MES_STMT_SET_VAR16, \
	[0x05] = MES_STMT_SET_FLAG_EXPR, \
	[0x06] = MES_STMT_PTR16_SET8, \
	[0x07] = MES_STMT_PTR16_SET16, \
	[0x08] = MES_STMT_PTR32_SET32, \
	[0x09] = MES_STMT_JZ, \
	[0x0A] = MES_STMT_JMP, \
	[0x0B] = MES_STMT_SYS, \
	[0x0C] = MES_STMT_JMP_MES, \
	[0x0D] = MES_STMT_CALL_MES, \
	[0x0E] = MES_STMT_DEF_MENU, \
	[0x0F] = MES_STMT_CALL_PROC, \
	[0x10] = MES_STMT_UTIL, \
	[0x11] = MES_STMT_LINE, \
	[0x12] = MES_STMT_DEF_PROC, \
	[0x13] = MES_STMT_MENU_EXEC, \
	[0x14] = MES_STMT_SET_VAR32, \
	[0x15] = MES_CODE_INVALID, \
	[0x16] = MES_CODE_INVALID, \
	[0x17] = MES_CODE_INVALID, \
	[0x18] = MES_CODE_INVALID, \
	[0x19] = MES_CODE_INVALID, \
	[0x1a] = MES_CODE_INVALID, \
	[0x1b] = MES_CODE_INVALID, \
	[0x1c] = MES_CODE_INVALID, \
	[0x1d] = MES_CODE_INVALID, \
}

#define DEFAULT_EXPR_OP_TO_INT { \
	[MES_EXPR_GET_VAR16]      = 0x80, \
	[MES_EXPR_PTR16_GET16]    = 0xA0, \
	[MES_EXPR_PTR16_GET8]     = 0xC0, \
	[MES_EXPR_PLUS]           = 0xE0, \
	[MES_EXPR_MINUS]          = 0xE1, \
	[MES_EXPR_MUL]            = 0xE2, \
	[MES_EXPR_DIV]            = 0xE3, \
	[MES_EXPR_MOD]            = 0xE4, \
	[MES_EXPR_RAND]           = 0xE5, \
	[MES_EXPR_AND]            = 0xE6, \
	[MES_EXPR_OR]             = 0xE7, \
	[MES_EXPR_BITAND]         = 0xE8, \
	[MES_EXPR_BITIOR]         = 0xE9, \
	[MES_EXPR_BITXOR]         = 0xEA, \
	[MES_EXPR_LT]             = 0xEB, \
	[MES_EXPR_GT]             = 0xEC, \
	[MES_EXPR_LTE]            = 0xED, \
	[MES_EXPR_GTE]            = 0xEE, \
	[MES_EXPR_EQ]             = 0xEF, \
	[MES_EXPR_NEQ]            = 0xF0, \
	[MES_EXPR_IMM16]          = 0xF1, \
	[MES_EXPR_IMM32]          = 0xF2, \
	[MES_EXPR_GET_FLAG_CONST] = 0xF3, \
	[MES_EXPR_GET_FLAG_EXPR]  = 0xF4, \
	[MES_EXPR_PTR32_GET32]    = 0xF5, \
	[MES_EXPR_GET_VAR32]      = 0xF6, \
	[MES_EXPR_GET_ARG_CONST]  = MES_CODE_INVALID, \
	[MES_EXPR_GET_ARG_EXPR]   = MES_CODE_INVALID, \
	[MES_EXPR_END]            = 0xFF, \
}

#define DEFAULT_INT_TO_EXPR_OP { \
	[0x80] = MES_EXPR_GET_VAR16, \
	[0xA0] = MES_EXPR_PTR16_GET16, \
	[0xC0] = MES_EXPR_PTR16_GET8, \
	[0xE0] = MES_EXPR_PLUS, \
	[0xE1] = MES_EXPR_MINUS, \
	[0xE2] = MES_EXPR_MUL, \
	[0xE3] = MES_EXPR_DIV, \
	[0xE4] = MES_EXPR_MOD, \
	[0xE5] = MES_EXPR_RAND, \
	[0xE6] = MES_EXPR_AND, \
	[0xE7] = MES_EXPR_OR, \
	[0xE8] = MES_EXPR_BITAND, \
	[0xE9] = MES_EXPR_BITIOR, \
	[0xEA] = MES_EXPR_BITXOR, \
	[0xEB] = MES_EXPR_LT, \
	[0xEC] = MES_EXPR_GT, \
	[0xED] = MES_EXPR_LTE, \
	[0xEE] = MES_EXPR_GTE, \
	[0xEF] = MES_EXPR_EQ, \
	[0xF0] = MES_EXPR_NEQ, \
	[0xF1] = MES_EXPR_IMM16, \
	[0xF2] = MES_EXPR_IMM32, \
	[0xF3] = MES_EXPR_GET_FLAG_CONST, \
	[0xF4] = MES_EXPR_GET_FLAG_EXPR, \
	[0xF5] = MES_EXPR_PTR32_GET32, \
	[0xF6] = MES_EXPR_GET_VAR32, \
	[0xFF] = MES_EXPR_END, \
}

#define DEFAULT_SYSVAR16_TO_INT { \
	[MES_SYS_VAR_HEAP] = 0, \
	[MES_SYS_VAR_DST_SURFACE] = 1, \
	[MES_SYS_VAR_FLAGS] = 2, \
	[MES_SYS_VAR_CURSOR_X] = 3, \
	[MES_SYS_VAR_CURSOR_Y] = 4, \
	[MES_SYS_VAR_TEXT_START_X] = 5, \
	[MES_SYS_VAR_TEXT_START_Y] = 6, \
	[MES_SYS_VAR_TEXT_END_X] = 7, \
	[MES_SYS_VAR_TEXT_END_Y] = 8, \
	[MES_SYS_VAR_TEXT_CURSOR_X] = 9, \
	[MES_SYS_VAR_TEXT_CURSOR_Y] = 10, \
	[MES_SYS_VAR_BG_COLOR] = 11, \
	[MES_SYS_VAR_FG_COLOR] = 12, \
	[MES_SYS_VAR_DISPLAY_NUMBER_FLAGS] = 13, \
	[MES_SYS_VAR_FONT_WIDTH] = 14, \
	[MES_SYS_VAR_FONT_HEIGHT] = 15, \
	[MES_SYS_VAR_FONT_WEIGHT] = 16, \
	[MES_SYS_VAR_CHAR_SPACE] = 17, \
	[MES_SYS_VAR_LINE_SPACE] = 18, \
	[MES_SYS_VAR_CG_X] = 19, \
	[MES_SYS_VAR_CG_Y] = 20, \
	[MES_SYS_VAR_CG_W] = 21, \
	[MES_SYS_VAR_CG_H] = 22, \
	[MES_SYS_VAR_MASK_COLOR] = 23, \
	[MES_SYS_VAR_NR_MENU_ENTRIES] = 24, \
	[MES_SYS_VAR_MENU_NO] = 25, \
}

#define DEFAULT_INT_TO_SYSVAR16 { \
	[0]  = MES_SYS_VAR_HEAP, \
	[1]  = MES_SYS_VAR_DST_SURFACE, \
	[2]  = MES_SYS_VAR_FLAGS, \
	[3]  = MES_SYS_VAR_CURSOR_X, \
	[4]  = MES_SYS_VAR_CURSOR_Y, \
	[5]  = MES_SYS_VAR_TEXT_START_X, \
	[6]  = MES_SYS_VAR_TEXT_START_Y, \
	[7]  = MES_SYS_VAR_TEXT_END_X, \
	[8]  = MES_SYS_VAR_TEXT_END_Y, \
	[9]  = MES_SYS_VAR_TEXT_CURSOR_X, \
	[10] = MES_SYS_VAR_TEXT_CURSOR_Y, \
	[11] = MES_SYS_VAR_BG_COLOR, \
	[12] = MES_SYS_VAR_FG_COLOR, \
	[13] = MES_SYS_VAR_DISPLAY_NUMBER_FLAGS, \
	[14] = MES_SYS_VAR_FONT_WIDTH, \
	[15] = MES_SYS_VAR_FONT_HEIGHT, \
	[16] = MES_SYS_VAR_FONT_WEIGHT, \
	[17] = MES_SYS_VAR_CHAR_SPACE, \
	[18] = MES_SYS_VAR_LINE_SPACE, \
	[19] = MES_SYS_VAR_CG_X, \
	[20] = MES_SYS_VAR_CG_Y, \
	[21] = MES_SYS_VAR_CG_W, \
	[22] = MES_SYS_VAR_CG_H, \
	[23] = MES_SYS_VAR_MASK_COLOR, \
	[24] = MES_SYS_VAR_NR_MENU_ENTRIES, \
	[25] = MES_SYS_VAR_MENU_NO, \
	[26] = MES_CODE_INVALID, \
	[27] = MES_CODE_INVALID, \
}

#define DEFAULT_SYSVAR32_TO_INT { \
	[MES_SYS_VAR_MEMORY] = 0, \
	[MES_SYS_VAR_CG_OFFSET] = 1, \
	[MES_SYS_VAR_DATA_OFFSET] = 2, \
	[MES_SYS_VAR_MPX_OFFSET] = 3, \
	[MES_SYS_VAR_CCD_OFFSET] = 4, \
	[MES_SYS_VAR_EVE_OFFSET] = 5, \
	[MES_SYS_VAR_PALETTE] = MES_CODE_INVALID, \
	[MES_SYS_VAR_A6_OFFSET] = 6, \
	[MES_SYS_VAR_FILE_DATA] = 7, \
	[MES_SYS_VAR_MENU_ENTRY_ADDRESSES] = 8, \
	[MES_SYS_VAR_MENU_ENTRY_NUMBERS] = 9, \
	[MES_SYS_VAR_MAP_DATA] = 10, \
	[MES_SYS_VAR_MASK_COLOR24] = 11, \
}

#define DEFAULT_INT_TO_SYSVAR32 { \
	[0]  = MES_SYS_VAR_MEMORY, \
	[1]  = MES_SYS_VAR_CG_OFFSET, \
	[2]  = MES_SYS_VAR_DATA_OFFSET, \
	[3]  = MES_SYS_VAR_MPX_OFFSET, \
	[4]  = MES_SYS_VAR_CCD_OFFSET, \
	[5]  = MES_SYS_VAR_EVE_OFFSET, \
	[6]  = MES_SYS_VAR_A6_OFFSET, \
	[7]  = MES_SYS_VAR_FILE_DATA, \
	[8]  = MES_SYS_VAR_MENU_ENTRY_ADDRESSES, \
	[9]  = MES_SYS_VAR_MENU_ENTRY_NUMBERS, \
	[10] = MES_SYS_VAR_MAP_DATA, \
	[11] = MES_SYS_VAR_MASK_COLOR24, \
	[12] = MES_CODE_INVALID, \
	[13] = MES_CODE_INVALID, \
	[14] = MES_CODE_INVALID, \
	[15] = MES_CODE_INVALID, \
	[16] = MES_CODE_INVALID, \
	[17] = MES_CODE_INVALID, \
	[18] = MES_CODE_INVALID, \
	[19] = MES_CODE_INVALID, \
	[20] = MES_CODE_INVALID, \
	[21] = MES_CODE_INVALID, \
	[22] = MES_CODE_INVALID, \
	[23] = MES_CODE_INVALID, \
	[24] = MES_CODE_INVALID, \
	[25] = MES_CODE_INVALID, \
	[26] = MES_CODE_INVALID, \
	[27] = MES_CODE_INVALID, \
}

struct mes_code_tables default_tables = {
	.stmt_op_to_int = DEFAULT_STMT_OP_TO_INT,
	.int_to_stmt_op = DEFAULT_INT_TO_STMT_OP,
	.expr_op_to_int = DEFAULT_EXPR_OP_TO_INT,
	.int_to_expr_op = DEFAULT_INT_TO_EXPR_OP,
	.sysvar16_to_int = DEFAULT_SYSVAR16_TO_INT,
	.int_to_sysvar16 = DEFAULT_INT_TO_SYSVAR16,
	.sysvar32_to_int = DEFAULT_SYSVAR32_TO_INT,
	.int_to_sysvar32 = DEFAULT_INT_TO_SYSVAR32,
};
// default opcode tables }}}
// aishimai {{{

#define AI_SHIMAI_SYSVAR16_TO_INT { \
	[MES_SYS_VAR_HEAP] = 0, \
	[MES_SYS_VAR_DST_SURFACE] = 1, \
	[MES_SYS_VAR_FLAGS] = 2, \
	[MES_SYS_VAR_CURSOR_X] = 3, \
	[MES_SYS_VAR_CURSOR_Y] = 4, \
	[MES_SYS_VAR_TEXT_START_X] = 5, \
	[MES_SYS_VAR_TEXT_START_Y] = 6, \
	[MES_SYS_VAR_TEXT_END_X] = 7, \
	[MES_SYS_VAR_TEXT_END_Y] = 8, \
	[MES_SYS_VAR_TEXT_CURSOR_X] = 9, \
	[MES_SYS_VAR_TEXT_CURSOR_Y] = 10, \
	[MES_SYS_VAR_BG_COLOR] = MES_CODE_INVALID, \
	[MES_SYS_VAR_FG_COLOR] = MES_CODE_INVALID, \
	[MES_SYS_VAR_DISPLAY_NUMBER_FLAGS] = 11, \
	[MES_SYS_VAR_FONT_WIDTH] = 12, \
	[MES_SYS_VAR_FONT_HEIGHT] = 13, \
	[MES_SYS_VAR_FONT_WEIGHT] = 14, \
	[MES_SYS_VAR_CHAR_SPACE] = 15, \
	[MES_SYS_VAR_LINE_SPACE] = 16, \
	[MES_SYS_VAR_CG_X] = 17, \
	[MES_SYS_VAR_CG_Y] = 18, \
	[MES_SYS_VAR_CG_W] = 19, \
	[MES_SYS_VAR_CG_H] = 20, \
	[MES_SYS_VAR_MASK_COLOR] = MES_CODE_INVALID, \
	[MES_SYS_VAR_NR_MENU_ENTRIES] = 21, \
	[MES_SYS_VAR_MENU_NO] = 22, \
}

#define AI_SHIMAI_INT_TO_SYSVAR16 { \
	[0]  = MES_SYS_VAR_HEAP, \
	[1]  = MES_SYS_VAR_DST_SURFACE, \
	[2]  = MES_SYS_VAR_FLAGS, \
	[3]  = MES_SYS_VAR_CURSOR_X, \
	[4]  = MES_SYS_VAR_CURSOR_Y, \
	[5]  = MES_SYS_VAR_TEXT_START_X, \
	[6]  = MES_SYS_VAR_TEXT_START_Y, \
	[7]  = MES_SYS_VAR_TEXT_END_X, \
	[8]  = MES_SYS_VAR_TEXT_END_Y, \
	[9]  = MES_SYS_VAR_TEXT_CURSOR_X, \
	[10] = MES_SYS_VAR_TEXT_CURSOR_Y, \
	[11] = MES_SYS_VAR_DISPLAY_NUMBER_FLAGS, \
	[12] = MES_SYS_VAR_FONT_WIDTH, \
	[13] = MES_SYS_VAR_FONT_HEIGHT, \
	[14] = MES_SYS_VAR_FONT_WEIGHT, \
	[15] = MES_SYS_VAR_CHAR_SPACE, \
	[16] = MES_SYS_VAR_LINE_SPACE, \
	[17] = MES_SYS_VAR_CG_X, \
	[18] = MES_SYS_VAR_CG_Y, \
	[19] = MES_SYS_VAR_CG_W, \
	[20] = MES_SYS_VAR_CG_H, \
	[21] = MES_SYS_VAR_NR_MENU_ENTRIES, \
	[22] = MES_SYS_VAR_MENU_NO, \
	[23] = MES_CODE_INVALID, \
	[24] = MES_CODE_INVALID, \
	[25] = MES_CODE_INVALID, \
	[26] = MES_CODE_INVALID, \
	[27] = MES_CODE_INVALID, \
}

struct mes_code_tables ai_shimai_tables = {
	.stmt_op_to_int = DEFAULT_STMT_OP_TO_INT,
	.int_to_stmt_op = DEFAULT_INT_TO_STMT_OP,
	.expr_op_to_int = DEFAULT_EXPR_OP_TO_INT,
	.int_to_expr_op = DEFAULT_INT_TO_EXPR_OP,
	.sysvar16_to_int = AI_SHIMAI_SYSVAR16_TO_INT,
	.int_to_sysvar16 = AI_SHIMAI_INT_TO_SYSVAR16,
	.sysvar32_to_int = DEFAULT_SYSVAR32_TO_INT,
	.int_to_sysvar32 = DEFAULT_INT_TO_SYSVAR32,
};

// aishimai }}}
// allstars / beyond {{{

#define ALLSTARS_SYSVAR16_TO_INT { \
	[MES_SYS_VAR_HEAP] = 0, \
	[MES_SYS_VAR_DST_SURFACE] = 1, \
	[MES_SYS_VAR_FLAGS] = 2, \
	[MES_SYS_VAR_CURSOR_X] = 3, \
	[MES_SYS_VAR_CURSOR_Y] = 4, \
	[MES_SYS_VAR_TEXT_START_X] = 5, \
	[MES_SYS_VAR_TEXT_START_Y] = 6, \
	[MES_SYS_VAR_TEXT_END_X] = 7, \
	[MES_SYS_VAR_TEXT_END_Y] = 8, \
	[MES_SYS_VAR_TEXT_CURSOR_X] = 9, \
	[MES_SYS_VAR_TEXT_CURSOR_Y] = 10, \
	[MES_SYS_VAR_BG_COLOR] = MES_CODE_INVALID, \
	[MES_SYS_VAR_FG_COLOR] = MES_CODE_INVALID, \
	[MES_SYS_VAR_DISPLAY_NUMBER_FLAGS] = 11, \
	[MES_SYS_VAR_FONT_WIDTH] = 12, \
	[MES_SYS_VAR_FONT_HEIGHT] = 13, \
	[MES_SYS_VAR_FONT_WEIGHT] = 14, \
	[MES_SYS_VAR_CHAR_SPACE] = 15, \
	[MES_SYS_VAR_LINE_SPACE] = 16, \
	[MES_SYS_VAR_CG_X] = 17, \
	[MES_SYS_VAR_CG_Y] = 18, \
	[MES_SYS_VAR_CG_W] = 19, \
	[MES_SYS_VAR_CG_H] = 20, \
	[MES_SYS_VAR_NR_MENU_ENTRIES] = 21, \
	[MES_SYS_VAR_MENU_NO] = 22, \
	[MES_SYS_VAR_MASK_COLOR] = 23, \
}

#define ALLSTARS_INT_TO_SYSVAR16 { \
	[0]  = MES_SYS_VAR_HEAP, \
	[1]  = MES_SYS_VAR_DST_SURFACE, \
	[2]  = MES_SYS_VAR_FLAGS, \
	[3]  = MES_SYS_VAR_CURSOR_X, \
	[4]  = MES_SYS_VAR_CURSOR_Y, \
	[5]  = MES_SYS_VAR_TEXT_START_X, \
	[6]  = MES_SYS_VAR_TEXT_START_Y, \
	[7]  = MES_SYS_VAR_TEXT_END_X, \
	[8]  = MES_SYS_VAR_TEXT_END_Y, \
	[9]  = MES_SYS_VAR_TEXT_CURSOR_X, \
	[10] = MES_SYS_VAR_TEXT_CURSOR_Y, \
	[11] = MES_SYS_VAR_DISPLAY_NUMBER_FLAGS, \
	[12] = MES_SYS_VAR_FONT_WIDTH, \
	[13] = MES_SYS_VAR_FONT_HEIGHT, \
	[14] = MES_SYS_VAR_FONT_WEIGHT, \
	[15] = MES_SYS_VAR_CHAR_SPACE, \
	[16] = MES_SYS_VAR_LINE_SPACE, \
	[17] = MES_SYS_VAR_CG_X, \
	[18] = MES_SYS_VAR_CG_Y, \
	[19] = MES_SYS_VAR_CG_W, \
	[20] = MES_SYS_VAR_CG_H, \
	[21] = MES_SYS_VAR_NR_MENU_ENTRIES, \
	[22] = MES_SYS_VAR_MENU_NO, \
	[23] = MES_SYS_VAR_MASK_COLOR, \
	[24] = MES_CODE_INVALID, \
	[25] = MES_CODE_INVALID, \
	[26] = MES_CODE_INVALID, \
	[27] = MES_CODE_INVALID, \
}

struct mes_code_tables allstars_tables = {
	.stmt_op_to_int = DEFAULT_STMT_OP_TO_INT,
	.int_to_stmt_op = DEFAULT_INT_TO_STMT_OP,
	.expr_op_to_int = DEFAULT_EXPR_OP_TO_INT,
	.int_to_expr_op = DEFAULT_INT_TO_EXPR_OP,
	.sysvar16_to_int = ALLSTARS_SYSVAR16_TO_INT,
	.int_to_sysvar16 = ALLSTARS_INT_TO_SYSVAR16,
	.sysvar32_to_int = DEFAULT_SYSVAR32_TO_INT,
	.int_to_sysvar32 = DEFAULT_INT_TO_SYSVAR32,
	.system = &mes_sys_allstars,
	.util = &mes_util_none,
};

struct mes_code_tables beyond_tables = {
	.stmt_op_to_int = DEFAULT_STMT_OP_TO_INT,
	.int_to_stmt_op = DEFAULT_INT_TO_STMT_OP,
	.expr_op_to_int = DEFAULT_EXPR_OP_TO_INT,
	.int_to_expr_op = DEFAULT_INT_TO_EXPR_OP,
	.sysvar16_to_int = ALLSTARS_SYSVAR16_TO_INT,
	.int_to_sysvar16 = ALLSTARS_INT_TO_SYSVAR16,
	.sysvar32_to_int = DEFAULT_SYSVAR32_TO_INT,
	.int_to_sysvar32 = DEFAULT_INT_TO_SYSVAR32,
	.system = &mes_sys_beyond,
	.util = &mes_util_beyond,
};

// allstars / beyond }}}
// elf_classics {{{
#define CLASSICS_STMT_OP_TO_INT { \
	[MES_STMT_END]            = 0x00, \
	[MES_STMT_ZENKAKU]        = 0x01, \
	[MES_STMT_HANKAKU]        = 0x02, \
	[MES_STMT_SET_FLAG_CONST] = 0x03, \
	[MES_STMT_SET_VAR16]      = 0x04, \
	[MES_STMT_SET_FLAG_EXPR]  = 0x05, \
	[MES_STMT_PTR16_SET8]     = 0x06, \
	[MES_STMT_PTR16_SET16]    = 0x07, \
	[MES_STMT_PTR32_SET32]    = 0x08, \
	[MES_STMT_PTR32_SET16]    = 0x09, \
	[MES_STMT_PTR32_SET8]     = 0x0A, \
	[MES_STMT_SET_ARG_CONST]  = MES_CODE_INVALID, \
	[MES_STMT_SET_ARG_EXPR]   = MES_CODE_INVALID, \
	[MES_STMT_JZ]             = 0x0B, \
	[MES_STMT_JMP]            = 0x0C, \
	[MES_STMT_SYS]            = 0x0D, \
	[MES_STMT_JMP_MES]        = 0x0E, \
	[MES_STMT_CALL_MES]       = 0x0F, \
	[MES_STMT_DEF_MENU]       = 0x10, \
	[MES_STMT_CALL_PROC]      = 0x11, \
	[MES_STMT_UTIL]           = 0x12, \
	[MES_STMT_LINE]           = 0x13, \
	[MES_STMT_DEF_PROC]       = 0x14, \
	[MES_STMT_MENU_EXEC]      = 0x15, \
	[MES_STMT_SET_VAR32]      = 0x16, \
	[MES_STMT_17]             = MES_CODE_INVALID, \
	[MES_STMT_18]             = MES_CODE_INVALID, \
	[MES_STMT_19]             = MES_CODE_INVALID, \
	[MES_STMT_1A]             = MES_CODE_INVALID, \
	[MES_STMT_1B]             = MES_CODE_INVALID, \
	[MES_STMT_DEF_SUB]        = MES_CODE_INVALID, \
	[MES_STMT_CALL_SUB]       = MES_CODE_INVALID, \
	[MES_STMT_1F]             = MES_CODE_INVALID, \
}

#define CLASSICS_INT_TO_STMT_OP { \
	[0x00] = MES_STMT_END, \
	[0x01] = MES_STMT_ZENKAKU, \
	[0x02] = MES_STMT_HANKAKU, \
	[0x03] = MES_STMT_SET_FLAG_CONST, \
	[0x04] = MES_STMT_SET_VAR16, \
	[0x05] = MES_STMT_SET_FLAG_EXPR, \
	[0x06] = MES_STMT_PTR16_SET8, \
	[0x07] = MES_STMT_PTR16_SET16, \
	[0x08] = MES_STMT_PTR32_SET32, \
	[0x09] = MES_STMT_PTR32_SET16, \
	[0x0A] = MES_STMT_PTR32_SET8, \
	[0x0B] = MES_STMT_JZ, \
	[0x0C] = MES_STMT_JMP, \
	[0x0D] = MES_STMT_SYS, \
	[0x0E] = MES_STMT_JMP_MES, \
	[0x0F] = MES_STMT_CALL_MES, \
	[0x10] = MES_STMT_DEF_MENU, \
	[0x11] = MES_STMT_CALL_PROC, \
	[0x12] = MES_STMT_UTIL, \
	[0x13] = MES_STMT_LINE, \
	[0x14] = MES_STMT_DEF_PROC, \
	[0x15] = MES_STMT_MENU_EXEC, \
	[0x16] = MES_STMT_SET_VAR32, \
	[0x17] = MES_CODE_INVALID, \
	[0x18] = MES_CODE_INVALID, \
	[0x19] = MES_CODE_INVALID, \
	[0x1a] = MES_CODE_INVALID, \
	[0x1b] = MES_CODE_INVALID, \
	[0x1c] = MES_CODE_INVALID, \
	[0x1d] = MES_CODE_INVALID, \
}

#define CLASSICS_EXPR_OP_TO_INT { \
	[MES_EXPR_IMM]            = 0x00, \
	[MES_EXPR_GET_VAR16]      = 0x80, \
	[MES_EXPR_PTR16_GET16]    = 0xA0, \
	[MES_EXPR_PTR16_GET8]     = 0xC0, \
	[MES_EXPR_PLUS]           = 0xE0, \
	[MES_EXPR_MINUS]          = 0xE1, \
	[MES_EXPR_MUL]            = 0xE2, \
	[MES_EXPR_DIV]            = 0xE3, \
	[MES_EXPR_MOD]            = 0xE4, \
	[MES_EXPR_RAND]           = 0xE5, \
	[MES_EXPR_AND]            = 0xE6, \
	[MES_EXPR_OR]             = 0xE7, \
	[MES_EXPR_BITAND]         = 0xE8, \
	[MES_EXPR_BITIOR]         = 0xE9, \
	[MES_EXPR_BITXOR]         = 0xEA, \
	[MES_EXPR_LT]             = 0xEB, \
	[MES_EXPR_GT]             = 0xEC, \
	[MES_EXPR_LTE]            = 0xED, \
	[MES_EXPR_GTE]            = 0xEE, \
	[MES_EXPR_EQ]             = 0xEF, \
	[MES_EXPR_NEQ]            = 0xF0, \
	[MES_EXPR_IMM16]          = 0xF1, \
	[MES_EXPR_IMM32]          = 0xF2, \
	[MES_EXPR_GET_FLAG_CONST] = 0xF3, \
	[MES_EXPR_GET_FLAG_EXPR]  = 0xF4, \
	[MES_EXPR_PTR32_GET32]    = 0xF5, \
	[MES_EXPR_PTR32_GET16]    = 0xF6, \
	[MES_EXPR_PTR32_GET8]     = 0xF7, \
	[MES_EXPR_GET_VAR32]      = 0xF8, \
	[MES_EXPR_GET_ARG_CONST]  = MES_CODE_INVALID, \
	[MES_EXPR_GET_ARG_EXPR]   = MES_CODE_INVALID, \
	[MES_EXPR_END]            = 0xFF, \
}

#define CLASSICS_INT_TO_EXPR_OP { \
	[0x80] = MES_EXPR_GET_VAR16, \
	[0xA0] = MES_EXPR_PTR16_GET16, \
	[0xC0] = MES_EXPR_PTR16_GET8, \
	[0xE0] = MES_EXPR_PLUS, \
	[0xE1] = MES_EXPR_MINUS, \
	[0xE2] = MES_EXPR_MUL, \
	[0xE3] = MES_EXPR_DIV, \
	[0xE4] = MES_EXPR_MOD, \
	[0xE5] = MES_EXPR_RAND, \
	[0xE6] = MES_EXPR_AND, \
	[0xE7] = MES_EXPR_OR, \
	[0xE8] = MES_EXPR_BITAND, \
	[0xE9] = MES_EXPR_BITIOR, \
	[0xEA] = MES_EXPR_BITXOR, \
	[0xEB] = MES_EXPR_LT, \
	[0xEC] = MES_EXPR_GT, \
	[0xED] = MES_EXPR_LTE, \
	[0xEE] = MES_EXPR_GTE, \
	[0xEF] = MES_EXPR_EQ, \
	[0xF0] = MES_EXPR_NEQ, \
	[0xF1] = MES_EXPR_IMM16, \
	[0xF2] = MES_EXPR_IMM32, \
	[0xF3] = MES_EXPR_GET_FLAG_CONST, \
	[0xF4] = MES_EXPR_GET_FLAG_EXPR, \
	[0xF5] = MES_EXPR_PTR32_GET32, \
	[0xF6] = MES_EXPR_PTR32_GET16, \
	[0xF7] = MES_EXPR_PTR32_GET8, \
	[0xF8] = MES_EXPR_GET_VAR32, \
	[0xFF] = MES_EXPR_END, \
}

#define CLASSICS_SYSVAR16_TO_INT { \
	[MES_SYS_VAR_HEAP] = 0, \
	[MES_SYS_VAR_DST_SURFACE] = 1, \
	[MES_SYS_VAR_FLAGS] = 2, \
	[MES_SYS_VAR_CURSOR_X] = 3, \
	[MES_SYS_VAR_CURSOR_Y] = 4, \
	[MES_SYS_VAR_TEXT_START_X] = 5, \
	[MES_SYS_VAR_TEXT_START_Y] = 6, \
	[MES_SYS_VAR_TEXT_END_X] = 7, \
	[MES_SYS_VAR_TEXT_END_Y] = 8, \
	[MES_SYS_VAR_TEXT_CURSOR_X] = 9, \
	[MES_SYS_VAR_TEXT_CURSOR_Y] = 10, \
	[MES_SYS_VAR_BG_COLOR] = MES_CODE_INVALID, \
	[MES_SYS_VAR_FG_COLOR] = MES_CODE_INVALID, \
	[MES_SYS_VAR_DISPLAY_NUMBER_FLAGS] = 11, \
	[MES_SYS_VAR_FONT_WIDTH] = 12, \
	[MES_SYS_VAR_FONT_HEIGHT] = 13, \
	[MES_SYS_VAR_FONT_WEIGHT] = 14, \
	[MES_SYS_VAR_CHAR_SPACE] = 15, \
	[MES_SYS_VAR_LINE_SPACE] = 16, \
	[MES_SYS_VAR_CG_X] = 17, \
	[MES_SYS_VAR_CG_Y] = 18, \
	[MES_SYS_VAR_CG_W] = 19, \
	[MES_SYS_VAR_CG_H] = 20, \
	[MES_SYS_VAR_NR_MENU_ENTRIES] = 21, \
	[MES_SYS_VAR_MENU_NO] = 22, \
	[MES_SYS_VAR_MASK_COLOR] = 23, \
}

#define CLASSICS_INT_TO_SYSVAR16 { \
	[0]  = MES_SYS_VAR_HEAP, \
	[1]  = MES_SYS_VAR_DST_SURFACE, \
	[2]  = MES_SYS_VAR_FLAGS, \
	[3]  = MES_SYS_VAR_CURSOR_X, \
	[4]  = MES_SYS_VAR_CURSOR_Y, \
	[5]  = MES_SYS_VAR_TEXT_START_X, \
	[6]  = MES_SYS_VAR_TEXT_START_Y, \
	[7]  = MES_SYS_VAR_TEXT_END_X, \
	[8]  = MES_SYS_VAR_TEXT_END_Y, \
	[9]  = MES_SYS_VAR_TEXT_CURSOR_X, \
	[10] = MES_SYS_VAR_TEXT_CURSOR_Y, \
	[11] = MES_SYS_VAR_DISPLAY_NUMBER_FLAGS, \
	[12] = MES_SYS_VAR_FONT_WIDTH, \
	[13] = MES_SYS_VAR_FONT_HEIGHT, \
	[14] = MES_SYS_VAR_FONT_WEIGHT, \
	[15] = MES_SYS_VAR_CHAR_SPACE, \
	[16] = MES_SYS_VAR_LINE_SPACE, \
	[17] = MES_SYS_VAR_CG_X, \
	[18] = MES_SYS_VAR_CG_Y, \
	[19] = MES_SYS_VAR_CG_W, \
	[20] = MES_SYS_VAR_CG_H, \
	[21] = MES_SYS_VAR_NR_MENU_ENTRIES, \
	[22] = MES_SYS_VAR_MENU_NO, \
	[23] = MES_SYS_VAR_MASK_COLOR, \
	[24] = MES_CODE_INVALID, \
	[25] = MES_CODE_INVALID, \
	[26] = MES_CODE_INVALID, \
	[27] = MES_CODE_INVALID, \
}

#define CLASSICS_SYSVAR32_TO_INT { \
	[MES_SYS_VAR_MEMORY] = 0, \
	[MES_SYS_VAR_CG_OFFSET] = 1, \
	[MES_SYS_VAR_DATA_OFFSET] = 2, \
	[MES_SYS_VAR_PALETTE] = 5, \
	[MES_SYS_VAR_A6_OFFSET] = MES_CODE_INVALID, \
	[MES_SYS_VAR_FILE_DATA] = 7, \
	[MES_SYS_VAR_MENU_ENTRY_ADDRESSES] = 8, \
	[MES_SYS_VAR_MENU_ENTRY_NUMBERS] = 9, \
	[MES_SYS_VAR_MAP_DATA] = MES_CODE_INVALID, \
}

#define CLASSICS_INT_TO_SYSVAR32 { \
	[0]  = MES_SYS_VAR_MEMORY, \
	[1]  = MES_SYS_VAR_CG_OFFSET, \
	[2]  = MES_SYS_VAR_DATA_OFFSET, \
	[3]  = MES_CODE_INVALID, \
	[4]  = MES_CODE_INVALID, \
	[5]  = MES_SYS_VAR_PALETTE, \
	[6]  = MES_CODE_INVALID, \
	[7]  = MES_SYS_VAR_FILE_DATA, \
	[8]  = MES_SYS_VAR_MENU_ENTRY_ADDRESSES, \
	[9]  = MES_SYS_VAR_MENU_ENTRY_NUMBERS, \
	[10] = MES_CODE_INVALID, \
	[11] = MES_CODE_INVALID, \
	[12] = MES_CODE_INVALID, \
	[13] = MES_CODE_INVALID, \
	[14] = MES_CODE_INVALID, \
	[15] = MES_CODE_INVALID, \
	[16] = MES_CODE_INVALID, \
	[17] = MES_CODE_INVALID, \
	[18] = MES_CODE_INVALID, \
	[19] = MES_CODE_INVALID, \
	[20] = MES_CODE_INVALID, \
	[21] = MES_CODE_INVALID, \
	[22] = MES_CODE_INVALID, \
	[23] = MES_CODE_INVALID, \
	[24] = MES_CODE_INVALID, \
	[25] = MES_CODE_INVALID, \
	[26] = MES_CODE_INVALID, \
	[27] = MES_CODE_INVALID, \
}

static struct mes_code_tables elf_classics_tables = {
	.stmt_op_to_int = CLASSICS_STMT_OP_TO_INT,
	.int_to_stmt_op = CLASSICS_INT_TO_STMT_OP,
	.expr_op_to_int = CLASSICS_EXPR_OP_TO_INT,
	.int_to_expr_op = CLASSICS_INT_TO_EXPR_OP,
	.sysvar16_to_int = CLASSICS_SYSVAR16_TO_INT,
	.int_to_sysvar16 = CLASSICS_INT_TO_SYSVAR16,
	.sysvar32_to_int = CLASSICS_SYSVAR32_TO_INT,
	.int_to_sysvar32 = CLASSICS_INT_TO_SYSVAR32,
};
// elf_classics }}}
// kakyuusei {{{

#define KAKYUUSEI_SYSVAR16_TO_INT { \
	[MES_SYS_VAR_HEAP] = 0, \
	[MES_SYS_VAR_DST_SURFACE] = 1, \
	[MES_SYS_VAR_FLAGS] = 3, \
	[MES_SYS_VAR_CURSOR_X] = 4, \
	[MES_SYS_VAR_CURSOR_Y] = 5, \
	[MES_SYS_VAR_TEXT_START_X] = 6, \
	[MES_SYS_VAR_TEXT_START_Y] = 7, \
	[MES_SYS_VAR_TEXT_END_X] = 8, \
	[MES_SYS_VAR_TEXT_END_Y] = 9, \
	[MES_SYS_VAR_TEXT_CURSOR_X] = 10, \
	[MES_SYS_VAR_TEXT_CURSOR_Y] = 11, \
	[MES_SYS_VAR_BG_COLOR] = 12, \
	[MES_SYS_VAR_FG_COLOR] = MES_CODE_INVALID, \
	[MES_SYS_VAR_DISPLAY_NUMBER_FLAGS] = 13, \
	[MES_SYS_VAR_FONT_WIDTH] = 14, \
	[MES_SYS_VAR_FONT_HEIGHT] = 15, \
	[MES_SYS_VAR_FONT_WEIGHT] = 16, \
	[MES_SYS_VAR_CHAR_SPACE] = 17, \
	[MES_SYS_VAR_LINE_SPACE] = 18, \
	[MES_SYS_VAR_CG_X] = 21, \
	[MES_SYS_VAR_CG_Y] = 22, \
	[MES_SYS_VAR_CG_W] = 23, \
	[MES_SYS_VAR_CG_H] = 24, \
	[MES_SYS_VAR_MASK_COLOR] = 25, \
	[MES_SYS_VAR_NR_MENU_ENTRIES] = 26, \
	[MES_SYS_VAR_MENU_NO] = 27, \
}

#define KAKYUUSEI_INT_TO_SYSVAR16 { \
	[0]  = MES_SYS_VAR_HEAP, \
	[1]  = MES_SYS_VAR_DST_SURFACE, \
	[2]  = MES_CODE_INVALID, \
	[3]  = MES_SYS_VAR_FLAGS, \
	[4]  = MES_SYS_VAR_CURSOR_X, \
	[5]  = MES_SYS_VAR_CURSOR_Y, \
	[6]  = MES_SYS_VAR_TEXT_START_X, \
	[7]  = MES_SYS_VAR_TEXT_START_Y, \
	[8]  = MES_SYS_VAR_TEXT_END_X, \
	[9]  = MES_SYS_VAR_TEXT_END_Y, \
	[10] = MES_SYS_VAR_TEXT_CURSOR_X, \
	[11] = MES_SYS_VAR_TEXT_CURSOR_Y, \
	[12] = MES_SYS_VAR_BG_COLOR, \
	[13] = MES_SYS_VAR_DISPLAY_NUMBER_FLAGS, \
	[14] = MES_SYS_VAR_FONT_WIDTH, \
	[15] = MES_SYS_VAR_FONT_HEIGHT, \
	[16] = MES_SYS_VAR_FONT_WEIGHT, \
	[17] = MES_SYS_VAR_CHAR_SPACE, \
	[18] = MES_SYS_VAR_LINE_SPACE, \
	[19] = MES_CODE_INVALID, \
	[20] = MES_CODE_INVALID, \
	[21] = MES_SYS_VAR_CG_X, \
	[22] = MES_SYS_VAR_CG_Y, \
	[23] = MES_SYS_VAR_CG_W, \
	[24] = MES_SYS_VAR_CG_H, \
	[25] = MES_SYS_VAR_MASK_COLOR, \
	[26] = MES_SYS_VAR_NR_MENU_ENTRIES, \
	[27] = MES_SYS_VAR_MENU_NO, \
}

#define KAKYUUSEI_SYSVAR32_TO_INT { \
	[MES_SYS_VAR_MEMORY] = MES_CODE_INVALID, \
	[MES_SYS_VAR_CG_OFFSET] = 0, \
	[MES_SYS_VAR_DATA_OFFSET] = 1, \
	[MES_SYS_VAR_MPX_OFFSET] = 2, \
	[MES_SYS_VAR_CCD_OFFSET] = 3, \
	[MES_SYS_VAR_EVE_OFFSET] = 4, \
	[MES_SYS_VAR_PALETTE] = MES_CODE_INVALID, \
	[MES_SYS_VAR_A6_OFFSET] = 5, \
	[MES_SYS_VAR_FILE_DATA] = 6, \
	[MES_SYS_VAR_MENU_ENTRY_ADDRESSES] = 7, \
	[MES_SYS_VAR_MENU_ENTRY_NUMBERS] = 8, \
	[MES_SYS_VAR_MAP_DATA] = 9, \
}

#define KAKYUUSEI_INT_TO_SYSVAR32 { \
	[0]  = MES_SYS_VAR_CG_OFFSET, \
	[1]  = MES_SYS_VAR_DATA_OFFSET, \
	[2]  = MES_SYS_VAR_MPX_OFFSET, \
	[3]  = MES_SYS_VAR_CCD_OFFSET, \
	[4]  = MES_SYS_VAR_EVE_OFFSET, \
	[5]  = MES_SYS_VAR_A6_OFFSET, \
	[6]  = MES_SYS_VAR_FILE_DATA, \
	[7]  = MES_SYS_VAR_MENU_ENTRY_ADDRESSES, \
	[8]  = MES_SYS_VAR_MENU_ENTRY_NUMBERS, \
	[9]  = MES_SYS_VAR_MAP_DATA, \
	[10] = MES_CODE_INVALID, \
	[11] = MES_CODE_INVALID, \
	[12] = MES_CODE_INVALID, \
	[13] = MES_CODE_INVALID, \
	[14] = MES_CODE_INVALID, \
	[15] = MES_CODE_INVALID, \
	[16] = MES_CODE_INVALID, \
	[17] = MES_CODE_INVALID, \
	[18] = MES_CODE_INVALID, \
	[19] = MES_CODE_INVALID, \
	[20] = MES_CODE_INVALID, \
	[21] = MES_CODE_INVALID, \
	[22] = MES_CODE_INVALID, \
	[23] = MES_CODE_INVALID, \
	[24] = MES_CODE_INVALID, \
	[25] = MES_CODE_INVALID, \
	[26] = MES_CODE_INVALID, \
	[27] = MES_CODE_INVALID, \
}

struct mes_code_tables kakyuusei_tables = {
	.stmt_op_to_int = DEFAULT_STMT_OP_TO_INT,
	.int_to_stmt_op = DEFAULT_INT_TO_STMT_OP,
	.expr_op_to_int = DEFAULT_EXPR_OP_TO_INT,
	.int_to_expr_op = DEFAULT_INT_TO_EXPR_OP,
	.sysvar16_to_int = KAKYUUSEI_SYSVAR16_TO_INT,
	.int_to_sysvar16 = KAKYUUSEI_INT_TO_SYSVAR16,
	.sysvar32_to_int = KAKYUUSEI_SYSVAR32_TO_INT,
	.int_to_sysvar32 = KAKYUUSEI_INT_TO_SYSVAR32,
	.system = &mes_sys_kakyuusei,
	.util = &mes_util_none,
};

// kakyuusei }}}
// doukyuusei 2 {{{

struct mes_code_tables doukyuusei2_tables = {
	.stmt_op_to_int = DEFAULT_STMT_OP_TO_INT,
	.int_to_stmt_op = DEFAULT_INT_TO_STMT_OP,
	.expr_op_to_int = DEFAULT_EXPR_OP_TO_INT,
	.int_to_expr_op = DEFAULT_INT_TO_EXPR_OP,
	.sysvar16_to_int = KAKYUUSEI_SYSVAR16_TO_INT,
	.int_to_sysvar16 = KAKYUUSEI_INT_TO_SYSVAR16,
	.sysvar32_to_int = KAKYUUSEI_SYSVAR32_TO_INT,
	.int_to_sysvar32 = KAKYUUSEI_INT_TO_SYSVAR32,
	.system = &mes_sys_doukyuusei2,
	.util = &mes_util_none,
};

// doukyuusei 2 }}}
// shuusaku {{{

#define SHUUSAKU_SYSVAR16_TO_INT { \
	[MES_SYS_VAR_HEAP] = MES_CODE_INVALID, \
	[MES_SYS_VAR_CURSOR_X] = MES_CODE_INVALID, \
	[MES_SYS_VAR_CURSOR_Y] = MES_CODE_INVALID, \
	[MES_SYS_VAR_FG_COLOR] = MES_CODE_INVALID, \
	[MES_SYS_VAR_DISPLAY_NUMBER_FLAGS] = MES_CODE_INVALID, \
	[MES_SYS_VAR_FONT_WIDTH] = MES_CODE_INVALID, \
	[MES_SYS_VAR_FONT_HEIGHT] = MES_CODE_INVALID, \
	[MES_SYS_VAR_FONT_WEIGHT] = MES_CODE_INVALID, \
	[MES_SYS_VAR_CHAR_SPACE] = MES_CODE_INVALID, \
	[MES_SYS_VAR_NR_MENU_ENTRIES] = MES_CODE_INVALID, \
	[MES_SYS_VAR_MENU_NO] = MES_CODE_INVALID, \
	[MES_SYS_VAR_MASK_COLOR] = MES_CODE_INVALID, \
	[MES_SYS_VAR_TEXT_CURSOR_X] = 0, \
	[MES_SYS_VAR_TEXT_CURSOR_Y] = 1, \
	[MES_SYS_VAR_TEXT_START_X] = 2, \
	[MES_SYS_VAR_TEXT_START_Y] = 3, \
	[MES_SYS_VAR_TEXT_END_X] = 4, \
	[MES_SYS_VAR_TEXT_END_Y] = 5, \
	[MES_SYS_VAR_BG_COLOR] = 7, \
	[MES_SYS_VAR_CG_X] = 8, \
	[MES_SYS_VAR_CG_Y] = 9, \
	[MES_SYS_VAR_CG_W] = 10, \
	[MES_SYS_VAR_CG_H] = 11, \
	[MES_SYS_VAR_DST_SURFACE] = 12, \
	[MES_SYS_VAR_FLAGS] = 14, \
	[MES_SYS_VAR_LINE_SPACE] = 18, \
}

#define SHUUSAKU_INT_TO_SYSVAR16 { \
	[0]  = MES_SYS_VAR_TEXT_CURSOR_X, \
	[1]  = MES_SYS_VAR_TEXT_CURSOR_Y, \
	[2]  = MES_SYS_VAR_TEXT_START_X, \
	[3]  = MES_SYS_VAR_TEXT_START_Y, \
	[4]  = MES_SYS_VAR_TEXT_END_X, \
	[5]  = MES_SYS_VAR_TEXT_END_Y, \
	[6]  = MES_CODE_INVALID, \
	[7]  = MES_SYS_VAR_BG_COLOR, \
	[8]  = MES_SYS_VAR_CG_X, \
	[9]  = MES_SYS_VAR_CG_Y, \
	[10] = MES_SYS_VAR_CG_W, \
	[11] = MES_SYS_VAR_CG_H, \
	[12] = MES_SYS_VAR_DST_SURFACE, \
	[13] = MES_CODE_INVALID, \
	[14] = MES_SYS_VAR_FLAGS, \
	[15] = MES_CODE_INVALID, \
	[16] = MES_CODE_INVALID, \
	[17] = MES_CODE_INVALID, \
	[18] = MES_SYS_VAR_LINE_SPACE, \
	[19] = MES_CODE_INVALID, \
	[20] = MES_CODE_INVALID, \
	[21] = MES_CODE_INVALID, \
	[22] = MES_CODE_INVALID, \
	[23] = MES_CODE_INVALID, \
	[24] = MES_CODE_INVALID, \
	[25] = MES_CODE_INVALID, \
	[26] = MES_CODE_INVALID, \
	[27] = MES_CODE_INVALID, \
}

struct mes_code_tables shuusaku_tables = {
	.sysvar16_to_int = SHUUSAKU_SYSVAR16_TO_INT,
	.int_to_sysvar16 = SHUUSAKU_INT_TO_SYSVAR16,
	.system = &mes_sys_shuusaku,
	.util = &mes_util_none,
};

// shuusaku }}}
// kawarazaki-ke {{{

struct mes_code_tables kawarazakike_tables = {
	.system = &mes_sys_kawarazakike,
	.util = &mes_util_none,
};

// kawarazaki-ke }}}
// {{{ nonomura byouin

#define NONOMURA_STMT_OP_TO_INT { \
	[MES_STMT_END]            = 0x00, \
	[MES_STMT_ZENKAKU]        = 0x01, \
	[MES_STMT_HANKAKU]        = 0x02, \
	[MES_STMT_SET_FLAG_CONST] = 0x03, \
	[MES_STMT_SET_VAR16]      = 0x04, \
	[MES_STMT_SET_FLAG_EXPR]  = 0x05, \
	[MES_STMT_PTR16_SET8]     = 0x06, \
	[MES_STMT_PTR16_SET16]    = 0x07, \
	[MES_STMT_PTR32_SET32]    = 0x08, \
	[MES_STMT_SET_ARG_CONST]  = 0x09, \
	[MES_STMT_SET_ARG_EXPR]   = 0x0A, \
	[MES_STMT_JZ]             = 0x0B, \
	[MES_STMT_JMP]            = 0x0C, \
	[MES_STMT_SYS]            = 0x0D, \
	[MES_STMT_JMP_MES]        = 0x0E, \
	[MES_STMT_CALL_MES]       = 0x0F, \
	[MES_STMT_DEF_MENU]       = 0x10, \
	[MES_STMT_CALL_PROC]      = 0x11, \
	[MES_STMT_UTIL]           = 0x12, \
	[MES_STMT_LINE]           = 0x13, \
	[MES_STMT_DEF_PROC]       = 0x14, \
	[MES_STMT_MENU_EXEC]      = 0x15, \
	[MES_STMT_SET_VAR32]      = 0x16, \
	[MES_STMT_17]             = 0x17, \
	[MES_STMT_18]             = 0x18, \
	[MES_STMT_19]             = 0x19, \
	[MES_STMT_1A]             = 0x1a, \
	[MES_STMT_1B]             = 0x1b, \
	[MES_STMT_DEF_SUB]        = 0x1c, \
	[MES_STMT_CALL_SUB]       = 0x1d, \
	[MES_STMT_1F]             = 0x1f, \
}

#define NONOMURA_INT_TO_STMT_OP { \
	[0x00] = MES_STMT_END, \
	[0x01] = MES_STMT_ZENKAKU, \
	[0x02] = MES_STMT_HANKAKU, \
	[0x03] = MES_STMT_SET_FLAG_CONST, \
	[0x04] = MES_STMT_SET_VAR16, \
	[0x05] = MES_STMT_SET_FLAG_EXPR, \
	[0x06] = MES_STMT_PTR16_SET8, \
	[0x07] = MES_STMT_PTR16_SET16, \
	[0x08] = MES_STMT_PTR32_SET32, \
	[0x09] = MES_STMT_SET_ARG_CONST, \
	[0x0A] = MES_STMT_SET_ARG_EXPR, \
	[0x0B] = MES_STMT_JZ, \
	[0x0C] = MES_STMT_JMP, \
	[0x0D] = MES_STMT_SYS, \
	[0x0E] = MES_STMT_JMP_MES, \
	[0x0F] = MES_STMT_CALL_MES, \
	[0x10] = MES_STMT_DEF_MENU, \
	[0x11] = MES_STMT_CALL_PROC, \
	[0x12] = MES_STMT_UTIL, \
	[0x13] = MES_STMT_LINE, \
	[0x14] = MES_STMT_DEF_PROC, \
	[0x15] = MES_STMT_MENU_EXEC, \
	[0x16] = MES_STMT_SET_VAR32, \
	[0x17] = MES_STMT_17, \
	[0x18] = MES_STMT_18, \
	[0x19] = MES_STMT_19, \
	[0x1a] = MES_STMT_1A, \
	[0x1b] = MES_STMT_1B, \
	[0x1c] = MES_STMT_DEF_SUB, \
	[0x1d] = MES_STMT_CALL_SUB, \
	[0x1f] = MES_STMT_1F, \
}

#define NONOMURA_EXPR_OP_TO_INT { \
	[MES_EXPR_IMM]            = 0x00, \
	[MES_EXPR_GET_VAR16]      = 0x80, \
	[MES_EXPR_PTR16_GET16]    = 0xA0, \
	[MES_EXPR_PTR16_GET8]     = 0xC0, \
	[MES_EXPR_PLUS]           = 0xE0, \
	[MES_EXPR_MINUS]          = 0xE1, \
	[MES_EXPR_MUL]            = 0xE2, \
	[MES_EXPR_DIV]            = 0xE3, \
	[MES_EXPR_MOD]            = 0xE4, \
	[MES_EXPR_RAND]           = 0xE5, \
	[MES_EXPR_AND]            = 0xE6, \
	[MES_EXPR_OR]             = 0xE7, \
	[MES_EXPR_BITAND]         = 0xE8, \
	[MES_EXPR_BITIOR]         = 0xE9, \
	[MES_EXPR_BITXOR]         = 0xEA, \
	[MES_EXPR_LT]             = 0xEB, \
	[MES_EXPR_GT]             = 0xEC, \
	[MES_EXPR_LTE]            = 0xED, \
	[MES_EXPR_GTE]            = 0xEE, \
	[MES_EXPR_EQ]             = 0xEF, \
	[MES_EXPR_NEQ]            = 0xF0, \
	[MES_EXPR_IMM16]          = 0xF1, \
	[MES_EXPR_IMM32]          = 0xF2, \
	[MES_EXPR_GET_FLAG_CONST] = 0xF3, \
	[MES_EXPR_GET_FLAG_EXPR]  = 0xF5, \
	[MES_EXPR_GET_ARG_CONST]  = 0xF6, \
	[MES_EXPR_GET_ARG_EXPR]   = 0xF7, \
	[MES_EXPR_GET_VAR32]      = 0xF8, \
	[MES_EXPR_PTR32_GET32]    = 0xF9, \
	[MES_EXPR_END]            = 0xFF, \
}

#define NONOMURA_INT_TO_EXPR_OP { \
	[0x80] = MES_EXPR_GET_VAR16, \
	[0xA0] = MES_EXPR_PTR16_GET16, \
	[0xC0] = MES_EXPR_PTR16_GET8, \
	[0xE0] = MES_EXPR_PLUS, \
	[0xE1] = MES_EXPR_MINUS, \
	[0xE2] = MES_EXPR_MUL, \
	[0xE3] = MES_EXPR_DIV, \
	[0xE4] = MES_EXPR_MOD, \
	[0xE5] = MES_EXPR_RAND, \
	[0xE6] = MES_EXPR_AND, \
	[0xE7] = MES_EXPR_OR, \
	[0xE8] = MES_EXPR_BITAND, \
	[0xE9] = MES_EXPR_BITIOR, \
	[0xEA] = MES_EXPR_BITXOR, \
	[0xEB] = MES_EXPR_LT, \
	[0xEC] = MES_EXPR_GT, \
	[0xED] = MES_EXPR_LTE, \
	[0xEE] = MES_EXPR_GTE, \
	[0xEF] = MES_EXPR_EQ, \
	[0xF0] = MES_EXPR_NEQ, \
	[0xF1] = MES_EXPR_IMM16, \
	[0xF2] = MES_EXPR_IMM32, \
	[0xF3] = MES_EXPR_GET_FLAG_CONST, \
	[0xF5] = MES_EXPR_GET_FLAG_EXPR, \
	[0xF6] = MES_EXPR_GET_ARG_CONST, \
	[0xF7] = MES_EXPR_GET_ARG_EXPR, \
	[0xF8] = MES_EXPR_GET_VAR32, \
	[0xF9] = MES_EXPR_PTR32_GET32, \
	[0xFF] = MES_EXPR_END, \
}

struct mes_code_tables nonomura_tables = {
	.stmt_op_to_int = NONOMURA_STMT_OP_TO_INT,
	.int_to_stmt_op = NONOMURA_INT_TO_STMT_OP,
	.expr_op_to_int = NONOMURA_EXPR_OP_TO_INT,
	.int_to_expr_op = NONOMURA_INT_TO_EXPR_OP,
	.sysvar16_to_int = DEFAULT_SYSVAR16_TO_INT,
	.int_to_sysvar16 = DEFAULT_INT_TO_SYSVAR16,
	.sysvar32_to_int = DEFAULT_SYSVAR32_TO_INT,
	.int_to_sysvar32 = DEFAULT_INT_TO_SYSVAR32,
	.system = &mes_sys_none,
	.util = &mes_util_none,
};

// }}} nonomura byouin

struct mes_code_tables mes_code_tables = {
	.stmt_op_to_int = DEFAULT_STMT_OP_TO_INT,
	.int_to_stmt_op = DEFAULT_INT_TO_STMT_OP,
	.expr_op_to_int = DEFAULT_EXPR_OP_TO_INT,
	.int_to_expr_op = DEFAULT_INT_TO_EXPR_OP,
	.sysvar16_to_int = DEFAULT_SYSVAR16_TO_INT,
	.int_to_sysvar16 = DEFAULT_INT_TO_SYSVAR16,
	.sysvar32_to_int = DEFAULT_SYSVAR32_TO_INT,
	.int_to_sysvar32 = DEFAULT_INT_TO_SYSVAR32,
	.system = &mes_sys_none,
	.util = &mes_util_none,
};

static struct mes_code_tables *get_code_tables(enum ai5_game_id id)
{
	switch (id) {
	case GAME_SHANGRLIA:
	case GAME_SHANGRLIA2:
	case GAME_YUNO:
		return &elf_classics_tables;
	case GAME_AI_SHIMAI:
		return &ai_shimai_tables;
	case GAME_ALLSTARS:
		return &allstars_tables;
	case GAME_BEYOND:
		return &beyond_tables;
	case GAME_DOUKYUUSEI2:
		return &doukyuusei2_tables;
	case GAME_KAKYUUSEI:
		return &kakyuusei_tables;
	case GAME_SHUUSAKU:
		return &shuusaku_tables;
	case GAME_KAWARAZAKIKE:
		return &kawarazakike_tables;
	case GAME_NONOMURA:
		return &nonomura_tables;
	default:
		return &default_tables;
	}
}

static mes_namespace_t get_system_namespace(enum ai5_game_id id)
{
	switch (id) {
	case GAME_ISAKU:        return &mes_sys_isaku;
	case GAME_DOUKYUUSEI:   return &mes_sys_doukyuusei;
	case GAME_DOUKYUUSEI2:  return &mes_sys_doukyuusei2;
	case GAME_KAKYUUSEI:    return &mes_sys_kakyuusei;
	case GAME_ALLSTARS:     return &mes_sys_allstars;
	case GAME_AI_SHIMAI:    return &mes_sys_ai_shimai;
	case GAME_BEYOND:       return &mes_sys_beyond;
	case GAME_SHANGRLIA:    return &mes_sys_classics;
	case GAME_YUNO:         return &mes_sys_classics;
	case GAME_SHUUSAKU:     return &mes_sys_shuusaku;
	case GAME_KAWARAZAKIKE: return &mes_sys_kawarazakike;
	default:                return &mes_sys_none;
	}
}

static mes_namespace_t get_util_namespace(enum ai5_game_id id)
{
	switch (id) {
	case GAME_ISAKU:     return &mes_util_isaku;
	case GAME_AI_SHIMAI: return &mes_util_aishimai;
	case GAME_BEYOND:    return &mes_util_beyond;
	case GAME_SHANGRLIA: return &mes_util_shangrlia;
	case GAME_YUNO:      return &mes_util_yuno;
	default:             return &mes_util_none;
	}
}

void mes_set_game(enum ai5_game_id id)
{
	memcpy(&mes_code_tables, get_code_tables(id), sizeof(mes_code_tables));
	mes_code_tables.system = get_system_namespace(id);
	mes_code_tables.util = get_util_namespace(id);
}
