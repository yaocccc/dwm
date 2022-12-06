#! /bin/bash

touch $DWM/statusbar/temp

# 设置某个模块的状态 update cpu mem ...
update() {
    [ ! "$1" ] && return                                                 # 当指定模块为空时 结束
    bash $DWM/statusbar/packages/$1.sh                                   # 执行指定模块脚本
    shift 1                                                              # 从参数列表中删除第一个参数
    update $*                                                            # 递归调用
}

# 处理状态栏点击
click() {
    [ ! "$1" ] && return                                                 # 未传递参数时 结束
    bash $DWM/statusbar/packages/$1.sh click $2                          # 执行指定模块脚本
    update $1                                                            # 更新指定模块
    refresh                                                              # 刷新状态栏
}

# 更新状态栏
refresh() {
    _icons=''; _cpu=''; _mem=''; _date=''; _vol=''; _bat='';             # 重置所有模块的状态为空
    source $DWM/statusbar/temp                                           # 从 temp 文件中读取模块的状态
    xsetroot -name "$_icons$_cpu$_mem$_date$_vol$_bat"                   # 更新状态栏
}

# 启动定时更新状态栏 不用的package有不同的刷新周期 注意不要重复启动该func
cron() {
    while true; do update cpu mem;        sleep 20;  done &              # 每隔20s更新cpu 内存
    while true; do update vol icons;      sleep 60;  done &              # 每隔60s更新音量 图标
    while true; do update bat;            sleep 300; done &              # 每隔300s更新电池
    while true; do update date; refresh;  sleep 5;   done &              # 每隔5s更新时间并更新状态栏
}

case $1 in
    cron) cron ;;
    refresh) refresh ;;
    update) shift 1; update $*; refresh ;;
    updateall) update icons cpu mem date vol bat; refresh ;;
    *) click $1 $2 ;; # 接收clickstatusbar传递过来的信号 $1: 模块名  $2: 按键(L|M|R)
esac
