#!/bin/sh -x
#############################################################################
# Copyright (c) 2016 Balabit
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 as published
# by the Free Software Foundation, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# As an additional exemption you are allowed to compile & link against the
# OpenSSL libraries as published by the OpenSSL project. See the file
# COPYING for details.
#
#############################################################################

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
