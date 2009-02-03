
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "item.h"
#include "main.h"
#include "navit.h"
#include "vehicle.h"
#include "speech.h"
#include "gui.h"
#include "debug.h"
#include "callback.h"
#include "command.h"

/*
gui.fullscreen()
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
	char *var;
	int varlen;
	char *attrn;
	int attrnlen;
};

struct context {
	struct attr *attr;
	int error;
	char *expr;
	struct result res;
};

enum error {
	no_error=0,missing_closing_brace, missing_colon, wrong_type, illegal_number_format, illegal_character, missing_closing_bracket, invalid_type
};

static void eval_comma(struct context *ctx, struct result *res);
static struct attr ** eval_list(struct context *ctx);

static char *
get_op(struct context *ctx, int test, ...)
{
	char *op,*ret=NULL;
	va_list ap;
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

static int
is_int(struct result *res)
{
	return 1;
}

static int
is_double(struct result *res)
{
	return 0;
}

static void
dump(struct result *res)
{
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
command_object_get_attr(struct attr *object, enum attr_type attr_type, struct attr *ret)
{
	switch (object->type) {
	case attr_gui:
		return gui_get_attr(object->u.gui, attr_type, ret, NULL);
	case attr_navit:
		return navit_get_attr(object->u.navit, attr_type, ret, NULL);
	case attr_speech:
		return speech_get_attr(object->u.speech, attr_type, ret, NULL);
	case attr_vehicle:
		return vehicle_get_attr(object->u.vehicle, attr_type, ret, NULL);
	default:
		return 0;
	}
}

static void
command_get_attr(struct context *ctx, struct result *res)
{
	int result;
	enum attr_type attr_type=command_attr_type(res);
	result=command_object_get_attr(&res->attr, attr_type, &res->attr);
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
	newres->attr.type=attr_type;
	switch (res->attr.type) {
	case attr_navit:
		result=navit_set_attr(res->attr.u.navit, &newres->attr);
		break;
	case attr_speech:
		result=speech_set_attr(res->attr.u.speech, &newres->attr);
		break;
	default:
		break;
	}
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
resolve(struct context *ctx, struct result *res, struct attr *parent)
{
	resolve_object(ctx, res);
	if (res->attr.type >= attr_type_object_begin && res->attr.type <= attr_type_object_end) {
		if (res->attrn)
			command_get_attr(ctx, res);
		return;
	}
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
	res->val=val;
}

static void
set_int(struct context *ctx, struct result *res, int val)
{
	res->attr.type=attr_type_int_begin;
	res->attr.u.num=val;
}


static void
eval_value(struct context *ctx, struct result *res) {
	char *op;
	int dots=0;
	op=ctx->expr;
	res->varlen=0;
	res->var=NULL;
	res->attrnlen=0;
	res->attrn=NULL;
	if (op[0] >= 'a' && op[0] <= 'z') {
		res->attr.type=attr_none;
		res->var=op;
		while ((op[0] >= 'a' && op[0] <= 'z') || op[0] == '_') {
			res->varlen++;
			op++;
		}
		ctx->expr=op;
		return;
	}
	if ((op[0] >= '0' && op[0] <= '9') ||
	    (op[0] == '.' && op[1] >= '0' && op[1] <= '9')) {
		while ((op[0] >= '0' && op[0] <= '9') || op[0] == '.') {
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
	ctx->error=illegal_character;
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
	dbg(0,"function=%s\n", function);
	if (ctx->expr[0] != ')') {
		list=eval_list(ctx);	
		if (ctx->error) return;
	}
	if (!get_op(ctx,0,")",NULL)) {
		ctx->error=missing_closing_brace;
		return;
	}
	if (command_object_get_attr(&res->attr, attr_callback_list, &cbl)) {
		int valid;
		dbg(0,"function call %s from %s\n",function, attr_to_name(res->attr.type));
		callback_list_call_attr_4(cbl.u.callback_list, attr_command, function, list, NULL, &valid);
	}
	res->var=NULL;
	res->varlen=0;
	res->attrn=NULL;
	res->attrnlen=0;
	res->attr.type=attr_none;
}

static void
eval_postfix(struct context *ctx, struct result *res)
{
	struct result tmp;
	char *op;

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
			dbg(0,"function call\n");
			resolve_object(ctx, res);
			command_call_function(ctx, res);
		}
	}
}

static void
eval_unary(struct context *ctx, struct result *res) 
{
	char *op;
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
	char *op;

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
	char *op;

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
	char *op;

    	eval_additive(ctx, res);
	if (ctx->error) return;
	for (;;) {
		if (!(op=get_op(ctx,0,"==","!=",NULL))) return;
    		eval_additive(ctx, &tmp);
		if (ctx->error) return;
		if (op[0]  == '=') 
			set_int(ctx, res, (get_int(ctx, res) == get_int(ctx, &tmp)));
		else
			set_int(ctx, res, (get_int(ctx, res) != get_int(ctx, &tmp)));
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

void
command_evaluate_to_void(struct attr *attr, char *expr)
{
	struct result res;
	struct context ctx;
	dbg(0,"command=%s attr.type=%s\n", expr, attr_to_name(attr->type));
	memset(&res, 0, sizeof(res));
	memset(&ctx, 0, sizeof(ctx));
	ctx.attr=attr;
	ctx.error=0;
	ctx.expr=expr;
	eval_comma(&ctx,&res);
	resolve(&ctx, &res, NULL);
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
command_add_table(struct callback_list *cbl, struct command_table *table, int count, void *data)
{
	struct callback *cb=callback_new_attr_3(callback_cast(command_table_call),attr_command, table, count, data);
	callback_list_add(cbl, cb);
}

