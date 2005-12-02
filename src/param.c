#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "param.h"

void
param_add_string(char *name, char *value, struct param_list **param, int *count)
{
	if (*count > 0) {
		(*param)->name=malloc(strlen(value)+strlen(name)+2);
		(*param)->value=(*param)->name+strlen(name)+1;
		strcpy((*param)->name, name);
		strcpy((*param)->value, value);
		(*count)--;
		(*param)++;
	}
	
}

void
param_add_dec(char *name, unsigned long value, struct param_list **param, int *count)
{
	char buffer[1024];
	sprintf(buffer, "%ld", value);
	param_add_string(name, buffer, param, count);	
}


void
param_add_hex(char *name, unsigned long value, struct param_list **param, int *count)
{
	char buffer[1024];
	sprintf(buffer, "0x%lx", value);
	param_add_string(name, buffer, param, count);	
}

void
param_add_hex_sig(char *name, long value, struct param_list **param, int *count)
{
	char buffer[1024];
	if (value < 0) 
		sprintf(buffer, "-0x%lx", -value);
	else
		sprintf(buffer, "0x%lx", value);
	param_add_string(name, buffer, param, count);	
}
