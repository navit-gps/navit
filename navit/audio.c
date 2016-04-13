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
 * @brief audio_player_get_name
 * @param
 * @return
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
	int ret;
	switch (attr->type) {
	case attr_playing:
		
		break;
	case attr_shuffle:
		
		break;
	case attr_repeat:
	
		break;
	default:
		break;
	}
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
	switch (attr->type) {
	case attr_playing:
		
		break;
	case attr_shuffle:
		
		break;
	case attr_repeat:
	
		break;
	default:
		break;
	}
	if (ret == 1 && attr->type != attr_navit && attr->type != attr_pdl_gps_update)
		this_->attrs=attr_generic_set_attr(this_->attrs, attr);
	return ret != 0;
}


/**
 * Generic add function
 *
 * @param this_ A audio
 * @param attr The attribute to add
 *
 * @return true if the attribute was added, false if not.
 */
int
audio_add_attr(struct audio *this_, struct attr *attr)
{
	int ret=1;
	switch (attr->type) {
	case attr_playing:
		this_->
		break;
	case attr_shuffle:
		
		break;
	case attr_repeat:
	
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
 * @param this_ A audio
 * @param attr
 */
int
audio_remove_attr(struct audio *this_, struct attr *attr)
{
	struct callback *cb;
	this_->attrs=attr_generic_remove_attr(this_->attrs, attr);
	return 0;
}








/**
* @brief
* 
* @param
* 
* @return
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
	dbg(lvl_info,"type='%s'\n", attr->u.str);
	audiotype_new=plugin_get_audio_type(attr->u.str);
	dbg(lvl_info,"new=%p\n", audio_new);
	if (! audiotype_new) {
			dbg(lvl_error,"wrong type '%s'\n", attr->u.str);
			return NULL;
	}
	this_=g_new0(struct audio, 1);
	this_->name = g_strdup(attr->u.str);
	this_->priv = audiotype_new(&this_->meth, attrs, parent);
	if (!this_->priv) {
		dbg(lvl_error, "audio_new failed\n");
		g_free(this_);
		return NULL;
	}
	if (this_->meth.volume) {
		dbg(lvl_info, "%s.volume=%p\n", attr->u.str, this_->meth.volume);
	} else {
		dbg(lvl_error, "The plugin %s cannot manage the volume\n", attr->u.str);
	}
	if (this_->meth.playback) {
		dbg(lvl_info, "%s.playback=%p\n", attr->u.str, this_->meth.playback);
	} else {
		dbg(lvl_error, "The plugin %s cannot handle playback\n", attr->u.str);
	}
	if (this_->meth.action_do) {
		dbg(lvl_info, "%s.action_do=%p\n", attr->u.str, this_->meth.playback);
	} else {
		dbg(lvl_error, "The plugin %s cannot handle action_do's\n", attr->u.str);
	}
	if (this_->meth.tracks) {
		dbg(lvl_info, "%s.tracks=%p\n", attr->u.str, this_->meth.tracks);
	} else {
		dbg(lvl_error, "The plugin %s cannot handle tracks\n", attr->u.str);
	}
	if (this_->meth.playlists) {
		dbg(lvl_info, "%s.playlists=%p\n", attr->u.str, this_->meth.playlists);
	} else {
		dbg(lvl_error, "The plugin %s cannot handle playlists\n", attr->u.str);
	}
     if (this_->meth.actions) {
		dbg(lvl_info, "%s.actions=%p\n", attr->u.str, this_->meth.playlists);
	} else {
		dbg(lvl_error, "The plugin %s cannot handle actions\n", attr->u.str);
	}
	   dbg(lvl_info,"return %p\n", this_);

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
