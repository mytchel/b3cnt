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

    c->bw = 1;

    if(head == NULL) {
        c->prev = NULL;
        head = c;
    } else {
        for(t=head;t->next;t=t->next);

        c->prev = t;
        t->next = c;
    }

    current = c;

    actually_update_client(c);
    grabbuttons(c);
}

void remove_window(Window w) {
    monitor *m, *ms;
    desktop *d, *ds;
    
    save_monitor();
    ms = current_monitor; 
    for (m = head_monitor; m; m = m->next) {
        select_monitor(m);
        ds = current_desktop;
        for (d = m->head_desktop; d; d = d->next) {
            select_desktop(d);

            if (remove_window_from_current(w)) {
                layout();
                update_current();
            }
           
            save_desktop(d);
        }
        select_desktop(ds);
        save_monitor();
    }
    select_monitor(ms);
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
    if (mode != FLOATING) // I don't care what size you are if you arn't cool enough to do what the master wants.
        return;

    actually_update_client(c);
}



void actually_update_client(client *c) {
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
