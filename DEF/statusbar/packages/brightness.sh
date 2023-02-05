#! /bin/bash 
# 获取亮度

tempfile=$(cd $(dirname $0);cd ..;pwd)/temp

this=_brightness
icon_color="^c#442266^^b#7879560x88^"
text_color="^c#442266^^b#7879560x99^"
signal=$(echo "^s$this^" | sed 's/_//')

m=$(xrandr | grep -v disconnected | grep connected | awk '{print $1}')
brightness=$(xrandr --verbose | grep Brightness | awk '{ print $2 }')

update() {
  brightness=$(xrandr --verbose | grep Brightness | awk '{ print $2 }')
  result=`awk -v x=$(xrandr --verbose | grep Brightness | awk '{ print $2 }') 'BEGIN{printf "%.0f",x*100}'`

  icon=" 󰳲 "
  text=" $result% "

  sed -i '/^export '$this'=.*$/d' $DWM/statusbar/temp
  printf "export %s='%s%s%s%s%s'\n" $this "$signal" "$icon_color" "$icon" "$text_color" "$text" >> $tempfile
}

notify() {
    update
    notify-send -r 9527 -h int:value:$result -h string:hlcolor:#dddddd "$icon Brightness"
}

click() {
  case "$1" in
      L) notify ;; # 仅通知
      U) xrandr --output $m --brightness `awk -v x=$brightness 'BEGIN{printf "%.2f",x+0.01}'` ; notify ;; # 亮度加
      D) xrandr --output $m --brightness `awk -v x=$brightness 'BEGIN{printf "%.2f",x-0.01}'` ; notify ;; # 亮度减
  esac
}


case "$1" in
    click) click $2 ;;
    notify) notify ;;
    *) update ;;
esac
