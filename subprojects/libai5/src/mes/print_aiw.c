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
#include "nulib/port.h"
#include "nulib/string.h"
#include "ai5/mes.h"

static void indent_print(struct port *out, int indent)
{
	for (int i = 0; i < indent; i++) {
		port_putc(out, '\t');
	}
}

// expressions {{{

static const char *binary_op_to_string(enum aiw_mes_expression_op op)
{
	switch (op) {
	case AIW_MES_EXPR_PLUS: return "+";
	case AIW_MES_EXPR_MINUS: return "-";
	case AIW_MES_EXPR_MUL: return "*";
	case AIW_MES_EXPR_DIV: return "/";
	case AIW_MES_EXPR_MOD: return "%";
	case AIW_MES_EXPR_AND: return "&&";
	case AIW_MES_EXPR_OR: return "||";
	case AIW_MES_EXPR_BITAND: return "&";
	case AIW_MES_EXPR_BITIOR: return "|";
	case AIW_MES_EXPR_BITXOR: return "^";
	case AIW_MES_EXPR_LT: return "<";
	case AIW_MES_EXPR_GT: return ">";
	case AIW_MES_EXPR_LTE: return "<=";
	case AIW_MES_EXPR_GTE: return ">=";
	case AIW_MES_EXPR_EQ: return "==";
	case AIW_MES_EXPR_NEQ: return "!=";
	default: ERROR("invalid binary operator: %d", op);
	}
}

static bool is_binary_op(enum aiw_mes_expression_op op)
{
	switch (op) {
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
		return true;
	default:
		return false;
	}
}

static bool binary_parens_required(enum aiw_mes_expression_op op, struct mes_expression *sub)
{
	if (!is_binary_op(sub->aiw_op))
		return false;

	switch (op) {
	case AIW_MES_EXPR_IMM:
	case AIW_MES_EXPR_VAR32:
	case AIW_MES_EXPR_PTR_GET8:
	case AIW_MES_EXPR_RAND:
	case AIW_MES_EXPR_IMM16:
	case AIW_MES_EXPR_IMM32:
	case AIW_MES_EXPR_GET_FLAG_CONST:
	case AIW_MES_EXPR_GET_FLAG_EXPR:
	case AIW_MES_EXPR_GET_VAR16_CONST:
	case AIW_MES_EXPR_GET_VAR16_EXPR:
	case AIW_MES_EXPR_GET_SYSVAR_CONST:
	case AIW_MES_EXPR_GET_SYSVAR_EXPR:
	case AIW_MES_EXPR_END:
		ERROR("invalid binary operator: %d", op);
	case AIW_MES_EXPR_MUL:
	case AIW_MES_EXPR_DIV:
	case AIW_MES_EXPR_MOD:
		return true;
	case AIW_MES_EXPR_PLUS:
	case AIW_MES_EXPR_MINUS:
		switch (sub->aiw_op) {
		case AIW_MES_EXPR_MUL:
		case AIW_MES_EXPR_DIV:
		case AIW_MES_EXPR_MOD:
			return false;
		default:
			return true;
		}
	case AIW_MES_EXPR_LT:
	case AIW_MES_EXPR_GT:
	case AIW_MES_EXPR_GTE:
	case AIW_MES_EXPR_LTE:
	case AIW_MES_EXPR_EQ:
	case AIW_MES_EXPR_NEQ:
		switch (sub->aiw_op) {
		case AIW_MES_EXPR_PLUS:
		case AIW_MES_EXPR_MINUS:
		case AIW_MES_EXPR_MUL:
		case AIW_MES_EXPR_DIV:
		case AIW_MES_EXPR_MOD:
			return false;
		default:
			return true;
		}
	case AIW_MES_EXPR_BITAND:
	case AIW_MES_EXPR_BITIOR:
	case AIW_MES_EXPR_BITXOR:
		return true;
	case AIW_MES_EXPR_AND:
	case AIW_MES_EXPR_OR:
		switch (sub->aiw_op) {
		case AIW_MES_EXPR_AND:
		case AIW_MES_EXPR_OR:
			return true;
		default:
			return false;
		}
	}
	return true;
}

static void mes_binary_expression_print(enum mes_expression_op op, struct mes_expression *lhs,
		struct mes_expression *rhs, struct port *out, bool bitwise)
{
	if (binary_parens_required(op, rhs)) {
		port_putc(out, '(');
		_aiw_mes_expression_print(rhs, out, bitwise);
		port_putc(out, ')');
	} else {
		_aiw_mes_expression_print(rhs, out, bitwise);
	}
	port_printf(out, " %s ", binary_op_to_string(op));
	if (binary_parens_required(op, lhs)) {
		port_putc(out, '(');
		_aiw_mes_expression_print(lhs, out, bitwise);
		port_putc(out, ')');
	} else {
		_aiw_mes_expression_print(lhs, out, bitwise);
	}
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

static void print_sysvar_name(unsigned var_no, struct port *out)
{
	enum mes_system_var16 v = mes_index_to_sysvar16(var_no);
	if (v < MES_NR_SYSTEM_VARIABLES)
		port_printf(out, "System.%s", mes_system_var16_names[v]);
	else
		port_printf(out, "System.var16[%u]", var_no);
}

void _aiw_mes_expression_print(struct mes_expression *expr, struct port *out, bool bitwise)
{
	switch (expr->aiw_op) {
	case AIW_MES_EXPR_IMM:
		print_number(expr->arg8, out, bitwise);
		break;
	case AIW_MES_EXPR_VAR32:
		port_printf(out, "var32[%u]", (unsigned)expr->arg8);
		break;
	case AIW_MES_EXPR_PTR_GET8:
		port_printf(out, "var32[%d]->byte[", (int)expr->arg8);
		_aiw_mes_expression_print(expr->sub_a, out, false);
		port_putc(out, ']');
		break;
	case AIW_MES_EXPR_PLUS:
	case AIW_MES_EXPR_MINUS:
	case AIW_MES_EXPR_MUL:
	case AIW_MES_EXPR_DIV:
	case AIW_MES_EXPR_MOD:
		// bitwise context is preserved
		mes_binary_expression_print(expr->op, expr->sub_a, expr->sub_b, out, bitwise);
		break;
	case AIW_MES_EXPR_AND:
	case AIW_MES_EXPR_OR:
	case AIW_MES_EXPR_LT:
	case AIW_MES_EXPR_GT:
	case AIW_MES_EXPR_LTE:
	case AIW_MES_EXPR_GTE:
	case AIW_MES_EXPR_EQ:
	case AIW_MES_EXPR_NEQ:
		// leaving bitwise context
		mes_binary_expression_print(expr->op, expr->sub_a, expr->sub_b, out, false);
		break;
	case AIW_MES_EXPR_BITAND:
	case AIW_MES_EXPR_BITIOR:
	case AIW_MES_EXPR_BITXOR:
		// entering bitwise context
		mes_binary_expression_print(expr->op, expr->sub_a, expr->sub_b, out, true);
		break;
	case AIW_MES_EXPR_RAND:
		port_printf(out, "rand(%u)", expr->arg16);
		break;
	case AIW_MES_EXPR_IMM16:
		print_number(expr->arg16, out, bitwise);
		break;
	case AIW_MES_EXPR_IMM32:
		print_number(expr->arg32, out, bitwise);
		break;
	case AIW_MES_EXPR_GET_FLAG_CONST:
		port_printf(out, "var4[%u]", (unsigned)expr->arg16);
		break;
	case AIW_MES_EXPR_GET_FLAG_EXPR:
		port_puts(out, "var4[");
		_aiw_mes_expression_print(expr->sub_a, out, false);
		port_putc(out, ']');
		break;
	case AIW_MES_EXPR_GET_VAR16_CONST:
		port_printf(out, "var16[%u]", (unsigned)expr->arg16);
		break;
	case AIW_MES_EXPR_GET_VAR16_EXPR:
		port_puts(out, "var16[");
		_aiw_mes_expression_print(expr->sub_a, out, false);
		port_putc(out, ']');
		break;
	case AIW_MES_EXPR_GET_SYSVAR_CONST:
		print_sysvar_name(expr->arg16, out);
		break;
	case AIW_MES_EXPR_GET_SYSVAR_EXPR:
		port_puts(out, "System.var16[");
		_aiw_mes_expression_print(expr->sub_a, out, false);
		port_putc(out, ']');
		break;
	case MES_EXPR_END:
		ERROR("encountered END expression when printing");
	}
}

void aiw_mes_expression_list_print(mes_expression_list list, struct port *out)
{
	for (unsigned i = 0; i < vector_length(list); i++) {
		if (i > 0)
			port_putc(out, ',');
		_aiw_mes_expression_print(vector_A(list, i), out, false);
	}
}

// expressions }}}
// statements {{{

static void print_call(struct port *out, struct mes_statement *stmt, const char *name)
{
	port_puts(out, name);
	mes_parameter_list_print(stmt->CALL.params, out);
	port_puts(out, ";\n");
}

/*
 * defmenu <expr> {
 *     case [expr] {
 *         <stmt>
 *         ...
 *     }
 * }
 */
static void print_defmenu(struct port *out, struct mes_statement *stmt, int indent)
{
	port_puts(out, "defmenu ");
	_aiw_mes_expression_print(stmt->AIW_DEF_MENU.expr, out, false);

	port_puts(out, " {\n");

	struct aiw_mes_menu_case *c;
	vector_foreach_p(c, stmt->AIW_DEF_MENU.cases) {
		indent_print(out, indent + 1);
		port_puts(out, "case ");
		if (c->cond) {
			_aiw_mes_expression_print(c->cond, out, false);
			port_putc(out, ' ');
		}
		port_puts(out, "{\n");

		struct mes_statement *s;
		vector_foreach(s, c->body) {
			if (s->is_jump_target)
				mes_label_print(s->address, ":\n", out);
			_aiw_mes_statement_print(s, out, indent + 2);
		}

		indent_print(out, indent + 1);
		port_puts(out, "}\n");
	}

	indent_print(out, indent);
	port_puts(out, "}\n");
}

static void print_syscall(struct mes_statement *stmt, struct port *out)
{
	unsigned skip_params;
	string name = mes_get_syscall_name(stmt->op, stmt->CALL.params, &skip_params, NULL);
	port_puts(out, name);
	mes_parameter_list_print_from(stmt->CALL.params, skip_params, out);
	port_puts(out, ";\n");
	string_free(name);
}

void _aiw_mes_statement_print(struct mes_statement *stmt, struct port *out, int indent)
{
	indent_print(out, indent);

	switch (stmt->aiw_op) {
	case AIW_MES_STMT_FE:
		port_puts(out, "OP_0xFE;\n");
		break;
	case AIW_MES_STMT_END:
		port_puts(out, "return;\n");
		break;
	case AIW_MES_STMT_TXT:
		// SJIS string
		if (stmt->TXT.unprefixed)
			port_puts(out, "unprefixed ");
		if (!stmt->TXT.terminated)
			port_puts(out, "unterminated ");
		port_printf(out, "\"%s\";\n", stmt->TXT.text);
		break;
	case AIW_MES_STMT_JMP:
		port_puts(out, "goto ");
		mes_label_print(stmt->JMP.addr, ";\n", out);
		break;
	case AIW_MES_STMT_UTIL:
	case AIW_MES_STMT_LOAD:
	case AIW_MES_STMT_SAVE:
	case AIW_MES_STMT_NUM:
	case AIW_MES_STMT_SET_TEXT_COLOR:
	case AIW_MES_STMT_WAIT:
	case AIW_MES_STMT_21:
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
		print_syscall(stmt, out);
		break;
	case AIW_MES_STMT_JMP_MES:
		print_call(out, stmt, "jump");
		break;
	case AIW_MES_STMT_CALL_MES:
		print_call(out, stmt, "call");
		break;
	case AIW_MES_STMT_SET_FLAG_CONST:
		port_printf(out, "var4[%u] = ", (unsigned)stmt->SET_VAR_CONST.var_no);
		aiw_mes_expression_list_print(stmt->SET_VAR_CONST.val_exprs, out);
		port_puts(out, ";\n");
		break;
	case AIW_MES_STMT_SET_FLAG_EXPR:
		port_puts(out, "var4[");
		_aiw_mes_expression_print(stmt->SET_VAR_EXPR.var_expr, out, false);
		port_puts(out, "] = ");
		aiw_mes_expression_list_print(stmt->SET_VAR_EXPR.val_exprs, out);
		port_puts(out, ";\n");
		break;
	case AIW_MES_STMT_SET_VAR32:
		port_printf(out, "var32[%d] = ", (unsigned)stmt->SET_VAR_CONST.var_no);
		aiw_mes_expression_list_print(stmt->SET_VAR_CONST.val_exprs, out);
		port_puts(out, ";\n");
		break;
	case AIW_MES_STMT_PTR_SET8:
		port_printf(out, "var32[%u]->byte[", (unsigned)stmt->PTR_SET.var_no);
		_aiw_mes_expression_print(stmt->PTR_SET.off_expr, out, false);
		port_puts(out, "] = ");
		aiw_mes_expression_list_print(stmt->PTR_SET.val_exprs, out);
		port_puts(out, ";\n");
		break;
	case AIW_MES_STMT_PTR_SET16:
		port_printf(out, "var32[%u]->word[", (unsigned)stmt->PTR_SET.var_no);
		_aiw_mes_expression_print(stmt->PTR_SET.off_expr, out, false);
		port_puts(out, "] = ");
		aiw_mes_expression_list_print(stmt->PTR_SET.val_exprs, out);
		port_puts(out, ";\n");
		break;
	case AIW_MES_STMT_SET_VAR16_CONST:
		port_printf(out, "var16[%u] = ", (unsigned)stmt->SET_VAR_CONST.var_no);
		aiw_mes_expression_list_print(stmt->SET_VAR_CONST.val_exprs, out);
		port_puts(out, ";\n");
		break;
	case AIW_MES_STMT_SET_VAR16_EXPR:
		port_puts(out, "var16[");
		_aiw_mes_expression_print(stmt->SET_VAR_EXPR.var_expr, out, false);
		port_puts(out, "] = ");
		aiw_mes_expression_list_print(stmt->SET_VAR_EXPR.val_exprs, out);
		port_puts(out, ";\n");
		break;
	case AIW_MES_STMT_SET_SYSVAR_CONST:
		print_sysvar_name(stmt->SET_VAR_CONST.var_no, out);
		port_puts(out, " = ");
		aiw_mes_expression_list_print(stmt->SET_VAR_CONST.val_exprs, out);
		port_puts(out, ";\n");
		break;
	case AIW_MES_STMT_SET_SYSVAR_EXPR:
		port_puts(out, "System.var16[");
		_aiw_mes_expression_print(stmt->SET_VAR_EXPR.var_expr, out, false);
		port_puts(out, "] = "); aiw_mes_expression_list_print(stmt->SET_VAR_EXPR.val_exprs, out);
		port_puts(out, ";\n");
		break;
	case AIW_MES_STMT_JZ:
		port_puts(out, "jz ");
		_aiw_mes_expression_print(stmt->JZ.expr, out, false);
		port_putc(out, ' ');
		mes_label_print(stmt->JZ.addr, ";\n", out);
		break;
	case AIW_MES_STMT_DEF_PROC:
		port_puts(out, "defproc ");
		_aiw_mes_expression_print(stmt->DEF_PROC.no_expr, out, false);
		port_putc(out, ' ');
		mes_label_print(stmt->DEF_PROC.skip_addr, ";\n", out);
		break;
	case AIW_MES_STMT_CALL_PROC:
		print_call(out, stmt, "call");
		break;
	case AIW_MES_STMT_DEF_MENU: {
		print_defmenu(out, stmt, indent);
		break;
	}
	case AIW_MES_STMT_MENU_EXEC:
		port_puts(out, "menuexec ");
		aiw_mes_expression_list_print(stmt->AIW_MENU_EXEC.exprs, out);
		port_puts(out, ";\n");
		break;
	case AIW_MES_STMT_35:
		port_printf(out, "OP_0x35 %u %u;\n", stmt->AIW_0x35.a, stmt->AIW_0x35.b);
		break;
	case AIW_MES_STMT_37:
		port_printf(out, "OP_0x37 %u;\n", stmt->JMP.addr);
		break;
	default:
		ERROR("Unhandled opcode: %d", stmt->aiw_op);
	}
}

// statements }}}
