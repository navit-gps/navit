sudo apt-get install xdotool
# Use xinput test 4 when running x11vnc on the circleci server to find mouse coordinates

[ -d $CIRCLE_ARTIFACTS/logs_${1}/frames/ ] || mkdir $CIRCLE_ARTIFACTS/logs_${1}/frames/

event=0

send_event (){
    file=`printf "%05d\n" $event`

    import -window root $CIRCLE_ARTIFACTS/logs_${1}/frames/tmp.png
    if [[ "$2" == "mousemove" ]]; then
        composite -gravity NorthWest -geometry +$3+$4 ~/navit/ci/pointer-64.png $CIRCLE_ARTIFACTS/logs_${1}/frames/tmp.png $CIRCLE_ARTIFACTS/logs_${1}/frames/${file}.png
    else
        mv $CIRCLE_ARTIFACTS/logs_${1}/frames/tmp.png $CIRCLE_ARTIFACTS/logs_${1}/frames/${file}.png
    fi
    event=$((event+1))
    xdotool $@
    sleep 1
}


# Center the view
send_event ${1} key KP_Enter # Open main menu
send_event ${1} key Down     # Select 'Actions'
send_event ${1} key KP_Enter # Validate
send_event ${1} key Down     # Scroll to 'Bookmarks'
send_event ${1} key Right    # Scroll to 'Former destinations'
send_event ${1} key Right    # Select 'Town'
send_event ${1} key KP_Enter # Validate
# Send 'Berk'
send_event ${1} key b
send_event ${1} key e
send_event ${1} key r
send_event ${1} key k
send_event ${1} key Down     # Highlight search area
send_event ${1} key Down     # Highlight first result
send_event ${1} key KP_Enter # Validate

# Set the position
send_event ${1} mousemove 482 318 click 1 # Open main menu, clicking on a somewhat random position on the map
send_event ${1} key Down     # Select 'Actions'
send_event ${1} key KP_Enter # Validate
send_event ${1} key Down     # Scroll to 'Bookmarks'
send_event ${1} key Right    # Scroll to 'Former destinations'
send_event ${1} key Right    # Select current coordinates
send_event ${1} key KP_Enter # Validate

# Set a destination
send_event ${1} key KP_Enter # Open main menu
send_event ${1} key Down     # Select 'Actions'
send_event ${1} key KP_Enter # Validate
send_event ${1} key Down     # Scroll to 'Bookmarks'
send_event ${1} key Right    # Scroll to 'Former destinations'
send_event ${1} key Right    # Select 'Town'
send_event ${1} key KP_Enter # Validate
# Send 'oakl'
send_event ${1} key o
send_event ${1} key a
send_event ${1} key k
send_event ${1} key l
send_event ${1} key Down     # Highlight search area
send_event ${1} key Down     # Highlight first result
send_event ${1} key KP_Enter # Validate

# Switch to 3d view
send_event ${1} key KP_Enter # Open main menu
send_event ${1} key Down     # Select 'Actions'
send_event ${1} key Right    # Select 'Settings'
send_event ${1} key KP_Enter # Validate
send_event ${1} key Down     # Select 'Display'
send_event ${1} key KP_Enter # Validate
send_event ${1} key Down     # Scroll to 'Layout'
send_event ${1} key Right    # Scroll to 'Fullscreen'
send_event ${1} key Right    # Select '3d'
send_event ${1} key KP_Enter # Validate
# Send 'Berk'

# capture 5 seconds of usage
for i in `seq 99994 99999`; do
	import -window root $CIRCLE_ARTIFACTS/logs_${1}/frames/${i}.png
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
convert   -delay 100 -loop 0 $CIRCLE_ARTIFACTS/logs_${1}/frames/*.png $CIRCLE_ARTIFACTS/logs_${1}/town_search.gif
