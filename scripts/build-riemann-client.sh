#!/bin/sh -x

build_riemann_client() {
#  build_protobufc || exit 1
  build_riemann_client_core || exit 1
}

build_protobufc() {
  build_protobuf
  build_protobufc_core
}

build_protobuf() {
  if ! pkg-config --exists "protobuf >= 2.6.0"; then
    git clone "https://github.com/google/protobuf" || exit 1
    cd protobuf || exit 1
    ./autogen.sh || exit 1
    ./configure --prefix="$SLNG_CACHE" \
      --disable-maintainer-mode \
      --disable-dependency-tracking || exit 1
    make -j 4 install || exit 1
    cd .. || exit 1
    rm -Rf protobuf
  fi
  true
}

build_protobufc_core() {
  if ! which protoc-c; then
    git clone "https://github.com/protobuf-c/protobuf-c" || exit 1
    cd protobuf-c || exit 1
    ./autogen.sh || exit 1
    ./configure --prefix="$SLNG_CACHE" \
      --disable-dependency-tracking || exit 1
    make -j 4 install || exit 1
    cd .. || exit 1
    rm -Rf protobuf-c
  fi
  true
}

build_riemann_client_core() {
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
