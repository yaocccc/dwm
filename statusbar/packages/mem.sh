#! /bin/bash
# MEM

source ~/.profile

this=_mem
s2d_reset="^d^"
color="^c#442266^^b#334466^"
signal=$(echo "^s$this^" | sed 's/_//')

update() {
    mem_total=$(cat /proc/meminfo | grep "MemTotal:"|awk '{print $2}')
    mem_free=$(cat /proc/meminfo | grep "MemFree:"|awk '{print $2}')
    mem_buffers=$(cat /proc/meminfo | grep "Buffers:"|awk '{print $2}')
    mem_cached=$(cat /proc/meminfo | grep -w "Cached:"|awk '{print $2}')
    men_usage_rate=$(((mem_total - mem_free - mem_buffers - mem_cached) * 100 / mem_total))
	mem_icon=""
    mem_text=$(echo $men_usage_rate | awk '{printf "%02d%", $1}')
    text=" $mem_icon $mem_text "
    echo $text
    sed -i '/^export '$this'=.*$/d' $DWM/statusbar/temp
    printf "export %s='%s%s%s%s'\n" $this "$color" "$signal" "$text" "$s2d_reset" >> $DWM/statusbar/temp
}

click() {
    case "$1" in
        L) ;;
        M) st -g 82x25 -c noborder -e btop ;;
        R) st -g 82x25 -c noborder -e btop ;;
    esac
}

case "$1" in
    click) click $2 ;;
    *) update ;;
esac
