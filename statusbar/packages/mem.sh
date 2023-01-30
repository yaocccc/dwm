#! /bin/bash
# MEM

source ~/.profile

this=_mem
icon_color="^c#3B001B^^b#6873790x88^"
text_color="^c#3B001B^^b#6873790x99^"
signal=$(echo "^s$this^" | sed 's/_//')

update() {
	mem_icon=""
    mem_total=$(cat /proc/meminfo | grep "MemTotal:"| awk '{print $2}')
    mem_free=$(cat /proc/meminfo | grep "MemFree:"| awk '{print $2}')
    mem_buffers=$(cat /proc/meminfo | grep "Buffers:"| awk '{print $2}')
    mem_cached=$(cat /proc/meminfo | grep -w "Cached:"| awk '{print $2}')
    men_usage_rate=$(((mem_total - mem_free - mem_buffers - mem_cached) * 100 / mem_total))
    mem_text=$(echo $men_usage_rate | awk '{printf "%02d%", $1}')

    icon=" $mem_icon "
    text=" $mem_text "

    sed -i '/^export '$this'=.*$/d' $DWM/statusbar/temp
    printf "export %s='%s%s%s%s%s'\n" $this "$signal" "$icon_color" "$icon" "$text_color" "$text" >> $DWM/statusbar/temp
}

notify() {
    free_result=`free -h`
    text="
可用:\t $(echo "$free_result" | sed -n 2p | awk '{print $7}')
用量:\t $(echo "$free_result" | sed -n 2p | awk '{print $3}')/$(echo "$free_result" | sed -n 2p | awk '{print $2}')
swap:\t $(echo "$free_result" | sed -n 3p | awk '{print $3}')/$(echo "$free_result" | sed -n 3p | awk '{print $2}')
"
    notify-send " Memory" "$text" -r 9527
}

call_btop() {
    pid1=`ps aux | grep 'st -t statusutil' | grep -v grep | awk '{print $2}'`
    pid2=`ps aux | grep 'st -t statusutil_mem' | grep -v grep | awk '{print $2}'`
    mx=`xdotool getmouselocation --shell | grep X= | sed 's/X=//'`
    my=`xdotool getmouselocation --shell | grep Y= | sed 's/Y=//'`
    kill $pid1 && kill $pid2 || st -t statusutil_mem -g 82x25+$((mx - 328))+$((my + 20)) -c FGN -e btop
}

click() {
    case "$1" in
        L) notify ;;
        R) call_btop ;;
    esac
}

case "$1" in
    click) click $2 ;;
    notify) notify ;;
    *) update ;;
esac
