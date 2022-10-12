#! /bin/bash
# CPU

source ~/.profile

this=_cpu
s2d_reset="^d^"
color="^c#2D1B46^^b#335566^"

main() {
    cpu_icon="閭"
    cpu_text=$(top -n 1 -b | sed -n '3p' | awk '{printf "%02d%", 100 - $8}')
    temp_text=$(sensors | grep Tctl | awk '{printf "%d°C", $2}')  

    text=" $cpu_icon $cpu_text $temp_text "
    sed -i '/^export '$this'=.*$/d' $DWM/statusbar/temp
    printf "export %s='%s%s%s'\n" $this "$color" "$text" "$s2d_reset" >> $DWM/statusbar/temp
}

main
