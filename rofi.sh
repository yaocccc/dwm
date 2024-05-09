# 打印菜单
call_menu() {
    echo ' set wallpaper'
    echo '艹 update statusbar'
    [ "$(sudo docker ps | grep v2raya)" ] && echo ' close v2raya' || echo ' open v2raya'
    [ "$(ps aux | grep picom | grep -v 'grep\|rofi\|nvim')" ] && echo ' close picom' || echo ' open picom'
}

# 执行菜单
execute_menu() {
    case $1 in
        ' set wallpaper')
            feh --randomize --bg-fill ~/Pictures/wallpaper/*.png
            ;;
        '艹 update statusbar')
            coproc ($DWM/statusbar/statusbar.sh updateall > /dev/null 2>&1)
            ;;
        ' open v2raya')
            coproc (sudo docker restart v2raya > /dev/null && $DWM/statusbar/statusbar.sh updateall > /dev/null)
            ;;
        ' close v2raya')
            coproc (sudo docker stop v2raya > /dev/null && $DWM/statusbar/statusbar.sh updateall > /dev/null)
            ;;
        ' open picom')
            coproc (picom --experimental-backends --config ~/scripts/config/picom.conf > /dev/null 2>&1)
            ;;
        ' close picom')
            killall picom
            ;;
    esac
}

execute_menu "$(call_menu | rofi -dmenu -p "")"
