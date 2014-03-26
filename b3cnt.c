/*
 *  Copyright (c) 2010, Rinaldini Julien, julien.rinaldini@heig-vd.ch
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <X11/extensions/Xinerama.h>

#define LEN(X)            (sizeof(X)/sizeof(*X))

#define MOVE 0
#define RESIZE 1
#define MINWSZ 20

typedef struct key key;
typedef struct button button;
typedef struct client client;
typedef struct desktop desktop;
typedef struct monitor monitor;

struct Arg {
    char** com;
    key *map;
    int i;
};

struct key {
    unsigned int mod;
    KeySym keysym;
    void (*function)(struct Arg arg);
    struct Arg arg;
};

struct button {
    unsigned int mask;
    unsigned int button;
    void (*function)(struct Arg arg);
    struct Arg arg;
};

struct client {
    client *next;
    client *prev;

    unsigned int bw;
    int x, y, w, h;
    int ox, oy, ow, oh;

    Window win;
};

struct desktop {
    client *head;
    client *current;

    desktop *prev;
    desktop *next;
};

struct monitor {
    int sw;
    int sh;
    int sx;
    int sy;

    monitor *prev;
    monitor *next;
};

static void setup();
static void start();
static void die(const char* e);
static void quit();

static unsigned long getcolor(const char* color);

static void send_kill_signal(Window w);
static void sigchld(int unused);

#include "input.h"
#include "windows.h"
#include "events.h"
#include "monitors.h"
#include "desktops.h"
#include "layout.h"
#include "god.h"

// Include the config file (which I may do away with soon)
#include "config.h"

// Variables
static Display *dis;
static Window root;
static int screen;
static int bool_quit;

// Preferences
static unsigned int win_focus;
static unsigned int win_unfocus;
static int followmouse;
static int bs;
static int desktop_num;

// Current monitor and desktop config.
static int sw, sh;
static client *head;
static client *current;

static desktop *head_desktop;
static desktop *current_desktop;

static monitor *monitors;

// Events array
static void (*events[LASTEvent])(XEvent *e) = {
    [KeyPress] = keypress,
    [MapRequest] = maprequest,
    [UnmapNotify] = unmapnotify,
    [DestroyNotify] = destroynotify,
    [ConfigureNotify] = configurenotify,
    [ConfigureRequest] = configurerequest,
    [EnterNotify] = enternotify,
    [ButtonPress] = buttonpress,
};

// Load other functions that need the variables here.
#include "input.c"
#include "windows.c"
#include "events.c"
#include "monitors.c"
#include "desktops.c"
#include "layout.c"
#include "god.c"

void setup() {
    // Install a signal
    sigchld(0);

    bool_quit = 0;
    head = NULL;
    current = NULL;

    // Settings
    win_focus = getcolor(FOCUS);
    win_unfocus = getcolor(UNFOCUS);
    followmouse = FOLLOW_MOUSE;
    bs = BORDER_SPACE;
    desktop_num = DESKTOP_NUM;

    // Screen and root window
    screen = DefaultScreen(dis);
    root = RootWindow(dis,screen);

    monitors = NULL;

    update_monitors();
    init_desktops();

    XSelectInput(dis, root, SubstructureRedirectMask|SubstructureNotifyMask);
    grabkeys();
}

void start() {
    XEvent ev;

    // Main loop, just dispatch events (thx to dwm ;)
    while(!bool_quit && !XNextEvent(dis,&ev)) {
        if(events[ev.type])
            events[ev.type](&ev);
    }
}

void die(const char* e) {
    fprintf(stderr,"b3cnt: %s\n",e);
    exit(1);
}

unsigned long getcolor(const char* color) {
    XColor c;
    Colormap map = DefaultColormap(dis,screen);

    if(!XAllocNamedColor(dis,map,color,&c,&c))
        die("Error parsing color!");

    return c.pixel;
}

void send_kill_signal(Window w) {
    int n, i;
    XEvent ke;
    Atom *protocols;

    if (XGetWMProtocols(dis, w, &protocols, &n) != 0) {
        for (i = n; i >= 0; i--) {
            ke.type = ClientMessage;
            ke.xclient.window = w;
            ke.xclient.message_type = XInternAtom(dis, "WM_PROTOCOLS", False);
            ke.xclient.format = 32;
            ke.xclient.data.l[0] = XInternAtom(dis, "WM_DELETE_WINDOW", False);
            ke.xclient.data.l[1] = CurrentTime;
            XSendEvent(dis, w, False, NoEventMask, &ke);
        }
    }
}

void sigchld(int unused) {
    // Again, thx to dwm ;)
    if(signal(SIGCHLD, sigchld) == SIG_ERR)
        die("Can't install SIGCHLD handler");
    while(0 < waitpid(-1, NULL, WNOHANG));
}

void quit() {
    Window root_return, parent;
    Window *children;
    int i;
    unsigned int nchildren; 
    XEvent ev;

    /*
     * if a client refuses to terminate itself,
     * we kill every window remaining the brutal way.
     * Since we're stuck in the while(nchildren > 0) { ... } loop
     * we can't exit through the main method.
     * This all happens if MOD+q is pushed a second time.
     */
    if(bool_quit == 1) {
        XUngrabKey(dis, AnyKey, AnyModifier, root);
        XDestroySubwindows(dis, root);
        fprintf(stdout, "b3cnt: Thanks for using!\n");
        XCloseDisplay(dis);
        die("forced shutdown");
    }

    bool_quit = 1;
    XQueryTree(dis, root, &root_return, &parent, &children, &nchildren);
    for(i = 0; i < nchildren; i++) {
        send_kill_signal(children[i]);
    }
    //keep alive until all windows are killed
    while(nchildren > 0) {
        XQueryTree(dis, root, &root_return, &parent, &children, &nchildren);
        XNextEvent(dis,&ev);
        if(events[ev.type])
            events[ev.type](&ev);
    }

    XUngrabKey(dis,AnyKey,AnyModifier,root);
    fprintf(stdout,"b3cnt: Thanks for using!\n");
}

int main(int argc, char **argv) {
    // Open display
    if(!(dis = XOpenDisplay(NULL)))
        die("Cannot open display!");

    // Setup env
    setup();

    // Start wm
    start();

    // Close display
    XCloseDisplay(dis);

    return 0;
}
