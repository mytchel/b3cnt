/*
 *
 * Event handling. If it's an xorg event it should be here. If it's not yell at me
 * with as much profanity as humanly possible and I may change it.
 *
 */

static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static void destroynotify(XEvent *e);
static void maprequest(XEvent *e);
static void unmapnotify(XEvent *e);
static void enternotify(XEvent *e);
static void keypress(XEvent *e);
static void buttonpress(XEvent *e);
