#!/bin/bash

wget -q https://codeforces.com/profile/$1 -O stats.html
j=$(cat stats.html |grep "(max\.")
if [[ $j == "" ]]
then
	echo 0 0
else
	echo $(echo $j |cut -d ">" -f 2 |cut -d "<" -f 1) $(echo $j |cut -d ">" -f 7 |cut -d "<" -f 1)
fi
rm stats.html
