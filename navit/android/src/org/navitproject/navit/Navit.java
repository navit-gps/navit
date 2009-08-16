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
import android.content.res.Resources;
import android.util.Log;
import java.util.Locale;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;


public class Navit extends Activity implements Handler.Callback
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
	Locale locale=java.util.Locale.getDefault();
	String lang=locale.getLanguage();
	String langu=lang;
	String langc=lang;
	int pos=langu.indexOf('_');
	if (pos != -1) {
		langc=langu.substring(0, pos);
		langu=langc + langu.substring(pos).toUpperCase(locale);
	}
	Log.e("Navit","Language " + lang);
	Resources res=getResources();
	String resname=langc;
	
	Log.e("Navit","Res Name " + resname);
	int id=res.getIdentifier(resname,"raw","org.navitproject.navit");
	Log.e("Navit","Res ID " + id);
	if (id != 0) {
		Log.e("Navit","at 1");
		InputStream fis=res.openRawResource(id);
		Log.e("Navit","at 2");
		String data="/data/data/org.navitproject.navit/locale";
		Log.e("Navit","at 3");
		File file=new File(data);
		Log.e("Navit","at 4");
		file.mkdir();
		Log.e("Navit","at 5");
		data+="/"+resname;
		Log.e("Navit","at 6");
		file=new File(data);
		Log.e("Navit","at 7");
		file.mkdir();
		Log.e("Navit","at 8");
		data+="/LC_MESSAGES";
		file=new File(data);
		file.mkdir();
		data+="/navit.mo";
		file=new File(data);
		try {
			FileOutputStream fos=new FileOutputStream(file);
			byte[] buf = new byte[1024];
			int i = 0;
			while ((i = fis.read(buf)) != -1) {
				fos.write(buf, 0, i);
			}
		} 
		catch (Exception e) {
			Log.e("Navit","Exception "+e.getMessage());
		}
	}
	// Debug.startMethodTracing("calc");
        NavitMain(this, langu);
    }

    public void disableSuspend()
    {
	wl.acquire();
	wl.release();
    }

    public boolean handleMessage(Message m) {
	Log.e("Navit","Handler received message");
	return true;
    }

    /* A native method that is implemented by the
     * 'hello-jni' native library, which is packaged
     * with this application.
     */
    public native void NavitMain(Navit x, String lang);

    /* this is used to load the 'hello-jni' library on application
     * startup. The library has already been unpacked into
     * /data/data/com.example.Navit/lib/libhello-jni.so at
     * installation time by the package manager.
     */
    static {
        System.loadLibrary("navit");
    }
}

