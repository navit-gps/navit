#include <libosso.h>
#include <stdlib.h>
#include "debug.h"
#include "version.h"
#include "item.h"
#include "attr.h"
#include "navit.h"
#include "plugin.h"
#include "callback.h"
#include "config_.h"

static osso_context_t *osso_context;
static struct attr callback = { attr_callback };

struct cb_hw_state_trail {
	struct navit* nav;
	osso_hw_state_t *state;
};

static void
osso_display_on(struct navit *this_)
{
	osso_return_t err;
	err = osso_display_blanking_pause(osso_context);
	dbg(1, "Unblank result: ",
	    err == OSSO_OK ? "Ok" : (err ==
				     OSSO_ERROR ? "Error" :
				     "Invalid context"));
}

static gboolean
osso_cb_hw_state_idle(struct cb_hw_state_trail * params)
{
	dbg(0, "(inact=%d, save=%d, shut=%d, memlow=%d, state=%d)\n",
	    params->state->system_inactivity_ind,
	    params->state->save_unsaved_data_ind, params->state->shutdown_ind,
	    params->state->memory_low_ind, params->state->sig_device_mode_ind);

	if (params->state->shutdown_ind) {
		/* we  are going  down, down,  down */
		navit_destroy(params->nav);		
	}

	g_free(params->state);
	g_free(params);

	return FALSE;
}

/**
 * * Handle osso events  
 * * @param state Osso hardware state  
 * * @param  data ptr to private data  
 * * @returns nothing  
 **/
static void
osso_cb_hw_state(osso_hw_state_t * state, gpointer data)
{
	struct navit *nav = (struct navit*)data;
	struct cb_hw_state_trail *params = g_new(struct cb_hw_state_trail,1);
	params->nav=nav;
	params->state = g_new(osso_hw_state_t, 1);
	memcpy(params->state, state, sizeof(osso_hw_state_t));
	g_idle_add((GSourceFunc) osso_cb_hw_state_idle, params);
}

static void
osso_navit(struct navit *nav, int add)
{
	dbg(1, "Installing osso context for org.navit_project.navit\n");
	osso_context = osso_initialize("org.navit_project.navit", VERSION, TRUE, NULL);
	if (osso_context == NULL) {
		dbg(0, "error initiating osso context\n");
	}
	osso_hw_set_event_cb(osso_context, NULL, osso_cb_hw_state, nav);

	if (add > 0) {
		/* add callback to unblank screen */
		navit_add_callback(nav, callback_new_attr_0(callback_cast (osso_display_on), attr_unsuspend));
	}
}

void
plugin_init(void)
{
	//struct callback *cb;

	dbg(2, "enter\n");

	callback.u.callback = callback_new_attr_0(callback_cast(osso_navit), attr_navit);
	config_add_attr(config, &callback);
}

void
plugin_deinit(void)
{
	osso_deinitialize(osso_context);
}
