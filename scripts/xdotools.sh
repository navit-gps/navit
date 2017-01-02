sudo apt-get install xdotool
# Use xinput test 4 when running x11vnc on the circleci server to find mouse coordinates

[ -d $CIRCLE_ARTIFACTS/frames/ ] || mkdir $CIRCLE_ARTIFACTS/frames/

event=0

send_event (){
    xdotool $@
    sleep 1
    file=`printf "%05d\n" $event`

    import -window root $CIRCLE_ARTIFACTS/frames/tmp.png
    if [[ "$1" == "mousemove" ]]; then
    	composite -geometry +$2+$3 ~/navit/ci/pointer-64.png $CIRCLE_ARTIFACTS/frames/tmp.png $CIRCLE_ARTIFACTS/frames/${file}.png
    else
	mv $CIRCLE_ARTIFACTS/frames/tmp.png $CIRCLE_ARTIFACTS/frames/${file}.png
    fi
    event=$((event+1))
}

# Center the view
# Open main menu
send_event mousemove 450 475 click 1
# Click 'Actions'
send_event mousemove 450 475 click 1
# Click 'Town'
send_event mousemove 420 600 click 1
# Send 'Berk'
send_event key b
send_event key e
send_event key r
send_event key k
# Click on the first result
send_event mousemove 100 100 click 1
# Click 'view on map'
send_event mousemove 150 280 click 1

# Set the position
# Open main menu, clicking on a somewhat random position on the map
send_event mousemove 482 318 click 1
# Click 'Actions'
send_event mousemove 450 475 click 1
# Click 'Coordinates'
send_event mousemove 850 430 click 1
# Click 'Set as position'
send_event mousemove 180 150 click 1

# Set a destination
# Open main menu
send_event mousemove 450 475 click 1
# Click 'Actions'
send_event mousemove 450 475 click 1
# Click 'Town'
send_event mousemove 420 600 click 1
# Send 'Berk'
send_event key o
send_event key a
send_event key k
send_event key l
# Click on the first result
send_event mousemove 100 100 click 1
# Click 'set as destination'
send_event mousemove 150 150 click 1

# Assemble the gif
convert   -delay 100 -loop 0 $CIRCLE_ARTIFACTS/frames/*.png $CIRCLE_ARTIFACTS/town_search.gif
