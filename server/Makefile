CROSS_COMPILE =

ifeq ($(CROSS_COMPILE),aarch64-none-linux-gnu-)
	CC = aarch64-none-linux-gnu-gcc 
endif

all: aesdsocket.c
	$(CC) -o aesdsocket aesdsocket.c

.PHONY: clean
clean:
	rm -rf aesdsocket.o aesdsocket
