#! /bin/bash
# ICONS 部分特殊的标记图标 这里是我自己用的，你用不上的话去掉就行

tempfile=$(cd $(dirname $0);cd ..;pwd)/temp

this=_icons
color="^c#2D1B46^^b#5555660x66^"
signal=$(echo "^s$this^" | sed 's/_//')

update() {
    icons=("")
    [ "$(ps aux | grep -v grep | grep 'v2raya')" ] && icons=(${icons[@]} "")
    [ "$(bluetoothctl info 88:C9:E8:14:2A:72 | grep 'Connected: yes')" ] && icons=(${icons[@]} "")
    [ "$AUTOSCREEN" = "OFF" ] && icons=(${icons[@]} "ﴸ")

    text=" ${icons[@]} "

    sed -i '/^export '$this'=.*$/d' $tempfile
    printf "export %s='%s%s%s'\n" $this "$signal" "$color" "$text" >> $tempfile
}

notify() {
    texts=""
    [ "$(ps aux | grep -v grep | grep 'v2raya')" ] && texts="$texts\n v2raya 已启动"
    [ "$(bluetoothctl info 88:C9:E8:14:2A:72 | grep 'Connected: yes')" ] && texts="$texts\n WH-1000XM4 已链接"
    [ "$texts" != "" ] && notify-send " Info" "$texts" -r 9527
}

call_menu() {
    case $(echo -e ' 关机\n 重启\n 休眠\n 锁定' | rofi -dmenu -window-title power -theme ~/scripts/config/rofi.rasi) in
        " 关机") poweroff ;;
        " 重启") reboot ;;
        " 休眠") systemctl hibernate ;;
        " 锁定") ~/scripts/blurlock.sh ;;
    esac
}

click() {
    case "$1" in
        L) notify; feh --randomize --bg-fill ~/Pictures/wallpaper/*.png ;;
        R) call_menu ;;
    esac
}

case "$1" in
    click) click $2 ;;
    notify) notify ;;
    *) update ;;
esac
