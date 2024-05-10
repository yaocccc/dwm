#!/bin/bash

# 获取当前连接的显示器名称
display=$(xrandr | grep " connected" | awk '{print $1}')

# 获取最大分辨率和最大刷新率
max_resolution=$(xrandr | grep -w "$display" -A1 | tail -n1 | awk '{print $1}')
max_refresh_rate=$(xrandr | grep -w "$display" -A2 | tail -n1 | awk '{print $(NF-1)}')

# 设置最大分辨率和最大刷新率
xrandr --output "$display" --mode "$max_resolution" --rate "$max_refresh_rate"

# 缩放系数 数字越大显示效果越小
scale_factor="0.8"

# 使用 xrandr 命令放大屏幕
xrandr --output "$display" --scale "$scale_factor"x"$scale_factor"

# 设置壁纸
feh --randomize --bg-fill ~/Pictures/wallpaper/*.png
