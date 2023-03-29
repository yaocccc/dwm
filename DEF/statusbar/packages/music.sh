#! /bin/bash
# music 脚本

tempfile=$(cd $(dirname $0);cd ..;pwd)/temp

this=_music
icon_color="^c#3B102B^^b#6873790x88^"
text_color="^c#3B102B^^b#6873790x99^"
signal=$(echo "^s$this^" | sed 's/_//')

[ ! "$(command -v mpc)" ] && echo command not found: mpc && return

update() {
    music_text="$(mpc current)"
    icon=" 󰝚 "
    text=" $music_text "
    [ "$(mpc status | grep "paused")" ] && icon=" 󰐎 "

    sed -i '/^export '$this'=.*$/d' $tempfile
    [ ! "$music_text" ] && return
    printf "export %s='%s%s%s%s%s'\n" $this "$signal" "$icon_color" "$icon" "$text_color" "$text" >> $tempfile
}

notify() {
    notify-send "♫ Now Playing" "$(mpc current)" -r 9527
}

click() {
    case "$1" in
        L) mpc toggle; notify ;;
        R) mpc toggle ;;
        U) mpc prev ;;
        D) mpc next ;;
    esac
}

case "$1" in
    click) click $2 ;;
    notify) notify ;;
    *) update ;;
esac
