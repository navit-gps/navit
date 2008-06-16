struct window {
	void *priv;
	int (*fullscreen)(struct window *win, int on);
};
