#! /bin/bash

# 设置某个模块的状态 update cpu mem ...
update() {
    [ ! "$1" ] && return
    bash $DWM/statusbar/packages/$1.sh
    shift 1
    update $@
}

# 更新状态栏
refresh() {
    source $DWM/statusbar/temp
    xsetroot -name "$_icons$_coin$_cpu$_mem$_date$_vol$_bat"
}

# 启动定时更新状态栏 不用的package有不同的刷新周期 注意不要重复启动该func
cron() {
    while true; do update cpu mem; sleep 20; done &        # 每隔20s更新cpu 内存
    while true; do update vol icons coin; sleep 60; done & # 每隔60s更新音量 图标 币价
    while true; do update bat; sleep 300; done &           # 每隔300s更新电池
    while true; do update date; refresh; sleep 5; done &   # 每隔5s更新时间并更新状态栏
}

case $1 in
    cron) cron ;;
    refresh) refresh ;;
    update) shift 1; update $*; refresh ;;
    updateall) update icons coin cpu mem date vol bat; refresh ;;
esac
