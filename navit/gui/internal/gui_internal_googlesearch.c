/* vim: set tabstop=4 expandtab: */
#include <glib.h>
#include <navit/main.h>
#include <navit/debug.h>
#include <navit/point.h>
#include <navit/navit.h>
#include <navit/callback.h>
#include <navit/color.h>
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
#include "gui_internal_googlesearch.h"
#include "network.h"
#include <time.h> // Benchmarking the multithreading code, to be removed
#include <pthread.h>

#include "jansson.h"

extern const char *googleplaces_apikey;

struct googleplace {
    char * id;
    char * name;
    struct pcoord c;
    struct coord_geo g;
    struct gui_priv *gui_priv;
    struct widget *wm;
};

/**
 * @brief	Fetches the details about a given place
 * @param[in]	id	- the googleplace id
 *
 * @return	googleplace - an object containing the details about the place
 *
 * Fetches the details about a given place
 *
 */
struct googleplace 
fetch_googleplace_details(char * id)
{
      char url[256];
      strcpy (url, g_strdup_printf ("https://maps.googleapis.com/maps/api/place/details/json?key=%s&placeid=%s",googleplaces_apikey,id));
      dbg(0,"Url %s\n", url);

      json_t *root;
      json_error_t error;
      char * item_js= fetch_url_to_string(url);
      dbg(1,"%s\n",item_js);
      root = json_loads (item_js, 0, &error);
      free(item_js);
      if(!root)
      {
          dbg(0,"Invalid json for url %s, giving up for place id %s\n",url,id);
          json_decref (root);
          return;
      }

      struct googleplace gp;
      json_t *result, *geometry, *location, *name;
      result = json_object_get (root, "result");
      geometry = json_object_get (result, "geometry");
      name = json_object_get (result, "name");
      location = json_object_get (geometry, "location");
      gp.g.lat = json_real_value (json_object_get (location, "lat"));
      gp.g.lng = json_real_value (json_object_get (location, "lng"));
      gp.c.pro=projection_mg;
      struct coord c;
      transform_from_geo (projection_mg, &gp.g, &c);
      gp.c.x=c.x;
      gp.c.y=c.y;
      dbg (0, "Item %s as at : %4.16f x %4.16f [ %x x %x ]\n", json_string_value(name), gp.g.lat, gp.g.lng, gp.c.x, gp.c.y);
      gp.name=g_strdup(json_string_value(name));
      json_decref (root);
      return gp;
}

/**
 * @brief	Set the destination to a given place, read from a widget
 * @param[in]	this	- the current gui_priv object
 *              wm      - the widget containing the place reference
 *              data    - container for extra datas. Not used here
 *
 * @return	nothing
 *
 * Set the destination to a given place, read from a widget
 *
 */
static void
googlesearch_set_destination (struct gui_priv *this, struct widget *wm, void *data)
{
  struct googleplace gp=fetch_googleplace_details(wm->name);
  dbg (0, "%s c=%d:0x%x,0x%x [ %4.16f x %4.16f }\n", gp.name, gp.c.pro, gp.c.x, gp.c.y, gp.g.lat, gp.g.lng);
  navit_set_destination (this->nav, &gp.c, gp.name, 1);
  gui_internal_prune_menu (this, NULL);
}

/**
 * @brief	Updates the search results list, autocompletion
 * @param[in]	this	- the current gui_priv object
 *              wm      - the widget containing the text input
 *              data    - container for extra datas. Not used here
 *
 * @return	nothing
 *
 * Updates the search results list, refining the search as the user types
 *
 */
static void
gui_internal_cmd_googlesearch_filter_do(struct gui_priv *this, struct widget *wm, void *data)
{
    struct widget *w=data;

    if(!w->text)
        return;

    char *prefix = 0;
    char track_icon[64];
    struct coord_geo g;

    struct transformation *trans;
    trans = navit_get_trans (this->nav);
    struct coord c;
    // We want the search to be relative to where we clicked on the map
    c.x = this->clickp.x;
    c.y = this->clickp.y;

    transform_to_geo (transform_get_projection (trans), &c, &g);

    dbg (1, "googlesearch called for %d x %d, converted to %4.16f x %4.16f\n", wm->c.x, wm->c.y, g.lat, g.lng);

    char *baseurl = "https://maps.googleapis.com/maps/api/place/autocomplete/json?";
    char url[256];
    char lat_string[50];
    snprintf (lat_string, 50, "%f", g.lat);
    char lng_string[50];
    snprintf (lng_string, 50, "%f", g.lng);
    char *radius="20000";

    strcpy (url, g_strdup_printf ("%slocation=%s,%s&key=%s&input=%s&radius=%s",  baseurl, lat_string, lng_string, googleplaces_apikey, w->text, radius));

    char * js=fetch_url_to_string(url);

    json_t *root;
    json_error_t error;
    root = json_loads (js, 0, &error);

    json_t *response, *venues;
    response = json_object_get (root, "response");
    venues = json_object_get (root, "predictions");

    struct timespec now, tmstart;
    clock_gettime(CLOCK_REALTIME, &tmstart);

    int i = 0;

    // The autocomplete API returns max 5 results
    pthread_t thread_id[5];
    //  struct googleplace gp[5];
    for (i = 0; i < json_array_size (venues); i++)
    {
        json_t *venue, *description, *id;
        venue = json_array_get (venues, i);
        description = json_object_get (venue, "description");
        id = json_object_get (venue, "place_id");

        dbg (1, "Found [%i] %s with id %s\n", i, json_string_value (description), json_string_value (id));
        strcpy (track_icon, "default");
#if 0
    /* Threads were not efficient on a mobile connection. They have been deactivated */
      gp[i].id=g_strdup(json_string_value (id));
      gp[i].description=g_strdup(json_string_value (description));
      gp[i].gui_priv=this;
      gp[i].wm=wm;
      pthread_create( &thread_id[i], NULL, &fetch_googleplace_details_as_thread, &gp[i] );
#endif
  
        struct widget *wtable=gui_internal_menu_data(this)->search_list;
        struct widget *wc;
        struct widget *row;
        gui_internal_widget_append (wtable, row =
            gui_internal_widget_table_row_new (this,
                gravity_left | orientation_horizontal | flags_fill));
        gui_internal_widget_append (row, wc =
            gui_internal_button_new_with_callback (this,
                json_string_value(description),
                image_new_xs (this, "gui_active"),
                gravity_left_center | orientation_horizontal | flags_fill,
                googlesearch_set_destination,
                NULL));
    
        wc->selection_id = wm->selection_id;
        wc->name = g_strdup (json_string_value(id));
        wc->c.pro = projection_mg;
        wc->prefix = g_strdup (wm->prefix);
    }
#if 0
   int j;
   for(j=0; j < json_array_size (venues); j++)
   {
        dbg(1,"Checking thread #%i with p:%p\n",j,thread_id[j]);
        pthread_join( thread_id[j], NULL);
   }
#endif
    clock_gettime(CLOCK_REALTIME, &now);

    double seconds = (double)((now.tv_sec+now.tv_nsec*1e-9) - (double)(tmstart.tv_sec+tmstart.tv_nsec*1e-9));
    dbg(0,"wall time %fs\n", seconds);
    gui_internal_menu_render(this);
    g_free (prefix);
    json_decref (root);
}

/**
 * @brief	Callback called when the user types in the search box
 * @param[in]	this	- the current gui_priv object
 *              wm      - the parent widget
 *              data    - container for extra datas. Not used here
 *
 * @return nothing
 *
 * This callback is called everytime the user types in the search box
 *
 */
static void
gui_internal_cmd_google_filter_changed(struct gui_priv *this, struct widget *wm, void *data)
{
//        if (wm->text && wm->reason==gui_internal_reason_keypress_finish) {
//                gui_internal_cmd_googlesearch_filter_do(this, wm, wm);
//        }
    if (wm->text) {
        gui_internal_widget_table_clear(this, gui_internal_menu_data(this)->search_list);
        gui_internal_cmd_googlesearch_filter_do(this, wm, wm);
    }
}

/**
 * @brief	Builds the Googleplaces search menu
 * @param[in]	this	- the current gui_priv object
 *              wm      - the parent widget
 *              data    - container for extra datas. Not used here
 *
 * @return	nothing
 *
 * Builds the Googleplaces search menu
 *
 */
void
gui_internal_googlesearch_search(struct gui_priv *this, struct widget *wm, void *data)
{
    struct widget *wb, *w, *wr, *wk, *we, *wl;
    int keyboard_mode;
    keyboard_mode=2+gui_internal_keyboard_init_mode(getenv("LANG"));
    wb=gui_internal_menu(this,"Search");
    w=gui_internal_box_new(this, gravity_center|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(wb, w);
    wr=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(w, wr);
    we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
    gui_internal_widget_append(wr, we);

    gui_internal_widget_append(we, wk=gui_internal_label_new(this, NULL));
    wk->state |= STATE_EDIT|STATE_EDITABLE;
    wk->func=gui_internal_cmd_google_filter_changed;
    wk->background=this->background;
    wk->flags |= flags_expand|flags_fill;
    wk->name=g_strdup("POIsFilter");
    wk->c=wm->c;
    dbg (0, "googlesearch filter called for %d x %d\n", this->clickp.x, this->clickp.y);
    gui_internal_widget_append(we, wb=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
    wb->state |= STATE_SENSITIVE;
    wb->func = gui_internal_cmd_googlesearch_filter_do;
    wb->name=g_strdup("NameFilter");
    wb->c=this->clickp;
    wb->data=wk;
    wl=gui_internal_widget_table_new(this,gravity_left_top | flags_fill | flags_expand |orientation_vertical,1);
    gui_internal_widget_append(wr, wl);
    gui_internal_menu_data(this)->search_list=wl;

    if (this->keyboard)
        gui_internal_widget_append(w, gui_internal_keyboard(this,keyboard_mode));
    gui_internal_menu_render(this);
}
