#!/bin/sh

main() {
  local ROOT="$HOME/git/syslog-ng.tmp/"
#  local MOD="modules/afmongodb/tests/test-mongodb-config"
#  local MOD="modules/json/tests/test_format_json"
#  local MOD="modules/cef/tests/test-format-cef-extension"
#  local MOD="modules/kvformat/tests/test_kv_scanner"
  local MOD="modules/basicfuncs/tests/test_basicfuncs"
  local EXEC="$ROOT/$MOD"
  local DATA="`dirname "$0"`"
  export G_SLICE=always-malloc,debug-blocks
  export G_DEBUG=fatal-warnings,fatal-criticals,gc-friendly

  time \
  valgrind \
    --fullpath-after="$ROOT" \
    --sim-hints=no-nptl-pthread-stackcache \
     --num-callers=30 \
    --tool=memcheck \
    --leak-check=full \
     --show-leak-kinds=all \
     --errors-for-leak-kinds=all \
    --keep-stacktraces=alloc-and-free \
    --freelist-vol=200''000''000 \
    --freelist-big-blocks=10''000''000 \
    --malloc-fill=55 \
    --free-fill=AA \
     --read-var-info=yes \
     --merge-recursive-frames=3 \
     --track-origins=yes \
     --keep-stacktraces=alloc-and-free \
     --gen-suppressions=all \
    --suppressions="$DATA/memcheck.external.supp" \
    --suppressions="$DATA/memcheck.unknown.supp" \
    --suppressions="$DATA/memcheck.bug.supp" \
    --error-exitcode=42 \
    $EXEC 2>&1

  S=$?
  echo $S
  return $S

cat << EOF > /dev/null
# quick test
    --num-callers=12 \
    --keep-stacktraces=none \
    --error-exitcode=42

# diagnose
    --num-callers=30 \
    --read-var-info=yes \
    --merge-recursive-frames=3 \
    --show-leak-kinds=all \
    --errors-for-leak-kinds=definite \
    --track-origins=yes \
    --keep-stacktraces=alloc-and-free \
    --gen-suppressions=all \
#
EOF
}

main "$@"
