#############################################################################
# Copyright (c) 2007-2013 Balabit
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

import os
import re
from globals import LOGSTORE_STORE_SUPPORTED
from log import print_user
import control
import messagegen


def logstore_reader(fname):
    try:
        return os.popen("../../src/logcat %s.lgs" % fname, "r")
    except OSError, err:
        print_user("Error opening file: %s, %s" % (fname, str(err)))


def file_reader(fname):
    try:
        return open(fname + ".log", "r")
    except IOError, err:
        print_user("Error opening file: %s, %s" % (fname, str(err)))


def sql_reader(name):
    (db, table) = name
    try:
        return os.popen("""echo "select * from %s order by msg;" | sqlite3 -separator " "  %s """ % (table, db), "r")
    except OSError, err:
        print_user("Error opening db: %s %s, %s" % (db, table, str(err)))


def check_contents(f, messages, syslog_prefix, skip_prefix):

    if not f:
        print_user("Failed to open file/database")
        return False
    print_user("Starting output file content check")

    lineno = 1
    # we handle message reordering by going through the complete
    # file and looking for the message
    # end the collected ranges must span the number of messages that
    # we expect
    matches = {}

    read_line = f.readline().strip()
    while read_line:
        prefix_len = len(syslog_prefix)

        if read_line[skip_prefix:prefix_len + skip_prefix] != syslog_prefix:
            print_user("message does not start with syslog_prefix=%s, file=%s:%d, line=%s"
                       % (syslog_prefix, f, lineno, read_line))
            return False
        msg = read_line[skip_prefix + prefix_len:]
        match = re.match(r"^ (\S+) (\d+)/(\d+)", msg)

        if not match:
            print_user("message payload unexpected format, file=%s:%d, line=%s" % (f, lineno, read_line))
            return False
        msg = match.group(1)
        session = int(match.group(2))
        sid = int(match.group(3))
        if (msg, session) not in matches:
            if sid != 1:
                print_user("the id of the first message in a session is not 1, session=%d, "
                           "id=%d, file=%s:%d, line=%s"
                           % (session, sid, f, lineno, read_line))
                return False
        else:
            if matches[(msg, session)] != sid - 1:
                print_user("message reordering/drop detected in the same session, session=%d, "
                           "id=%d, expected_id=%d, file=%s:%d, line=%s"
                           % (session, sid, matches[(msg, session)] + 1, f, lineno, read_line))
                return False
        matches[(msg, session)] = sid
        read_line = f.readline().strip()
        lineno = lineno + 1

    for (msg, session, count) in messages:

        if (msg, session) not in matches:
            print_user("output log files lack this kind of message: %s session: %d, count: %d"
                       % (msg, session, count))
            return False
        if matches[(msg, session)] != count - 1:
            print_user("not enough messages found, message: %s, session: %d, last_id: %d, count: %d"
                       % (msg, session, matches[(msg, session)], count))
            return False
        del matches[(msg, session)]

    if len(matches) > 0:
        print_user("output contains more messages than expected: %s" % str(matches))
        return False

    return True


def check_reader_expected(reader, messages, settle_time, syslog_prefix, skip_prefix):
    return check_contents(reader, messages, syslog_prefix, skip_prefix)


def check_file_expected(fname, messages, settle_time=1, syslog_prefix=messagegen.SYSLOG_PREFIX, skip_prefix=0):
    print_user("Checking contents of output files: %s" % fname)
    control.flush_files(settle_time)

    if LOGSTORE_STORE_SUPPORTED:
        readers = (file_reader, logstore_reader)
    else:
        readers = (file_reader,)
    for reader in readers:
        return check_reader_expected(reader(fname), messages, settle_time, syslog_prefix, skip_prefix)


def check_sql_expected(dbname, tablename, messages, settle_time=1, syslog_prefix="", skip_prefix=0):
    print_user("Checking contents of output database %s, table: %s" % (dbname, tablename))
    return check_reader_expected(sql_reader((dbname, tablename)), messages, settle_time, syslog_prefix, skip_prefix)
