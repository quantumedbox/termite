# todo: OS specific targets

CC = gcc
OPTFLAGS = -fomit-frame-pointer -fno-strict-aliasing -fno-aggressive-loop-optimizations -fconserve-stack -fmerge-constants -ffast-math
NOCRT = nocrt0/nocrt0c.c
LINKER_ENTRY = -e mainCRTStartup

WORKER_SOURCES = src/worker.c src/common.c src/win.c

all: debug

release:
	$(CC) $(WORKER_SOURCES) $(NOCRT) -o termite-worker -nostartfiles -nostdlib -ftree-vectorize -mavx2 -flto $(OPTFLAGS) -Os -lkernel32 -Wall -Wextra -pedantic $(LINKER_ENTRY)

debug:
	$(CC) $(WORKER_SOURCES) $(NOCRT) -g -o termite-worker -nostartfiles -nostdlib $(OPTFLAGS) -lkernel32 -Wall -Wextra -pedantic $(LINKER_ENTRY)
