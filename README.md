# DWM YES

dwm 是一个非常快速, 小巧并使用动态管理窗口的窗口管理器

[展示视频: BV1Ef4y1Z7kA](https://www.bilibili.com/video/BV1Ef4y1Z7kA/)

## 功能

- 支持布局 tile(磁块)、magicgrid(进阶的网格布局)
- 键盘移动/调整窗口大小 且移动/调整时有窗口间吸附效果
- 窗口隐藏
- 窗口可自定义是否全局(在所有tag内展示)
- 更好的浮动窗口支持
- 优化后的status2d 状态栏，可用鼠标点击操作
- 系统托盘支持
- overview
- mod + tab, 在窗口间切换 有浮动窗口时仅在浮动窗口切换
- mod + [tag], 切换tag到指定目录时 可指定一个cmd，若目标tag无窗口 则执行该tag

## 安装

```plaintext
  每次修改源代码后都需要执行
  sudo make clean install
```

## 运行 dwm

将你的dwm源代码目录写入 ~/.profile, 例如  

```plaintext
export DWM=~/workspace/dwm
```

将以下行添加到 .xinitrc 中来通过 `startx` 启动 dwm:  

```plaintext
exec dwm
```

### Nix Flake

```sh
nix run github:yaocccc/dwm
```

## 状态栏

请将每一个块置为下列样式, 可直接使用本仓库statusbar相关脚本 或参考使用

```plaintext
  ^sdate^^c#2D1B46^^b#335566^  11/04 00:42 ^d^

  ^s?????^ => 点击时的信号值
  ^c?????^ => 前景色
  ^b?????^ => 背景色
  ^d^      => 重置颜色

  也可以直接从^c ^b 定义前景色 后景色透明度
  ^c#??????0xff^ => 0xff 前景色透明度
  ^b#??????0x11^ => 0x11 后景色透明度

  本仓库维护了 statusbar脚本 入口为 ./statusbar/statusbar.sh
  
  模块列表见 ./statusbar/packages
  
  若需要使用 请逐个去查看 修改packages中的脚本文件
  
  请在dwm启动时 调用 $DWM/statusbar/statusbar.sh cron

  注意 ~/.profile中需要有 该环境变量为强依赖关系
  export DWM=~/workspace/dwm

  点击事件发生时 会调用 $DWM/statusbar/statusbar.sh 并传入信号值 请自行处理
  例如 $DWM/statusbar/statusbar.sh date L  # 其中date为信号值 L为按键 (L左键 M中键 R右键)

  可执行 $DWM/statusbar/statusbar.sh check 检查是否有模块存在问题
```

## 随DWM启动的自启动命令

dwm启动时会去调用 $DWM/autostart.sh 脚本

可参考 [autostart脚本](https://github.com/yaocccc/dwm/blob/master/autostart.sh)

## Q & A

1. 如何启动dwm？

确保 ~/.xinitrc 中有 exec dwm。在tty中使用 startx 命令启动

2. 进入后是黑屏啥都没

壁纸需要用类似feh的软件设置 `feh --randomize --bg-fill ~/pictures/*.png`

3. 打不开终端

务必先修改config.h中启动终端的快捷键，本项目的config.h是我自用的配置 你需要手动修改

例如 可以修改以下部分(如果你用的终端是st的话) 

```plaintext
    /* spawn + SHCMD 执行对应命令 */
    { MODKEY,              XK_Return,       spawn,            SHCMD("st") },
```

4. 字体显示不全

请自行安装字体 仅以archlinux举例

```shell
yay -S nerd-fonts-jetbrains-mono
yay -S ttf-material-design-icons
yay -S ttf-joypixels
yay -S wqy-microhei
```

## 贡献者 THX

- [yaocccc](https://github.com/yaocccc)
- [p3psi-boo](https://github.com/p3psi-boo)
  - [PR#4 添加 Nix Flake 支持](https://github.com/yaocccc/dwm/pull/4)
- [gxt-kt](https://github.com/gxt-kt)
  - [PR#7 修复hide/show窗口栈索引带来的无法恢复窗口的bug](https://github.com/yaocccc/dwm/pull/7)

## ENJOY IT 😃
