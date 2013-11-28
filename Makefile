UNAME=$(shell uname)

CFLAGS=-m32 -g -Wall -W -ansi
LDFLAGS+=-rdynamic
ifeq ($(UNAME),Linux)
	LDFLAGS+=-ldl
	LDFLAGS+=-Wl,-Ttext-segment=0x2000000
endif

all: el elg elf elf-tiny elf-clean

test: el elg
	./test.sh
	./test.sh ./elg

elf.c: elsf.c gen_elf.rb elf.txt
	ruby gen_elf.rb

els.c: elg.c gen_els.rb
	ruby gen_els.rb

%.o: %.c Makefile
	$(CC) $(CFLAGS) -ansi -c $< -o $@

el: el.o
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@
elg: elg.o
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@
elf: elf.o
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@
elsf: elsf.o
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@
elf-tiny: elf-tiny.o
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@
elf-clean: elf-clean.o
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

clean:
	rm -f el elg elf *.o
