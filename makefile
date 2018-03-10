CC = gcc
CFLAGS = -g3 -Wall -Werror
CFLAGS += -I.
LDFLAGS = -L.
LIBS = -lpthread

APPS = qsort bank_transfer reenter_test micro_func raw_socket parse_url
CLEAN_APPS = $(APPS:%=%_clean)

all: $(APPS)

clean: $(CLEAN_APPS)

qsort: %:%.o
	$(CC) $< -o $@

qsort.o: %.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

qsort_clean:
	rm -f qsort.o qsort

micro_func: %:%.o
	$(CC) $< -o $@

micro_func.o: %.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

micro_func_clean:
	rm -f micro_func.o micro_func

bank_transfer: %:%.o
	$(CC) $(LDFLAGS) $(LIBS) $^ -o $@

bank_transfer.o: %.o:%.c
	$(CC) -c $(CFLAGS) $^ -o $@

bank_transfer_clean:
	rm -f bank_transfer.o bank_transfer

reenter_test: %:%.o link_list.o
	$(CC) $(LDFLAGS) $(LIBS) $^ -o $@

reenter_test.o: %.o:%.c
	$(CC) -c $(CFLAGS) $^ -o $@

reenter_test_clean: link_list_clean
	rm -f reenter_test.o reenter_test

link_list.o: %.o:%.c
	$(CC) -c $(CFLAGS) $^ -o $@

link_list_clean:
	rm -f link_list.o

raw_socket: %:%.o
	$(CC) $(LDFLAGS) $(LIBS) $^ -o $@

raw_socket.o: %.o:%.c
	$(CC) -c $(CFLAGS) $^ -o $@

raw_socket_clean:
	rm -f raw_socket.o raw_socket

parse_url: %:%.o
	$(CC) $(LDFLAGS) $(LIBS) $^ -o $@

parse_url.o: %.o:%.c
	$(CC) -c $(CFLAGS) $^ -o $@

parse_url_clean:
	rm -f parse_url.o parse_url
