#! /bin/bash
# music 脚本

tempfile=$(cd $(dirname $0);cd ..;pwd)/temp

this=_music
icon_color="^c#3B102B^^b#6873790x88^"
text_color="^c#3B102B^^b#6873790x99^"
signal=$(echo "^s$this^" | sed 's/_//')

[ ! "$(command -v playerctl)" ] && echo command not found: mpc && return

update() {
    music_text="$(playerctl metadata title)"
    icon=" 󰝚 "
    if $music_text=~"\""; then
        text=$(echo $music_text | sed -e "s/\"\\\\\"/g")
    else
        text=" $music_text "
    fi
    [ "$(playerctl status | grep "Paused")" ] && icon=" 󰐎 "

    sed -i '/^export '$this'=.*$/d' $tempfile
    [ ! "$music_text" ] && return
    printf "export %s=\"%s%s%s%s%s\"\n" $this "$signal" "$icon_color" "$icon" "$text_color" "$text" >> $tempfile
}

click() {
    case "$1" in
        L) playerctl play-pause ;;
        R) playerctl play-pause ;;
        U) playerctl previous ;;
        D) playerctl next ;;
    esac
}

case "$1" in
    click) click $2 ;;
    notify) notify ;;
    *) update ;;
esac
