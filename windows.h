/*
 *
 * Functions for managing windows, changing the master area, changing the focus (except for 
 * follow mouse which is in events (as enternotify)) and so on.
 *
 */

static void next_win();
static void prev_win();

static void add_window(Window w);
static void remove_window(Window w);
static int remove_window_from_current(Window w);

static client* client_from_window(Window w);
static void update_client(client *c);
static void actually_update_client(client *c);
