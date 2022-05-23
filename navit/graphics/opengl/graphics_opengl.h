struct graphics_opengl_window_system;
struct graphics_opengl_platform;


struct graphics_opengl_window_system_methods {
	void (*destroy)(struct graphics_opengl_window_system *);
	void *(*get_display)(struct graphics_opengl_window_system *);
	void *(*get_window)(struct graphics_opengl_window_system *);
	void (*set_callbacks)(struct graphics_opengl_window_system *, void *data, void *resize, void *button, void *motion, void *keypress);
};

struct graphics_opengl_platform_methods {
	void (*destroy)(struct graphics_opengl_platform *);
	void (*swap_buffers)(struct graphics_opengl_platform *);
};

struct graphics_opengl_window_system *graphics_opengl_x11_new(void *displayname, int w, int h, int depth, struct graphics_opengl_window_system_methods **methods);
struct graphics_opengl_platform *graphics_opengl_egl_new(void *display, void *window, struct graphics_opengl_platform_methods **methods);
