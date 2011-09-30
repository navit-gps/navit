
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include "obj_filter.h" 
#include "item.h" 
#include "attr.h" 
#include "xmlconfig.h" 

union result_t {
  int res_int;
  int res_bool;
  char* res_str;
};

enum error_t {ERR_NONE=0, ERR_SYNTAX, ERR_UNKNOWN_ATTRIBUTE, ERR_INTERNAL, ERR_NONTERMINATED_STRING };

struct ctx {
  char*expr;
  union result_t res;
  enum error_t error;
  struct attr *navit_obj;
  struct object_func *obj_func;
};

//prototypes
static int parse_numeric_expr(struct ctx*the_ctx);
static int parse_cond_expr(struct ctx*the_ctx);
static int parse_bool_expr(struct ctx*the_ctx);
static enum operator_t get_operator_by_symbol(char*sym);
static char* get_opstr_by_op(enum operator_t op);
static int whitespace_num(char*str);

static char*next_sym_ptr;

/*
 * parses an input like osd[attr1==2 && attr2==3][2]   or   [attr1==2 && attr2==3]
 * returns 0 on failure , returns the number of parsed characters on success
 */ 
int parse_obj_filter(const char*input, struct obj_filter_t* out)
{
  char *curr_ch, *itt, *expr;
  out->idx = 0; //default idx

  //skip whitespaces
  curr_ch = (char*)input;
  while(isspace(*curr_ch)) {++curr_ch;}

  //parse attr type for the list(iterator)
  itt = out->iterator_type; 
  if(isalpha(*curr_ch) || *curr_ch=='_') {
    while(isalnum(*curr_ch) || *curr_ch=='_' ) {
      *itt = *curr_ch;
      ++itt;
      ++curr_ch;
    }
  }
  *itt = 0;
  //skip whitespaces
  while(isspace(*curr_ch)) {++curr_ch;}
  //parse the filter expression (first section between[]), ignore [] chars between "" or ''  
  expr = out->filter_expr;
  if(*curr_ch++ == '[') {
    while (*curr_ch && *curr_ch!=']') {
      *expr = *curr_ch;
      ++expr;
      ++curr_ch;
    }
  } else {
    return 0;
  }
  *expr = 0;
  if (*curr_ch==']') {
    char*end1;
    ++curr_ch;
    //skip whitespaces
    while(isspace(*curr_ch)) {++curr_ch;}
    end1 = curr_ch; //possible end of expression if no idx specified
    //parse the optional idx part (another part between [] contains numeric expreession(currently only constant numers are supported)) 
    if (*curr_ch=='[') {
      char numstr[64];
      char*numch;
      ++curr_ch;
      //skip whitespaces
      while(isspace(*curr_ch)) {++curr_ch;}

      numch = numstr;
      while ('0'<=*curr_ch && *curr_ch<='9') {
        *numch = *curr_ch;
        ++numch;
        ++curr_ch;
      }

      //skip whitespaces
      while(isspace(*curr_ch)) {++curr_ch;}

      if (*curr_ch==']') {
        ++curr_ch;
        *numch = 0;
        out->idx = atoi(numstr);
        return curr_ch-input;
      } else {
        return end1-input;
      }
    } else {
      return end1-input;
    }
  } else {
    return 0;
  }
}

struct attr* filter_object(struct attr* root,char*iter_type,char*expr, int idx)
{
  struct attr_iter *iter;
  struct attr the_attr;
  int curr_idx = 0;
  struct object_func *obj_func_root;
  struct object_func *obj_func_iter;

  if (!root || !iter_type || !expr) {
    return NULL;
  }

  obj_func_root = object_func_lookup(((struct attr*)root)->type);
  obj_func_iter = object_func_lookup(attr_from_name(iter_type));
  if(   !obj_func_root || !obj_func_root->get_attr || !obj_func_root->iter_new || !obj_func_root->iter_destroy ||
        !obj_func_iter || !obj_func_iter->get_attr ) {
    return NULL;
  }

  iter = obj_func_root->iter_new(NULL);
  while ( obj_func_root->get_attr(root->u.data, attr_from_name(iter_type), &the_attr, iter ) ) {
    struct ctx the_ctx;
    the_ctx.expr = expr;
    the_ctx.navit_obj = &the_attr; 
    the_ctx.obj_func = obj_func_iter; 
    if (parse_bool_expr(&the_ctx)) { 
      if(the_ctx.res.res_bool) {
        if (curr_idx==idx) {
          return attr_dup(&the_attr);
        }
        ++curr_idx;
      }
    }
  } 
  obj_func_root->iter_destroy(iter);

  return NULL;
}

enum operator_t  {OP_NONE=0, OP_GT, OP_GE, OP_LT, OP_LE, OP_EQ, OP_NE, OP_LOGICAL_AND, OP_LOGICAL_OR, OP_LOGICAL_XOR, OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD};

struct op_table_entry {
  char*name;
  enum operator_t op_type;
};

struct op_table_entry op_table[] = { 
                    {"<" , OP_GT}, 
                    {"<=", OP_GE}, 
                    {">" , OP_LT}, 
                    {">=", OP_LE}, 
                    {"!=", OP_NE}, 
                    {"==", OP_EQ}, 
                    {"&&", OP_LOGICAL_AND}, 
                    {"||", OP_LOGICAL_OR}, 
                    {"^" , OP_LOGICAL_XOR}, 
                    {"+" , OP_ADD}, 
                    {"-" , OP_SUB}, 
                    {"*" , OP_MUL}, 
                    {"/" , OP_DIV}, 
                    {"%" , OP_MOD}, 
                    {NULL, OP_NONE} 
                  };

static enum operator_t get_operator_by_symbol(char*sym)
{
  struct op_table_entry* ote = op_table;
  while(sym && ote->name) {
    if ( ! strcmp(ote->name, sym)) {
      return ote->op_type;
    }
    ++ote;
    }
  return OP_NONE;
}

static char* get_opstr_by_op(enum operator_t op)
{
  struct op_table_entry* ote = op_table;
  while(ote->name) {
    if ( ote->op_type == op ) {
      return ote->name;
    }
    ++ote;
    }
  return NULL;
}

/*
GRAMMAR

ident = [ [A-Z,a-z][A-Z,a-z,0-9]+ 
	] .

number = [ [A-Z,a-z][A-Z,a-z,0-9]+ 
	] .

numeric_additive_expr = [ 
	| numeric_mul_operand numeric_mul_operator numeric_mul_expr
	| numeric_add_operand numeric_add_operator numeric_add_expr
	| "(" numeric_expr ")"
	| numeric_operand 
	] 

bool_expr = [ bool_expr bool_operator bool_operand 
	| "!" bool_operand 
	| bool_operand 
	] 

cond_expr = [
	cond_operand cond_operator cond_operand
	]

//TODO support precedence between additive and multiplicative operators (the order of +- and * / evaluation)

numeric_multiplicative_operator = [ "*" "/" "%"
	]

numeric_additive_operator = [ "+" "-"  
	]

bool_operator = [ "&&" "||" "^" 
	]

cond_operator = [ "==" "!=" "< "> "<=" ">=" 
	]

cond_operand = 
	[ ident 
	| number 
	| numeric_expr
	] . 

numeric_operand = 
	[ ident 
	| number 
	| "(" numeric_expr ")"
	] . 

bool_operand = 
	| "(" bool_expr ")"
	| cond_expr
	| "true"
	| "false"
	] . 
*/

static int whitespace_num(char*str)
{
  int ret = 0;
  while(isspace(*str)) {
    ++str;
    ++ret;
  }
  return ret;
}

static char *
get_op(char *expr, ...)
{
	char *op;
	char *ret=NULL;
	va_list ap;

	while (isspace(*expr)) {
		expr++;
	}

	va_start(ap, expr);
	while ((op = va_arg(ap, char *))) {
		if (!strncmp(expr, op, strlen(op))) {
			ret = op;
			break;
		}
	}
	va_end(ap);
	return ret;
}

static int ident_length(char*input)
{
  int ret = 0;
  if(input[0] != '@') {
    return 0;
  }
  if(isalpha((int)input[1])) {
    int ii;
    ++ret;
    for(ii=2 ; ii<strlen(input) ; ++ii) {
      if(!isalnum((int)input[ii])) {
        return ret;
      } else {
        ++ret;
      }
    }
    return ret;
  }
  return 0;
}

static enum operator_t parse_bool_operator(struct ctx*the_ctx)
{
  char* input = the_ctx->expr;
  char *sym;
  enum operator_t op;
  sym = get_op(input,"&&","||","^",NULL);
 
  op = get_operator_by_symbol(sym);
  switch(op) {
    case OP_LOGICAL_AND:
    case OP_LOGICAL_OR:
    case OP_LOGICAL_XOR:
      return op;
      break;
    default:
      return OP_NONE;
      break;
  };
}

static enum operator_t parse_cond_operator(struct ctx*the_ctx)
{
  char* input = the_ctx->expr;
  char *sym;
  enum operator_t op;
  sym = get_op(input, "==", "<", ">", "<=", ">=", "!=", NULL);
 
  op = get_operator_by_symbol(sym);
  switch(op) {
    case OP_EQ:
    case OP_GT:
    case OP_LT:
    case OP_GE:
    case OP_LE:
    case OP_NE:
      return op;
      break;
    default:
      return OP_NONE;
      break;
  };
}

static enum operator_t parse_numeric_multiplicative_operator(struct ctx*the_ctx)
{
  char* input = the_ctx->expr;
  char *sym;
  enum operator_t op;
  sym = get_op(input, "*", "/", "%", NULL);
 
  op = get_operator_by_symbol(sym);
  switch(op) {
    case OP_MUL:
    case OP_DIV:
    case OP_MOD:
      return op;
      break;
    default:
      return OP_NONE;
      break;
  };
}

static enum operator_t parse_numeric_additive_operator(struct ctx*the_ctx)
{
  char* input = the_ctx->expr;
  char *sym;
  enum operator_t op;
  sym = get_op(input, "+", "-", NULL);
 
  op = get_operator_by_symbol(sym);
  switch(op) {
    case OP_ADD:
    case OP_SUB:
      return op;
      break;
    default:
      return OP_NONE;
      break;
  };
}

static int parse_string_operand(struct ctx*the_ctx)
{
  char* input = the_ctx->expr;
  char sym[256];
  struct ctx sub_ctx;
  int ilen;
  sub_ctx.navit_obj = the_ctx->navit_obj;
  sub_ctx.obj_func = the_ctx->obj_func;
  
  while(isspace(*input)) {++input;}

  if (*input == '"' || *input=='\'') {
    char delimiter = *input;
    char* psym = sym;
    ++input;
    while(*input != delimiter && *input!='\0') {
      *psym = *input;
      ++psym;
      ++input;
    }
    if(*input=='\0') {
      the_ctx->error = ERR_NONTERMINATED_STRING;
      return 0;
    } else { //terminating delimiter found
      *psym = '\0';
      the_ctx->res.res_str = g_strdup(sym);
      the_ctx->error = ERR_NONE;
      next_sym_ptr = input + 1;
      return 1;
    }
  }

  ilen = ident_length(input);
  if(ilen) {
    struct attr the_attr;
    strncpy(sym,input+1,ilen);
    sym[ilen] = '\0';
    if ( attr_from_name(sym)!=attr_none && the_ctx->obj_func->get_attr(the_ctx->navit_obj->u.data, attr_from_name(sym), &the_attr, NULL ) 
      && attr_type_string_begin<=the_attr.type && the_attr.type <= attr_type_string_end) { 
      the_ctx->res.res_str = g_strdup(the_attr.u.str);
      the_ctx->error = ERR_NONE;
      next_sym_ptr = input+ilen+1;
      return 1;
    } else {
      the_ctx->error = ERR_UNKNOWN_ATTRIBUTE;
      return 0;
    }
  }

  //currently we are not supporting string expressions (eg: concatenation)
  /*
  if(get_op(input,"(",NULL)) {
    sub_ctx.expr = input + 1 + whitespace_num(input);
    if ( ! parse_string_expr(&sub_ctx)) {
      the_ctx->error = ERR_SYNTAX;
      return 0;
    } else {
      the_ctx->res.res_int = sub_ctx.res.res_int;
      the_ctx->error = ERR_NONE;
    }
    //expect ")"
    if ( get_op(next_sym_ptr,")",NULL)) { 
      next_sym_ptr += 1 + whitespace_num(next_sym_ptr);
      return 1;
    } else {
      the_ctx->res.res_int = 0;
      the_ctx->error = ERR_SYNTAX;
      return 0;
    }
  }
  */
  return 0;
}


static int parse_numeric_operand(struct ctx*the_ctx)
{
  char* input = the_ctx->expr;
  char sym[256];
  struct ctx sub_ctx;
  char* endptr = NULL;
  int res, ilen;
  sub_ctx.navit_obj = the_ctx->navit_obj;
  sub_ctx.obj_func = the_ctx->obj_func;
  
  res = strtol(input,&endptr,0);
  if( endptr!=input ) { 
    the_ctx->res.res_int = res;
    the_ctx->error = ERR_NONE;
    next_sym_ptr = endptr;
    return 1;
  }

  while(isspace(*input)) {++input;}
  ilen = ident_length(input);
  if(ilen) {
    //get int value from context object and set result member of context argument
    struct attr the_attr;
    strncpy(sym,input+1,ilen);
    sym[ilen] = '\0';
    if ( attr_from_name(sym)!=attr_none && the_ctx->obj_func->get_attr(the_ctx->navit_obj->u.data, attr_from_name(sym), &the_attr, NULL ) 
      && attr_type_int_begin<=the_attr.type && the_attr.type <= attr_type_int_end) { //TODO support other numeric types
      the_ctx->res.res_int = the_attr.u.num; 
      the_ctx->error = ERR_NONE;
      next_sym_ptr = input+ilen+1;
      return 1;
    } else {
      the_ctx->error = ERR_UNKNOWN_ATTRIBUTE;
      return 0;
    }
  }
  if(get_op(input,"(",NULL)) {
    sub_ctx.expr = input + 1 + whitespace_num(input);
    if ( ! parse_numeric_expr(&sub_ctx)) {
      the_ctx->error = ERR_SYNTAX;
      return 0;
    } else {
      the_ctx->res.res_int = sub_ctx.res.res_int;
      the_ctx->error = ERR_NONE;
    }
    //expect ")"
    if ( get_op(next_sym_ptr,")",NULL)) { 
      next_sym_ptr += 1 + whitespace_num(next_sym_ptr);
      return 1;
    } else {
      the_ctx->res.res_int = 0;
      the_ctx->error = ERR_SYNTAX;
      return 0;
    }
  }
  return 0;
}

static int parse_cond_operand(struct ctx*the_ctx)
{
  char* input = the_ctx->expr;
  char sym[256];
  char* endptr = NULL;
  int res, ilen;
  struct attr the_attr;
  struct ctx sub_ctx;
  sub_ctx.navit_obj = the_ctx->navit_obj;
  sub_ctx.obj_func = the_ctx->obj_func;
  sub_ctx.expr = input;
  if(parse_numeric_expr(&sub_ctx)) {
    the_ctx->res.res_int = sub_ctx.res.res_int;
    the_ctx->error = ERR_NONE;
    return 1;
  }

  res = strtol(input,&endptr,0);
  if( endptr!=input ) { 
    the_ctx->res.res_int = res;
    the_ctx->error = ERR_NONE;
    next_sym_ptr = endptr;
    return 1;
  }
  while(isspace(*input)) {++input;}
  ilen = ident_length(input);
  strncpy(sym,input+1,ilen);
  sym[ilen] = '\0';
  if ( attr_from_name(sym)!=attr_none && the_ctx->obj_func->get_attr(the_ctx->navit_obj->u.data, attr_from_name(sym), &the_attr, NULL ) 
    && attr_type_int_begin<=the_attr.type && the_attr.type <= attr_type_int_end) { //TODO support other numeric types
    the_ctx->res.res_int = the_attr.u.num; 
    the_ctx->error = ERR_NONE;
    next_sym_ptr = input+ilen+1;
    return 1;
  } else {
    the_ctx->error = ERR_UNKNOWN_ATTRIBUTE;
    return 0;
  }
  the_ctx->error = ERR_SYNTAX;
  return 0;
}

static int parse_bool_operand(struct ctx*the_ctx)
{
  char* input = the_ctx->expr;
  struct ctx sub_ctx;
  sub_ctx.navit_obj = the_ctx->navit_obj;
  sub_ctx.obj_func = the_ctx->obj_func;
  if(get_op(input,"true",NULL)) {
    the_ctx->res.res_bool = 1;
    the_ctx->error = ERR_NONE;
    next_sym_ptr += whitespace_num(input)+strlen("true");
    return 1;
  }
  if(get_op(input,"false",NULL)) {
    the_ctx->res.res_bool = 0;
    the_ctx->error = ERR_NONE;
    next_sym_ptr += whitespace_num(input)+strlen("false");
    return 1;
  }
  if(get_op(input,"(",NULL)) {
    sub_ctx.expr = input + 1 + whitespace_num(input);
    if ( ! parse_bool_expr(&sub_ctx)) {
      the_ctx->error = ERR_SYNTAX;
      return 0;
    } else {
      the_ctx->res.res_bool = sub_ctx.res.res_bool;
      the_ctx->error = ERR_NONE;
    }
    //expect ")"
    if(get_op(next_sym_ptr,"(",NULL)) {
      next_sym_ptr += 1 + whitespace_num(next_sym_ptr);
      return 1;
    } else {
      the_ctx->error = ERR_SYNTAX;
      return 0;
    }
  }
  //cond_expr
  sub_ctx.expr = input;
  if (parse_cond_expr(&sub_ctx)) {
      the_ctx->res.res_bool = sub_ctx.res.res_bool;
      the_ctx->error = ERR_NONE;
    return 1;
  }
  the_ctx->error = ERR_SYNTAX;
  return 0;
}

static int parse_cond_expr(struct ctx*the_ctx)
{
  char* input = the_ctx->expr;
  struct ctx sub_ctx;
  sub_ctx.navit_obj = the_ctx->navit_obj;
  sub_ctx.obj_func = the_ctx->obj_func;
  sub_ctx.expr = input;


  //expect cond operand
  if(parse_cond_operand(&sub_ctx)) {
    enum operator_t op;
    int op1 = sub_ctx.res.res_int;
    //expect cond operand
    sub_ctx.expr = next_sym_ptr;
    if( (op=parse_cond_operator(&sub_ctx)) ) {
      next_sym_ptr += whitespace_num(next_sym_ptr) + strlen(get_opstr_by_op(op));
      sub_ctx.expr = next_sym_ptr;
      if(parse_cond_operand(&sub_ctx)) {
        int op2 = sub_ctx.res.res_int;
        switch(op) {
          case OP_EQ:
            the_ctx->res.res_bool = op1==op2;
            break;
          case OP_GT:
            the_ctx->res.res_bool = op1<op2;
            break;
          case OP_LT:
            the_ctx->res.res_bool = op1>op2;
            break;
          case OP_GE:
            the_ctx->res.res_bool = op1<=op2;
            break;
          case OP_LE:
            the_ctx->res.res_bool = op1>=op2;
            break;
          case OP_NE:
            the_ctx->res.res_bool = op1!=op2;
            break;
          default:
            //unknown operator
            the_ctx->error = ERR_INTERNAL;
            return 0;
            break;
        };
        the_ctx->error = ERR_NONE;
        return 1;
      }
    }
  }



  //expect cond operand
  if(parse_string_operand(&sub_ctx)) {
    char* op1 = sub_ctx.res.res_str;
    enum operator_t op;

    //expect cond operand
    sub_ctx.expr = next_sym_ptr;
    if( (op=parse_cond_operator(&sub_ctx)) ) {
      next_sym_ptr += whitespace_num(next_sym_ptr) + strlen(get_opstr_by_op(op));
      sub_ctx.expr = next_sym_ptr;
      if(parse_string_operand(&sub_ctx)) {
        char* op2 = sub_ctx.res.res_str;
        switch(op) {
          case OP_EQ:
            the_ctx->res.res_bool = !strcmp(op1,op2);
            break;
          case OP_GT:
            the_ctx->res.res_bool = strcmp(op1,op2)<=0;
            break;
          case OP_LT:
            the_ctx->res.res_bool = strcmp(op1,op2)>=0;
            break;
          case OP_GE:
            the_ctx->res.res_bool = strcmp(op1,op2)<0;
            break;
          case OP_LE:
            the_ctx->res.res_bool = strcmp(op1,op2)>0;
            break;
          case OP_NE:
            the_ctx->res.res_bool = !!strcmp(op1,op2);
            break;
          default:
            //unknown operator
            the_ctx->error = ERR_INTERNAL;
            return 0;
            break;
        };
        the_ctx->error = ERR_NONE;
        return 1;
      }
    }
  }
  the_ctx->error = ERR_SYNTAX;
  return 0;
}

/*
numeric_expr = [ 
	| numeric_operand numeric_multiplicative_operator numeric_expr
	| numeric_operand numeric_additive_operator numeric_expr
	| "(" numeric_expr ")"
	| numeric_operand 
	] 
*/
static int parse_numeric_expr(struct ctx*the_ctx)
{
char* input = the_ctx->expr;
struct ctx sub_ctx;
sub_ctx.navit_obj = the_ctx->navit_obj;
sub_ctx.obj_func = the_ctx->obj_func;
sub_ctx.expr = input;
if(parse_numeric_operand(&sub_ctx)) {
  enum operator_t op;
  int op1 = sub_ctx.res.res_int;
  //expect numeric_multiplicative operator
  sub_ctx.expr = next_sym_ptr;
  if( (op=parse_numeric_multiplicative_operator(&sub_ctx)) ) {
    next_sym_ptr += whitespace_num(next_sym_ptr) + strlen(get_opstr_by_op(op));
    sub_ctx.expr = next_sym_ptr;
    if(parse_numeric_expr(&sub_ctx)) {
      int op2 = sub_ctx.res.res_int;
      switch(op) {
        case OP_MUL:
          the_ctx->res.res_int = op1*op2;
          break;
        case OP_DIV:
          the_ctx->res.res_int = op1/op2;
          break;
        case OP_MOD:
          the_ctx->res.res_int = op1%op2;
          break;
        default:
          //unknown operator
          the_ctx->error = ERR_INTERNAL;
          return 0;
          break;
      };
      the_ctx->error = ERR_NONE;
      return 1;
    }
  } 
}

sub_ctx.expr = input;
if(parse_numeric_operand(&sub_ctx)) {
  //expect numeric_additive operator
  enum operator_t op;
  int op1 = sub_ctx.res.res_int;
  sub_ctx.expr = next_sym_ptr;
  if((op=parse_numeric_additive_operator(&sub_ctx))) {
    next_sym_ptr += whitespace_num(next_sym_ptr) + strlen(get_opstr_by_op(op));
    sub_ctx.expr = next_sym_ptr;
    if(parse_numeric_expr(&sub_ctx)) {
      int op2 = sub_ctx.res.res_int;
      switch(op) {
        case OP_ADD:
          the_ctx->res.res_int = op1+op2;
          break;
        case OP_SUB:
          the_ctx->res.res_int = op1-op2;
          break;
        default:
          //unknown operator
          the_ctx->error = ERR_INTERNAL;
          return 0;
          break;
      };
      the_ctx->error = ERR_NONE;
      return 1;
    }
  }
}

sub_ctx.expr = input;
if(parse_numeric_operand(&sub_ctx) ) {
  the_ctx->res.res_int = sub_ctx.res.res_int;
  the_ctx->error = ERR_NONE;
  return 1;
}
  the_ctx->error = ERR_SYNTAX;
  return 0;
}

/*
bool_expr = [ bool_operand bool_operator bool_expr
	| bool_operand 
	] 
*/
static int parse_bool_expr(struct ctx*the_ctx)
{
  char* input = the_ctx->expr;
  struct ctx sub_ctx;
  sub_ctx.navit_obj = the_ctx->navit_obj;
  sub_ctx.obj_func = the_ctx->obj_func;
  sub_ctx.expr = input;
  if(parse_bool_operand(&sub_ctx)) {
    enum operator_t op;
    int op1 = sub_ctx.res.res_bool;
    //expect bool operator
    sub_ctx.expr = next_sym_ptr;
    if((op=parse_bool_operator(&sub_ctx))) {
      next_sym_ptr += whitespace_num(next_sym_ptr) + strlen(get_opstr_by_op(op));
      sub_ctx.expr = next_sym_ptr;
      if(parse_bool_expr(&sub_ctx)) {
        int op2 = sub_ctx.res.res_bool;
        switch(op) {
          case OP_LOGICAL_AND:
            the_ctx->res.res_bool = op1 && op2;
            break;
          case OP_LOGICAL_OR:
            the_ctx->res.res_bool = op1 || op2;
            break;
          case OP_LOGICAL_XOR:
            the_ctx->res.res_bool = op1 ^ op2;
            break;
          default:
            //unknown operator
            the_ctx->error = ERR_INTERNAL;
            return 0;
            break;
        };
        return 1;
      }
    } 
  }

  if(get_op(input,"!",NULL)) {
    next_sym_ptr += 1 + whitespace_num(input);
    sub_ctx.expr = next_sym_ptr;
    if(parse_bool_expr(&sub_ctx)) {
      the_ctx->res.res_bool = ! sub_ctx.res.res_bool;
      the_ctx->error = ERR_NONE;
      return 1;
    }
  }
  sub_ctx.expr = input;
  if(parse_bool_operand(&sub_ctx) ) {
    the_ctx->res.res_bool = sub_ctx.res.res_bool;
    the_ctx->error = ERR_NONE;
    return 1;
  }
  the_ctx->error = ERR_SYNTAX;
  return 0;
}

