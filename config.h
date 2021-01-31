#include <X11/XF86keysym.h>

static const unsigned int borderpx       = 2;         /* 窗口边框大小 */
static const unsigned int systraypinning = 0;         /* 托盘跟随的显示器 0代表不指定显示器 */
static const int systraypinningfailfirst = 1;         /* 托盘跟随的显示器 0代表上个聚焦的显示器 1代表当前聚焦的显示器 */
static const unsigned int systrayspacing = 2;         /* 托盘间距 */
static int showsystray                   = 1;         /* 是否显示托盘栏 */
static const unsigned int gappih         = 14;        /* 垂直方向 窗口与窗口 缝隙大小 */
static const unsigned int gappiv         = 14;        /* 水平方向 窗口与窗口 缝隙大小 */
static const unsigned int gappoh         = 14;        /* 垂直方向 窗口与边缘 缝隙大小 */
static const unsigned int gappov         = 14;        /* 水平方向 窗口与边缘 缝隙大小 */
static const int smartgaps               = 0;         /* 设置为1时 单窗口无缝隙 */
static const int showbar                 = 1;         /* 是否显示状态栏 */
static const int topbar                  = 1;         /* 指定状态栏位置 0底部 1顶部 */
static const float mfact                 = 0.5;       /* 主工作区 大小比例 */
static const int   dynamicmfact          = 1;         /* 设置为1时 当栈窗口数量大于主窗口数量时 动态分配宽度 --= */
static const int   nmaster               = 1;         /* 主工作区 窗口数量 */
static const unsigned int snap           = 32;        /* */
static const int   resizehints           = 1;         /* */
static const char *fonts[]               = { "JetBrainsMono Nerd Font Mono:size=12", "Symbola:size=12" };
static const char col_gray1[]            = "#222222";
static const char col_gray2[]            = "#444444";
static const char col_gray3[]            = "#bbbbbb";
static const char col_gray4[]            = "#ffffff";
static const char col_cyan[]             = "#37474F";
static const char col_border[]           = "#42A5F5";
static const unsigned int baralpha       = 0xc0;      /* 状态栏透明度 */
static const unsigned int borderalpha    = 0xdd;      /* 边框透明度 */
static const char *colors[][3]           = { [SchemeNorm] = { col_gray3, col_gray1, col_gray2  }, [SchemeSel]  = { col_gray4, col_cyan,  col_border }, [SchemeHid]  = { col_cyan,  col_gray1, col_border } };
static const unsigned int alphas[][3]    = { [SchemeNorm] = { OPAQUE, baralpha, borderalpha }, [SchemeSel]  = { OPAQUE, baralpha, borderalpha } };

/* 自定义tag名称 */
/* 自定义特定实例的显示状态 */
static const char *tags[] = { "一", "二", "三", "四", "五", "六", "七", "八", "九", "C", "M", "P", "Q", "W", "L" };
static const Rule rules[] = {
    /* class                    instance    title             tags mask     isfloating   monitor */
    { NULL,                     NULL,      "broken",          0,            1,           -1 },
    { NULL,                     NULL,      "图片查看",        0,            1,           -1 },
    {"Google-chrome",           NULL,       NULL,             1 << 9,       0,           -1 },
    {"netease-cloud-music",     NULL,       NULL,             1 << 10,      1,           -1 },
    {"Postman",                 NULL,       NULL,             1 << 11,      0,           -1 },
    { NULL,                     NULL,      "TIM",             1 << 12,      0,           -1 },
    { NULL,                     NULL,      "MySQL Workbench", 1 << 10,      0,           -1 },
};
/* 自定义布局 */
static const Layout layouts[] = {
    { "♥",   tile },    /* 平铺 */
    { "[M]", monocle }, /* 单窗口 */
    { "><>", NULL },
};

#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }
#define MODKEY Mod4Mask
#define TAGKEYS(KEY, TAG, cmd1, cmd2) \
    { MODKEY,              KEY, view,       {.ui = 1 << TAG, .v = cmd1} }, \
    { MODKEY|ShiftMask,    KEY, tag,        {.ui = 1 << TAG, .v = cmd2} }, \
    { MODKEY|ControlMask,  KEY, toggleview, {.ui = 1 << TAG} }, \

static const char *scratchpadcmd[] = { "st", "-t", "scratchpad", "-g", "120x40", NULL }; // 临时小窗口的启动命令

static Key keys[] = {
    /* modifier            key              function          argument */
    { MODKEY,              XK_minus,        togglescratch,    {.v = scratchpadcmd } },   /* super -            |  打开临时小窗 st */
    { MODKEY,              XK_equal,        togglesystray,    {0} },                     /* super +            |  切换 托盘栏显示状态 */
    
    { MODKEY,              XK_Tab,          focusstack,       {.i = +1 } },              /* super tab          |  本tag内切换聚焦窗口 */
	{ MODKEY|ShiftMask,    XK_Tab,          rotatestack,      {.i = +1 } },              /* super shift tab    |  同上 并将新聚焦的窗口置为主窗口 */
    
    { MODKEY,              XK_Left,         viewtoleft,       {0} },                     /* super left         |  聚焦到左边的tag */
    { MODKEY,              XK_Right,        viewtoright,      {0} },                     /* super right        |  聚焦到右边的tag */
	{ MODKEY|ShiftMask,    XK_Left,         tagtoleft,        {0} },                     /* super shift left   |  将本窗口移动到左边tag */
	{ MODKEY|ShiftMask,    XK_Right,        tagtoright,       {0} },                     /* super shift right  |  将本窗口移动到右边tag */

    { MODKEY,              XK_comma,        setmfact,         {.f = -0.05} },            /* super ,            |  缩小主工作区 */
    { MODKEY,              XK_period,       setmfact,         {.f = +0.05} },            /* super .            |  放大主工作区 */
    { MODKEY|ControlMask,  XK_comma,        setmfact,         {.f = -0.05} },            /* super ctrl ,       |  缩小主工作区 */
    { MODKEY|ControlMask,  XK_period,       setmfact,         {.f = +0.05} },            /* super ctrl .       |  放大主工作区 */
    { MODKEY|ControlMask,  XK_minus,        setgaps,          {.i = +7} },               /* super ctrl -       |  放大间隙 */
    { MODKEY|ControlMask,  XK_equal,        setgaps,          {.i = -7} },               /* super ctrl +       |  缩小间隙 */
    { MODKEY|ControlMask,  XK_Left,         setvgaps,         {.i = +7} },               /* super ctrl left    |  放大水平间隙 */
    { MODKEY|ControlMask,  XK_Right,        setvgaps,         {.i = -7} },               /* super ctrl right   |  缩小水平间隙 */
    { MODKEY|ControlMask,  XK_Down,         sethgaps,         {.i = +7} },               /* super ctrl down    |  放大垂直间隙 */
    { MODKEY|ControlMask,  XK_Up,           sethgaps,         {.i = -7} },               /* super ctrl up      |  缩小垂直间隙 */

    { MODKEY,              XK_h,            hidewin,          {0} },                     /* super h            |  隐藏 窗口 */
    { MODKEY|ShiftMask,    XK_h,            restorewin,       {0} },                     /* super shift h      |  取消隐藏 窗口 */
    { MODKEY,              XK_o,            hideotherwins,    {0} },                     /* super o            |  隐藏全部其他窗口(o: only) */
    { MODKEY|ShiftMask,    XK_o,            restoreotherwins, {0} },                     /* super shift o      |  取消隐藏 其他窗口 */
    
    { MODKEY|ShiftMask,    XK_Return,       zoom,             {0} },                     /* super shift enter  |  将当前聚焦窗口置为主窗口 */
    { MODKEY,              XK_t,            togglefloating,   {0} },                     /* super t            |  开启/关闭 聚焦目标的float模式 */
	{ MODKEY,              XK_f,            fullscreen,       {0} },                     /* super f            |  开启/关闭 全屏 */
    { MODKEY,              XK_space,        setlayout,        {0} },                     /* super space        |  在 monocle(单窗口) tile(平铺) 模式中切换 */
    { MODKEY,              XK_e,            incnmaster,       {.i = +1 } },              /* super e            |  改变主工作区窗口数量 (1 2中切换) */
    
    { MODKEY,              XK_b,            focusmon,         {.i = +1 } },              /* super b            |  光标移动到另一个显示器 */
    { MODKEY|ShiftMask,    XK_b,            tagmon,           {.i = +1 } },              /* super shift b      |  将聚焦窗口移动到另一个显示器 */
    
    { MODKEY,              XK_q,            killclient,       {0} },                     /* super q            |  窗口 */
    { MODKEY|ControlMask,  XK_F12,          quit,             {0} },                     /* super ctrl f12     |  退出dwm */

    /* spawn + SHCMD 执行对应命令 */
    { MODKEY|ShiftMask,    XK_a,            spawn,            SHCMD("~/scripts/app-starter.sh flameshot") },
    { MODKEY,              XK_d,            spawn,            SHCMD("~/scripts/app-starter.sh rofi") },
    { MODKEY|ShiftMask,    XK_k,            spawn,            SHCMD("~/scripts/app-starter.sh screenkey") },
    { MODKEY,              XK_k,            spawn,            SHCMD("~/scripts/app-starter.sh blurlock") },
    { MODKEY,              XK_F1,           spawn,            SHCMD("~/scripts/app-starter.sh pcmanfm") },
    { MODKEY,              XK_Return,       spawn,            SHCMD("~/scripts/app-starter.sh st") },
    { MODKEY|ShiftMask,    XK_Up,           spawn,            SHCMD("~/scripts/app-starter.sh set_vol up &") },
    { MODKEY|ShiftMask,    XK_Down,         spawn,            SHCMD("~/scripts/app-starter.sh set_vol down &") },
    { MODKEY|ShiftMask,    XK_s,            spawn,            SHCMD("~/scripts/app-starter.sh set_vol toggle &") },
    { ShiftMask|ControlMask, XK_c,          spawn,            SHCMD("xclip -o | xclip -selection c") },

    /* super key : 跳转到对应tag */
    /* super shift key : 将聚焦窗口移动到对应tag */
    /* 若跳转后的tag无窗口且附加了cmd1或者cmd2就执行对应的cmd */
    /* key tag cmd1 cmd2 */
    TAGKEYS(XK_1, 0,  0,  0)
    TAGKEYS(XK_2, 1,  0,  0)
    TAGKEYS(XK_3, 2,  0,  0)
    TAGKEYS(XK_4, 3,  0,  0)
    TAGKEYS(XK_5, 4,  0,  0)
    TAGKEYS(XK_6, 5,  0,  0)
    TAGKEYS(XK_7, 6,  0,  0)
    TAGKEYS(XK_8, 7,  0,  0)
    TAGKEYS(XK_9, 8,  "~/scripts/app-starter.sh pcmanfm", "~/scripts/app-starter.sh pcmanfm")
    TAGKEYS(XK_c, 9,  "~/scripts/app-starter.sh chrome",  "~/scripts/app-starter.sh chrome")
    TAGKEYS(XK_m, 10, "~/scripts/app-starter.sh music",   "~/scripts/app-starter.sh pavucontrol")
    TAGKEYS(XK_p, 11, "~/scripts/app-starter.sh postman", "~/scripts/app-starter.sh postman")
    TAGKEYS(XK_0, 12, "~/scripts/app-starter.sh tim",     "~/scripts/app-starter.sh tim")
    TAGKEYS(XK_w, 13, "~/scripts/app-starter.sh wechat",  "~/scripts/app-starter.sh wechat")
    TAGKEYS(XK_l, 14, "~/scripts/app-starter.sh wxwork",  "~/scripts/app-starter.sh wxwork")
};
static Button buttons[] = {
    /* click               event mask       button            function        argument  */
    { ClkWinTitle,         0,               Button1,          togglewin,      {0} },    // 左键        |  点击标题  |  切换窗口隐藏状态
    { ClkTagBar,           0,               Button1,          view,           {0} },    // 左键        |  点击tag   |  切换tag 
    { ClkTagBar,           0,               Button3,          toggleview,     {0} },    // 右键        |  点击tag   |  显示tag
    { ClkClientWin,        MODKEY,          Button1,          movemouse,      {0} },    // super+左键  |  拖拽窗口  |  拖拽窗口
    { ClkClientWin,        MODKEY,          Button3,          resizemouse,    {0} },    // super+右键  |  拖拽窗口  |  改变窗口大小
    { ClkTagBar,           MODKEY,          Button1,          tag,            {0} },    // super+左键  |  点击tag   |  将窗口移动到对应tag
};
