/*
 *
 * Layout handling, not sure if update_current wants to be here now that I 
 * think of it seeing as it hasn't got much to do with layout but it can stay 
 * here for a while.
 *
 */

void layout() {
    client *c;

    for (c = head; c; c = c->next) { 
        XMoveResizeWindow(dis, c->win, c->x, c->y, c->w, c->h);
    }
}

void update_current() {
    client *c;

    for(c = head; c; c = c->next) {
        XSetWindowBorderWidth(dis,c->win,c->bw);
        if(current == c) {
            // "Enable" current window
            XSetWindowBorder(dis, c->win, win_focus);
            XSetInputFocus(dis, c->win, RevertToPointerRoot, CurrentTime);
            XRaiseWindow(dis, c->win);
        } else {
            XSetWindowBorder(dis, c->win, win_unfocus);
        }
    }
}

void expand_window() {
    monitor *m;
    if (!current)
        return;

    if (current->bw == 0) {
        current->x = current->ox;
        current->y = current->oy;
        current->w = current->ow;
        current->h = current->oh;
        current->bw = 1;
    } else {
        update_client(current);

        for (m = monitors; m; m = m->next) {
            if (current->x >= m->sx && current->y >= m->sy) {
                if (current->x <= m->sx + m->sw && current->y <= m->sy + m->sh) {
                    current->x = m->sx;
                    current->y = m->sy;
                    current->w = m->sw;
                    current->h = m->sh;
                    current->bw = 0;
                }
            }
        }
    }

    layout();
    update_current();
}
