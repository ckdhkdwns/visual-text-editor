CC = gcc
CFLAGS = -Wall

ifeq ($(shell uname), Linux)
	CFLAGS += -DLINUX
endif
ifeq ($(shell uname), Darwin)
	CFLAGS += -DMACOS
endif
ifeq ($(OS), Windows_NT)
    CC = mingw32-gcc
	CFLAGS += -DWINDOWS
endif

all: vite

vite: main.c
	$(CC) $(CFLAGS) -o vite main.c -lcurses

clean:
	rm -f vite
