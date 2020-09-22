From 20967685d6879bd611a856ade154df19da9ddc7b Mon Sep 17 00:00:00 2001
From: Stein Gunnar Bakkeby <bakkeby@gmail.com>
Date: Wed, 8 May 2019 08:07:14 +0200
Subject: [PATCH] Vanity gaps - allows control of both inner and outer gaps
 between windows and screen edge

---
 config.def.h |  21 +++++++++
 dwm.c        | 150 +++++++++++++++++++++++++++++++++++++++++++++++++++++++----
 2 files changed, 161 insertions(+), 10 deletions(-)

diff --git a/config.def.h b/config.def.h
index 1c0b587..0927c2d 100644
--- a/config.def.h
+++ b/config.def.h
@@ -3,6 +3,11 @@
 /* appearance */
 static const unsigned int borderpx  = 1;        /* border pixel of windows */
 static const unsigned int snap      = 32;       /* snap pixel */
+static const unsigned int gappih    = 10;       /* horiz inner gap between windows */
+static const unsigned int gappiv    = 10;       /* vert inner gap between windows */
+static const unsigned int gappoh    = 10;       /* horiz outer gap between windows and screen edge */
+static const unsigned int gappov    = 10;       /* vert outer gap between windows and screen edge */
+static const int smartgaps          = 0;        /* 1 means no outer gap when there is only one window */
 static const int showbar            = 1;        /* 0 means no bar */
 static const int topbar             = 1;        /* 0 means bottom bar */
 static const char *fonts[]          = { "monospace:size=10" };
@@ -70,6 +75,22 @@ static Key keys[] = {
 	{ MODKEY,                       XK_d,      incnmaster,     {.i = -1 } },
 	{ MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
 	{ MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
+	{ MODKEY|Mod4Mask,              XK_h,      incrgaps,       {.i = +1 } },
+	{ MODKEY|Mod4Mask,              XK_l,      incrgaps,       {.i = -1 } },
+	{ MODKEY|Mod4Mask|ShiftMask,    XK_h,      incrogaps,      {.i = +1 } },
+	{ MODKEY|Mod4Mask|ShiftMask,    XK_l,      incrogaps,      {.i = -1 } },
+	{ MODKEY|Mod4Mask|ControlMask,  XK_h,      incrigaps,      {.i = +1 } },
+	{ MODKEY|Mod4Mask|ControlMask,  XK_l,      incrigaps,      {.i = -1 } },
+	{ MODKEY|Mod4Mask,              XK_0,      togglegaps,     {0} },
+	{ MODKEY|Mod4Mask|ShiftMask,    XK_0,      defaultgaps,    {0} },
+	{ MODKEY,                       XK_y,      incrihgaps,     {.i = +1 } },
+	{ MODKEY,                       XK_o,      incrihgaps,     {.i = -1 } },
+	{ MODKEY|ControlMask,           XK_y,      incrivgaps,     {.i = +1 } },
+	{ MODKEY|ControlMask,           XK_o,      incrivgaps,     {.i = -1 } },
+	{ MODKEY|Mod4Mask,              XK_y,      incrohgaps,     {.i = +1 } },
+	{ MODKEY|Mod4Mask,              XK_o,      incrohgaps,     {.i = -1 } },
+	{ MODKEY|ShiftMask,             XK_y,      incrovgaps,     {.i = +1 } },
+	{ MODKEY|ShiftMask,             XK_o,      incrovgaps,     {.i = -1 } },
 	{ MODKEY,                       XK_Return, zoom,           {0} },
 	{ MODKEY,                       XK_Tab,    view,           {0} },
 	{ MODKEY|ShiftMask,             XK_c,      killclient,     {0} },
diff --git a/dwm.c b/dwm.c
index 4465af1..88f3e04 100644
--- a/dwm.c
+++ b/dwm.c
@@ -119,6 +119,10 @@ struct Monitor {
 	int by;               /* bar geometry */
 	int mx, my, mw, mh;   /* screen size */
 	int wx, wy, ww, wh;   /* window area  */
+	int gappih;           /* horizontal gap between windows */
+	int gappiv;           /* vertical gap between windows */
+	int gappoh;           /* horizontal outer gaps */
+	int gappov;           /* vertical outer gaps */
 	unsigned int seltags;
 	unsigned int sellt;
 	unsigned int tagset[2];
@@ -199,6 +203,16 @@ static void sendmon(Client *c, Monitor *m);
 static void setclientstate(Client *c, long state);
 static void setfocus(Client *c);
 static void setfullscreen(Client *c, int fullscreen);
+static void setgaps(int oh, int ov, int ih, int iv);
+static void incrgaps(const Arg *arg);
+static void incrigaps(const Arg *arg);
+static void incrogaps(const Arg *arg);
+static void incrohgaps(const Arg *arg);
+static void incrovgaps(const Arg *arg);
+static void incrihgaps(const Arg *arg);
+static void incrivgaps(const Arg *arg);
+static void togglegaps(const Arg *arg);
+static void defaultgaps(const Arg *arg);
 static void setlayout(const Arg *arg);
 static void setmfact(const Arg *arg);
 static void setup(void);
@@ -240,6 +254,7 @@ static char stext[256];
 static int screen;
 static int sw, sh;           /* X display screen geometry width, height */
 static int bh, blw = 0;      /* bar geometry */
+static int enablegaps = 1;   /* enables gaps, used by togglegaps */
 static int lrpad;            /* sum of left and right padding for text */
 static int (*xerrorxlib)(Display *, XErrorEvent *);
 static unsigned int numlockmask = 0;
@@ -638,6 +653,10 @@ createmon(void)
 	m->nmaster = nmaster;
 	m->showbar = showbar;
 	m->topbar = topbar;
+	m->gappih = gappih;
+	m->gappiv = gappiv;
+	m->gappoh = gappoh;
+	m->gappov = gappov;
 	m->lt[0] = &layouts[0];
 	m->lt[1] = &layouts[1 % LENGTH(layouts)];
 	strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);
@@ -1498,6 +1517,111 @@ setfullscreen(Client *c, int fullscreen)
 }
 
 void
+setgaps(int oh, int ov, int ih, int iv)
+{
+	if (oh < 0) oh = 0;
+	if (ov < 0) ov = 0;
+	if (ih < 0) ih = 0;
+	if (iv < 0) iv = 0;
+
+	selmon->gappoh = oh;
+	selmon->gappov = ov;
+	selmon->gappih = ih;
+	selmon->gappiv = iv;
+	arrange(selmon);
+}
+
+void
+togglegaps(const Arg *arg)
+{
+	enablegaps = !enablegaps;
+	arrange(selmon);
+}
+
+void
+defaultgaps(const Arg *arg)
+{
+	setgaps(gappoh, gappov, gappih, gappiv);
+}
+
+void
+incrgaps(const Arg *arg)
+{
+	setgaps(
+		selmon->gappoh + arg->i,
+		selmon->gappov + arg->i,
+		selmon->gappih + arg->i,
+		selmon->gappiv + arg->i
+	);
+}
+
+void
+incrigaps(const Arg *arg)
+{
+	setgaps(
+		selmon->gappoh,
+		selmon->gappov,
+		selmon->gappih + arg->i,
+		selmon->gappiv + arg->i
+	);
+}
+
+void
+incrogaps(const Arg *arg)
+{
+	setgaps(
+		selmon->gappoh + arg->i,
+		selmon->gappov + arg->i,
+		selmon->gappih,
+		selmon->gappiv
+	);
+}
+
+void
+incrohgaps(const Arg *arg)
+{
+	setgaps(
+		selmon->gappoh + arg->i,
+		selmon->gappov,
+		selmon->gappih,
+		selmon->gappiv
+	);
+}
+
+void
+incrovgaps(const Arg *arg)
+{
+	setgaps(
+		selmon->gappoh,
+		selmon->gappov + arg->i,
+		selmon->gappih,
+		selmon->gappiv
+	);
+}
+
+void
+incrihgaps(const Arg *arg)
+{
+	setgaps(
+		selmon->gappoh,
+		selmon->gappov,
+		selmon->gappih + arg->i,
+		selmon->gappiv
+	);
+}
+
+void
+incrivgaps(const Arg *arg)
+{
+	setgaps(
+		selmon->gappoh,
+		selmon->gappov,
+		selmon->gappih,
+		selmon->gappiv + arg->i
+	);
+}
+
+void
 setlayout(const Arg *arg)
 {
 	if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt])
@@ -1673,26 +1797,32 @@ tagmon(const Arg *arg)
 void
 tile(Monitor *m)
 {
-	unsigned int i, n, h, mw, my, ty;
+	unsigned int i, n, h, r, oe = enablegaps, ie = enablegaps, mw, my, ty;
 	Client *c;
 
 	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
 	if (n == 0)
 		return;
 
+	if (smartgaps == n) {
+		oe = 0; // outer gaps disabled
+	}
+
 	if (n > m->nmaster)
-		mw = m->nmaster ? m->ww * m->mfact : 0;
+		mw = m->nmaster ? (m->ww + m->gappiv*ie) * m->mfact : 0;
 	else
-		mw = m->ww;
-	for (i = my = ty = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
+		mw = m->ww - 2*m->gappov*oe + m->gappiv*ie;
+	for (i = 0, my = ty = m->gappoh*oe, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
 		if (i < m->nmaster) {
-			h = (m->wh - my) / (MIN(n, m->nmaster) - i);
-			resize(c, m->wx, m->wy + my, mw - (2*c->bw), h - (2*c->bw), 0);
-			my += HEIGHT(c);
+			r = MIN(n, m->nmaster) - i;
+			h = (m->wh - my - m->gappoh*oe - m->gappih*ie * (r - 1)) / r;
+			resize(c, m->wx + m->gappov*oe, m->wy + my, mw - (2*c->bw) - m->gappiv*ie, h - (2*c->bw), 0);
+			my += HEIGHT(c) + m->gappih*ie;
 		} else {
-			h = (m->wh - ty) / (n - i);
-			resize(c, m->wx + mw, m->wy + ty, m->ww - mw - (2*c->bw), h - (2*c->bw), 0);
-			ty += HEIGHT(c);
+			r = n - i;
+			h = (m->wh - ty - m->gappoh*oe - m->gappih*ie * (r - 1)) / r;
+			resize(c, m->wx + mw + m->gappov*oe, m->wy + ty, m->ww - mw - (2*c->bw) - 2*m->gappov*oe, h - (2*c->bw), 0);
+			ty += HEIGHT(c) + m->gappih*ie;
 		}
 }
 
-- 
2.7.4

