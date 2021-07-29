#!/bin/bash
#
# Statically built Linux OpenSSL binaries for use with legacy Bitcoin code
# (Roll your own...)
# Author: opsinphark@GitHub

TIMESTAMP="$(date +"%Y%m%d%H%M%S")"

HOST="x86_64-linux-gnu"
HOST_CFLAGS="-O2 -pipe"
HOST_CXXFLAGS="-O2 -pipe"

pkg=openssl
pkg_version=1.1.1k
pkg_dl_path=https://www.openssl.org/source
pkg_file_name=$pkg-$pkg_version.tar.gz
pkg_sha256_hash=892a0875b9872acd04a9fde79b1f943075d5ea162415de3047c327df33fbaee5
pkg_cflags="$HOST_CFLAGS"
pkg_cxxflags="$pkg_cflags"

base_dir=$(pwd)

source_dir=$base_dir/sources
mkdir -p $source_dir

work_dir=$base_dir/work
mkdir -p $work_dir

staging_dir=$base_dir/staging/$pkg-$pkg_version-static
mkdir -p $staging_dir

builds_dir=$base_dir/builds
mkdir -p $builds_dir

# Fetch and verify source
#cd $source_dir
#wget "$pkg_dl_path/$pkg_file_name"
#echo "$pkg_sha256_hash $pkg_file_name" | sha256sum -c

# Extract archive
cd $work_dir
tar xvfz $source_dir/$pkg_file_name

# Configure
cd $pkg-$pkg_version
CFLAGS=$pkg_cflags \
CXXFLAGS=$pkg_cxxflags \
./Configure \
--prefix=$staging_dir \
	linux-x86_64 \
        no-camellia \
        no-capieng \
        no-cast \
        no-comp \
        no-dso \
        no-dtls1 \
        no-gost \
        no-heartbeats \
        no-idea \
        no-md2 \
        no-mdc2 \
        no-rc4 \
        no-rc5 \
        no-rdrand \
        no-rfc3779 \
        no-sctp \
        no-seed \
        no-shared \
        no-ssl-trace \
        no-ssl3 \
        no-unit-test \
        no-weak-ssl-ciphers \
        no-whirlpool \
        no-zlib \
        no-zlib-dynamic
make depend
make install

# Archive build
cd $builds_dir

tar cvfJ $pkg-$pkg_version-$HOST-static-$TIMESTAMP.tar.xz $staging_dir/
sha256sum $pkg-$pkg_version-$HOST-static-$TIMESTAMP.tar.xz >> $pkg-$pkg_version-$HOST-static-$TIMESTAMP.tar.xz.sha256

# Cleanup
cd $base_dir
rm -rf staging_dir
rm -rf work_dir
