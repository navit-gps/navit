.. _bookmarks_development:

Bookmarks Development
=====================

How to work with bookmarks.

First of all - you have to acquire bookmarks object handle:

.. code:: c

   struct attr mattr;
   navit_get_attr(navit_object, attr_bookmarks, &mattr, NULL);

Next you have an option, to get underlying bookmarks map (yes,it's a
map, so `you can use it in your mapset and see your bookmarks on
map <http://trac.navit-project.org/ticket/735>`__) and work with it in
usual way:

.. code:: c

   struct map *map;
   struct map_rect *mr;
   if (map=bookmarks_get_map(mattr.u.bookmarks) && map && (mr=map_rect_new(u.map, NULL)) {
       while ((item=map_rect_get_item(mr))) {
           //item processing
       }
   }

Bookmarks items have 'label' attribute, containing item name, 'path'
attribute, containing full item path, and type:

-  type_bookmark is for bookmarks
-  type_bookmark_folder is for bookmark folders

'type_bookmark' items also have coordinates set.

Another one way of accessing bookmarks - bookmarks API:

You still need the bookmarks object handle, but instead of getting map
and iterating over it's items, you may ask directly for items from
bookmarks object: while ((item=bookmarks_get_item(mattr.u.bookmarks))) {
//Process as usual item }

The most amazing thing, is that bookmarks API handles bookmarks tree
internally, with the following calls:

-  void bookmarks_move_root(struct bookmarks \*this_) - set's current
   position in tree to a tree root
-  void bookmarks_move_up(struct bookmarks \*this_) - moves current
   position in tree one level up (if we already at upper level it will
   do nothing)
-  int bookmarks_move_down(struct bookmarks \*this_,const char\* name) -
   looks for folder labeled 'name' and tries to move current position
   down to it. Returns FALSE in case of failure and TRUE when actual
   move happened.

There are also exists two helper calls:

-  void bookmarks_item_rewind(struct bookmarks\* this_) - tells
   bookmarks_get_item to start from the beginning.

const char\* bookmarks_item_cwd(struct bookmarks\* this_); - returns
label of a current tree folder (or NULL if you are at topmost folder)

Bookmarks will be useless without ability to change bookmarks data. Here
we goe, with some editing stuff:

-  int bookmarks_add_bookmark(struct bookmarks \*this_, struct pcoord
   \*c, const char \*description) - Adds a bookmark labeled
   "description" and coordinates "c". If you pass NULL instead of
   coordinates, it will create folder, not bookmark.
-  int bookmarks_cut_bookmark(struct bookmarks \*this_, const char
   \*label) - Copies data of bookmark labeled "label" to temporary
   storage and deletes the bookmar. Be warned - temporary storage is not
   persistent, so you will loose you bookmark if you quit the navit and
   didn't paste the bookmark somewhere.
-  int bookmarks_copy_bookmark(struct bookmarks \*this_, const char
   \*label) - Copies data of bookmark labeled "label" to temporary
   storage. Like "cut", but without deletion
-  int bookmarks_paste_bookmark(struct bookmarks \*this_) - Copies
   bookmark from the temporary storage to current position in tree. If
   bookmark with same name already exists, it will be replaced.
   Temporary storage isn't cleaned after that operation, so you may
   paste several times.
-  int bookmarks_rename_bookmark(struct bookmarks \*this_, const char
   \*oldName, const char\* newName) - Changes name for bookmark
   'oldName' to 'newName'
-  int bookmarks_delete_bookmark(struct bookmarks \*this_, const char
   \*label) - Deletes bookmark labeled "label"

All data modification functions operate only at current tree position.
You have to select required position before calling modification
function. Those functions return TRUE if wverything went fine and FALSE
otherwise. If the bookmarks with specified name couldn't be found, those
fnctions will return FALSE and do nothing.

The last important thing is a callbak, issued on map change:

.. code:: c

       bookmarks_add_callback(attr.u.bookmarks, callback_new_attr_1(callback_cast(gui_gtk_bookmarks_update), attr_bookmark_map, this));

It is issued only after bookmark operation was successfully written to
file.
