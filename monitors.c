/*
 *
 * Functions for handling monitors.
 *
 */

void update_monitors() {
    int i, monitor_count = 0;
    monitor *m, *new;
    desktop *d;
    client *c;

    if (XineramaIsActive(dis)) {
 
        for (m = head_monitor; m; m = m->next)
            monitor_count++;
     
        int count = 0;
        XineramaScreenInfo *info = NULL;

        if (!(info = XineramaQueryScreens(dis, &count)))
            die("Error xineramaQueryScreens!");

        if (count > monitor_count) {
            fprintf(stderr, "Adding monitors\n");

            m = NULL;
            if (head_monitor) 
                for (m = head_monitor; m->next; m = m->next);

            for (i = monitor_count; i < count; ++i) {
                if(!(new = (monitor *)calloc(1,sizeof(monitor))))
                    die("Error calloc!");

                new->sw = info[i].width;
                new->sh = info[i].height;
                new->x = info[i].x_org;
                new->y = info[i].y_org;
                
                init_desktops(new);
               
                new->prev = m;
                new->next = NULL;
                
                if (m)
                    m->next = new;
                else {
                    head_monitor = new;
                }
                
                m = new;
            }
        } else if (count < monitor_count) {
            fprintf(stderr, "Removing monitors\n");
            if (count < 1)
                die("A monitor could help");

            current_monitor = head_monitor;

            for (m = head_monitor, i = 0; m; m = m->next, i++) {
                if (i >= monitor_count) {
                    for (d = m->head_desktop; d; d = d->next) {
                        for (c = d->head; c; c = c->next) {
                            remove_window(c->win);
                            add_window(c->win);
                        }
                    }
                    m->prev->next = NULL;
                }
            }
        }

        for (m = head_monitor, i = 0; m; m = m->next, i++) {
            m->sw = info[i].width;
            m->sh = info[i].height;
            m->x = info[i].x_org;
            m->y = info[i].y_org;
        }

        XFree(info);
    }
}

void select_monitor(monitor *m) {
    current_monitor = m;

    sh = m->sh;
    sw = m->sw;
    sx = m->x;
    sy = m->y;

    head_desktop = m->head_desktop;
    current_desktop = m->current_desktop;

    select_desktop(current_desktop);
}

monitor* cycle_monitor(int d) {
   if (d == 1) {
       if (current_monitor->next)
           return current_monitor->next;
       else
           return head_monitor;
   } else if (d == -1) {
       if (current_monitor->prev)
           return current_monitor->prev;
       else {
            monitor *t;
            for (t = head_monitor; t->next; t = t->next);
            return t;
       }
   }
}

void save_monitor() {
    current_monitor->current_desktop = current_desktop;
    save_desktop(current_desktop);
    current_monitor->head_desktop = head_desktop;
}

void change_monitor_direction(struct Arg arg) {
    change_monitor(cycle_monitor(arg.i));
}

void client_to_monitor_direction(struct Arg arg) {
    client_to_monitor(cycle_monitor(arg.i));
}

void change_monitor(monitor *m) {
    client *c;

    save_monitor();

    // Unfocus the current window in the non focused monitor
    current = NULL;
    update_current();

    select_monitor(m);

    // Map all windows
    if(head != NULL)
        for(c=head;c;c=c->next)
            XMapWindow(dis,c->win);

    layout();
    update_current();
}

void client_to_monitor(monitor *m) {
    client *tmp;

    if (current == NULL)
        return;

    tmp = current;

    remove_window(tmp->win);

    change_monitor(m);

    add_window(tmp->win);

    layout();
    update_current();
}

