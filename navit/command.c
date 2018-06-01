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
osd[@type=="xxx"].active=0;osd[@type=="yyy"].active=0
*/


/**
 * The result, or interim result, of evaluating a saved command.
 */
struct result {
    struct attr attr;	/**< The attribute. If {@code allocated} is true, it stores an object that was
						 *   successfully retrieved. Else it is either a placeholder or a constant value.
						 */
    double val;
    const char *var;	/**< If {@code allocated} is false, the name of the object to be resolved.
						 *   Else, it is the name of the object successfully retrieved and stored in
						 *   {@code attr}, or {@code NULL} if retrieval failed.
						 *   Only the first {@code varlen} characters are significant.
						 */
    int varlen;			/**< Number of significant characters in {@code var} */
    const char *attrn;	/**< The name of an object that has been resolved but not yet retrieved,
						 *   {@code NULL} otherwise. Only the first {@code attrnlen} characters are
						 *   significant.
						 */
    int attrnlen;		/**< Number of significant characters in {@code attrn} */
    int allocated;		/**< Whether the result has been calculated */
};

struct result_list {
    struct attr **attrs;
};

struct context {
    struct attr *attr;
    int error;
    int skip;
    const char *expr;
    struct result res;
};

/**
 * Information about a callback function for a saved command.
 */
struct command_saved_cb {
    struct callback *cb;	/**< The callback function */
    struct attr attr;
};

/**
 * A saved command.
 */
struct command_saved {
    struct context ctx;
    struct result res;
    char *command;				/**< The command string itself **/
    struct event_idle *idle_ev;		/**< Event to update this command **/
    struct callback *idle_cb;
    struct callback *register_cb;			/**< Callback to register all the callbacks **/
    struct event_idle *register_ev;		/**< Idle event to register all the callbacks **/
    struct attr context_attr;			/**< The root of the object hierarchy, which will be assumed as
										 *   the parent of all unqualified or partially qualified object
										 *   references. **/
    int num_cbs;						/**< Number of entries in {@code cbs} **/
    struct command_saved_cb *cbs;		/**< List of callbacks for this saved command **/
    struct callback *cb; /**< Callback that should be called when we re-evaluate **/
    int error;
    int async;
};

enum error {
    no_error=0, missing_double_quote, missing_opening_parenthesis, missing_closing_parenthesis, missing_closing_brace, missing_colon, missing_semicolon, wrong_type, illegal_number_format, illegal_character, missing_closing_bracket, invalid_type, not_ready, internal, eof_reached
};

enum op_type {
    op_type_binary, op_type_prefix, op_type_suffix
};

enum set_type {
    set_type_symbol, set_type_string, set_type_integer, set_type_float
};



static void eval_comma(struct context *ctx, struct result *res);
static struct attr ** eval_list(struct context *ctx);

/**
 * @brief Converts an error to human-readable text.
 *
 * @param err The error code
 *
 * @return A string containing the error description. The caller is responsible for freeing up the string by
 * calling {@code g_free()} when it is no longer needed.
 */
char *command_error_to_text(int err) {
    switch (err) {
    case no_error:
        return g_strdup("no_error");
    case missing_double_quote:
        return g_strdup("missing_double_quote");
    case missing_opening_parenthesis:
        return g_strdup("missing_opening_parenthesis");
    case missing_closing_parenthesis:
        return g_strdup("missing_closing_parenthesis");
    case missing_closing_brace:
        return g_strdup("missing_closing_brace");
    case missing_colon:
        return g_strdup("missing_colon");
    case missing_semicolon:
        return g_strdup("missing_semicolon");
    case wrong_type:
        return g_strdup("wrong_type");
    case illegal_number_format:
        return g_strdup("illegal_number_format");
    case illegal_character:
        return g_strdup("illegal_character");
    case missing_closing_bracket:
        return g_strdup("missing_closing_bracket");
    case invalid_type:
        return g_strdup("invalid_type");
    case not_ready:
        return g_strdup("not_ready");
    case internal:
        return g_strdup("internal");
    case eof_reached:
        return g_strdup("eof_reached");
    default:
        return g_strdup("unknown");
    }
}

static void result_free(struct result *res) {
    if(res->allocated) {
        attr_free_content(&res->attr);
        res->allocated=0;
    } else {
        res->attr.type=type_none;
        res->attr.u.data=NULL;
    }
}



static int command_register_callbacks(struct command_saved *cs);

static const char *get_op(struct context *ctx, int test, ...) {
    char *op;
    const char *ret=NULL;
    va_list ap;

    while (*ctx->expr && g_ascii_isspace(*ctx->expr)) {
        ctx->expr++;
    }

    va_start(ap, test);
    while ((op = va_arg(ap, char *))) {
        if (!strncmp(ctx->expr, op, strlen(op))) {
            ret=op;
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

static int is_double(struct result *res) {
    return 0;
}

static void dump(struct result *res) {
#if 0
    char object[res->varlen+1];
    char attribute[res->attrnlen+1];
    if (res->var)
        strncpy(object, res->var, res->varlen);
    object[res->varlen]='\0';
    if (res->attrn)
        strncpy(attribute, res->attrn, res->attrnlen);
    attribute[res->attrnlen]='\0';
    dbg(lvl_debug,"type:%s", attr_to_name(res->attr.type));
    dbg(lvl_debug,"attribute '%s' from '%s'", attribute, object);
#endif
}

static enum attr_type command_attr_type(struct result *res) {
    char *attrn=g_alloca(sizeof(char)*(res->attrnlen+1));

    strncpy(attrn, res->attrn, res->attrnlen);
    attrn[res->attrnlen]='\0';
    return attr_from_name(attrn);
}

/**
 * @brief Retrieves an attribute from an object.
 *
 * This function will retrieve the first matching attribute by calling the {@code get_attr} method for
 * the object type. If {@code object} does not refer to a valid object, or the {@code get_attr} method
 * for the object type could not be defined, the function fails and zero is returned.
 *
 * @param ctx The context (ignored)
 * @param object The object for which the attribute is to be retrieved.
 * @param attr_type The type of attribute to retrieve
 * @param ret Points to a {@code struct attr} to which the attribute will be copied
 *
 * @return True if a matching attribute was found, false if no matching attribute was found or an error
 * occurred
 */
static int command_object_get_attr(struct context *ctx, struct attr *object, enum attr_type attr_type,
                                   struct attr *ret) {
    int r;
    struct attr dup;
    struct object_func *func=object_func_lookup(object->type);
    if (!object->u.data || !func || !func->get_attr) {
        dbg(lvl_warning, "cannot retrieve attributes from %s (%p), func=%p", attr_to_name(object->type), object->u.data, func)
        return 0;
    }
    r=func->get_attr(object->u.data, attr_type, &dup, NULL);
    if(r)
        attr_dup_content(&dup,ret);
    else
        dbg(lvl_warning, "%s (%p) has no attribute %s", attr_to_name(object->type), object->u.data, attr_to_name(attr_type))
        return r;
}

static int command_object_add_attr(struct context *ctx, struct attr *object, struct attr *attr) {
    struct object_func *func=object_func_lookup(object->type);
    if (!object->u.data || !func || !func->add_attr)
        return 0;
    return func->add_attr(object->u.data, attr);
}

static int command_object_remove_attr(struct context *ctx, struct attr *object, struct attr *attr) {
    struct object_func *func=object_func_lookup(object->type);
    if (!object->u.data || !func || !func->remove_attr)
        return 0;
    return func->remove_attr(object->u.data, attr);
}


/**
 * @brief Retrieves the current value of an attribute and stores it in {@code res}.
 *
 * If {@code ctx->skip} is true, the function aborts and no action is taken.
 *
 * Before calling this function, object references in {@code res} must be resolved. That is,
 * {@code res->attr} holds a copy of {@code ctx->attr} and the first {@code res->attrnlen} characters of
 * {@code res->attrn} correspond to the object name.
 *
 * After this function completes, {@code res->allocated} is true, and {@code res->attrn} and
 * {@code res->attrnlen} are reset.
 *
 * If the attribute was successfully retrieved, the first {@code res->varlen} characters of
 * {@code res->var} correspond to an object name and {@code res->attr} holds the attribute.
 *
 * If the attribute could not be retrieved, {@code res->attr.type} is set to {@code attr_none}, and
 * {@code res->var} and {@code res->varlen} are reset.
 *
 * @param ctx The context
 * @param res The result
 */
static void command_get_attr(struct context *ctx, struct result *res) {
    int result;
    struct result tmp= {{0,},};
    enum attr_type attr_type=command_attr_type(res);
    enum attr_type parent_type = res->attr.type; /* for debugging only */
    if (ctx->skip)
        return;
    result=command_object_get_attr(ctx, &res->attr, attr_type, &tmp.attr);
    result_free(res);
    *res=tmp;
    res->allocated=1;
    if (result) {
        dbg(lvl_debug, "successfully retrieved '%s' from '%s'", attr_to_name(attr_type), attr_to_name(parent_type));
        res->var=res->attrn;
        res->varlen=res->attrnlen;
    } else {
        dbg(lvl_warning, "could not retrieve '%s' from '%s'", attr_to_name(attr_type), attr_to_name(parent_type));
        result_free(res);
        res->attr.type=attr_none;
        res->var=NULL;
        res->varlen=0;
    }
    res->attrn=NULL;
    res->attrnlen=0;
    dump(res);
}

static void command_set_attr(struct context *ctx, struct result *res, struct result *newres) {
    enum attr_type attr_type=command_attr_type(res);
    struct object_func *func=object_func_lookup(res->attr.type);
    if (ctx->skip)
        return;
    if (!res->attr.u.data || !func || !func->set_attr)
        return;
    if (attr_type == attr_attr_types) {
        char *attrn=g_alloca(sizeof(char)*(res->attrnlen+1));
        struct attr *tmp;
        strncpy(attrn, res->attrn, res->attrnlen);
        attrn[res->attrnlen]='\0';
        tmp=attr_new_from_text(attrn, newres->attr.u.str);
        newres->attr.u.data=tmp->u.data;
        newres->allocated=1;
        g_free(tmp);
    }
    newres->attr.type=attr_type;
    func->set_attr(res->attr.u.data, &newres->attr);
    result_free(res);
    *res=*newres;
}

/**
 * @brief Resolves an object reference.
 *
 * Prior to calling this function, {@code res} must contain a valid, unresolved object reference:
 * {@code res->attr.type} must be {@code attr_none}, and the first {@code res->varlen} characters of
 * {@code res->var} must correspond to an object name.
 *
 * After the function returns, {@code res->attr} holds a copy of {@code ctx->attr} and the first
 * {@code res->attrnlen} characters of {@code res->attrn} correspond to the object name.
 * {@code res->var} and {@code res->varlen} are reset.
 *
 * @param ctx The context
 * @param res The result
 */
static void resolve_object(struct context *ctx, struct result *res) {
    if (res->attr.type == attr_none && res->varlen) {
        res->attr=*ctx->attr;
        res->attrn=res->var;
        res->attrnlen=res->varlen;
        res->var=NULL;
        res->varlen=0;
    }
}

/**
 * @brief Resolves and retrieves an object and stores it in {@code res}.
 *
 * Prior to calling this function, {@code res} must contain a valid, unresolved object reference:
 * {@code res->attr.type} must be {@code attr_none}, and the first {@code res->varlen} characters of
 * {@code res->var} must correspond to an object name.
 *
 * If {@code ctx->skip} is true, the object reference will be resolved but the object will not be
 * retrieved: the first {@code res->attrnlen} characters of {@code res->attrn} correspond to the object
 * name after the function returns, while {@code res->var} and {@code res->varlen} are reset.
 *
 * If {@code ctx->skip} is false, {@code res->allocated} is true after this function completes. The
 * object is stored in {@code res->attr} if it was successfully retrieved, otherwise {@code res->var}
 * and {@code res->varlen} are reset.
 *
 * @param ctx The context
 * @param res The result
 */
static void resolve(struct context *ctx, struct result *res) {
    resolve_object(ctx, res);
    if (res->attrn)
        command_get_attr(ctx, res);
}

static double get_double(struct context *ctx, struct result *res) {
    resolve(ctx, res);
    return res->val;
}


/**
 * @brief Returns an integer or bool representation of the result of an expression
 *
 * This function evaluates the result of an expression ({@code res->attr}).
 *
 * If {@code res->attr} is of a numeric type, its integer part is returned.
 *
 * If {@code is_bool} is false and {@code res->attr} is not of a numeric type, 0 is returned.
 *
 * If {@code is_bool} is true and {@code res->attr} is of an object or string type, true is returned
 * for non-null, false otherwise.
 *
 * For all other types of {@code res->attr}, 0 (false) is returned.
 *
 * @param ctx The context for the expression
 * @param is_bool If true, return boolean representation, else return integer representation. See description.
 * @param res The result of the evaluation
 *
 * @return The result of the expression, see description.
 */
static int get_int_bool(struct context *ctx, int is_bool, struct result *res) {
    resolve(ctx, res);
    if (res->attr.type == attr_none)
        return 0;
    if (res->attr.type >= attr_type_int_begin && res->attr.type <= attr_type_int_end) {
        return res->attr.u.num;
    }
    if (res->attr.type >= attr_type_double_begin && res->attr.type <= attr_type_double_end) {
        return (int) (*res->attr.u.numd);
    }
    if (is_bool && ATTR_IS_OBJECT(res->attr.type))
        return res->attr.u.data != NULL;
    if (is_bool && ATTR_IS_STRING(res->attr.type))
        return res->attr.u.data != NULL;
    dbg(lvl_debug,"bool %d %s",is_bool,attr_to_name(res->attr.type));
    ctx->error=wrong_type;
    return 0;
}

/**
 * @brief Returns an integer representation of the result of an expression
 *
 * This function is a wrapper around {@code get_int_bool()}. It is equivalent to
 * {@code get_int_bool(ctx, 0, res)}. See {@code get_int_bool()} for a description.
 */
static int get_int(struct context *ctx, struct result *res) {
    return get_int_bool(ctx, 0, res);
}

/**
 * @brief Returns a boolean representation of the result of an expression
 *
 * This function is a wrapper around {@code get_int_bool()}. It is equivalent to
 * {@code get_int_bool(ctx, 1, res)}. See {@code get_int_bool()} for a description.
 */
static int get_bool(struct context *ctx, struct result *res) {
    return !!get_int_bool(ctx, 1, res);
}


static char *get_string(struct context *ctx, struct result *res) {
    resolve(ctx, res);
    return attr_to_text(&res->attr, NULL, 0);
}

static void set_double(struct result *res, double val) {
    result_free(res);
    res->attr.type=attr_type_double_begin;
    res->attr.u.numd=&res->val;
    res->val=val;
}

static void set_int(struct result *res, int val) {
    result_free(res);
    res->attr.type=attr_type_int_begin;
    res->attr.u.num=val;
}

static void result_op(struct context *ctx, enum op_type op_type, const char *op, struct result *inout,
                      struct result *in) {
    if (ctx->skip)
        return;
    switch (op_type) {
    case op_type_prefix:
        switch ((op[0] << 8) | op[1]) {
        case ('!' << 8):
            set_int(inout, !get_bool(ctx, inout));
            return;
        case ('~' << 8):
            set_int(inout, ~get_int(ctx, inout));
            return;
        }
        break;
    case op_type_binary:
        resolve(ctx, inout);
        resolve(ctx, in);
        switch ((op[0] << 8) | op[1]) {
        case ('=' << 8)|'=':
            if (inout->attr.type == attr_none || in->attr.type == attr_none) {
                set_int(inout, 0);
            } else if (ATTR_IS_STRING(inout->attr.type) && ATTR_IS_STRING(in->attr.type)) {
                char *s1=get_string(ctx, inout),*s2=get_string(ctx, in);
                set_int(inout, (!strcmp(s1,s2)));
                g_free(s1);
                g_free(s2);
            } else if (ATTR_IS_OBJECT(inout->attr.type) && ATTR_IS_OBJECT(in->attr.type)) {
                set_int(inout, inout->attr.u.data == in->attr.u.data);
            } else
                set_int(inout, (get_int(ctx, inout) == get_int(ctx, in)));
            return;
        case ('!' << 8)|'=':
            if (inout->attr.type == attr_none || in->attr.type == attr_none) {
                set_int(inout, 1);
            } else if (ATTR_IS_STRING(inout->attr.type) && ATTR_IS_STRING(in->attr.type)) {
                char *s1=get_string(ctx, inout),*s2=get_string(ctx, in);
                set_int(inout, (!!strcmp(s1,s2)));
                g_free(s1);
                g_free(s2);
            } else if (ATTR_IS_OBJECT(inout->attr.type) && ATTR_IS_OBJECT(in->attr.type)) {
                set_int(inout, inout->attr.u.data != in->attr.u.data);
            } else
                set_int(inout, (get_int(ctx, inout) != get_int(ctx, in)));
            return;
        case ('<' << 8):
            set_int(inout, (get_int(ctx, inout) < get_int(ctx, in)));
            return;
        case ('<' << 8)|'=':
            set_int(inout, (get_int(ctx, inout) <= get_int(ctx, in)));
            return;
        case ('>' << 8):
            set_int(inout, (get_int(ctx, inout) > get_int(ctx, in)));
            return;
        case ('>' << 8)|'=':
            set_int(inout, (get_int(ctx, inout) >= get_int(ctx, in)));
            return;
        case ('*' << 8):
            if (is_double(inout) || is_double(in))
                set_double(inout, get_double(ctx, inout) * get_double(ctx, in));
            else
                set_int(inout, get_int(ctx, inout) * get_int(ctx, in));
            return;
        case ('/' << 8):
            if (is_double(inout) || is_double(in))
                set_double(inout, get_double(ctx, inout) * get_double(ctx, in));
            else
                set_int(inout, get_int(ctx, inout) * get_int(ctx, in));
            return;
        case ('%' << 8):
            set_int(inout, get_int(ctx, inout) % get_int(ctx, in));
            return;
        case ('+' << 8):
            if (is_double(inout) || is_double(in))
                set_double(inout, get_double(ctx, inout) + get_double(ctx, in));
            else if (ATTR_IS_STRING(inout->attr.type) && ATTR_IS_STRING(in->attr.type)) {
                char *str=g_strdup_printf("%s%s",inout->attr.u.str,in->attr.u.str);
                result_free(inout);
                inout->attr.type=attr_type_string_begin;
                inout->attr.u.str=str;
                inout->allocated=1;
            } else
                set_int(inout, get_int(ctx, inout) + get_int(ctx, in));
            return;
        case ('-' << 8):
            if (is_double(inout) || is_double(in))
                set_int(inout, get_int(ctx, inout) - get_int(ctx, in));
            else
                set_double(inout, get_double(ctx, inout) - get_double(ctx, in));
            return;
        case ('&' << 8):
            set_int(inout, get_int(ctx, inout) & get_int(ctx, in));
            return;
        case ('^' << 8):
            set_int(inout, get_int(ctx, inout) ^ get_int(ctx, in));
            return;
        case ('|' << 8):
            set_int(inout, get_int(ctx, inout) | get_int(ctx, in));
            return;
        case (('&' << 8) | '&'):
            set_int(inout, get_int(ctx, inout) && get_int(ctx, in));
            return;
        case (('|' << 8) | '|'):
            set_int(inout, get_int(ctx, inout) || get_int(ctx, in));
            return;
        default:
            break;
        }
    default:
        break;
    }
    dbg(lvl_error,"Unknown op %d %s",op_type,op);
    ctx->error=internal;
}

static void result_set(struct context *ctx, enum set_type set_type, const char *op, int len, struct result *out) {
    if (ctx->skip)
        return;
    switch (set_type) {
    case set_type_symbol:
        out->attr.type=attr_none;
        out->var=op;
        out->varlen=len;
        return;
    case set_type_integer:
        out->attr.type=attr_type_int_begin;
        out->attr.u.num=atoi(ctx->expr);
        return;
    case set_type_float:
        out->val = strtod(ctx->expr, NULL);
        out->attr.type=attr_type_double_begin;
        out->attr.u.numd=&out->val;
        return;
    case set_type_string:
        if (len >= 2) {
            out->attr.type=attr_type_string_begin;
            out->attr.u.str=g_malloc(len-1);
            strncpy(out->attr.u.str, op+1, len-2);
            out->attr.u.str[len-2]='\0';
            out->allocated=1;
            return;
        }
        break;
    default:
        break;
    }
    dbg(lvl_error,"unknown set type %d %s",set_type,op);
    ctx->error=internal;
}

/**
 * @brief Evaluates a value and stores its result.
 *
 * This function evaluates the first value in {@code ctx->expr}. A value can be either an object name
 * (such as {@code vehicle.position_speed}) or a literal value.
 *
 * If evaluation is successful, the result is stored in {@code res->attr}.
 *
 * If an object name is encountered, the result has an attribute type of {@code attr_none} and the first
 * {@code res->varlen} characters of {@code res->var} will point to the object name.
 *
 * If a literal value is encountered, the result's attribute type is set to the corresponding generic
 * data type and its value is stored with the attribute.
 *
 * After this function returns, {@code ctx->expr} contains the rest of the expression string, which was
 * not evaluated. Leading spaces before the value will be discarded with the value.
 *
 * If {@code ctx->expr}, after eliminating any leading whitespace, does not begin with a valid value,
 * one of the following errors is stored in {@code ctx->error}:
 * <ul>
 * <li>{@code illegal_number_format} An illegal number format, such as a second decimal dot, was
 * encountered.</li>
 * <li>{@code missing_double_quote} A double quote without a matching second double quote was found.</li>
 * <li>{@code eof_reached} The expression string is empty.</li>
 * <li>{@code illegal_character} The expression string begins with a character which is illegal in a
 * value. This may happen when the expression string begins with an operator.</li>
 * </ul>
 *
 * @param ctx The context to evaluate
 * @param res Points to a {@code struct res} in which the result will be stored
 */
static void eval_value(struct context *ctx, struct result *res) {
    const char *op;
    int dots=0;

    result_free(res);

    res->varlen=0;
    res->var=NULL;
    res->attrnlen=0;
    res->attrn=NULL;

    while (g_ascii_isspace(*(ctx->expr))) {
        ctx->expr++;
    }
    op = ctx->expr;

    if ((op[0] >= 'a' && op[0] <= 'z') || (op[0] >= 'A' && op[0] <= 'Z') || op[0] == '_') {
        const char *s=op;
        for (;;) {
            while ((op[0] >= 'a' && op[0] <= 'z') || (op[0] >= 'A' && op[0] <= 'Z') || (op[0] >= '0' && op[0] <= '9')
                    || op[0] == '_') {
                op++;
            }
            if (op-s == 3 && !strncmp(s,"new",3) && op[0] == ' ') {
                op++;
            } else
                break;
        }
        result_set(ctx, set_type_symbol, ctx->expr, op-ctx->expr, res);
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
            op++;
        }
        result_set(ctx, dots?set_type_float:set_type_integer, ctx->expr, op-ctx->expr, res);
        ctx->expr=op;
        return;
    }
    if (op[0] == '"') {
        int escaped=0;
        do {
            if (op[0] == '\\') {
                escaped=1;
                if (op[1] == '"')
                    op++;
            }
            op++;
        } while (op[0] && op[0] != '"');
        if(!*op) {
            ctx->error=missing_double_quote;
            return;
        }
        op++;
        if (escaped) {
            char *tmpstr=g_malloc(op-ctx->expr+1),*s=tmpstr;
            op=ctx->expr;
            do {
                if (op[0] == '\\')
                    op++;
                *s++=*op++;
            } while (op[0] != '"');
            *s++=*op++;
            result_set(ctx, set_type_string, tmpstr, s-tmpstr, res);
            g_free(tmpstr);
        } else
            result_set(ctx, set_type_string, ctx->expr, op-ctx->expr, res);
        ctx->expr=op;
        return;
    }
    if (!*op)
        ctx->error=eof_reached;
    else {
        /*
         * If we get here, ctx->expr does not begin with a variable or a literal value. This is not an
         * error if this function is being called to test if an expression begins with a value.
         */
        dbg(lvl_debug, "character 0x%x is illegal in a value",*op);
        ctx->error=illegal_character;
    }
}

/**
 * @brief Retrieves the next object reference from an expression.
 *
 * This function scans the expression string {@code ctx->expr} for the next object reference. Anything
 * other than an object reference (whitespace characters, literal values, operators and even illegal
 * characters) is discarded until either the end of the string is reached or an object reference is
 * encountered.
 *
 * After this function completes successfully, {@code res->attr.type} is {@code attr_none} and the first
 * {@code res->varlen} characters of {@code res->var} point to the object name.
 *
 * Object names retrieved by this function are unqualified, i.e. {@code vehicle.position_speed} will be
 * retrieved as {@code vehicle} on the first call (return value 2) and {@code position_speed} on the
 * second call (return value 1).
 *
 * @param ctx The context
 * @param res Points to a {@code struct result} where the result will be stored.
 *
 * @return If a complete object name has been retrieved, the return value is 1. If a partial object name
 * has been retrieved (e.g. {@code vehicle} from {@code vehicle.position_speed}), the return value is 2.
 * If no object references were found, the return value is 0.
 */
static int get_next_object(struct context *ctx, struct result *res) {

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

static void eval_brace(struct context *ctx, struct result *res) {
    if (get_op(ctx,0,"(",NULL)) {
        eval_comma(ctx, res);
        if (ctx->error) return;
        if (!get_op(ctx,0,")",NULL))
            ctx->error=missing_closing_parenthesis;
        return;
    }
    eval_value(ctx, res);
}

static void command_call_function(struct context *ctx, struct result *res) {
    struct attr cbl,**list=NULL;
    char *function=g_alloca(sizeof(char)*(res->attrnlen+1));
    if (res->attrn)
        strncpy(function, res->attrn, res->attrnlen);
    function[res->attrnlen]='\0';
    dbg(lvl_debug,"function=%s", function);
    if (ctx->expr[0] != ')') {
        list=eval_list(ctx);
        if (ctx->error) {
            attr_list_free(list);
            return;
        }
    }
    if (!get_op(ctx,0,")",NULL)) {
        attr_list_free(list);
        ctx->error=missing_closing_parenthesis;
        return;
    }
    if (!ctx->skip) {
        if (!strcmp(function,"_") && list && list[0] && list[0]->type >= attr_type_string_begin
                && list[0]->type <= attr_type_string_end) {
            result_free(res);
            res->attr.type=list[0]->type;
            res->attr.u.str=g_strdup(navit_nls_gettext(list[0]->u.str));
            res->allocated=1;

        } else if (!strncmp(function,"new ",4)) {
            enum attr_type attr_type=attr_from_name(function+4);
            result_free(res);
            if (ATTR_IS_INT(attr_type)) {
                if (list && list[0] && ATTR_IS_INT(list[0]->type)) {
                    res->attr.type=attr_type;
                    res->attr.u.num=list[0]->u.num;
                    res->allocated=0;
                } else {
                    dbg(lvl_error,"don't know how to create int of args");
                }
            } else if (ATTR_IS_STRING(attr_type)) {
                if (list && list[0] && ATTR_IS_STRING(list[0]->type)) {
                    res->attr.type=attr_type;
                    res->attr.u.str=g_strdup(list[0]->u.str);
                    res->allocated=1;
                } else {
                    dbg(lvl_error,"don't know how to create string of args");
                }
            } else if (ATTR_IS_OBJECT(attr_type)) {
                struct object_func *func=object_func_lookup(attr_type);
                if (func && func->create) {
                    res->attr.type=attr_type;
                    res->attr.u.data=func->create(list[0], list+1);
                    /* Setting allocated to 1 here will make object to be destroyed when last reference is destroyed.
                       So created persistent objects should be stored with set_attr_var command. */
                    res->allocated=1;
                }
            } else {
                dbg(lvl_error,"don't know how to create %s (%s)",attr_to_name(attr_type),function+4);
            }
        } else if (!strcmp(function,"add_attr")) {
            command_object_add_attr(ctx, &res->attr, list[0]);
        } else if (!strcmp(function,"remove_attr")) {
            command_object_remove_attr(ctx, &res->attr, list[0]);
        } else {
            if (command_object_get_attr(ctx, &res->attr, attr_callback_list, &cbl)) {
                int valid =0;
                struct attr **out=NULL;
                dbg(lvl_debug,"function call %s from %s",function, attr_to_name(res->attr.type));
                callback_list_call_attr_4(cbl.u.callback_list, attr_command, function, list, &out, &valid);
                if (valid!=1) {
                    dbg(lvl_error, "invalid command ignored: \"%s\"; see http://wiki.navit-project.org/index.php/"
                        "OSD#Navit_commands for valid commands.", function);
                }
                if (out && out[0]) {
                    result_free(res);
                    attr_dup_content(out[0], &res->attr);
                    res->allocated=1;
                    attr_list_free(out);
                } else
                    result_free(res);
            } else
                result_free(res);
        }
    }
    attr_list_free(list);
    res->var=NULL;
    res->varlen=0;
    res->attrn=NULL;
    res->attrnlen=0;
}

static void eval_postfix(struct context *ctx, struct result *res) {
    struct result tmp= {{0,},};
    const char *op;

    eval_brace(ctx, res);
    if (ctx->error) return;
    for (;;) {
        if (!(op=get_op(ctx,0,"[","(",".",NULL)))
            return;
        if (op[0] == '.') {
            eval_brace(ctx, &tmp);
            if (ctx->error) return;
            resolve(ctx, res);
            if (ctx->error) return;
            res->attrn=tmp.var;
            res->attrnlen=tmp.varlen;
            dump(res);
        } else if (op[0] == '[') {
            resolve_object(ctx, res);
            if (ctx->error) return;
            if (get_op(ctx,0,"@",NULL)) {
                if (!ctx->skip) {
                    struct object_func *obj_func=object_func_lookup(res->attr.type);
                    struct attr_iter *iter;
                    struct attr attr;
                    enum attr_type attr_type=command_attr_type(res);
                    void *obj=res->attr.u.data;
                    if (!obj) {
                        dbg(lvl_error,"no object");
                        return;
                    }
                    if (!obj_func) {
                        dbg(lvl_error,"no object func");
                        return;
                    }
                    if (!obj_func->iter_new || !obj_func->iter_destroy) {
                        dbg(lvl_error,"no iter func");
                        return;
                    }
                    iter = obj_func->iter_new(NULL);
                    result_free(res);
                    res->varlen=0;
                    res->attrn=NULL;
                    while (obj_func->get_attr(obj, attr_type, &attr, iter)) {
                        if (command_evaluate_to_boolean(&attr, ctx->expr, &ctx->error)) {
                            result_free(res);
                            res->attr=attr;
                        }
                    }
                    obj_func->iter_destroy(iter);
                }
                if (ctx->error) return;
                ctx->expr+=command_evaluate_to_length(ctx->expr, &ctx->error);
            }
            if (!get_op(ctx,0,"]",NULL)) {
                ctx->error=missing_closing_bracket;
                return;
            }
        } else if (op[0] == '(') {
            dbg(lvl_debug,"function call");
            resolve_object(ctx, res);
            command_call_function(ctx, res);
        }
    }
}

static void eval_unary(struct context *ctx, struct result *res) {
    const char *op;
    op=get_op(ctx,0,"~","!",NULL);
    if (op) {
        eval_unary(ctx, res);
        if (ctx->error) return;
        result_op(ctx, op_type_prefix, op, res, NULL);
    } else
        eval_postfix(ctx, res);
}

static void eval_multiplicative(struct context *ctx, struct result *res) {
    struct result tmp= {{0,},};
    const char *op;

    eval_unary(ctx, res);
    if (ctx->error) return;
    for (;;) {
        if (!(op=get_op(ctx,0,"*","/","%",NULL))) return;
        eval_unary(ctx, &tmp);
        if (!ctx->error)
            result_op(ctx, op_type_binary, op, res, &tmp);
        result_free(&tmp);
        if (ctx->error) return;
    }
}

static void eval_additive(struct context *ctx, struct result *res) {
    struct result tmp= {{0,},};
    const char *op;

    eval_multiplicative(ctx, res);
    if (ctx->error) return;
    for (;;) {
        if (!(op=get_op(ctx,0,"-","+",NULL))) return;
        eval_multiplicative(ctx, &tmp);
        if (!ctx->error)
            result_op(ctx, op_type_binary, op, res, &tmp);
        result_free(&tmp);
        if (ctx->error) return;
    }
}

static void eval_equality(struct context *ctx, struct result *res) {
    struct result tmp= {{0,},};
    const char *op;

    eval_additive(ctx, res);
    if (ctx->error) return;
    for (;;) {
        if (!(op=get_op(ctx,0,"==","!=","<=",">=","<",">",NULL))) return;
        eval_additive(ctx, &tmp);
        if (!ctx->error)
            result_op(ctx, op_type_binary, op, res, &tmp);
        result_free(&tmp);
        if (ctx->error) return;
    }
}


static void eval_bitwise_and(struct context *ctx, struct result *res) {
    struct result tmp= {{0,},};

    eval_equality(ctx, res);
    if (ctx->error) return;
    for (;;) {
        if (get_op(ctx,1,"&&",NULL)) return;
        if (!get_op(ctx,0,"&",NULL)) return;
        eval_equality(ctx, &tmp);
        if (!ctx->error)
            result_op(ctx, op_type_binary, "&", res, &tmp);
        result_free(&tmp);
        if (ctx->error) return;
    }
}

static void eval_bitwise_xor(struct context *ctx, struct result *res) {
    struct result tmp= {{0,},};

    eval_bitwise_and(ctx, res);
    if (ctx->error) return;
    for (;;) {
        if (!get_op(ctx,0,"^",NULL)) return;
        eval_bitwise_and(ctx, &tmp);
        if (!ctx->error)
            result_op(ctx, op_type_binary, "^", res, &tmp);
        result_free(&tmp);
        if (ctx->error) return;
    }
}

static void eval_bitwise_or(struct context *ctx, struct result *res) {
    struct result tmp= {{0,},};

    eval_bitwise_xor(ctx, res);
    if (ctx->error) return;
    for (;;) {
        if (get_op(ctx,1,"||",NULL)) return;
        if (!get_op(ctx,0,"|",NULL)) return;
        eval_bitwise_xor(ctx, &tmp);
        if (!ctx->error)
            result_op(ctx, op_type_binary, "|", res, &tmp);
        result_free(&tmp);
        if (ctx->error) return;
    }
}

static void eval_logical_and(struct context *ctx, struct result *res) {
    struct result tmp= {{0,},};

    eval_bitwise_or(ctx, res);
    if (ctx->error) return;
    for (;;) {
        if (!get_op(ctx,0,"&&",NULL)) return;
        eval_bitwise_or(ctx, &tmp);
        if (!ctx->error)
            result_op(ctx, op_type_binary, "&&", res, &tmp);
        result_free(&tmp);
        if (ctx->error) return;
    }
}

static void eval_logical_or(struct context *ctx, struct result *res) {
    struct result tmp= {{0,},};

    eval_logical_and(ctx, res);
    if (ctx->error) return;
    for (;;) {
        if (!get_op(ctx,0,"||",NULL)) return;
        eval_logical_and(ctx, &tmp);
        if (!ctx->error)
            result_op(ctx, op_type_binary, "||", res, &tmp);
        result_free(&tmp);
        if (ctx->error) return;
    }
}

static void eval_conditional(struct context *ctx, struct result *res) {
    struct result tmp= {{0,},};
    int cond=0;
    int skip;

    eval_logical_or(ctx, res);
    if (ctx->error) return;
    if (!get_op(ctx,0,"?",NULL)) return;
    skip=ctx->skip;
    cond=get_bool(ctx, res);
    result_free(res);
    if (ctx->error) return;
    ctx->skip=!cond || skip;
    eval_logical_or(ctx, &tmp);
    ctx->skip=skip;
    if (ctx->error) {
        result_free(&tmp);
        return;
    }

    *res=tmp;
    memset(&tmp,0,sizeof(tmp));

    if (!get_op(ctx,0,":",NULL)) {
        dbg(lvl_debug,"ctxerr");
        ctx->error=missing_colon;
        return;
    }
    ctx->skip=cond || skip;
    eval_logical_or(ctx, &tmp);
    ctx->skip=skip;
    if (ctx->error) {
        result_free(&tmp);
        return;
    }
    if (!cond) {
        result_free(res);
        *res=tmp;
    } else
        result_free(&tmp);
}

/* = *= /= %= += -= >>= <<= &= ^= |= */

static void eval_assignment(struct context *ctx, struct result *res) {
    struct result tmp= {{0,},};
    eval_conditional(ctx, res);
    if (ctx->error) return;
    if (!get_op(ctx,0,"=",NULL)) return;
    eval_conditional(ctx, &tmp);
    if (ctx->error) {
        result_free(&tmp);
        return;
    }
    resolve(ctx, &tmp);
    if (ctx->error) {
        result_free(&tmp);
        return;
    }
    resolve_object(ctx, res);
    command_set_attr(ctx, res, &tmp);
}

/* , */
static void eval_comma(struct context *ctx, struct result *res) {
    struct result tmp= {{0,},};

    eval_assignment(ctx, res);
    if (ctx->error) return;
    for (;;) {
        if (!get_op(ctx,0,",",NULL)) return;
        eval_assignment(ctx, &tmp);
        if (ctx->error) return;
        result_free(res);
        *res=tmp;
    }
}

static struct attr ** eval_list(struct context *ctx) {
    struct result tmp= {{0,},};

    struct attr **ret=NULL;
    for (;;) {
        eval_assignment(ctx, &tmp);
        if (ctx->error) {
            result_free(&tmp);
            attr_list_free(ret);
            return NULL;
        }
        resolve(ctx, &tmp);
        ret=attr_generic_add_attr(ret, &tmp.attr);
        result_free(&tmp);
        if (!get_op(ctx,0,",",NULL)) return ret;
    }
}

#if 0

void command(struct attr *attr, char *expr) {
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
    resolve(&ctx, &res);
    dump(&res);
    printf("%s\n", get_string(&ctx, &res));
}
#endif

static void command_evaluate_to(struct attr *attr, const char *expr, struct context *ctx, struct result *res) {
    result_free(res);
    memset(res, 0, sizeof(*res));
    memset(ctx, 0, sizeof(*ctx));
    ctx->attr=attr;
    ctx->expr=expr;
    eval_comma(ctx,res);
}

enum attr_type command_evaluate_to_attr(struct attr *attr, char *expr, int *error, struct attr *ret) {
    struct result res={{0,},};
    struct context ctx;
    command_evaluate_to(attr, expr, &ctx, &res);
    if (ctx.error)
        return attr_none;
    resolve_object(&ctx, &res);
    *ret=res.attr;
    dbg(lvl_debug,"type %s",attr_to_name(command_attr_type(&res)));
    return command_attr_type(&res);
}

void command_evaluate_to_void(struct attr *attr, char *expr, int *error) {
    struct result res= {{0,},};
    struct context ctx;
    command_evaluate_to(attr, expr, &ctx, &res);
    if (!ctx.error)
        resolve(&ctx, &res);
    if (error)
        *error=ctx.error;
    result_free(&res);

}

char *command_evaluate_to_string(struct attr *attr, char *expr, int *error) {
    struct result res= {{0,},};
    struct context ctx;
    char *ret=NULL;

    command_evaluate_to(attr, expr, &ctx, &res);
    if (!ctx.error)
        resolve(&ctx, &res);
    if (!ctx.error)
        ret=get_string(&ctx, &res);
    if (error)
        *error=ctx.error;

    result_free(&res);

    if (ctx.error)
        return NULL;
    else
        return ret;
}

int command_evaluate_to_int(struct attr *attr, char *expr, int *error) {
    struct result res= {{0,},};
    struct context ctx;
    int ret=0;

    command_evaluate_to(attr, expr, &ctx, &res);
    if (!ctx.error)
        resolve(&ctx, &res);
    if (!ctx.error)
        ret=get_int(&ctx, &res);
    if (error)
        *error=ctx.error;

    result_free(&res);

    if (ctx.error)
        return 0;
    else
        return ret;
}

int command_evaluate_to_boolean(struct attr *attr, const char *expr, int *error) {
    struct result res= {{0,},};
    struct context ctx;
    int ret=0;

    command_evaluate_to(attr, expr, &ctx, &res);
    if (!ctx.error)
        resolve(&ctx, &res);
    if (!ctx.error) {
        if (res.attr.type == attr_none)
            ret=0;
        else if ((res.attr.type >= attr_type_int_begin && res.attr.type <= attr_type_int_end) ||
                 (res.attr.type >= attr_type_double_begin && res.attr.type <= attr_type_double_end))
            ret=get_int(&ctx, &res);
        else
            ret=res.attr.u.data != NULL;
    }

    result_free(&res);

    if (error)
        *error=ctx.error;
    if (ctx.error)
        return 0;
    else
        return ret;
}

int command_evaluate_to_length(const char *expr, int *error) {
    struct attr attr;
    struct result res= {{0,},};
    struct context ctx;

    attr.type=attr_none;
    attr.u.data=NULL;
    command_evaluate_to(&attr, expr, &ctx, &res);

    result_free(&res);

    if (!ctx.error)
        return ctx.expr-expr;
    return 0;
}

static int command_evaluate_single(struct context *ctx) {
    struct result res= {{0,},},tmp= {{0,},};
    const char *op,*a,*f,*end;
    enum attr_type attr_type;
    void *obj;
    struct object_func *obj_func;
    struct attr_iter *iter;
    struct attr attr;
    int cond=0;
    int skip=ctx->skip;
    if (!(op=get_op(ctx,0,"foreach","if","{",NULL))) {
        eval_comma(ctx,&res);
        if (ctx->error)
            return 0;
        resolve(ctx, &res);
        result_free(&res);
        if (ctx->error)
            return 0;
        return get_op(ctx,0,";",NULL) != NULL;
    }
    switch (op[0]) {
    case 'f':
        if (!get_op(ctx,0,"(",NULL)) {
            ctx->error=missing_opening_parenthesis;
            return 0;
        }
        ctx->skip=1;
        a=ctx->expr;
        eval_conditional(ctx, &res);
        resolve_object(ctx, &res);
        ctx->skip=skip;
        if (!get_op(ctx,0,";",NULL)) {
            ctx->error=missing_semicolon;
            return 0;
        }
        eval_comma(ctx,&res);
        attr_type=command_attr_type(&res);
        obj=res.attr.u.data;
        obj_func=object_func_lookup(res.attr.type);
        if (!get_op(ctx,0,")",NULL)) {
            ctx->error=missing_closing_parenthesis;
            return 0;
        }
        f=ctx->expr;
        ctx->skip=1;
        if (!command_evaluate_single(ctx)) {
            ctx->skip=skip;
            return 0;
        }
        ctx->skip=skip;
        if (ctx->skip) {
            result_free(&res);
            return 1;
        }
        end=ctx->expr;
        if (!obj) {
            dbg(lvl_error,"no object");
            return 0;
        }
        if (!obj_func) {
            dbg(lvl_error,"no object func");
            return 0;
        }
        if (!obj_func->iter_new || !obj_func->iter_destroy) {
            dbg(lvl_error,"no iter func");
            return 0;
        }
        iter = obj_func->iter_new(NULL);
        while (obj_func->get_attr(obj, attr_type, &attr, iter)) {
            ctx->expr=a;
            eval_conditional(ctx, &res);
            resolve_object(ctx, &res);
            tmp.attr=attr;
            resolve(ctx, &tmp);
            if (ctx->error) {
                result_free(&tmp);
                return 0;
            }
            command_set_attr(ctx, &res, &tmp);
            result_free(&tmp);
            ctx->expr=f;
            if (!command_evaluate_single(ctx)) {
                obj_func->iter_destroy(iter);
                return 0;
            }
        }
        obj_func->iter_destroy(iter);
        ctx->expr=end;
        return 1;
    case 'i':
        if (!get_op(ctx,0,"(",NULL)) {
            ctx->error=missing_opening_parenthesis;
            return 0;
        }
        eval_comma(ctx,&res);
        if (!skip && !ctx->error)
            cond=get_bool(ctx, &res);
        result_free(&res);
        if (ctx->error)
            return 0;
        if (!get_op(ctx,0,")",NULL)) {
            ctx->error=missing_closing_parenthesis;
            return 0;
        }
        ctx->skip=!cond || skip;
        command_evaluate_single(ctx);
        ctx->skip=skip;
        if (ctx->error)
            return 0;
        if (get_op(ctx,0,"else",NULL)) {
            ctx->skip=cond || skip;
            command_evaluate_single(ctx);
            ctx->skip=skip;
            if (ctx->error)
                return 0;
        }
        return 1;
    case '{':
        while (!get_op(ctx,0,"}",NULL)) {
            if (!command_evaluate_single(ctx))
                return 0;
        }
        return 1;
    default:
        return 0;
    }
}

void command_evaluate(struct attr *attr, const char *expr) {
    /* Once the eval has started we can't rely anymore on the content of
     * expr which may be freed when the calling widget is destroyed by a
     * subsequent command call. Hence the g_strdup. */

    char *expr_dup;
    char *err = NULL;	/* Error description */
    struct context ctx= {0,};
    ctx.attr=attr;
    ctx.error=0;
    ctx.expr=expr_dup=g_strdup(expr);
    for (;;) {
        if (!command_evaluate_single(&ctx))
            break;
    }
    if (ctx.error && ctx.error != eof_reached) {
        char expr[32];
        strncpy(expr, ctx.expr, 32);
        expr[31]='\0';
        err = command_error_to_text(ctx.error);
        dbg(lvl_error, "error %s starting at %s", err, expr);
        g_free(err);
    }
    g_free(expr_dup);
}

#if 0
void command_interpreter(struct attr *attr) {
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

static void command_table_call(struct command_table *table, int count, void *data, char *command, struct attr **in, struct attr ***out, int *valid) {
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

void command_add_table_attr(struct command_table *table, int count, void *data, struct attr *attr) {
    attr->type=attr_callback;
    attr->u.callback=callback_new_attr_3(callback_cast(command_table_call),attr_command, table, count, data);
}

void command_add_table(struct callback_list *cbl, struct command_table *table, int count, void *data) {
    struct attr attr;
    command_add_table_attr(table, count, data, &attr);
    callback_list_add(cbl, attr.u.callback);
}

void command_saved_set_cb (struct command_saved *cs, struct callback *cb) {
    cs->cb = cb;
}

/**
 * @brief Returns an integer representation of the evaluation result of a saved command
 *
 * This function is a wrapper around {@code get_int()}. It is equivalent to
 * {@code get_int(&cs->ctx, &cs->res)}. See {@code get_int()} for a description.
 */
int command_saved_get_int (struct command_saved *cs) {
    return get_int(&cs->ctx, &cs->res);
}

int command_saved_error (struct command_saved *cs) {
    return cs->error;
}

/**
 * @brief Idle function to evaluate a command
 *
 * This function is called from an idle loop for asynchronous evaluation but may also be called in-line.
 *
 * The result of the evaluation can be retrieved from {@code cs->res} after this function returns. If an
 * error occurred, it will be stored in {@code cs->error}.
 *
 * @param cs The command to evaluate
 */
static void command_saved_evaluate_idle (struct command_saved *cs) {
    dbg(lvl_debug, "enter: cs=%p, cs->command=%s", cs, cs->command);
    // Only run once at a time
    if (cs->idle_ev) {
        event_remove_idle(cs->idle_ev);
        cs->idle_ev = NULL;
    }

    command_evaluate_to(&cs->context_attr, cs->command, &cs->ctx, &cs->res);

    if (!cs->ctx.error) {
        cs->error = 0;

        if (cs->cb) {
            callback_call_1(cs->cb, cs);
        }
    } else {
        cs->error = cs->ctx.error;
    }
}

/**
 * @brief Evaluates a command
 *
 * This function examines {@code cs->async} to determine if the command should be evaluated immediately.
 * If {@code cs->async} is true, an idle event is registered to register the command. Else the command
 * is evaluated immediately and the result can be retrieved immediately after this function returns.
 *
 * See {@link command_saved_evaluate_idle(struct command_saved *)} for details.
 *
 * @param cs The command to evaluate
 */
static void command_saved_evaluate(struct command_saved *cs) {
    dbg(lvl_debug, "enter: cs=%p, cs->async=%d, cs->command=%s", cs, cs->async, cs->command);
    if (!cs->async) {
        command_saved_evaluate_idle(cs);
        return;
    }
    if (cs->idle_ev) {
        // We're already scheduled for reevaluation
        return;
    }

    if (!cs->idle_cb) {
        cs->idle_cb = callback_new_1(callback_cast(command_saved_evaluate_idle), cs);
    }

    cs->idle_ev = event_add_idle(100, cs->idle_cb);
}

/**
 * @brief Recreates all callbacks for a saved command
 *
 * @param cs The saved command
 */
static void command_saved_callbacks_changed(struct command_saved *cs) {
    // For now, we delete each and every callback and then re-create them
    int i;
    struct object_func *func;
    struct attr attr;

    dbg(lvl_debug, "enter: cs=%p, cs->async=%d, cs->command=%s", cs, cs->async, cs->command);

    if (cs->register_ev) {
        event_remove_idle(cs->register_ev);
        cs->register_ev = NULL;
    }

    attr.type = attr_callback;

    for (i = 0; i < cs->num_cbs; i++) {
        func = object_func_lookup(cs->cbs[i].attr.type);

        if (!func->remove_attr) {
            dbg(lvl_error, "Could not remove command-evaluation callback because remove_attr is missing for type %i!",
                cs->cbs[i].attr.type);
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

/**
 * @brief Registers callbacks for a saved command
 *
 * This function registers callbacks for each attribute used in a saved command, causing the command to
 * be re-evaluated whenever its value might change.
 *
 * This function will fail if an object used in the expression could not be resolved. This may happen
 * during startup if this function is called before all objects have been created. In this case, the
 * caller should schedule the function to be called again at a later time.
 *
 * It will also fail if an error is encountered. This can be determined by examining
 * {@code cs->ctx.error} after the function returns.
 *
 * @param cs The command
 *
 * @return True if all callbacks were successfully registered, false if the function failed
 */
static int command_register_callbacks(struct command_saved *cs) {
    struct attr prev;	/* The parent of the next object which will be retrieved. */
    struct attr cb_attr;
    int status;
    struct object_func *func;
    struct callback *cb;
    int tmpoffset;	/* For debugging. Because we work with pointers into the same string instance.
					 * we can figure out offsets by using simple pointer arithmetics.
					 */

    dbg(lvl_debug, "enter: cs=%p, cs->async=%d, cs->command=%s", cs, cs->async, cs->command);
    cs->ctx.expr = cs->command;
    prev = cs->context_attr;

    while ((status = get_next_object(&cs->ctx, &cs->res)) != 0) {
        tmpoffset = cs->res.var - cs->command;
        cs->ctx.attr = &prev;
        resolve(&cs->ctx, &cs->res);

        if (cs->ctx.error) {
            /* An error occurred while parsing the command */
            tmpoffset = cs->ctx.expr - cs->command;
            dbg(lvl_error, "parsing error: cs=%p, cs->ctx.error=%d", cs, cs->ctx.error);
            dbg(lvl_error, "\t%s", cs->command);
            dbg(lvl_error, "\t%*s", tmpoffset + 1, "^");
            return 0;
        } else if (cs->res.attr.type == attr_none) {
            /* We could not resolve an object, perhaps because it has not been created */
            dbg(lvl_error, "could not resolve object in cs=%p:", cs);
            dbg(lvl_error, "\t%s", cs->command);
            dbg(lvl_error, "\t%*s", tmpoffset + 1, "^");
            return 0;
        }

        if (prev.type != attr_none) {
            func = object_func_lookup(prev.type);

            if (func->add_attr) {
                if (status == 2) { // This is not the final attribute name
                    cb = callback_new_attr_1(callback_cast(command_saved_callbacks_changed), cs->res.attr.type, (void*)cs);
                } else if (status == 1) { // This is the final attribute name
                    cb = callback_new_attr_1(callback_cast(command_saved_evaluate), cs->res.attr.type, (void*)cs);
                    cs->ctx.attr = &cs->context_attr;
                } else {
                    dbg(lvl_error, "Error: Strange status returned from get_next_object()");
                }

                cs->num_cbs++;
                cs->cbs = g_realloc(cs->cbs, (sizeof(struct command_saved_cb) * cs->num_cbs));
                cs->cbs[cs->num_cbs-1].cb = cb;
                cs->cbs[cs->num_cbs-1].attr = prev;

                cb_attr.u.callback = cb;
                cb_attr.type = attr_callback;

                func->add_attr(prev.u.data, &cb_attr);

            } else {
                dbg(lvl_error, "Could not add callback because add_attr is missing for type %i", prev.type);
            }
        }

        if (status == 2) {
            prev = cs->res.attr;
        } else {
            prev = cs->context_attr;
        }
    }

    command_saved_evaluate_idle(cs);

    dbg(lvl_debug, "done: cs=%p, cs->command=%s", cs, cs->command);
    return 1;
}

/**
 * @brief Creates a new saved command.
 *
 * @param command The command string
 * @param attr The context attribute for the saved command
 * @param cb The callback to call whenver the command is re-evaluated
 * @param async Whether the saved command should be flagged as asynchronous, causing it to be evaluated
 * in an idle callback
 */
struct command_saved *
command_saved_attr_new(char *command, struct attr *attr, struct callback *cb, int async) {
    struct command_saved *ret;

    ret = g_new0(struct command_saved, 1);
    dbg(lvl_debug, "enter, ret=%p, command=%s", ret, command);
    ret->command = g_strdup(command);
    ret->context_attr = *attr;
    ret->cb = cb;
    ret->error = not_ready;
    ret->async = async;

    if (!command_register_callbacks(ret)) {
        // We try this as an idle call again
        dbg(lvl_debug, "could not register callbacks, will retry as an idle call");
        ret->register_cb = callback_new_1(callback_cast(command_saved_callbacks_changed), ret);
        ret->register_ev = event_add_idle(300, ret->register_cb);
    }

    return ret;
}

struct command_saved *
command_saved_new(char *command, struct navit *navit, struct callback *cb, int async) {
    struct attr attr=ATTR_OBJECT(navit, navit);
    return command_saved_attr_new(command, &attr, cb, async);
}

void command_saved_destroy(struct command_saved *cs) {
    g_free(cs->command);
    g_free(cs);
}
