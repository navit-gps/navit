
#ifndef __GRAPHICS_SDL_H
#define __GRAPHICS_SDL_H

#include <glib.h>

#ifdef USE_WEBOS
# define USE_WEBOS_ACCELEROMETER
#endif

gboolean graphics_sdl_idle(void *);

#endif
