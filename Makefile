PREFIX?=/usr

CC?=cc

b3cnt-floating: b3cnt.c config.h
	${CC} -Wall -o $@ $< -lX11 -lXinerama

install: b3cnt-floating
	install -Dm 755 b3cnt-floating ${DESTDIR}${PREFIX}/bin/b3cnt-floating

uninstall: 
	rm ${DESTDIR}${PREFIX}/bin/b3cnt-floating

clean:
	rm -f b3cnt-floating

.PHONY: install uninstall clean
