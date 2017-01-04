#include "debug.h"
#include "glib_slice.h"

#include "item.h"
#include "audio.h"
#include "plugin.h"
#include "xmlconfig.h"

#include "attr.h"
#include <callback.h>
struct object_func audio_func;
struct audio_priv;

// FIXME : we can't name it active because it conflicts with speech.active. Odd!
struct attr audio_active=ATTR_INT(active, 0);
struct attr *audio_default_attrs[]={
	&audio_active,
	NULL,
};

/**
 * @brief audio_player_get_name
 *
 * @return the name of the audio plugin instance
 * 
 */
char* audio_player_get_name (void)
{
		dbg(lvl_error,"//fixme: get the name of the music player... where is the name set?\n");
        return "Music Player";
}

/**
 * Generic get function
 *
 * @param this_ Pointer to a audio structure
 * @param type The attribute type to look for
 * @param attr Pointer to a {@code struct attr} to store the attribute
 * @param iter A audio attr_iter. This is only used for generic attributes; for attributes specific to the audio object it is ignored.
 * @return True for success, false for failure
 */
int
audio_get_attr(struct audio *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter)
{
	int ret=1;
	dbg(lvl_debug, "Get attr: %p (%s), attrs: %p\n", attr, attr_to_name(type), this_->attrs);
	
	ret=this_->meth.attr_get(this_->priv, type, attr);
	if(ret)	
		return ret;
	return attr_generic_get_attr(this_->attrs, NULL, type, attr, iter);
}

/**
 * Generic set function
 *
 * @param this_ A audio
 * @param attr The attribute to set
 * @return False on success, true on failure
 */
int
audio_set_attr(struct audio *this_, struct attr *attr)
{
	int ret=1;
	dbg(lvl_debug, "Set attr: %p (%s), attrs: %p\n", attr, attr_to_name(attr->type), this_->attrs);

	switch (attr->type) {
	case attr_callback:
		callback_list_add(this_->cbl, attr->u.callback);
	case attr_name:
	case attr_playing:
	case attr_shuffle:
	case attr_repeat:
		ret=this_->meth.attr_set(this_->priv, attr);
		break;
	default:
		break;
	}
	if (ret == 1 && attr->type != attr_navit)
		this_->attrs=attr_generic_set_attr(this_->attrs, attr);
	return ret != 0;
}

/*
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
 * /

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

/**
* @brief Creates a new audio plugin instance
* 
* @param parent The parent attribute
* @param attrs a list of attributes from navit.xml
* 
* @return The created audio plugin instance
*/
struct audio *
audio_new(struct attr *parent, struct attr **attrs)
{
	dbg(lvl_info,"Initializing audio plugin\nParent: %p, Attrs: %p\n", parent, attrs);
	struct audio *this_;
	struct attr *attr;
	struct audio_priv *(*audiotype_new)(struct audio_methods *meth,
						struct callback_list * cbl,
						struct attr **attrs,
						struct attr *parent
	);
	/*struct audio_methods *meth, struct callback_list * cbl, struct attr **attrs, struct attr *parent*/
	attr=attr_search(attrs, NULL, attr_type);
	if (! attr) {
			dbg(lvl_error,"type missing\n");
			return NULL;
	}
	dbg(lvl_info,"type='%s'\n", attr->u.str);
	audiotype_new=plugin_get_category_audio(attr->u.str);
	dbg(lvl_info,"new=%p\n", audio_new);
	if (! audiotype_new) {
			dbg(lvl_error,"wrong type '%s'\n", attr->u.str);
			return NULL;
	}
	
	this_=g_new0(struct audio, 1);
	this_->func=&audio_func;

	navit_object_ref((struct navit_object *)this_);
	
	attr=attr_search(attrs, NULL, attr_name);
	if(attr){
		
		this_->name = g_strdup(attr->u.str);
		dbg(lvl_info, "audio name: %s \n", this_->name);
	}
	
	this_->cbl=callback_list_new();
	this_->priv = audiotype_new(&this_->meth, this_->cbl, attrs, parent);
	
	if (!this_->priv) {
		dbg(lvl_error, "audio_new failed\n");
		callback_list_destroy(this_->cbl);
		g_free(this_);
		return NULL;
	}
	dbg(lvl_info, "Attrs: %p\n", attrs);
	/*
	if(attrs != NULL){
		dbg(lvl_info, "*Attrs: %p\n", *attrs);
		if(*attrs != NULL){
			this_->attrs=attr_list_dup(attrs);
		}
	}
	//*/
	dbg(lvl_info, "Attrs: %p\n", this_->attrs);
	//*
	if (this_->meth.volume) {
		dbg(lvl_info, "%s.volume=%p\n", this_->name, this_->meth.volume);
	} else {
		dbg(lvl_error, "The plugin %s cannot manage the volume\n", this_->name);
	}
	if (this_->meth.playback) {
		dbg(lvl_info, "%s.playback=%p\n", this_->name, this_->meth.playback);
	} else {
		dbg(lvl_error, "The plugin %s cannot handle playback\n", this_->name);
	}
	if (this_->meth.action_do) {
		dbg(lvl_info, "%s.action_do=%p\n", this_->name, this_->meth.playback);
	} else {
		dbg(lvl_error, "The plugin %s cannot handle action_do's\n", this_->name);
	}
	if (this_->meth.tracks) {
		dbg(lvl_info, "%s.tracks=%p\n", this_->name, this_->meth.tracks);
	} else {
		dbg(lvl_error, "The plugin %s cannot handle tracks\n", this_->name);
	}
	if (this_->meth.playlists) {
		dbg(lvl_info, "%s.playlists=%p\n", this_->name, this_->meth.playlists);
	} else {
		dbg(lvl_error, "The plugin %s cannot handle playlists\n", this_->name);
	}
     if (this_->meth.actions) {
		dbg(lvl_info, "%s.actions=%p\n", this_->name, this_->meth.playlists);
	} else {
		dbg(lvl_error, "The plugin %s cannot handle actions\n", this_->name);
	}
	//*/
	dbg(lvl_info,"return %p\n", this_);
	
    return this_;
}


/**
 * Creates an attribute iterator to be used with audio plugins
 */
struct attr_iter *
audio_attr_iter_new(void)
{
	return (struct attr_iter *)g_new0(void *,1);
}

/**
 * Destroys a audio attribute iterator
 *
 * @param iter a audio attr_iter
 */
void
audio_attr_iter_destroy(struct attr_iter *iter)
{
	g_free(iter);
}


/**
 * Generic add function
 *
 * @param this_ The audio audio instance that holds the list of attributes
 * @param attr The attribute to add
 *
 * @return true if the attribute was added, false if not.
 */
int
audio_add_attr(struct audio *this_, struct attr *attr)
{
	dbg(lvl_debug, "Add attr: %p (%s), attrs: %p\n", attr, attr_to_name(attr->type), this_->attrs);
	int ret=1;
	switch (attr->type) {
	case attr_callback:
		dbg(lvl_debug, "Add attr: %p (%s), attrs: %p, to cbl: %p\n", attr, attr_to_name(attr->type), this_->attrs, this_->cbl);
		callback_list_add(this_->cbl, attr->u.callback);
		break;
	default:
		break;
	}
	if (ret)
		this_->attrs=attr_generic_add_attr(this_->attrs, attr);
	return ret;
}

/**
 * @brief Generic remove function.
 *
 * Used to remove a callback from the audio.
 * @param this_ The audio audio instance that holds the list of attributes
 * @param attr the attribute to be removed
 * @return True on success false on failure
 */
int
audio_remove_attr(struct audio *this_, struct attr *attr)
{
	dbg(lvl_debug, "Remove attr: %p (%s), attrs: %p\n", attr, attr_to_name(attr->type), this_->attrs);
	switch (attr->type) {
	case attr_callback:
		callback_list_remove(this_->cbl, attr->u.callback);
		break;
	default:
		this_->attrs=attr_generic_remove_attr(this_->attrs, attr);
		return 0;
	}
	return 1;
}


struct object_func audio_func = {
	attr_audio,
        (object_func_new)			audio_new,
        (object_func_get_attr)		audio_get_attr,
        (object_func_iter_new)		audio_attr_iter_new,
        (object_func_iter_destroy)	audio_attr_iter_destroy,
        (object_func_set_attr)		audio_set_attr,
        (object_func_add_attr)		audio_add_attr,
        (object_func_remove_attr)	audio_remove_attr,
        (object_func_init)			NULL,
        (object_func_destroy)		NULL,
        (object_func_dup)			NULL,
        (object_func_ref)			navit_object_ref,
        (object_func_unref)			navit_object_unref,
};
