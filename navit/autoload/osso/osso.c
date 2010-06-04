#include <libosso.h>
#include <stdlib.h>
#include "debug.h"
#include "version.h"
#include "item.h"
#include "attr.h"
#include "navit.h"
#include "plugin.h"

osso_context_t *osso_context; 

static void  
navit_osso_display_on(struct navit *this_)  
{
    osso_return_t err;
    err=osso_display_blanking_pause(osso_context);
    dbg(1,"Unblank result: ", err == OSSO_OK ? "Ok" : (err == OSSO_ERROR ? "Error" : "Invalid context"));
}

static gboolean  
osso_cb_hw_state_idle(osso_hw_state_t *state)  
{
    dbg(0,"(inact=%d, save=%d, shut=%d, memlow=%d, state=%d)\n",  
    state->system_inactivity_ind,
    state->save_unsaved_data_ind, state->shutdown_ind,
    state->memory_low_ind, state->sig_device_mode_ind);

    if(state->shutdown_ind)  
    {
        /* we  are going  down, down,  down */
        //navit_destroy(global_navit);
        exit(1);
    }

    g_free(state);  

    return FALSE; 
}

/**
 * * Handle osso events  
 * * @param state Osso hardware state  
 * * @param  data ptr to private data  
 * * @returns nothing  
 **/
static void
osso_cb_hw_state(osso_hw_state_t *state, gpointer data)
{
    osso_hw_state_t *state_copy = g_new(osso_hw_state_t, 1);
    memcpy(state_copy, state, sizeof(osso_hw_state_t));
    g_idle_add((GSourceFunc)osso_cb_hw_state_idle, state_copy);
}


void
plugin_init(void) 
{
    dbg(1,"Installing osso context for org.navit_project.navit\n");  
    osso_context = osso_initialize("org.navit_project.navit",VERSION, TRUE, NULL);  
    if (osso_context == NULL) {  
	dbg(0, "error initiating osso context\n");  
    }  
    osso_hw_set_event_cb(osso_context, NULL, osso_cb_hw_state, NULL);  

    /* add callback to unblank screen on gps event */  
    //navit_add_callback(this_, callback_new_attr_0(callback_cast(plugin_osso_display_on), attr_unsuspend_));
}

void
osso_plugin_destroy(void) 
{
    osso_deinitialize(osso_context); 
}
