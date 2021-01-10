#include <X11/XF86keysym.h>

/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int borderpx       = 2;        /* border pixel of windows */
static const unsigned int snap           = 32;       /* snap pixel */
static const unsigned int systraypinning = 0;        /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayspacing = 2;        /* systray spacing */
static const int systraypinningfailfirst = 1;        /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static int showsystray                   = 1;        /* 0 means no systray */
static const unsigned int gappih         = 7;        /* horiz inner gap between windows */
static const unsigned int gappiv         = 7;        /* vert inner gap between windows */
static const unsigned int gappoh         = 7;        /* horiz outer gap between windows and screen edge */
static const unsigned int gappov         = 7;        /* vert outer gap between windows and screen edge */
static const int smartgaps               = 0;        /* 1 means no outer gap when there is only one window */
static const int showbar                 = 1;        /* 0 means no bar */
static const int topbar                  = 1;        /* 0 means bottom bar */
static const char *fonts[]               = { "JetBrainsMono Nerd Font Mono:size=12" };
static const char col_gray1[]            = "#222222";
static const char col_gray2[]            = "#444444";
static const char col_gray3[]            = "#bbbbbb";
static const char col_gray4[]            = "#ffffff";
static const char col_cyan[]             = "#37474F";
static const char col_border[]           = "#42A5F5";
static const unsigned int baralpha       = 0xc0;
static const unsigned int borderalpha    = 0xdd;
static const char *colors[][3]           = {
    [SchemeNorm] = { col_gray3, col_gray1, col_gray2  },
    [SchemeSel]  = { col_gray4, col_cyan,  col_border },
    [SchemeHid]  = { col_cyan,  col_gray1, col_border },
};
static const unsigned int alphas[][3]    = {
    [SchemeNorm] = { OPAQUE, baralpha, borderalpha },
    [SchemeSel]  = { OPAQUE, baralpha, borderalpha },
};

/* tagging */
static const char *tags[] = { "一", "二", "三", "四", "五", "六", "七", "八", "九", "C", "M", "P", "Q", "W", "L" };
static const Rule rules[] = {
    /* class                    instance    title               tags mask     isfloating   monitor */
    { "Google-chrome",          NULL,       NULL,               1 << 9,       0,           -1 },
    { "netease-cloud-music",    NULL,       NULL,               1 << 10,      1,           -1 },
    { NULL,                     NULL,       "MySQL Workbench",  1 << 10,      0,           -1 },
    { "Postman",                NULL,       NULL,               1 << 11,      0,           -1 },
    { NULL,                     NULL,       "TIM",              1 << 12,      0,           -1 },
    { NULL,                     NULL,       "qq",               1 << 12,      1,           -1 },
    { NULL,                     NULL,       "QQ",               1 << 12,      1,           -1 },
    { NULL,                     NULL,       "QQ聊天",           1 << 12,      1,           -1 },
    { NULL,                     NULL,       "微信",             1 << 13,      0,           -1 },
    { NULL,                     NULL,       "飞书",             1 << 14,      0,           -1 },
    { NULL,                     NULL,       "broken",           0,            1,           -1 },
};

static const float mfact       = 0.5;  /* factor of master area size [0.05..0.95] */
static const int   nmaster     = 1;    /* number of clients in master area */
static const int   resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const Layout layouts[] = {
    { "♥",         tile },             /* first entry is default */
    { "[M]",       monocle },
    { "><>",       NULL },             /* no layout function means floating behavior */
};

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY, TAG, cmd1, cmd2) \
    { MODKEY,                       KEY,      view,           {.ui = 1 << TAG, .v = cmd1} }, \
    { MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
    { MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG, .v = cmd2} }, \

/* commands */
static const char *screenshotcmd[] = { "flameshot", "gui", NULL };
static const char scratchpadname[] = "scratchpad";
static const char *scratchpadcmd[] = { "st", "-t", scratchpadname, "-g", "120x40", NULL };

static Key keys[] = {
    /* modifier            key              function        argument */
    { MODKEY|ShiftMask,    XK_a,            spawn,            {.v = screenshotcmd } },
    { MODKEY,              XK_d,            spawn,            SHCMD("rofi -show run") },
    { MODKEY,              XK_F1,           spawn,            SHCMD("pcmanfm") },
    { MODKEY|ShiftMask,    XK_k,            spawn,            SHCMD("killall screenkey || screenkey -p fixed -g 50%x8%+100%-11% &") },
    { MODKEY,              XK_k,            spawn,            SHCMD("blurlock") },
    { MODKEY,              XK_Return,       spawn,            SHCMD("st") },
    { MODKEY|ShiftMask,    XK_Up,           spawn,            SHCMD("~/scripts/set-vol.sh up &") },
    { MODKEY|ShiftMask,    XK_Down,         spawn,            SHCMD("~/scripts/set-vol.sh down &") },
    { MODKEY|ShiftMask,    XK_s,            spawn,            SHCMD("~/scripts/set-vol.sh toggle &") },
    { MODKEY|ShiftMask,    XK_t,            spawn,            SHCMD("printf 'H%%vhm4KvTya#jdC6sYoSYCLeL^cZ' | xclip -selection c") },
    { ShiftMask|ControlMask, XK_c,          spawn,            SHCMD("xclip -o | xclip -selection c") },

    { MODKEY,              XK_minus,        togglescratch,    {.v = scratchpadcmd } },
    { MODKEY,              XK_equal,        togglesystray,    {0} },

    { MODKEY,              XK_Tab,          focusstack,       {.i = +1 } },
	{ MODKEY|ShiftMask,    XK_Tab,          rotatestack,      {.i = +1 } },
    { MODKEY,              XK_Left,         viewtoleft,       {0} },
    { MODKEY,              XK_Right,        viewtoright,      {0} },
	{ MODKEY|ShiftMask,    XK_Left,         tagtoleft,        {0} },
	{ MODKEY|ShiftMask,    XK_Right,        tagtoright,       {0} },

    { MODKEY,              XK_comma,        setmfact,         {.f = -0.05} },
    { MODKEY,              XK_period,       setmfact,         {.f = +0.05} },

    { MODKEY,              XK_h,            hidewin,          {0} },
    { MODKEY|ShiftMask,    XK_h,            restorewin,       {0} },
    { MODKEY,              XK_o,            hideotherwins,    {0} },
    { MODKEY|ShiftMask,    XK_o,            restoreotherwins, {0} },

    { MODKEY|ShiftMask,    XK_Return,       zoom,             {0} },
    { MODKEY,              XK_t,            togglefloating,   {0} },
	{ MODKEY,              XK_f,            fullscreen,       {0} },
    { MODKEY,              XK_space,        setlayout,        {0} },
    { MODKEY,              XK_e,            incnmaster,       {.i = +1 } },

    { MODKEY,              XK_a,            view,             {.ui = ~0 } },

    { MODKEY,              XK_b,            focusmon,         {.i = +1 } },
    { MODKEY|ShiftMask,    XK_b,            tagmon,           {.i = +1 } },

    { MODKEY,              XK_q,            killclient,       {0} },
    { MODKEY|ControlMask,  XK_F12,          quit,             {0} },

    TAGKEYS(XK_1, 0,  0, 0)
    TAGKEYS(XK_2, 1,  0, 0)
    TAGKEYS(XK_3, 2,  0, 0)
    TAGKEYS(XK_4, 3,  0, 0)
    TAGKEYS(XK_5, 4,  0, 0)
    TAGKEYS(XK_6, 5,  0, 0)
    TAGKEYS(XK_7, 6,  0, 0)
    TAGKEYS(XK_8, 7,  0, 0)
    TAGKEYS(XK_9, 8,  0, 0)
    TAGKEYS(XK_c, 9,  "google-chrome-stable", "google-chrome-stable")
    TAGKEYS(XK_m, 10, "source ~/.profile && netease-cloud-music", "pavucontrol")
    TAGKEYS(XK_p, 11, "postman", "postman")
    TAGKEYS(XK_0, 12, "/opt/deepinwine/apps/Deepin-TIM/run.sh", "/opt/deepinwine/apps/Deepin-TIM/run.sh")
    TAGKEYS(XK_w, 13, "source ~/.profile && wechat-uos", "source ~/.profile && wechat-uos")
    TAGKEYS(XK_l, 14, "source ~/.profile && cd ~/workspace/electron-lark && electron .", "source ~/.profile && cd ~/workspace/electron-lark && electron .")
};

/* button definitions */
static Button buttons[] = {
    /* click               event mask       button            function        argument  */
    { ClkWinTitle,         0,               Button1,          togglewin,      {0} },
    { ClkStatusText,       0,               Button3,          spawn,          SHCMD("st") },
    { ClkClientWin,        MODKEY,          Button1,          movemouse,      {0} },
    { ClkClientWin,        MODKEY,          Button3,          resizemouse,    {0} },
    { ClkTagBar,           0,               Button1,          view,           {0} },
    { ClkTagBar,           0,               Button3,          toggleview,     {0} },
    { ClkTagBar,           MODKEY,          Button1,          tag,            {0} },
    { ClkTagBar,           MODKEY,          Button3,          toggletag,      {0} },
};
