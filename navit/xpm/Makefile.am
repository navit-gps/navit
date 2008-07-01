include $(top_srcdir)/Makefile.inc
xpm_DATA = airport.xpm attraction.xpm bank.xpm bar.xpm bus.xpm cafe.xpm camping.xpm car_dealer.xpm church.xpm cinema.xpm dumping-station.xpm exit.xpm fastfood.xpm firebrigade.xpm flag_bk_tr.xpm flag_bk_wh.xpm flag_bl_wh.xpm flag_wh_bk.xpm fuel.xpm golf.xpm highway_exit.xpm hospital.xpm hotel.xpm information.xpm library.xpm museum.xpm nav_left_1.xpm nav_left_2.xpm nav_right_1.xpm nav_right_2.xpm nav_straight.xpm parking.xpm pharmacy.xpm picnic.xpm police.xpm post.xpm restaurant.xpm restroom.xpm skiing.xpm sports.xpm swimming.xpm telephone.xpm theater.xpm tower.xpm traffic_signals.xpm trailerpark.xpm unknown.xpm gc_tradi.xpm gc_multi.xpm gc_mystery.xpm gc_event.xpm
xpm_DATA += gc_reference.xpm gc_webcam.xpm gc_question.xpm gc_stages.xpm
xpm_DATA += shopping.xpm heliport.xpm cursor.xpm peak.xpm mini_roundabout.xpm
xpm_DATA += townhall.xpm level_crossing.xpm boat_ramp.xpm
xpm_DATA += nav_left_1_32.xpm nav_left_2_32.xpm nav_right_1_32.xpm nav_right_2_32.xpm nav_straight_32.xpm
xpm_DATA += osd_minus.png osd_plus.png
svgs = gui_actions.svg gui_active.svg gui_bookmark.svg gui_display.svg gui_home.svg gui_inactive.svg gui_map.svg gui_maps.svg gui_rules.svg gui_settings.svg gui_sound.svg gui_tools.svg gui_fullscreen.svg gui_leave_fullscreen.svg gui_quit.svg gui_stop.svg
svgs += gui_zoom_in.svg gui_zoom_out.svg
if USE_SVG
   xpm_DATA += $(svgs)
endif

if USE_SVG2PNG
%.png:*.svg
	$(srcdir)/svg2png "$@"
xpm_DATA += $(addsuffix .png,$(basename $(svgs)))
if USE_SVG2PNG_SCALES
xpm_DATA += $(foreach scale, @SVG2PNG_SCALES@, $(addsuffix _$(scale)_$(scale).png,$(basename $(svgs))))
endif
endif

EXTRA_DIST = $(xpm_DATA)
#burgerking.ico:
#	wget http://www.burgerking.de/favicon.ico -O burgerking.ico
