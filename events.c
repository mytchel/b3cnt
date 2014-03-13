/*
 *
 * Event handling. If it's an xorg event it should be here. If it's not yell at me
 * with as much profanity as humanly possible and I may change it.
 *
 */

void enternotify(XEvent *e) {
    if (!followmouse)
        return;

    client *c;

    XCrossingEvent *ev = &e->xcrossing; 

    for (c = head; c; c = c->next) {
        if (ev->window == c->win) {
            current = c;
        }
    }

    update_current();
}

void maprequest(XEvent *e) {
    XMapRequestEvent *ev = &e->xmaprequest;
    client *c;

    // For fullscreen mplayer (and maybe some other programs)
    for(c = head; c; c = c->next)
        if(ev->window == c->win)
            break;

    if (!c)
        add_window(ev->window);

    XMapWindow(dis,ev->window);

    layout();
    update_current();
}

/*
 * Having some problems with this and changing desktops.
 *
 * Currently the closest to a fix I have is not unmapping the 
 * windows when changing desktops, just making them rather small
 * and hidding them away. Doesn't seem right though.
 *
 */
void unmapnotify(XEvent *e) {
    XUnmapEvent *ev = &e->xunmap;
    
    if (!ev->send_event)
        remove_window(ev->window);
}

void destroynotify(XEvent *e) {
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    remove_window(ev->window);
}

/*
   If anyone could get this working that would be great. As it is, it's unimportant.
   By work I mean make it detect when a monitor is added or removed.
   */
void configurenotify(XEvent *e) {

}

/*
 * Removing XConfigureWindow seems to stop it from crashing when running minecraft
 * (It was a one time thing I swear!!!) and probebly some other things. I shall see 
 * if it causes any problems.
 */
void configurerequest(XEvent *e) {
    // Paste from DWM, thx again \o/
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;
    wc.x = ev->x;
    wc.y = ev->y;
    wc.width = ev->width;
    wc.height = ev->height;
    wc.border_width = ev->border_width;
    wc.sibling = ev->above;
    wc.stack_mode = ev->detail;
    XConfigureWindow(dis, ev->window, ev->value_mask, &wc); // I had this commented out for some unknown reason.
}

void keypress(XEvent *e) {
    int i;
    XKeyEvent ke = e->xkey;
    KeySym keysym = XLookupKeysym(&ke, 0);

    for (i = 0; keys[i].function != NULL; i++) {
        if (keys[i].keysym == keysym && keys[i].mod == ke.state) {
            keys[i].function(keys[i].arg);
        }
    }
}

void buttonpress(XEvent *e) {
    int i;
    for (i = 0; buttons[i].function; i++) {
        if (buttons[i].function && buttons[i].mask == e->xbutton.state && buttons[i].button == e->xbutton.button) {
            buttons[i].function(buttons[i].arg);
        }
    }
}
