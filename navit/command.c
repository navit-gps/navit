#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <glib.h>
#include "item.h"
#include "xmlconfig.h"
#include "main.h"
#include "navit.h"
#include "vehicle.h"
#include "speech.h"
#include "gui.h"
#include "debug.h"
#include "callback.h"
#include "command.h"
#include "event.h"
#include "navit_nls.h"

/*
gui.fullscreen=!gui.fullscreen
gui.menu()
gui.get_data() 
zoom_in() 
zoom_out()
speech.active=!speech.active
osd_configuration=1
Not yet:
osd[type=="xxx"].active=0;osd[type=="yyy"].active=0
*/


struct result {
	struct attr attr;
	double val;
	const char *var;
	int varlen;
	const char *attrn;
	int attrnlen;
	int allocated;
};

struct context {
	struct attr *attr;
	int error;
	const char *expr;
	struct result res;
};

struct command_saved_cb {
	struct callback *cb;
	struct attr attr;
};

struct command_saved {
	struct context ctx;
	struct result res;
	char *command;				// The command string itself
	struct event_idle *idle_ev;		// Event to update this command
	struct callback *idle_cb;
	struct callback *register_cb;			// Callback to register all the callbacks
	struct event_idle *register_ev;		// Idle event to register all the callbacks
	struct attr navit;
	int num_cbs;
	struct command_saved_cb *cbs;		// List of callbacks for this saved command
	struct callback *cb; // Callback that should be called when we re-evaluate
	int error;
};

enum error {
	no_error=0,missing_closing_brace, missing_colon, wrong_type, illegal_number_format, illegal_character, missing_closing_bracket, invalid_type, not_ready
};

static void eval_comma(struct context *ctx, struct result *res);
static struct attr ** eval_list(struct context *ctx);

static void
result_free(struct result *res)
{
}

static int command_register_callbacks(struct command_saved *cs);

static const char *
get_op(struct context *ctx, int test, ...)
{
	char *op;
	const char *ret=NULL;
	va_list ap;

	while (g_ascii_isspace(*ctx->expr)) {
		ctx->expr++;
	}

	va_start(ap, test);
	while ((op = va_arg(ap, char *))) {
		if (!strncmp(ctx->expr, op, strlen(op))) {
			ret=ctx->expr;
			if (! test)
				ctx->expr+=strlen(op);
			break;
		}
	}
	va_end(ap);
	return ret;
}

/*static int
is_int(struct result *res)
{
	return 1;
}*/

static int
is_double(struct result *res)
{
	return 0;
}

static void
dump(struct result *res)
{
#if 0
	char object[res->varlen+1];
	char attribute[res->attrnlen+1];
	if (res->var)
		strncpy(object, res->var, res->varlen);
	object[res->varlen]='\0';
	if (res->attrn)
		strncpy(attribute, res->attrn, res->attrnlen);
	attribute[res->attrnlen]='\0';
	dbg(0,"type:%s\n", attr_to_name(res->attr.type));
	dbg(0,"attribute '%s' from '%s'\n", attribute, object);
#endif
}

static enum attr_type
command_attr_type(struct result *res)
{
	char attrn[res->attrnlen+1];

	strncpy(attrn, res->attrn, res->attrnlen);
	attrn[res->attrnlen]='\0';
	return attr_from_name(attrn);
}

static int
command_object_get_attr(struct context *ctx, struct attr *object, enum attr_type attr_type, struct attr *ret)
{
	struct object_func *func=object_func_lookup(object->type);
	if (!func || !func->get_attr)
		return 0;
	return func->get_attr(object->u.data, attr_type, ret, NULL);
}

static void
command_get_attr(struct context *ctx, struct result *res)
{
	int result;
	enum attr_type attr_type=command_attr_type(res);
	result=command_object_get_attr(ctx, &res->attr, attr_type, &res->attr);
	if (result) {
		res->var=res->attrn;
		res->varlen=res->attrnlen;
	} else {
		res->attr.type=attr_none;
		res->var=NULL;
		res->varlen=0;
	}
	res->attrn=NULL;
	res->attrnlen=0;
	dump(res);
}

static void
command_set_attr(struct context *ctx, struct result *res, struct result *newres)
{
	int result=0;
	enum attr_type attr_type=command_attr_type(res);
	struct object_func *func=object_func_lookup(res->attr.type);
	if (!func || !func->set_attr)
		return;
	newres->attr.type=attr_type;
	result=func->set_attr(res->attr.u.data, &newres->attr);
	*res=*newres;
}

static void
resolve_object(struct context *ctx, struct result *res)
{
	if (res->attr.type == attr_none && res->varlen) {
		res->attr=*ctx->attr;
		res->attrn=res->var;
		res->attrnlen=res->varlen;
		res->var=NULL;
		res->varlen=0;
	}
}

static void
resolve(struct context *ctx, struct result *res, struct attr *parent) //FIXME What is that parent for?
{
	resolve_object(ctx, res);
	if (res->attrn)
		command_get_attr(ctx, res);
}

static double
get_double(struct context *ctx, struct result *res)
{
	resolve(ctx, res, NULL);
	return res->val;
}



static int
get_int(struct context *ctx, struct result *res)
{
	resolve(ctx, res, NULL);
	if (res->attr.type == attr_none)
		return 0;
	if (res->attr.type >= attr_type_int_begin && res->attr.type <= attr_type_int_end) {
		return res->attr.u.num;
	}
	if (res->attr.type >= attr_type_double_begin && res->attr.type <= attr_type_double_end) {
		return (int) (*res->attr.u.numd);
	}
	ctx->error=wrong_type;
	return 0;
}


static char *
get_string(struct context *ctx, struct result *res)
{
	resolve(ctx, res, NULL);
	return attr_to_text(&res->attr, NULL, 0);
}

static void
set_double(struct context *ctx, struct result *res, double val)
{
	result_free(res);
	res->val=val;
}

static void
set_int(struct context *ctx, struct result *res, int val)
{
	result_free(res);
	res->attr.type=attr_type_int_begin;
	res->attr.u.num=val;
}


static void
eval_value(struct context *ctx, struct result *res) {
	const char *op;
	int len,dots=0;
	op=ctx->expr;
	res->varlen=0;
	res->var=NULL;
	res->attrnlen=0;
	res->attrn=NULL;

	while (g_ascii_isspace(*op)) {
		op++;
	}

	if ((op[0] >= 'a' && op[0] <= 'z') || op[0] == '_') {
		res->attr.type=attr_none;
		res->var=op;
		for (;;) {
			while ((op[0] >= 'a' && op[0] <= 'z') || op[0] == '_') {
				res->varlen++;
				op++;
			}
			if (res->varlen == 3 && !strncmp(res->var,"new",3) && op[0] == ' ') {
				res->varlen++;
				op++;
			} else
				break;
		}
		ctx->expr=op;
		return;
	}
	if ((op[0] >= '0' && op[0] <= '9') ||
	    (op[0] == '.' && op[1] >= '0' && op[1] <= '9') ||
	    (op[0] == '-' && op[1] >= '0' && op[1] <= '9') ||
	    (op[0] == '-' && op[1] == '.' && op[2] >= '0' && op[2] <= '9')) {
		while ((op[0] >= '0' && op[0] <= '9') || op[0] == '.' || (res->varlen == 0 && op[0] == '-')) {
			if (op[0] == '.')
				dots++;
			if (dots > 1) {
				ctx->error=illegal_number_format;
				return;
			}
			res->varlen++;
			op++;
		}
		if (dots) {
			res->val = strtod(ctx->expr, NULL);
			res->attr.type=attr_type_double_begin;
			res->attr.u.numd=&res->val;
		} else {
			res->attr.type=attr_type_int_begin;
			res->attr.u.num=atoi(ctx->expr);
		}
		ctx->expr=op;
		return;
	}
	if (op[0] == '"') {
		do {
			op++;
		} while (op[0] != '"');
		res->attr.type=attr_type_string_begin;
		len=op-ctx->expr-1;
		res->attr.u.str=g_malloc(len+1);
		strncpy(res->attr.u.str, ctx->expr+1, len);
		res->attr.u.str[len]='\0';
		op++;
		ctx->expr=op;
		return;
	}
	ctx->error=illegal_character;
}

static int
get_next_object(struct context *ctx, struct result *res) {
	
	while (*ctx->expr) {
		res->varlen = 0;
		ctx->error = 0;

		eval_value(ctx,res);
		
		if ((res->attr.type == attr_none) && (res->varlen > 0)) {
			if (ctx->expr[0] != '.') {
				return 1;		// 1 means "this is the final object name"
			} else {
				return 2;		// 2 means "there are more object names following" (e.g. we just hit 'vehicle' in 'vehicle.position_speed'
			}
		}

		if (ctx->error) {
			// Probably hit an operator
			ctx->expr++;
		}
	}

	return 0;
}

static void
eval_brace(struct context *ctx, struct result *res)
{
	if (get_op(ctx,0,"(",NULL)) {
		eval_comma(ctx, res);
		if (ctx->error) return;
		if (!get_op(ctx,0,")",NULL)) 
			ctx->error=missing_closing_brace;
		return;
	}
	eval_value(ctx, res);
}

static void
command_call_function(struct context *ctx, struct result *res)
{
	struct attr cbl,**list=NULL;
	char function[res->attrnlen+1];
	if (res->attrn)
		strncpy(function, res->attrn, res->attrnlen);
	function[res->attrnlen]='\0';
	dbg(1,"function=%s\n", function);
	if (ctx->expr[0] != ')') {
		list=eval_list(ctx);	
		if (ctx->error) return;
	}
	if (!get_op(ctx,0,")",NULL)) {
		ctx->error=missing_closing_brace;
		return;
	}
	if (!strcmp(function,"_") && list && list[0] && list[0]->type >= attr_type_string_begin && list[0]->type <= attr_type_string_end) {
		res->attr.type=list[0]->type;
		res->attr.u.str=g_strdup(gettext(list[0]->u.str));	
		
	} else if (!strncmp(function,"new ",4)) {
		enum attr_type attr_type=attr_from_name(function+4);
		if (attr_type != attr_none) {
			struct object_func *func=object_func_lookup(attr_type);
			if (func && func->new) {
				res->attr.type=attr_type;
				res->attr.u.data=func->new(NULL, list);
			}
		}
	} else {
		if (command_object_get_attr(ctx, &res->attr, attr_callback_list, &cbl)) {
			int valid;
			struct attr **out=NULL;
			dbg(1,"function call %s from %s\n",function, attr_to_name(res->attr.type));
			callback_list_call_attr_4(cbl.u.callback_list, attr_command, function, list, &out, &valid);
			if (out && out[0]) {
				attr_dup_content(out[0], &res->attr);
				attr_list_free(out);
			} else
				res->attr.type=attr_none;
		} else
			res->attr.type=attr_none;
	}
	res->var=NULL;
	res->varlen=0;
	res->attrn=NULL;
	res->attrnlen=0;
}

static void
eval_postfix(struct context *ctx, struct result *res)
{
	struct result tmp;
	const char *op;

    	eval_brace(ctx, res);
	if (ctx->error) return;
	for (;;) {
		if (!(op=get_op(ctx,0,"[","(",".",NULL)))
			return;
		if (op[0] == '.') {
    			eval_brace(ctx, &tmp);
			if (ctx->error) return;
			resolve(ctx, res,NULL);
			if (ctx->error) return;
			res->attrn=tmp.var;
			res->attrnlen=tmp.varlen;
			dump(res);
		} else if (op[0] == '[') {
			if (!get_op(ctx,0,"]",NULL)) {
				ctx->error=missing_closing_bracket;
				return;
			}
		} else if (op[0] == '(') {
			dbg(1,"function call\n");
			resolve_object(ctx, res);
			command_call_function(ctx, res);
		}
	}
}

static void
eval_unary(struct context *ctx, struct result *res) 
{
	const char *op;
	op=get_op(ctx,0,"~","!",NULL);
	if (op) {
	    	eval_unary(ctx, res);
		if (ctx->error) return;
		if (op[0] == '~')
			set_int(ctx, res, ~get_int(ctx, res));
		else
			set_int(ctx, res, !get_int(ctx, res));
	} else
		eval_postfix(ctx, res);
}

static void
eval_multiplicative(struct context *ctx, struct result *res) 
{
	struct result tmp;
	const char *op;

    	eval_unary(ctx, res);
	if (ctx->error) return;
	for (;;) {
		if (!(op=get_op(ctx,0,"*","/","%",NULL))) return;
    		eval_unary(ctx, &tmp);
		if (ctx->error) return;
		if (is_double(res) || is_double(&tmp)) {
			if (op[0] == '*')
				set_double(ctx, res, get_double(ctx, res) * get_double(ctx, &tmp));
			else if (op[0] == '/')
				set_double(ctx, res, get_double(ctx, res) / get_double(ctx, &tmp));
			else {
				ctx->error=invalid_type;
				return;
			}
		} else {
			if (op[0] == '*')
				set_int(ctx, res, get_int(ctx, res) * get_int(ctx, &tmp));
			else if (op[0] == '/')
				set_int(ctx, res, get_int(ctx, res) / get_int(ctx, &tmp));
			else
				set_int(ctx, res, get_int(ctx, res) % get_int(ctx, &tmp));
		}
		if (ctx->error) return;
	}
}

static void
eval_additive(struct context *ctx, struct result *res) 
{
	struct result tmp;
	const char *op;

    	eval_multiplicative(ctx, res);
	if (ctx->error) return;
	for (;;) {
		if (!(op=get_op(ctx,0,"-","+",NULL))) return;
    		eval_multiplicative(ctx, &tmp);
		if (ctx->error) return;
		if (is_double(res) || is_double(&tmp)) {
			if (op[0] == '+')
				set_double(ctx, res, get_double(ctx, res) + get_double(ctx, &tmp));
			else
				set_double(ctx, res, get_double(ctx, res) - get_double(ctx, &tmp));
		} else {
			if (op[0] == '+')
				set_int(ctx, res, get_int(ctx, res) + get_int(ctx, &tmp));
			else
				set_int(ctx, res, get_int(ctx, res) - get_int(ctx, &tmp));
		}
		if (ctx->error) return;
	}
}

static void
eval_equality(struct context *ctx, struct result *res) 
{
	struct result tmp;
	const char *op;

    	eval_additive(ctx, res);
	if (ctx->error) return;
	for (;;) {
		if (!(op=get_op(ctx,0,"==","!=","<=",">=","<",">",NULL))) return;
    		eval_additive(ctx, &tmp);
		if (ctx->error) return;

		switch (op[0]) {
		case '=':
			set_int(ctx, res, (get_int(ctx, res) == get_int(ctx, &tmp)));
			break;
		case '!':
			set_int(ctx, res, (get_int(ctx, res) != get_int(ctx, &tmp)));
			break;
		case '<':
			if (op[1] == '=') {
				set_int(ctx, res, (get_int(ctx, res) <= get_int(ctx, &tmp)));
			} else {
				set_int(ctx, res, (get_int(ctx, res) < get_int(ctx, &tmp)));
			}
			break;
		case '>':
			if (op[1] == '=') {
				set_int(ctx, res, (get_int(ctx, res) >= get_int(ctx, &tmp)));
			} else {
				set_int(ctx, res, (get_int(ctx, res) > get_int(ctx, &tmp)));
			}
			break;
		default:
			break;
		}
		result_free(&tmp);
	}
}


static void
eval_bitwise_and(struct context *ctx, struct result *res) 
{
	struct result tmp;

    	eval_equality(ctx, res);
	if (ctx->error) return;
	for (;;) {
		if (get_op(ctx,1,"&&",NULL)) return;
		if (!get_op(ctx,0,"&",NULL)) return;
    		eval_equality(ctx, &tmp);
		if (ctx->error) return;
		set_int(ctx, res, get_int(ctx, res) & get_int(ctx, &tmp));
		if (ctx->error) return;
	}
}

static void
eval_bitwise_xor(struct context *ctx, struct result *res) 
{
	struct result tmp;

    	eval_bitwise_and(ctx, res);
	if (ctx->error) return;
	for (;;) {
		if (!get_op(ctx,0,"^",NULL)) return;
    		eval_bitwise_and(ctx, &tmp);
		if (ctx->error) return;
		set_int(ctx, res, get_int(ctx, res) ^ get_int(ctx, &tmp));
		if (ctx->error) return;
	}
}

static void
eval_bitwise_or(struct context *ctx, struct result *res) 
{
	struct result tmp;

    	eval_bitwise_xor(ctx, res);
	if (ctx->error) return;
	for (;;) {
		if (get_op(ctx,1,"||",NULL)) return;
		if (!get_op(ctx,0,"|",NULL)) return;
    		eval_bitwise_xor(ctx, &tmp);
		if (ctx->error) return;
		set_int(ctx, res, get_int(ctx, res) | get_int(ctx, &tmp));
		if (ctx->error) return;
	}
}

static void
eval_logical_and(struct context *ctx, struct result *res) 
{
	struct result tmp;

    	eval_bitwise_or(ctx, res);
	if (ctx->error) return;
	for (;;) {
		if (!get_op(ctx,0,"&&",NULL)) return;
    		eval_bitwise_or(ctx, &tmp);
		if (ctx->error) return;
		set_int(ctx, res, get_int(ctx, res) && get_int(ctx, &tmp));
		if (ctx->error) return;
	}
}

static void
eval_logical_or(struct context *ctx, struct result *res) 
{
	struct result tmp;

    	eval_logical_and(ctx, res);
	if (ctx->error) return;
	for (;;) {
		if (!get_op(ctx,0,"||",NULL)) return;
    		eval_logical_and(ctx, &tmp);
		if (ctx->error) return;
		set_int(ctx, res, get_int(ctx, res) || get_int(ctx, &tmp));
		if (ctx->error) return;
	}
}

static void
eval_conditional(struct context *ctx, struct result *res)
{
	struct result tmp;
	int cond;

    	eval_logical_or(ctx, res);
	if (ctx->error) return;
	if (!get_op(ctx,0,"?",NULL)) return;
	cond=!!get_int(ctx, res);
	if (ctx->error) return;
    	eval_logical_or(ctx, &tmp);
	if (ctx->error) return;
	if (cond)
		*res=tmp;
	if (!get_op(ctx,0,":",NULL)) {
		ctx->error=missing_colon;
		return;
	}
    	eval_logical_or(ctx, &tmp);
	if (ctx->error) return;
	if (!cond)
		*res=tmp;
}

/* = *= /= %= += -= >>= <<= &= ^= |= */

static void
eval_assignment(struct context *ctx, struct result *res)
{
	struct result tmp;
    	eval_conditional(ctx, res);
	if (ctx->error) return;
	if (!get_op(ctx,0,"=",NULL)) return;
    	eval_conditional(ctx, &tmp);
	if (ctx->error) return;
	resolve(ctx, &tmp, NULL);
	if (ctx->error) return;
	resolve_object(ctx, res);
	command_set_attr(ctx, res, &tmp);
}

/* , */
static void
eval_comma(struct context *ctx, struct result *res)
{
	struct result tmp;

	eval_assignment(ctx, res);
	if (ctx->error) return;
	for (;;) {
		if (!get_op(ctx,0,",",NULL)) return;
    		eval_assignment(ctx, &tmp);
		if (ctx->error) return;
		*res=tmp;
	}
}

static struct attr **
eval_list(struct context *ctx)
{
	struct result tmp;

	struct attr **ret=NULL;
	for (;;) {
    		eval_assignment(ctx, &tmp);
		if (ctx->error) {
			attr_list_free(ret);
			return NULL;
		}
		resolve(ctx, &tmp, NULL);
		ret=attr_generic_add_attr(ret, &tmp.attr);
		if (!get_op(ctx,0,",",NULL)) return ret;
	}
}

#if 0

void command(struct attr *attr, char *expr)
{
	struct result res;
	struct context ctx;
	memset(&res, 0, sizeof(res));
	memset(&ctx, 0, sizeof(ctx));
	ctx.attr=attr;
	ctx.error=0;
	ctx.expr=expr;
	printf("command='%s'\n", expr);
	eval_comma(&ctx,&res);
	printf("err=%d %s\n", ctx.error, ctx.expr);
	dump(&res);
	printf("***\n");
	resolve(&ctx, &res, NULL);
	dump(&res);
	printf("%s\n", get_string(&ctx, &res));
}
#endif

static void
command_evaluate_to(struct attr *attr, const char *expr, struct context *ctx, struct result *res)
{
	memset(res, 0, sizeof(*res));
	memset(ctx, 0, sizeof(*ctx));
	ctx->attr=attr;
	ctx->expr=expr;
	eval_comma(ctx,res);
}

void
command_evaluate_to_void(struct attr *attr, char *expr, int *error)
{
	struct result res;
	struct context ctx;
	command_evaluate_to(attr, expr, &ctx, &res);
	if (!ctx.error)
		resolve(&ctx, &res, NULL);
	if (error)
		*error=ctx.error;

}

char *
command_evaluate_to_string(struct attr *attr, char *expr, int *error)
{
	struct result res;
	struct context ctx;
	char *ret=NULL;

	command_evaluate_to(attr, expr, &ctx, &res);
	if (!ctx.error)
		resolve(&ctx, &res, NULL);
	if (!ctx.error)
		ret=get_string(&ctx, &res);
	if (error)
		*error=ctx.error;
	if (ctx.error)
		return NULL;
	else
		return ret;
}

int
command_evaluate_to_int(struct attr *attr, char *expr, int *error)
{
	struct result res;
	struct context ctx;
	int ret=0;

	command_evaluate_to(attr, expr, &ctx, &res);
	if (!ctx.error)
		resolve(&ctx, &res, NULL);
	if (!ctx.error)
		ret=get_int(&ctx, &res);
	if (error)
		*error=ctx.error;
	if (ctx.error)
		return 0;
	else
		return ret;
}

int
command_evaluate_to_boolean(struct attr *attr, const char *expr, int *error)
{
	struct result res;
	struct context ctx;
	int ret=0;

	command_evaluate_to(attr, expr, &ctx, &res);
	if (!ctx.error)
		resolve(&ctx, &res, NULL);
	if (!ctx.error) {
		if (res.attr.type == attr_none)
			ret=0;
		else if ((res.attr.type >= attr_type_int_begin && res.attr.type <= attr_type_int_end) ||
			 (res.attr.type >= attr_type_double_begin && res.attr.type <= attr_type_double_end))
			ret=get_int(&ctx, &res);
		else 
			ret=res.attr.u.data != NULL;
	}
	if (error)
		*error=ctx.error;
	if (ctx.error)
		return 0;
	else	
		return ret;
}

void
command_evaluate(struct attr *attr, const char *expr)
{
	struct result res;
	struct context ctx;
	memset(&res, 0, sizeof(res));
	memset(&ctx, 0, sizeof(ctx));
	ctx.attr=attr;
	ctx.error=0;
	ctx.expr=expr;
	for (;;) {
		eval_comma(&ctx,&res);
		if (ctx.error)
			return;
		resolve(&ctx, &res, NULL);
		if (ctx.error)
			return;
		if (!get_op(&ctx,0,";",NULL)) return;
	}
}

#if 0
void
command_interpreter(struct attr *attr)
{
                char buffer[4096];
                int size;
                for (;;) {
                size=read(0, buffer, 4095);
                buffer[size]='\0';
                if (size) {
                        buffer[size-1]='\0';
                }
                command(attr, buffer);
                }
}
#endif

static void
command_table_call(struct command_table *table, int count, void *data, char *command, struct attr **in, struct attr ***out, int *valid)
{
	int i;
	for (i = 0 ; i < count ; i++) {
		if (!strcmp(command,table->command)) {
			if (valid)
				*valid=1;
			table->func(data, command, in, out);
		}
		table++;
	}
}

void
command_add_table_attr(struct command_table *table, int count, void *data, struct attr *attr)
{
	attr->type=attr_callback;
	attr->u.callback=callback_new_attr_3(callback_cast(command_table_call),attr_command, table, count, data);
}

void
command_add_table(struct callback_list *cbl, struct command_table *table, int count, void *data)
{
	struct attr attr;
	command_add_table_attr(table, count, data, &attr);
	callback_list_add(cbl, attr.u.callback);
}

void
command_saved_set_cb (struct command_saved *cs, struct callback *cb)
{
	cs->cb = cb;
}

int
command_saved_get_int (struct command_saved *cs)
{
	return get_int(&cs->ctx, &cs->res);
}

int 
command_saved_error (struct command_saved *cs)
{
	return cs->error;
}

static void
command_saved_evaluate_idle (struct command_saved *cs) 
{
	// Only run once at a time
	if (cs->idle_ev) {
		event_remove_idle(cs->idle_ev);
		cs->idle_ev = NULL;
	}

	command_evaluate_to(&cs->navit, cs->command, &cs->ctx, &cs->res);

	if (!cs->ctx.error) {
		cs->error = 0;

		if (cs->cb) {
			callback_call_1(cs->cb, cs);
		}
	} else {
		cs->error = cs->ctx.error;
	}
}

static void
command_saved_evaluate(struct command_saved *cs)
{	
	if (cs->idle_ev) {
		// We're already scheduled for reevaluation
		return;
	}

	if (!cs->idle_cb) {
		cs->idle_cb = callback_new_1(callback_cast(command_saved_evaluate_idle), cs);
	}

	cs->idle_ev = event_add_idle(100, cs->idle_cb);
}

static void
command_saved_callbacks_changed(struct command_saved *cs)
{
	// For now, we delete each and every callback and then re-create them
	int i;
	struct object_func *func;
	struct attr attr;

	if (cs->register_ev) {
		event_remove_idle(cs->register_ev);
		cs->register_ev = NULL;
	}

	attr.type = attr_callback;

	for (i = 0; i < cs->num_cbs; i++) {
		func = object_func_lookup(cs->cbs[i].attr.type);
		
		if (!func->remove_attr) {
			dbg(0, "Could not remove command-evaluation callback because remove_attr is missing for type %i!\n", cs->cbs[i].attr.type);
			continue;
		}

		attr.u.callback = cs->cbs[i].cb;

		func->remove_attr(cs->cbs[i].attr.u.data, &attr);
		callback_destroy(cs->cbs[i].cb);
	}

	g_free(cs->cbs);
	cs->cbs = NULL;
	cs->num_cbs = 0;

	// Now, re-create all the callbacks
	command_register_callbacks(cs);
}

static int
command_register_callbacks(struct command_saved *cs)
{
	struct attr prev,cb_attr,attr;
	int status;
	struct object_func *func;
	struct callback *cb;
	
	attr = cs->navit;
	cs->ctx.expr = cs->command;
	cs->ctx.attr = &attr;
	prev = cs->navit;	

	while ((status = get_next_object(&cs->ctx, &cs->res)) != 0) {
		resolve(&cs->ctx, &cs->res, NULL);

		if (cs->ctx.error || (cs->res.attr.type == attr_none)) {
			// We could not resolve an object, perhaps because it has not been created
			return 0;
		}

		if (prev.type != attr_none) {
			func = object_func_lookup(prev.type);

			if (func->add_attr) {
				if (status == 2) { // This is not the final attribute name
					cb = callback_new_attr_1(callback_cast(command_saved_callbacks_changed), cs->res.attr.type, (void*)cs);
					attr = cs->res.attr;
				} else if (status == 1) { // This is the final attribute name
					cb = callback_new_attr_1(callback_cast(command_saved_evaluate), cs->res.attr.type, (void*)cs);
					cs->ctx.attr = &cs->navit;
				} else {
					dbg(0, "Error: Strange status returned from get_next_object()\n");
				}

				cs->num_cbs++;
				cs->cbs = g_realloc(cs->cbs, (sizeof(struct command_saved_cb) * cs->num_cbs));
				cs->cbs[cs->num_cbs-1].cb = cb;
				cs->cbs[cs->num_cbs-1].attr = prev;
					
				cb_attr.u.callback = cb;
				cb_attr.type = attr_callback;

				func->add_attr(prev.u.data, &cb_attr);

			} else {
				dbg(0, "Could not add callback because add_attr is missing for type %i}n", prev.type);
			}
		}

		if (status == 2) {
			prev = cs->res.attr;
		} else {
			prev = cs->navit;
		}
	}

	command_saved_evaluate_idle(cs);

	return 1;
}

struct command_saved
*command_saved_new(char *command, struct navit *navit, struct callback *cb)
{
	struct command_saved *ret;

	ret = g_new0(struct command_saved, 1);
	ret->command = g_strdup(command);
	ret->navit.u.navit = navit;
	ret->navit.type = attr_navit;
	ret->cb = cb;
	ret->error = not_ready;

	if (!command_register_callbacks(ret)) {
		// We try this as an idle call again
		ret->register_cb = callback_new_1(callback_cast(command_saved_callbacks_changed), ret);
		ret->register_ev = event_add_idle(300, ret->register_cb);
	}		

	return ret;
}

void 
command_saved_destroy(struct command_saved *cs)
{
	g_free(cs->command);
	g_free(cs);
}
