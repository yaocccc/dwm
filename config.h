#include <X11/XF86keysym.h>

/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int borderpx       = 2;        /* border pixel of windows */
static const unsigned int snap           = 32;       /* snap pixel */
static const unsigned int systraypinning = 0;        /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayspacing = 2;        /* systray spacing */
static const int systraypinningfailfirst = 1;        /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static int showsystray                   = 1;        /* 0 means no systray */
static const unsigned int gappih         = 7;       /* horiz inner gap between windows */
static const unsigned int gappiv         = 7;       /* vert inner gap between windows */
static const unsigned int gappoh         = 7;       /* horiz outer gap between windows and screen edge */
static const unsigned int gappov         = 7;       /* vert outer gap between windows and screen edge */
static const int smartgaps               = 0;        /* 1 means no outer gap when there is only one window */
static const int showbar                 = 1;        /* 0 means no bar */
static const int topbar                  = 1;        /* 0 means bottom bar */
static const Bool viewontag              = True;     /* Switch view on tag switch */
static const char *fonts[]               = { "JetBrainsMono Nerd Font Mono:size=12" };
static const char dmenufont[]            = "Liberation Mono:size=12";
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
static const char *tags[] = { "一", "二", "三", "四", "五", "六", "七", "八", "九" };
static const Rule rules[] = {
    /* class                    instance    title       tags mask     isfloating   monitor */
    { "netease-cloud-music",    NULL,       NULL,       0,            1,           -1 },
    { NULL,                     NULL,       "TIM",      0,            1,           -1 },
    { NULL,                     NULL,       "broken",   0,            1,           -1 },
};

static const float mfact       = 0.5;  /* factor of master area size [0.05..0.95] */
static const int   nmaster     = 1;    /* number of clients in master area */
static const int   resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const Layout layouts[] = {
    { "♥",         tile },             /* first entry is default */
    { "[M]",       monocle },
    { "><>",       NULL },             /* no layout function means floating behavior */
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
    { MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
    { MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2]            = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[]      = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", col_gray1, "-nf", col_gray3, "-sb", col_cyan, "-sf", col_gray4, NULL };
static const char *screenshotcmd[] = { "flameshot", "gui", NULL };
static const char scratchpadname[] = "scratchpad";
static const char *scratchpadcmd[] = { "st", "-t", scratchpadname, "-g", "120x40", NULL };

static Key keys[] = {
    /* modifier            key              function        argument */
    { MODKEY|ShiftMask,    XK_a,            spawn,            {.v = screenshotcmd } },
    { MODKEY,              XK_c,            spawn,            SHCMD("google-chrome-stable") },
    { MODKEY,              XK_d,            spawn,            SHCMD("rofi -show run") },
    { MODKEY|ShiftMask,    XK_d,            spawn,            {.v = dmenucmd } },
    { MODKEY,              XK_F1,           spawn,            SHCMD("pcmanfm") },
    { MODKEY,              XK_k,            spawn,            SHCMD("killall screenkey || screenkey &") },
    { MODKEY,              XK_l,            spawn,            SHCMD("blurlock") },
    { MODKEY,              XK_m,            spawn,            SHCMD("source ~/.profile && netease-cloud-music") },
    { MODKEY|ShiftMask,    XK_m,            spawn,            SHCMD("pavucontrol") },
    { MODKEY,              XK_p,            spawn,            SHCMD("~/scripts/set-privoxy.sh &") },
    { MODKEY|ShiftMask,    XK_q,            spawn,            SHCMD("/opt/deepinwine/apps/Deepin-TIM/run.sh &") },
    { MODKEY,              XK_w,            spawn,            SHCMD("feh --randomize --bg-fill ~/Pictures/* &") },
    { MODKEY,              XK_Return,       spawn,            SHCMD("st") },
    { MODKEY,              XK_minus,        togglescratch,    {.v = scratchpadcmd } },
    { MODKEY,              XK_equal,        togglesystray,    {0} },

    { MODKEY,              XK_Tab,          focusstack,       {.i = +1 } },
	{ MODKEY|ShiftMask,    XK_Tab,          rotatestack,      {.i = +1 } },
    { MODKEY,              XK_Left,         viewtoleft,       {0} },
    { MODKEY,              XK_Right,        viewtoright,      {0} },
	{ MODKEY|ShiftMask,    XK_Left,         tagtoleft,        {0} },
	{ MODKEY|ShiftMask,    XK_Right,        tagtoright,       {0} },
    { MODKEY|ShiftMask,    XK_Up,           spawn,            SHCMD("~/scripts/set-vol.sh up &") },
    { MODKEY|ShiftMask,    XK_Down,         spawn,            SHCMD("~/scripts/set-vol.sh down &") },
    { MODKEY|ShiftMask,    XK_s,            spawn,            SHCMD("~/scripts/set-vol.sh toggle &") },
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

    { MODKEY,              XK_0,            view,             {.ui = ~0 } },
    { MODKEY|ShiftMask,    XK_0,            tag,              {.ui = ~0 } },

    { MODKEY,              XK_b,            focusmon,         {.i = +1 } },
    { MODKEY|ShiftMask,    XK_b,            tagmon,           {.i = +1 } },

    { MODKEY,              XK_q,            killclient,       {0} },
    { MODKEY|ControlMask,  XK_F12,          quit,             {0} },

    TAGKEYS(               XK_1,                              0)
    TAGKEYS(               XK_2,                              1)
    TAGKEYS(               XK_3,                              2)
    TAGKEYS(               XK_4,                              3)
    TAGKEYS(               XK_5,                              4)
    TAGKEYS(               XK_6,                              5)
    TAGKEYS(               XK_7,                              6)
    TAGKEYS(               XK_8,                              7)
    TAGKEYS(               XK_9,                              8)
};

/* button definitions */
static Button buttons[] = {
    /* click               event mask       button            function        argument  */
    { ClkWinTitle,         0,               Button1,          togglewin,      {0} },
    { ClkWinTitle,         0,               Button2,          zoom,           {0} },
    { ClkStatusText,       0,               Button2,          spawn,          SHCMD("st") },
    { ClkClientWin,        MODKEY,          Button1,          movemouse,      {0} },
    { ClkClientWin,        MODKEY,          Button2,          togglefloating, {0} },
    { ClkClientWin,        MODKEY,          Button3,          resizemouse,    {0} },
    { ClkTagBar,           0,               Button1,          view,           {0} },
    { ClkTagBar,           0,               Button3,          toggleview,     {0} },
    { ClkTagBar,           MODKEY,          Button1,          tag,            {0} },
    { ClkTagBar,           MODKEY,          Button3,          toggletag,      {0} },
};
