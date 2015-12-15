/*
 * Copyright (c) 2015 Balabit
 * Copyright (c) 2015 Adam Arsenault <adam.arsenault@balabit.com>
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

package org.syslog_ng.http;

import org.syslog_ng.*;

import java.io.*;

import org.syslog_ng.options.InvalidOptionException;
import org.syslog_ng.logging.SyslogNgInternalLogger;
import org.apache.log4j.Level;
import org.apache.log4j.Logger;

import java.lang.SecurityException;
import java.lang.IllegalStateException;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;


public class HTTPDestination extends TextLogDestination {
    private URL url;
    Logger logger;
    private HTTPDestinationOptions options;

    public HTTPDestination(long handle) {
        super(handle);
        logger = Logger.getRootLogger();
        SyslogNgInternalLogger.register(logger);
        options = new HTTPDestinationOptions(this);
    }

    @Override
    public String getNameByUniqOptions() {
        return String.format("HTTPDestination,%s", options.getURL());
    }


    @Override
    public boolean init() {
        try {
            options.init();
        } catch (InvalidOptionException e) {
            return false;
        }
        try {
            this.url = new URL(options.getURL());
        } catch (MalformedURLException e) {
            logger.error("A properly formatted URL is a required option for this destination");
            return false;
        }
        return true;
    }

    @Override
    public boolean open() {
        return isOpened();
    }

    @Override
    public boolean isOpened() {
        return true;
    }


    @Override
    public boolean send(String message) {
        int responseCode = 0;
        try {
            HttpURLConnection connection = (HttpURLConnection) this.url.openConnection();
            connection.setRequestMethod(options.getMethod());
            connection.setRequestProperty("content-length", Integer.toString(message.length()));
            connection.setDoOutput(true);
            try (OutputStreamWriter osw = new OutputStreamWriter(connection.getOutputStream())) {
                osw.write(message);
                osw.close();
            } catch (IOException e) {
		logger.error("error in writing message.");
		return false;
            }
            responseCode = connection.getResponseCode();
        } catch (IOException | SecurityException | IllegalStateException e) {
            logger.debug("error in writing message." +
                    (responseCode != 0 ? "Response code is " + responseCode : ""));
            return false;
        }
        return true;
    }

    @Override
    public void onMessageQueueEmpty() {
    }

    @Override
    public void close() {
    }

    @Override
    public void deinit() {
    }


}

