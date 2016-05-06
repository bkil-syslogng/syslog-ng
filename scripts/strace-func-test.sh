#!/bin/sh -x

strace_func_test() {
  printf '#!/bin/bash\n
    '"$HOME"'/install/syslog-ng/sbin/syslog-ng "$@" &
    PID=$!;
    trap "kill --signal 15 $PID" 15;
    exec strace -s 64 -o '"$PWD"'/strace.log -f -p $PID;
    \n' > "$PWD"/x.sh &&
  chmod +x "$PWD"/x.sh &&
  SYSLOG_NG_BINARY="$PWD"/x.sh \
  make func-test V=1

  S=$?
  if [ $S != 0 ]; then
    cat strace.log
    exit $S
  fi
}

strace_func_test "$@"
