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
import android.hardware.Camera;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.io.IOException;


class NavitCamera extends SurfaceView implements SurfaceHolder.Callback {

    private Camera mCamera;
    private static final String TAG = "NavitCamera";

    NavitCamera(Context context) {
        super(context);
        if (android.support.v4.content.ContextCompat.checkSelfPermission(context,
                android.Manifest.permission.CAMERA)
                != android.content.pm.PackageManager.PERMISSION_GRANTED) {
            Log.e(TAG,"No permission to access camera");
            return;
        }
        SurfaceHolder holder;
        holder = getHolder();
        holder.addCallback(this);
        holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        Log.v(TAG,"Creator");
    }


    /**
     * {@inheritDoc}
     *
     * <p>acquire the camera and tell it where to draw.</p>
     */
    public void surfaceCreated(SurfaceHolder holder) {
        try {
            mCamera = Camera.open();
            mCamera.setPreviewDisplay(holder);
        } catch (IOException exception) {
            mCamera.release();
            mCamera = null;
            Log.e(TAG, "IOException");
        }
        Log.i(TAG, "surfaceCreated");

    }


    /**
     * {@inheritDoc}
     *
     * <p>stop the preview and release the camera.</p>
     */
    public void surfaceDestroyed(SurfaceHolder holder) {
        if (mCamera != null) {
            mCamera.stopPreview();
            mCamera = null;
            Log.e(TAG, "surfaceDestroyed");
        }
    }


    /**
     * {@inheritDoc}
     *
     * <p>set up the camera with the new parameters.</p>
     */
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        Log.e(TAG,"surfaceChanged " + w + "x " + h);
        if (mCamera != null) {
            mCamera.stopPreview();
            Camera.Parameters parameters = mCamera.getParameters();
            parameters.setPreviewSize(w, h);
            mCamera.setParameters(parameters);
            mCamera.startPreview();
        }
    }
}

