if(USE_DOWNLOAD)
	set(map_downloader_sources gui_internal_map_downloader.c )
endif(USE_DOWNLOAD)

module_add_library(gui_internal ${map_downloader_sources}
	gui_internal.c gui_internal_bookmark.c
	gui_internal_command.c gui_internal_gesture.c gui_internal_html.c
	gui_internal_menu.c gui_internal_poi.c gui_internal_search.c
	gui_internal_widget.c gui_internal_keyboard.c)
