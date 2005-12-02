struct speech;
struct speech *speech_new(void);
int speech_say(struct speech *this, char *text);
int speech_sayf(struct speech *this, char *format, ...);
int speech_destroy(struct speech *this);
