#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "coord.h"
#include "data_window.h"
#include "route.h"
#include "cursor.h"
#include "menu.h"
#include "command.h"
#include "transform.h"
#include "street.h"
#include "statusbar.h"
#include "destination.h"
#include "main.h"
#include "container.h"
#include "graphics.h"

void
menu_route_do_update(struct container *co)
{
	if (co->cursor) {
		route_set_position(co->route, cursor_pos_get(co->cursor));
		graphics_redraw(co);
		if (co->statusbar && co->statusbar->statusbar_route_update)
			co->statusbar->statusbar_route_update(co->statusbar, co->route);
	}
}

void
menu_route_update(struct container *co)
{
	menu_route_do_update(co);
	graphics_redraw(co);
}
