/*
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

package org.navitproject.navit;

import android.os.Handler;
import android.os.Message;
import android.util.Log;

class NavitWatch implements Runnable {

    private static WatchHandler sHandler = new WatchHandler();
    private Thread mThread;
    private boolean mRemoved;
    private long mWatchFunc;
    private int mWatchFd;
    private int mWatchCond;
    private long mWatchCallbackid;
    private boolean mCallbackPending;
    private Runnable mCallbackRunnable;

    NavitWatch(long func, int fd, int cond, long callbackid) {
        Log.d("NavitWatch","Creating new thread for " + fd + " " + cond + " from current thread "
                + java.lang.Thread.currentThread().getName());
        mWatchFunc = func;
        mWatchFd = fd;
        mWatchCond = cond;
        mWatchCallbackid = callbackid;
        final NavitWatch navitwatch = this;
        mCallbackRunnable = new Runnable() {
            public void run() {
                navitwatch.callback();
            }
        };
        mThread = new Thread(this, "poll thread");
        mThread.start();
    }

    public native void poll(long func, int fd, int cond);

    public native void watchCallback(long id);

    public void run() {
        for (; ; ) {
            // Log.e("NavitWatch","Polling "+watch_fd+" "+watch_cond + " from "
            // + java.lang.Thread.currentThread().getName());
            poll(mWatchFunc, mWatchFd, mWatchCond);
            // Log.e("NavitWatch","poll returned");
            if (mRemoved) {
                break;
            }
            mCallbackPending = true;
            sHandler.post(mCallbackRunnable);
            try {
                // Log.e("NavitWatch","wait");
                synchronized (this) {
                    if (mCallbackPending) {
                        this.wait();
                    }
                }
                // Log.e("NavitWatch","wait returned");
            } catch (Exception e) {
                Log.e("NavitWatch", "Exception " + e.getMessage());
            }
            if (mRemoved) {
                break;
            }
        }
    }

    private void callback() {
        // Log.e("NavitWatch","Calling Callback");
        if (!mRemoved) {
            watchCallback(mWatchCallbackid);
        }
        synchronized (this) {
            mCallbackPending = false;
            // Log.e("NavitWatch","Waking up");
            this.notify();
        }
    }

    public void remove() {
        mRemoved = true;
        mThread.interrupt();
    }

    static class WatchHandler extends Handler {
        public void handleMessage(Message m) {
            Log.d("NavitWatch", "Handler received message");
        }
    }
}

