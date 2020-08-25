#!/bin/bash

key=$(date +%s)

#Brasil
#WCCODE=$(echo "b37affa47bb6c35ea1b6a51db59113d8")

#AmericaLatina
WCCODE=$(echo "a2fb64b9227f71cbdf7c5644904974bf")

BOCASERVER=$(echo "global.naquadah.com.br")

#Online
#wget -4 --no-check-certificate "https://$BOCASERVER/boca/admin/report/webcast.php?webcastcode=$WCCODE" -O log/webcast_$key.zip >/dev/null 2>/dev/null

#Offline
cp webcast_sample/webcast_1573336220.zip log/webcast_$key.zip

cp log/webcast_$key.zip /tmp/webcast.zip
unzip -o /tmp/webcast.zip -d /tmp/ > /dev/null 2> /dev/null

