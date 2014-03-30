/*
 *
 * Functions for managing windows, changing the master area, changing the focus (except for 
 * follow mouse which is in events (as enternotify)) and so on.
 *
 */

void next_win() {
    client *c;

    if(current != NULL && head != NULL) {
        if(current->next == NULL)
            c = head;
        else
            c = current->next;

        current = c;
        update_current();
    }
}

void prev_win() {
    client *c;

    if(current != NULL && head != NULL) {
        if(current->prev == NULL)
            for(c=head;c->next;c=c->next);
        else
            c = current->prev;

        current = c;
        update_current();
    }
}

void add_window(Window w) {
    client *c,*t;

    if(!(c = (client *)calloc(1,sizeof(client))))
        die("Error calloc!");

    XSelectInput(dis, w, 
            EnterWindowMask|
            FocusChangeMask|
            PropertyChangeMask|
            StructureNotifyMask|
            ResizeRedirectMask|
            ExposureMask);

    c->next = NULL;
    c->win = w;

    if(head == NULL) {
        c->prev = NULL;
        head = c;
    } else {
        for(t=head;t->next;t=t->next);

        c->prev = t;
        t->next = c;
    }

    current = c;

    update_client(c);
    if (c->x == 0 && c->y == 0 && c->w == sw && c->h == sh)
        c->bw = 0;
    else
        c->bw = 1;

    grabbuttons(c);
}

void remove_window(Window w) {
    desktop *d, *ds;
   
    save_desktop(current_desktop);
    ds = current_desktop;
    for (d = head_desktop; d; d = d->next) {
        select_desktop(d);

        if (remove_window_from_current(w)) {
            layout();
            update_current();
        }
           
        save_desktop(d);
    }
    select_desktop(ds);
}

int remove_window_from_current(Window w) {
    client *c;

    c = client_from_window(w);

    if (c == NULL) { // Could not find window.
        return 0;
    }
    else if (c->prev == NULL && c->next == NULL) {
        head = NULL;
        current = NULL;
    }
    else if (c->next == NULL) {
        c->prev->next = NULL;
        if (current == c)
            current = c->prev;
    }
    else if (c->prev == NULL) {
        head = c->next;
        c->next->prev = NULL;
    }
    else {
        c->next->prev = c->prev;
        c->prev->next = c->next;
    }

    if (current == c)
        current = c->next;

    free(c);

    return 1;
}

client* client_from_window(Window w) {
    client *c;

    for (c = head; c && c->win != w; c = c->next);

    return c;
}

void update_client(client *c) {
    XWindowAttributes wa;

    if (!XGetWindowAttributes(dis, c->win, &wa))
        return;

    c->ox = c->x;
    c->oy = c->y;
    c->ow = c->w;
    c->oh = c->h;

    c->x = wa.x;
    c->y = wa.y;
    c->w = wa.width;
    c->h = wa.height;
}
