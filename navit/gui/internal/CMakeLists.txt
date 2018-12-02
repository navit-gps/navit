if (GUI_INTERNAL_VISUAL_DBG)
	add_definitions ("-DGUI_INTERNAL_VISUAL_DBG")
endif(GUI_INTERNAL_VISUAL_DBG)

module_add_library(gui_internal gui_internal.c gui_internal_bookmark.c gui_internal_command.c gui_internal_gesture.c
                   gui_internal_html.c gui_internal_menu.c gui_internal_poi.c gui_internal_search.c gui_internal_widget.c
                   gui_internal_keyboard.c)
