#!/bin/sh

if [ $# -lt 1 ]; then
	exit 1
fi

if [ $1 = "start" ]; then
	echo "start"
	start-stop-daemon --start --exec /usr/bin/aesdsocket -- -d
	exit 0
elif [ $1 = "stop" ]; then
	echo "stop"
	start-stop-daemon --stop --exec /usr/bin/aesdsocket --retry 5
	exit 0
fi

echo "command $1 not found"
exit 1
