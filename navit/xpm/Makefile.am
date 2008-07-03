include $(top_srcdir)/Makefile.inc
xpm_DATA  = airport.xpm
xpm_DATA += attraction.xpm
xpm_DATA += bank.xpm
xpm_DATA += bar.xpm
xpm_DATA += boat_ramp.xpm
xpm_DATA += bus.xpm
xpm_DATA += cafe.xpm
xpm_DATA += camping.xpm
xpm_DATA += car_dealer.xpm
xpm_DATA += church.xpm
xpm_DATA += cinema.xpm
xpm_DATA += cursor.xpm
xpm_DATA += dumping-station.xpm
xpm_DATA += exit.xpm
xpm_DATA += fastfood.xpm
xpm_DATA += firebrigade.xpm
xpm_DATA += fuel.xpm 
xpm_DATA += flag_bk_tr.xpm flag_bk_wh.xpm flag_bl_wh.xpm flag_wh_bk.xpm
xpm_DATA += golf.xpm
xpm_DATA += gc_tradi.xpm
xpm_DATA += gc_multi.xpm
xpm_DATA += gc_mystery.xpm
xpm_DATA += gc_event.xpm
xpm_DATA += gc_reference.xpm
xpm_DATA += gc_webcam.xpm
xpm_DATA += gc_question.xpm
xpm_DATA += gc_stages.xpm
xpm_DATA += heliport.xpm
xpm_DATA += highway_exit.xpm
xpm_DATA += hospital.xpm hotel.xpm
xpm_DATA += information.xpm
xpm_DATA += level_crossing.xpm
xpm_DATA += library.xpm
xpm_DATA += mini_roundabout.xpm
xpm_DATA += museum.xpm
xpm_DATA += nav_left_1.xpm nav_left_2.xpm
xpm_DATA += nav_right_1.xpm nav_right_2.xpm
xpm_DATA += nav_straight.xpm nav_straight_32.xpm
xpm_DATA += nav_left_1_32.xpm nav_left_2_32.xpm
xpm_DATA += nav_right_1_32.xpm nav_right_2_32.xpm
xpm_DATA += parking.xpm
xpm_DATA += peak.xpm
xpm_DATA += pharmacy.xpm
xpm_DATA += picnic.xpm
xpm_DATA += police.xpm
xpm_DATA += post.xpm
xpm_DATA += restaurant.xpm
xpm_DATA += restroom.xpm
xpm_DATA += shopping.xpm
xpm_DATA += skiing.xpm
xpm_DATA += sports.xpm
xpm_DATA += swimming.xpm
xpm_DATA += telephone.xpm
xpm_DATA += theater.xpm
xpm_DATA += tower.xpm
xpm_DATA += townhall.xpm
xpm_DATA += traffic_signals.xpm
xpm_DATA += trailerpark.xpm
xpm_DATA += unknown.xpm

svgs  = gui_actions.svg
svgs += gui_active.svg
svgs += gui_bookmark.svg
svgs += gui_display.svg
svgs += gui_home.svg
svgs += gui_inactive.svg
svgs += gui_map.svg
svgs += gui_maps.svg
svgs += gui_rules.svg
svgs += gui_settings.svg
svgs += gui_sound.svg gui_tools.svg
svgs += gui_fullscreen.svg
svgs += gui_leave_fullscreen.svg
svgs += gui_quit.svg gui_stop.svg
svgs += gui_zoom_in.svg
svgs += gui_zoom_out.svg

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
