#! /bin/bash

dwmdir=$(cd $(dirname $0); cd .. ;pwd)

sink=$(pactl info | grep 'Default Sink' | awk '{print $3}')
vol=$(pactl list sinks | grep $sink -A 7 | sed -n '8p' | awk '{printf int($5)}')
mod=$((vol % 5))

case $1 in
    up) target="+$((5 - mod))%" ;;
    down) [ $mod -eq 0 ] && target="-5%" || target="-$mod%" ;;
esac

pactl set-sink-volume @DEFAULT_SINK@ $target
bash $dwmdir/statusbar/statusbar.sh update vol
bash $dwmdir/statusbar/packages/vol.sh notify 
