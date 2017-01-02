sudo apt-get install xdotool
# Use xinput test 4 when running x11vnc on the circleci server to find mouse coordinates

[ -d $CIRCLE_ARTIFACTS/frames/ ] || mkdir $CIRCLE_ARTIFACTS/frames/

event=0

send_event (){
    file=`printf "%05d\n" $event`

    import -window root $CIRCLE_ARTIFACTS/frames/tmp.png
    if [[ "$1" == "mousemove" ]]; then
        composite -gravity NorthWest -geometry +$2+$3 ~/navit/ci/pointer-64.png $CIRCLE_ARTIFACTS/frames/tmp.png $CIRCLE_ARTIFACTS/frames/${file}.png
    else
        mv $CIRCLE_ARTIFACTS/frames/tmp.png $CIRCLE_ARTIFACTS/frames/${file}.png
    fi
    event=$((event+1))
    xdotool $@
    sleep 1
}


# Center the view
send_event mousemove 450 475 click 1 # Open main menu
send_event mousemove 450 475 click 1 # Click 'Actions'
send_event mousemove 420 600 click 1 # Click 'Town'
# Send 'Berk'
send_event key b
send_event key e
send_event key r
send_event key k
send_event mousemove 100 100 click 1 # Click on the first result
send_event mousemove 150 280 click 1 # Click 'view on map'

# Set the position
send_event mousemove 482 318 click 1 # Open main menu, clicking on a somewhat random position on the map
send_event mousemove 450 475 click 1 # Click 'Actions'
send_event mousemove 850 430 click 1 # Click 'Coordinates'
send_event mousemove 180 120 click 1 # Click 'Set as position'

# Set a destination
send_event mousemove 450 475 click 1 # Open main menu
send_event mousemove 450 475 click 1 # Click 'Actions'
send_event mousemove 420 600 click 1 # Click 'Town'
# Send 'oakl'
send_event key o
send_event key a
send_event key k
send_event key l
send_event mousemove 100 100 click 1 # Click on the first result
send_event mousemove 150 150 click 1 # Click 'set as destination'

# Switch to 3d view
send_event mousemove 640 450 click 1 # Open main menu
send_event mousemove 640 450 click 1 # Click 'Settings'
send_event mousemove 475 450 click 1 # Click 'Display'
send_event mousemove 870 450 click 1 # Click '3D'

# capture the last image
import -window root $CIRCLE_ARTIFACTS/frames/99999.png

# Assemble the gif
convert   -delay 100 -loop 0 $CIRCLE_ARTIFACTS/frames/*.png $CIRCLE_ARTIFACTS/town_search.gif
