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

// The two next methods are called from navit.c
int
change_volume(struct audio *this_, const int direction)
{
	dbg(lvl_error,"Calling plugin volume management\n");
        return (this_->meth.volume)(this_->priv, direction);
}

int
playback_do(struct audio *this_, const int action)
{
        return (this_->meth.playback)(this_->priv, action);
}

int
playlists(struct audio *this_, const int action)
{
        return (this_->meth.playlists)(this_->priv, action);
}


int
audio_player_get_playlists_count (struct navit *this, char *function, struct attr **in, struct attr ***out, int *valid)
{

}

char *
audio_player_get_current_playlist_name ()
{
        return "OK!";
}


struct audio *
audio_new(struct attr *parent, struct attr **attrs)
{
	dbg(lvl_error,"Initializing audio plugin\n");
	struct audio *this_;
	struct attr *attr;
	struct audio_priv *(*audiotype_new)(struct audio_methods *meth, struct attr **attrs, struct attr *parent);

        attr=attr_search(attrs, NULL, attr_type);
        if (! attr) {
                dbg(lvl_error,"type missing\n");
                return NULL;
        }
        dbg(lvl_error,"type='%s'\n", attr->u.str);
        audiotype_new=plugin_get_audio_type(attr->u.str);
        dbg(lvl_error,"new=%p\n", audio_new);
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
	dbg(lvl_error, "checking volume method\n");
	if (this_->meth.volume) {
		dbg(lvl_error, "volume=%p\n", this_->meth.volume);
	} else {
		dbg(lvl_error, "The plugin %s cannot manage the volume\n", attr->u.str);
	}
	dbg(lvl_error, "checking playback method\n");
	if (this_->meth.playback) {
		dbg(lvl_error, "playback=%p\n", this_->meth.volume);
	} else {
		dbg(lvl_error, "The plugin %s cannot handle playback\n", attr->u.str);
	}
        dbg(lvl_error,"return %p\n", this_);

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
