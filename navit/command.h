
struct command_table {
	char *command;
	int (*func)(void *data, char *cmd, struct attr **in, struct attr ***out);
};

#define command_cast(x) (int (*)(void *, char *, struct attr **, struct attr ***))(x)

/* prototypes */
struct attr;
struct callback;
struct callback_list;
struct command_saved;
struct command_table;
struct navit;
void command_evaluate_to_void(struct attr *attr, char *expr, int **error);
char *command_evaluate_to_string(struct attr *attr, char *expr, int **error);
int command_evaluate_to_int(struct attr *attr, char *expr, int **error);
int command_evaluate_to_boolean(struct attr *attr, char *expr, int **error);
void command_evaluate(struct attr *attr, char *expr);
void command_add_table_attr(struct command_table *table, int count, void *data, struct attr *attr);
void command_add_table(struct callback_list *cbl, struct command_table *table, int count, void *data);
void command_saved_set_cb(struct command_saved *cs, struct callback *cb);
int command_saved_get_int(struct command_saved *cs);
int command_saved_error(struct command_saved *cs);
struct command_saved *command_saved_new(char *command, struct navit *navit, struct callback *cb);
void command_saved_destroy(struct command_saved *cs);
/* end of prototypes */
