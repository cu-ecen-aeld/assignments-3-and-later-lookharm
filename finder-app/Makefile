CROSS_COMPILE =

ifeq ($(CROSS_COMPILE),aarch64-none-linux-gnu-)
	CC = aarch64-none-linux-gnu-gcc 
endif

build: writer.c
	$(CC) -o writer writer.c

.PHONY: clean
clean:
	rm -rf writer.o writer
