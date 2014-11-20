/*
 *  Copyright (c) 2010, Rinaldini Julien, julien.rinaldini@heig-vd.ch
 *  Copyright (c) 2014, Mytchel Hammond, mytchel at openmailbox dot org
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

// Number of steps to be taken for the animation when changing desktops.
#define ANIMATION_STEPS 1000
// The number of desktops.
#define DESKTOP_NUM     7
#define BORDER_WIDTH    3
#define FOCUS           "#ffffff"
#define UNFOCUS         "#666666"

char* menucmd[]     = {"selectexec", NULL};
char* termcmd[]     = {"st", "-e", "tmux", NULL};
char* batterycmd[]  = {"showbattery", NULL};
char* timecmd[]     = {"showtime", NULL};

// You will need this at the end of key arrays.
#define END {0,0,NULL,{NULL}}

static struct key rootmap[] = {
  { 0,                  XK_o,      shiftfocus,     {.i = 1}},
  { ShiftMask,          XK_o,      shiftfocus,     {.i = -1}},
  { 0,                  XK_i,      focusprev,      {NULL}},
  
  { 0,                  XK_n,      shiftwindow,    {.i = 1}},
  { 0,                  XK_p,      shiftwindow,    {.i = -1}},
  { 0,                  XK_space,  bringtotop,     {NULL}},

  { 0,                  XK_l,      changedesktop,  {.i = 1}}, 
  { ShiftMask,          XK_l,      clienttodesktop,{.i = 1}}, 
  { 0,                  XK_h,      changedesktop,  {.i = -1}}, 
  { ShiftMask,          XK_h,      clienttodesktop,{.i = -1}}, 
  
  { 0,                  XK_equal,  fullwidth,      {NULL}},
  { ShiftMask,          XK_equal,  fullheight,     {NULL}},
  { 0,                  XK_f,      fullscreen,     {NULL}},
  { 0,                  XK_b,      toggleborder,   {NULL}},

  { 0,                  XK_t,      spawn,          {.com = timecmd}},

  {0,                   XK_Escape, exitsubmap,     {NULL}},
  {ControlMask,         XK_g,      exitsubmap,     {NULL}},
   
  END
};

static struct key keys[] = {
  { Mod1Mask,               XK_p,           submap,         {.map = rootmap}},
  { Mod1Mask|ShiftMask,     XK_backslash,   updatemonitors, {.i = 0}},

  { Mod1Mask|ControlMask,   XK_Return,      spawn,          {.com = termcmd}}, 
  { Mod1Mask|ShiftMask,     XK_slash ,      spawn,          {.com = menucmd}}, 
  
  { Mod1Mask|ControlMask,   XK_x,           killclient,     {NULL}},
  
  
  { Mod1Mask|ControlMask|ShiftMask,XK_q,   quit,            {NULL}},

  END
};

static struct button buttons[] = {
  { Mod1Mask,           Button1,    mousemove,      {NULL}},
  { Mod1Mask,           Button2,    mouseresize,    {NULL}},
  { Mod1Mask,           Button3,    mouseresize,    {NULL}},
};


