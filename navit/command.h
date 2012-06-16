/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2010 Navit Team
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

#ifndef NAVIT_COMMAND_H
#define NAVIT_COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif

struct command_table {
	const char *command;
	int (*func)(void *data, char *cmd, struct attr **in, struct attr ***out);
};

#define command_cast(x) (int (*)(void *, char *, struct attr **, struct attr ***))(x)

/* prototypes */
enum attr_type;
struct attr;
struct callback;
struct callback_list;
struct command_saved;
struct command_table;
struct navit;
enum attr_type command_evaluate_to_attr(struct attr *attr, char *expr, int *error, struct attr *ret);
void command_evaluate_to_void(struct attr *attr, char *expr, int *error);
char *command_evaluate_to_string(struct attr *attr, char *expr, int *error);
int command_evaluate_to_int(struct attr *attr, char *expr, int *error);
int command_evaluate_to_boolean(struct attr *attr, const char *expr, int *error);
void command_evaluate(struct attr *attr, const char *expr);
void command_add_table_attr(struct command_table *table, int count, void *data, struct attr *attr);
void command_add_table(struct callback_list *cbl, struct command_table *table, int count, void *data);
void command_saved_set_cb(struct command_saved *cs, struct callback *cb);
int command_saved_get_int(struct command_saved *cs);
int command_saved_error(struct command_saved *cs);
struct command_saved *command_saved_new(char *command, struct navit *navit, struct callback *cb, int async);
void command_saved_destroy(struct command_saved *cs);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

 #endif /* NAVIT_COMMAND_H */
