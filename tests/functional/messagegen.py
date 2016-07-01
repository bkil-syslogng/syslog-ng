#############################################################################
# Copyright (c) 2007-2010 Balabit
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

import struct
import stat
import socket
import os
import sys
import errno
import time

from log import print_user

syslog_prefix = "2004-09-07T10:43:21+01:00 bzorp prog[12345]:"
syslog_new_prefix = "2004-09-07T10:43:21+01:00 bzorp prog 12345 - -"
session_counter = 0
need_to_flush = False


class MessageSender(object):
    def __init__(self, repeat=100, new_protocol=0, dgram=0):
        self.repeat = repeat
        self.new_protocol = new_protocol
        self.dgram = dgram

    def send_messages(self, msg, pri=7):
        global session_counter
        global need_to_flush
        global syslog_prefix  # pylint: disable=global-variable-not-assigned
        global syslog_new_prefix  # pylint: disable=global-variable-not-assigned

        padding = 'x' * 250
        need_to_flush = True

        print_user("generating %d messages using transport %s" % (self.repeat, str(self)))

        self.init_sender()
        expected = []

        for counter in range(1, self.repeat):
            if self.new_protocol == 0:
                line = '<%d>%s %s %03d/%05d %s %s' % (
                    pri, syslog_prefix, msg, session_counter, counter, str(self), padding)
            else:
                line = '<%d>1 %s %s %03d/%05d %s %s' % (
                    pri, syslog_new_prefix, msg, session_counter, counter, str(self), padding)

            # add framing on tcp with new protocol
            if self.dgram == 0 and self.new_protocol == 1:
                line = '%d %s' % (len(line), line)
            self.send_message(line)  # file or socket
        expected.append((msg, session_counter, self.repeat))
        session_counter = session_counter + 1
        return expected


class SocketSender(MessageSender):
    # pylint: disable=too-many-arguments,too-many-instance-attributes
    def __init__(
        self, family, sock_name, dgram=0, send_by_bytes=0, terminate_seq='\n',
            repeat=100, use_ssl=0, new_protocol=0):
        MessageSender.__init__(self, repeat, new_protocol, dgram)
        self.family = family
        self.sock_name = sock_name
        self.sock = None
        self.dgram = dgram
        self.send_by_bytes = send_by_bytes
        self.terminate_seq = terminate_seq
        self.ssl = use_ssl
        self.new_protocol = new_protocol

    def init_sender(self):
        if self.dgram:
            self.sock = socket.socket(self.family, socket.SOCK_DGRAM)
        else:
            self.sock = socket.socket(self.family, socket.SOCK_STREAM)

        self.sock.connect(self.sock_name)
        if self.dgram:
            self.sock.send('')
        if sys.platform == 'linux2':
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDTIMEO, struct.pack('ll', 3, 0))
        if not self.dgram and self.ssl:
            self.sock = socket.ssl(self.sock)

    def send_data(self, data):
        try:
            # WTF? SSLObject only has write, whereas sockets only have send methods
            if self.ssl:
                self.sock.write(data)
            else:
                self.sock.send(data)
            return False
        except OSError, error:
            if error.errno == errno.ENOBUFS:
                print_user('got ENOBUFS, sleeping...')
                time.sleep(0.5)
                return True
            else:
                print_user("hmm... got an error to the 'send' call, maybe syslog-ng is not accepting messages?")
                raise
        except:
            print_user("hmm... got an unknown error to the 'send' call, maybe syslog-ng is not accepting messages?")
            raise

    def send_message(self, msg):
        line = '%s%s' % (msg, self.terminate_seq)
        if self.send_by_bytes:
            for ch in line:
                while self.send_data(ch):
                    pass

        else:
            while self.send_data(line):
                if self.dgram:
                    time.sleep(0.01)

    def __str__(self):
        if self.family == socket.AF_UNIX:
            if self.dgram:
                return 'unix-dgram(%s)' % (self.sock_name)
            else:
                return 'unix-stream(%s)' % (self.sock_name)
        else:
            if self.dgram:
                return 'udp(%s)' % (self.sock_name,)
            elif not self.ssl:
                return 'tcp(%s)' % (self.sock_name,)
            else:
                return 'tls(%s)' % (self.sock_name,)


class FileSender(MessageSender):
    def __init__(self, file_name, padding=0, send_by_bytes=0, terminate_seq='\n', repeat=100):
        MessageSender.__init__(self, repeat)
        self.file_name = file_name
        self.padding = padding
        self.send_by_bytes = send_by_bytes
        self.terminate_seq = terminate_seq
        self.fd = None

        try:
            self.is_pipe = stat.S_ISFIFO(os.stat(file_name).st_mode)
        except OSError:
            self.is_pipe = False

    def __del__(self):
        if self.fd:
            self.fd.flush()
            self.fd.close()

    def init_sender(self):
        if self.is_pipe:
            self.fd = open(self.file_name, "w")
        else:
            self.fd = open(self.file_name, "a")

    def send_message(self, msg):
        line = '%s%s' % (msg, self.terminate_seq)
        if self.padding:
            line += '\0' * (self.padding - len(line))
        if self.send_by_bytes:
            for ch in line:
                self.fd.write(ch)
                self.fd.flush()
        else:
            self.fd.write(line)
            self.fd.flush()

    def __str__(self):
        if self.is_pipe:
            if self.padding:
                return 'pipe(%s[%d])' % (self.file_name, self.padding)
            else:
                return 'pipe(%s)' % (self.file_name,)
        else:
            return 'file(%s)' % (self.file_name,)
