/*
 * Copyright (c) 2014, Mytchel Hammond, mytchel at openmailbox dot org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

void debug(char *msg) {
	// Comment out the next line if you don't want debug messages.
	fprintf(stderr, msg); 
}

// The number of desktops.
#define DESKTOP_NUM	9
#define BORDER_WIDTH	2
#define FOCUS		"#333333"
#define UNFOCUS		"#aaaaaa"

// Min window width/height
#define MIN		100

char *termcmd[]		= {"st", NULL};

/* scripts from nenu */
char *menucmd[]		= {"nexec", NULL};
char *wincmd[]		= {"nwindow", NULL};
char *batterycmd[]	= {"nbatt", NULL};
char *timecmd[]		= {"ntime", NULL};

char *lightin[]		= {"xbacklight", "-inc", "10", NULL};
char *lightde[]		= {"xbacklight", "-dec", "10", NULL};
char *mute[]		= {"amixer", "set", "Master", "toggle", NULL};
char *volup[]		= {"amixer", "set", "Master", "5%+", NULL};
char *voldown[]		= {"amixer", "set", "Master", "5%-", NULL};

#define DESKTOP(key, num) \
{ Mod1Mask|ControlMask, key, changedesktop, {.i = num}}, \
{ Mod1Mask|ShiftMask, key, clienttodesktop, {.i = num}}

static struct Key mainmap[] = {
	{ 0,                    XK_o,      shiftfocus,     {.i = 1}},
	{ ShiftMask,            XK_o,      shiftfocus,     {.i = -1}},
	{ 0,                    XK_i,      focusold,       {NULL}},

	{ 0,                    XK_p,      sendtoback,     {NULL}},
	
	{ 0,                    XK_equal,  fullheight,     {NULL}},
	{ ShiftMask,            XK_equal,  fullwidth,      {NULL}},
	{ 0,                    XK_f,      fullscreen,     {NULL}},
	{ 0,                    XK_f,      toggleborder,   {NULL}},
	{ ShiftMask,            XK_b,      toggleborder,   {NULL}},

	{ 0,                    XK_t,      spawn,          {.com = timecmd}},
	{ 0,                    XK_b,      spawn,          {.com = batterycmd}},

	{ 0,                    XK_Escape, exitsubmap,	   {NULL}},
	{ControlMask,           XK_g,      exitsubmap,     {NULL}},
	
	{},
};

static struct Key keys[] = {
	{ Mod1Mask,             XK_p,           submap,         {.map = mainmap, .i = 0}},

	{ Mod1Mask|ControlMask,	XK_Return,      spawn,          {.com = termcmd}}, 
	{ Mod1Mask|ShiftMask,   XK_slash,       spawn,          {.com = menucmd}}, 

	{ Mod1Mask|ControlMask,	XK_x,           killclient,     {NULL}},

	{ Mod1Mask,             XK_space,       spawn,          {.com = wincmd}},

	{ Mod1Mask,             XK_F1,          spawn,          {.com = mute}},
	{ Mod1Mask,             XK_F2,          spawn,          {.com = voldown}},
	{ Mod1Mask,             XK_F3,          spawn,          {.com = volup}},
	{ Mod1Mask,             XK_F5,          spawn,          {.com = lightde}},
	{ Mod1Mask,             XK_F6,          spawn,          {.com = lightin}},

	DESKTOP(XK_1, 0),
	DESKTOP(XK_2, 1),
	DESKTOP(XK_3, 2),
	DESKTOP(XK_4, 3),
	DESKTOP(XK_5, 4),
	DESKTOP(XK_6, 5),
	DESKTOP(XK_7, 6),

	{ Mod1Mask|ControlMask|ShiftMask,XK_q,   quit,          {NULL}},
	
	{}
};

static struct Button buttons[] = {
	{ Mod1Mask,         Button1,    mousemove,      {NULL}},
	{ Mod1Mask,         Button3,    mouseresize,    {NULL}},
	{}
};


