/*
 *
 * Functions for dealing with desktops.
 *
 */

static void save_desktop(desktop *d);
static void select_desktop(desktop *d);
static desktop* desktop_from_index(int i);
static void change_desktop(const struct Arg arg);
static void client_to_desktop(const struct Arg arg);
static void init_desktops(); 
