CC = gcc
OPTFLAGS = -fomit-frame-pointer -fno-strict-aliasing -fno-aggressive-loop-optimizations -fconserve-stack -fmerge-constants

all:
	$(CC) main.c -g -o termite -nostartfiles -nostdlib -ftree-vectorize -mavx2 -march=native $(OPTFLAGS) -lkernel32 -Wall -Wextra
