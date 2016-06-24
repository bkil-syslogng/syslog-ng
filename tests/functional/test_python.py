#############################################################################
# Copyright (c) 2015 Balabit
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
import socket
from globals import port_number, has_module
import messagegen
import messagecheck
import control

config = """@version: 3.8

source s_int { internal(); };
source s_tcp { tcp(port(%(port_number)d)); };

destination d_python {
    python(class(sngtestmod.DestTest)
           value-pairs(key('MSG') pair('HOST', 'bzorp') pair('DATE', '$ISODATE') key('MSGHDR')));
};

log { source(s_tcp); destination(d_python); };

""" % locals()


def get_source_dir():
    return os.path.abspath(os.path.dirname(__file__))


def check_env():

    if not has_module('mod-python'):
        print 'Python module is not available, skipping Python test'
        return False

    parent_dir = get_source_dir()
    print parent_dir
    if 'PYTHONPATH' not in os.environ or parent_dir not in os.environ['PYTHONPATH']:
        os.environ['PYTHONPATH'] = os.environ.get('PYTHONPATH', '') + ':' + parent_dir

    print 'Python module found, proceeding to Python tests'
    return True


def test_python():

    messages = (
        'python1',
        'python2'
    )
    sender = messagegen.SocketSender(socket.AF_INET, ('localhost', port_number), dgram=0)

    expected = []
    for msg in messages:
        expected.extend(sender.send_messages(msg, pri=7))
    stopped = control.stop_syslogng()
    if not stopped or not messagecheck.check_file_expected('test-python', expected, settle_time=2):
        return False
    return True
