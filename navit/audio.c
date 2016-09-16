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

/**
 * @brief instantiates an audio plugin
 *
 * This function initializes an audio plugin according to the settings 
 * from navit.xml. Plugins can have different capabilities:
 * - they can manage the global audio volume
 * - they can manage the playback (previous/next, play/pause)
 * - they can support a list of tracks
 * - they can support a list of playlists
 * 
 * A plugin does not need to have all capabilities. Usually,
 * an output plugin will support setting up the volume, and
 * a playback plugin should not need to care about the volume.
 *
 * @param attr *parent the parent attribute object
 * @param attr *attrs the currents attributes object
 * 
 * @return the audio object if initialization was successful
 */

struct audio *
audio_new(struct attr *parent, struct attr **attrs)
{
	dbg(lvl_info,"Initializing audio plugin\n");
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
	this_->priv = audiotype_new(&this_->meth, attrs, parent);
	if (!this_->priv) {
		dbg(lvl_error, "audio_new failed\n");
		g_free(this_);
		return NULL;
	}
	if (this_->meth.volume) {
		dbg(lvl_info, "%s.volume=%p\n", attr->u.str, this_->meth.volume);
	} else {
		dbg(lvl_info, "The plugin %s cannot manage the volume\n", attr->u.str);
	}
	if (this_->meth.playback) {
		dbg(lvl_info, "%s.playback=%p\n", attr->u.str, this_->meth.playback);
	} else {
		dbg(lvl_info, "The plugin %s cannot handle playback\n", attr->u.str);
	}
	if (this_->meth.tracks) {
		dbg(lvl_info, "%s.tracks=%p\n", attr->u.str, this_->meth.tracks);
	} else {
		dbg(lvl_info, "The plugin %s cannot handle tracks\n", attr->u.str);
	}
	if (this_->meth.playlists) {
		dbg(lvl_info, "%s.playlists=%p\n", attr->u.str, this_->meth.playlists);
	} else {
		dbg(lvl_info, "The plugin %s cannot handle playlists\n", attr->u.str);
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
