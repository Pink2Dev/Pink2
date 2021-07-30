#!/bin/bash
#
# Statically built Linux Berkeley dB binaries for use with legacy Bitcoin code
# (Roll your own...)
# Author: opsinphark@GitHub

TIMESTAMP="$(date +"%Y%m%d%H%M%S")"

HOST="x86_64-linux-gnu"
HOST_CFLAGS="-O2 -pipe"
HOST_CXXFLAGS="-O2 -pipe"

pkg=db
pkg_version=4.8.30.NC
pkg_dl_path=http://download.oracle.com/berkeley-db
pkg_file_name=$pkg-$pkg_version.tar.gz
pkg_sha256_hash=12edc0df75bf9abd7f82f821795bcee50f42cb2e5f76a6a281b85732798364ef
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
cd $source_dir
wget "$pkg_dl_path/$pkg_file_name"
echo "$pkg_sha256_hash $pkg_file_name" | sha256sum -c

# Extract archive
cd $work_dir
tar xvfz $source_dir/$pkg_file_name

# Patch
cd $pkg-$pkg_version
sed -i.old 's/__atomic_compare_exchange/__atomic_compare_exchange_db/' dbinc/atomic.h && \
sed -i.old 's/atomic_init/atomic_init_db/' dbinc/atomic.h mp/mp_region.c mp/mp_mvcc.c mp/mp_fget.c mutex/mut_method.c mutex/mut_tas.c

# Configure
cd build_unix/
CFLAGS=$pkg_cflags \
CXXFLAGS=$pkg_cxxflags \
        ../dist/configure \
        --prefix=$staging_dir \
        --enable-cxx \
        --disable-replication \
        --disable-shared \
        --with-pic
make install

# Archive build
cd $builds_dir

tar cvfJ $pkg-$pkg_version-$HOST-static-$TIMESTAMP.tar.xz $staging_dir/
sha256sum $pkg-$pkg_version-$HOST-static-$TIMESTAMP.tar.xz >> $pkg-$pkg_version-$HOST-static-$TIMESTAMP.tar.xz.sha256

# Cleanup
cd $base_dir
rm -rf staging_dir
rm -rf work_dir