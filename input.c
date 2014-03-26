/*
 *
 * Functions for keyboard input once events has handed them over to this.
 *
 */

void grabkeys() {
    int i;
    KeyCode code;

    for (i = 0; keys[i].function != NULL; i++) {
        if ((code = XKeysymToKeycode(dis, keys[i].keysym))) {
            XGrabKey(dis, code, keys[i].mod, root, True, GrabModeAsync, GrabModeAsync);
        }
    }
}

int grabkeyboard() {
    int i;
    for (i = 0; i < 100; i ++) {
        if (XGrabKeyboard(dis, root, True, GrabModeAsync, GrabModeAsync, CurrentTime) == GrabSuccess)
            return 1;
        usleep(1000);
    }
    return 0;
}

void releasekeyboard() {
    XUngrabKeyboard(dis, CurrentTime);
}

int keyismod(KeySym keysym) {
    if (keysym == XK_Shift_L ||
            keysym == XK_Shift_R ||
            keysym == XK_Control_L || 
            keysym == XK_Control_R || 
            keysym == XK_Meta_L || 
            keysym == XK_Meta_R || 
            keysym == XK_Alt_L || 
            keysym == XK_Alt_R 
       )
        return 1;
    return 0;
}

void submap(struct Arg arg) {
    XEvent ev;
    KeySym keysym;

    if (!grabkeyboard())
        return;

    while (1) {
        XNextEvent(dis, &ev);
        if (ev.type != KeyPress) {
            if (ev.type == ConfigureRequest || ev.type == MapRequest)
                events[ev.type](&ev);

            continue;
        }
        
        keysym = XLookupKeysym(&(ev.xkey), 0);

        if (keysym == XK_Escape)
            break;

        if (keyismod(keysym))
            continue;

        key_pressed(ev.xkey, arg.map);
        break;
    }

    releasekeyboard();
}

void stickysubmap(struct Arg arg) {
    XEvent ev;
    KeySym keysym;

    if (!grabkeyboard())
        return;
    
    while (1) {
        XNextEvent(dis, &ev);
        if (ev.type != KeyPress) {
            if (ev.type == ConfigureRequest || ev.type == MapRequest)
                events[ev.type](&ev);

            continue;
        }
        
        keysym = XLookupKeysym(&(ev.xkey), 0);

        if (keysym == XK_Escape)
            break;

        if (keyismod(keysym))
            continue;

        key_pressed(ev.xkey, arg.map);
    }

    releasekeyboard();
}

int key_pressed(XKeyEvent ke, key *map) {
    int i;
    KeySym keysym = XLookupKeysym(&ke, 0);
    
    for (i = 0; map[i].function != NULL; i++) {
        if (map[i].keysym == keysym && map[i].mod == ke.state) {
            map[i].function(map[i].arg);
            return 1;
        }
    } 

    return 0;
}

void grabbuttons(client *c) {
    int i;

    for (i = 0; buttons[i].function; i++) {
        XGrabButton(dis, buttons[i].button, buttons[i].mask, c->win,
                False, ButtonPressMask|ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
    }
}

void mousemotion(struct Arg arg) {
    XWindowAttributes wa;
    XEvent ev;
    int rx, ry, c, xw, yh;
    unsigned int v;
    Window w;

    if (!XQueryPointer(dis, root, &w, &w, &rx, &ry, &c, &c, &v))
        return;

    current = client_from_window(w);
    current->bw = 1;

    update_current();

    if (!current || !XGetWindowAttributes(dis, current->win, &wa))
        return;

    if (arg.i == RESIZE)
        XWarpPointer(dis, current->win, current->win, 0, 0, 0, 0, wa.width, wa.height);

    if (!XQueryPointer(dis, root, &w, &w, &rx, &ry, &c, &c, &v))
        return;

    if (XGrabPointer(dis, root, False, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync,
                GrabModeAsync, None, None, CurrentTime) != GrabSuccess) return;

    do {
        XMaskEvent(dis, ButtonPressMask|ButtonReleaseMask|PointerMotionMask|SubstructureRedirectMask, &ev);
        if (ev.type == MotionNotify) {
            xw = (arg.i == MOVE ? wa.x : wa.width) + ev.xmotion.x - rx;
            yh = (arg.i == MOVE ? wa.y : wa.height) + ev.xmotion.y - ry;

            if (arg.i == RESIZE)
                XResizeWindow(dis, current->win,
                        xw > MINWSZ ? xw : MINWSZ, yh > MINWSZ ? yh : MINWSZ);

            else if (arg.i == MOVE)
                XMoveWindow(dis, current->win, xw, yh);

        } else if (ev.type == ConfigureRequest || ev.type == MapRequest)
            events[ev.type](&ev);

    } while (ev.type != ButtonRelease);

    update_client(current);

    XUngrabPointer(dis, CurrentTime);
}
