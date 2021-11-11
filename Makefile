# todo: OS specific targets

CC = gcc
OPTFLAGS = -fomit-frame-pointer -fno-strict-aliasing -fno-aggressive-loop-optimizations -fconserve-stack -fmerge-constants -ffast-math
CRT = src/wincrt.c
LINKER_ENTRY = -e _start
WORKER_SOURCES = src/worker.c src/common.c src/win.c

all: debug

release:
	$(CC) -std=c11 $(LINKER_ENTRY) $(WORKER_SOURCES) $(CRT) \
	-o termite-worker -nostartfiles -nostdlib \
	$(OPTFLAGS) -ftree-vectorize -mavx2 -flto -Os \
	-lkernel32 -Wall -Wextra -pedantic

debug:
	$(CC) -std=c11 $(LINKER_ENTRY) $(WORKER_SOURCES) $(CRT) \
	-o termite-worker -nostartfiles -nostdlib -g \
	$(OPTFLAGS) -lkernel32 -Wall -Wextra -pedantic
