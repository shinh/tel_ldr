# Tiny ELF loader

## Introduction

This is a failed submission for IOCCC 2013.

This is a tiny dynamic linker/loader for ELF. This loads programs
built on Linux, and it runs on Linux, Mac OSX, Cygwin, and possibly on
other OS. thanks to its Linux/glibc emulation layer. This means you
can run Linux programs on other OSes. This works only for x86. This
program is similar to how WINE works.

## Usage

Note that all commands below assumes you have all of my submission in
the current directory.

### Usage for Linux and Mac OSX

    $ make
    $ ./elf bin/hello
    $ ./elf bin/i386-tcc-32  # help is shown

To compile something with TinyCC (http://tinycc.org/), which is based
on a former IOCCC winning entry (http://www0.us.ioccc.org/2001/bellard.c),
you need to set up your environment by mkenv.sh. This script downloads
two debian packages and extracts it to set up "linux" directory which
contains include files and object files for TinyCC. This script
requires curl, ar, tar, and perl.

    $ ./mkenv.sh
    $ ./elf bin/i386-tcc-32 -E ./hello.c
    $ ./elf bin/i386-tcc-32 ./hello.c -o hello-tcc
    $ ./elf ./hello-tcc

You can compile more complex programs with the TCC loaded by this ELF
loader. For example, let's compile the source code of TCC itself.

    $ curl -L -O http://download.savannah.gnu.org/releases/tinycc/tcc-0.9.26.tar.bz2  # or wget
    $ tar -xvjf tcc-0.9.26.tar.bz2
    $ cd tcc-0.9.26
    $ ./configure
    $ cd ..
    $ ./elf bin/i386-tcc-32 -o i386-tcc-32-tcc tcc-0.9.26/tcc.c -DONE_SOURCE -DTCC_TARGET_I386 -DCONFIG_SYSROOT='"linux"' -DCONFIG_TCCDIR='"linux/tcc"' -g -O2 -m32 -lm -ldl
    $ ./elf ./i386-tcc-32-tcc

Of course, you can load the TCC built by the original TCC.

    $ ./elf ./i386-tcc-32-tcc -o i386-tcc-32-tcc-tcc tcc-0.9.26/tcc.c -DONE_SOURCE -DTCC_TARGET_I386 -DCONFIG_SYSROOT='"linux"' -DCONFIG_TCCDIR='"linux/tcc"' -g -O2 -m32 -lm -ldl
    $ ./elf ./i386-tcc-32-tcc-tcc  # this still works

### Usage for Cygwin

See also usage for Linux and Mac as well.

Unfortunately, Cygwin does not support MAP_FIXED for 4k boundaries so
we need to use special Linux binaries whose segments are aligned to
64k boundaries.

    (cygwin) $ tar -xvzf for_cygwin.tgz
    (cygwin) $ make
    (cygwin) $ ./elf bin/hello-aligned
    (cygwin) $ ./elf bin/i386-tcc-32-aligned  # help is shown

You can build Linux binaries with i386-tcc-32-aligned, but you cannot
run the output because it is not aligned properly. However, you can
run the output on Linux.

    (cygwin) $ ./mkenv.sh
    (cygwin) $ ./elf bin/i386-tcc-32-aligned -E ./hello.c
    (cygwin) $ ./elf bin/i386-tcc-32-aligned ./hello.c -o hello-tcc-win
    (cygwin) $ ./elf ./hello-tcc-win  # mmap fails
    (linux) $ ./hello-tcc-win  # works

You can reproduce the -aligned binaries by using align.lds.

    (linux) $ gcc -m32 hello.c -Wl,-Talign.lds -o hello-aligned

### Chain load

You can load this loader itself.

    $ ./elf bin/elf-linux bin/hello
    $ ./elf bin/elf-linux bin/i386-tcc-32

For Cygwin, please use hello-aligned and i386-tcc-32-aligned instead.

Of course, on Linux and Mac, you still can run programs built by TCC
chain-loaded by this loader loaded by this loader.

    $ ./elf bin/elf-linux bin/i386-tcc-32 ./hello.c -o hello-tcc
    $ ./elf bin/elf-linux ./hello-tcc

You can reproduce elf-linux by

    (linux) $ gcc -m32 -g -Wall -W elf.o -rdynamic -ldl -Wl,-Ttext-segment=0x3000000 -Wl,-Talign.lds -o elf-linux

As you see, the start address of elf-linux was adjusted for Linux, and
the alignment of elf-linux was adjusted for Cygwin.

Note that you cannot load elf-linux twice, because the address layout
of elf-linux is fixed.

    $ ./elf bin/elf-linux bin/elf-linux  # fails

### Add Linux only APIs

This loader cannot run arbitrary Linux binaries on other OSes mainly
because its Linux emulation layer lacks a lot of functions. However,
you can easily add such functions. For example, see the following
C code:

    #include <stdio.h>
    #include <string.h>
    int main() {
      char buf[] = "hello";
      memfrob(buf, 5);
      puts(buf);
      return 0;
    }

This code uses memfrob, which is a glibc-only function, and this will
not work on Mac or Cygwin.

    $ ./elf ./memfrob  # linux only

However, by providing the implementation of memfrob in elf.c, you can
run this program on Mac or Cygwin. Please add the following code at
the bottom of elf.c:

    void* memfrob(void* v, size_t n) {
      char* p = (char*)v;
      while (n--) {
        *p++ ^= 42;
      }
      return v;
    }

    $ make
    $ ./elf ./memfrob  # now it works on everywhere!

## Obfuscation techniques

### ASCII arts

The code itself provides some ideas about what code does. The first
three letters, 'E', 'L', and 'F', are just some preprocessor
directives and some data. The face of elf is Linux emulation layer.

Then, the next box which has four cells explains how ELF objects look
like. An ELF object always starts with an ELF header. The code around
the first cell actually parses the ELF headers. Notice the string in
the cell ("ELF Header") is used as a part of the error message.

    $ ./elf hello.c  # not ELF Header

Then, multiple program headers follow. You see the following for-loop
at the top of the second cell.

    for(K=E+=13;K<E+E[-2]%65536*8;K+=8){

This is the loop which handles program headers. Then, next line starts
with

    if(*K==1)

The code in this if-clause handles PT_LOAD (==1).

At the top of the 4th cell, you will see

    if(*K==2)

The code after this handles PT_DYNAMIC (==2).

### Compactness

Another notable characteristic of this code is its
compactness. elf-tiny.c is the compressed version of this program,
which has no error checks. elf-tiny.c has only less than 1000
bytes. I'd claim this is the tiniest ELF loader in the world, but it
just works on multiple OSes:

    $ make elf-tiny
    $ ./elf-tiny ./hello

To achieve this extreme compactness, a number of techniques are
used. One good example is

    1[(I*)O]

at the top of the 4th cell. This <index>[<array>] style is well known
obfuscation technique, but this code uses this style because this is
shorter than

    ((I*)O)[1]

or

    *((I*)O+1)

Another example is magic numbers like 7417633*159. This is 0x464c457f,
which is the magic of ("\x7fELF") in little endian.

Finally, the following code snippet is one of my favorite in this
program:

    O=strstr(T,H=*((char**)D[6]+M/256*4)+D[5]),G=O?U[(O-T)/6]:Y(0,H)

This obtains an address of a symbol from its name. Do you see how it
works? Why is strstr for T necessary?

## Philosophy

This entry focuses on a fairly overlooked tool, the dynamic linking
loader. I wanted to show how compact the code of dynamic loaders can
be by implementing the loader in less than 1000 bytes. Another goal of
this entry was to show how useful the dynamic loaders can be, by
allowing users to run Linux binaries on another OS.

One more thing this entry would demonstrate is the portability of x86.
