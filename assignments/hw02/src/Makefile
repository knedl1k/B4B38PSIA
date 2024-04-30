# GNU General Public License v3.0
# @knedl1k

.PHONY: all

CC=gcc
CFLAGS+=-std=gnu17 -Wall -Wextra -pedantic -g -O3 #-Werror
LFLAGS+=-lssl -lcrypto
#LDFLAGS+=-static

COMBINED_SOURCES+=hash.c CRC/crc32.c
SENDER_SOURCES+=sender.c
RECEIVE_SOURCES+=receiver.c
SENDER_TARGET+=send.bin
RECEIVE_TARGET+=rec.bin

ARCHIVE_TARGET=hw02
CFILES+=$(SENDER_SOURCES) $(RECEIVE_SOURCES) $(COMBINED_SOURCES)
HFILES+=packets.h hash.h CRC/checksum.h CRC/gentab32.inc

all: compile-sender compile-receiver

cs compile-sender:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SENDER_SOURCES) -o $(SENDER_TARGET) $(COMBINED_SOURCES) $(LFLAGS)

cr compile-receiver:
	$(CC) $(CFLAGS) $(LDFLAGS) $(RECEIVE_SOURCES) -o $(RECEIVE_TARGET) $(COMBINED_SOURCES) $(LFLAGS)

cl clean:
	rm -f *.bin 
	find . -name '*.zip'    -delete
	find . -name '*.tar.xz' -delete

z zip:
	zip $(ARCHIVE_TARGET).zip $(CFILES) $(HFILES)

t tar:
	tar -czvf $(ARCHIVE_TARGET).tar.xz $(CFILES) $(HFILES)
