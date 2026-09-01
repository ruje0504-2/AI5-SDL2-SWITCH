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

#include "nulib.h"
#include "nulib/hashtable.h"
#include "nulib/port.h"
#include "nulib/string.h"
#include "ai5/game.h"
#include "ai5/mes.h"

// expressions {{{

static const char *binary_op_to_string(enum mes_expression_op op)
{
	switch (op) {
	case MES_EXPR_PLUS: return "+";
	case MES_EXPR_MINUS: return "-";
	case MES_EXPR_MUL: return "*";
	case MES_EXPR_DIV: return "/";
	case MES_EXPR_MOD: return "%";
	case MES_EXPR_AND: return "&&";
	case MES_EXPR_OR: return "||";
	case MES_EXPR_BITAND: return "&";
	case MES_EXPR_BITIOR: return "|";
	case MES_EXPR_BITXOR: return "^";
	case MES_EXPR_LT: return "<";
	case MES_EXPR_GT: return ">";
	case MES_EXPR_LTE: return "<=";
	case MES_EXPR_GTE: return ">=";
	case MES_EXPR_EQ: return "==";
	case MES_EXPR_NEQ: return "!=";
	default: ERROR("invalid binary operator: %d", op);
	}
}

static bool is_binary_op(enum mes_expression_op op)
{
	switch (op) {
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
		return true;
	default:
		return false;
	}
}

static bool binary_parens_required(enum mes_expression_op op, struct mes_expression *sub)
{
	if (!is_binary_op(sub->op))
		return false;

	switch (op) {
	case MES_EXPR_IMM:
	case MES_EXPR_GET_VAR16:
	case MES_EXPR_PTR16_GET16:
	case MES_EXPR_PTR16_GET8:
	case MES_EXPR_RAND:
	case MES_EXPR_IMM16:
	case MES_EXPR_IMM32:
	case MES_EXPR_GET_FLAG_CONST:
	case MES_EXPR_GET_FLAG_EXPR:
	case MES_EXPR_PTR32_GET32:
	case MES_EXPR_PTR32_GET16:
	case MES_EXPR_PTR32_GET8:
	case MES_EXPR_GET_VAR32:
	case MES_EXPR_GET_ARG_CONST:
	case MES_EXPR_GET_ARG_EXPR:
	case MES_EXPR_END:
		ERROR("invalid binary operator: %d", op);
	case MES_EXPR_MUL:
	case MES_EXPR_DIV:
	case MES_EXPR_MOD:
		return true;
	case MES_EXPR_PLUS:
	case MES_EXPR_MINUS:
		switch (sub->op) {
		case MES_EXPR_MUL:
		case MES_EXPR_DIV:
		case MES_EXPR_MOD:
			return false;
		default:
			return true;
		}
	case MES_EXPR_LT:
	case MES_EXPR_GT:
	case MES_EXPR_GTE:
	case MES_EXPR_LTE:
	case MES_EXPR_EQ:
	case MES_EXPR_NEQ:
		switch (sub->op) {
		case MES_EXPR_PLUS:
		case MES_EXPR_MINUS:
		case MES_EXPR_MUL:
		case MES_EXPR_DIV:
		case MES_EXPR_MOD:
			return false;
		default:
			return true;
		}
	case MES_EXPR_BITAND:
	case MES_EXPR_BITIOR:
	case MES_EXPR_BITXOR:
		return true;
	case MES_EXPR_AND:
	case MES_EXPR_OR:
		switch (sub->op) {
		case MES_EXPR_AND:
		case MES_EXPR_OR:
			return true;
		default:
			return false;
		}
	}
	return true;
}

static void _mes_expression_print(struct mes_expression *expr, struct port *out, bool bitwise);

static void mes_binary_expression_print(enum mes_expression_op op, struct mes_expression *lhs,
		struct mes_expression *rhs, struct port *out, bool bitwise)
{
	if (binary_parens_required(op, rhs)) {
		port_putc(out, '(');
		_mes_expression_print(rhs, out, bitwise);
		port_putc(out, ')');
	} else {
		_mes_expression_print(rhs, out, bitwise);
	}
	port_printf(out, " %s ", binary_op_to_string(op));
	if (binary_parens_required(op, lhs)) {
		port_putc(out, '(');
		_mes_expression_print(lhs, out, bitwise);
		port_putc(out, ')');
	} else {
		_mes_expression_print(lhs, out, bitwise);
	}
}

static const char *system_var16_name(uint8_t no)
{
	enum mes_system_var16 v = mes_index_to_sysvar16(no);
	if (v < MES_NR_SYSTEM_VARIABLES)
		return mes_system_var16_names[v];
	return NULL;
}

static void op_array16_get16_print(struct mes_expression *expr, struct port *out)
{
	// if arg is 0, we're reading a system variable
	const char *name;
	if (expr->arg8 == 0 && expr->sub_a->op == MES_EXPR_IMM
			&& (name = system_var16_name(expr->sub_a->arg8))) {
		port_printf(out, "System.%s", name);
		return;
	}

	// system variable with non-immediate index or unknown name
	if (expr->arg8 == 0) {
		port_puts(out, "System.var16[");
		_mes_expression_print(expr->sub_a, out, false);
		port_putc(out, ']');
		return;
	}

	// read short from memory
	// variable is offset from start of memory
	// expression is index into short array at that offset
	port_printf(out, "var16[%d]->word[", (int)expr->arg8 - 1);
	_mes_expression_print(expr->sub_a, out, false);
	port_putc(out, ']');
}

static const char *system_var32_name(uint8_t no)
{
	enum mes_system_var32 v = mes_index_to_sysvar32(no);
	if (v < MES_NR_SYSTEM_VARIABLES)
		return mes_system_var32_names[v];
	return NULL;
}

static void op_array32_get32_print(struct mes_expression *expr, struct port *out)
{
	// if arg is 0, we're reading a system pointer
	const char *name;
	if (expr->arg8 == 0 && expr->sub_a->op == MES_EXPR_IMM
			&& (name = system_var32_name(expr->sub_a->arg8))) {
		port_printf(out, "System.%s", name);
		return;
	}

	// system pointer with non-immediate index or unknown name
	if (expr->arg8 == 0) {
		port_puts(out, "System.var32[");
		_mes_expression_print(expr->sub_a, out, false);
		port_putc(out, ']');
		return;
	}

	port_printf(out, "var32[%d]->dword[", (int)expr->arg8 - 1);
	_mes_expression_print(expr->sub_a, out, false);
	port_putc(out, ']');
}

static void print_number(uint32_t n, struct port *out, bool hex)
{
	if (n < 255) {
		/* nothing */
	} else if ((n & 0xff) == 0) {
		hex = true;
	} else if ((n & (n - 1)) == 0 || ((n+1) & n) == 0) {
		hex = true;
	}

	if (hex) {
		port_printf(out, "0x%x", n);
	} else {
		port_printf(out, "%u", n);
	}
}

static void _mes_expression_print(struct mes_expression *expr, struct port *out, bool bitwise)
{
	switch (expr->op) {
	case MES_EXPR_IMM:
		print_number(expr->arg8, out, bitwise);
		break;
	case MES_EXPR_GET_VAR16:
		port_printf(out, "var16[%u]", (unsigned)expr->arg8);
		break;
	case MES_EXPR_PTR16_GET16:
		op_array16_get16_print(expr, out);
		break;
	case MES_EXPR_PTR16_GET8:
		port_printf(out, "var16[%d]->byte[", (int)expr->arg8);
		_mes_expression_print(expr->sub_a, out, false);
		port_putc(out, ']');
		break;
	case MES_EXPR_PLUS:
	case MES_EXPR_MINUS:
	case MES_EXPR_MUL:
	case MES_EXPR_DIV:
	case MES_EXPR_MOD:
		// bitwise context is preserved
		mes_binary_expression_print(expr->op, expr->sub_a, expr->sub_b, out, bitwise);
		break;
	case MES_EXPR_AND:
	case MES_EXPR_OR:
	case MES_EXPR_LT:
	case MES_EXPR_GT:
	case MES_EXPR_LTE:
	case MES_EXPR_GTE:
	case MES_EXPR_EQ:
	case MES_EXPR_NEQ:
		// leaving bitwise context
		mes_binary_expression_print(expr->op, expr->sub_a, expr->sub_b, out, false);
		break;
	case MES_EXPR_BITAND:
	case MES_EXPR_BITIOR:
	case MES_EXPR_BITXOR:
		// entering bitwise context
		mes_binary_expression_print(expr->op, expr->sub_a, expr->sub_b, out, true);
		break;
	case MES_EXPR_RAND:
		port_puts(out, "rand(");
		_mes_expression_print(expr->sub_a, out, false);
		port_putc(out, ')');
		break;
	case MES_EXPR_IMM16:
		print_number(expr->arg16, out, bitwise);
		break;
	case MES_EXPR_IMM32:
		print_number(expr->arg32, out, bitwise);
		break;
	case MES_EXPR_GET_FLAG_CONST:
		port_printf(out, "var4[%u]", (unsigned)expr->arg16);
		break;
	case MES_EXPR_GET_FLAG_EXPR:
		port_puts(out, "var4[");
		_mes_expression_print(expr->sub_a, out, false);
		port_putc(out, ']');
		break;
	case MES_EXPR_PTR32_GET32:
		op_array32_get32_print(expr, out);
		break;
	case MES_EXPR_PTR32_GET16:
		port_printf(out, "var32[%d]->word[", (int)expr->arg8 - 1);
		_mes_expression_print(expr->sub_a, out, false);
		port_putc(out, ']');
		break;
	case MES_EXPR_PTR32_GET8:
		port_printf(out, "var32[%d]->byte[", (int)expr->arg8 - 1);
		_mes_expression_print(expr->sub_a, out, false);
		port_putc(out, ']');
		break;
	case MES_EXPR_GET_VAR32:
		port_printf(out, "var32[%d]", (int)expr->arg8);
		break;
	case MES_EXPR_GET_ARG_CONST:
		port_printf(out, "arg[%d]", (int)expr->arg16);
		break;
	case MES_EXPR_GET_ARG_EXPR:
		port_puts(out, "arg[");
		_mes_expression_print(expr->sub_a, out, false);
		port_putc(out, ']');
		break;
	case MES_EXPR_END:
		ERROR("encountered END expression when printing");
	}
}

void mes_expression_print(struct mes_expression *expr, struct port *out)
{
	if (game_is_aiwin())
		_aiw_mes_expression_print(expr, out, false);
	else
		_mes_expression_print(expr, out, false);
}

void mes_expression_list_print(mes_expression_list list, struct port *out)
{
	for (unsigned i = 0; i < vector_length(list); i++) {
		if (i > 0)
			port_putc(out, ',');
		mes_expression_print(vector_A(list, i), out);
	}
}

// expressions }}}
// parameters {{{

void mes_parameter_print(struct mes_parameter *param, struct port *out)
{
	switch (param->type) {
	case MES_PARAM_STRING:
		port_printf(out, "\"%s\"", param->str);
		break;
	case MES_PARAM_EXPRESSION:
		mes_expression_print(param->expr, out);
		break;
	}
}

void mes_parameter_list_print_from(mes_parameter_list list, unsigned start, struct port *out)
{
	port_putc(out, '(');
	for (unsigned i = start; i < vector_length(list); i++) {
		if (i > start)
			port_putc(out, ',');
		mes_parameter_print(&vector_A(list, i), out);
	}
	port_putc(out, ')');
}

void mes_parameter_list_print(mes_parameter_list list, struct port *out)
{
	mes_parameter_list_print_from(list, 0, out);
}

static void indent_print(struct port *out, int indent)
{
	for (int i = 0; i < indent; i++) {
		port_putc(out, '\t');
	}
}

// parameters }}}
// statements {{{

static void stmt_seta_at_print(struct mes_statement *stmt, struct port *out)
{
	const char *name;
	if (stmt->PTR_SET.var_no == 0 && stmt->PTR_SET.off_expr->op == MES_EXPR_IMM
			&& (name = system_var16_name(stmt->PTR_SET.off_expr->arg8))) {
		port_printf(out, "System.%s", name);
	} else if (stmt->PTR_SET.var_no == 0) {
		port_puts(out, "System.var16[");
		mes_expression_print(stmt->PTR_SET.off_expr, out);
		port_putc(out, ']');
	} else {
		port_printf(out, "var16[%d]->word[", (int)stmt->PTR_SET.var_no - 1);
		mes_expression_print(stmt->PTR_SET.off_expr, out);
		port_putc(out, ']');
	}
	port_puts(out, " = ");
	mes_expression_list_print(stmt->PTR_SET.val_exprs, out);
	port_puts(out, ";\n");
}

static void stmt_setad_print(struct mes_statement *stmt, struct port *out)
{
	const char *name;
	if (stmt->PTR_SET.var_no == 0 && stmt->PTR_SET.off_expr->op == MES_EXPR_IMM
			&& (name = system_var32_name(stmt->PTR_SET.off_expr->arg8))) {
		port_printf(out, "System.%s", name);
	} else if (stmt->PTR_SET.var_no == 0) {
		port_puts(out, "System.var32[");
		mes_expression_print(stmt->PTR_SET.off_expr, out);
		port_putc(out, ']');
	} else {
		port_printf(out, "var32[%d]->dword[", (int)stmt->PTR_SET.var_no - 1);
		mes_expression_print(stmt->PTR_SET.off_expr, out);
		port_putc(out, ']');
	}
	port_puts(out, " = ");
	mes_expression_list_print(stmt->PTR_SET.val_exprs, out);
	port_puts(out, ";\n");
}

static void stmt_sys_print(struct mes_statement *stmt, struct port *out)
{
	if (stmt->SYS.expr->op != MES_EXPR_IMM) {
		port_puts(out, "System.function[");
		mes_expression_print(stmt->SYS.expr, out);
		port_putc(out, ']');
		mes_parameter_list_print(stmt->SYS.params, out);
		port_puts(out, ";\n");
		return;
	}

	unsigned skip_params;
	string name = mes_get_syscall_name(stmt->SYS.expr->arg8, stmt->SYS.params,
			&skip_params, "System");
	port_puts(out, name);
	mes_parameter_list_print_from(stmt->SYS.params, skip_params, out);
	port_puts(out, ";\n");
	string_free(name);
}

static void stmt_util_print(struct mes_statement *stmt, struct port *out)
{
	unsigned skip_params;
	string name = mes_get_util_name(stmt->CALL.params, &skip_params);
	port_puts(out, name);
	mes_parameter_list_print_from(stmt->CALL.params, skip_params, out);
	port_puts(out, ";\n");
	string_free(name);
}

declare_hashtable_int_type(label_map, unsigned);
define_hashtable_int(label_map, unsigned);
static int prev_label_seq = 0;

static hashtable_t(label_map) label_map = hashtable_initializer(label_map);
bool mes_sequence_labels = true;

void mes_clear_labels(void)
{
	hashtable_destroy(label_map, &label_map);
	label_map = hashtable_initializer(label_map);
	prev_label_seq = 0;
}

void mes_label_print(uint32_t addr, const char *suffix, struct port *out)
{
	if (!mes_sequence_labels) {
		port_printf(out, "L_%08x%s", addr, suffix);
		return;
	}

	// get sequence number from label map
	int ret;
	hashtable_iter_t k = hashtable_put(label_map, &label_map, addr, &ret);
	if (ret != HASHTABLE_KEY_PRESENT) {
		// new label
		hashtable_val(&label_map, k) = ++prev_label_seq;
	}

	port_printf(out, "L_%u%s", hashtable_val(&label_map, k), suffix);
}

void _mes_statement_print(struct mes_statement *stmt, struct port *out, int indent)
{
	indent_print(out, indent);

	switch (stmt->op) {
	case MES_STMT_END:
		port_puts(out, "return;\n");
		break;
	case MES_STMT_ZENKAKU:
		// SJIS string
		if (stmt->TXT.unprefixed)
			port_puts(out, "unprefixed ");
		if (!stmt->TXT.terminated)
			port_puts(out, "unterminated ");
		port_printf(out, "\"%s\";\n", stmt->TXT.text);
		break;
	case MES_STMT_HANKAKU:
		// ASCII string
		if (stmt->TXT.unprefixed)
			port_puts(out, "unprefixed ");
		if (!stmt->TXT.terminated)
			port_puts(out, "unterminated ");
		port_printf(out, "\"%s\";\n", stmt->TXT.text);
		break;
	case MES_STMT_SET_FLAG_CONST:
		// var4[v] = ...;
		port_printf(out, "var4[%u] = ", (unsigned)stmt->SET_VAR_CONST.var_no);
		mes_expression_list_print(stmt->SET_VAR_CONST.val_exprs, out);
		port_puts(out, ";\n");
		break;
	case MES_STMT_SET_VAR16:
		// var16[v] = ...;
		port_printf(out, "var16[%u] = ", (unsigned)stmt->SET_VAR_CONST.var_no);
		mes_expression_list_print(stmt->SET_VAR_CONST.val_exprs, out);
		port_puts(out, ";\n");
		break;
	case MES_STMT_SET_FLAG_EXPR:
		// var4[e] = ...;
		port_puts(out, "var4[");
		mes_expression_print(stmt->SET_VAR_EXPR.var_expr, out);
		port_puts(out, "] = ");
		mes_expression_list_print(stmt->SET_VAR_EXPR.val_exprs, out);
		port_puts(out, ";\n");
		break;
	case MES_STMT_PTR16_SET8:
		// var16[v]->byte[e] = ...;
		port_printf(out, "var16[%u]->byte[", (unsigned)stmt->PTR_SET.var_no);
		mes_expression_print(stmt->PTR_SET.off_expr, out);
		port_puts(out, "] = ");
		mes_expression_list_print(stmt->PTR_SET.val_exprs, out);
		port_puts(out, ";\n");
		break;
	case MES_STMT_PTR16_SET16:
		// var16[v-1]->word[e] = ...;
		// System.var16[e] = ...; // when v = 0
		stmt_seta_at_print(stmt, out);
		break;
	case MES_STMT_PTR32_SET32:
		// var32[v]->dword[e] = ...;
		// System.var32[e] = ...; // when v = 0
		stmt_setad_print(stmt, out);
		break;
	case MES_STMT_PTR32_SET16:
		// var32[v]->word[e] = ...;
		port_printf(out, "var32[%d]->word[", (int)stmt->PTR_SET.var_no - 1);
		mes_expression_print(stmt->PTR_SET.off_expr, out);
		port_puts(out, "] = ");
		mes_expression_list_print(stmt->PTR_SET.val_exprs, out);
		port_puts(out, ";\n");
		break;
	case MES_STMT_PTR32_SET8:
		// var32[v]->byte[e] = ...;
		port_printf(out, "var32[%d]->byte[", (int)stmt->PTR_SET.var_no - 1);
		mes_expression_print(stmt->PTR_SET.off_expr, out);
		port_puts(out, "] = ");
		mes_expression_list_print(stmt->PTR_SET.val_exprs, out);
		port_puts(out, ";\n");
		break;
	case MES_STMT_JZ:
		port_puts(out, "jz ");
		mes_expression_print(stmt->JZ.expr, out);
		port_putc(out, ' ');
		mes_label_print(stmt->JZ.addr, ";\n", out);
		break;
	case MES_STMT_JMP:
		port_puts(out, "goto ");
		mes_label_print(stmt->JMP.addr, ";\n", out);
		break;
	case MES_STMT_SYS:
		stmt_sys_print(stmt, out);
		break;
	case MES_STMT_JMP_MES:
		port_puts(out, "jump");
		mes_parameter_list_print(stmt->CALL.params, out);
		port_puts(out, ";\n");
		break;
	case MES_STMT_CALL_MES:
		port_puts(out, "call");
		mes_parameter_list_print(stmt->CALL.params, out);
		port_puts(out, ";\n");
		break;
	case MES_STMT_DEF_MENU:
		port_puts(out, "defmenu");
		mes_parameter_list_print(stmt->DEF_MENU.params, out);
		port_putc(out, ' ');
		mes_label_print(stmt->DEF_MENU.skip_addr, ";\n", out);
		break;
	case MES_STMT_CALL_PROC:
		port_puts(out, "call");
		mes_parameter_list_print(stmt->CALL.params, out);
		port_puts(out, ";\n");
		break;
	case MES_STMT_UTIL:
		stmt_util_print(stmt, out);
		break;
	case MES_STMT_LINE:
		port_printf(out, "line %u;\n", (unsigned)stmt->LINE.arg);
		break;
	case MES_STMT_DEF_PROC:
		port_puts(out, "defproc ");
		mes_expression_print(stmt->DEF_PROC.no_expr, out);
		port_putc(out, ' ');
		mes_label_print(stmt->DEF_PROC.skip_addr, ";\n", out);
		break;
	case MES_STMT_MENU_EXEC:
		port_puts(out, "menuexec");
		if (ai5_target_game == GAME_NONOMURA) {
			mes_parameter_list_print(stmt->DEF_MENU.params, out);
		}
		port_puts(out, ";\n");
		break;
	case MES_STMT_SET_VAR32:
		port_printf(out, "var32[%d] = ", (unsigned)stmt->SET_VAR_CONST.var_no);
		mes_expression_list_print(stmt->SET_VAR_CONST.val_exprs, out);
		port_puts(out, ";\n");
		break;
	case MES_STMT_17:
		port_printf(out, "OP_0x17 %u;\n", (unsigned)stmt->JMP.addr);
		break;
	case MES_STMT_18:
		port_puts(out, "OP_0x18 ");
		mes_expression_print(stmt->SET_VAR_EXPR.var_expr, out);
		port_puts(out, ";\n");
		break;
	case MES_STMT_19:
		port_puts(out, "OP_0x19;\n");
		break;
	case MES_STMT_1A:
		port_puts(out, "OP_0x1A;\n");
		break;
	case MES_STMT_1B:
		port_puts(out, "OP_0x1B");
		mes_parameter_list_print(stmt->CALL.params, out);
		port_puts(out, ";\n");
		break;
	case MES_STMT_DEF_SUB:
		port_puts(out, "defsub ");
		mes_expression_print(stmt->DEF_PROC.no_expr, out);
		port_putc(out, ' ');
		mes_label_print(stmt->DEF_PROC.skip_addr, ";\n", out);
		break;
	case MES_STMT_CALL_SUB:
		port_puts(out, "call_sub");
		mes_parameter_list_print(stmt->CALL.params, out);
		port_puts(out, ";\n");
		break;
	case MES_STMT_1F:
		port_printf(out, "OP_0x1F %u;\n", stmt->JMP.addr);
		break;
	case MES_STMT_SET_ARG_CONST:
		port_printf(out, "arg[%d] = ", (unsigned)stmt->SET_VAR_CONST.var_no);
		mes_expression_list_print(stmt->SET_VAR_CONST.val_exprs, out);
		port_puts(out, ";\n");
		break;
	case MES_STMT_SET_ARG_EXPR:
		port_puts(out, "arg[");
		mes_expression_print(stmt->SET_VAR_EXPR.var_expr, out);
		port_puts(out, "] = ");
		mes_expression_list_print(stmt->SET_VAR_EXPR.val_exprs, out);
		port_puts(out, ";\n");
		break;
	}
}

void mes_statement_print(struct mes_statement *stmt, struct port *out)
{
	if (game_is_aiwin())
		_aiw_mes_statement_print(stmt, out, 1);
	else
		_mes_statement_print(stmt, out, 1);
}

void _mes_statement_list_print(mes_statement_list statements, struct port *out, int indent)
{
	void (*print)(struct mes_statement*, struct port*, int) = _mes_statement_print;
	if (game_is_aiwin())
		print = _aiw_mes_statement_print;

	struct mes_statement *stmt;
	vector_foreach(stmt, statements) {
		print(stmt, out, indent);
	}
}

void mes_statement_list_print(mes_statement_list statements, struct port *out)
{
	_mes_statement_list_print(statements, out, 1);
}

void mes_flat_statement_list_print(mes_statement_list statements, struct port *out)
{
	void (*print)(struct mes_statement*, struct port*, int) = _mes_statement_print;
	if (game_is_aiwin())
		print = _aiw_mes_statement_print;

	struct mes_statement *stmt;
	vector_foreach(stmt, statements) {
		if (stmt->is_jump_target)
			mes_label_print(stmt->address, ":\n", out);
		print(stmt, out, 1);
	}
}

// statements }}}
