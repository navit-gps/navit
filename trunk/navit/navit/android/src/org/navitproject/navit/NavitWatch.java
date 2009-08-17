/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.navitproject.navit;

import java.lang.Thread;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class NavitWatch implements Runnable {
	private Thread thread;
	private static Handler handler =new Handler() {
		public void handleMessage(Message m) {
			Log.e("NavitWatch","Handler received message");
		}
	};
	private boolean removed;
	private int watch_fd;
	private int watch_cond;
	private int watch_callbackid;
	private Runnable callback_runnable;
	public native void poll(int fd, int cond);
	public native void WatchCallback(int id);

	NavitWatch(int fd, int cond, int callbackid) 
	{
		// Log.e("NavitWatch","Creating new thread for "+fd+" "+cond+" from current thread " + java.lang.Thread.currentThread().getName());
		watch_fd=fd;
		watch_cond=cond;
		watch_callbackid=callbackid;
		final NavitWatch navitwatch=this;
		callback_runnable = new Runnable() {
			public void run()
			{
				navitwatch.callback();
			}
		};
		thread = new Thread(this, "poll thread");
		thread.start();
	}
	public void run()
	{
		for (;;) {
			// Log.e("NavitWatch","Polling "+watch_fd+" "+watch_cond + " from " + java.lang.Thread.currentThread().getName());
			poll(watch_fd, watch_cond);
			// Log.e("NavitWatch","poll returned");
			if (removed)
				break;
			handler.post(callback_runnable);	
			try {
				synchronized(this) {
					this.wait();
				}
			} catch (Exception e) {
				Log.e("NavitWatch","Exception "+e.getMessage());
			}
			if (removed)
				break;
		}
	}
	public void callback()
	{
		// Log.e("NavitWatch","Calling Callback");
		if (!removed)
			WatchCallback(watch_callbackid);
		synchronized(this) {
			this.notify();
		}
	}
	public void remove()
	{
		removed=true;
		thread.interrupt();
	}
}

