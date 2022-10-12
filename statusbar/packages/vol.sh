#! /bin/bash
# VOL 音量

source ~/.profile

this=_vol
s2d_reset="^d^"
color="^c#553388^^b#334466^"

main() {
    OUTPORT=$SPEAKER
    [ "$(pactl list sinks | grep $HEADPHONE_HSP_HFP)" ] && OUTPORT=$HEADPHONE_HSP_HFP
    [ "$(pactl list sinks | grep $HEADPHONE_HSP_HFP_SONY)" ] && OUTPORT=$HEADPHONE_HSP_HFP_SONY
    volunmuted=$(pactl list sinks | grep $OUTPORT -A 10 | grep 'Mute: no')
    vol_text=$(pactl list sinks | grep $OUTPORT -A 8 | sed -n '8p' | awk '{printf int($5)}')
    if [ "$vol_text" -eq 0 ] || [ ! "$volunmuted" ]; then vol_text="--"; vol_icon="婢";
    elif [ "$vol_text" -lt 10 ]; then vol_icon="奄"; vol_text=0$vol_text;
    elif [ "$vol_text" -le 20 ]; then vol_icon="奄";
    elif [ "$vol_text" -le 60 ]; then vol_icon="奔";
    else vol_icon="墳"; fi

    vol_text=$vol_text%

    text=" $vol_icon $vol_text "
    sed -i '/^export '$this'=.*$/d' $DWM/statusbar/temp
    printf "export %s='%s%s%s'\n" $this "$color" "$text" "$s2d_reset" >> $DWM/statusbar/temp
}

main
