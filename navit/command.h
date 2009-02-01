
struct command_table {
	char *command;
	int (*func)(void *data, char *cmd, struct attr **in, struct attr ***out);
};

#define command_cast(x) (int (*)(void *, char *, struct attr **, struct attr ***))(x)

void command_evaluate_to_void(struct attr *attr, char *expr);
void command_add_table(struct callback_list *cbl, struct command_table *table, int count, void *data);

