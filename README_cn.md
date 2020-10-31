我的 dwm 构建
============================
[English Ver.](./README.md)

dwm 是一个非常快速, 小巧并使用动态管理窗口的窗口管理器

要求
------------
构建 dwm 前, 你需要有 `Xlib` 头文件
In order to build dwm you need the Xlib header files.


安装
------------
编辑 `config.mk` 来匹配你的本地设置 (dwm 将默认安装在 /usr/local)

之后通过以下命令安装 dwm (必须使用 root 用户):

   sudo make clean install

运行 dwm
-----------
将以下行添加到 .xinitrc 中来通过 `startx` 启动 dwm:

    exec dwm

如果你需要使用多显示器启动 dwm, 你需要设置屏幕变量, 以下是一个例子:

    DISPLAY=foo.bar:1 exec dwm

(这样将会启动 dwm 并显示在显示器一上)

如果你想自定义你的状态栏, 你可以在 .xinitrc 添加行, 以下是一个例子:

    while xsetroot -name "`date` `uptime | sed 's/.*,//'`"
    do
    	sleep 1
    done &
    exec dwm
