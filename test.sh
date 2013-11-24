#!/bin/sh

set -ex

if [ "x$@" = "x" ]; then
    el=./el
else
    el="$@"
fi

make

$el bin/hello
$el bin/stdout
$el bin/args foo bar baz
#$el i386-tcc-32
$el bin/i386-tcc-32 stdout.c -o stdout-tcc
$el stdout-tcc

if [ $(uname) = Darwin ]; then
    $el bin/elf-linux bin/hello
    $el bin/elf-linux bin/args foo bar baz
    $el bin/elf-linux bin/i386-tcc-32 stdout.c -o stdout-tcc2
    $el stdout-tcc2
fi

echo PASS
