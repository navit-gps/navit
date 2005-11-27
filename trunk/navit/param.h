struct param_list {
	char *name;
	char *value;	
};

void param_add_string(char *name, char *value, struct param_list **param, int *count);
void param_add_dec(char *name, unsigned long value, struct param_list **param, int *count);
void param_add_hex(char *name, unsigned long value, struct param_list **param, int *count);
void param_add_hex_sig(char *name, long value, struct param_list **param, int *count);
