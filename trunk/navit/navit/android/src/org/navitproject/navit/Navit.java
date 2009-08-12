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

import android.app.Activity;
import android.widget.TextView;
import android.os.Bundle;
import android.os.Debug;
import android.os.Message;
import android.os.Handler;
import android.os.PowerManager;
import android.content.Context;
import android.util.Log;


public class Navit extends Activity
{
    public Handler handler;
    private PowerManager.WakeLock wl;
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
	PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);  
	wl = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK|PowerManager.ON_AFTER_RELEASE, "NavitDoNotDimScreen"); 
	handler =new Handler() {
		public void handleMessage(Message m) {
			Log.e("Navit","Handler received message");
		}
	};
	// Debug.startMethodTracing("calc");
        NavitMain(this);
    }

    public void disableSuspend()
    {
	wl.acquire();
	wl.release();
    }

    /* A native method that is implemented by the
     * 'hello-jni' native library, which is packaged
     * with this application.
     */
    public native void NavitMain(Navit x);

    /* this is used to load the 'hello-jni' library on application
     * startup. The library has already been unpacked into
     * /data/data/com.example.Navit/lib/libhello-jni.so at
     * installation time by the package manager.
     */
    static {
        System.loadLibrary("navit");
    }
}

