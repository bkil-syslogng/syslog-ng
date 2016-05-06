#!/bin/sh

build_dbi() {
  dpkg -l libdbi1 && exit 1
  dpkg -l libdbd-sqlite3 && exit 1

  build_libdbi || exit 1
  build_libdbd || exit 1
}

build_libdbi() {
  if ! pkg-config --exists "dbi >= 0.9.0"; then
    OPWD="$PWD"
    mkdir -p "$SLNG_CACHE/deps" || exit 1
    cd "$SLNG_CACHE/deps" || exit 1

    if [ ! -d "libdbi" ]; then
      git clone --quiet --branch libdbi-0.9.0 \
        git://git.code.sf.net/p/libdbi/libdbi || exit 1
    fi

    cd libdbi || exit 1
    if [ ! -f configure ]; then
      ./autogen.sh || exit 1
    fi;

    if [ ! -f Makefile ]; then
      ./configure --prefix="$SLNG" \
        --disable-docs --disable-dependency-tracking || exit 1
    fi

    make -j install-exec || exit 1
    cd "$OPWD"
  fi
}

build_libdbd() {
  if [ ! -d "$SLNG/lib/dbd" ]; then
    local OPWD="$PWD"
    mkdir -p "$SLNG_CACHE/deps" || exit 1
    cd "$SLNG_CACHE/deps" || exit 1

    if [ ! -d "libdbi-drivers" ]; then
      git clone --quiet --branch libdbi-drivers-0.9.0 \
        git://git.code.sf.net/p/libdbi-drivers/libdbi-drivers || exit 1
    fi

    cd libdbi-drivers || exit 1
    if [ ! -f configure ]; then
      sed -i "s~^ac_dbi_libdir=\"no\"$~\# & \# syslog-ng HACK~" configure.in || exit 1
      ./autogen.sh || exit 1
    fi

    if [ ! -f Makefile ]; then
      local DBIINC="`pkg-config --variable=includedir dbi`" || exit 1
      [ -n "$DBIINC" ] || exit 1
      local DBILIB="`pkg-config --variable=libdir dbi`" || exit 1
      [ -n "$DBILIB" ] || exit 1

      ./configure --prefix="$SLNG" --with-sqlite3 \
                --with-dbi-libdir="$DBILIB" \
                --with-dbi-incdir="$DBIINC/.." \
                --disable-docs --disable-dependency-tracking || exit 1
    fi

    make -j install-exec || exit 1
    cd "$OPWD"
  fi
}

build_dbi "$@"
