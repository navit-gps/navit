if(USE_GOOGLEPLACES)
        set(api_LIBS "-ljansson -lpthread")
        set(googleplaces_sources gui_internal_googlesearch.c )
endif(USE_GOOGLEPLACES)
set(gui_internal_LIBS ${api_LIBS})
module_add_library(gui_internal ${googleplaces_sources}
        gui_internal.c gui_internal_bookmark.c
        gui_internal_command.c gui_internal_gesture.c gui_internal_html.c
        gui_internal_menu.c gui_internal_poi.c gui_internal_search.c
        gui_internal_widget.c gui_internal_keyboard.c)
