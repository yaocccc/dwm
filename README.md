# DWM

dwm 是一个非常快速, 小巧并使用动态管理窗口的窗口管理器

## 安装

 sudo make clean install

## 运行 dwm

将你的dwm源代码目录写入 ~/.profile, 例如  

```plaintext
export DWM=~/workspace/dwm
```

将以下行添加到 .xinitrc 中来通过 `startx` 启动 dwm:  

```plaintext
exec dwm
```

## 状态栏

本dwm版本使用了 status2d patch，请参考文档使用 [status2d](http://dwm.suckless.org/patches/status2d/)

```plaintext
  本仓库维护了 statusbar脚本 入口为 ./statusbar/statusbar.sh
  
  模块列表见 ./statusbar/packages
  
  若需要使用 请逐个去查看 修改packages中的脚本文件
  
  请在dwm启动时 调用 $DWM/statusbar/statusbar.sh cron

  注意 ~/.profile中需要有 该环境变量为强依赖关系
  export DWM=~/workspace/dwm
```

## 随DWM启动的自启动命令

dwm启动时会去调用 ~/scripts/autostart.sh 脚本(如果存在的话)

可参考 [autostart脚本](https://github.com/yaocccc/scripts/blob/master/autostart.sh)

## 展示

[BV1Ef4y1Z7kA](https://www.bilibili.com/video/BV1Ef4y1Z7kA/)
