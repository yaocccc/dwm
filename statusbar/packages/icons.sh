#! /bin/bash
# ICONS 部分特殊的标记图标 这里是我自己用的，你用不上的话去掉就行

source ~/.profile

this=_icons
s2d_reset="^d^"
color="^c#223344^^b#4E5173^"
signal=$(echo "^s$this^" | sed 's/_//')

update() {
    icons=()
    [ "$(ps -aux | grep 'aria2' | sed 1d)" ] && icons=(${icons[@]} "")
    [ "$(sudo docker ps | grep 'v2raya')" ] && icons=(${icons[@]} "")
    [ "$(bluetoothctl info 64:03:7F:7C:81:15 | grep 'Connected: yes')" ] && icons=(${icons[@]} "")
    [ "$(bluetoothctl info 8C:DE:F9:E6:E5:6B | grep 'Connected: yes')" ] && icons=(${icons[@]} "")
    [ "$(bluetoothctl info 88:C9:E8:14:2A:72 | grep 'Connected: yes')" ] && icons=(${icons[@]} "")
    [ "$AUTOSCREEN" = "OFF" ] && icons=(${icons[@]} "ﴸ")

    sed -i '/^export '$this'=.*$/d' $DWM/statusbar/temp
    if [ "$icons" ]; then
        text=" ${icons[@]} "
        printf "export %s='%s%s%s%s'\n" $this "$color" "$signal" "$text" "$s2d_reset" >> $DWM/statusbar/temp
    fi
}

click() {
}

case "$1" in
    click) click $2 ;;
    *) update ;;
esac
