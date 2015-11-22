PREFIX?=/usr/local

CC?=cc
CFLAGS=-I/usr/X11R6/include
LDFLAGS=-L/usr/X11R6/lib -lX11 -lXinerama

b3cnt: b3cnt.c config.h
	${CC} -Wall -o $@ $< ${CFLAGS} ${LDFLAGS}

install: b3cnt
	install -Dm 755 b3cnt ${DESTDIR}${PREFIX}/bin/b3cnt

uninstall: 
	rm ${DESTDIR}${PREFIX}/bin/b3cnt

clean:
	rm -f b3cnt

.PHONY: install uninstall clean
