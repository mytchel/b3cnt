/*
 *	Copyright (c) 2010, Rinaldini Julien, julien.rinaldini@heig-vd.ch
 *	Copyright (c) 2014, Mytchel Hammond, mytchel at openmailbox dot org
 *
 *	Permission is hereby granted, free of charge, to any person obtaining a
 *	copy of this software and associated documentation files (the "Software"),
 *	to deal in the Software without restriction, including without limitation
 *	the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *	and/or sell copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be included in
 *	all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *	DEALINGS IN THE SOFTWARE.
 *
 */

// Number of steps to be taken for the animation when changing desktops.
#define ANIMATION_STEPS 1000
// The number of desktops.
#define DESKTOP_NUM		7
#define BORDER_WIDTH	2
#define FOCUS			"#ffffff"
#define UNFOCUS			"#666666"

char* menucmd[]     = {"selectexec", NULL};
char* wincmd[]      = {"selectwindow", NULL};
char* termcmd[]     = {"st", "-e", "tmux", NULL};
char* batterycmd[]  = {"showbattery", NULL};
char* timecmd[]     = {"showtime", NULL};
char* lockcmd[]     = {"slock", NULL};
char* pausecmd[]    = {"mmusicd", "pause", NULL};
char* alsacmd[]     = {"st", "-e", "alsamixer", NULL};

// You will need this at the end of key arrays.
#define END {0,0,NULL,{NULL}}

#define DESKTOP(key, num) \
{ Mod1Mask, key, changedesktop, {.i = num}}, \
{ Mod1Mask|ControlMask, key, clienttodesktop, {.i = num}}

static struct Key mainmap[] = {
	{ 0,                    XK_o,      shiftfocus,     {.i = 1}},
	{ ShiftMask,            XK_o,      shiftfocus,     {.i = -1}},
	{ 0,                    XK_i,      focusprev,      {NULL}},

	{ 0,                    XK_n,      shiftwindow,    {.i = 1}},
	{ 0,                    XK_p,      shiftwindow,    {.i = -1}},
	{ 0,                    XK_space,  bringtotop,     {NULL}},

	{ 0,                    XK_equal,  fullheight,     {NULL}},
	{ ShiftMask,            XK_equal,  fullwidth,      {NULL}},
	{ 0,                    XK_f,      fullscreen,     {NULL}},
	{ ShiftMask,            XK_b,      toggleborder,   {NULL}},

	{ 0,                    XK_t,      spawn,          {.com = timecmd}},
	{ 0,                    XK_b,      spawn,          {.com = batterycmd}},

	{ 0,                    XK_s,      spawn,          {.com = alsacmd}},

	{0,                     XK_Escape, exitsubmap,	   {NULL}},
	{ControlMask,			XK_g,      exitsubmap,     {NULL}},

	{ShiftMask,             XK_p,      spawn,          {.com = pausecmd}},

	END
};

static struct Key keys[] = {
	{ Mod1Mask,             XK_p,           submap,         {.map = mainmap}},
	{ Mod1Mask|ShiftMask,   XK_backslash,   updatemonitors, {.i = 0}},

	{ Mod1Mask|ControlMask,	XK_Return,      spawn,          {.com = termcmd}}, 
	{ Mod1Mask|ShiftMask,   XK_slash,       spawn,          {.com = menucmd}}, 

	{ Mod1Mask|ControlMask,	XK_x,           killclient,     {NULL}},

	{ Mod1Mask|ShiftMask,   XK_space,       spawn,          {.com = lockcmd}},

	DESKTOP(XK_1, 0),
	DESKTOP(XK_2, 1),
	DESKTOP(XK_3, 2),
	DESKTOP(XK_4, 3),
	DESKTOP(XK_5, 4),
	DESKTOP(XK_6, 5),
	DESKTOP(XK_7, 6),

	{ Mod1Mask|ControlMask|ShiftMask,XK_q,   quit,           {NULL}},

	END
};

static struct Button buttons[] = {
	{ Mod1Mask,         Button1,    mousemove,      {NULL}},
	{ Mod1Mask,         Button2,    spawn,          {.com = wincmd}},
	{ Mod1Mask,         Button3,    mouseresize,    {NULL}},
};


