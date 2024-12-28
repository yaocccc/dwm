#! /bin/bash
# DATE 获取日期和时间的脚本

tempfile=$(cd $(dirname $0);cd ..;pwd)/temp

this=_date
icon_color="^c#4B005B^^b#7E51680x88^"
text_color="^c#4B005B^^b#7E51680x99^"
signal=$(echo "^s$this^" | sed 's/_//')

update() {
    time_text="$(date '+%m/%d %H:%M')"
    case "$(date '+%I')" in
        "01") time_icon="" ;;
        "02") time_icon="" ;;
        "03") time_icon="" ;;
        "04") time_icon="" ;;
        "05") time_icon="" ;;
        "06") time_icon="" ;;
        "07") time_icon="" ;;
        "08") time_icon="" ;;
        "09") time_icon="" ;;
        "10") time_icon="" ;;
        "11") time_icon="" ;;
        "12") time_icon="" ;;
    esac

    icon=" $time_icon "
    text=" $time_text "

    sed -i '/^export '$this'=.*$/d' $tempfile
    printf "export %s='%s%s%s%s%s'\n" $this "$signal" "$icon_color" "$icon" "$text_color" "$text" >> $tempfile
}

notify() {
    d1="D:$(date '+%Y-%m-%d')"; d2="D:$(date -d '-1 day ago' '+%Y-%m-%d')"; d3="D:$(date -d '-2 day ago' '+%Y-%m-%d')"
    _cal=$(cal --color=always | sed 1,2d | sed 's/..7m/<b><span color="#ff79c6">/;s/..0m/<\/span><\/b>/' )
    _all=$(cat ~/.todo.md | grep "\- \[.\]" | wc -l)
    _alltask=$(cat ~/.todo.md | grep "\- \[ \]" | sed 's/- \[ \] //' | sed 's/[SD]:.*//')
    _today=$(cat ~/.todo.md | grep "\- \[.\]" | grep "$d1" | wc -l)
    _near3day=$(cat ~/.todo.md | grep "\- \[.\]" | grep "$d1\|$d2\|$d3" | wc -l)
    _todaytask=$(cat ~/.todo.md | grep "\- \[ \]" | grep "$d1" | sed 's/- \[ \] /- /' | sed 's/[SD]:.*//')
    t1="<b><span color=\"#54FF9F\">任务:$_all</span></b>"
    t2="<b><span color=\"#FFB90F\">近期:$_near3day</span></b>"
    t3="<b><span color=\"#ff79c6\">今日:$_today</span></b>"
    _todotext="$t1 $t2 $t3"
    [ "$_todaytask" ] && _todaytext="<b><span color=\"#ff79c6\">\n\n$_todaytask</span></b>"
    [ ! "$_todaytask" ] && _todaytext="<b><span color=\"#ffe0c8dd\">\n\n$_alltask</span></b>"
    
    notify-send "  Calendar" "\n$_cal\n\n$_todotext$_todaytext" -r 9527
}

call_todo() {
    pid1=`ps aux | grep 'st -t statusutil' | grep -v grep | awk '{print $2}'`
    pid2=`ps aux | grep 'st -t statusutil_todo' | grep -v grep | awk '{print $2}'`
    mx=`xdotool getmouselocation --shell | grep X= | sed 's/X=//'`
    my=`xdotool getmouselocation --shell | grep Y= | sed 's/Y=//'`
    kill $pid1 && kill $pid2 || st -t statusutil_todo -g 50x15+$((mx - 200))+$((my + 20)) -c FGN -e nvim ~/.todo.md
}

click() {
    case "$1" in
        L) notify ;;
        R) call_todo ;;
    esac
}

case "$1" in
    click) click $2 ;;
    notify) notify ;;
    *) update ;;
esac
