CC = gcc
CFLAGS = -g3 -Wall -Werror

APPS = libccalled
CLEAN_APPS = $(APPS:%=%_clean)

all: $(APPS)

clean: $(CLEAN_APPS)

libccalled: ccalled.o
	$(CC) -shared -o libccalled.so $<

ccalled.o: %.o:%.c
	$(CC) -fpic -c $(CFLAGS) $< -o $@

libccalled_clean:
	rm -f ccalled.o libccalled.so
