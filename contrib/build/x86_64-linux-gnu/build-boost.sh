#!/bin/bash
#
# Statically built Linux Boost libraries for use with legacy Bitcoin code
# (Roll your own...)
# Author: opsinphark@GitHub

TIMESTAMP="$(date +"%Y%m%d%H%M%S")"

HOST="x86_64-linux-gnu"
HOST_CFLAGS="-O2 -pipe"
HOST_CXXFLAGS="-O2 -pipe"

pkg=boost
pkg_version=1_76_0
pkg_dl_path=https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/
pkg_file_name=$pkg"_"$pkg_version.tar.bz2
pkg_sha256_hash=f0397ba6e982c4450f27bf32a2a83292aba035b827a5623a14636ea583318c41
pkg_cflags="$HOST_CFLAGS"
pkg_cxxflags="$pkg_cflags -std=c++11 -fvisibility=hidden"
pkg_cxxflags_linux="$pkg_cflags -fPIC"
pkg_config_libraries="chrono,filesystem,program_options,system,thread,test,iostreams"
pkg_config_opts="variant=release --layout=tagged --build-type=complete threading=multi link=static -sNO_COMPRESSION=1"
pkg_config_opts_mingw32="target-os=windows binary-format=pe threadapi=win32 runtime-link=static"
pkg_config_opts_linux="target-os=linux threadapi=pthread runtime-link=shared"
pkg_config_opts_darwin="target-os=darwin runtime-link=shared"
pkg_config_opts_x86_64="architecture=x86 address-model=64"
pkg_config_opts_i686="architecture=x86 address-model=32"
pkg_config_opts_aarch64="address-model=64"
pkg_config_opts_armv7a="address-model=32"

base_dir=$(pwd)

source_dir=$base_dir/sources
mkdir -p $source_dir

work_dir=$base_dir/work
mkdir -p $work_dir

staging_dir=$base_dir/staging/$pkg-$pkg_version-static
mkdir -p $staging_dir

builds_dir=$base_dir/builds
mkdir -p $builds_dir

# Fetch & Verify
cd $source_dir
wget "$pkg_dl_path/$pkg_file_name"
echo "$pkg_sha256_hash $pkg_file_name" | sha256sum -c

# Extract
tar xvfj $pkg_file_name -C $work_dir

# Configure & Compile
cd $work_dir/$pkg"_"$pkg_version
CFLAGS="$pkg_cflags" \
CXXFLAGS="$pkg_cxxflags $pkg_cxxflags_linux" \
CPPFLAGS="$pkg_cppflags" \
LDFLAGS="$pkg_ldflags" \
./bootstrap.sh \
    --without-icu \
    --with-libraries="$pkg_config_libraries"
./b2 -d2 -d1 \
    --prefix=$staging_dir \
    $pkg_config_opts \
    $pkg_config_opts_linux \
    $pkg_config_opts_x86_64 \
    stage
./b2 -d0 \
    --prefix=$staging_dir \
    $pkg_config_opts \
    $pkg_config_opts_linux \
    $pkg_config_opts_x86_64 \
    install

# Archive build
cd $builds_dir

tar cvfJ $pkg-$pkg_version-$HOST-static-$TIMESTAMP.tar.xz $staging_dir/
sha256sum $pkg-$pkg_version-$HOST-static-$TIMESTAMP.tar.xz >> $pkg-$pkg_version-$HOST-static-$TIMESTAMP.tar.xz.sha256

# Cleanup
cd $base_dir
rm -rf staging_dir
rm -rf work_dir