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

#include <stdio.h>
#include <ctype.h>

#include "nulib.h"
#include "nulib/buffer.h"
#include "nulib/hashtable.h"
#include "nulib/string.h"
#include "nulib/utfsjis.h"
#include "nulib/vector.h"
#include "ai5/mes.h"
#include "ai5/game.h"

#define DC_WARNING(addr, fmt, ...) \
	sys_warning("WARNING: At 0x%08x: " fmt "\n", (unsigned)addr, ##__VA_ARGS__)

#define DC_ERROR(addr, fmt, ...) \
	sys_warning("ERROR: At 0x%08x: " fmt "\n", (unsigned)addr, ##__VA_ARGS__)

static struct mes_expression *stack_pop(unsigned addr, struct mes_expression **stack, int *stack_ptr)
{
	if (unlikely(*stack_ptr < 1)) {
		DC_WARNING(addr, "Stack empty in stack_pop");
		return NULL;
	}
	*stack_ptr = *stack_ptr - 1;
	return stack[*stack_ptr];
}

static struct mes_expression *_mes_parse_expression(struct buffer *mes)
{
	int stack_ptr = 0;
	struct mes_expression *stack[4096];

	for (int i = 0; ; i++) {
		struct mes_expression *expr = xcalloc(1, sizeof(struct mes_expression));
		if (unlikely(stack_ptr >= 4096)) {
			DC_ERROR(mes->index, "Expression stack overflow");
			goto error;
		}
		uint8_t b = buffer_read_u8(mes);
		switch ((expr->op = mes_opcode_to_expr(b))) {
		case MES_EXPR_GET_VAR16:
			expr->arg8 = buffer_read_u8(mes);
			break;
		case MES_EXPR_PTR16_GET16:
			expr->arg8 = buffer_read_u8(mes);
			if (!(expr->sub_a = stack_pop(mes->index-1, stack, &stack_ptr)))
				goto error;
			break;
		case MES_EXPR_PTR16_GET8:
			expr->arg8 = buffer_read_u8(mes);
			if (!(expr->sub_a = stack_pop(mes->index-1, stack, &stack_ptr)))
				goto error;
			break;
		case MES_EXPR_PLUS:
		case MES_EXPR_MINUS:
		case MES_EXPR_MUL:
		case MES_EXPR_DIV:
		case MES_EXPR_MOD:
		case MES_EXPR_AND:
		case MES_EXPR_OR:
		case MES_EXPR_BITAND:
		case MES_EXPR_BITIOR:
		case MES_EXPR_BITXOR:
		case MES_EXPR_LT:
		case MES_EXPR_GT:
		case MES_EXPR_LTE:
		case MES_EXPR_GTE:
		case MES_EXPR_EQ:
		case MES_EXPR_NEQ:
			if (!(expr->sub_a = stack_pop(mes->index-1, stack, &stack_ptr)))
				goto error;
			if (!(expr->sub_b = stack_pop(mes->index-1, stack, &stack_ptr)))
				goto error;
			break;
		case MES_EXPR_RAND:
			switch (ai5_target_game) {
			case GAME_DOUKYUUSEI:
			case GAME_DOUKYUUSEI2:
			case GAME_KAKYUUSEI:
				expr->sub_a = xcalloc(1, sizeof(struct mes_expression));
				expr->sub_a->op = MES_EXPR_IMM16;
				expr->sub_a->arg16 = buffer_read_u16(mes);
				break;
			default:
				if (!(expr->sub_a = stack_pop(mes->index-1, stack, &stack_ptr)))
					goto error;
				break;
			}
			break;
		case MES_EXPR_IMM16:
			expr->arg16 = buffer_read_u16(mes);
			break;
		case MES_EXPR_IMM32:
			expr->arg32 = buffer_read_u32(mes);
			break;
		case MES_EXPR_GET_FLAG_CONST:
		case MES_EXPR_GET_ARG_CONST:
			expr->arg16 = buffer_read_u16(mes);
			break;
		case MES_EXPR_GET_FLAG_EXPR:
		case MES_EXPR_GET_ARG_EXPR:
			if (!(expr->sub_a = stack_pop(mes->index-1, stack, &stack_ptr)))
				goto error;
			break;
		case MES_EXPR_PTR32_GET32:
		case MES_EXPR_PTR32_GET16:
		case MES_EXPR_PTR32_GET8:
			expr->arg8 = buffer_read_u8(mes);
			if (!(expr->sub_a = stack_pop(mes->index-1, stack, &stack_ptr)))
				goto error;
			break;
		case MES_EXPR_GET_VAR32:
			expr->arg8 = buffer_read_u8(mes);
			break;
		case MES_EXPR_END:
			if (unlikely(stack_ptr != 1)) {
				if (stack_ptr == 0) {
					// XXX: allstars NAMESELECT.MES triggers this
					DC_WARNING(mes->index-1, "Empty expression");
					expr->op = MES_EXPR_IMM;
					expr->arg8 = 0;
					return expr;
				}
				DC_ERROR(mes->index-1, "Invalid stack size at END expression");
				goto error;
			}
			free(expr);
			return stack[0];
		case MES_EXPR_IMM:
		default:
			expr->arg8 = b;
			break;
		}
		stack[stack_ptr++] = expr;
		continue;
error:
		if (expr->sub_a)
			mes_expression_free(expr->sub_a);
		if (expr->sub_b)
			mes_expression_free(expr->sub_b);
		free(expr);
		for (int i = 0; i < stack_ptr; i++) {
			mes_expression_free(stack[i]);
		}
		return NULL;
	}
}

struct mes_expression *mes_parse_expression(uint8_t *data, size_t data_size)
{
	struct buffer buf;
	buffer_init(&buf, data, data_size);
	return _mes_parse_expression(&buf);
}

static bool mes_parse_expression_list(struct buffer *mes, mes_expression_list *exprs)
{
	vector_init(*exprs);

	do {
		struct mes_expression *expr = _mes_parse_expression(mes);
		if (!expr)
			goto error;
		vector_push(struct mes_expression*, *exprs, expr);
	} while (buffer_read_u8(mes));
	return true;
error: ;
	struct mes_expression *expr;
	vector_foreach(expr, *exprs) {
		mes_expression_free(expr);
	}
	vector_destroy(*exprs);
	vector_init(*exprs);
	return false;
}

static bool mes_parse_string_param(struct buffer *mes, struct mes_parameter *param)
{
	// XXX: Actual max size of string parameter is 24, but the VM doesn't
	//      bounds check and the limit is exceeded in some cases (e.g.
	//      Doukyuusei/NAME.MES).
	char str[64];
	size_t str_i = 0;
	bool warned_overflow = false;

	uint8_t c;
	for (str_i = 0; (c = buffer_read_u8(mes)); str_i++) {
		if (unlikely(str_i > 61)) {
			DC_ERROR(mes->index, "string parameter overflowed parse buffer");
			return false;
		}
		if (unlikely(str_i > 22 && !warned_overflow)) {
			DC_WARNING(mes->index, "string parameter would overflow VM buffer");
			warned_overflow = true;
		}
		switch (c) {
		case '\\':
			str[str_i++] = '\\';
			str[str_i] = '\\';
			break;
		case '\n':
			str[str_i++] = '\\';
			str[str_i] = 'n';
			break;
		case '\t':
			str[str_i++] = '\\';
			str[str_i] = 't';
			break;
		default:
			// TODO: \x
			str[str_i] = c;
			if (SJIS_2BYTE(c)) {
				if (!(c = buffer_read_u8(mes))) {
					DC_WARNING(mes->index, "string parameter truncated");
					mes->index--;
					break;
				}
				str[++str_i] = c;
			}
			break;
		}
	}
	str[str_i] = '\0';
	param->str = gbk_cstring_to_utf8(str, str_i);
	return true;
}

static bool mes_parse_parameter_list(struct buffer *mes, mes_parameter_list *params)
{
	vector_init(*params);

	uint8_t b;
	for (int i = 0; (b = buffer_read_u8(mes)); i++) {
		vector_push(struct mes_parameter, *params, (struct mes_parameter){.type=b});
		if (b == MES_PARAM_STRING) {
			if (!mes_parse_string_param(mes, &vector_A(*params, i)))
				goto error;
		} else if (b == MES_PARAM_EXPRESSION) {
			struct mes_expression *expr = _mes_parse_expression(mes);
			if (!expr)
				goto error;
			vector_A(*params, i).expr = expr;
		} else {
			DC_ERROR(mes->index-1, "Unhandled parameter type: 0x%02x", (unsigned)b);
			goto error;
		}
	}
	return true;
error: ;
	struct mes_parameter *p;
	vector_foreach_p(p, *params) {
		if (p->type == MES_PARAM_EXPRESSION && p->expr)
			mes_expression_free(p->expr);
	}
	vector_destroy(*params);
	vector_init(*params);
	return false;
}

static bool in_range(int v, int low, int high)
{
	return v >= low && v <= high;
}

bool mes_char_is_hankaku(uint8_t b)
{
	return !in_range(b, 0x81, 0xfe);
}

bool mes_char_is_zenkaku(uint8_t b)
{
	return in_range(b, 0x81, 0xfe);
}

#define TXT_BUF_SIZE 4096

static string mes_parse_txt(struct buffer *mes, bool *terminated)
{
	// FIXME: don't use fixed-size buffer
	size_t str_i = 0;
	char str[TXT_BUF_SIZE];

	uint8_t c;
	while ((c = buffer_peek_u8(mes))) {
		if (unlikely(str_i >= (TXT_BUF_SIZE - 7))) {
			DC_ERROR(mes->index, "TXT buffer overflow");
			return NULL;
		}
		if (unlikely(!mes_char_is_zenkaku(c))) {
			DC_WARNING(mes->index, "Invalid byte in TXT statement: %02x", (unsigned)c);
			*terminated = false;
			goto unterminated;
		}
		if (sjis_char_is_valid(buffer_strdata(mes))) {
			str[str_i++] = buffer_read_u8(mes);
			str[str_i++] = buffer_read_u8(mes);
		} else {
			uint8_t b1 = buffer_read_u8(mes);
			uint8_t b2 = buffer_read_u8(mes);
			str_i += sprintf(str+str_i, "\\X%02x%02x", b1, b2);
		}
	}
	buffer_read_u8(mes);
	*terminated = true;
unterminated:
	str[str_i] = 0;
	return gbk_cstring_to_utf8(str, str_i);
}

static string mes_parse_str(struct buffer *mes, bool *terminated)
{
	// FIXME: don't use fixed-size buffer
	size_t str_i = 0;
	char str[TXT_BUF_SIZE];

	uint8_t c;
	while ((c = buffer_peek_u8(mes))) {
		if (unlikely(str_i >= (TXT_BUF_SIZE - 5))) {
			DC_ERROR(mes->index, "STR buffer overflow");
			return NULL;
		}
		if (unlikely(!mes_char_is_hankaku(c))) {
			DC_WARNING(mes->index, "Invalid byte in STR statement: %02x", (unsigned)c);
			*terminated = false;
			goto unterminated;
		}
		switch (c) {
		case '\n':
			str[str_i++] = '\\';
			str[str_i++] = 'n';
			break;
		case '\t':
			str[str_i++] = '\\';
			str[str_i++] = 't';
			break;
		case '$':
			str[str_i++] = '\\';
			str[str_i++] = '$';
			break;
		case '\\':
			str[str_i++] = '\\';
			str[str_i++] = '\\';
			break;
		default:
			if (c > 0x7f || !isprint(c)) {
				str_i += sprintf(str+str_i, "\\x%02x", c);
			} else {
				str[str_i++] = c;
			}
		}
		buffer_read_u8(mes);
	}
	buffer_read_u8(mes);
	*terminated = true;
unterminated:
	str[str_i] = 0;
	return gbk_cstring_to_utf8(str, str_i);
}

static struct mes_statement *_mes_parse_statement(struct buffer *mes)
{
	struct mes_statement *stmt = xcalloc(1, sizeof(struct mes_statement));
	stmt->address = mes->index;
	uint8_t b = buffer_read_u8(mes);
	switch ((stmt->op = mes_opcode_to_stmt(b))) {
	case MES_STMT_END:
		break;
	case MES_STMT_ZENKAKU:
		if (!(stmt->TXT.text = mes_parse_txt(mes, &stmt->TXT.terminated)))
			goto error;
		break;
	case MES_STMT_HANKAKU:
		if (!(stmt->TXT.text = mes_parse_str(mes, &stmt->TXT.terminated)))
			goto error;
		break;
	case MES_STMT_SET_FLAG_CONST:
	case MES_STMT_SET_ARG_CONST:
		stmt->SET_VAR_CONST.var_no = buffer_read_u16(mes);
		if (!mes_parse_expression_list(mes, &stmt->SET_VAR_CONST.val_exprs))
			goto error;
		break;
	case MES_STMT_SET_VAR16:
	case MES_STMT_SET_VAR32:
		stmt->SET_VAR_CONST.var_no = buffer_read_u8(mes);
		if (!mes_parse_expression_list(mes, &stmt->SET_VAR_CONST.val_exprs))
			goto error;
		break;
	case MES_STMT_SET_FLAG_EXPR:
	case MES_STMT_SET_ARG_EXPR:
		if (!(stmt->SET_VAR_EXPR.var_expr = _mes_parse_expression(mes)))
			goto error;
		if (!mes_parse_expression_list(mes, &stmt->SET_VAR_EXPR.val_exprs)) {
			mes_expression_free(stmt->SET_VAR_EXPR.var_expr);
			goto error;
		}
		break;
	case MES_STMT_PTR16_SET8:
	case MES_STMT_PTR16_SET16:
	case MES_STMT_PTR32_SET8:
	case MES_STMT_PTR32_SET16:
	case MES_STMT_PTR32_SET32:
		if (!(stmt->PTR_SET.off_expr = _mes_parse_expression(mes)))
			goto error;
		stmt->PTR_SET.var_no = buffer_read_u8(mes);
		if (!mes_parse_expression_list(mes, &stmt->PTR_SET.val_exprs)) {
			mes_expression_free(stmt->PTR_SET.off_expr);
			goto error;
		}
		break;
	case MES_STMT_JZ:
		if (!(stmt->JZ.expr = _mes_parse_expression(mes)))
			goto error;
		stmt->JZ.addr = buffer_read_u32(mes);
		break;
	case MES_STMT_JMP:
	case MES_STMT_17:
	case MES_STMT_1F:
		stmt->JMP.addr = buffer_read_u32(mes);
		break;
	case MES_STMT_SYS:
		if (!(stmt->SYS.expr = _mes_parse_expression(mes)))
			goto error;
		if (!mes_parse_parameter_list(mes, &stmt->SYS.params)) {
			mes_expression_free(stmt->SYS.expr);
			goto error;
		}
		break;
	case MES_STMT_JMP_MES:
	case MES_STMT_CALL_MES:
	case MES_STMT_CALL_PROC:
	case MES_STMT_UTIL:
	case MES_STMT_CALL_SUB:
	case MES_STMT_1B:
		if (!mes_parse_parameter_list(mes, &stmt->CALL.params))
			goto error;
		break;
	case MES_STMT_DEF_MENU:
		if (!mes_parse_parameter_list(mes, &stmt->DEF_MENU.params))
			goto error;
		stmt->DEF_MENU.skip_addr = buffer_read_u32(mes);
		break;
	case MES_STMT_LINE:
		stmt->LINE.arg = buffer_read_u8(mes);
		break;
	case MES_STMT_DEF_PROC:
	case MES_STMT_DEF_SUB:
		if (!(stmt->DEF_PROC.no_expr = _mes_parse_expression(mes)))
			goto error;
		stmt->DEF_PROC.skip_addr = buffer_read_u32(mes);
		break;
	case MES_STMT_MENU_EXEC:
		if (ai5_target_game == GAME_NONOMURA) {
			if (!mes_parse_parameter_list(mes, &stmt->DEF_MENU.params))
				goto error;
		}
		break;
	case MES_STMT_18:
		if (!(stmt->SET_VAR_EXPR.var_expr = _mes_parse_expression(mes)))
			goto error;
		break;
	case MES_STMT_19:
	case MES_STMT_1A:
		break;
	default:
		mes->index--;
		DC_WARNING(mes->index, "Unprefixed text: 0x%02x (possibly unhandled statement)", b);
		ERROR("Unprefixed text: 0x%02x (possibly unhandled statement)", b);
		if (mes_char_is_hankaku(buffer_peek_u8(mes))) {
			stmt->op = MES_STMT_HANKAKU;
			if (!(stmt->TXT.text = mes_parse_str(mes, &stmt->TXT.terminated)))
				goto error;
			stmt->TXT.unprefixed = true;
		} else {
			stmt->op = MES_STMT_ZENKAKU;
			if (!(stmt->TXT.text = mes_parse_txt(mes, &stmt->TXT.terminated)))
				goto error;
			stmt->TXT.unprefixed = true;
		}
	}
	stmt->next_address = mes->index;
	return stmt;
error:
	free(stmt);
	return NULL;
}

struct mes_statement *aiw_mes_parse_statement(struct buffer *mes);

struct mes_statement *mes_parse_statement(uint8_t *data, size_t data_size)
{
	struct buffer buf;
	buffer_init(&buf, data, data_size);
	if (game_is_aiwin())
		return aiw_mes_parse_statement(&buf);
	return _mes_parse_statement(&buf);
}

declare_hashtable_int_type(addr_table, struct mes_statement*);
define_hashtable_int(addr_table, struct mes_statement*);

static void tag_jump_targets(mes_statement_list statements)
{
	hashtable_t(addr_table) table = hashtable_initializer(addr_table);

	struct mes_statement *p;
	vector_foreach(p, statements) {
		int ret;
		hashtable_iter_t k = hashtable_put(addr_table, &table, p->address, &ret);
		if (unlikely(ret == HASHTABLE_KEY_PRESENT))
			ERROR("multiple statements with same address");
		hashtable_val(&table, k) = p;
	}

	vector_foreach(p, statements) {
		hashtable_iter_t k;
		switch (p->op) {
		case MES_STMT_JZ:
			k = hashtable_get(addr_table, &table, p->JZ.addr);
			if (unlikely(k == hashtable_end(&table)))
				ERROR("invalid address in JZ statement");
			kh_value(&table, k)->is_jump_target = true;
			break;
		case MES_STMT_JMP:
			k = hashtable_get(addr_table, &table, p->JMP.addr);
			if (unlikely(k == hashtable_end(&table)))
				ERROR("invalid address in JMP statement");
			kh_value(&table, k)->is_jump_target = true;
			break;
		case MES_STMT_DEF_MENU:
			k = hashtable_get(addr_table, &table, p->DEF_MENU.skip_addr);
			if (unlikely(k == hashtable_end(&table)))
				ERROR("invalid address in DEF_MENU statement");
			kh_value(&table, k)->is_jump_target = true;
			break;
		case MES_STMT_DEF_PROC:
		case MES_STMT_DEF_SUB:
			k = hashtable_get(addr_table, &table, p->DEF_PROC.skip_addr);
			if (unlikely(k == hashtable_end(&table)))
				ERROR("invalid address in DEF_PROC statement");
			kh_value(&table, k)->is_jump_target = true;
			break;
		default:
			break;
		}
	}

	hashtable_destroy(addr_table, &table);
}

bool aiw_mes_parse_statements(uint8_t *data, size_t data_size, mes_statement_list *statements);

bool mes_parse_statements(uint8_t *data, size_t data_size, mes_statement_list *statements)
{
	if (game_is_aiwin())
		return aiw_mes_parse_statements(data, data_size, statements);

	// TODO: read this?
	if (ai5_target_game == GAME_NONOMURA) {
		// address table
		if (data_size < 4)
			return false;
		uint32_t table_size = 4 + le_get32(data, 0) * 4;
		if (data_size < table_size)
			return false;
		data += table_size;
		data_size -= table_size;
	}

	struct buffer mes;
	buffer_init(&mes, data, data_size);

	while (!buffer_end(&mes)) {
		struct mes_statement *stmt = _mes_parse_statement(&mes);
		if (!stmt)
			goto error;
		vector_push(struct mes_statement*, *statements, stmt);
	}

	tag_jump_targets(*statements);
	return true;
error: ;
	struct mes_statement *stmt;
	vector_foreach(stmt, *statements) {
		mes_statement_free(stmt);
	}
	vector_destroy(*statements);
	vector_init(*statements);
	return false;
}

// parse }}}

void mes_expression_free(struct mes_expression *expr)
{
	if (!expr)
		return;
	mes_expression_free(expr->sub_a);
	mes_expression_free(expr->sub_b);
	free(expr);
}

void mes_expression_list_free(mes_expression_list list)
{
	for (unsigned i = 0; i < vector_length(list); i++) {
		mes_expression_free(vector_A(list, i));
	}
	vector_destroy(list);
}

void mes_parameter_list_free(mes_parameter_list list)
{
	for (unsigned i = 0; i < vector_length(list); i++) {
		if (vector_A(list, i).type == MES_PARAM_EXPRESSION)
			mes_expression_free(vector_A(list, i).expr);
		else
			string_free(vector_A(list, i).str);
	}
	vector_destroy(list);
}

void aiw_mes_statement_free(struct mes_statement *stmt);

void mes_statement_free(struct mes_statement *stmt)
{
	if (game_is_aiwin()) {
		aiw_mes_statement_free(stmt);
		return;
	}

	switch (stmt->op) {
	case MES_STMT_ZENKAKU:
	case MES_STMT_HANKAKU:
		string_free(stmt->TXT.text);
		break;
	case MES_STMT_SET_FLAG_CONST:
	case MES_STMT_SET_VAR16:
	case MES_STMT_SET_VAR32:
	case MES_STMT_SET_ARG_CONST:
		mes_expression_list_free(stmt->SET_VAR_CONST.val_exprs);
		break;
	case MES_STMT_SET_FLAG_EXPR:
	case MES_STMT_SET_ARG_EXPR:
		mes_expression_free(stmt->SET_VAR_EXPR.var_expr);
		mes_expression_list_free(stmt->SET_VAR_EXPR.val_exprs);
		break;
	case MES_STMT_PTR16_SET8:
	case MES_STMT_PTR16_SET16:
	case MES_STMT_PTR32_SET32:
	case MES_STMT_PTR32_SET16:
	case MES_STMT_PTR32_SET8:
		mes_expression_free(stmt->PTR_SET.off_expr);
		mes_expression_list_free(stmt->PTR_SET.val_exprs);
		break;
	case MES_STMT_JZ:
		mes_expression_free(stmt->JZ.expr);
		break;
	case MES_STMT_SYS:
		mes_expression_free(stmt->SYS.expr);
		mes_parameter_list_free(stmt->SYS.params);
		break;
	case MES_STMT_JMP_MES:
	case MES_STMT_CALL_MES:
	case MES_STMT_CALL_PROC:
	case MES_STMT_UTIL:
	case MES_STMT_CALL_SUB:
	case MES_STMT_1B:
		mes_parameter_list_free(stmt->CALL.params);
		break;
	case MES_STMT_DEF_MENU:
		mes_parameter_list_free(stmt->DEF_MENU.params);
		break;
	case MES_STMT_DEF_PROC:
	case MES_STMT_DEF_SUB:
		mes_expression_free(stmt->DEF_PROC.no_expr);
		break;
	case MES_STMT_END:
	case MES_STMT_JMP:
	case MES_STMT_LINE:
	case MES_STMT_17:
	case MES_STMT_19:
	case MES_STMT_1A:
	case MES_STMT_1F:
		break;
	case MES_STMT_MENU_EXEC:
		if (ai5_target_game == GAME_NONOMURA)
			mes_parameter_list_free(stmt->DEF_MENU.params);
		break;
	case MES_STMT_18:
		mes_expression_free(stmt->SET_VAR_EXPR.var_expr);
	}
	free(stmt);
}

void mes_statement_list_free(mes_statement_list list)
{
	for (unsigned i = 0; i < vector_length(list); i++) {
		mes_statement_free(vector_A(list, i));
	}
	vector_destroy(list);
}

void mes_qname_free(mes_qname name)
{
	struct mes_qname_part *p;
	vector_foreach_p(p, name) {
		if (p->type == MES_QNAME_IDENT) {
			string_free(p->ident);
		}
	}
	vector_destroy(name);
}
