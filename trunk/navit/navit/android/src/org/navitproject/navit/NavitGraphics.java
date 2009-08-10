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
package org.navitproject.navitgraphics;

import android.app.Activity;
import android.content.Context;
import android.graphics.*;
import android.os.Bundle;
import android.view.View;
import android.util.Log;

public class NavitGraphics extends View {
	public NavitGraphics(Activity activity) {
		super(activity);
		Log.e("NavitGraphics", "constructor");
	}
	public native void SizeChangedCallback(int id, int x, int y);
	private Canvas draw_canvas;
	private Bitmap draw_bitmap;
	private int SizeChangedCallbackID;

	@Override protected void onDraw(Canvas canvas)
	{
		super.onDraw(canvas);
		Log.e("NavitGraphics", "onDraw");
		canvas.drawBitmap(draw_bitmap, 0, 0, null);
	}
	protected void onSizeChanged(int w, int h, int oldw, int oldh)
	{
		Log.e("NavitGraphics", "onSizeChanged "+SizeChangedCallbackID+" "+w+" "+h);
		draw_bitmap=Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
		draw_canvas=new Canvas(draw_bitmap);
		int c[]={10,20,30,40};
		draw_polyline(c);
		SizeChangedCallback(SizeChangedCallbackID, w, h);
	}
	public void setSizeChangedCallback(int id)
	{
		SizeChangedCallbackID=id;
	}

	protected void draw_polyline(int c[])
	{
		Paint paint = new Paint();
		paint.setStyle(Paint.Style.STROKE);
		paint.setStrokeWidth(2);
		paint.setColor(Color.BLUE);
		Path path = new Path();
		path.moveTo(c[0], c[1]);
		for (int i = 2 ; i < c.length ; i+=2) {
			path.lineTo(c[i], c[i+1]);
		}
		draw_canvas.drawPath(path, paint);
	}

	protected void draw_polygon(int c[])
	{
		Paint paint = new Paint();
		paint.setStyle(Paint.Style.STROKE);
		paint.setStrokeWidth(2);
		paint.setColor(Color.BLUE);
		Path path = new Path();
		path.moveTo(c[0], c[1]);
		for (int i = 2 ; i < c.length ; i+=2) {
			path.lineTo(c[i], c[i+1]);
		}
		draw_canvas.drawPath(path, paint);
	}
}
