#!/usr/bin/env bash

# This script is part of navit, a navigation system.
# It can be used to make sure that navit is only started
# once. If navit is already running it will be brought to
# the front.

# Set this to a place where a pidfile should be stored.
# Make sure you have write access...
PIDFILE="/var/run/navit/navit.pid"

# Set this to navit's executable.
NAVIT="./navit"

# Optional: Set this to an alternative configuration file
#CONFIG="./navit.xml"

############################################################
### You should not need to edit anything below this line ###
############################################################

function check_wmctrl()
{
	if ! which wmctrl > /dev/null
	then
		echo "I need the 'wmctrl' program. Exit."
		exit 1
	fi
}

function start_navit()
{
	if [ "x" != "x$CONFIG" ] ; then
		$NAVIT -c $CONFIG &
	else
		$NAVIT &
	fi

	pid=$!

	if ! echo -n "$pid" > $PIDFILE
	then
		echo "Started navit with PID $pid."
	else
		kill $pid
		echo "Could not create pidfile!"
		exit 1
	fi

	# Waiting for navit to close...
	wait $pid

	rm $PIDFILE
}

function check_navit()
{
	if [ -f $PIDFILE ] ; then
    pid=$(cat $PIDFILE)
		if ! kill -0 $pid 2>/dev/null
		then
            echo "Bringing Navit to front"

            winid=$(wmctrl -l -p | grep -e "^[^:blank:]*[:blank:]*[^:blank:]*[:blank:]*$pid[:blank:]*" | sed 's/ .*//')
            wmctrl -i -R $winid

			exit 0
		fi
	fi
}


### Start of the main script ###

check_wmctrl

check_navit

start_navit
