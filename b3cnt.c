/*
 *	Copyright (c) 2010, Rinaldini Julien, julien.rinaldini@heig-vd.ch
 *	Copyright (c) 2014, Mytchel Hammond, mytchel at openmailbox dot org
 *
 *	Permission is hereby granted, free of charge, to any person obtaining a
 *	copy of this software and associated documentation files (the "Software"),
 *	to deal in the Software without restriction, including without limitation
 *	the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *	and/or sell copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be included in
 *	all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *	DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/Xinerama.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/XF86keysym.h>

#define LEN(X)	  (sizeof(X)/sizeof(*X))
#define CLEANMASK(mask) (mask & ~(numlockmask | LockMask))
#define ROOTMASK   SubstructureRedirectMask|ButtonPressMask| \
	SubstructureNotifyMask|PropertyChangeMask

#define MINWSZ 20

enum { RESIZE, MOVE };
enum { WM_PROTOCOLS, WM_DELETE_WINDOW, WM_COUNT };
enum { NET_SUPPORTED, NET_FULLSCREEN, NET_WM_STATE, NET_ACTIVE, NET_COUNT };

typedef struct Key Key;
typedef struct Button Button;
typedef struct Client Client;
typedef struct Desktop Desktop;
typedef struct Monitor Monitor;

struct Arg {
	char** com;
	int i;
	Key *map;
};

struct Key {
	unsigned int mod;
	KeySym keysym;
	void (*function)(struct Arg arg);
	struct Arg arg;
};

struct Button {
	unsigned int mask;
	unsigned int button;
	void (*function)(struct Arg arg);
	struct Arg arg;
};

struct Client {
	Client *next, *prev;
	int x, y, w, h;
	int full_width, full_height;
	int b;
	Window win;
};

struct Desktop {
	Client *head, *current, *old;
	Desktop *next, *prev;
};

struct Monitor {
	int y, x, w, h;
	Monitor *next, *prev;
};

static void changedesktop(struct Arg arg);
static void clienttodesktop(struct Arg arg);

static void shiftfocus(struct Arg arg);
static void focusprev();
static void shiftwindow(struct Arg arg);
static void bringtotop();

static void fullwidth();
static void fullheight();
static void fullscreen();
static void toggleborder();

static void mousemove();
static void mouseresize();

static void submap(struct Arg arg);
static void stickysubmap(struct Arg arg);
static void exitsubmap();

static void spawn(const struct Arg arg);
static void killclient();

static void updatemonitors();
static void quit();

#include "config.h"

static void setup();
static void die(const char* e);
static unsigned long getcolor(const char *color, const int screen);

static void sendkillsignal(Window w);
static void sigchld(int unused);

static Client *copyclient(Client *o);
static Monitor *monitorholdingclient(Client *c);
static int wintoclient(Window w, Client **c, Desktop **d);

static void layout(Desktop *d);
static void focus(Client *c, Desktop *d);

static void addwindow(Window w, Desktop *d);
// adds c after a on Desktop d. if a is null then sets c to be head.
static void addclient(Client *c, Client *a, Desktop *d);
static void removeclient(Client *c, Desktop *d);
static void removewindow(Window w);
static void clienttotop(Client *c, Desktop *d);
static Client *lastclient(Desktop *d);

static void updateclient(Client *c);

static void mousemotion(int t);

static void grabkeys();
static int keypressed(XKeyEvent ke, Key *map);
static int iskeymod(KeySym keysym);
static void listensubmap(Key *map, int sticky);

static void destroynotify(XEvent *e);
static void maprequest(XEvent *e);
static void unmapnotify(XEvent *e);
static void enternotify(XEvent *e);
static void keypress(XEvent *e);
static void buttonpress(XEvent *e);
static void configurerequest(XEvent *e);
static void mappingnotify(XEvent *e);
static int xerror(__attribute((unused)) Display *dis, XErrorEvent *ee);

// Variables
static Display *dis;
static Window root;
static Atom wmatoms[WM_COUNT];
static int screen, bool_quit, numlockmask;
static unsigned int win_unfocus, win_focus;
static Monitor *monitors;
static Desktop desktops[DESKTOP_NUM];
static int current;

// Events array
static void (*events[LASTEvent])(XEvent *e) = {
	[MapRequest] = maprequest,
	[UnmapNotify] = unmapnotify,
	[DestroyNotify] = destroynotify,
	[ConfigureRequest] = configurerequest,
	[KeyPress] = keypress,
	[ButtonPress] = buttonpress,
	[EnterNotify] = enternotify,
	[MappingNotify] = mappingnotify,
};

void setup() {
	int k, j, i;
	// Install a signal
	sigchld(0);

	bool_quit = False;

	// Screen and root window
	screen = DefaultScreen(dis);
	root = RootWindow(dis,screen);

	win_focus = getcolor(FOCUS, screen);
	win_unfocus = getcolor(UNFOCUS, screen);

	XModifierKeymap *modmap = XGetModifierMapping(dis);
	for (k = 0; k < 8; k++) for (j = 0; j < modmap->max_keypermod; j++)
		if (modmap->modifiermap[modmap->max_keypermod*k + j] 
				== XKeysymToKeycode(dis, XK_Num_Lock))
			numlockmask = (1 << k);
	XFreeModifiermap(modmap);

	wmatoms[WM_PROTOCOLS] = XInternAtom(dis, "WM_PROTOCOLS", False);
	wmatoms[WM_DELETE_WINDOW] = XInternAtom(dis, "WM_DELETE_WINDOW", False);

	XSelectInput(dis, root, ROOTMASK);
	XSync(dis, False);
	XSetErrorHandler(xerror);
	XSync(dis, False);

	current = 0;
	for (i = 0; i < DESKTOP_NUM; i++)
		desktops[i].head = desktops[i].current = desktops[i].old = NULL;

	updatemonitors(NULL);
	grabkeys();

	XSync(dis, False);
}

unsigned long getcolor(const char *color, const int screen) {
	XColor c; Colormap map = DefaultColormap(dis, screen);
	if (!XAllocNamedColor(dis, map, color, &c, &c))
		die("Cannot allocate color.");
	return c.pixel;
}

void die(const char* e) {
	fprintf(stderr, "b3cnt: %s\n",e);
	exit(1);
}

void sendkillsignal(Window w) {
	int n, i;
	XEvent ke;

	ke.type = ClientMessage;
	ke.xclient.window = w;
	ke.xclient.format = 32;
	ke.xclient.message_type = wmatoms[WM_PROTOCOLS];
	ke.xclient.data.l[0]	= wmatoms[WM_DELETE_WINDOW];
	ke.xclient.data.l[1]	= CurrentTime;
	XSendEvent(dis, w, False, NoEventMask, &ke);
}

void sigchld(int unused) {
	// Again, thx to dwm ;)
	if(signal(SIGCHLD, sigchld) == SIG_ERR)
		die("Can't install SIGCHLD handler");
	while(0 < waitpid(-1, NULL, WNOHANG));
}

void quit() {
	Client *c; int i;

	fprintf(stdout, "b3cnt: FINALLY. They've left,"
			"now I can have some piece and quiet.\n");
	bool_quit = 1;

	fprintf(stdout, "b3cnt: Killing all their windows.\n");
	for (i = 0; i < DESKTOP_NUM; i++)
		for (c = desktops[i].head; c; c = c->next)
			sendkillsignal(c->win);
}

void updatemonitors() {
	XineramaScreenInfo *info = NULL;
	int i, j, Monitor_count = 0, count = 0;
	Monitor *m, *new, *first;
	Desktop *d;
	Client *c;

	if (XineramaIsActive(dis)) {
		for (first = monitors; first && first->prev; first = first->prev);

		for (m = first; m; m = m->next)
			Monitor_count++;

		if (!(info = XineramaQueryScreens(dis, &count)))
			die("Error xineramaQueryScreens!");

		if (count > Monitor_count) {
			for (m = first; m && m->next; m = m->next); // Go to last Monitor

			for (i = Monitor_count; i < count; i++) {
				if(!(new = (Monitor *)calloc(1,sizeof(Monitor))))
					die("Error calloc!");

				new->prev = m;
				new->next = NULL;

				if (m)
					m->next = new;

				m = new;
			}

			for (monitors = m; monitors->prev; monitors = monitors->prev);
			first = monitors;
		} else if (count < Monitor_count) {
			fprintf(stderr, "removing montiors\n");
			if (count < 1)
				die("A Monitor could help");

			monitors = first;

			// Just move all Clients to first Monitor for now.
			// Later on may make it only move from monitors that are no longer
			// there.

			for (i = 0; i < DESKTOP_NUM; i++) {
				for (c = desktops[i].head; c; c = c->next) {
					if (monitorholdingclient(c) != first) {
						c->x = monitors->x;
						c->y = monitors->y;
					}
				}
			}
		}

		for (m = first, i = 0; m; m = m->next, i++) {
			m->w = info[i].width;
			m->h = info[i].height;
			m->x = info[i].x_org;
			m->y = info[i].y_org;
		}

		monitors = first;

		layout(&desktops[current]);

		XFree(info);
	}
}

void changedesktop(const struct Arg arg) {
	Client *c;

	if (arg.i < 0 || arg.i > DESKTOP_NUM || arg.i == current)
		return;

	// Update client positions before removing them.
	for (c = desktops[current].head; c; c = c->next) 
		updateclient(c);

	for (c = desktops[arg.i].head; c; c = c->next) 
		XMapWindow(dis, c->win);

	XChangeWindowAttributes(dis, root, CWEventMask, 
			&(XSetWindowAttributes)
			{.do_not_propagate_mask = SubstructureNotifyMask});

	for (c = desktops[current].head; c; c = c->next) 
		XUnmapWindow(dis, c->win);

	// Now ok to listen to events.
	XChangeWindowAttributes(dis, root, CWEventMask, 
			&(XSetWindowAttributes){.event_mask = ROOTMASK});

	current = arg.i;

	layout(&desktops[current]);
}

void clienttodesktop(const struct Arg arg) {
	Client *c;

	if (!desktops[current].current)
		return;

	c = desktops[current].current;
	removeclient(c, &desktops[current]);
	changedesktop(arg);
	addclient(c, lastclient(&desktops[current]), &desktops[current]);
}

Client *copyclient(Client *o) {
	Client *c;
	c = malloc(sizeof(Client));
	c->win = o->win; c->next = o->next; c->prev = o->prev;
	c->x = o->x; c->y = o->y; c->w = o->w; c->h = o->h;
	c->full_width = o->full_width; c->full_height = o->full_height;
	return c; 
}

void focus(Client *c, Desktop *d) {
	Client *t;
	int i;

	if (d != &desktops[current]) {
		for (i = 0; i < DESKTOP_NUM && d != &desktops[i]; i++);
		struct Arg arg;
		arg.i = i;
		changedesktop(arg);
	}

	if (d->current != c) {
		if (d->current)
			XSetWindowBorder(dis, d->current->win, win_unfocus);
		d->old = d->current;
		d->current = c;
	}

	if (d->current) {
		XSetWindowBorder(dis, d->current->win, win_focus);
		XSetInputFocus(dis, d->current->win, RevertToPointerRoot, CurrentTime);
	}
}

void layout(Desktop *d) {
	Client *t; Monitor *m;
	for (t = d->head; t; t = t->next) {
		XSetWindowBorderWidth(dis, t->win, t->b ? BORDER_WIDTH : 0);
		XSetWindowBorder(dis, t->win, 
				t == d->current ? win_focus : win_unfocus);
		XRaiseWindow(dis, t->win);

		if (t->full_width || t->full_height) {
			m = monitorholdingclient(t);
			if (!m) continue;
			if (t->full_width && t->full_height)
				XMoveResizeWindow(dis, t->win, m->x, m->y, 
						m->w - (t->b ? BORDER_WIDTH : 0) * 2,
						m->h - (t->b ? BORDER_WIDTH : 0) * 2);
			else if (t->full_width)
				XMoveResizeWindow(dis, t->win, m->x, t->y, 
						m->w - (t->b ? BORDER_WIDTH : 0) * 2, t->h);
			else if (t->full_height)
				XMoveResizeWindow(dis, t->win, t->x, m->y, 
						t->w, m->h - (t->b ? BORDER_WIDTH : 0) * 2);
		} else
			XMoveResizeWindow(dis, t->win, t->x, t->y, t->w, t->h);
	}

	focus(d->current, d);
}

Monitor *monitorholdingclient(Client *c) {
	Monitor *m;
	if (!c) return NULL;
	for (m = monitors; m; m = m->next) 
		if (m->x <= c->x && m->x + m->w > c->x
				&& m->y <= c->y && m->y + m->h > c->y)
			return m;
	return monitors; // It was probably too far to the left to be in the first.
}

void fullwidth() {
	Client *c;
	c = desktops[current].current;
	if (!c) return;
	c->full_width = !c->full_width;
	layout(&desktops[current]);
}

void fullheight() {
	Client *c;
	c = desktops[current].current;
	if (!c) return;
	c->full_height = !c->full_height;
	layout(&desktops[current]);
}

void fullscreen() {
	Client *c;
	c = desktops[current].current;
	if (!c) return;
	if (c->full_height && c->full_width)
		c->full_height = c->full_width = False;
	else
		c->full_height = c->full_width = True;
	layout(&desktops[current]);
}

void toggleborder() {
	Client *c;
	c = desktops[current].current;
	if (!c) return;
	c->b = !c->b;
	layout(&desktops[current]);
}

void updateclient(Client *c) {
	XWindowAttributes window_attributes;

	if (!c)
		return;

	if (!XGetWindowAttributes(dis, c->win, &window_attributes)) {
		fprintf(stderr, "Failed XGetWindowAttributes!\n");
		return;
	}

	c->x = window_attributes.x;
	c->y = window_attributes.y;
	c->w = window_attributes.width;
	c->h = window_attributes.height;
}

void bringtotop() {
	if (desktops[current].current)
		clienttotop(desktops[current].current, &desktops[current]);
}

void clienttotop(Client *c, Desktop *d) {
	Client *o;
	removeclient(c, d);
	for (o = d->head; o && o->next; o = o->next);
	addclient(c, lastclient(d), d);
}

void shiftfocus(struct Arg arg) {
	Client *c;

	c = desktops[current].current;
	if (!c)
		return;  

	if (arg.i > 0) {
		if (c->next)
			focus(c->next, &desktops[current]);
		else
			focus(desktops[current].head, &desktops[current]);
	} else if (arg.i < 0) {
		if (c->prev)
			focus(c->prev, &desktops[current]);
		else {
			for (; c->next; c = c->next);
			focus(c, &desktops[current]);
		}
	}
}

void focusprev() {
	Client *c;

	if (desktops[current].old)
		focus(desktops[current].old, &desktops[current]);
}

void shiftwindow(struct Arg arg) {
	Client *c;

	c = desktops[current].current;
	if (!c) return;

	if (arg.i > 0 && c->next) {
		removeclient(c, &desktops[current]);
		addclient(c, c->next, &desktops[current]);
	} else if (arg.i < 0 && c->prev) {
		removeclient(c, &desktops[current]);
		addclient(c, c->prev->prev, &desktops[current]);
	}
}

void addwindow(Window w, Desktop *d) {
	int i, mi;
	int modifiers[] = {0, LockMask, numlockmask, numlockmask|LockMask };
	Window win_away; // Don't care about this.
	int rx, ry, dont_care, v;
	Client *c; Monitor *m;

	if(!(c = malloc(sizeof(Client))))
		die("Error malloc!");

	XSelectInput(dis, w, EnterWindowMask);

	for (mi = 0; mi < LEN(modifiers); mi++) 
		for (i = 0; i < LEN(buttons); i++)
			XGrabButton(dis, buttons[i].button, 
					buttons[i].mask|modifiers[mi], w,
					False, ButtonPressMask|ButtonReleaseMask,
					GrabModeAsync, GrabModeAsync, None, None);

	c->win = w;
	c->full_width = c->full_height = False;
	c->b = True;
	updateclient(c);

	if (XQueryPointer(dis, root, &win_away, &win_away, &rx, &ry,
				&dont_care, &dont_care, &v)) {
		c->x = rx - c->w / 2;
		c->y = ry - c->h / 2;
	}

	m = monitorholdingclient(c);

	if (c->x < 0)
		c->x = 0;
	if (c->x + c->w > m->w)
		c->x = m->w - c->w - BORDER_WIDTH * 2 - 1;
	if (c->y < 0)
		c->y = 0;
	if (c->y + c->h > m->h)
		c->y = m->h - c->h - BORDER_WIDTH * 2 - 1;

	addclient(c, lastclient(d), d);
}

void addclient(Client *n, Client *a, Desktop *d) {
	if (!d->head) {
		d->head = n;
		d->current = d->old = NULL;
		n->next = NULL;
		n->prev = NULL;
	} else if (!a) {
		d->head->prev = n;
		n->prev = NULL;
		n->next = d->head;
		d->head = n;
	} else {
		n->prev = a;
		n->next = a->next;
		if (a->next)
			a->next->prev = n;
		a->next = n;
	}

	layout(d);
	focus(n, d);
}

Client *lastclient(Desktop *d) {
	Client *c;
	for (c = d->head; c && c->next; c = c->next);
	return c;
}

void removeclient(Client *c, Desktop *d) {
	Client *p;

	if (d->head == c)
		d->head = c->next;

	if (d->current == c) {
		if (c->prev)
			d->current = c->prev;
		else
			d->current = d->head;
		d->old = d->current;
	}

	if (c->prev)
		c->prev->next = c->next;
	if (c->next)
		c->next->prev = c->prev;
}

int wintoclient(Window w, Client **c, Desktop **d) {
	int i;

	for (i = 0; i < DESKTOP_NUM; i++) {
		(*d) = &desktops[i];
		for (*c = (*d)->head; *c; *c = (*c)->next) {
			if ((*c)->win == w)
				return True;
		}
	}

	return False;
}

void grabkeys() {
	int i, m;
	int modifiers[] = {0, LockMask, numlockmask, numlockmask|LockMask };
	KeyCode code;

	for (i = 0, m = 0; keys[i].function != NULL; i++, m = 0) {
		if (!(code = XKeysymToKeycode(dis, keys[i].keysym))) continue;
		while (m < LEN(modifiers)) {
			XGrabKey(dis, code, keys[i].mod | modifiers[m++], root, True,
					GrabModeAsync, GrabModeAsync);
		}
	}
}

int keypressed(XKeyEvent ke, Key *map) {
	int i;
	KeySym keysym = XLookupKeysym(&ke, 0);
	for (i = 0; map[i].function; i++) {
		if (map[i].keysym == keysym 
				&& CLEANMASK(map[i].mod) == CLEANMASK(ke.state)) {
			if (map[i].function == exitsubmap)
				return False;
			else
				map[i].function(map[i].arg);
		}
	}
	return True;
}

void exitsubmap(struct Arg arg) {}

int iskeymod(KeySym keysym) {
	int i;
	KeySym mods[] = {XK_Shift_L, XK_Shift_R, XK_Control_L, XK_Control_R,
		XK_Meta_L, XK_Meta_R, XK_Alt_L, XK_Alt_R};
	for (i = 0; i < LEN(mods); i++)
		if (mods[i] == keysym) return True;
	return False;
}

void listensubmap(Key *map, int sticky) {
	XEvent ev;
	KeySym keysym;

	if (!XGrabKeyboard(dis, root, True, GrabModeAsync, GrabModeAsync,
				CurrentTime) == GrabSuccess) return;

	while (True) {
		XNextEvent(dis, &ev);
		if (ev.type == KeyPress) {
			keysym = XLookupKeysym(&(ev.xkey), 0);

			if (iskeymod(keysym)) continue;

			if (!keypressed(ev.xkey, map) || !sticky)
				break;
		} else if (ev.type == ConfigureRequest || ev.type == MapRequest)
			events[ev.type](&ev);
	}

	XUngrabKeyboard(dis, CurrentTime);
}

void submap(struct Arg arg) {
	listensubmap(arg.map, False);
}

void stickysubmap(struct Arg arg) {
	listensubmap(arg.map, True);
}

void mousemove() {
	mousemotion(MOVE);
}

void mouseresize() {
	mousemotion(RESIZE);
}

void mousemotion(int t) {
	int rx, ry, dc, xw, yh, v;
	Window w, wr;
	XWindowAttributes wa;
	XEvent ev;
	Client *c; Desktop *d;

	if (!XQueryPointer(dis, root, &w, &wr, &rx, &ry, &dc, &dc, &v) 
			|| !wintoclient(wr, &c, &d)) return;

	if (!XGetWindowAttributes(dis, c->win, &wa)) return;

	if (t == RESIZE) {
		XWarpPointer(dis, c->win, c->win, 0, 0, 0, 0, wa.width, wa.height);
		rx = wa.x + wa.width;
		ry = wa.y + wa.height;
	}

	if (XGrabPointer(dis, root, False, ButtonPressMask|ButtonReleaseMask
				|PointerMotionMask, GrabModeAsync, GrabModeAsync, 
				None, None, CurrentTime) != GrabSuccess) return;

	clienttotop(c, d);
	c->full_width = c->full_height = 0;

	do {
		XMaskEvent(dis, ButtonPressMask|ButtonReleaseMask|PointerMotionMask
				|SubstructureRedirectMask, &ev);
		if (ev.type == MotionNotify) {
			xw = (t == MOVE ? wa.x:wa.width) + ev.xmotion.x - rx;
			yh = (t == MOVE ? wa.y:wa.height) + ev.xmotion.y - ry;
			if (t == RESIZE) 
				XResizeWindow(dis, c->win, 
						xw > 0 ? xw : 5, yh > 0 ? yh : 5);
			else if (t == MOVE) XMoveWindow(dis, c->win, xw, yh);
		} else if (ev.type == ConfigureRequest || ev.type == MapRequest)
			events[ev.type](&ev);
	} while (ev.type != ButtonRelease);

	XUngrabPointer(dis, CurrentTime);
	updateclient(c);
}

void maprequest(XEvent *e) {
	fprintf(stderr, "map\n");
	XMapRequestEvent *ev = &e->xmaprequest;
	Client *c; Desktop *d;

	XMapWindow(dis, ev->window);

	if (!wintoclient(ev->window, &c, &d)) 
		addwindow(ev->window, &desktops[current]);
}

void removewindow(Window w) {
	Client *c; Desktop *d;
	if (wintoclient(w, &c, &d)) {
		removeclient(c, d);
		free(c);
		if (&desktops[current] == d)
			layout(d);
	}
}

void unmapnotify(XEvent *e) {
	fprintf(stderr, "unmap\n");
	removewindow(e->xunmap.window);
}

void destroynotify(XEvent *e) {
	fprintf(stderr, "destroy\n");
	removewindow(e->xdestroywindow.window);
}

void enternotify(XEvent *e) {
	Client *c; Desktop *d;
	if (wintoclient(e->xcrossing.window, &c, &d)) 
		focus(c, d); 
}

void configurerequest(XEvent *e) {
	fprintf(stderr, "configurerequest?\n");
	Client *c; Desktop *d; Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	if (wintoclient(ev->window, &c, &d)) {
		c->x = ev->x;
		c->y = ev->y;
		c->w = ev->width;
		c->h = ev->height;
		m = monitorholdingclient(c);
		c->full_width = c->full_height = False;
		if (c->w == m->w)
			c->full_width = True;
		if (c->h == m->h)
			c->full_height = True;
		layout(d);
		focus(c, d);
	}
}

void keypress(XEvent *e) {
	keypressed(e->xkey, keys);
}

void buttonpress(XEvent *e) {
	int i;
	for (i = 0; i < LEN(buttons); i++) {
		if (CLEANMASK(buttons[i].mask) == CLEANMASK(e->xbutton.state) &&
				buttons[i].function 
				&& buttons[i].button == e->xbutton.button) {
			buttons[i].function(buttons[i].arg);
		}
	}
}

void mappingnotify(XEvent *e) {
	fprintf(stderr, "Mapping notify!\n");
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys();
}

int xerror(__attribute((unused)) Display *dis, XErrorEvent *ee) {
	if ((ee->error_code == BadAccess && (ee->request_code == X_GrabKey
					||	ee->request_code == X_GrabButton))
			|| (ee->error_code == BadMatch 
				&& (ee->request_code == X_SetInputFocus
					||	ee->request_code == X_ConfigureWindow))
			|| (ee->error_code == BadWindow)) {
		focus(desktops[current].current, &desktops[current]);
		return 0;
	}
	err(EXIT_FAILURE, "XError: request: %d code %d", 
			ee->request_code, ee->error_code);
}

void killclient(struct Arg arg) {
	if (desktops[current].current) 
		sendkillsignal(desktops[current].current->win);
}

void spawn(struct Arg arg) {
	if (fork() == 0) {
		if (fork() == 0) {
			if(dis)
				close(ConnectionNumber(dis));

			setsid();
			execvp((char*)arg.com[0],(char**)arg.com);
		}
		exit(0);
	}
}

int main(int argc, char **argv) {
	XEvent ev; 

	if(!(dis = XOpenDisplay(NULL)))
		die("Cannot open display!");

	setup();

	while(!bool_quit && !XNextEvent(dis,&ev))
		if(events[ev.type])
			events[ev.type](&ev);

	XCloseDisplay(dis);

	return 0;
}
