/*
 * Copyright (c) 2015 Balabit
 * Copyright (c) 2015 Tibor Benke
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

package org.syslog_ng.test;

import static org.junit.Assert.*;

import java.util.Hashtable;

import org.syslog_ng.LogDestination;
import org.syslog_ng.options.Option;
import org.syslog_ng.options.InvalidOptionException;

public class TestOption {
	public LogDestination owner;
	public Hashtable<String, String> options;

	public void setUp() throws Exception {
		options = new Hashtable<String, String>();
		owner = new MockLogDestination(options);
	}


	public void assertInitOptionSuccess(Option option) {
		try {
			option.validate();
		}
		catch (InvalidOptionException e) {
			throw new AssertionError("Initialization failed: " + e.getMessage());
		}
	}

	public void assertInitOptionFailed(Option option) {
		assertInitOptionFailed(option, null);
	}

	public void assertInitOptionFailed(Option option, String expectedErrorMessage) {
		try {
			option.validate();
			throw new AssertionError("Initialization should be failed");
		}
		catch (InvalidOptionException e) {
			if (expectedErrorMessage != null) {
				assertEquals(expectedErrorMessage, e.getMessage().substring(0, expectedErrorMessage.length()));
			}
		}
	}
}
