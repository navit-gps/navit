/*
  Navit, a modular navigation system.
  Copyright (C) 2005-2008 Navit Team

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  version 2 as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA  02110-1301, USA.
 */

package org.navitproject.navit;

import android.os.Handler;
import android.os.Message;
import android.util.Log;


class NavitTimeout implements Runnable {

    private static final TimeoutHandler handler = new TimeoutHandler();
    private final long mEventCallbackid;
    private final int mEventTimeout;
    private final boolean mEventMulti;

    NavitTimeout(int timeout, boolean multi, long callbackid) {
        mEventTimeout = timeout;
        mEventMulti = multi;
        mEventCallbackid = callbackid;
        handler.postDelayed(this, mEventTimeout);
    }

    public native void timeoutCallback(long id);

    public void run() {
        Log.v("Navit","Handle Event");
        if (mEventMulti) {
            handler.postDelayed(this, mEventTimeout);
        }
        timeoutCallback(mEventCallbackid);
    }

    public void remove() {
        handler.removeCallbacks(this);
    }

    static class TimeoutHandler extends Handler {
        public void handleMessage(Message m) {
            Log.d("NavitTimeout", "Handler received message");
        }
    }
}
