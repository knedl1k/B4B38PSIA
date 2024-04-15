# GNU General Public License v3.0
# @knedl1k

.PHONY: all

CC=gcc
CFLAGS+=-std=gnu17 -Wall -Wextra -pedantic -g -O3 #-Werror
#LFLAGS+=-lpthread
LDFLAGS+=-static

SENDER_SOURCES+=sender.c
RECEIVE_SOURCES+=receiver.c
SENDER_TARGET+=send.bin
RECEIVE_TARGET+=rec.bin

ARCHIVE_TARGET=hw01

all: compile-sender compile-receiver

cs compile-sender:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SENDER_SOURCES) -o $(SENDER_TARGET) $(LFLAGS)

cr compile-receiver:
	$(CC) $(CFLAGS) $(LDFLAGS) $(RECEIVE_SOURCES) -o $(RECEIVE_TARGET) $(LFLAGS)

cl clean:
	rm *.bin *.zip *.tar.xz

zip:
	zip $(ARCHIVE_TARGET).zip $(CFILES) $(HFILES)

tar:
	tar -czvf $(ARCHIVE_TARGET).tar.xz $(CFILES) $(HFILES)
