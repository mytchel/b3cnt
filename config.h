/*
 *  Copyright (c) 2010, Rinaldini Julien, julien.rinaldini@heig-vd.ch
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef CONFIG_H
#define CONFIG_H

// Mod (Mod1 == alt) and master size
#define MOD             Mod1Mask
#define BORDER_SPACE    0
// The number of desktops.
#define DESKTOP_NUM     5 
// Set to 0 to disable (messages or follow_mouse).
#define MESSAGES        0
/*
 * It should be warned that enabling follow_mouse causes some crashes when closing some
 * windows (chromium, haven't noticed it with anything else. I'm working on it but other 
 * than that it works fine.
 *
 * It seems to be caused by xsetinputfocus in update_current but I can't get any further.
 */
#define FOLLOW_MOUSE    0

#define MASTER_SIZE     0.5

// Colors
#define FOCUS           "rgb:aa/50/e0"
#define UNFOCUS         "rgb:33/33/33"

char* dmenucmd[]  = {"dmenu_run", NULL};
char* termcmd[]   = {"st", NULL};
char* webcmd[]    = {"chromium", NULL};
char* timecmd[]   = {"showtime", NULL};

// Avoid multiple paste
#define DESKTOPCHANGE(K,N) { MOD,  K,  change_desktop, {.i = N}}, { MOD|ShiftMask,  K,    client_to_desktop, {.i = N}}

#define END {0,0,NULL,{.i = 0}}
#define ENDB {0, 0, NULL, {.i = 0}}

static struct key rootmap[] = {
    { 0,               XK_k,      kill_client,     {NULL}},

    { 0,               XK_o,      next_win,        {NULL}},
    { ShiftMask,       XK_o,      prev_win,        {NULL}},

    { 0,               XK_c,      spawn,           {.com = termcmd}},
    { 0,               XK_f,      spawn,           {.com = webcmd}},
    { 0,               XK_a,      spawn,           {.com = timecmd}},
    { ShiftMask,       XK_1,      spawn,           {.com = dmenucmd}},

    { 0,               XK_space,  expand_window,  {NULL}},

    { 0,               XK_comma,  prev_monitor,   {NULL}},
    { 0,               XK_period, next_monitor,   {NULL}},

    { MOD,             XK_q,      quit,            {NULL}},

    END
};

// Shortcuts
static struct key keys[] = {
    { MOD,           XK_p,        submap,  {.map = rootmap}},

    DESKTOPCHANGE(   XK_1,        0),
    DESKTOPCHANGE(   XK_2,        1),
    DESKTOPCHANGE(   XK_3,        2),
    DESKTOPCHANGE(   XK_4,        3),
    DESKTOPCHANGE(   XK_5,        4),
    /*DESKTOPCHANGE(   XK_6,        5),
    DESKTOPCHANGE(   XK_7,        6),
    DESKTOPCHANGE(   XK_8,        7),
    DESKTOPCHANGE(   XK_9,        8),
    DESKTOPCHANGE(   XK_0,        9),
*/
    END
};

static struct button buttons[] = {
    { MOD,               Button1,   mousemotion,  {.i = MOVE}},
    { MOD|ShiftMask,     Button1,   mousemotion,  {.i = RESIZE}},

    ENDB
};

#endif

