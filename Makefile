DESTDIR?=
PREFIX?=/usr

all: b3cnt-floating

b3cnt-floating: b3cnt.c config.h
	gcc -o b3cnt-floating b3cnt.c -lX11 -lXinerama

.PHONY:
install: all
	install -Dm 755 b3cnt-floating ${DESTDIR}${PREFIX}/bin/b3cnt-floating

.PHONY:
uninstall: 
	rm ${DESTDIR}${PREFIX}/bin/b3cnt-floating

.PHONY:
clean:
	rm -f b3cnt-floating
