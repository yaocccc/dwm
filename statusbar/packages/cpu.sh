#! /bin/bash
# CPU 获取CPU使用率和温度的脚本

source ~/.profile

this=_cpu
s2d_reset="^d^"
color="^c#2D1B46^^b#335566^"
signal=$(echo "^s$this^" | sed 's/_//')

update() {
    cpu_icon="閭"
    cpu_text=$(top -n 1 -b | sed -n '3p' | awk '{printf "%02d%", 100 - $8}')
    temp_text=$(sensors | grep Tctl | awk '{printf "%d°C", $2}')  

    text=" $cpu_icon $cpu_text $temp_text "
    echo $text
    sed -i '/^export '$this'=.*$/d' $DWM/statusbar/temp
    printf "export %s='%s%s%s%s'\n" $this "$color" "$signal" "$text" "$s2d_reset" >> $DWM/statusbar/temp
}

notify() {
    notify-send "閭 CPU tops" "\n$(ps axch -o cmd:15,%cpu --sort=-%cpu | head)\\n\\n(100% per core)" -r 9527
}

click() {
    case "$1" in
        L) notify ;;
        M) st -g 82x25 -c noborder -e btop ;;
        R) st -g 82x25 -c noborder -e btop ;;
        U) ;;
        D) ;;
    esac
}

case "$1" in
    click) click $2 ;;
    notify) notify ;;
    *) update ;;
esac
