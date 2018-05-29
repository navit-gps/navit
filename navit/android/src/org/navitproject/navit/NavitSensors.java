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

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;



public class NavitSensors implements SensorEventListener {
    private SensorManager mSensorManager;
    private int callbackid;
    public native void SensorCallback(int id, int sensor, float x, float y, float z);


    NavitSensors(Context context, int cbid) {
        mSensorManager = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
        mSensorManager.registerListener((SensorEventListener)this, mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),
                                        SensorManager.SENSOR_DELAY_NORMAL);
        mSensorManager.registerListener((SensorEventListener)this, mSensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD),
                                        SensorManager.SENSOR_DELAY_NORMAL);
        callbackid=cbid;
    }

    public void onAccuracyChanged(Sensor sensor, int accuracy) {
    }

    public void onSensorChanged(SensorEvent sev) {
        // Log.e("NavitSensor","Type:" + sev.sensor.getType() + " X:" + sev.values[0] + " Y:"+sev.values[1]+" Z:"+sev.values[2]);
        SensorCallback(callbackid, sev.sensor.getType(), sev.values[0], sev.values[1], sev.values[2]);
    }
}

