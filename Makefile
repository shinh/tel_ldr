UNAME=$(shell uname)

CFLAGS=-g -Wall -W -ansi
LDFLAGS+=-rdynamic
ifeq ($(UNAME),Linux)
	LDFLAGS+=-ldl
	LDFLAGS+=-Wl,-Ttext-segment=0x2000000
endif

all: el elg elf elf-tiny elf-clean elf-clean64

test: el elg
	./test.sh
	./test.sh ./elg

elf.c: elsf.c gen_elf.rb elf.txt
	ruby gen_elf.rb

els.c: elg.c gen_els.rb
	ruby gen_els.rb

%64.o: %.c Makefile
	$(CC) $(CFLAGS) -ansi -c $< -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) -m32 -ansi -c $< -o $@

elf-clean64: elf-clean64.o
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@
el: el.o
	$(CC) $(CFLAGS) -m32 $< $(LDFLAGS) -o $@
elg: elg.o
	$(CC) $(CFLAGS) -m32 $< $(LDFLAGS) -o $@
elf: elf.o
	$(CC) $(CFLAGS) -m32 $< $(LDFLAGS) -o $@
elsf: elsf.o
	$(CC) $(CFLAGS) -m32 $< $(LDFLAGS) -o $@
elf-tiny: elf-tiny.o
	$(CC) $(CFLAGS) -m32 $< $(LDFLAGS) -o $@
elf-clean: elf-clean.o
	$(CC) $(CFLAGS) -m32 $< $(LDFLAGS) -o $@

clean:
	rm -f el elg elf *.o
