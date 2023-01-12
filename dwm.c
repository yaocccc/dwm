/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance. Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>

#include "drw.h"
#include "util.h"

/* macros */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define ISVISIBLE(C)            ((C->mon->isoverview || C->isglobal || C->tags & C->mon->tagset[C->mon->seltags]))
#define HIDDEN(C)               ((getstate(C->win) == IconicState))
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TAGMASK                 ((1 << LENGTH(tags)) - 1)
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)
#define SYSTEM_TRAY_REQUEST_DOCK    0

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY      0
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_FOCUS_IN             4
#define XEMBED_MODALITY_ON         10

#define XEMBED_MAPPED              (1 << 0)
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_WINDOW_DEACTIVATE    2

#define VERSION_MAJOR               0
#define VERSION_MINOR               0
#define XEMBED_EMBEDDED_VERSION (VERSION_MAJOR << 16) | VERSION_MINOR

#define OPAQUE                  0xffU

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum {
    SchemeNorm,       // 普通
    SchemeSel,        // 选中的
    SchemeSelGlobal,  // 全局并选中的
    SchemeHid,        // 隐藏的
    SchemeSystray,    // 托盘
    SchemeNormTag,    // 普通标签
    SchemeSelTag,     // 选中的标签
    SchemeUnderline,  // 下划线
    SchemeBarEmpty,   // 状态栏空白部分
    SchemeStatusText  // 状态栏文本
}; /* color schemes */
enum { NetSupported, NetWMName, NetWMState, NetWMCheck,
       NetSystemTray, NetSystemTrayOP, NetSystemTrayOrientation, NetSystemTrayOrientationHorz,
       NetWMFullscreen, NetActiveWindow, NetWMWindowType,
       NetWMWindowTypeDialog, NetClientList, NetLast }; /* EWMH atoms */
enum { Manager, Xembed, XembedInfo, XLast }; /* Xembed atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; /* default atoms */
enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkBarEmpty,
       ClkClientWin, ClkRootWin, ClkLast }; /* clicks */
enum { UP, DOWN, LEFT, RIGHT }; /* movewin */
enum { V_EXPAND, V_REDUCE, H_EXPAND, H_REDUCE }; /* resizewins */

typedef struct {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
	char name[256];
	float mina, maxa;
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int bw, oldbw;
    int taskw;
	unsigned int tags;
	int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen, isglobal, isnoborder, isscratchpad;
	Client *next;
	Client *snext;
	Monitor *mon;
	Window win;
};

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
} Layout;

typedef struct Pertag Pertag;
struct Monitor {
	char ltsymbol[16];
	float mfact;
	int nmaster;
	int num;
	int by;               /* bar geometry */
	int bt;               /* number of tasks */
	int mx, my, mw, mh;   /* screen size */
	int wx, wy, ww, wh;   /* window area  */
	unsigned int seltags;
	unsigned int sellt;
	unsigned int tagset[2];
	int showbar;
	int topbar;
	Client *clients;
	Client *sel;
	Client *stack;
	Monitor *next;
	Window barwin;
	const Layout *lt[2];
	Pertag *pertag;
    uint isoverview;
};

typedef struct {
	const char *class;
	const char *instance;
	const char *title;
	unsigned int tags;
	int isfloating;
    int isglobal;
	int isnoborder;
	int monitor;
} Rule;

typedef struct Systray   Systray;
struct Systray {
	Window win;
	Client *icons;
};

/* function declarations */
static void logtofile(char log[100]);

static void tile(Monitor *m);
static void magicgrid(Monitor *m);
static void overview(Monitor *m);
static void grid(Monitor *m, uint gappo, uint uappi);

static void applyrules(Client *c);
static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachstack(Client *c);
static void buttonpress(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static void clickstatusbar(const Arg *arg);
static Monitor *createmon(void);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static Monitor *dirtomon(int dir);

static void drawbar(Monitor *m);
static void drawbars(void);
static int drawstatusbar(Monitor *m, int bh, char* text);

static void enternotify(XEvent *e);
static void expose(XEvent *e);

static void focusin(XEvent *e);
static void focus(Client *c);
static void focusmon(const Arg *arg);
static void focusstack(const Arg *arg);

static void pointerfocuswin(Client *c);

static Atom getatomprop(Client *c, Atom prop);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static unsigned int getsystraywidth();
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);

static void grabbuttons(Client *c, int focused);
static void grabkeys(void);

static void hide(Client *c);
static void show(Client *c);
static void showtag(Client *c);
static void hidewin(const Arg *arg);
static void hideotherwins(const Arg *arg);
static void showonlyorall(const Arg *arg);
static int issinglewin(const Arg *arg);
static void togglewin(const Arg *arg);
static void toggleglobal(const Arg *arg);
static void restorewin(const Arg *arg);

static void incnmaster(const Arg *arg);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void forcekillclient(const Arg *arg);

static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static void movewin(const Arg *arg);
static void resizewin(const Arg *arg);
static Client *nexttiled(Client *c);
static void pop(Client *);
static void propertynotify(XEvent *e);
static void quit(const Arg *arg);
static void setup(void);
static void seturgent(Client *c, int urg);
static void sigchld(int unused);
static void spawn(const Arg *arg);
static Monitor *systraytomon(Monitor *m);

static Monitor *recttomon(int x, int y, int w, int h);
static void removesystrayicon(Client *i);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizebarwin(Monitor *m);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void resizerequest(XEvent *e);
static void restack(Monitor *m);

static void run(void);
static void runAutostart(void);
static void scan(void);
static int sendevent(Window w, Atom proto, int m, long d0, long d1, long d2, long d3, long d4);
static void sendmon(Client *c, Monitor *m);
static void setclientstate(Client *c, long state);
static void setfocus(Client *c);

static void selectlayout(const Arg *arg);
static void setlayout(const Arg *arg);

static void fullscreen(const Arg *arg);
static void setfullscreen(Client *c, int fullscreen);
static void setmfact(const Arg *arg);

static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static void tagtoleft(const Arg *arg);
static void tagtoright(const Arg *arg);

static void togglebar(const Arg *arg);
static void togglesystray();
static void togglefloating(const Arg *arg);
static void toggleallfloating(const Arg *arg);
static void togglescratch(const Arg *arg);

static void unfocus(Client *c, int setfocus);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);

static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateclientlist(void);
static int updategeom(void);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatesystray(void);
static void updatesystrayicongeom(Client *i, int w, int h);
static void updatesystrayiconstate(Client *i, XPropertyEvent *ev);
static void updatetitle(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);

static void setgap(const Arg *arg);

static void view(const Arg *arg);
static void viewtoleft(const Arg *arg);
static void viewtoright(const Arg *arg);
static void toggleview(const Arg *arg);
static void toggleoverview(const Arg *arg);

static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static Client *wintosystrayicon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void xinitvisual();
static void zoom(const Arg *arg);

/* variables */
static Systray *systray =  NULL;
static const char broken[] = "broken";
static char stext[1024];
static int screen;
static int sw, sh;           /* X display screen geometry width, height */
static int bh, blw = 0;      /* bar geometry */
static int lrpad;            /* sum of left and right padding for text */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[Expose] = expose,
	[FocusIn] = focusin,
	[KeyPress] = keypress,
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[MotionNotify] = motionnotify,
	[PropertyNotify] = propertynotify,
	[ResizeRequest] = resizerequest,
	[UnmapNotify] = unmapnotify
};
static Atom wmatom[WMLast], netatom[NetLast], xatom[XLast];
static int running = 1;
static Cur *cursor[CurLast];
static Clr **scheme;
static Display *dpy;
static Drw *drw;
static int useargb = 0;
static Visual *visual;
static int depth;
static Colormap cmap;
static Monitor *mons, *selmon;
static Window root, wmcheckwin;

static int hiddenWinStackTop = -1;
static Client *hiddenWinStack[100];

/* configuration, allows nested code to access above variables */
#include "config.h"

struct Pertag {
	unsigned int curtag, prevtag; /* current and previous tag */
	int nmasters[LENGTH(tags) + 1]; /* number of windows in master area */
	float mfacts[LENGTH(tags) + 1]; /* mfacts per tag */
	unsigned int sellts[LENGTH(tags) + 1]; /* selected layouts */
	const Layout *ltidxs[LENGTH(tags) + 1][2]; /* matrix of tags and layouts indexes  */
	int showbars[LENGTH(tags) + 1]; /* display bar for the current tag */
};

/* function implementations */
void
logtofile(char log[100])
{
    char cmd [150];
    sprintf(cmd, "echo '%s' >> ~/log", log);
    system(cmd);
}

void
applyrules(Client *c)
{
    const char *class, *instance;
    unsigned int i;
    const Rule *r;
    Monitor *m;
    XClassHint ch = { NULL, NULL };

    /* rule matching */
    c->isfloating = 0;
    c->isglobal = 0;
    c->isnoborder = 0;
    c->isscratchpad = 0;
    c->tags = 0;
    XGetClassHint(dpy, c->win, &ch);
    class    = ch.res_class ? ch.res_class : broken;
    instance = ch.res_name  ? ch.res_name  : broken;

    for (i = 0; i < LENGTH(rules); i++) {
        r = &rules[i];
        if ((!r->title || strstr(c->name, r->title))
                && (!r->class || strstr(class, r->class))
                && (!r->instance || strstr(instance, r->instance)))
        {
            c->isfloating = r->isfloating;
            c->isglobal = r->isglobal;
            c->isnoborder = r->isnoborder;
            c->tags |= r->tags;
            if (r->isnoborder)
                c->bw = 0;
            for (m = mons; m && m->num != r->monitor; m = m->next);
            if (m)
                c->mon = m;
        }
    }
    if (!strcmp(c->name, scratchpadname)) {
        c->isscratchpad = 1;
        c->isglobal = 1; // scratchpad is default global
    } 
    if (ch.res_class)
        XFree(ch.res_class);
    if (ch.res_name)
        XFree(ch.res_name);
    c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->tagset[c->mon->seltags];
}

int
applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact)
{
    int baseismin;
    Monitor *m = c->mon;

    /* set minimum possible */
    *w = MAX(1, *w);
    *h = MAX(1, *h);
    if (interact) {
        if (*x > sw)
            *x = sw - WIDTH(c);
        if (*y > sh)
            *y = sh - HEIGHT(c);
        if (*x + *w + 2 * c->bw < 0)
            *x = 0;
        if (*y + *h + 2 * c->bw < 0)
            *y = 0;
    } else {
        if (*x >= m->wx + m->ww)
            *x = m->wx + m->ww - WIDTH(c);
        if (*y >= m->wy + m->wh)
            *y = m->wy + m->wh - HEIGHT(c);
        if (*x + *w + 2 * c->bw <= m->wx)
            *x = m->wx;
        if (*y + *h + 2 * c->bw <= m->wy)
            *y = m->wy;
    }
    if (*h < bh)
        *h = bh;
    if (*w < bh)
        *w = bh;
    if (c->isfloating) {
        /* see last two sentences in ICCCM 4.1.2.3 */
        baseismin = c->basew == c->minw && c->baseh == c->minh;
        if (!baseismin) { /* temporarily remove base dimensions */
            *w -= c->basew;
            *h -= c->baseh;
        }
        /* adjust for aspect limits */
        if (c->mina > 0 && c->maxa > 0) {
            if (c->maxa < (float)*w / *h)
                *w = *h * c->maxa + 0.5;
            else if (c->mina < (float)*h / *w)
                *h = *w * c->mina + 0.5;
        }
        if (baseismin) { /* increment calculation requires this */
            *w -= c->basew;
            *h -= c->baseh;
        }
        /* adjust for increment value */
        if (c->incw)
            *w -= *w % c->incw;
        if (c->inch)
            *h -= *h % c->inch;
        /* restore base dimensions */
        *w = MAX(*w + c->basew, c->minw);
        *h = MAX(*h + c->baseh, c->minh);
        if (c->maxw)
            *w = MIN(*w, c->maxw);
        if (c->maxh)
            *h = MIN(*h, c->maxh);
    }
    return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void
arrange(Monitor *m)
{
    if (m)
        showtag(m->stack);
    else for (m = mons; m; m = m->next)
        showtag(m->stack);
    if (m) {
        arrangemon(m);
        restack(m);
    } else for (m = mons; m; m = m->next)
        arrangemon(m);
}

void
arrangemon(Monitor *m)
{
    if (m->isoverview) {
        strncpy(m->ltsymbol, overviewlayout.symbol, sizeof m->ltsymbol);
        overviewlayout.arrange(m);
    } else {
        strncpy(m->ltsymbol, m->lt[m->sellt]->symbol, sizeof m->ltsymbol);
        m->lt[m->sellt]->arrange(m);
    }
}

void
attach(Client *c)
{
    if (!newclientathead) {
        Client **tc;
        for (tc = &c->mon->clients; *tc; tc = &(*tc)->next);
        *tc = c;
        c->next = NULL;
    } else {
        c->next = c->mon->clients;
        c->mon->clients = c;
    }
}

void
attachstack(Client *c)
{
    c->snext = c->mon->stack;
    c->mon->stack = c;
}

void
buttonpress(XEvent *e)
{
    unsigned int i, x, click, occ = 0;
    Arg arg = {0};
    Client *c;
    Monitor *m;
    XButtonPressedEvent *ev = &e->xbutton;

    // 判断鼠标点击的位置
    click = ClkRootWin;
    /* focus monitor if necessary */
    if ((m = wintomon(ev->window)) && m != selmon) {
        unfocus(selmon->sel, 1);
        selmon = m;
        focus(NULL);
    }
    int status_w = drawstatusbar(selmon, bh, stext);
    if (ev->window == selmon->barwin) { // 点击在bar上
        i = x = 0;
        blw = TEXTW(selmon->ltsymbol);
        
        if (selmon->isoverview) {
            x += TEXTW(overviewtag);
            i = ~0;
            if (ev->x > x)
                i = LENGTH(tags);
        } else {
            for (c = m->clients; c; c = c->next)
                occ |= c->tags == TAGMASK ? 0 : c->tags;
            do {
                /* do not reserve space for vacant tags */
                if (!(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
                    continue;
                x += TEXTW(tags[i]);
            } while (ev->x >= x && ++i < LENGTH(tags));
        }
        if (i < LENGTH(tags)) {
            click = ClkTagBar;
            arg.ui = 1 << i;
        } else if (ev->x < x + blw)
            click = ClkLtSymbol;
        else if (ev->x > selmon->ww - status_w -  (selmon == systraytomon(selmon) ? getsystraywidth() : 0)) {
            click = ClkStatusText;
            arg.i = ev->x - (selmon->ww - status_w - (selmon == systraytomon(selmon) ? getsystraywidth() : 0));
            arg.ui = ev->button; // 1 => L，2 => M，3 => R, 5 => U, 6 => D
        } else {
            click = ClkBarEmpty;

            x += blw;
            c = m->clients;

            if (m->bt == 0) return;

            do {
                if (!ISVISIBLE(c))
                    continue;
                else
                    x += c->taskw;
            } while (ev->x > x && (c = c->next));

            if (c) {
                click = ClkWinTitle;
                arg.v = c;
            }
        }
    } else if ((c = wintoclient(ev->window))) {
        focus(c);
        restack(selmon);
        XAllowEvents(dpy, ReplayPointer, CurrentTime);
        click = ClkClientWin;
    }
    for (i = 0; i < LENGTH(buttons); i++)
        if (click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
                && CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
            buttons[i].func((click == ClkTagBar || click == ClkWinTitle || click == ClkStatusText) && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
}

void
checkotherwm(void)
{
    xerrorxlib = XSetErrorHandler(xerrorstart);
    /* this causes an error if some other window manager is running */
    XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
    XSync(dpy, False);
    XSetErrorHandler(xerror);
    XSync(dpy, False);
}

void
cleanup(void)
{
    Arg a = {.ui = ~0};
    Layout foo = { "", NULL };
    Monitor *m;
    size_t i;

    view(&a);
    selmon->lt[selmon->sellt] = &foo;
    for (m = mons; m; m = m->next)
        while (m->stack)
            unmanage(m->stack, 0);
    XUngrabKey(dpy, AnyKey, AnyModifier, root);
    while (mons)
        cleanupmon(mons);
    if (showsystray) {
        XUnmapWindow(dpy, systray->win);
        XDestroyWindow(dpy, systray->win);
        free(systray);
    }
    for (i = 0; i < CurLast; i++)
        drw_cur_free(drw, cursor[i]);
    for (i = 0; i < LENGTH(colors) + 1; i++)
        free(scheme[i]);
    XDestroyWindow(dpy, wmcheckwin);
    drw_free(drw);
    XSync(dpy, False);
    XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
    XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

void
cleanupmon(Monitor *mon)
{
    Monitor *m;

    if (mon == mons)
        mons = mons->next;
    else {
        for (m = mons; m && m->next != mon; m = m->next);
        m->next = mon->next;
    }
    XUnmapWindow(dpy, mon->barwin);
    XDestroyWindow(dpy, mon->barwin);
    free(mon);
}

void
clientmessage(XEvent *e)
{
    XWindowAttributes wa;
    XSetWindowAttributes swa;
    XClientMessageEvent *cme = &e->xclient;
    Client *c = wintoclient(cme->window);

    if (showsystray && cme->window == systray->win && cme->message_type == netatom[NetSystemTrayOP]) {
        /* add systray icons */
        if (cme->data.l[1] == SYSTEM_TRAY_REQUEST_DOCK) {
            if (!(c = (Client *)calloc(1, sizeof(Client))))
                die("fatal: could not malloc() %u bytes\n", sizeof(Client));
            if (!(c->win = cme->data.l[2])) {
                free(c);
                return;
            }
            c->mon = selmon;
            c->next = systray->icons;
            systray->icons = c;
            XGetWindowAttributes(dpy, c->win, &wa);
            c->x = c->oldx = c->y = c->oldy = 0;
            c->w = c->oldw = wa.width;
            c->h = c->oldh = wa.height;
            c->oldbw = wa.border_width;
            c->bw = 0;
            c->isfloating = True;
            /* reuse tags field as mapped status */
            c->tags = 1;
            updatesizehints(c);
            updatesystrayicongeom(c, wa.width, wa.height);
            XAddToSaveSet(dpy, c->win);
            XSelectInput(dpy, c->win, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
            XReparentWindow(dpy, c->win, systray->win, 0, 0);
            XClassHint ch = {"dwmsystray", "dwmsystray"};
            XSetClassHint(dpy, c->win, &ch);
            /* use parents background color */
            swa.background_pixel  = scheme[SchemeNorm][ColBg].pixel;
            XChangeWindowAttributes(dpy, c->win, CWBackPixel, &swa);
            sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_EMBEDDED_NOTIFY, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
            /* FIXME not sure if I have to send these events, too */
            sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_FOCUS_IN, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
            sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
            sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_MODALITY_ON, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
            XSync(dpy, False);
            resizebarwin(selmon);
            updatesystray();
            setclientstate(c, NormalState);
        }
        return;
    }
    if (!c)
        return;
    if (cme->message_type == netatom[NetWMState]) {
        if (cme->data.l[1] == netatom[NetWMFullscreen]
                || cme->data.l[2] == netatom[NetWMFullscreen])
            setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
                        || (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !c->isfullscreen)));
    } else if (cme->message_type == netatom[NetActiveWindow]) {
        if (c != selmon->sel && !c->isurgent)
            seturgent(c, 1);
        if (c == selmon->sel) return;
        // 若不是当前显示器 则跳转到对应显示器
        if (c->mon != selmon) {
            focusmon(&(Arg) { .i = +1 });
        }
        // 若不是当前tag 则跳转到对应tag
        if (!ISVISIBLE(c)) {
            view(&(Arg) { .ui = c->tags });
        }
    }
}

void
configure(Client *c)
{
    XConfigureEvent ce;

    ce.type = ConfigureNotify;
    ce.display = dpy;
    ce.event = c->win;
    ce.window = c->win;
    ce.x = c->x;
    ce.y = c->y;
    ce.width = c->w;
    ce.height = c->h;
    ce.border_width = c->bw;
    ce.above = None;
    ce.override_redirect = False;
    XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void
configurenotify(XEvent *e)
{
    Monitor *m;
    Client *c;
    XConfigureEvent *ev = &e->xconfigure;
    int dirty;

    /* TODO: updategeom handling sucks, needs to be simplified */
    if (ev->window == root) {
        dirty = (sw != ev->width || sh != ev->height);
        sw = ev->width;
        sh = ev->height;
        if (updategeom() || dirty) {
            drw_resize(drw, sw, bh);
            updatebars();
            for (m = mons; m; m = m->next) {
                for (c = m->clients; c; c = c->next)
                    if (c->isfullscreen)
                        resizeclient(c, m->mx, m->my, m->mw, m->mh);
                resizebarwin(m);
            }
            focus(NULL);
            arrange(NULL);
        }
    }
}

void
configurerequest(XEvent *e)
{
    Client *c;
    Monitor *m;
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;

    if ((c = wintoclient(ev->window))) {
        if (ev->value_mask & CWBorderWidth)
            c->bw = ev->border_width;
        else if (c->isfloating) {
            m = c->mon;
            if (ev->value_mask & CWX) {
                c->oldx = c->x;
                c->x = m->mx + ev->x;
            }
            if (ev->value_mask & CWY) {
                c->oldy = c->y;
                c->y = m->my + ev->y;
            }
            if (ev->value_mask & CWWidth) {
                c->oldw = c->w;
                c->w = ev->width;
            }
            if (ev->value_mask & CWHeight) {
                c->oldh = c->h;
                c->h = ev->height;
            }
            if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
                c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
            if ((c->y + c->h) > m->my + m->mh && c->isfloating)
                c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
            if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
                configure(c);
            if (ISVISIBLE(c))
                XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
        } else
            configure(c);
    } else {
        wc.x = ev->x;
        wc.y = ev->y;
        wc.width = ev->width;
        wc.height = ev->height;
        wc.border_width = ev->border_width;
        wc.sibling = ev->above;
        wc.stack_mode = ev->detail;
        XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
    }
    XSync(dpy, False);
}

Monitor *
createmon(void)
{
    Monitor *m;
    unsigned int i;

    m = ecalloc(1, sizeof(Monitor));
    m->tagset[0] = m->tagset[1] = 1;
    m->mfact = mfact;
    m->nmaster = nmaster;
    m->showbar = showbar;
    m->topbar = topbar;
    m->lt[0] = &layouts[0];
    m->lt[1] = &layouts[1 % LENGTH(layouts)];
    strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);
    m->pertag = ecalloc(1, sizeof(Pertag));
    m->pertag->curtag = m->pertag->prevtag = 1;
    m->isoverview = 0;

    for (i = 0; i <= LENGTH(tags); i++) {
        m->pertag->nmasters[i] = m->nmaster;
        m->pertag->mfacts[i] = m->mfact;

        m->pertag->ltidxs[i][0] = m->lt[0];
        m->pertag->ltidxs[i][1] = m->lt[1];
        m->pertag->sellts[i] = m->sellt;

        m->pertag->showbars[i] = m->showbar;
    }

    return m;
}

void
destroynotify(XEvent *e)
{
    Client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    if ((c = wintoclient(ev->window)))
        unmanage(c, 1);
    else if ((c = wintosystrayicon(ev->window))) {
        removesystrayicon(c);
        resizebarwin(selmon);
        updatesystray();
    }
}

void
detach(Client *c)
{
    Client **tc;

    for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next);
    *tc = c->next;
}

void
detachstack(Client *c)
{
    Client **tc, *t;

    for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext);
    *tc = c->snext;

    if (c == c->mon->sel) {
        for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
        c->mon->sel = t;
    }
}

Monitor *
dirtomon(int dir)
{
    Monitor *m = NULL;

    if (dir > 0) {
        if (!(m = selmon->next))
            m = mons;
    } else if (selmon == mons)
        for (m = mons; m->next; m = m->next);
    else
        for (m = mons; m->next != selmon; m = m->next);
    return m;
}

void
drawbar(Monitor *m)
{
    int x, empty_w;
    int w = 0;
    int system_w = 0, tasks_w = 0, status_w;
    unsigned int i, occ = 0, n = 0, urg = 0, scm;
    Client *c;
	int boxw = 2;

    if (!m->showbar)
        return;

    // 获取系统托盘的宽度
    if(showsystray && m == systraytomon(m))
        system_w = getsystraywidth();

    // 绘制STATUSBAR
    status_w = drawstatusbar(m, bh, stext);

    // 未知
    for (c = m->clients; c; c = c->next) {
        if (ISVISIBLE(c))
            n++;
        occ |= c->tags == TAGMASK ? 0 : c->tags;
        if (c->isurgent)
            urg |= c->tags;
    }

    // 绘制TAGS
    x = 0;

    // 代表为overview tag状态
    if (m->isoverview) {
        w = TEXTW(overviewtag);
        drw_setscheme(drw, scheme[SchemeSelTag]);
        drw_text(drw, x, 0, w, bh, lrpad / 2, overviewtag, 0);
        drw_setscheme(drw, scheme[SchemeUnderline]);
        drw_rect(drw, x, bh - boxw, w + lrpad, boxw, 1, 0);
        x += w;
    } else {
        for (i = 0; i < LENGTH(tags); i++) {
            /* do not draw vacant tags */
            if (!(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
                continue;

            w = TEXTW(tags[i]);
            drw_setscheme(drw, scheme[m->tagset[m->seltags] & 1 << i ? SchemeSelTag : SchemeNormTag]);
            drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], urg & 1 << i);
            if (m->tagset[m->seltags] & 1 << i) {
                drw_setscheme(drw, scheme[SchemeUnderline]);
                drw_rect(drw, x + 2, bh - boxw, w + lrpad - 4, boxw, 1, 0);
            }
            x += w;
        }
    }

    // 绘制模式图标
    w = TEXTW(overviewlayout.symbol);
    drw_setscheme(drw, scheme[SchemeNorm]);
    x = drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0);

    // 绘制TASKS
    for (c = m->clients; c; c = c->next) {
        // 判断是否需要绘制 && 判断颜色设置
        if (!ISVISIBLE(c))
            continue;
        if (m->sel == c)
            scm = SchemeSel;
        else if (HIDDEN(c))
            scm = SchemeHid;
        else
            scm = SchemeNorm;
        drw_setscheme(drw, scheme[scm]);

        // 绘制TASK
        w = MIN(TEXTW(c->name), TEXTW("          "));
        empty_w = m->ww - x - status_w - system_w;
        if (w > empty_w) { // 如果当前TASK绘制后长度超过最大宽度
            w = empty_w;
            x = drw_text(drw, x, 0, w, bh, lrpad / 2, "...", 0);
            c->taskw = w;
            tasks_w += w;
            break;
        } else {
            x = drw_text(drw, x, 0, w, bh, lrpad / 2, c->name, 0);
            c->taskw = w;
            tasks_w += w;
        }
    }
    empty_w = m->ww - x - status_w - system_w; // 最后多加了一个w
    if (empty_w > 0) {
        drw_setscheme(drw, scheme[SchemeBarEmpty]);
        drw_rect(drw, x, 0, empty_w, bh, 1, 1);
    }

    m->bt = n;
    drw_map(drw, m->barwin, 0, 0, m->ww - system_w, bh);

    resizebarwin(m);
}

void
drawbars(void)
{
    Monitor *m;

    for (m = mons; m; m = m->next)
        drawbar(m);
}

int
drawstatusbar(Monitor *m, int bh, char* stext) {
    int status_w = 0, i, w, x, len, system_w = 0;
    short isCode = 0;
    char *text;
    char *p;
    char buf8[8], buf5[5];
    uint textsalpha;

    if(showsystray && m == systraytomon(m))
        system_w = getsystraywidth();

    len = strlen(stext) + 1 ;
    if (!(text = (char*) malloc(sizeof(char)*len)))
        die("malloc");
    p = text;
    memcpy(text, stext, len);

    /* compute width of the status text */
    w = 0;
    i = -1;
    while (text[++i]) {
        if (text[i] == '^') {
            if (!isCode) {
                isCode = 1;
                text[i] = '\0';
                w += TEXTW(text) - lrpad;
                text[i] = '^';
            } else {
                isCode = 0;
                text = text + i + 1;
                i = -1;
            }
        }
    }
    if (!isCode)
        w += TEXTW(text) - lrpad;
    else
        isCode = 0;
    text = p;

    // w += 2; /* 1px padding on both sides */
    x = m->ww - w - system_w;

    drw_setscheme(drw, scheme[LENGTH(colors)]);
    drw->scheme[ColFg] = scheme[SchemeNorm][ColFg];
    drw->scheme[ColBg] = scheme[SchemeNorm][ColBg];
    drw_rect(drw, x, 0, w, bh, 1, 1);
    x++;

    /* process status text */
    i = -1;
    while (text[++i]) {
        if (text[i] == '^' && !isCode) {
            isCode = 1;

            text[i] = '\0';
            w = TEXTW(text) - lrpad;
            drw_text(drw, x, 0, w, bh, 0, text, 0);
            status_w += w;

            x += w;

            /* process code */
            while (text[++i] != '^') {
                if (text[i] == 'c') {
                    memcpy(buf8, (char*)text+i+1, 7);
                    buf8[7] = '\0';
                    i += 7;

                    textsalpha = alphas[SchemeStatusText][ColFg];
                    if (text[i + 1] != '^') {
                        memcpy(buf5, (char*)text+i+1, 4);
                        buf5[4] = '\0';
                        i += 4;
                        sscanf(buf5, "%x", &textsalpha);
                    }

                    drw_clr_create(drw, &drw->scheme[ColFg], buf8, textsalpha);
                } else if (text[i] == 'b') {
                    memcpy(buf8, (char*)text+i+1, 7);
                    buf8[7] = '\0';
                    i += 7;

                    textsalpha = alphas[SchemeStatusText][ColBg];
                    if (text[i + 1] != '^') {
                        memcpy(buf5, (char*)text+i+1, 4);
                        buf5[4] = '\0';
                        i += 4;
                        sscanf(buf5, "%x", &textsalpha);
                    }
                    drw_clr_create(drw, &drw->scheme[ColBg], buf8, textsalpha);
                } else if (text[i] == 's') {
                    while (text[i + 1] != '^') i++;
                } else if (text[i] == 'd') {
                    drw->scheme[ColFg] = scheme[SchemeNorm][ColFg];
                    drw->scheme[ColBg] = scheme[SchemeNorm][ColBg];
                }
            }

            text = text + i + 1;
            i=-1;
            isCode = 0;
        }
    }

    if (!isCode) {
        w = TEXTW(text) - lrpad;
        drw_text(drw, x, 0, w, bh, 0, text, 0);
        status_w += w;
    }

    drw_setscheme(drw, scheme[SchemeNorm]);
    free(p);

    return status_w - 2;
}

// 点击状态栏时执行的func
// 传入参数为 i  => 鼠标点击的位置相对于左边界的距离
// 传入参数为 ui => 鼠标按键, 1 => 左键, 2 => 中键, 3 => 右键, 4 => 滚轮向上, 5 => 滚轮向下
long lastclickstatusbartime = 0; // 用于避免过于频繁的点击和滚轮行为
void
clickstatusbar(const Arg *arg)
{
    if (!arg->i && arg->i < 0)
        return;

    // 用于避免过于频繁的点击和滚轮行为
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long now = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    if (now - lastclickstatusbartime < 100)
        return;
    lastclickstatusbartime = now;

    int offset = -1;
    int status_w = 0;
    int iscode = 0, issignal = 0, signalindex = 0;
    char signal [20];
    char text [100];
    char *button = "L";
    int limit, max = sizeof(stext);


    while (stext[++offset] != '\0') {
        // 左侧^
        if (stext[offset] == '^' && !iscode) {
            iscode = 1;
            offset++;
            if (stext[offset] == 's') { // 查询到s->signal
                issignal = 1;
                signalindex = 0;
                memset(signal, '\0', sizeof(signal));
            } else {
                issignal = 0;
            }
            continue;
        }

        // 右侧^
        if (stext[offset] == '^' && iscode) {
            iscode = 0;
            issignal = 0;
            continue;
        }

        if (issignal) { // 逐位读取signal
            signal[signalindex++] = stext[offset];
        }

        // 是普通文本
        if (!iscode) {
            // 查找到下一个^ 或 游标到达最后
            limit = 0;
            while (stext[offset + ++limit] != '^' && offset + limit < max);
            if (offset + limit == max)
                break;

            memset(text, '\0', sizeof(text));
            strncpy(text, stext + offset, limit);
            offset += --limit;
            status_w += TEXTW(text) - lrpad;
            if (status_w > arg->i)
                break;
        }
    }

    switch (arg->ui) {
        case Button1: button = "L"; break;
        case Button2: button = "M"; break;
        case Button3: button = "R"; break;
        case Button4: button = "U"; break;
        case Button5: button = "D"; break;
    }

    memset(text, '\0', sizeof(text));
    sprintf(text, "%s %s %s &", statusbarscript, signal, button);
    system(text);
}

void
enternotify(XEvent *e)
{
    Client *c;
    Monitor *m;
    XCrossingEvent *ev = &e->xcrossing;

    if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
        return;
    c = wintoclient(ev->window);
    m = c ? c->mon : wintomon(ev->window);
    if (m != selmon) {
        unfocus(selmon->sel, 1);
        selmon = m;
    } else if (!c || c == selmon->sel)
        return;
    focus(c);
}

void
expose(XEvent *e)
{
    Monitor *m;
    XExposeEvent *ev = &e->xexpose;

    if (ev->count == 0 && (m = wintomon(ev->window))) {
        drawbar(m);
        if (m == selmon)
            updatesystray();
    }
}

void
focus(Client *c)
{
    if (!c || !ISVISIBLE(c) || HIDDEN(c))
        for (c = selmon->stack; c && (!ISVISIBLE(c) || HIDDEN(c)); c = c->snext);
    if (selmon->sel && selmon->sel != c)
        unfocus(selmon->sel, 0);
    if (c) {
        if (c->mon != selmon)
            selmon = c->mon;
        if (c->isurgent)
            seturgent(c, 0);
        detachstack(c);
        attachstack(c);
        grabbuttons(c, 1);
        XSetWindowBorder(dpy, c->win, scheme[c->isglobal ? SchemeSelGlobal : SchemeSel][ColBorder].pixel);
        setfocus(c);
    } else {
        XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
        XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
    }
    selmon->sel = c;
    drawbars();
}

/* there are some broken focus acquiring clients needing extra handling */
void
focusin(XEvent *e)
{
    XFocusChangeEvent *ev = &e->xfocus;

    if (selmon->sel && ev->window != selmon->sel->win)
        setfocus(selmon->sel);
}

void
focusmon(const Arg *arg)
{
    Monitor *m;

    if (!mons->next)
        return;
    if ((m = dirtomon(arg->i)) == selmon)
        return;
    unfocus(selmon->sel, 0);
    selmon = m;
    focus(NULL);
    pointerfocuswin(NULL);
}

void
focusstack(const Arg *arg)
{
    Client *tempClients[100];
    Client *c = NULL, *tc = selmon->sel;
    int last = -1, cur = 0, issingle = issinglewin(NULL), hasfloating = 0;

    if (tc && tc->isfullscreen) /* no support for focusstack with fullscreen windows */
        return;
    if (!tc)
        tc = selmon->clients;
    if (!tc)
        return;

    // 判断是否有浮动窗口
    for (c = selmon->clients; c && !hasfloating; c = c->next)
        if (ISVISIBLE(c) && !HIDDEN(c) && c->isfloating)
            hasfloating = 1;

    for (c = selmon->clients; c; c = c->next) {
        if (hasfloating != c->isfloating) continue; // 如果有浮动窗口，只在浮动窗口中切换

        if (ISVISIBLE(c) && (issingle || !HIDDEN(c))) {
            last ++;
            tempClients[last] = c;
            if (c == tc) cur = last;
        }
    }
    if (last < 0) return;

    if (arg && arg->i == -1) {
        if (cur - 1 >= 0) c = tempClients[cur - 1];
        else c = tempClients[last];
    } else {
        if (cur + 1 <= last) c = tempClients[cur + 1];
        else c = tempClients[0];
    }

    if (issingle) {
        if (c)
            hideotherwins(&(Arg) { .v = c });
    } else {
        if (c) {
            pointerfocuswin(c);
            restack(selmon);
        }
    }
}

void
pointerfocuswin(Client *c)
{
    if (c) {
        XWarpPointer(dpy, None, root, 0, 0, 0, 0, c->x + c->w / 2, c->y + c->h / 2);
        focus(c);
    } else
        XWarpPointer(dpy, None, root, 0, 0, 0, 0, selmon->wx + selmon->ww / 3, selmon->wy + selmon->wh / 2);
}

Atom
getatomprop(Client *c, Atom prop)
{
    int di;
    unsigned long dl;
    unsigned char *p = NULL;
    Atom da, atom = None;
    /* FIXME getatomprop should return the number of items and a pointer to
     * the stored data instead of this workaround */
    Atom req = XA_ATOM;
    if (prop == xatom[XembedInfo])
        req = xatom[XembedInfo];

    if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, req,
                &da, &di, &dl, &dl, &p) == Success && p) {
        atom = *(Atom *)p;
        if (da == xatom[XembedInfo] && dl == 2)
            atom = ((Atom *)p)[1];
        XFree(p);
    }
    return atom;
}

int
getrootptr(int *x, int *y)
{
    int di;
    unsigned int dui;
    Window dummy;

    return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long
getstate(Window w)
{
    int format;
    long result = -1;
    unsigned char *p = NULL;
    unsigned long n, extra;
    Atom real;

    if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
                &real, &format, &n, &extra, (unsigned char **)&p) != Success)
        return -1;
    if (n != 0)
        result = *p;
    XFree(p);
    return result;
}

unsigned int
getsystraywidth()
{
    unsigned int w = 0;
    Client *i;
    if(showsystray)
        for(i = systray->icons; i; w += MAX(i->w, bh) + systrayspacing, i = i->next) ;
    return w ? w + systrayspacing : 1;
}

int
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
    char **list = NULL;
    int n;
    XTextProperty name;

    if (!text || size == 0)
        return 0;
    text[0] = '\0';
    if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
        return 0;
    if (name.encoding == XA_STRING)
        strncpy(text, (char *)name.value, size - 1);
    else {
        if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
            strncpy(text, *list, size - 1);
            XFreeStringList(list);
        }
    }
    text[size - 1] = '\0';
    XFree(name.value);
    return 1;
}

void
grabbuttons(Client *c, int focused)
{
    updatenumlockmask();
    {
        unsigned int i, j;
        unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
        XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
        if (!focused)
            XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
                    BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
        for (i = 0; i < LENGTH(buttons); i++)
            if (buttons[i].click == ClkClientWin)
                for (j = 0; j < LENGTH(modifiers); j++)
                    XGrabButton(dpy, buttons[i].button,
                            buttons[i].mask | modifiers[j],
                            c->win, False, BUTTONMASK,
                            GrabModeAsync, GrabModeSync, None, None);
    }
}

void
grabkeys(void)
{
    updatenumlockmask();
    {
        unsigned int i, j;
        unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
        KeyCode code;

        XUngrabKey(dpy, AnyKey, AnyModifier, root);
        for (i = 0; i < LENGTH(keys); i++)
            if ((code = XKeysymToKeycode(dpy, keys[i].keysym)))
                for (j = 0; j < LENGTH(modifiers); j++)
                    XGrabKey(dpy, code, keys[i].mod | modifiers[j], root,
                            True, GrabModeAsync, GrabModeAsync);
    }
}

void
hide(Client *c) {
    if (!c || HIDDEN(c))
        return;

    Window w = c->win;
    static XWindowAttributes ra, ca;

    // more or less taken directly from blackbox's hide() function
    XGrabServer(dpy);
    XGetWindowAttributes(dpy, root, &ra);
    XGetWindowAttributes(dpy, w, &ca);
    // prevent UnmapNotify events
    XSelectInput(dpy, root, ra.your_event_mask & ~SubstructureNotifyMask);
    XSelectInput(dpy, w, ca.your_event_mask & ~StructureNotifyMask);
    XUnmapWindow(dpy, w);
    setclientstate(c, IconicState);
    XSelectInput(dpy, root, ra.your_event_mask);
    XSelectInput(dpy, w, ca.your_event_mask);
    XUngrabServer(dpy);

    hiddenWinStack[++hiddenWinStackTop] = c;
    focus(c->snext);
    arrange(c->mon);
}

void
hideotherwins(const Arg *arg) {
    Client *c = (Client*)arg->v, *tc = NULL;
    for (tc = selmon->clients; tc; tc = tc->next)
        if (tc != c && ISVISIBLE(tc))
            hide(tc);
    show(c);
    focus(c);
}

void
showonlyorall(const Arg *arg) {
    Client *c;
    if (issinglewin(NULL) || !selmon->sel) {
        for (c = selmon->clients; c; c = c->next)
            if (ISVISIBLE(c))
                show(c);
    } else
        hideotherwins(&(Arg) { .v = selmon->sel });
}


void
incnmaster(const Arg *arg)
{
    int nmaster = selmon->nmaster + arg->i;
    if (selmon->bt <= 1)
        nmaster = 1;
    else if (nmaster >= 3)
        nmaster = 1;
    selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag] = MAX(nmaster, 1);
    arrange(selmon);
}

#ifdef XINERAMA
static int
isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info)
{
    while (n--)
        if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
                && unique[n].width == info->width && unique[n].height == info->height)
            return 0;
    return 1;
}
#endif /* XINERAMA */

void
keypress(XEvent *e)
{
    unsigned int i;
    KeySym keysym;
    XKeyEvent *ev;

    ev = &e->xkey;
    keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
    for (i = 0; i < LENGTH(keys); i++)
        if (keysym == keys[i].keysym
                && CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
                && keys[i].func)
            keys[i].func(&(keys[i].arg));
}

void
killclient(const Arg *arg)
{
    Client *c;
    int n = 0;

    if (!selmon->sel)
        return;
    if (!sendevent(selmon->sel->win, wmatom[WMDelete], NoEventMask, wmatom[WMDelete], CurrentTime, 0 , 0, 0)) {
        XGrabServer(dpy);
        XSetErrorHandler(xerrordummy);
        XSetCloseDownMode(dpy, DestroyAll);
        XKillClient(dpy, selmon->sel->win);
        XSync(dpy, False);
        XSetErrorHandler(xerror);
        XUngrabServer(dpy);
    }
    for (c = selmon->clients; c; c = c->next)
        if (ISVISIBLE(c) && !HIDDEN(c))
            n++;
    if (n <= 1)
        focusstack(NULL);
}

void
forcekillclient(const Arg *arg)
{
    if (!selmon->sel)
        return;
    killclient(arg);
    unmanage(selmon->sel, 1);
}


void
managefloating(Client *c)
{
    Client *tc;
    int d1 = 0, d2 = 0, tx, ty;
    int tryed = 0;
    while (tryed++ < 10) {
        int dw, dh, existed = 0;
        dw = (selmon->ww / 20) * d1, dh = (selmon->wh / 20) * d2;
        tx = c->x + dw, ty = c->y + dh;
        for (tc = selmon->clients; tc; tc = tc->next) {
            if (ISVISIBLE(tc) && !HIDDEN(tc) && tc != c && tc->x == tx && tc->y == ty) {
                existed = 1;
                break;
            }
        }
        if (!existed) {
            c->x = tx;
            c->y = ty;
            break;
        } else {
            while (d1 == 0) d1 = rand()%7 - 3;
            while (d2 == 0) d2 = rand()%7 - 3;
        }
    }
}

void
manage(Window w, XWindowAttributes *wa)
{
    Client *c, *t = NULL;
    Window trans = None;
    XWindowChanges wc;

    c = ecalloc(1, sizeof(Client));
    c->win = w;
    /* geometry */
    c->x = c->oldx = wa->x;
    c->y = c->oldy = wa->y;
    c->w = c->oldw = wa->width;
    c->h = c->oldh = wa->height;
    c->oldbw = wa->border_width;
    c->bw = borderpx;

    updatetitle(c);
    if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
        c->mon = t->mon;
        c->tags = t->tags;
    } else {
        c->mon = selmon;
        applyrules(c);
    }
    wc.border_width = c->bw;

    if (c->x + WIDTH(c) > c->mon->mx + c->mon->mw)
        c->x = c->mon->mx + c->mon->mw - WIDTH(c);
    if (c->y + HEIGHT(c) > c->mon->my + c->mon->mh)
        c->y = c->mon->my + c->mon->mh - HEIGHT(c);
    c->x = MAX(c->x, c->mon->mx);
    /* only fix client y-offset, if the client center might cover the bar */
    c->y = MAX(c->y, ((c->mon->by == c->mon->my) && (c->x + (c->w / 2) >= c->mon->wx)
                && (c->x + (c->w / 2) < c->mon->wx + c->mon->ww)) ? bh : c->mon->my);

    if (c->isfloating) {
        if (wa->x==0 && wa->y==0) {
            c->x = selmon->wx + (selmon->ww - c->w) / 2;
            c->y = selmon->wy + (selmon->wh - c->h) / 2;
        }
        managefloating(c);
    }

    XConfigureWindow(dpy, w, CWBorderWidth, &wc);
    XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
    configure(c); /* propagates border_width, if size doesn't change */
    updatewindowtype(c);
    updatesizehints(c);
    updatewmhints(c);
    XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
    grabbuttons(c, 0);
    if (!c->isfloating)
        c->isfloating = c->oldstate = trans != None || c->isfixed;
    if (c->isfloating)
        XRaiseWindow(dpy, c->win);
    attach(c);
    attachstack(c);
    XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
            (unsigned char *) &(c->win), 1);
    XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
    if (!HIDDEN(c))
        setclientstate(c, NormalState);
    if (c->mon == selmon)
        unfocus(selmon->sel, 0);
    c->mon->sel = c;
    arrange(c->mon);
    if (!HIDDEN(c))
        XMapWindow(dpy, c->win);
    focus(NULL);
}

void
mappingnotify(XEvent *e)
{
    XMappingEvent *ev = &e->xmapping;

    XRefreshKeyboardMapping(ev);
    if (ev->request == MappingKeyboard)
        grabkeys();
}

void
maprequest(XEvent *e)
{
    static XWindowAttributes wa;
    XMapRequestEvent *ev = &e->xmaprequest;
    Client *i;
    if ((i = wintosystrayicon(ev->window))) {
        sendevent(i->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0, systray->win, XEMBED_EMBEDDED_VERSION);
        resizebarwin(selmon);
        updatesystray();
    }

    if (!XGetWindowAttributes(dpy, ev->window, &wa))
        return;
    if (wa.override_redirect)
        return;
    if (!wintoclient(ev->window))
        manage(ev->window, &wa);
}

void
motionnotify(XEvent *e)
{
    static Monitor *mon = NULL;
    Monitor *m;
    XMotionEvent *ev = &e->xmotion;

    if (ev->window != root)
        return;
    if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
        unfocus(selmon->sel, 1);
        selmon = m;
        focus(NULL);
    }
    mon = m;
}

void
movemouse(const Arg *arg)
{
    int x, y, ocx, ocy, nx, ny;
    Client *c;
    Monitor *m;
    XEvent ev;
    Time lasttime = 0;

    if (!(c = selmon->sel))
        return;
    if (c->isfullscreen) /* no support moving fullscreen windows by mouse */
        return;
    restack(selmon);
    ocx = c->x;
    ocy = c->y;
    if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
        return;
    if (!getrootptr(&x, &y))
        return;
    do {
        XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
        switch(ev.type) {
            case ConfigureRequest:
            case Expose:
            case MapRequest:
                handler[ev.type](&ev);
                break;
            case MotionNotify:
                if ((ev.xmotion.time - lasttime) <= (1000 / 120))
                    continue;
                lasttime = ev.xmotion.time;

                nx = ocx + (ev.xmotion.x - x);
                ny = ocy + (ev.xmotion.y - y);
                if (abs(selmon->wx - nx) < snap)
                    nx = selmon->wx;
                else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
                    nx = selmon->wx + selmon->ww - WIDTH(c);
                if (abs(selmon->wy - ny) < snap)
                    ny = selmon->wy;
                else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
                    ny = selmon->wy + selmon->wh - HEIGHT(c);
                if (!c->isfloating && (abs(nx - c->x) > snap || abs(ny - c->y) > snap)) {
                    c->isfloating = 1;
                    arrange(selmon);
                    if (ev.xmotion.x - nx < c->w / 2 && ev.xmotion.y - ny < c->h / 2 && (c->w > selmon->ww * 0.5 || c->h > selmon->wh * 0.5)) {
                        resize(c, nx, ny, c->w > selmon->ww * 0.5 ? c->w / 2 : c->w, c->h > selmon->wh * 0.5 ? c->h / 2 : c->h, 0);
                        break;
                    }
                }
                if (c->isfloating)
                    resize(c, nx, ny, c->w, c->h, 1);
                break;
        }
    } while (ev.type != ButtonRelease);
    XUngrabPointer(dpy, CurrentTime);
    if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
        sendmon(c, m);
        selmon = m;
        focus(NULL);
    }
}

void
movewin(const Arg *arg)
{
    Client *c, *tc;
    int nx, ny;
    int buttom, top, left, right, tar;
    c = selmon->sel;
    if (!c || c->isfullscreen)
        return;
    if (!c->isfloating)
        togglefloating(NULL);
    nx = c->x;
    ny = c->y;
    switch (arg->ui) {
        case UP:
            tar = -99999;
            top = c->y;
            ny -= c->mon->wh / 4;
            for (tc = c->mon->clients; tc; tc = tc->next) {
                // 若浮动tc c的顶边会穿过tc的底边 
                if (!ISVISIBLE(tc) || !tc->isfloating || tc == c) continue;
                if (c->x + WIDTH(c) < tc->x || c->x > tc->x + WIDTH(tc)) continue;
                buttom = tc->y + HEIGHT(tc) + gappi;  
                if (top > buttom && ny < buttom) {  
                    tar = MAX(tar, buttom);
                };
            }
            ny = tar == -99999 ? ny : tar;
            ny = MAX(ny, c->mon->wy + gappo);
            break;
        case DOWN:
            tar = 99999;
            buttom = c->y + HEIGHT(c);
            ny += c->mon->wh / 4;
            for (tc = c->mon->clients; tc; tc = tc->next) {
                // 若浮动tc c的底边会穿过tc的顶边 
                if (!ISVISIBLE(tc) || !tc->isfloating || tc == c) continue;
                if (c->x + WIDTH(c) < tc->x || c->x > tc->x + WIDTH(tc)) continue;
                top = tc->y - gappi;
                if (buttom < top && (ny + HEIGHT(c)) > top) {  
                    tar = MIN(tar, top - HEIGHT(c));
                };
            }
            ny = tar == 99999 ? ny : tar;
            ny = MIN(ny, c->mon->wy + c->mon->wh - gappo - HEIGHT(c));
            break;
        case LEFT:
            tar = -99999;
            left = c->x;
            nx -= c->mon->ww / 6;
            for (tc = c->mon->clients; tc; tc = tc->next) {
                // 若浮动tc c的左边会穿过tc的右边 
                if (!ISVISIBLE(tc) || !tc->isfloating || tc == c) continue;
                if (c->y + HEIGHT(c) < tc->y || c->y > tc->y + HEIGHT(tc)) continue;
                right = tc->x + WIDTH(tc) + gappi;
                if (left > right && nx < right) {
                    tar = MAX(tar, right);
                };
            }
            nx = tar == -99999 ? nx : tar;
            nx = MAX(nx, c->mon->wx + gappo);
            break;
        case RIGHT:
            tar = 99999;
            right = c->x + WIDTH(c);
            nx += c->mon->ww / 6;
            for (tc = c->mon->clients; tc; tc = tc->next) {
                // 若浮动tc c的右边会穿过tc的左边 
                if (!ISVISIBLE(tc) || !tc->isfloating || tc == c) continue;
                if (c->y + HEIGHT(c) < tc->y || c->y > tc->y + HEIGHT(tc)) continue;
                left = tc->x - gappi;
                if (right < left && (nx + WIDTH(c)) > left) {
                    tar = MIN(tar, left - WIDTH(c));
                };
            }
            nx = tar == 99999 ? nx : tar;
            nx = MIN(nx, c->mon->wx + c->mon->ww - gappo - WIDTH(c));
            break;
    }
    resize(c, nx, ny, c->w, c->h, 1);
    pointerfocuswin(c);
    restack(selmon);
}

void
resizewin(const Arg *arg)
{
    Client *c, *tc;
    int nh, nw;
    int buttom, top, left, right, tar;
    c = selmon->sel;
    if (!c || c->isfullscreen)
        return;
    if (!c->isfloating)
        togglefloating(NULL);
    nw = c->w;
    nh = c->h;
    switch (arg->ui) {
        case H_EXPAND: // 右
            tar = 99999;
            right = c->x + WIDTH(c);
            nw += selmon->ww / 16;
            for (tc = c->mon->clients; tc; tc = tc->next) {
                // 若浮动tc c的右边会穿过tc的左边 
                if (!ISVISIBLE(tc) || !tc->isfloating || tc == c) continue;
                if (c->y + HEIGHT(c) < tc->y || c->y > tc->y + HEIGHT(tc)) continue;
                left = tc->x - gappi;
                if (right < left && (c->x + nw) > left) {
                    tar = MIN(tar, left - c->x - 2 * c->bw);
                };
            }
            nw = tar == 99999 ? nw : tar;
            if (c->x + nw + gappo + 2 * c->bw > selmon->wx + selmon->ww)
                nw = selmon->wx + selmon->ww - c->x - gappo - 2 * c->bw;
            break;
        case H_REDUCE: // 左
            nw -= selmon->ww / 16;
            nw = MAX(nw, selmon->ww / 10);
            break;
        case V_EXPAND: // 下
            tar = -99999;
            buttom = c->y + HEIGHT(c);
            nh += selmon->wh / 8;
            for (tc = c->mon->clients; tc; tc = tc->next) {
                // 若浮动tc c的底边会穿过tc的顶边 
                if (!ISVISIBLE(tc) || !tc->isfloating || tc == c) continue;
                if (c->x + WIDTH(c) < tc->x || c->x > tc->x + WIDTH(tc)) continue;
                top = tc->y - gappi;
                if (buttom < top && (c->y + nh) > top) {  
                    tar = MAX(tar, top - c->y - 2 * c->bw);
                };
            }
            nh = tar == -99999 ? nh : tar;
            if (c->y + nh + gappo + 2 * c->bw > selmon->wy + selmon->wh)
                nh = selmon->wy + selmon->wh - c->y - gappo - 2 * c->bw;
            break;
        case V_REDUCE: // 上
            nh -= selmon->wh / 8;
            nh = MAX(nh, selmon->wh / 10);
            break;
    }
    resize(c, c->x, c->y, nw, nh, 1);
    XWarpPointer(dpy, None, root, 0, 0, 0, 0, c->x + c->w - 2 * c->bw, c->y + c->h - 2 * c->bw);
    restack(selmon);
}

Client *
nexttiled(Client *c)
{
    for (; c && (c->isfloating || !ISVISIBLE(c) || HIDDEN(c)); c = c->next);
    return c;
}

void
pop(Client *c)
{
	detach(c);
    c->next = c->mon->clients;
    c->mon->clients = c;
	focus(c);
	arrange(c->mon);
    pointerfocuswin(c);
}

void
propertynotify(XEvent *e)
{
    Client *c;
    Window trans;
    XPropertyEvent *ev = &e->xproperty;

    if ((c = wintosystrayicon(ev->window))) {
        if (ev->atom == XA_WM_NORMAL_HINTS) {
            updatesizehints(c);
            updatesystrayicongeom(c, c->w, c->h);
        }
        else
            updatesystrayiconstate(c, ev);
        resizebarwin(selmon);
        updatesystray();
    }
    if ((ev->window == root) && (ev->atom == XA_WM_NAME))
        updatestatus();
    else if (ev->state == PropertyDelete)
        return; /* ignore */
    else if ((c = wintoclient(ev->window))) {
        switch(ev->atom) {
            default: break;
            case XA_WM_TRANSIENT_FOR:
                     if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
                             (c->isfloating = (wintoclient(trans)) != NULL))
                         arrange(c->mon);
                     break;
            case XA_WM_NORMAL_HINTS:
                     updatesizehints(c);
                     break;
            case XA_WM_HINTS:
                     updatewmhints(c);
                     drawbars();
                     break;
        }
        if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
            updatetitle(c);
            if (c == c->mon->sel)
                drawbar(c->mon);
        }
        if (ev->atom == netatom[NetWMWindowType])
            updatewindowtype(c);
    }
}

void
quit(const Arg *arg)
{
    running = 0;
}

Monitor *
recttomon(int x, int y, int w, int h)
{
    Monitor *m, *r = selmon;
    int a, area = 0;

    for (m = mons; m; m = m->next)
        if ((a = INTERSECT(x, y, w, h, m)) > area) {
            area = a;
            r = m;
        }
    return r;
}

void
removesystrayicon(Client *i)
{
    Client **ii;

    if (!showsystray || !i)
        return;
    for (ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next);
    if (ii)
        *ii = i->next;
    free(i);
}


void
resize(Client *c, int x, int y, int w, int h, int interact)
{
    if (applysizehints(c, &x, &y, &w, &h, interact)) {
        resizeclient(c, x, y, w, h);
    }
}

void
resizebarwin(Monitor *m) {
    unsigned int w = m->ww;
    if (showsystray && m == systraytomon(m))
        w -= getsystraywidth();
    XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, w, bh);
}

void
resizeclient(Client *c, int x, int y, int w, int h)
{
    XWindowChanges wc;

    c->oldx = c->x; c->x = wc.x = x;
    c->oldy = c->y; c->y = wc.y = y;
    c->oldw = c->w; c->w = wc.width = w;
    c->oldh = c->h; c->h = wc.height = h;
    wc.border_width = c->bw;

    if (((nexttiled(c->mon->clients) == c && !nexttiled(c->next)))
            && !c->isfullscreen && !c->isfloating) {
        c->w = wc.width += c->bw * 2;
        c->h = wc.height += c->bw * 2;
        wc.border_width = 0;
    }
    XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
    configure(c);
    XSync(dpy, False);
}

void
resizemouse(const Arg *arg)
{
    int ocx, ocy, nw, nh;
    int ocx2, ocy2, nx, ny;
    Client *c;
    Monitor *m;
    XEvent ev;
    int horizcorner, vertcorner;
	int di;
	unsigned int dui;
	Window dummy;
    Time lasttime = 0;

    if (!(c = selmon->sel))
        return;
    if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
        return;
    restack(selmon);
    ocx = c->x;
    ocy = c->y;
	ocx2 = c->x + c->w;
	ocy2 = c->y + c->h;
    if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
        return;
    if (!XQueryPointer(dpy, c->win, &dummy, &dummy, &di, &di, &nx, &ny, &dui))
        return;
    horizcorner = nx < c->w / 2;
    vertcorner = ny < c->h / 2;
	XWarpPointer (dpy, None, c->win, 0, 0, 0, 0,
			horizcorner ? (-c->bw) : (c->w + c->bw -1),
			vertcorner  ? (-c->bw) : (c->h + c->bw -1));
    do {
        XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
        switch(ev.type) {
            case ConfigureRequest:
            case Expose:
            case MapRequest:
                handler[ev.type](&ev);
                break;
            case MotionNotify:
                if ((ev.xmotion.time - lasttime) <= (1000 / 120))
                    continue;
                lasttime = ev.xmotion.time;

	    		nx = horizcorner ? ev.xmotion.x : c->x;
	    		ny = vertcorner ? ev.xmotion.y : c->y;
	    		nw = MAX(horizcorner ? (ocx2 - nx) : (ev.xmotion.x - ocx - 2 * c->bw + 1), 1);
	    		nh = MAX(vertcorner ? (ocy2 - ny) : (ev.xmotion.y - ocy - 2 * c->bw + 1), 1);
                if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
                        && c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh) {
                    if (!c->isfloating && (abs(nw - c->w) > snap || abs(nh - c->h) > snap)) {
                        c->isfloating = 1;
                        arrange(selmon);
                    }
                }
                if (c->isfloating)
                    resize(c, nx, ny, nw, nh, 1);
                break;
        }
    } while (ev.type != ButtonRelease);
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0,
		      horizcorner ? (-c->bw) : (c->w + c->bw - 1),
		      vertcorner ? (-c->bw) : (c->h + c->bw - 1));
    XUngrabPointer(dpy, CurrentTime);
    while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
    if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
        sendmon(c, m);
        selmon = m;
        focus(NULL);
    }
}

void
resizerequest(XEvent *e)
{
    XResizeRequestEvent *ev = &e->xresizerequest;
    Client *i;

    if ((i = wintosystrayicon(ev->window))) {
        updatesystrayicongeom(i, ev->width, ev->height);
        resizebarwin(selmon);
        updatesystray();
    }
}

void
restack(Monitor *m)
{
    Client *c;
    XEvent ev;
    XWindowChanges wc;

    drawbar(m);
    if (!m->sel)
        return;
    if (m->sel->isfloating)
        XRaiseWindow(dpy, m->sel->win);
    wc.stack_mode = Below;
    wc.sibling = m->barwin;
    for (c = m->stack; c; c = c->snext)
        if (!c->isfloating && ISVISIBLE(c)) {
            XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
            wc.sibling = c->win;
        }
    XSync(dpy, False);
    while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
run(void)
{
    XEvent ev;
    /* main event loop */
    XSync(dpy, False);
    while (running && !XNextEvent(dpy, &ev))
        if (handler[ev.type])
            handler[ev.type](&ev); /* call handler */
}

void
runAutostart(void) {
    char cmd [100];
    sprintf(cmd, "%s &", autostartscript);
    system(cmd);
}

void
scan(void)
{
    unsigned int i, num;
    Window d1, d2, *wins = NULL;
    XWindowAttributes wa;

    if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
        for (i = 0; i < num; i++) {
            if (!XGetWindowAttributes(dpy, wins[i], &wa)
                    || wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
                continue;
            if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
                manage(wins[i], &wa);
        }
        for (i = 0; i < num; i++) { /* now the transients */
            if (!XGetWindowAttributes(dpy, wins[i], &wa))
                continue;
            if (XGetTransientForHint(dpy, wins[i], &d1)
                    && (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
                manage(wins[i], &wa);
        }
        if (wins)
            XFree(wins);
    }
}

void
sendmon(Client *c, Monitor *m)
{
    if (c->mon == m)
        return;
    unfocus(c, 1);
    detach(c);
    detachstack(c);
    c->mon = m;
    c->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
    attach(c);
    attachstack(c);
    focus(NULL);
    arrange(NULL);
}

void
setclientstate(Client *c, long state)
{
    long data[] = { state, None };

    XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
            PropModeReplace, (unsigned char *)data, 2);
}

void
tagtoleft(const Arg *arg) {
    if(selmon->sel != NULL
            && __builtin_popcount(selmon->tagset[selmon->seltags] & TAGMASK) == 1
            && selmon->tagset[selmon->seltags] > 1) {
        tag(&(Arg) { .ui = selmon->tagset[selmon->seltags] >> 1 });
    }
}

void
tagtoright(const Arg *arg) {
    if(selmon->sel != NULL
            && __builtin_popcount(selmon->tagset[selmon->seltags] & TAGMASK) == 1
            && selmon->tagset[selmon->seltags] & (TAGMASK >> 1)) {
        tag(&(Arg) { .ui = selmon->tagset[selmon->seltags] << 1 });
    }
}

int
sendevent(Window w, Atom proto, int mask, long d0, long d1, long d2, long d3, long d4)
{
    int n;
    Atom *protocols, mt;
    int exists = 0;
    XEvent ev;

    if (proto == wmatom[WMTakeFocus] || proto == wmatom[WMDelete]) {
        mt = wmatom[WMProtocols];
        if (XGetWMProtocols(dpy, w, &protocols, &n)) {
            while (!exists && n--)
                exists = protocols[n] == proto;
            XFree(protocols);
        }
    }
    else {
        exists = True;
        mt = proto;
    }
    if (exists) {
        ev.type = ClientMessage;
        ev.xclient.window = w;
        ev.xclient.message_type = mt;
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = d0;
        ev.xclient.data.l[1] = d1;
        ev.xclient.data.l[2] = d2;
        ev.xclient.data.l[3] = d3;
        ev.xclient.data.l[4] = d4;
        XSendEvent(dpy, w, False, mask, &ev);
    }
    return exists;
}

void
setfocus(Client *c)
{
    if (!c->neverfocus) {
        XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
        XChangeProperty(dpy, root, netatom[NetActiveWindow],
                XA_WINDOW, 32, PropModeReplace,
                (unsigned char *) &(c->win), 1);
    }
    sendevent(c->win, wmatom[WMTakeFocus], NoEventMask, wmatom[WMTakeFocus], CurrentTime, 0, 0, 0);
}

void
setfullscreen(Client *c, int fullscreen)
{
    if (fullscreen && !c->isfullscreen) {
        XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
                PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
        c->isfullscreen = 1;
        c->oldstate = c->isfloating;
        c->oldbw = c->bw;
        c->bw = 0;
        c->isfloating = 1;
        resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
        XRaiseWindow(dpy, c->win);
    } else if (!fullscreen && c->isfullscreen){
        XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
                PropModeReplace, (unsigned char*)0, 0);
        c->isfullscreen = 0;
        c->isfloating = c->oldstate;
        c->bw = c->oldbw;
        c->x = c->oldx;
        c->y = c->oldy;
        c->w = c->oldw;
        c->h = c->oldh;
        resizeclient(c, c->x, c->y, c->w, c->h);
        arrange(c->mon);
    }
}

void
fullscreen(const Arg *arg)
{
    if (selmon->sel) {
        if (selmon->showbar)
            setfullscreen(selmon->sel, 1);
        else
            setfullscreen(selmon->sel, 0);
    }
    togglebar(arg);
}

void
selectlayout(const Arg *arg)
{
    const Layout *cur = selmon->lt[selmon->sellt];
    const Layout *target = cur == arg->v ? &layouts[0] : arg->v;
    setlayout(&(Arg) { .v = target });
}

void
setlayout(const Arg *arg)
{
    if (arg->v != selmon->lt[selmon->sellt])
        selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag] ^= 1;
    if (arg->v)
        selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt] = (Layout *)arg->v;
    arrange(selmon);
}

void
setmfact(const Arg *arg)
{
    float f;

    if (!arg)
        return;
    f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
    if (f < 0.05 || f > 0.95)
        return;
    selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag] = f;
    arrange(selmon);
}

void
setup(void)
{
    int i;
    XSetWindowAttributes wa;
    Atom utf8string;

    /* clean up any zombies immediately */
    sigchld(0);

    /* init screen */
    screen = DefaultScreen(dpy);
    sw = DisplayWidth(dpy, screen);
    sh = DisplayHeight(dpy, screen);
    root = RootWindow(dpy, screen);
    xinitvisual();
    drw = drw_create(dpy, screen, root, sw, sh, visual, depth, cmap);
    if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
        die("no fonts could be loaded.");
    lrpad = drw->fonts->h;
    bh = drw->fonts->h + 2;
    updategeom();
    /* init atoms */
    utf8string = XInternAtom(dpy, "UTF8_STRING", False);
    wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
    wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
    wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
    netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
    netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
    netatom[NetSystemTray] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
    netatom[NetSystemTrayOP] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
    netatom[NetSystemTrayOrientation] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION", False);
    netatom[NetSystemTrayOrientationHorz] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION_HORZ", False);
    netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
    netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
    netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
    netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
    netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
    xatom[Manager] = XInternAtom(dpy, "MANAGER", False);
    xatom[Xembed] = XInternAtom(dpy, "_XEMBED", False);
    xatom[XembedInfo] = XInternAtom(dpy, "_XEMBED_INFO", False);
    /* init cursors */
    cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
    cursor[CurResize] = drw_cur_create(drw, XC_sizing);
    cursor[CurMove] = drw_cur_create(drw, XC_fleur);
    /* init appearance */
    scheme = ecalloc(LENGTH(colors) + 1, sizeof(Clr *));
	scheme[LENGTH(colors)] = drw_scm_create(drw, colors[0], alphas[0], 3);
    for (i = 0; i < LENGTH(colors); i++)
        scheme[i] = drw_scm_create(drw, colors[i], alphas[i], 3);
    /* init system tray */
    updatesystray();
    /* init bars */
    updatebars();
    updatestatus();
    /* supporting window for NetWMCheck */
    wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
    XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
            PropModeReplace, (unsigned char *) &wmcheckwin, 1);
    XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
            PropModeReplace, (unsigned char *) "dwm", 3);
    XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
            PropModeReplace, (unsigned char *) &wmcheckwin, 1);
    /* EWMH support per view */
    XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
            PropModeReplace, (unsigned char *) netatom, NetLast);
    XDeleteProperty(dpy, root, netatom[NetClientList]);
    /* select events */
    wa.cursor = cursor[CurNormal]->cursor;
    wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
        |ButtonPressMask|PointerMotionMask|EnterWindowMask
        |LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
    XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
    XSelectInput(dpy, root, wa.event_mask);
    grabkeys();
    focus(NULL);
}


void
seturgent(Client *c, int urg)
{
    XWMHints *wmh;

    c->isurgent = urg;
    if (!(wmh = XGetWMHints(dpy, c->win)))
        return;
    wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
    XSetWMHints(dpy, c->win, wmh);
    XFree(wmh);
}

void
show(Client *c)
{
    if (!c || !HIDDEN(c))
        return;

    XMapWindow(dpy, c->win);
    setclientstate(c, NormalState);
    hiddenWinStackTop--;
    arrange(c->mon);
}

// 该方法为显示当前tag下的窗口的func，切换时会将原窗口下的win放到屏幕之外 (左边的屏幕隐藏到屏幕左边 右边的屏幕隐藏到屏幕右边)
void
showtag(Client *c)
{
    if (!c)
        return;
    if (ISVISIBLE(c)) {
        /** 将可见的client从屏幕边缘移动到屏幕内 */
        XMoveWindow(dpy, c->win, c->x, c->y);
        if (c->isfloating && !c->isfullscreen)
            resize(c, c->x, c->y, c->w, c->h, 0);
        showtag(c->snext);
    } else {
        /* 将不可见的client移动到屏幕之外 */
        showtag(c->snext);
        if (c->mon->mx == 0) {
            XMoveWindow(dpy, c->win, WIDTH(c) * -1.5, c->y);
        } else {
            XMoveWindow(dpy, c->win, c->mon->mx + c->mon->mw + WIDTH(c) * 1.5, c->y);
        }
    }
}

void
sigchld(int unused)
{
    if (signal(SIGCHLD, sigchld) == SIG_ERR)
        die("can't install SIGCHLD handler:");
    while (0 < waitpid(-1, NULL, WNOHANG));
}

void
spawn(const Arg *arg)
{
    if (fork() == 0) {
        if (dpy)
            close(ConnectionNumber(dpy));
        setsid();
        execvp(((char **)arg->v)[0], (char **)arg->v);
        fprintf(stderr, "dwm: execvp %s", ((char **)arg->v)[0]);
        perror(" failed");
        exit(EXIT_SUCCESS);
    }
}

void
tag(const Arg *arg)
{
    if (selmon->sel && !selmon->sel->isglobal && arg->ui & TAGMASK) {
        selmon->sel->tags = arg->ui & TAGMASK;
        focus(NULL);
        arrange(selmon);
        view(&(Arg) { .ui = arg->ui });
    } else
        view(arg);
}

void
tagmon(const Arg *arg)
{
    if (!selmon->sel || !mons->next)
        return;
    sendmon(selmon->sel, dirtomon(arg->i));
    focusmon(&(Arg) { .i = +1 });
    if (selmon->sel && selmon->sel->isfloating) {
        resize(selmon->sel, selmon->mx + (selmon->mw - selmon->sel->w) / 2, selmon->my + (selmon->mh - selmon->sel->h) / 2, selmon->sel->w, selmon->sel->h, 0);
    }
    pointerfocuswin(selmon->sel);
}

void
togglesystray()
{
    if (showsystray) {
        showsystray = 0;
        XUnmapWindow(dpy, systray->win);
    } else {
        showsystray = 1;
    }
    updatesystray();
    updatestatus();
}

void
togglebar(const Arg *arg)
{
    selmon->showbar = selmon->pertag->showbars[selmon->pertag->curtag] = !selmon->showbar;
    updatebarpos(selmon);
    resizebarwin(selmon);
    if (showsystray) {
        XWindowChanges wc;
        if (!selmon->showbar)
            wc.y = -bh;
        else if (selmon->showbar) {
            wc.y = 0;
            if (!selmon->topbar)
                wc.y = selmon->mh - bh;
        }
        XConfigureWindow(dpy, systray->win, CWY, &wc);
    }
    arrange(selmon);
    updatesystray();
}

void
togglefloating(const Arg *arg)
{
    if (!selmon->sel)
        return;
    if (selmon->sel->isfullscreen) {
        fullscreen(NULL);
        if (selmon->sel->isfloating)
            return;
    }

    selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed;

    if (selmon->sel->isfloating) {
        selmon->sel->x = selmon->wx + selmon->ww / 6,
        selmon->sel->y = selmon->wy + selmon->wh / 6,
        managefloating(selmon->sel);
        resize(selmon->sel, selmon->sel->x, selmon->sel->y, selmon->ww / 3 * 2, selmon->wh / 3 * 2, 0);
    }

    arrange(selmon);
    pointerfocuswin(selmon->sel);
}

void
toggleallfloating(const Arg *arg)
{
    Client *c = NULL;
    int somefloating = 0;

    if (!selmon->sel || selmon->sel->isfullscreen)
        return;

    for (c = selmon->clients; c; c = c->next)
        if (ISVISIBLE(c) && !HIDDEN(c) && c->isfloating) {
            somefloating = 1;
            break;
        }

    if (somefloating) {
        for (c = selmon->clients; c; c = c->next)
            if (ISVISIBLE(c) && !HIDDEN(c))
                c->isfloating = 0;
        arrange(selmon);
    } else {
        for (c = selmon->clients; c; c = c->next)
            if (ISVISIBLE(c) && !HIDDEN(c)) {
                c->isfloating = 1;
                resize(c, c->x + 2 * snap, c->y + 2 * snap, MAX(c->w - 4 * snap, snap) , MAX(c->h - 4 * snap, snap), 0);
            }
    }
    pointerfocuswin(selmon->sel);
}

void
togglescratch(const Arg *arg)
{
    Client *c;
    Monitor *m;
    unsigned int found = 0;

    for (m = mons; m && !found; m = m->next)
        for (c = m->clients; c && !(found = c->isscratchpad); c = c->next);
    if (found) {
        if (c->mon == selmon) // 在同屏幕则toggle win状态
            togglewin(&(Arg){.v = c});
        else {                // 不在同屏幕则将win移到当前屏幕 并显示
            sendmon(c, selmon);
            show(c);
            focus(c);
            if (c->isfloating) {
                resize(c, selmon->mx + (selmon->mw - selmon->sel->w) / 2, selmon->my + (selmon->mh - selmon->sel->h) / 2, selmon->sel->w, selmon->sel->h, 0);
            }
            pointerfocuswin(c);
        }
    } else
        spawn(arg);
}

void
restorewin(const Arg *arg) {
    int i = hiddenWinStackTop;
    while (i > -1) {
        if (HIDDEN(hiddenWinStack[i]) && ISVISIBLE(hiddenWinStack[i])) {
            show(hiddenWinStack[i]);
            focus(hiddenWinStack[i]);
            restack(selmon);
            return;
        }
        --i;
    }
}

void
hidewin(const Arg *arg) {
    if (!selmon->sel)
        return;
    Client *c = (Client *)selmon->sel;
    hide(c);
}

int
issinglewin(const Arg *arg) {
    Client *c = NULL;
    int cot = 0;

    for (c = selmon->clients; c; c = c->next) {
        if (ISVISIBLE(c) && !HIDDEN(c))
            cot++;
        if (cot > 1)
            return 0;
    }
    return 1;
}

void
togglewin(const Arg *arg)
{
    Client *c = (Client*)arg->v;
    if (c == selmon->sel)
        hide(c);
    else {
        if (HIDDEN(c))
            show(c);
        focus(c);
        restack(selmon);
    }
}

void
toggleview(const Arg *arg)
{
	unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);

	if (newtagset) {
		selmon->tagset[selmon->seltags] = newtagset;
		focus(NULL);
		arrange(selmon);
	}
}

void
toggleglobal(const Arg *arg)
{
    if (!selmon->sel)
        return;
    if (selmon->sel->isscratchpad) { // is scratchpad always global
        return;
    selmon->sel->isglobal ^= 1;
    selmon->sel->tags = selmon->sel->isglobal ? TAGMASK : selmon->tagset[selmon->seltags];
    focus(NULL);
    }
}
void
unfocus(Client *c, int setfocus)
{
    if (!c)
        return;
    grabbuttons(c, 0);
    XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel);
    if (setfocus) {
        XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
        XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
    }
}

void
unmanage(Client *c, int destroyed)
{
    Monitor *m = c->mon;
    XWindowChanges wc;

    detach(c);
    detachstack(c);
    if (!destroyed) {
        wc.border_width = c->oldbw;
        XGrabServer(dpy); /* avoid race conditions */
        XSetErrorHandler(xerrordummy);
        XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
        XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
        setclientstate(c, WithdrawnState);
        XSync(dpy, False);
        XSetErrorHandler(xerror);
        XUngrabServer(dpy);
    }
    free(c);
    focus(NULL);
    updateclientlist();
    arrange(m);
}

void
unmapnotify(XEvent *e)
{
    Client *c;
    XUnmapEvent *ev = &e->xunmap;

    if ((c = wintoclient(ev->window))) {
        if (ev->send_event)
            setclientstate(c, WithdrawnState);
        else
            unmanage(c, 0);
    }
    else if ((c = wintosystrayicon(ev->window))) {
        /* KLUDGE! sometimes icons occasionally unmap their windows, but do
         * _not_ destroy them. We map those windows back */
        XMapRaised(dpy, c->win);
        updatesystray();
    }
}

void
updatebars(void)
{
    unsigned int w;
    Monitor *m;
    XSetWindowAttributes wa = {
        .override_redirect = True,
        .background_pixel = 0,
        .border_pixel = 0,
        .colormap = cmap,
        .event_mask = ButtonPressMask|ExposureMask
    };
    XClassHint ch = {"dwm", "dwm"};
    for (m = mons; m; m = m->next) {
        if (m->barwin)
            continue;
        w = m->ww;
        if (showsystray && m == systraytomon(m))
            w -= getsystraywidth();
        m->barwin = XCreateWindow(dpy, root, m->wx, m->by, m->ww, bh, 0, depth,
                InputOutput, visual,
                CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWColormap|CWEventMask, &wa);
        XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
        if (showsystray && m == systraytomon(m))
            XMapRaised(dpy, systray->win);
        XMapRaised(dpy, m->barwin);
        XSetClassHint(dpy, m->barwin, &ch);
    }
}

void
updatebarpos(Monitor *m)
{
    m->wy = m->my;
    m->wh = m->mh;
    if (m->showbar) {
        m->wh -= bh;
        m->by = m->topbar ? m->wy : m->wy + m->wh;
        m->wy = m->topbar ? m->wy + bh : m->wy;
    } else
        m->by = -bh;
}

void
updateclientlist()
{
    Client *c;
    Monitor *m;

    XDeleteProperty(dpy, root, netatom[NetClientList]);
    for (m = mons; m; m = m->next)
        for (c = m->clients; c; c = c->next)
            XChangeProperty(dpy, root, netatom[NetClientList],
                    XA_WINDOW, 32, PropModeAppend,
                    (unsigned char *) &(c->win), 1);
}

int
updategeom(void)
{
    int dirty = 0;

#ifdef XINERAMA
    if (XineramaIsActive(dpy)) {
        int i, j, n, nn;
        Client *c;
        Monitor *m;
        XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
        XineramaScreenInfo *unique = NULL;

        for (n = 0, m = mons; m; m = m->next, n++);
        /* only consider unique geometries as separate screens */
        unique = ecalloc(nn, sizeof(XineramaScreenInfo));
        for (i = 0, j = 0; i < nn; i++)
            if (isuniquegeom(unique, j, &info[i]))
                memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
        XFree(info);
        nn = j;
        if (n <= nn) { /* new monitors available */
            for (i = 0; i < (nn - n); i++) {
                for (m = mons; m && m->next; m = m->next);
                if (m)
                    m->next = createmon();
                else
                    mons = createmon();
            }
            for (i = 0, m = mons; i < nn && m; m = m->next, i++)
                if (i >= n
                        || unique[i].x_org != m->mx || unique[i].y_org != m->my
                        || unique[i].width != m->mw || unique[i].height != m->mh)
                {
                    dirty = 1;
                    m->num = i;
                    m->mx = m->wx = unique[i].x_org;
                    m->my = m->wy = unique[i].y_org;
                    m->mw = m->ww = unique[i].width;
                    m->mh = m->wh = unique[i].height;
                    updatebarpos(m);
                }
        } else { /* less monitors available nn < n */
            for (i = nn; i < n; i++) {
                for (m = mons; m && m->next; m = m->next);
                while ((c = m->clients)) {
                    dirty = 1;
                    m->clients = c->next;
                    detachstack(c);
                    c->mon = mons;
                    attach(c);
                    attachstack(c);
                }
                if (m == selmon)
                    selmon = mons;
                cleanupmon(m);
            }
        }
        free(unique);
    } else
#endif /* XINERAMA */
    { /* default monitor setup */
        if (!mons)
            mons = createmon();
        if (mons->mw != sw || mons->mh != sh) {
            dirty = 1;
            mons->mw = mons->ww = sw;
            mons->mh = mons->wh = sh;
            updatebarpos(mons);
        }
    }
    if (dirty) {
        selmon = mons;
        selmon = wintomon(root);
    }
    return dirty;
}

void
updatenumlockmask(void)
{
    unsigned int i, j;
    XModifierKeymap *modmap;

    numlockmask = 0;
    modmap = XGetModifierMapping(dpy);
    for (i = 0; i < 8; i++)
        for (j = 0; j < modmap->max_keypermod; j++)
            if (modmap->modifiermap[i * modmap->max_keypermod + j]
                    == XKeysymToKeycode(dpy, XK_Num_Lock))
                numlockmask = (1 << i);
    XFreeModifiermap(modmap);
}

void
updatesizehints(Client *c)
{
    long msize;
    XSizeHints size;

    if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
        /* size is uninitialized, ensure that size.flags aren't used */
        size.flags = PSize;
    if (size.flags & PBaseSize) {
        c->basew = size.base_width;
        c->baseh = size.base_height;
    } else if (size.flags & PMinSize) {
        c->basew = size.min_width;
        c->baseh = size.min_height;
    } else
        c->basew = c->baseh = 0;
    if (size.flags & PResizeInc) {
        c->incw = size.width_inc;
        c->inch = size.height_inc;
    } else
        c->incw = c->inch = 0;
    if (size.flags & PMaxSize) {
        c->maxw = size.max_width;
        c->maxh = size.max_height;
    } else
        c->maxw = c->maxh = 0;
    if (size.flags & PMinSize) {
        c->minw = size.min_width;
        c->minh = size.min_height;
    } else if (size.flags & PBaseSize) {
        c->minw = size.base_width;
        c->minh = size.base_height;
    } else
        c->minw = c->minh = 0;
    if (size.flags & PAspect) {
        c->mina = (float)size.min_aspect.y / size.min_aspect.x;
        c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
    } else
        c->maxa = c->mina = 0.0;
    c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
}

void
updatestatus(void)
{
    Monitor *m;
    if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
        strcpy(stext, "^c#2D1B46^^b#335566^:) ^d^"); // 默认的状态栏文本
    for (m = mons; m; m = m->next)
        drawbar(m);
    updatesystray();
}

void
updatesystrayicongeom(Client *i, int w, int h)
{
    if (i) {
        i->h = bh;
        if (w == h)
            i->w = bh;
        else if (h == bh)
            i->w = w;
        else
            i->w = (int) ((float)bh * ((float)w / (float)h));
        applysizehints(i, &(i->x), &(i->y), &(i->w), &(i->h), False);
        /* force icons into the systray dimenons if they don't want to */
        if (i->h > bh) {
            if (i->w == i->h)
                i->w = bh;
            else
                i->w = (int) ((float)bh * ((float)i->w / (float)i->h));
            i->h = bh;
        }
    }
}

void
updatesystrayiconstate(Client *i, XPropertyEvent *ev)
{
    long flags;
    int code = 0;

    if (!showsystray || !i || ev->atom != xatom[XembedInfo] ||
            !(flags = getatomprop(i, xatom[XembedInfo])))
        return;

    if (flags & XEMBED_MAPPED && !i->tags) {
        i->tags = 1;
        code = XEMBED_WINDOW_ACTIVATE;
        XMapRaised(dpy, i->win);
        setclientstate(i, NormalState);
    }
    else if (!(flags & XEMBED_MAPPED) && i->tags) {
        i->tags = 0;
        code = XEMBED_WINDOW_DEACTIVATE;
        XUnmapWindow(dpy, i->win);
        setclientstate(i, WithdrawnState);
    }
    else
        return;
    sendevent(i->win, xatom[Xembed], StructureNotifyMask, CurrentTime, code, 0,
            systray->win, XEMBED_EMBEDDED_VERSION);
}

void
updatesystray(void)
{
    XSetWindowAttributes wa;
    XWindowChanges wc;
    Client *i;
    Monitor *m = systraytomon(NULL);
    unsigned int x = m->mx + m->mw;
    unsigned int w = 1;

    if (!showsystray)
        return;
    if (!systray) {
        /* init systray */
        if (!(systray = (Systray *)calloc(1, sizeof(Systray))))
            die("fatal: could not malloc() %u bytes\n", sizeof(Systray));
        systray->win = XCreateSimpleWindow(dpy, root, x, m->by, w, bh, 0, 0, scheme[SchemeSystray][ColBg].pixel);
        wa.event_mask        = ButtonPressMask | ExposureMask;
        wa.override_redirect = True;
        wa.background_pixel  = scheme[SchemeSystray][ColBg].pixel;
        XSelectInput(dpy, systray->win, SubstructureNotifyMask);
        XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32,
                PropModeReplace, (unsigned char *)&netatom[NetSystemTrayOrientationHorz], 1);
        XChangeWindowAttributes(dpy, systray->win, CWEventMask|CWOverrideRedirect|CWBackPixel, &wa);
        XMapRaised(dpy, systray->win);
        XSetSelectionOwner(dpy, netatom[NetSystemTray], systray->win, CurrentTime);
        if (XGetSelectionOwner(dpy, netatom[NetSystemTray]) == systray->win) {
            sendevent(root, xatom[Manager], StructureNotifyMask, CurrentTime, netatom[NetSystemTray], systray->win, 0, 0);
            XSync(dpy, False);
        }
        else {
            fprintf(stderr, "dwm: unable to obtain system tray.\n");
            free(systray);
            systray = NULL;
            return;
        }
    }
    for (w = 0, i = systray->icons; i; i = i->next) {
        /* make sure the background color stays the same */
        wa.background_pixel  = scheme[SchemeSystray][ColBg].pixel;
        XChangeWindowAttributes(dpy, i->win, CWBackPixel, &wa);
        XMapRaised(dpy, i->win);
        w += systrayspacing;
        i->x = w;
        XMoveResizeWindow(dpy, i->win, i->x + 3, 0 + 3, MAX(i->w - 6, bh - 6), bh - 6); // 限制过大的图标
        w += MAX(i->w, bh);
        if (i->mon != m)
            i->mon = m;
    }
    w = w ? w + systrayspacing : 1;
    x -= w;
    XMoveResizeWindow(dpy, systray->win, x, m->by, w, bh);
    wc.x = x; wc.y = m->by; wc.width = w; wc.height = bh;
    wc.stack_mode = Above; wc.sibling = m->barwin;
    XConfigureWindow(dpy, systray->win, CWX|CWY|CWWidth|CWHeight|CWSibling|CWStackMode, &wc);
    XMapWindow(dpy, systray->win);
    XMapSubwindows(dpy, systray->win);
    XSync(dpy, False);
}

void
updatetitle(Client *c)
{
    if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
        gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
    if (c->name[0] == '\0') /* hack to mark broken clients */
        strcpy(c->name, broken);
}

void
updatewindowtype(Client *c)
{
    Atom state = getatomprop(c, netatom[NetWMState]);
    Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

    if (state == netatom[NetWMFullscreen])
        setfullscreen(c, 1);
    if (wtype == netatom[NetWMWindowTypeDialog])
        c->isfloating = 1;
}

void
updatewmhints(Client *c)
{
    XWMHints *wmh;

    if ((wmh = XGetWMHints(dpy, c->win))) {
        if (c == selmon->sel && wmh->flags & XUrgencyHint) {
            wmh->flags &= ~XUrgencyHint;
            XSetWMHints(dpy, c->win, wmh);
        } else
            c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
        if (wmh->flags & InputHint)
            c->neverfocus = !wmh->input;
        else
            c->neverfocus = 0;
        XFree(wmh);
    }
}

void
setgap(const Arg *arg)
{
    gappi = arg->i ? MAX(gappi + arg->i, 0) : _gappi;
    gappo = arg->i ? MAX(gappo + arg->i, 0) : _gappo;
    arrange(selmon);
}

void
view(const Arg *arg)
{
    int i;
    unsigned int tmptag;
    Client *c;
    int n = 0;

    selmon->seltags ^= 1; /* toggle sel tagset */
    if (arg->ui & TAGMASK) {
        selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
        selmon->pertag->prevtag = selmon->pertag->curtag;

        if (arg->ui == ~0)
            selmon->pertag->curtag = 0;
        else {
            for (i = 0; !(arg->ui & 1 << i); i++) ;
            selmon->pertag->curtag = i + 1;
        }
    } else {
        tmptag = selmon->pertag->prevtag;
        selmon->pertag->prevtag = selmon->pertag->curtag;
        selmon->pertag->curtag = tmptag;
    }

    selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
    selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag];
    selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
    selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
    selmon->lt[selmon->sellt^1] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt^1];

    if (selmon->showbar != selmon->pertag->showbars[selmon->pertag->curtag])
        togglebar(NULL);

    focus(NULL);
    arrange(selmon);

    // 若当前tag无窗口 且附加了v参数 则执行
    if (arg->v) {
        for (c = selmon->clients; c; c = c->next)
            if (c->tags & arg->ui && !HIDDEN(c) && !c->isglobal)
                n++;
        if (n == 0) {
            spawn(&(Arg){ .v = (const char*[]){ "/bin/sh", "-c", arg->v, NULL } });
        }
    }
}

// 显示所有tag 或 跳转到聚焦窗口的tag
void
toggleoverview(const Arg *arg)
{
    if (selmon->sel && selmon->sel->isfullscreen) /* no support for fullscreen windows */
        return;

    uint target = selmon->sel && selmon->sel->tags != TAGMASK ? selmon->sel->tags : selmon->seltags;
    selmon->isoverview ^= 1;
    view(&(Arg){ .ui = target });
    pointerfocuswin(selmon->sel);
}

void
viewtoleft(const Arg *arg) {
    unsigned int target = selmon->tagset[selmon->seltags], pre;
    Client *c;
    while (1) {
        pre = target;
        target >>= 1;
        if (target == pre) return;

        for (c = selmon->clients; c; c = c->next) {
            if (c->isglobal && c->tags == TAGMASK) continue;
            if (c->tags & target && __builtin_popcount(selmon->tagset[selmon->seltags] & TAGMASK) == 1
                    && selmon->tagset[selmon->seltags] > 1) {
                view(&(Arg) { .ui = target });
                return;
            }
        }
    }
}

void
viewtoright(const Arg *arg) {
    unsigned int target = selmon->tagset[selmon->seltags];
    Client *c;
    while (1) {
        target = target == 0 ? 1 : target << 1;
        if (!(target & TAGMASK)) return;

        for (c = selmon->clients; c; c = c->next) {
            if (c->isglobal && c->tags == TAGMASK) continue;
            if (c->tags & target && __builtin_popcount(selmon->tagset[selmon->seltags] & TAGMASK) == 1
                    && selmon->tagset[selmon->seltags] & (TAGMASK >> 1)) {
                view(&(Arg) { .ui = target });
                return;
            }
        }
    }
}

void
tile(Monitor *m)
{
    unsigned int i, n, mw, mh, sh, my, sy; // mw: master的宽度, mh: master的高度, sh: stack的高度, my: master的y坐标, sy: stack的y坐标
    Client *c;

    for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
    if (n == 0) return;

    if (n > m->nmaster)
        mw = m->nmaster ? (m->ww + gappi) * m->mfact : 0;
    else
        mw = m->ww - 2 * gappo + gappi;

    mh = m->nmaster == 0 ? 0 : (m->wh - 2 * gappo - gappi * (m->nmaster - 1)) / m->nmaster;           // 单个master的高度
    sh = n == m->nmaster ? 0 : (m->wh - 2 * gappo - gappi * (n - m->nmaster - 1)) / (n - m->nmaster); // 单个stack的高度

    for (i = 0, my = sy = gappo, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
        if (i < m->nmaster) {
            resize(c,
                   m->wx + gappo,
                   m->wy + my,
                   mw - 2 * c->bw - gappi,
                   mh - 2 * c->bw,
                   0);
            my += HEIGHT(c) + gappi;
        } else {
            resize(c,
                   m->wx + mw + gappo,
                   m->wy + sy,
                   m->ww - mw - 2 * c->bw - 2 * gappo,
                   sh - 2* c->bw,
                   0);
            sy += HEIGHT(c) + gappi;
        }
}

void
magicgrid(Monitor *m)
{
    grid(m, gappo, gappi);
}

void
overview(Monitor *m)
{
    grid(m, overviewgappo, overviewgappi);
}

void
grid(Monitor *m, uint gappo, uint gappi)
{
    unsigned int i, n;
    unsigned int cx, cy, cw, ch;
    unsigned int dx;
    unsigned int cols, rows, overcols;
    Client *c;

    for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
    if (n == 0) return;
    if (n == 1) {
        c = nexttiled(m->clients);
        cw = (m->ww - 2 * gappo) * 0.7;
        ch = (m->wh - 2 * gappo) * 0.65;
        resize(c,
               m->mx + (m->mw - cw) / 2 + gappo,
               m->my + (m->mh - ch) / 2 + gappo,
               cw - 2 * c->bw,
               ch - 2 * c->bw,
               0);
        return;
    }
    if (n == 2) {
        c = nexttiled(m->clients);
        cw = (m->ww - 2 * gappo - gappi) / 2;
        ch = (m->wh - 2 * gappo) * 0.65;
        resize(c,
               m->mx + gappo,
               m->my + (m->mh - ch) / 2 + gappo,
               cw - 2 * c->bw,
               ch - 2 * c->bw,
               0);
        resize(nexttiled(c->next),
               m->mx + cw + gappo + gappi,
               m->my + (m->mh - ch) / 2 + gappo,
               cw - 2 * c->bw,
               ch - 2 * c->bw,
               0);
        return;
    }

    for (cols = 0; cols <= n / 2; cols++)
        if (cols * cols >= n)
            break;
    rows = (cols && (cols - 1) * cols >= n) ? cols - 1 : cols;
	ch = (m->wh - 2 * gappo - (rows - 1) * gappi) / rows;
	cw = (m->ww - 2 * gappo - (cols - 1) * gappi) / cols;

    overcols = n % cols;
    if (overcols) dx = (m->ww - overcols * cw - (overcols - 1) * gappi) / 2 - gappo;
	for (i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++) {
        cx = m->wx + (i % cols) * (cw + gappi);
        cy = m->wy + (i / cols) * (ch + gappi);
        if (overcols && i >= n - overcols) {
            cx += dx;
        }
        resize(c,
               cx + gappo,
               cy + gappo,
               cw - 2 * c->bw,
               ch - 2 * c->bw,
               0);
	}
}

Client *
wintoclient(Window w)
{
    Client *c;
    Monitor *m;

    for (m = mons; m; m = m->next)
        for (c = m->clients; c; c = c->next)
            if (c->win == w)
                return c;
    return NULL;
}

Client *
wintosystrayicon(Window w) {
    Client *i = NULL;

    if (!showsystray || !w)
        return i;
    for (i = systray->icons; i && i->win != w; i = i->next) ;
    return i;
}

Monitor *
wintomon(Window w)
{
    int x, y;
    Client *c;
    Monitor *m;

    if (w == root && getrootptr(&x, &y))
        return recttomon(x, y, 1, 1);
    for (m = mons; m; m = m->next)
        if (w == m->barwin)
            return m;
    if ((c = wintoclient(w)))
        return c->mon;
    return selmon;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */
int
xerror(Display *dpy, XErrorEvent *ee)
{
    if (ee->error_code == BadWindow
            || (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
            || (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
            || (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
            || (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
            || (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
            || (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
            || (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
            || (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
        return 0;
    fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
            ee->request_code, ee->error_code);
    return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerrordummy(Display *dpy, XErrorEvent *ee)
{
    return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display *dpy, XErrorEvent *ee)
{
    die("dwm: another window manager is already running");
    return -1;
}

Monitor *
systraytomon(Monitor *m) {
    Monitor *t;
    int i, n;
    if(!systraypinning) {
        if(!m)
            return selmon;
        return m == selmon ? m : NULL;
    }
    for(n = 1, t = mons; t && t->next; n++, t = t->next) ;
    for(i = 1, t = mons; t && t->next && i < systraypinning; i++, t = t->next) ;
    if(n < systraypinning)
        return mons;
    return t;
}

void
xinitvisual()
{
    XVisualInfo *infos;
    XRenderPictFormat *fmt;
    int nitems;
    int i;

    XVisualInfo tpl = {
        .screen = screen,
        .depth = 32,
        .class = TrueColor
    };
    long masks = VisualScreenMask | VisualDepthMask | VisualClassMask;

    infos = XGetVisualInfo(dpy, masks, &tpl, &nitems);
    visual = NULL;
    for(i = 0; i < nitems; i ++) {
        fmt = XRenderFindVisualFormat(dpy, infos[i].visual);
        if (fmt->type == PictTypeDirect && fmt->direct.alphaMask) {
            visual = infos[i].visual;
            depth = infos[i].depth;
            cmap = XCreateColormap(dpy, root, visual, AllocNone);
            useargb = 1;
            break;
        }
    }

    XFree(infos);

    if (! visual) {
        visual = DefaultVisual(dpy, screen);
        depth = DefaultDepth(dpy, screen);
        cmap = DefaultColormap(dpy, screen);
    }
}

void
zoom(const Arg *arg)
{
    Client *c = selmon->sel;

    if (c && (c->isfloating || c->isfullscreen))
        return;
    if (c == nexttiled(selmon->clients))
        if (!c || !(c = nexttiled(c->next)))
            return;
    pop(c);
}

int
main(int argc, char *argv[])
{
    if (argc == 2 && !strcmp("-v", argv[1]))
        die("dwm-6.3");
    else if (argc != 1)
        die("usage: dwm [-v]");
    if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
        fputs("warning: no locale support\n", stderr);
    if (!(dpy = XOpenDisplay(NULL)))
        die("dwm: cannot open display");
    checkotherwm();
    setup();
#ifdef __OpenBSD__
    if (pledge("stdio rpath proc exec", NULL) == -1)
        die("pledge");
#endif /* __OpenBSD__ */
    scan();
    runAutostart();
    run();
    cleanup();
    XCloseDisplay(dpy);
    return EXIT_SUCCESS;
}
