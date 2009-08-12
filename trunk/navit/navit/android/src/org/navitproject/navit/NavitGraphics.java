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
import android.content.Context;
import android.graphics.*;
import android.os.Bundle;
import android.os.Debug;
import android.view.*;
import android.util.Log;

public class NavitGraphics extends View {
	private NavitGraphics parent_graphics;
	public NavitGraphics(Activity activity, NavitGraphics parent) {
		super(activity);
		parent_graphics=parent;
	}
	public native void SizeChangedCallback(int id, int x, int y);
	public native void ButtonCallback(int id, int pressed, int button, int x, int y);
	public native void MotionCallback(int id, int x, int y);
	private Canvas draw_canvas;
	private Bitmap draw_bitmap;
	private int SizeChangedCallbackID,ButtonCallbackID,MotionCallbackID;
	// private int count;

	@Override protected void onDraw(Canvas canvas)
	{
		super.onDraw(canvas);
		canvas.drawBitmap(draw_bitmap, 0, 0, null);
	}
	@Override protected void onSizeChanged(int w, int h, int oldw, int oldh)
	{
		super.onSizeChanged(w, h, oldw, oldh);
		draw_bitmap=Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
		draw_canvas=new Canvas(draw_bitmap);
		SizeChangedCallback(SizeChangedCallbackID, w, h);
	}
        @Override public boolean onTouchEvent(MotionEvent event)
	{
		int action = event.getAction();
		int x=(int)event.getX();
		int y=(int)event.getY();
		if (action == MotionEvent.ACTION_DOWN) {
			Log.e("NavitGraphics", "onTouch down");
			ButtonCallback(ButtonCallbackID, 1, 1, x, y);
		}
		if (action == MotionEvent.ACTION_UP) {
			Log.e("NavitGraphics", "onTouch up");
			ButtonCallback(ButtonCallbackID, 0, 1, x, y);
			// if (++count == 3)
		        //	Debug.stopMethodTracing();
		}
		if (action == MotionEvent.ACTION_MOVE) {
			Log.e("NavitGraphics", "onTouch move");
			MotionCallback(MotionCallbackID, x, y);
		}
		return true;
	} 
	public void setSizeChangedCallback(int id)
	{
		SizeChangedCallbackID=id;
	}
	public void setButtonCallback(int id)
	{
		ButtonCallbackID=id;
	}
	public void setMotionCallback(int id)
	{
		MotionCallbackID=id;
	}

	protected void draw_polyline(Paint paint,int c[])
	{
		paint.setStyle(Paint.Style.STROKE);
		Path path = new Path();
		path.moveTo(c[0], c[1]);
		for (int i = 2 ; i < c.length ; i+=2) {
			path.lineTo(c[i], c[i+1]);
		}
		draw_canvas.drawPath(path, paint);
	}

	protected void draw_polygon(Paint paint,int c[])
	{
		paint.setStyle(Paint.Style.FILL);
		Path path = new Path();
		path.moveTo(c[0], c[1]);
		for (int i = 2 ; i < c.length ; i+=2) {
			path.lineTo(c[i], c[i+1]);
		}
		draw_canvas.drawPath(path, paint);
	}
	protected void draw_rectangle(Paint paint,int x, int y, int w, int h)
	{
		Rect r = new Rect(x, y, x+w, y+h);
		paint.setStyle(Paint.Style.FILL);
		draw_canvas.drawRect(r, paint);		
	}
	protected void draw_circle(Paint paint,int x, int y, int r)
	{
		float fx=x;
		float fy=y;
		float fr=r/2;
		paint.setStyle(Paint.Style.STROKE);
		draw_canvas.drawCircle(fx, fy, fr, paint);
	}
	protected void draw_text(Paint paint,int x, int y, String text)
	{
		float fx=x;
		float fy=y;
		draw_canvas.drawText(text, fx, fy, paint);
	}
	protected void draw_image(Paint paint, int x, int y, Bitmap bitmap)
	{
		float fx=x;
		float fy=y;
		draw_canvas.drawBitmap(bitmap, fx, fy, paint);
	}
	protected void draw_mode(int mode)
	{
		if (mode == 1)
			invalidate();
			
	}
}
