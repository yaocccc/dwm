#! /bin/bash

source ~/.profile

this=_wifi
s2d_reset="^d^"
color="^c#442266^^b#385056^"
signal=$(echo "^s$this^" | sed 's/_//')

update() {
    wifi_text=$(nmcli | grep 已连接 | awk '{print $3}')
    wifi_icon="直"
    [ "$wifi_text" = "" ] && wifi_icon="睊" && wifi_text="未连接"
    text=" $wifi_icon $wifi_text "
    echo $text
    sed -i '/^export '$this'=.*$/d' $DWM/statusbar/temp
    printf "export %s='%s%s%s%s'\n" $this "$color" "$signal" "$text" "$s2d_reset" >> $DWM/statusbar/temp
}

notify() {
    update
    connect=$(nmcli | grep 已连接 | awk '{print $3}')
    device=$(nmcli | grep 已连接 | awk '{print $1}'  | sed 's/：已连接//')
    text="设备: $device\n连接: $connect"
    [ "$connect" = "" ] && text="未连接到网络"
    notify-send -r 9527 "$wifi_icon Wifi" "\n$text"
}

call_nm() {
    pid1=`ps aux | grep 'st -t statusutil' | grep -v grep | awk '{print $2}'`
    pid2=`ps aux | grep 'st -t statusutil_nm' | grep -v grep | awk '{print $2}'`
    mx=`xdotool getmouselocation --shell | grep X= | sed 's/X=//'`
    my=`xdotool getmouselocation --shell | grep Y= | sed 's/Y=//'`
    kill $pid1 && kill $pid2 || st -t statusutil_nm -g 60x25+$((mx - 240))+$((my + 20)) -c noborder -e 'nmtui-connect'
}

click() {
    case "$1" in
        L) notify ;;
        R) call_nm ;;
    esac
}

case "$1" in
    click) click $2 ;;
    notify) notify ;;
    *) update ;;
esac
