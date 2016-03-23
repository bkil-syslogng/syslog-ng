#!/bin/sh
. `dirname "$0"`/../tests/build-log-cflags-propagation.sh

main() {
  local LOG="make.log"
  echo "info: log saved to $LOG" >&2
  {
    logged_main "$@"
    echo "exit status: $?"
  } 2>&1 |
  tee "$LOG"
}

logged_main() {
  exec_prop_check "make -j V=1 install"
  S=$?
  if [ "$S" = "42" ]; then
    return $S;
  elif [ "$S" != "0" ]; then
    make V=1 --keep-going # to make error messages more readable on error
    return 1
  fi

  export CFLAGS="$CFLAGS -Werror"
  exec_prop_check "make -j 1 distcheck V=1 --keep-going" &&

  make func-test V=1 --keep-going
}

main "$@"
