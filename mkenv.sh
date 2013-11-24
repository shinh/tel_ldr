#!/bin/sh

set -ex

extract_deb() {
    local url="$1"
    local deb=$(basename $url)
    if [ ! -e "$deb" ]; then
        curl -O $url || wget $url
    fi
    ar x $deb
    tar -xvzf data.tar.gz
}

mkdir -p linux
cd linux

extract_deb http://ftp.debian.org/debian/pool/main/e/eglibc/libc6_2.11.3-4_i386.deb
extract_deb http://ftp.debian.org/debian/pool/main/e/eglibc/libc6-dev_2.11.3-4_i386.deb
extract_deb http://ftp.debian.org/debian/pool/main/l/linux-2.6/linux-libc-dev_2.6.32-48squeeze4_i386.deb
tar -xvzf ../tcc.tgz

perl -i -p -e 's@ /@ '$(pwd)/'@g' usr/lib/libc.so

cd usr/lib
for so in *.so; do
    set +e
    to=$(readlink $so)
    r=$?
    set -e
    if [ $r = 0 ]; then
        ln -sf ../..$to $so
    fi
done
