#include <glib.h>
#include "debug.h"
#include "gui.h"
#include "statusbar.h"
#include "menu.h"
#include "data_window.h"
#include "plugin.h"

struct gui *
gui_new(struct navit *nav, const char *type, struct attr **attrs)
{
	struct gui *this_;
	struct gui_priv *(*guitype_new)(struct navit *nav, struct gui_methods *meth, struct attr **attrs);

        guitype_new=plugin_get_gui_type(type);
        if (! guitype_new)
                return NULL;

	this_=g_new0(struct gui, 1);
	this_->priv=guitype_new(nav, &this_->meth, attrs);
	return this_;
}

struct statusbar *
gui_statusbar_new(struct gui *gui)
{
	struct statusbar *this_;
	if (! gui->meth.statusbar_new)
		return NULL;
	this_=g_new0(struct statusbar, 1);
	this_->priv=gui->meth.statusbar_new(gui->priv, &this_->meth);
	if (! this_->priv) {
		g_free(this_);
		return NULL;
	}
	return this_;
}

struct menu *
gui_menubar_new(struct gui *gui)
{
	struct menu *this_;
	if (! gui->meth.menubar_new)
		return NULL;
	this_=g_new0(struct menu, 1);
	this_->priv=gui->meth.menubar_new(gui->priv, &this_->meth);
	if (! this_->priv) {
		g_free(this_);
		return NULL;
	}
	return this_;
}


struct menu *
gui_toolbar_new(struct gui *gui)
{
	struct menu *this_;
	if (! gui->meth.toolbar_new)
		return NULL;
	this_=g_new0(struct menu, 1);
	this_->priv=gui->meth.toolbar_new(gui->priv, &this_->meth);
	if (! this_->priv) {
		g_free(this_);
		return NULL;
	}
	return this_;
}

struct menu *
gui_popup_new(struct gui *gui)
{
	struct menu *this_;
	if (! gui->meth.popup_new)
		return NULL;
	this_=g_new0(struct menu, 1);
	this_->priv=gui->meth.popup_new(gui->priv, &this_->meth);
	if (! this_->priv) {
		g_free(this_);
		return NULL;
	}
	return this_;
}

struct datawindow *
gui_datawindow_new(struct gui *gui, char *name, struct callback *click, struct callback *close)
{
	struct datawindow *this_;
	if (! gui->meth.datawindow_new)
		return NULL;
	this_=g_new0(struct datawindow, 1);
	this_->priv=gui->meth.datawindow_new(gui->priv, name, click, close, &this_->meth);
	if (! this_->priv) {
		g_free(this_);
		return NULL;
	}
	return this_;
}

int
gui_add_bookmark(struct gui *gui, struct coord *c, char *description)
{
	int ret;
	dbg(2,"enter\n");
	if (! gui->meth.add_bookmark)
		return 0;
	ret=gui->meth.add_bookmark(gui->priv, c, description);
	
	dbg(2,"ret=%d\n", ret);
	return ret;
}

int
gui_set_graphics(struct gui *this_, struct graphics *gra)
{
	if (! this_->meth.set_graphics)
		return 1;
	return this_->meth.set_graphics(this_->priv, gra);
}

int
gui_has_main_loop(struct gui *this_)
{
	if (! this_->meth.run_main_loop)
		return 0;
	return 1;
}

int
gui_run_main_loop(struct gui *this_)
{
	if (! gui_has_main_loop(this_))
		return 1;
	return this_->meth.run_main_loop(this_->priv);
}

