#b3cnt-floating

b3cnt-floating is a simple and lightweight stacking window manager.

It is a fork of [b3cnt](https://github.com/mytch444/b3cnt) but now
the only real simularity is simplicity. It takes a lot of ideas from
cwm. Which I've used for about twenty minutes.

It supports multiple desktops, multiple monitors, key sub-maps (for
example M-p p: sends the focused window to bottom of stack), click
to focus.

I have no idea what the name means. It's an acronym for something but
I have long since forgotten what.

##Installation

    $ $EDITOR config.h
    $ make
    # make install

##Bugs
	- Thunderbird seems to have some problems. I'm not entirely sure
	what they are but when sending email if you change desktop you
	get pulled back once it has sent then if you try change focus
	with keys it crashes b3cnt-floating. I'm working on it. To fix
	this I just stoped using thunderbird. Sylpheed is much better.
