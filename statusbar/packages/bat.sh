#! /bin/bash
# 电池电量
# 需要安装acpi或者upower

tempfile=$(cd $(dirname $0);cd ..;pwd)/temp

this=_bat
icon_color="^c#3B001B^^b#4865660x88^"
text_color="^c#3B001B^^b#4865660x99^"
signal=$(echo "^s$this^" | sed 's/_//')

get_by_acpi() {
    [ ! "$(command -v acpi)" ] && echo command not found: acpi && return
    bat_text=$(acpi -b | sed '2,$d' | awk '{print $4}' | grep -Eo "[0-9]+")
    [ ! "$bat_text" ] && bat_text=$(acpi -b | sed '2,$d' | awk -F'[ %]' '{print $5}' | grep -Eo "[0-9]+")
    [ ! "$(acpi -b | grep 'Battery 0' | grep Discharging)" ] && charging="true"
    _time="可用时间: $(acpi | sed 's/^Battery 0: //g' | awk -F ',' '{print $3}' | sed 's/^[ ]//g' | awk '{print $1}')"
    [ "$_time" = "可用时间: " ] && _time=""
}

get_by_upower() {
    [ ! "$(command -v upower)" ] && echo command not found: upower && return
    bat=$(upower -e | grep BAT)
    bat_text=$(upower -i $bat | awk '/percentage/ {print $2}' | grep -Eo '[0-9]+')
    [ "$(upower -i $bat | grep -w charging)" ] && charging="true"
    [ "$(upower -i $bat | grep 'state:.*fully-charged')" ] && charging=""
}

update() {
    get_by_upower
    get_by_acpi
    [ -z $bat_text ] && bat_text=0
    if [ "$charging" ]; then
        if   [ "$bat_text" -ge 95 ]; then bat_icon="󰂋";
        elif [ "$bat_text" -ge 90 ]; then bat_icon="󰂋";
        elif [ "$bat_text" -ge 80 ]; then bat_icon="󰂊";
        elif [ "$bat_text" -ge 70 ]; then bat_icon="󰢞";
        elif [ "$bat_text" -ge 60 ]; then bat_icon="󰂉";
        elif [ "$bat_text" -ge 50 ]; then bat_icon="󰢝";
        elif [ "$bat_text" -ge 40 ]; then bat_icon="󰂈";
        elif [ "$bat_text" -ge 30 ]; then bat_icon="󰂇";
        elif [ "$bat_text" -ge 20 ]; then bat_icon="󰂆";
        elif [ "$bat_text" -ge 10 ]; then bat_icon="󰢜";
        else bat_icon="󰢟"; fi
    else
        if   [ "$bat_text" -ge 95 ]; then bat_icon="󰂂";
        elif [ "$bat_text" -ge 90 ]; then bat_icon="󰂂";
        elif [ "$bat_text" -ge 80 ]; then bat_icon="󰂁";
        elif [ "$bat_text" -ge 70 ]; then bat_icon="󰂀";
        elif [ "$bat_text" -ge 60 ]; then bat_icon="󰁿";
        elif [ "$bat_text" -ge 50 ]; then bat_icon="󰁾";
        elif [ "$bat_text" -ge 40 ]; then bat_icon="󰁽";
        elif [ "$bat_text" -ge 30 ]; then bat_icon="󰁼";
        elif [ "$bat_text" -ge 20 ]; then bat_icon="󰁻";
        elif [ "$bat_text" -ge 10 ]; then bat_icon="󰁺";
        else bat_icon="󰂎"; fi
    fi

    icon=" $bat_icon "
    text=" $bat_text% "

    sed -i '/^export '$this'=.*$/d' $tempfile
    printf "export %s='%s%s%s%s%s'\n" $this "$signal" "$icon_color" "$icon" "$text_color" "$text" >> $tempfile
}

notify() {
    update
    if [ "$charging" ]; then _time=""; fi
    notify-send "$bat_icon Battery" "\n剩余: $bat_text%\n$_time" -r 9527
}

click() {
    case "$1" in
        L) notify ;;
        R) killall xfce4-power-manager-settings || setsid xfce4-power-manager-settings ;;
    esac
}

case "$1" in
    click) click $2 ;;
    notify) notify ;;
    *) update ;;
esac
