
struct command_table {
	char *command;
	int (*func)(void *data, char *cmd, struct attr **in, struct attr ***out);
};

struct command_saved;

#define command_cast(x) (int (*)(void *, char *, struct attr **, struct attr ***))(x)

void command_evaluate_to_void(struct attr *attr, char *expr, int **error);
char *command_evaluate_to_string(struct attr *attr, char *expr, int **error);
int command_evaluate_to_int(struct attr *attr, char *expr, int **error);
void command_evaluate(struct attr *attr, char *expr);
void command_add_table(struct callback_list *cbl, struct command_table *table, int count, void *data);
struct command_saved *command_saved_new(char *command, struct navit *navit, struct callback *cb);
void command_saved_destroy(struct command_saved *cs);
int command_saved_get_int(struct command_saved *cs);
int command_saved_error(struct command_saved *cs);
void command_saved_set_cb(struct command_saved *cs, struct callback *cb);
