b3cnt
=====

b3cnt is a very simple and lightweight tiling window manager. It started as a fork of 
catwm <https://github.com/pyknite/catwm> and currently allows for space between windows
(like bspwm) and strings of key combinations (C-b o) like emacs with more planned for adding.

Status
------
 * 23.02.14 -> No longer uses a config file. I thought that 500 lines of code just for that
    was a bit excessive. I may have another go at it some other time.
 * 09.02.14 -> Now uses a config file, badly.
 * 06.02.14 -> Found a new name!!!
 * 29.01.14 -> Removed floating as I have yet to actually use it.
 * 11.01.14 -> Added Xinerama support, floating windows and some other things.
 * 07.01.14 -> Added key strings and border space.

from catwm
 * 05.07.19 -> v0.3. Multiple desktops and correct some bugs // is it ment to be 19?
 * 30.06.10 -> v0.2. Back again \o/
 * 15.03.10 -> v0.2. The wm is functional -> I only use this wm!
 * 24.02.10 -> First release, v0.1. In this release 0.1, the wm is almost functional

Modes
-----

There are no modes anymore. There is just floating and a fullscreeen toggle that works for the individual window.

Installation
------------

You will need Xlib,
then:

    $ vim config.h # And change to your liking.
    $ make
    $ make install

Bugs
----
 * Probebly the same as <https://github.com/mytch444/b3cnt>, possibly with some other annoying things.

Todo
----
 * Add a way of focusing windows so that an out of focus window can be infront of the current one.
 
If you have some particular request, just send me an e-mail, and I will see for it!
