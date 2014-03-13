/*
 *
 * Functions for creating and destroying windows (or not windows in the case of create)
 *
 */

void kill_client() {
    if(current)
        send_kill_signal(current->win);
}

void spawn(const struct Arg arg) {
    if(fork() == 0) {
        if(fork() == 0) {
            if(dis)
                close(ConnectionNumber(dis));

            setsid();
            execvp((char*)arg.com[0],(char**)arg.com);
        }
        exit(0);
    }
}
