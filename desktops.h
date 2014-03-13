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

// I'm not sure if this one wants to go here as it's more of a helper function
// for monitor stuff but oh well. It shall now be excluded from both groups with 
// this warning.
static void init_desktops(monitor *mon); 
