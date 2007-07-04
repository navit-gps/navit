#include <glib.h>
#include "gui.h"
#include "statusbar.h"
#include "menu.h"
#include "plugin.h"

struct gui *
gui_new(struct navit *nav, const char *type, int w, int h)
{
	struct gui *this_;
	struct gui_priv *(*guitype_new)(struct navit *nav, struct gui_methods *meth, int w, int h);

        guitype_new=plugin_get_gui_type(type);
        if (! guitype_new)
                return NULL;

	this_=g_new0(struct gui, 1);
	this_->priv=guitype_new(nav, &this_->meth, w, h);
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

