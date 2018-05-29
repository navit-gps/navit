/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "param.h"

void param_add_string(const char *name, const char *value, struct param_list **param, int *count) {
    char *param_name;
    char *param_value;
    if (*count > 0) {
        param_name=malloc(strlen(value)+strlen(name)+2);
        param_value=param_name+strlen(name)+1;
        strcpy(param_name, name);
        strcpy(param_value, value);
        (*param)->name=param_name;
        (*param)->value=param_value;

        (*count)--;
        (*param)++;
    }

}

void param_add_dec(const char *name, unsigned long value, struct param_list **param, int *count) {
    char buffer[1024];
    sprintf(buffer, "%ld", value);
    param_add_string(name, buffer, param, count);
}


void param_add_hex(const char *name, unsigned long value, struct param_list **param, int *count) {
    char buffer[1024];
    sprintf(buffer, "0x%lx", value);
    param_add_string(name, buffer, param, count);
}

void param_add_hex_sig(const char *name, long value, struct param_list **param, int *count) {
    char buffer[1024];
    if (value < 0)
        sprintf(buffer, "-0x%lx", -value);
    else
        sprintf(buffer, "0x%lx", value);
    param_add_string(name, buffer, param, count);
}
