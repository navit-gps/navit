
#ifndef __EVENT_SDL_H
#define __EVENT_SDL_H

#include "SDL.h"
#include "config.h"
#include "callback.h"
#include "event.h"
#include "graphics_sdl.h"

#ifdef USE_WEBOS
# define SDL_USEREVENT_CODE_TIMER 0x1
# define SDL_USEREVENT_CODE_CALL_CALLBACK 0x2
# define SDL_USEREVENT_CODE_IDLE_EVENT 0x4
# define SDL_USEREVENT_CODE_WATCH 0x8
# ifdef USE_WEBOS_ACCELEROMETER
#  define SDL_USEREVENT_CODE_ROTATE 0xA
# endif
#endif

struct event_timeout {
    SDL_TimerID id;
    int multi;
    struct callback *cb;
};

struct idle_task {
    int priority;
    struct callback *cb;
};

struct event_watch {
    struct pollfd *pfd;
    struct callback *cb;
};

int quit_event_loop 		= 0; // quit the main event loop
static GPtrArray *idle_tasks	= NULL;

void event_sdl_watch_thread (GPtrArray *);

void event_sdl_register(void);

#endif
