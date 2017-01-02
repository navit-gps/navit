sudo apt-get install xdotool

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

# Open main menu
send_event mousemove 350 350 click 1

# Click 'Actions'
send_event mousemove 350 350 click 1

# Click 'Town'
send_event mousemove 300 500 click 1

# Send 'Berk'
send_event key b
send_event key e
send_event key r
send_event key k

# Click on the first result
send_event mousemove 150 100 click 1

# Click 'view on map'
send_event mousemove 150 280 click 1

# Assemble the gif
convert   -delay 100 -loop 0 $CIRCLE_ARTIFACTS/frames/*.png $CIRCLE_ARTIFACTS/town_search.gif
