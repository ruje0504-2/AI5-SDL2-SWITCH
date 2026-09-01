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

static enum aiw_mes_expression_op aiw_mes_opcode_to_expr(uint8_t op)
{
	if (op < 0x80)
		return AIW_MES_EXPR_IMM;
	if (op < 0xa0)
		return AIW_MES_EXPR_VAR32;
	if (op < 0xe0)
		return AIW_MES_EXPR_PTR_GET8;
	return op;
}

static struct mes_expression *_aiw_mes_parse_expression(struct buffer *mes)
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
		switch ((expr->aiw_op = aiw_mes_opcode_to_expr(b))) {
		case AIW_MES_EXPR_IMM:
			expr->arg8 = b;
			break;
		case AIW_MES_EXPR_VAR32:
			expr->arg8 = b - 0x80;
			break;
		case AIW_MES_EXPR_PTR_GET8:
			expr->arg8 = b - 0xa0;
			if (!(expr->sub_a = stack_pop(mes->index - 1, stack, &stack_ptr)))
				goto error;
			break;
		case AIW_MES_EXPR_PLUS:
		case AIW_MES_EXPR_MINUS:
		case AIW_MES_EXPR_MUL:
		case AIW_MES_EXPR_DIV:
		case AIW_MES_EXPR_MOD:
		case AIW_MES_EXPR_AND:
		case AIW_MES_EXPR_OR:
		case AIW_MES_EXPR_BITAND:
		case AIW_MES_EXPR_BITIOR:
		case AIW_MES_EXPR_BITXOR:
		case AIW_MES_EXPR_LT:
		case AIW_MES_EXPR_GT:
		case AIW_MES_EXPR_LTE:
		case AIW_MES_EXPR_GTE:
		case AIW_MES_EXPR_EQ:
		case AIW_MES_EXPR_NEQ:
			if (!(expr->sub_a = stack_pop(mes->index-1, stack, &stack_ptr)))
				goto error;
			if (!(expr->sub_b = stack_pop(mes->index-1, stack, &stack_ptr)))
				goto error;
			break;
		case AIW_MES_EXPR_RAND:
		case AIW_MES_EXPR_IMM16:
		case AIW_MES_EXPR_GET_FLAG_CONST:
		case AIW_MES_EXPR_GET_VAR16_CONST:
		case AIW_MES_EXPR_GET_SYSVAR_CONST:
			expr->arg16 = buffer_read_u16(mes);
			break;
		case AIW_MES_EXPR_IMM32:
			expr->arg32 = buffer_read_u32(mes);
			break;
		case AIW_MES_EXPR_GET_FLAG_EXPR:
		case AIW_MES_EXPR_GET_VAR16_EXPR:
		case AIW_MES_EXPR_GET_SYSVAR_EXPR:
			if (!(expr->sub_a = stack_pop(mes->index-1, stack, &stack_ptr)))
				goto error;
			break;
		case AIW_MES_EXPR_END:
			if (unlikely(stack_ptr != 1)) {
				DC_ERROR(mes->index-1, "Invalid stack size at END expression");
				goto error;
			}
			free(expr);
			return stack[0];
		default:
			DC_ERROR(mes->index-1, "Unexpected opcode: %02x", b);
			goto error;
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

static bool aiw_mes_parse_expression_list(struct buffer *mes, mes_expression_list *exprs)
{
	vector_init(*exprs);

	uint8_t b;
	// XXX: Kawarazaki-ke terminates on both 0xff and 0xfe
	//      Shuusaku/Kisaku on 0xff only
	uint8_t mask = 0xff;
	if (ai5_target_game == GAME_KAWARAZAKIKE)
		mask = 0xfe;
	do {
		struct mes_expression *expr = _aiw_mes_parse_expression(mes);
		if (!expr)
			goto error;
		vector_push(struct mes_expression*, *exprs, expr);
		b = buffer_peek_u8(mes);
	} while ((b & mask) != mask);

	buffer_read_u8(mes);
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

static bool mes_parse_string_param(struct buffer *mes, struct mes_parameter *param, uint8_t term)
{
	char str[64];
	size_t str_i = 0;
	bool warned_overflow = false;

	uint8_t c;
	for (str_i = 0; ((c = buffer_read_u8(mes)) != term); str_i++) {
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

static bool aiw_mes_parse_parameter_list(struct buffer *mes, mes_parameter_list *params)
{
	vector_init(*params);

	// empty parameter list
	if (buffer_peek_u8(mes) == 0xff) {
		buffer_read_u8(mes);
		return true;
	}

	uint8_t b;
	uint8_t mask = 0xff;
	if (ai5_target_game == GAME_KAWARAZAKIKE)
		mask = 0xfe;
	for (int i = 0; ((b = buffer_read_u8(mes)) & mask) != mask; i++) {
		if (i > 15) {
			DC_ERROR(mes->index, "Too many parameters");
			goto error;
		}
		vector_push(struct mes_parameter, *params, (struct mes_parameter){0});
		struct mes_parameter *p = &vector_A(*params, i);
		if (b == 0xf5) {
			// string param
			p->type = MES_PARAM_STRING;
			if (!mes_parse_string_param(mes, p, 0xff))
				goto error;
		} else {
			// expression param
			mes->index--;
			struct mes_expression *expr = _aiw_mes_parse_expression(mes);
			if (!expr)
				goto error;
			p->type = MES_PARAM_EXPRESSION;
			p->expr = expr;
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

static enum aiw_mes_statement_op aiw_mes_opcode_to_stmt(uint8_t b)
{
	return b;
}

static string aiw_mes_parse_text(struct buffer *mes)
{
	uint8_t term = 0xff;
	if (ai5_target_game == GAME_KAWARAZAKIKE || ai5_target_game == GAME_KISAKU)
		term = 0;
	size_t rem = buffer_remaining(mes);
	uint8_t *str = (uint8_t*)buffer_strdata(mes);
	size_t len = 0;
	for (; len < rem && str[len] != term; len++)
		/* nothing */;
	if (len == rem)
		return NULL;
	string s = gbk_cstring_to_utf8((char*)str, len);
	buffer_skip(mes, len + 1);
	return s;
}

struct table_entry {
	uint32_t cond_addr;
	uint32_t body_addr;
};

static bool aiw_mes_parse_statements_until_end(struct buffer *mes, mes_statement_list *statements);

static bool parse_defmenu(struct buffer *mes, struct mes_statement *stmt)
{
	struct table_entry *entries = NULL;
	stmt->AIW_DEF_MENU.cases = (aiw_menu_table){0};

	if (!(stmt->AIW_DEF_MENU.expr = _aiw_mes_parse_expression(mes)))
		goto error;
	stmt->AIW_DEF_MENU.table_addr = buffer_read_u32(mes);
	if (stmt->AIW_DEF_MENU.table_addr >= mes->size)
		goto error;

	buffer_seek(mes, stmt->AIW_DEF_MENU.table_addr);

	// read menu entries into temporary table
	unsigned n = buffer_read_u8(mes);
	entries = xcalloc(n, sizeof(struct table_entry));
	for (unsigned i = 0; i < n; i++) {
		entries[i].cond_addr = buffer_read_u32(mes);
		entries[i].body_addr = buffer_read_u32(mes);
	}

	// we will continue parsing at this index later
	stmt->AIW_DEF_MENU.skip_addr = mes->index;

	// parse conditional expressions and case bodies
	for (unsigned i = 0; i < n; i++) {
		struct aiw_mes_menu_case c = {0};
		if (entries[i].cond_addr) {
			if (entries[i].cond_addr >= mes->size)
				goto error;
			buffer_seek(mes, entries[i].cond_addr);
			if (!(c.cond = _aiw_mes_parse_expression(mes)))
				goto error;
		}

		if (entries[i].body_addr >= mes->size)
			goto error;
		buffer_seek(mes, entries[i].body_addr);
		if (!aiw_mes_parse_statements_until_end(mes, &c.body))
			goto error;
		vector_push(struct aiw_mes_menu_case, stmt->AIW_DEF_MENU.cases, c);
	}

	// continue parsing at end of table
	buffer_seek(mes, stmt->AIW_DEF_MENU.skip_addr);
	free(entries);
	return true;
error:
	free(entries);
	mes_expression_free(stmt->AIW_DEF_MENU.expr);
	struct aiw_mes_menu_case *c;
	vector_foreach_p(c, stmt->AIW_DEF_MENU.cases) {
		mes_expression_free(c->cond);
		mes_statement_list_free(c->body);
	}
	vector_destroy(stmt->AIW_DEF_MENU.cases);
	return false;
}

struct mes_statement *aiw_mes_parse_statement(struct buffer *mes)
{
	struct mes_expression *tmp_expr;
	struct mes_statement *stmt = xcalloc(1, sizeof(struct mes_statement));
	stmt->address = mes->index;
	uint8_t b = buffer_read_u8(mes);
	switch ((stmt->aiw_op = aiw_mes_opcode_to_stmt(b))) {
	case AIW_MES_STMT_FE:
	case AIW_MES_STMT_END:
		break;
	case AIW_MES_STMT_TXT:
		if (!(stmt->TXT.text = aiw_mes_parse_text(mes)))
			goto error;
		stmt->TXT.terminated = true;
		break;
	case AIW_MES_STMT_JMP:
		stmt->JMP.addr = buffer_read_u32(mes);
		break;
	case AIW_MES_STMT_SET_FLAG_CONST:
	case AIW_MES_STMT_SET_VAR16_CONST:
	case AIW_MES_STMT_SET_SYSVAR_CONST:
		stmt->SET_VAR_CONST.var_no = buffer_read_u16(mes);
		if (!aiw_mes_parse_expression_list(mes, &stmt->SET_VAR_CONST.val_exprs))
			goto error;
		break;
	case AIW_MES_STMT_SET_FLAG_EXPR:
	case AIW_MES_STMT_SET_VAR16_EXPR:
	case AIW_MES_STMT_SET_SYSVAR_EXPR:
		if (!(stmt->SET_VAR_EXPR.var_expr = _aiw_mes_parse_expression(mes)))
			goto error;
		if (!aiw_mes_parse_expression_list(mes, &stmt->SET_VAR_EXPR.val_exprs)) {
			mes_expression_free(stmt->SET_VAR_EXPR.var_expr);
			goto error;
		}
		break;
	case AIW_MES_STMT_SET_VAR32:
		stmt->SET_VAR_CONST.var_no = buffer_read_u8(mes);
		vector_init(stmt->SET_VAR_CONST.val_exprs);
		if (!(tmp_expr = _aiw_mes_parse_expression(mes)))
			goto error;
		vector_push(struct mes_expression*, stmt->SET_VAR_CONST.val_exprs, tmp_expr);
		break;
	case AIW_MES_STMT_PTR_SET8:
	case AIW_MES_STMT_PTR_SET16:
		stmt->PTR_SET.var_no = buffer_read_u8(mes);
		if (!(stmt->PTR_SET.off_expr = _aiw_mes_parse_expression(mes)))
			goto error;
		if (!aiw_mes_parse_expression_list(mes, &stmt->PTR_SET.val_exprs)) {
			mes_expression_free(stmt->PTR_SET.off_expr);
			goto error;
		}
		break;
	case AIW_MES_STMT_JZ:
		if (!(stmt->JZ.expr = _aiw_mes_parse_expression(mes)))
			goto error;
		stmt->JZ.addr = buffer_read_u32(mes);
		break;
	case AIW_MES_STMT_UTIL:
	case AIW_MES_STMT_JMP_MES:
	case AIW_MES_STMT_CALL_MES:
	case AIW_MES_STMT_LOAD:
	case AIW_MES_STMT_SAVE:
	case AIW_MES_STMT_CALL_PROC:
	case AIW_MES_STMT_NUM:
	case AIW_MES_STMT_SET_TEXT_COLOR:
	case AIW_MES_STMT_WAIT:
	case AIW_MES_STMT_LOAD_IMAGE:
	case AIW_MES_STMT_SURF_COPY:
	case AIW_MES_STMT_SURF_COPY_MASKED:
	case AIW_MES_STMT_SURF_SWAP:
	case AIW_MES_STMT_SURF_FILL:
	case AIW_MES_STMT_SURF_INVERT:
	case AIW_MES_STMT_29:
	case AIW_MES_STMT_SHOW_HIDE:
	case AIW_MES_STMT_CROSSFADE:
	case AIW_MES_STMT_CROSSFADE2:
	case AIW_MES_STMT_CURSOR:
	case AIW_MES_STMT_ANIM:
	case AIW_MES_STMT_LOAD_AUDIO:
	case AIW_MES_STMT_LOAD_EFFECT:
	case AIW_MES_STMT_LOAD_VOICE:
	case AIW_MES_STMT_AUDIO:
	case AIW_MES_STMT_PLAY_MOVIE:
	case AIW_MES_STMT_34:
		if (!aiw_mes_parse_parameter_list(mes, &stmt->CALL.params))
			goto error;
		break;
	case AIW_MES_STMT_DEF_PROC:
		if (!(stmt->DEF_PROC.no_expr = _aiw_mes_parse_expression(mes)))
			goto error;
		stmt->DEF_PROC.skip_addr = buffer_read_u32(mes);
		break;
	case AIW_MES_STMT_DEF_MENU:
		if (!parse_defmenu(mes, stmt))
			goto error;
		break;
	case AIW_MES_STMT_MENU_EXEC:
		if (!aiw_mes_parse_expression_list(mes, &stmt->AIW_MENU_EXEC.exprs))
			goto error;
		break;
	case AIW_MES_STMT_21:
		break;
	case AIW_MES_STMT_COMMIT_MESSAGE:
		if (ai5_target_game == GAME_KAWARAZAKIKE) {
			if (!aiw_mes_parse_parameter_list(mes, &stmt->CALL.params))
				goto error;
		} else {
			stmt->CALL.params = (mes_parameter_list)vector_initializer;
		}
		break;
	case AIW_MES_STMT_35:
		stmt->AIW_0x35.a = buffer_read_u16(mes);
		stmt->AIW_0x35.b = buffer_read_u16(mes); // XXX: not used for anything
		break;
	case AIW_MES_STMT_37:
		// XXX: value is not used for anything
		stmt->JMP.addr = buffer_read_u32(mes);
		break;
	default:
		mes->index--;
		DC_WARNING(mes->index, "Unprefixed text: 0x%02x (possibly unhandled statement)", b);
		stmt->op = AIW_MES_STMT_TXT;
		if (!(stmt->TXT.text = aiw_mes_parse_text(mes)))
			goto error;
		stmt->TXT.unprefixed = true;
		stmt->TXT.terminated = true;
		break;
	}
	stmt->next_address = mes->index;
	return stmt;
error:
	NOTICE("error at statement @ %08lx", (unsigned long)mes->index);
	free(stmt);
	return NULL;
}

declare_hashtable_int_type(addr_table, struct mes_statement*);
define_hashtable_int(addr_table, struct mes_statement*);

static bool set_jump_target(hashtable_t(addr_table) *table, uint32_t addr)
{
	hashtable_iter_t k = hashtable_get(addr_table, table, addr);
	if (unlikely(k == hashtable_end(table)))
		return false;
	kh_value(table, k)->is_jump_target = true;
	return true;
}

static void create_address_table(hashtable_t(addr_table) *table, mes_statement_list statements)
{
	struct mes_statement *p;
	vector_foreach(p, statements) {
		int ret;
		hashtable_iter_t k = hashtable_put(addr_table, table, p->address, &ret);
		if (unlikely(ret == HASHTABLE_KEY_PRESENT))
			ERROR("multiple statements with same address");
		hashtable_val(table, k) = p;
		if (p->aiw_op == AIW_MES_STMT_DEF_MENU) {
			struct aiw_mes_menu_case *c;
			vector_foreach_p(c, p->AIW_DEF_MENU.cases) {
				create_address_table(table, c->body);
			}
		}
	}
}

static void _tag_jump_targets(hashtable_t(addr_table) *table, mes_statement_list statements)
{
	struct mes_statement *p;
	vector_foreach(p, statements) {
		switch (p->op) {
		case AIW_MES_STMT_JZ:
			if (!set_jump_target(table, p->JZ.addr))
				ERROR("invalid address in JZ statement");
			break;
		case AIW_MES_STMT_JMP:
			if (!set_jump_target(table, p->JMP.addr))
				ERROR("invalid address in JMP statement");
			break;
		case AIW_MES_STMT_DEF_PROC:
			if (!set_jump_target(table, p->DEF_PROC.skip_addr))
				ERROR("invalid address in DEF_PROC statement");
			break;
		case AIW_MES_STMT_DEF_MENU: {
			struct aiw_mes_menu_case *e;
			vector_foreach_p(e, p->AIW_DEF_MENU.cases) {
				_tag_jump_targets(table, e->body);
			}
			if (!set_jump_target(table, p->AIW_DEF_MENU.skip_addr))
				ERROR("Invalid address in DEF_MENU statement");
			break;
		}
		default:
			break;
		}
	}

}

static void tag_jump_targets(mes_statement_list statements)
{
	hashtable_t(addr_table) table = hashtable_initializer(addr_table);
	create_address_table(&table, statements);
	_tag_jump_targets(&table, statements);
	hashtable_destroy(addr_table, &table);
}

static bool aiw_mes_parse_statements_until_end(struct buffer *mes, mes_statement_list *statements)
{
	uint8_t b;
	do {
		b = buffer_peek_u8(mes);
		struct mes_statement *stmt = aiw_mes_parse_statement(mes);
		if (!stmt)
			goto error;
		vector_push(struct mes_statement*, *statements, stmt);
	} while (b != AIW_MES_STMT_END);
	return true;
error:
	struct mes_statement *stmt;
	vector_foreach(stmt, *statements) {
		mes_statement_free(stmt);
	}
	vector_destroy(*statements);
	vector_init(*statements);
	return false;
}

bool aiw_mes_parse_statements(uint8_t *data, size_t data_size, mes_statement_list *statements)
{
	// TODO: read this?
	if (ai5_target_game == GAME_KAWARAZAKIKE) {
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
		struct mes_statement *stmt = aiw_mes_parse_statement(&mes);
		if (!stmt)
			goto error;
		vector_push(struct mes_statement*, *statements, stmt);
	}

	tag_jump_targets(*statements);
	return true;
error:
	struct mes_statement *stmt;
	vector_foreach(stmt, *statements) {
		mes_statement_free(stmt);
	}
	vector_destroy(*statements);
	vector_init(*statements);
	return false;
}

void aiw_mes_statement_free(struct mes_statement *stmt)
{
	switch (stmt->aiw_op) {
	case AIW_MES_STMT_TXT:
		string_free(stmt->TXT.text);
		break;
	case AIW_MES_STMT_SET_FLAG_CONST:
	case AIW_MES_STMT_SET_VAR16_CONST:
	case AIW_MES_STMT_SET_SYSVAR_CONST:
		mes_expression_list_free(stmt->SET_VAR_CONST.val_exprs);
		break;
	case AIW_MES_STMT_SET_FLAG_EXPR:
	case AIW_MES_STMT_SET_VAR16_EXPR:
	case AIW_MES_STMT_SET_SYSVAR_EXPR:
		mes_expression_free(stmt->SET_VAR_EXPR.var_expr);
		mes_expression_list_free(stmt->SET_VAR_EXPR.val_exprs);
		break;
	case AIW_MES_STMT_SET_VAR32:
		mes_expression_list_free(stmt->SET_VAR_CONST.val_exprs);
		break;
	case AIW_MES_STMT_PTR_SET8:
	case AIW_MES_STMT_PTR_SET16:
		mes_expression_free(stmt->PTR_SET.off_expr);
		mes_expression_list_free(stmt->PTR_SET.val_exprs);
		break;
	case AIW_MES_STMT_JZ:
		mes_expression_free(stmt->JZ.expr);
		break;
	case AIW_MES_STMT_UTIL:
	case AIW_MES_STMT_JMP_MES:
	case AIW_MES_STMT_CALL_MES:
	case AIW_MES_STMT_LOAD:
	case AIW_MES_STMT_SAVE:
	case AIW_MES_STMT_CALL_PROC:
	case AIW_MES_STMT_NUM:
	case AIW_MES_STMT_SET_TEXT_COLOR:
	case AIW_MES_STMT_WAIT:
	case AIW_MES_STMT_COMMIT_MESSAGE:
	case AIW_MES_STMT_LOAD_IMAGE:
	case AIW_MES_STMT_SURF_COPY:
	case AIW_MES_STMT_SURF_COPY_MASKED:
	case AIW_MES_STMT_SURF_SWAP:
	case AIW_MES_STMT_SURF_FILL:
	case AIW_MES_STMT_SURF_INVERT:
	case AIW_MES_STMT_29:
	case AIW_MES_STMT_SHOW_HIDE:
	case AIW_MES_STMT_CROSSFADE:
	case AIW_MES_STMT_CROSSFADE2:
	case AIW_MES_STMT_CURSOR:
	case AIW_MES_STMT_ANIM:
	case AIW_MES_STMT_LOAD_AUDIO:
	case AIW_MES_STMT_LOAD_EFFECT:
	case AIW_MES_STMT_LOAD_VOICE:
	case AIW_MES_STMT_AUDIO:
	case AIW_MES_STMT_PLAY_MOVIE:
	case AIW_MES_STMT_34:
		mes_parameter_list_free(stmt->CALL.params);
		break;
	case AIW_MES_STMT_DEF_PROC:
		mes_expression_free(stmt->DEF_PROC.no_expr);
		break;
	case AIW_MES_STMT_DEF_MENU: {
		mes_expression_free(stmt->AIW_DEF_MENU.expr);
		struct aiw_mes_menu_case *c;
		vector_foreach_p(c, stmt->AIW_DEF_MENU.cases) {
			mes_expression_free(c->cond);
			mes_statement_list_free(c->body);
		}
		vector_destroy(stmt->AIW_DEF_MENU.cases);
		break;
	}
	case AIW_MES_STMT_MENU_EXEC:
		mes_expression_list_free(stmt->AIW_MENU_EXEC.exprs);
		break;
	case AIW_MES_STMT_JMP:
	case AIW_MES_STMT_21:
	case AIW_MES_STMT_35:
	case AIW_MES_STMT_37:
	case AIW_MES_STMT_FE:
	case AIW_MES_STMT_END:
		// nothing
		break;
	}
	free(stmt);
}
