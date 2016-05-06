#!/bin/sh

build_riemann_client() {
  if ! pkg-config --exists "riemann-client >= 1.6.0"; then
    git clone https://github.com/algernon/riemann-c-client.git || exit 1
    cd riemann-c-client || exit 1
    autoreconf -i || exit 1
    ./configure  --prefix="$SLNG_CACHE" \
      --disable-dependency-tracking || exit 1
    make install || exit 1
    cd ..
  fi
}

build_riemann_client "$@"
