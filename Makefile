CFLAGS+= -Wall
LDADD+= -lX11 -lXinerama
EXEC=b3cnt-floating
FILES=b3cnt.c config.h desktops.c events.c god.c input.c layout.c monitors.c windows.c
BINDIR?= /usr/bin

CC=gcc

all: $(EXEC)

$(EXEC): b3cnt.o
	$(CC) $(LDADD) b3cnt.o -o $(EXEC)

b3cnt.o: $(FILES)
	$(CC) -c b3cnt.c

install: all
	install -Dm 755 $(EXEC) $(BINDIR)/$(EXEC)

clean:
	rm -f $(EXEC) *.o
