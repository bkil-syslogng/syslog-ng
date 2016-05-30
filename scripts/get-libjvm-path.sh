#!/bin/sh

recursive_readlink() {
  local READLINK_TARGET="$1"
  while test -L "$READLINK_TARGET"; do
    local READLINK_TARGET=$(readlink "$READLINK_TARGET")
  done
  echo "$READLINK_TARGET"
}

JAVAC_BIN="`which javac`"
JAVAC_BIN="`recursive_readlink "$JAVAC_BIN"`"
if test -e "$JAVA_HOME_CHECKER"; then
  JNI_HOME=`$JAVA_HOME_CHECKER`
else
  JNI_HOME=`echo $JAVAC_BIN | sed "s~/bin/javac$~/~"`
fi
JNI_LIBDIR=`find $JNI_HOME \( -name "libjvm.so" -or -name "libjvm.dylib" \) \
        | sed "s-/libjvm\.so-/-" \
        | sed "s-/libjvm\.dylib-/-" | head -n 1`

export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$JNI_LIBDIR"
