
CC?=gcc
CFLAGS?=-O2
CFLAGS+=-Isrc
#CFLAGS+=-Wall -Wwrite-strings -pedantic -std=gnu99
LDFLAGS+=-pthread
LDLIBS=

NDS_OBJS=src/commandline.o src/conf.o \
	src/debug.o src/main.o src/hack_thread.o src/safe.o src/util.o

.PHONY: all clean install

all: hackdream hackcli

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

hackdream: $(NDS_OBJS) 
	$(CC) $(LDFLAGS) -o hackdream $+ $(LDLIBS)

hackcli: src/hackcli.o
	$(CC) $(LDFLAGS) -o hackcli $+ $(LDLIBS)
	

clean:
	rm -f hackream src/*.o
	rm -rf dist

install:
	strip hackdream
	mkdir -p $(DESTDIR)/usr/bin/
	cp hackdream $(DESTDIR)/usr/bin/
