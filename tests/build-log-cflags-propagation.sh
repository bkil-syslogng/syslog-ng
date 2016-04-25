#!/bin/sh

exec_prop_check() {
  local ERROREXIT="42"
  local CMD="$1"
  local BUILDLOG="build.$$.log"
  {
    eval "$CMD"
    echo $?
  } |
  tee "$BUILDLOG" |
  reduce_verbosity

  S=`tail -n 1 "$BUILDLOG"`
  if [ "$S" -eq 0 ]; then
    build_log_cflags_propagation "$BUILDLOG"
    S=$?
    rm "$BUILDLOG"
    return $S
  else
    rm "$BUILDLOG"
    return $ERROREXIT
  fi
}

####
# private functions:

reduce_verbosity() {
  grep --line-buffered --invert-match --extended-regexp "^(\
libtool: (link|relink|install): |\
depbase=|\
(test -z|rm|\./lib/merge-grammar.pl|\./doc/mallard2man\.py) |\
`printf "\t"`?/bin/bash |\
`printf "\t"`?(gcc|mv) \
)"
}

build_log_cflags_propagation() {
  local BUILDLOG="$1"
  local NOTPROP="`find_not_prop "$BUILDLOG"`"
  if [ -z "$NOTPROP" ]; then
    echo "info: CFLAGS propagation test passed" >&2
    true
  else
    local N=`echo "$NOTPROP" | wc -l`
    echo "error: -Wshadow/-Werror did not propagate via CFLAGS in $N cases:" >&2

    printf "%s\n" "$NOTPROP" |
    sed "s~^~> ~" >&2
    false
  fi
}

find_not_prop() {
  {
    find_gcc "$@" |
    ignore_submodule_gcc |
    grep -vE -- " -Wshadow( |.* )-Werror "
  } 2>&1
}

find_gcc() {
  grep -E "^(\
libtool: compile: +gcc |\
`printf "\t"`?/bin/bash \./libtool  --tag=CC   --mode=compile gcc |\
`printf "\t"`?gcc \
)" "$@"
}

ignore_submodule_gcc() {
  ignore_submodule_gcc_mongo_c_driver "$@" |
  ignore_submodule_gcc_rabbitmq_c |
  ignore_submodule_gcc_ivykis
}

ignore_submodule_gcc_mongo_c_driver() {
  grep -vE -- "\<(\
gcc( -std=gnu99)? -DPACKAGE_NAME=\\\\\"(libbson|mongo-c-driver)\\\\\"\
)" "$@"
}

ignore_submodule_gcc_rabbitmq_c() {
  grep -vE -- "\<(\
gcc -std=gnu99 -DHAVE_CONFIG_H -I\. +-I\./librabbitmq |\
gcc -std=gnu99 -DHAVE_CONFIG_H -I\. +-I\.\./\.\./\.\./\.\./modules/afamqp/rabbitmq-c \
)" "$@"
}

ignore_submodule_gcc_ivykis() {
  grep -vE -- "\<(\
gcc -DHAVE_CONFIG_H -I\. -I\.\./\.\. +-D_GNU_SOURCE -I\.\./\.\./src/include -I\.\./\.\./src/include |\
gcc -DHAVE_CONFIG_H -I\. -I\.\./\.\.  -I\.\./\.\./src/include -I\.\./\.\./src/include |\
gcc -DHAVE_CONFIG_H -I\. -I\.\. +-D_GNU_SOURCE -I\.\./src/include -I\.\./src/include |\
gcc( -std=gnu99)? -DHAVE_CONFIG_H -I\. -I\.\./\.\./\.\./\.\./lib/ivykis/src |\
gcc( -std=gnu99)? -DHAVE_CONFIG_H -I\. -I\.\./\.\./\.\./\.\./lib/ivykis/test |\
gcc( -std=gnu99)? -DHAVE_CONFIG_H -I\. -I\.\./\.\./\.\./\.\./\.\./lib/ivykis/contrib/iv_getaddrinfo |\
gcc -std=gnu99 -DHAVE_CONFIG_H -I\. -I\.\./\.\./\.\./\.\./\.\./lib/ivykis/contrib/kojines \
)" "$@"
}
