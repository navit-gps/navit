struct window {
	void *priv;
	int (*fullscreen)(struct window *win, int on);
	void (*disable_suspend)(struct window *win);
};
