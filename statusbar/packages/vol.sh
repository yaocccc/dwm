#! /bin/bash
# VOL 音量脚本
# 本脚本需要你自行修改音量获取命令
# 例如我使用的是 pipewire
#
# $ pactl lisk sinks | grep RUNNING -A 8
#         State: RUNNING
#         Name: bluez_output.88_C9_E8_14_2A_72.1
#         Description: WH-1000XM4
#         Driver: PipeWire
#         Sample Specification: float32le 2ch 48000Hz
#         Channel Map: front-left,front-right
#         Owner Module: 4294967295
# 静音 -> Mute: no                                                                                 
# 音量 -> Volume: front-left: 13183 /  20% / -41.79 dB,   front-right: 13183 /  20% / -41.79 dB

source ~/.profile

this=_vol
s2d_reset="^d^"
color="^c#553388^^b#334466^"
signal=$(echo "^s$this^" | sed 's/_//')

update() {
    sink=$(pactl info | grep 'Default Sink' | awk '{print $3}')
    volunmuted=$(pactl list sinks | grep $sink -A 6 | sed -n '7p' | grep 'Mute: no')
    vol_text=$(pactl list sinks | grep $sink -A 7 | sed -n '8p' | awk '{printf int($5)}')
    if [ "$vol_text" -eq 0 ] || [ ! "$volunmuted" ]; then vol_text="--"; vol_icon="婢";
    elif [ "$vol_text" -lt 10 ]; then vol_icon="奄"; vol_text=0$vol_text;
    elif [ "$vol_text" -le 20 ]; then vol_icon="奄";
    elif [ "$vol_text" -le 60 ]; then vol_icon="奔";
    else vol_icon="墳"; fi

    vol_text=$vol_text%

    text=" $vol_icon $vol_text "
    sed -i '/^export '$this'=.*$/d' $DWM/statusbar/temp
    printf "export %s='%s%s%s%s'\n" $this "$color" "$signal" "$text" "$s2d_reset" >> $DWM/statusbar/temp
}

click() {
    case "$1" in
        L) pavucontrol & ;;                             # 打开pavucontrol
        M) pactl set-sink-mute @DEFAULT_SINK@ toggle ;; # 切换静音
        R) pactl set-sink-mute @DEFAULT_SINK@ toggle ;; # 切换静音
        U) pactl set-sink-volume @DEFAULT_SINK@ +5%  ;; # 音量加
        D) pactl set-sink-volume @DEFAULT_SINK@ -5%  ;; # 音量减
    esac
}

case "$1" in
    click) click $2 ;;
    *) update ;;
esac
