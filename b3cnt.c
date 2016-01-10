/*
 * Copyright (c) 2015, Mytchel Hammond, mytchel at openmailbox dot org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
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
#include <X11/extensions/Xrandr.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/XF86keysym.h>

#define LEN(X)	  (sizeof(X)/sizeof(*X))
#define CLEANMASK(mask) (mask & ~(numlockmask | LockMask))
#define ROOTMASK   PropertyChangeMask|ButtonPressMask \
                 | SubstructureRedirectMask \
                 | SubstructureNotifyMask
#define SUBMASK    EnterWindowMask 

enum { RESIZE, MOVE };

typedef struct Arg Arg;
typedef struct Key Key;
typedef struct Button Button;
typedef struct Client Client;
typedef struct Desktop Desktop;

struct Arg {
	char** com;
	int i;
	Key *map;
};

struct Key {
	unsigned int mod;
	KeySym keysym;
	void (*function)(Client*, Desktop*, Arg);
	Arg arg;
};

struct Button {
	unsigned int mask;
	unsigned int button;
	void (*function)(Client*, Desktop*, Arg);
	Arg arg;
};

struct Client {
	Client *next, *prev;
	int x, y, w, h;
	int full_height, full_width;
	Window win;
};

struct Desktop {
	Client *head;
	Desktop *next, *prev;
};

static void changedesktop(Client *c, Desktop *d, Arg arg);
static void clienttodesktop(Client *c, Desktop *d, Arg arg);
static void focusold(Client *c, Desktop *d, Arg arg);
static void sendtoback(Client *c, Desktop *d, Arg arg);
static void fullwidth(Client *c, Desktop *d, Arg arg);
static void fullheight(Client *c, Desktop *d, Arg arg);
static void fullscreen(Client *c, Desktop *d, Arg arg);
static void mousemove(Client *c, Desktop *d, Arg arg);
static void mouseresize(Client *c, Desktop *d, Arg arg);

/* Listens for keys in a submap. arg.map is the map. Stays in map until exited
 * if arg.i is non zero.
 */

static void submap(Client *c, Desktop *d, Arg arg);

/* Special function to exit submaps. Does nothing else. */
static void exitsubmap(Client *c, Desktop *d, Arg arg);

static void spawn(Client *c, Desktop *d, Arg arg);
static void killclient(Client *c, Desktop *d, Arg arg);
static void quit();

#include "config.h"

static void setup();
static void die(char* e);
static unsigned long getcolor(char *color, int screen);
static void sendkillsignal(Window w);
static void sigchld(int unused);

static void monitorholdingclient(Client *c, int *mx, int *my, 
	int *mw, int *mh);

static int wintoclient(Window w, Client **c, Desktop **d);

static void updatefocus(Desktop *d);

static void addwindow(Window w, Desktop *d);
/* adds c after a on Desktop d. if a is null then sets c to be head. */
static void addclient(Client *c, Client *a, Desktop *d);
static void removeclient(Client *c, Desktop *d);
static void removewindow(Window w);
static Client *lastclient(Desktop *d);
static void updateclientdata(Client *c);
static void updateclientwin(Client *c);

static void mousemotion(int t);
static void grabkeys();
static int keypressed(XKeyEvent ke, Key *map);
static int iskeymod(KeySym keysym);

static void handleevent(XEvent *e);

static void destroynotify(XEvent *e);
static void maprequest(XEvent *e);
static void unmapnotify(XEvent *e);
static void keypress(XEvent *e);
static void buttonpress(XEvent *e);
static void configurerequest(XEvent *e);
static void clientmessage(XEvent *e);
static void screenchangenotify(XEvent *e);

static int xerror(__attribute((unused)) Display *dis, XErrorEvent *ee);

static Display *dis;
static Window root;
static int screen, bool_quit, numlockmask;
static unsigned int win_unfocus, win_focus;
static Desktop desktops[DESKTOP_NUM];
static int current;

enum { WM_PROTOCOLS, WM_DELETE_WINDOW, WM_COUNT };
enum { NET_SUPPORTED, NET_WM_STATE, NET_FULLSCREEN, NET_ACTIVE, NET_COUNT };

static Atom wmatoms[WM_COUNT], netatoms[NET_COUNT];
static int modifiers[] = {0, LockMask, 0, LockMask };
	
static int have_rr, rr_event_base, rr_error_base;
	
void setup() {
	int k, j;
	sigchld(0);

	bool_quit = False;

	screen = DefaultScreen(dis);
	root = RootWindow(dis,screen);

	win_focus = getcolor(FOCUS, screen);
	win_unfocus = getcolor(UNFOCUS, screen);

	XModifierKeymap *modmap = XGetModifierMapping(dis);
	for (k = 0; k < 8; k++) 
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[modmap->max_keypermod*k + j] 
				== XKeysymToKeycode(dis, XK_Num_Lock))
				numlockmask = (1 << k);
	XFreeModifiermap(modmap);
	
	modifiers[2] = numlockmask;
	modifiers[3] |= numlockmask;
	
	wmatoms[WM_PROTOCOLS]
	    = XInternAtom(dis, "WM_PROTOCOLS", False);
	wmatoms[WM_DELETE_WINDOW] 
	    = XInternAtom(dis, "WM_DELETE_WINDOW", False);
	
	netatoms[NET_SUPPORTED]
	    = XInternAtom(dis, "_NET_SUPPORTED", False);
	netatoms[NET_WM_STATE]
	    = XInternAtom(dis, "_NET_WM_STATE", False);
	netatoms[NET_FULLSCREEN]
	    = XInternAtom(dis, "_NET_WM_STATE_FULLSCREEN", False);
	netatoms[NET_ACTIVE]
	    = XInternAtom(dis, "_NET_ACTIVE_WINDOW", False);

	XChangeProperty(dis, root, netatoms[NET_SUPPORTED], XA_ATOM, 32,
	         PropModeReplace, (unsigned char *)netatoms, NET_COUNT);

	XSelectInput(dis, root, ROOTMASK);
	XSetErrorHandler(xerror);
	
	have_rr = XRRQueryExtension(dis, &rr_event_base, &rr_error_base);
	if (have_rr) {
		XRRSelectInput(dis, root, RRScreenChangeNotifyMask);
	}

	grabkeys();

	XSync(dis, False);
}

void quit() {
	bool_quit = 1;
}

void killclient(Client *c, Desktop *d, Arg arg) {
	if (c) sendkillsignal(c->win);
}

void spawn(Client *c, Desktop *d, Arg arg) {
	if (fork() == 0) {
		if (dis) close(ConnectionNumber(dis));
		
		setsid();
		execvp(arg.com[0], arg.com);
	}
}

void changedesktop(Client *c, Desktop *d, Arg arg) {
	if (arg.i < 0 || arg.i > DESKTOP_NUM || arg.i == current)
		return;

	for (c = desktops[arg.i].head; c; c = c->next) 
		XMapWindow(dis, c->win);

	XChangeWindowAttributes(dis, root, CWEventMask, 
			&(XSetWindowAttributes)
			{.do_not_propagate_mask = SubstructureNotifyMask});

	for (c = d->head; c; c = c->next) 
		XUnmapWindow(dis, c->win);

	/* Now ok to listen to events.*/
	XChangeWindowAttributes(dis, root, CWEventMask, 
			&(XSetWindowAttributes) {.event_mask = ROOTMASK});
	
	current = arg.i;
	updatefocus(&desktops[current]);
}

void clienttodesktop(Client *c, Desktop *d, Arg arg) {
	if (!c) return;
	removeclient(c, d);
	addclient(c, lastclient(&desktops[arg.i]), &desktops[arg.i]);
	changedesktop(NULL, d, arg);
}

void fullwidth(Client *c, Desktop *d, Arg arg) {
	if (!c) return;
	c->full_width = !c->full_width;
	updateclientwin(c);
}

void fullheight(Client *c, Desktop *d, Arg arg) {
	if (!c) return;
	c->full_height = !c->full_height;
	updateclientwin(c);
}

void fullscreen(Client *c, Desktop *d, Arg arg) {
	if (!c) return;
	if (c->full_width && c->full_height)
		c->full_width = c->full_height = 0;
	else
		c->full_width = c->full_height = 1;
	updateclientwin(c);
}

void focusold(Client *c, Desktop *d, Arg arg) {
	if (c && c->prev) {
		removeclient(c, d);
		addclient(c, c->prev->prev, d);
		updatefocus(d);
	}
}

void sendtoback(Client *c, Desktop *d, Arg arg) {
	if (!c) return;
	XLowerWindow(dis, c->win);
	
	removeclient(c, d);
	addclient(c, NULL, d);
	
	updatefocus(d);
}

void mousemove(Client *c, Desktop *d, Arg arg) {
	mousemotion(MOVE);
}

void mouseresize(Client *c, Desktop *d, Arg arg) {
	mousemotion(RESIZE);
}

void mousemotion(int t) {
	int rx, ry, xw, yh, dc;
	int mx, my, mw, mh;
	unsigned int v;
	Window w, wr;
	XWindowAttributes wa;
	XEvent ev;
	Client *c; Desktop *d;

	if (!XQueryPointer(dis, root, &w, &wr, &rx, &ry, &dc, &dc, &v) 
			|| !wintoclient(wr, &c, &d)) return;

	if (!XGetWindowAttributes(dis, c->win, &wa)) return;

	if (t == RESIZE) {
		XWarpPointer(dis, c->win, c->win, 0, 0, 0, 0, 
			wa.width, wa.height);
		rx = wa.x + wa.width;
		ry = wa.y + wa.height;
	}

	if (XGrabPointer(dis, root, False, 
		ButtonPressMask|ButtonReleaseMask|PointerMotionMask, 
		GrabModeAsync, GrabModeAsync, None, None, CurrentTime)
			 != GrabSuccess) return;

	monitorholdingclient(c, &mx, &my, &mw, &mh);
	if (c->full_width) {
		c->full_width = False;
		c->x = mx - BORDER_WIDTH;
		c->w = mw;
	}
	if (c->full_height) {
		c->full_height = False;
		c->y = my - BORDER_WIDTH;
		c->h = mh;
	}

	updateclientwin(c);
	
	do {
		XMaskEvent(dis, ButtonPressMask|ButtonReleaseMask
			|PointerMotionMask|SubstructureRedirectMask, &ev);

		if (ev.type == MotionNotify) {
			xw = (t == MOVE ? wa.x : wa.width - BORDER_WIDTH * 2) 
				+ ev.xmotion.x - rx;

			yh = (t == MOVE ? wa.y : wa.height - BORDER_WIDTH * 2)
				+ ev.xmotion.y - ry;

			if (t == RESIZE) {
				XResizeWindow(dis, c->win, 
						xw > MIN ? xw : MIN,
						yh > MIN ? yh : MIN);
			} else if (t == MOVE) {
				XMoveWindow(dis, c->win, xw, yh);
			}

		} else if (ev.type == ConfigureRequest || ev.type == MapRequest)
			handleevent(&ev);

	} while (ev.type != ButtonRelease);

	XUngrabPointer(dis, CurrentTime);
	updateclientdata(c);
}

void updatefocus(Desktop *d) {
	Client *c;
	int i;
	
	for (c = d->head; c && c->next; c = c->next) {
		for (i = 0; i < LEN(modifiers); i++)
			XGrabButton(dis, AnyButton, modifiers[i], c->win,
			       False, ButtonPressMask, GrabModeAsync, 
			       GrabModeAsync, None, None);
		XSetWindowBorder(dis, c->win, win_unfocus);
	}

	if (c) { /* Last client */
		XRaiseWindow(dis, c->win);
		XSetWindowBorder(dis, c->win, win_focus);
		XSetInputFocus(dis, c->win, RevertToPointerRoot, 
			       CurrentTime);
    		XChangeProperty(dis, root,
    		         netatoms[NET_ACTIVE], XA_WINDOW, 32,
    		         PropModeReplace, (unsigned char *) &c->win, 1);

		for (i = 0; i < LEN(modifiers); i++)
			XUngrabButton(dis, AnyButton, modifiers[i], c->win);
	} else {
		XDeleteProperty(dis, root, netatoms[NET_ACTIVE]);
	}
}

void updateclientdata(Client *c) {
	XWindowAttributes window_attributes;

	if (!c) return;

	if (!XGetWindowAttributes(dis, c->win, &window_attributes)) {
		fprintf(stderr, "Failed XGetWindowAttributes!\n");
		return;
	}

	c->x = window_attributes.x;
	c->y = window_attributes.y;
	c->w = window_attributes.width > MIN ? window_attributes.width : MIN;
	c->h = window_attributes.height > MIN ? window_attributes.height : MIN;
}

void updateclientwin(Client *c) {
	int x, y, w, h;
	
	if (c->full_width && c->full_height) {
		XSetWindowBorderWidth(dis, c->win, 0);
/*    		XChangeProperty(dis, c->win, netatoms[NET_WM_STATE], XA_ATOM, 32,
	        	PropModeReplace, (unsigned char*) &netatoms[NET_FULLSCREEN], 1);
*/	} else {
		XSetWindowBorderWidth(dis, c->win, BORDER_WIDTH);
/*    		XChangeProperty(dis, c->win, netatoms[NET_WM_STATE], XA_ATOM, 32,
		        PropModeReplace, 0, 0);
*/	}
	
	monitorholdingclient(c, &x, &y, &w, &h);

	if (!c->full_height) { y = c->y; h = c->h;}
	if (!c->full_width) { x = c->x; w = c->w;}
	
	XMoveResizeWindow(dis, c->win, x, y, w, h);
}

void monitorholdingclient(Client *c, int *mx, int *my, int *mw, int *mh) {
	XineramaScreenInfo *info = NULL;
	int count, i;
	int cx = c->x + c->w / 2;
	int cy = c->y + c->h / 2;
	
	if (XineramaIsActive(dis)) {
		if (!(info = XineramaQueryScreens(dis, &count))) {
			*mx = *my = 0;
			*mw = XDisplayWidth(dis, screen);
			*mh = XDisplayHeight(dis, screen);
			return;
		}

		if (count < 1)
			die("Error no screens.");

		for (i = 0; i < count; i++) {
			*mx = info[i].x_org;
			*my = info[i].y_org;
			*mw = info[i].width;
			*mh = info[i].height;
			if (cx >= *mx && cx <= *mx + *mw &&
				cy >= *my && cy <= *my + *mh)
				goto clean;
		}
	}

	*mx = *my = 0;
	*mw = *mh = -1;

clean:
	XFree(info);	
}

void addwindow(Window w, Desktop *d) {
	int i, mi;
	Window win_away; /* Don't care about this. */
	int rx, ry, dont_care;
	unsigned int v;
	Client *c;
	int mx, my, mw, mh;

	if(!(c = malloc(sizeof(Client)))) {
		fprintf(stderr, "Error malloc!");
		return;
	}

	XSelectInput(dis, w, SUBMASK);

	for (mi = 0; mi < LEN(modifiers); mi++) 
		for (i = 0; i < LEN(buttons); i++)
			XGrabButton(dis, buttons[i].button, 
				buttons[i].mask|modifiers[mi], w,
				False, ButtonPressMask,
				GrabModeAsync, GrabModeAsync, None, None);

	c->win = w;
	c->full_width = False;
	c->full_height = False;
	updateclientdata(c);

	if (XQueryPointer(dis, root, &win_away, &win_away, &rx, &ry,
	                  &dont_care, &dont_care, &v)) {
		c->x = rx - c->w / 2;
		c->y = ry - c->h / 2;
	}

	XSetWindowBorderWidth(dis, c->win, BORDER_WIDTH);
	
	if (d->head)
		XSetWindowBorder(dis, lastclient(d)->win, win_unfocus);

	monitorholdingclient(c, &mx, &my, &mw, &mh);
	if (c->x < mx)
		c->x = mx;
	if (c->x + c->w > mx + mw)
		c->x = mx + mw - c->w - BORDER_WIDTH * 2 - 1;
	if (c->y < my)
		c->y = my;
	if (c->y + c->h > my + mh)
		c->y = my + mh - c->h - BORDER_WIDTH * 2 - 1;

	updateclientwin(c);
	
	addclient(c, lastclient(d), d);
	updatefocus(d);
}

Client *lastclient(Desktop *d) {
	Client *c;
	for (c = d->head; c && c->next; c = c->next);
	return c;
}

void removewindow(Window w) {
	Client *c; Desktop *d;
	if (wintoclient(w, &c, &d)) {
		removeclient(c, d);
		free(c);
		if (d == &desktops[current])
			updatefocus(d);
	}
}

void addclient(Client *n, Client *a, Desktop *d) {
	if (!d->head) {
		d->head = n;
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
}

void removeclient(Client *c, Desktop *d) {
	if (d->head == c)
		d->head = c->next;
	if (c->prev)
		c->prev->next = c->next;
	if (c->next)
		c->next->prev = c->prev;
}

int wintoclient(Window w, Client **c, Desktop **d) {
	int i;

	for (i = 0; i < DESKTOP_NUM; i++) {
		*d = &desktops[i];
		for (*c = (*d)->head; *c; *c = (*c)->next) {
			if ((*c)->win == w)
				return True;
		}
	}

	return False;
}

void grabkeys() {
	int i, m;
	KeyCode code;

	for (i = 0; keys[i].function != NULL; i++) {
		if (!(code = XKeysymToKeycode(dis, keys[i].keysym))) continue;
		for (m = 0; m < LEN(modifiers); m++)
			XGrabKey(dis, code, keys[i].mod|modifiers[m], 
				root, True, GrabModeAsync, GrabModeAsync);
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
				map[i].function(lastclient(&desktops[current]),
					 &desktops[current], map[i].arg);
		}
	}
	return True;
}

void exitsubmap(Client *c, Desktop *d, Arg arg) {}

int iskeymod(KeySym keysym) {
	int i;
	KeySym mods[] = {XK_Shift_L, XK_Shift_R, XK_Control_L, XK_Control_R,
		XK_Meta_L, XK_Meta_R, XK_Alt_L, XK_Alt_R};
	for (i = 0; i < LEN(mods); i++)
		if (mods[i] == keysym) return True;
	return False;
}

void submap(Client *c, Desktop *d, Arg arg) {
	Key *map = arg.map;
	XEvent ev;
	KeySym keysym;

	if (!XGrabKeyboard(dis, root, True, GrabModeAsync, GrabModeAsync,
				CurrentTime) == GrabSuccess) return;

	while (True) {
		XNextEvent(dis, &ev);
		if (ev.type == KeyPress) {
			keysym = XLookupKeysym(&(ev.xkey), 0);

			if (iskeymod(keysym)) continue;

			if (!keypressed(ev.xkey, map) || !arg.i)
				break;
		} else if (ev.type == ConfigureRequest || ev.type == MapRequest)
			handleevent(&ev);
	}

	XUngrabKeyboard(dis, CurrentTime);
}

void maprequest(XEvent *e) {
	XMapRequestEvent *ev = &e->xmaprequest;
	Client *c; Desktop *d;

	XMapWindow(dis, ev->window);
	if (!wintoclient(ev->window, &c, &d)) {
		addwindow(ev->window, &desktops[current]);
	} else if (d != &desktops[current]) {
		int i;
		for (i = 0; d != &desktops[i]; i++);
		struct Arg arg = { .i = i };
		changedesktop(NULL, &desktops[current], arg);
	}
}

void unmapnotify(XEvent *e) {
	removewindow(e->xunmap.window);
}

void destroynotify(XEvent *e) {
	removewindow(e->xdestroywindow.window);
}

void configurerequest(XEvent *e) {
	Client *c; Desktop *d;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	
	XMoveResizeWindow(dis, ev->window, ev->x, ev->y,
			ev->width, ev->height);

	if (ev->detail == Above)
		XRaiseWindow(dis, ev->window);
	else if (ev->detail == Below)
		XLowerWindow(dis, ev->window);

	if (wintoclient(ev->window, &c, &d)) {
		if (ev->detail == Above) {
			removeclient(c, d);
			addclient(c, lastclient(d), d);
		} else if (ev->detail == Below) {
			removeclient(c, d);
			addclient(c, NULL, d);
		}
		
		if (d == &desktops[current])
			updatefocus(d);
		updateclientdata(c);
	}
}

void keypress(XEvent *e) {
	keypressed(e->xkey, keys);
}

void buttonpress(XEvent *e) {
	int i;
	XButtonEvent *ev = &e->xbutton;	
	Client *c; Desktop *d;

	/* You cant click a window that I have that
	 * isn't in the current desktop right? */
	if (wintoclient(ev->window, &c, &d)) {
		removeclient(c, d);
		addclient(c, lastclient(d), d);
		updatefocus(d);
	} else return;

	for (i = 0; i < LEN(buttons); i++) {
		if (buttons[i].function 
			&& CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state)
			&& buttons[i].button == ev->button) {
			buttons[i].function(c, d, buttons[i].arg);
		}
	}
}

void clientmessage(XEvent *e) {
	Client *c; Desktop *d;
	if (!wintoclient(e->xclient.window, &c, &d))
		return;
		
	if (e->xclient.message_type == netatoms[NET_WM_STATE]) {
	    if (e->xclient.data.l[1] == netatoms[NET_FULLSCREEN] ||
	        e->xclient.data.l[2] == netatoms[NET_FULLSCREEN]) {
	        fullscreen(c, d, (Arg) {.i = 0});        
	    }
	} else if (e->xclient.message_type == netatoms[NET_ACTIVE]) {
		if (d == &desktops[current]) {
			removeclient(c, d);
			addclient(c, lastclient(d), d);
			updatefocus(d);
		}
	}
}

void screenchangenotify(XEvent *e) {
	int i, mx, my, mw, mh;
	Client *c;
	XRRScreenChangeNotifyEvent *ev = (XRRScreenChangeNotifyEvent *) e;
	printf("screenchangenotify %i %i %i %i\n", ev->width, ev->height, ev->mwidth, ev->mheight);
	fflush(stdout);


	for (i = 0; i < DESKTOP_NUM; i++) {
		for (c = desktops[i].head; c; c = c->next) {
			monitorholdingclient(c, &mx, &my, &mw, &mh);
			if (mw == -1 && mh == -1) {
				printf("client not in a monitor, moving\n");
				fflush(stdout);
				c->x = 0;
				c->y = 0;
				updateclientwin(c);
			}
		}
	}
}

int xerror(__attribute((unused)) Display *dis, XErrorEvent *ee) {
	fprintf(stderr, "b3cnt caught error: request %d code %d\n",
		ee->request_code, ee->error_code);
	return 0;
}

unsigned long getcolor(char *color, int screen) {
	XColor c; Colormap map = DefaultColormap(dis, screen);
	if (!XAllocNamedColor(dis, map, color, &c, &c))
		die("Cannot allocate color.");
	return c.pixel;
}

void die(char* e) {
	fprintf(stderr, "b3cnt: %s\n",e);
	exit(1);
}

void sendkillsignal(Window w) {
	XEvent ke;
	ke.type = ClientMessage;
	ke.xclient.window = w;
	ke.xclient.format = 32;
	ke.xclient.message_type = wmatoms[WM_PROTOCOLS];
	ke.xclient.data.l[0]    = wmatoms[WM_DELETE_WINDOW];
	ke.xclient.data.l[1]	= CurrentTime;
	XSendEvent(dis, w, False, NoEventMask, &ke);
}

void sigchld(int unused) {
	if(signal(SIGCHLD, sigchld) == SIG_ERR)
		die("Can't install SIGCHLD handler");
	while(0 < waitpid(-1, NULL, WNOHANG));
}

void handleevent(XEvent *ev) {
	switch (ev->type) {
	case MapRequest:
		maprequest(ev);
		break;
	case UnmapNotify:
		unmapnotify(ev);
		break;
	case DestroyNotify:
		destroynotify(ev);
		break;
	case ConfigureRequest:
		configurerequest(ev);
		break;
	case KeyPress:
		keypress(ev);
		break;
	case ButtonPress:
		buttonpress(ev);
		break;
	case ClientMessage:
		clientmessage(ev);
		break;
	default:
		if (have_rr) {
			if (ev->type == rr_event_base + RRScreenChangeNotify) {
				screenchangenotify(ev);
				break;
			}
		}
		break;
	}
}

int main(int argc, char **argv) {
	XEvent ev; 

	if (!(dis = XOpenDisplay(NULL)))
		die("Cannot open display!");

	setup();

	while (!bool_quit && !XNextEvent(dis, &ev))
		handleevent(&ev);

	XCloseDisplay(dis);

	return 0;
}
