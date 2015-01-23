CFLAGS+= -Wall
LDADD+= -lX11 -lXinerama
EXEC=b3cnt-floating
FILES=b3cnt.c config.h
BINDIR?= /usr/local/bin

CC=gcc

all: $(EXEC)

$(EXEC): b3cnt.o
	$(CC) $(LDADD) b3cnt.o -o $(EXEC)

b3cnt.o: $(FILES)
	$(CC) -c b3cnt.c

install: all
	install -Dm 755 $(EXEC) $(BINDIR)/$(EXEC)

uninstall: 
	rm $(BINDIR)/$(EXEC)

clean:
	rm -f $(EXEC) *.o
