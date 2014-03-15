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
 
        for (m = monitors; m; m = m->next)
            monitor_count++;
     
        int count = 0;
        XineramaScreenInfo *info = NULL;

        if (!(info = XineramaQueryScreens(dis, &count)))
            die("Error xineramaQueryScreens!");

        if (count > monitor_count) {
            fprintf(stderr, "Adding monitors\n");

            m = NULL;
            if (monitors) 
                for (m = monitors; m->next; m = m->next);

            for (i = monitor_count; i < count; ++i) {
                if(!(new = (monitor *)calloc(1,sizeof(monitor))))
                    die("Error calloc!");

                new->sw = info[i].width;
                new->sh = info[i].height;
                new->sx = info[i].x_org;
                new->sy = info[i].y_org;
                
                new->prev = m;
                new->next = NULL;
                
                if (m)
                    m->next = new;
                else {
                    monitors = new;
                }
                
                m = new;
            }
        } else if (count < monitor_count) {
            fprintf(stderr, "Removing monitors\n");
            if (count < 1)
                die("A monitor could help");

            for (m = monitors, i = 0; m; m = m->next, i++) {
                if (i >= monitor_count) {
                    m->prev->next = NULL;
                    break;
                }
            }
        }

        sw = 0;
        sh = 0;
        for (m = monitors, i = 0; m; m = m->next, i++) {
            m->sw = info[i].width;
            m->sh = info[i].height;
            m->sx = info[i].x_org;
            m->sy = info[i].y_org;

            sw += m->sw;
            sh += m->sh;
        }

        XFree(info);
    }
}
