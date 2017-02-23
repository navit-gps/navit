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
send_event key KP_Enter # Open main menu
send_event key Down     # Select 'Actions'
send_event key KP_Enter # Validate
send_event key Down     # Scroll to 'Bookmarks'
send_event key Right    # Scroll to 'Former destinations'
send_event key Right    # Select 'Town'
send_event key KP_Enter # Validate
# Send 'Berk'
send_event key b
send_event key e
send_event key r
send_event key k
send_event key Down     # Highlight search area
send_event key Down     # Highlight first result
send_event key KP_Enter # Validate

# Set the position
send_event mousemove 482 318 click 1 # Open main menu, clicking on a somewhat random position on the map
send_event key Down     # Select 'Actions'
send_event key KP_Enter # Validate
send_event key Down     # Scroll to 'Bookmarks'
send_event key Right    # Scroll to 'Former destinations'
send_event key Right    # Select current coordinates
send_event key KP_Enter # Validate

# Set a destination
send_event key KP_Enter # Open main menu
send_event key Down     # Select 'Actions'
send_event key KP_Enter # Validate
send_event key Down     # Scroll to 'Bookmarks'
send_event key Right    # Scroll to 'Former destinations'
send_event key Right    # Select 'Town'
send_event key KP_Enter # Validate
# Send 'oakl'
send_event key o
send_event key a
send_event key k
send_event key l
send_event key Down     # Highlight search area
send_event key Down     # Highlight first result
send_event key KP_Enter # Validate

# Switch to 3d view
send_event key KP_Enter # Open main menu
send_event key Down     # Select 'Actions'
send_event key Right    # Select 'Settings'
send_event key KP_Enter # Validate
send_event key Down     # Select 'Display'
send_event key KP_Enter # Validate
send_event key Down     # Scroll to 'Layout'
send_event key Right    # Scroll to 'Fullscreen'
send_event key Right    # Select '3d'
send_event key KP_Enter # Validate
# Send 'Berk'

# capture 5 seconds of usage
for i in `seq 99994 99999`; do
	import -window root $CIRCLE_ARTIFACTS/frames/${i}.png
	sleep 1
done

# Quit
send_event key KP_Enter # Open main menu
send_event key Down     # Select 'Actions'
send_event key Down     # Select 'Route'
send_event key Right    # Select 'About'
send_event key Right    # Select 'Quit'
send_event key KP_Enter # Validate

# Assemble the gif
convert   -delay 100 -loop 0 $CIRCLE_ARTIFACTS/frames/*.png $CIRCLE_ARTIFACTS/town_search.gif
