#! /bin/bash 
# 获取亮度

tempfile=$(cd $(dirname $0);cd ..;pwd)/temp

this=_brightness
icon_color="^c#442266^^b#7879560x88^"
text_color="^c#442266^^b#7879560x99^"
signal=$(echo "^s$this^" | sed 's/_//')

monitor=$(xrandr | grep -v disconnected | grep connected | awk '{print $1}')
brightness=`awk -v x=$(xrandr --verbose | grep Brightness | awk '{ print $2 }') 'BEGIN{printf "%d",x*100}'`

update() {
  brightness=`awk -v x=$(xrandr --verbose | grep Brightness | awk '{ print $2 }') 'BEGIN{printf "%d",x*100}'`

  icon=" 󰳲 "
  text=" $brightness% "

  sed -i '/^export '$this'=.*$/d' $DWM/statusbar/temp
  printf "export %s='%s%s%s%s%s'\n" $this "$signal" "$icon_color" "$icon" "$text_color" "$text" >> $tempfile
}

notify() {
    update
    notify-send -r 9527 -h int:value:$brightness -h string:hlcolor:#dddddd "$icon $monitor Brightness"
}

adjust() {
  step=1
  if [ $brightness -gt 100 ]; then
    step=10
  fi

  // only UP
  if [ $brightness = 100 ] && [ "$1" = UP ]; then
    step=10
  fi

  case "$1" in
    UP)   xrandr --output $monitor --brightness `awk -v x=$brightness -v y=$step 'BEGIN{printf "%.2f",(x+y)/100}'` ;;
    DOWN) xrandr --output $monitor --brightness `awk -v x=$brightness -v y=$step 'BEGIN{printf "%.2f",(x-y)/100}'` ;;
  esac
}

click() {
  case "$1" in
      L) notify                ;; # 仅通知
      U) adjust UP    ; notify ;; # 亮度加
      D) adjust DOWN  ; notify ;; # 亮度减
  esac
}

case "$1" in
    click) click $2 ;;
    notify) notify ;;
    *) update ;;
esac
