#include <glib.h>
#include <navit/main.h>
#include <navit/debug.h>
#include <navit/point.h>
#include <navit/navit.h>
#include <navit/callback.h>
#include <navit/color.h>
#include "graphics.h"
#include <navit/event.h>
#include <navit/command.h>
#include <navit/config_.h>
#include <navit/transform.h>

#include "gui_internal.h"
#include "coord.h"
#include "math.h"
#include "gui_internal_menu.h"
#include "gui_internal_widget.h"
#include "gui_internal_priv.h"
#include "gui_internal_map_downloader.h"
#include "network.h"
#include <pthread.h>

pthread_t dl_thread;
struct map_download_info dl_info;

void
gui_internal_download_update(struct gui_priv * this)
{
    dbg(lvl_debug, "downloading status = %i\n", dl_info.downloading);
	if(dl_info.downloading == 1){
      event_add_timeout(500, 0, this->download_cb);
	} else {
		if(pthread_join(dl_thread, NULL)) {
		    dbg(lvl_error, "Error joining download thread\n");
		}
   }

    if(this->download_data.download_showing) {
        gui_internal_populate_download_table(this);
        graphics_draw_mode(this->gra, draw_mode_begin);
        gui_internal_menu_render(this);
        graphics_draw_mode(this->gra, draw_mode_end);
    }
}

void
gui_internal_map_downloader(struct gui_priv *this, struct widget *wm, void *data)
{
    struct widget *wb, *w, *wr, *wk, *we, *wl;
    wb=gui_internal_menu(this,"Map download");
    w=gui_internal_box_new(this, gravity_center|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(wb, w);
    wr=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(w, wr);
    we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
    gui_internal_widget_append(wr, we);

    struct widget * box;

    if(! this->download_cb)
    {
	  dbg(lvl_error, "Added callback\n");
      this->download_cb = callback_new_1(callback_cast(gui_internal_download_update), this);
      event_add_timeout(500, 0, this->download_cb);
    }

    this->download_data.download_table = gui_internal_widget_table_new(this,gravity_left_top | flags_fill | flags_expand |orientation_vertical,1);

    wb->wfree=gui_internal_download_screen_free;
    this->download_data.download_showing=1;
    this->download_data.download_table->spx = this->spacing;


    box = gui_internal_box_new(this, gravity_left_top| orientation_vertical | flags_fill | flags_expand);

    gui_internal_widget_append(box,this->download_data.download_table);
    box->w=wb->w;
    box->spx = this->spacing;
    this->download_data.download_table->w=box->w;
    gui_internal_widget_append(wb,box);
    gui_internal_populate_download_table(this);

	if ( dl_thread ) {
	  dbg(lvl_error, "Download already in progress\n");

	} else {
	  dbg(lvl_error, "Download will be started\n");
      strcpy (dl_info.url, g_strdup_printf ("http://maps9.navit-project.org/api/map/?bbox=-125.94,32.43,-114.08,42.07&timestamp=161223"));
      strcpy (dl_info.url, g_strdup_printf ("http://www.navit-project.org/maps/osm_bbox_11.3,47.9,11.7,48.2.osm.bz2"));
      dbg(lvl_error,"Url %s\n", dl_info.url);

      if(pthread_create(&dl_thread, NULL, download_map, &dl_info)) {
          dbg(lvl_error, "Error creating download thread\n");
          return 1;
      }

	}

    gui_internal_menu_render(this);
}

void
gui_internal_populate_download_table(struct gui_priv * this)
{   
    struct navigation * nav = NULL;
    struct item * item =NULL;
    struct attr attr,route; 
    struct widget * label = NULL;
    struct widget * row = NULL;
    struct coord c;

	if(this->download_data.download_showing){
        GList *toprow;
        struct item topitem={0};
        toprow=gui_internal_widget_table_top_row(this, this->download_data.download_table);
        if(toprow && toprow->data)
            topitem=((struct widget*)toprow->data)->item;
        gui_internal_widget_table_clear(this,this->download_data.download_table);

        row = gui_internal_widget_table_row_new(this,
                              gravity_left
                              | flags_fill
                              | orientation_horizontal);
        gui_internal_widget_append(this->download_data.download_table,row);
      double percent = dl_info.dl_now / dl_info.dl_total * 100;
      char * text;
      text=g_strdup_printf("Download is %s %lf / %lf = %lf% \n", dl_info.downloading == 1 ? "active" : "inactive" ,dl_info.dl_now, dl_info.dl_total, percent);
      label = gui_internal_label_new(this,text);
      gui_internal_widget_append(row,label);
	  g_free(text);
	}
}


/**
 * @brief Called when the download screen is closed (deallocated).
 *
 * The main purpose of this function is to remove the widgets from
 * references download_data because those widgets are about to be freed.
 */
void
gui_internal_download_screen_free(struct gui_priv * this,struct widget * w)
{
    if(this) {
        this->download_data.download_showing=0;
        this->download_data.download_table=NULL;
        g_free(w);
    }
}
