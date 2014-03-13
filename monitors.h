/*
 *
 * Functions for handling monitors.
 *
 */

static void update_monitors();
static void select_monitor(monitor *m);
static monitor* cycle_monitor(int d);
static void save_monitor();
static void change_monitor(monitor *m);
static void client_to_monitor(monitor *m);
static void change_monitor_direction(struct Arg arg);
static void client_to_monitor_direction(struct Arg arg);
