/*
 *
 * Functions for dealing with desktops.
 *
 */

void save_desktop(desktop *d) {
    d->head = head;
    d->current = current;
}

void select_desktop(desktop *d) {
    head = d->head;
    current = d->current;
    current_desktop = d;
}

void change_desktop(const struct Arg arg) {
    client *c;

    if (current_desktop == desktop_from_index(arg.i))
        return;

    save_desktop(current_desktop);

    for(c = head; c; c = c->next) {
        update_client(c);
    //    XMoveResizeWindow(dis, c->win, sw, sh, 1, 1);
        XUnmapWindow(dis, c->win);
    }
    
    // Take "properties" from the new desktop
    select_desktop(desktop_from_index(arg.i));
 
    for(c = head; c; c = c->next) {
        XMapWindow(dis, c->win);
    }

    layout();
    update_current();
}

void client_to_desktop(const struct Arg arg) {
    client *tmp = current;
    desktop *tmp2 = current_desktop;

    if (current == NULL)
        return;

    remove_window(current->win);
    save_desktop(tmp2);

    // Add client to desktop
    select_desktop(desktop_from_index(arg.i));
    add_window(tmp->win);
    save_desktop(desktop_from_index(arg.i));

    select_desktop(tmp2);

    change_desktop(arg);
}

// If you ask for a desktop with a index greater than what I have I will return the
// last one. Not sure if this is a good idea.
desktop* desktop_from_index(int i) {
    desktop *d;
    int j;

    for (j = 0, d = head_desktop; j < i && d; d = d->next, j++);
    return d;
}

void init_desktops() {
    int i;
    desktop *d, *n;

    for (i = 0, d = NULL; i < desktop_num; i++) {
        if(!(n = (desktop *)calloc(1,sizeof(desktop))))
            die("Error calloc!");

        n->current = NULL;
        n->head = NULL;
        n->prev = d;
        n->next = NULL;

        if (d) {
            d->next = n;
        } else {
            current_desktop = n;
            head_desktop = n;
        }

        d = n; 
    }
}
