#!/bin/sh -x

build_riemann_client() {
  if ! pkg-config --exists "riemann-client >= 1.6.0"; then
    git clone https://github.com/algernon/riemann-c-client.git || exit 1
    cd riemann-c-client || exit 1
    autoreconf -i || exit 1
    ./configure --prefix="$SLNG_CACHE" \
      --disable-dependency-tracking || exit 1
    make -j install-exec || exit 1
    cd .. || exit 1
    rm -Rf riemann-c-client
  fi
  true
}

build_riemann_client "$@"
