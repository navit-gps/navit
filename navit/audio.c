#include "debug.h"
#include "glib_slice.h"
#include "item.h"
#include "audio.h"
#include "plugin.h"
#include "xmlconfig.h"

struct object_func audio_func;


// FIXME : we can't name it active because it conflicts with speech.active. Odd!
struct attr audio_active=ATTR_INT(active, 0);
struct attr *audio_default_attrs[]={
	&audio_active,
	NULL,
};

char *
audio_player_get_name ()
{
		//fixme: get the name of the music player... where is the name set?
        return "Music Player";
}

struct audio *
audio_new(struct attr *parent, struct attr **attrs)
{
	dbg(lvl_debug,"Initializing audio plugin\n");
	struct audio *this_;
	struct attr *attr;
	struct audio_priv *(*audiotype_new)(struct audio_methods *meth, struct attr **attrs, struct attr *parent);

        attr=attr_search(attrs, NULL, attr_type);
        if (! attr) {
                dbg(lvl_error,"type missing\n");
                return NULL;
        }
        dbg(lvl_debug,"type='%s'\n", attr->u.str);
        audiotype_new=plugin_get_audio_type(attr->u.str);
        dbg(lvl_debug,"new=%p\n", audio_new);
        if (! audiotype_new) {
                dbg(lvl_error,"wrong type '%s'\n", attr->u.str);
                return NULL;
        }
        this_=g_new0(struct audio, 1);
	//this_->name = g_strdup(attr->u.str);
	this_->priv = audiotype_new(&this_->meth, attrs, parent);
	if (!this_->priv) {
		dbg(lvl_error, "audio_new failed\n");
		g_free(this_);
		return NULL;
	}
	if (this_->meth.volume) {
		dbg(lvl_debug, "%s.volume=%p\n", attr->u.str, this_->meth.volume);
	} else {
		dbg(lvl_debug, "The plugin %s cannot manage the volume\n", attr->u.str);
	}
	if (this_->meth.playback) {
		dbg(lvl_debug, "%s.playback=%p\n", attr->u.str, this_->meth.playback);
	} else {
		dbg(lvl_debug, "The plugin %s cannot handle playback\n", attr->u.str);
	}
	if (this_->meth.action_do) {
		dbg(lvl_debug, "%s.action_do=%p\n", attr->u.str, this_->meth.playback);
	} else {
		dbg(lvl_debug, "The plugin %s cannot handle action_do's\n", attr->u.str);
	}
	if (this_->meth.tracks) {
		dbg(lvl_debug, "%s.tracks=%p\n", attr->u.str, this_->meth.tracks);
	} else {
		dbg(lvl_debug, "The plugin %s cannot handle tracks\n", attr->u.str);
	}
	if (this_->meth.playlists) {
		dbg(lvl_debug, "%s.playlists=%p\n", attr->u.str, this_->meth.playlists);
	} else {
		dbg(lvl_debug, "The plugin %s cannot handle playlists\n", attr->u.str);
	}
     if (this_->meth.actions) {
		dbg(lvl_debug, "%s.actions=%p\n", attr->u.str, this_->meth.playlists);
	} else {
		dbg(lvl_debug, "The plugin %s cannot handle actions\n", attr->u.str);
	}
	   dbg(lvl_debug,"return %p\n", this_);

        return this_;
}

struct object_func audio_func = {
	attr_audio,
        (object_func_new)audio_new,
        (object_func_get_attr)NULL,
        (object_func_iter_new)NULL,
        (object_func_iter_destroy)NULL,
        (object_func_set_attr)NULL,
        (object_func_add_attr)NULL,
        (object_func_remove_attr)NULL,
        (object_func_init)NULL,
        (object_func_destroy)NULL,
        (object_func_dup)NULL,
        (object_func_ref)navit_object_ref,
        (object_func_unref)navit_object_unref,
};
