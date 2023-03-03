#! /bin/bash
# 电池电量

tempfile=$(cd $(dirname $0);cd ..;pwd)/temp

this=_bat
icon_color="^c#3B001B^^b#4865660x88^"
text_color="^c#3B001B^^b#4865660x99^"
signal=$(echo "^s$this^" | sed 's/_//')

# check
[ ! "$(command -v acpi)" ] && echo command not found: acpi && exit

update() {
    # 有人是 /org/freedesktop/UPower/devices/battery_BAT1 也有人最后是 battery_BAT0 , 为了兼容性先这么写
    # 可以自己执行一下然后写死，这个一般不会变动
    bat=$(upower -e | grep BAT)
    bat_text=$(upower -i $bat | awk '/percentage/ {print $2}' | grep -Eo '[0-9]+')
    charge_icon=" "
    # [ -z "$(acpi -a | grep on-line)" ] && charge_icon=" " 也可以
    [ -z "$(upower -i $bat | grep 'state:.*fully-charged')" ] && charge_icon=" "

    if   [ "$bat_text" -ge 95 ]; then bat_icon=""; charge_icon="";
    elif [ "$bat_text" -ge 90 ]; then bat_icon="";
    elif [ "$bat_text" -ge 80 ]; then bat_icon="";
    elif [ "$bat_text" -ge 70 ]; then bat_icon="";
    elif [ "$bat_text" -ge 60 ]; then bat_icon="";
    elif [ "$bat_text" -ge 50 ]; then bat_icon="";
    elif [ "$bat_text" -ge 40 ]; then bat_icon="";
    elif [ "$bat_text" -ge 30 ]; then bat_icon="";
    elif [ "$bat_text" -ge 20 ]; then bat_icon="";
    elif [ "$bat_text" -ge 10 ]; then bat_icon="";
    else bat_icon=""; fi

    icon=" $charge_icon$bat_icon "
    text=" $bat_text% "

    sed -i '/^export '$this'=.*$/d' $tempfile
    printf "export %s='%s%s%s%s%s'\n" $this "$signal" "$icon_color" "$icon" "$text_color" "$text" >> $tempfile
}

notify() {
    update
    _status="状态: $(acpi | sed 's/^Battery 0: //g' | awk -F ',' '{print $1}')"
    _remaining="剩余: $(acpi | sed 's/^Battery 0: //g' | awk -F ',' '{print $2}' | sed 's/^[ ]//g')"
    _time="可用时间: $(acpi | sed 's/^Battery 0: //g' | awk -F ',' '{print $3}' | sed 's/^[ ]//g' | awk '{print $1}')"
    [ "$_time" = "可用时间: " ] && _time=""
    notify-send "$bat_icon Battery" "\n$_status\n$_remaining\n$_time" -r 9527
}

click() {
    case "$1" in
        L) notify ;;
        R) killall xfce4-power-manager-settings || xfce4-power-manager-settings & ;;
    esac
}

case "$1" in
    click) click $2 ;;
    notify) notify ;;
    *) update ;;
esac
