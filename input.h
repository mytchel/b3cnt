/*
 *
 * Functions for keyboard input once events has handed them over to this.
 *
 */

static void grabkeys();
static int grabkeyboard();
static void releasekeyboard();
static int keyismod(KeySym keysym);

static void submap(struct Arg arg);
static void stickysubmap(struct Arg arg);

static void grabbuttons(client *c);
static void mousemotion(struct Arg arg);
