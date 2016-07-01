#############################################################################
# Copyright (c) 2007-2016 Balabit
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
# syslog-ng process control

import os
import signal
import time
import re

from globals import LOGSTORE_STORE_SUPPORTED, get_module_path, get_syslog_ng_binary
from log import print_user
import messagegen

SYSLOGNG_PID = 0

# pylint: disable=global-statement


def start_syslogng(conf, keep_persist=False, verbose=False):
    global SYSLOGNG_PID

    os.system('rm -f test-*.log test-*.lgs test-*.db wildcard/* log-file')
    if not keep_persist:
        os.system('rm -f syslog-ng.persist')

    if not LOGSTORE_STORE_SUPPORTED:
        conf = re.sub(r'logstore\(.*\);', '', conf)

    f = open('test.conf', 'w')
    f.write(conf)
    f.close()

    if verbose:
        verbose_opt = '-edv'
    else:
        verbose_opt = '-e'

    SYSLOGNG_PID = os.fork()
    if SYSLOGNG_PID == 0:
        os.putenv("RANDFILE", "rnd")
        module_path = get_module_path()
        os.execl(
            get_syslog_ng_binary(),
            get_syslog_ng_binary(),
            '-f', 'test.conf',
            '--fd-limit', '1024',
            '-F',
            verbose_opt,
            '-p', 'syslog-ng.pid',
            '-R', 'syslog-ng.persist',
            '--no-caps',
            '--enable-core',
            '--seed',
            '--module-path', module_path
        )
    time.sleep(5)
    print_user("Syslog-ng started")
    return True


def stop_syslogng():
    global SYSLOGNG_PID

    if SYSLOGNG_PID == 0:
        return True

    try:
        os.kill(SYSLOGNG_PID, signal.SIGTERM)
    except OSError:
        pass
    try:
        try:
            (_, rc) = os.waitpid(SYSLOGNG_PID, 0)
        except OSError:
            raise
    finally:
        SYSLOGNG_PID = 0
    print_user("syslog-ng stopped")
    if rc == 0:
        return True
    print_user("syslog-ng exited with a non-zero value (%d)" % rc)
    return False


def flush_files(settle_time=3):
    # pylint: disable=global-variable-not-assigned
    global SYSLOGNG_PID

    if SYSLOGNG_PID == 0 or not messagegen.NEED_TO_FLUSH:
        return True

    print_user("waiting for syslog-ng to settle down before SIGHUP (%d secs)" % settle_time)
    # allow syslog-ng to settle
    time.sleep(settle_time)

    # send_messages waits between etaps, so we assume that syslog-ng has
    # already received/processed everything we've sent to it. Go ahead send
    # a HUP signal.
    try:
        print_user("Sending syslog-ng the HUP signal (pid: %d)" % SYSLOGNG_PID)
        os.kill(SYSLOGNG_PID, signal.SIGHUP)
    except OSError:
        print_user("Error sending HUP signal to syslog-ng")
        raise
    # allow syslog-ng to perform config reload & file flush
    print_user("waiting for syslog-ng to process SIGHUP (%d secs)" % 2)
    time.sleep(2)
    messagegen.NEED_TO_FLUSH = False


def readpidfile(pidfile):
    f = open(pidfile, 'r')
    pid = f.read()
    f.close()
    return int(pid.strip())
