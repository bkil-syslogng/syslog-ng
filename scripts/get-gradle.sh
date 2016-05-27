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

get_gradle() {
  export GRADLE_HOME=$SLNG_CACHE/gradle-2.9
  export PATH=$GRADLE_HOME/bin:$PATH

  if ! which gradle; then
    wget https://downloads.gradle.org/distributions/gradle-2.9-bin.zip || exit 1
    unzip gradle-2.9-bin.zip -d "$SLNG_CACHE" || exit 1
    rm -v gradle-2.9-bin.zip
  fi
  true
}

get_gradle "$@"
