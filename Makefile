CC = gcc
OPTFLAGS = -fomit-frame-pointer -fno-strict-aliasing -fno-aggressive-loop-optimizations -fconserve-stack -fmerge-constants
NOCRT = nocrt0/nocrt0c.c
LINKER_ENTRY = -e mainCRTStartup

all: debug

release:
	$(CC) src/*.c $(NOCRT) -o termite-worker -nostartfiles -nostdlib -ftree-vectorize -mavx2 -flto $(OPTFLAGS) -Os -lkernel32 -Wall -Wextra -pedantic $(LINKER_ENTRY)

debug:
	$(CC) src/*.c $(NOCRT) -g -o termite-worker -nostartfiles -nostdlib $(OPTFLAGS) -lkernel32 -Wall -Wextra -pedantic $(LINKER_ENTRY)
