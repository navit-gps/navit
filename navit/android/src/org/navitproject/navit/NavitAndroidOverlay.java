/**
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

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Paint.Style;
import android.os.Bundle;
import android.os.Message;
import android.util.Log;
import android.view.MotionEvent;
import android.widget.ImageView;

public class NavitAndroidOverlay extends ImageView
{
	public Boolean					draw_bubble							= false;
	public long						bubble_showing_since				= 0L;
	public static long			bubble_max_showing_timespan	= 8000L; // 8 secs.

	public static BubbleThread	bubble_thread						= null;

	private class BubbleThread extends Thread
	{
		private Boolean					running;
		private long						bubble_showing_since	= 0L;
		private NavitAndroidOverlay	a_overlay				= null;

		BubbleThread(NavitAndroidOverlay a_ov)
		{
			this.running = true;
			this.a_overlay = a_ov;
			this.bubble_showing_since = System.currentTimeMillis();
			//Log.e("Navit", "BubbleThread created");
		}

		public void stop_me()
		{
			this.running = false;
		}

		public void run()
		{
			//Log.e("Navit", "BubbleThread started");
			while (this.running)
			{
				if ((System.currentTimeMillis() - this.bubble_showing_since) > bubble_max_showing_timespan)
				{
					//Log.e("Navit", "BubbleThread: bubble displaying too long, hide it");
					// with invalidate we call the onDraw() function, that will take care of it
					this.a_overlay.postInvalidate();
					this.running = false;
				}
				else
				{
					try
					{
						Thread.sleep(50);
					}
					catch (InterruptedException e)
					{
						// e.printStackTrace();
					}
				}
			}
			//Log.e("Navit", "BubbleThread ended");
		}
	}


	public static class NavitAndroidOverlayBubble
	{
		int		x;
		int		y;
		String	text	= null;
	}

	private NavitAndroidOverlayBubble	bubble_001	= null;

	public NavitAndroidOverlay(Context context)
	{
		super(context);
	}

	public void show_bubble()
	{
		//Log.e("Navit", "NavitAndroidOverlay -> show_bubble");
		if (!this.draw_bubble)
		{
			this.draw_bubble = true;
			this.bubble_showing_since = System.currentTimeMillis();
			bubble_thread = new BubbleThread(this);
			bubble_thread.start();

			// test test DEBUG
			Message msg = new Message();
			Bundle b = new Bundle();
			b.putInt("Callback", 4);
			b.putInt("x", this.bubble_001.x);
			b.putInt("y", this.bubble_001.y);
			msg.setData(b);
			Navit.N_NavitGraphics.callback_handler.sendMessage(msg);
		}
	}

	public void hide_bubble()
	{
		this.draw_bubble = false;
		this.bubble_showing_since = 0L;
		try
		{
			bubble_thread.stop_me();
			bubble_thread.stop();
		}
		catch (Exception e)
		{

		}
		//this.postInvalidate();
	}

	public void set_bubble(NavitAndroidOverlayBubble b)
	{
		this.bubble_001 = b;
	}

	@Override
	public boolean onTouchEvent(MotionEvent event)
	{
		//Log.e("Navit", "NavitAndroidOverlay -> onTouchEvent");
		super.onTouchEvent(event);

		int action = event.getAction();
		int x = (int) event.getX();
		int y = (int) event.getY();

		// test if we touched the grey rectangle
		//
		//		if ((x < 300) && (x > 10) && (y < 200) && (y > 10))
		//		{
		//			Log.e("Navit", "NavitAndroidOverlay -> onTouchEvent -> touch Rect!!");
		//			return true;
		//		}
		//		else
		//		{
		//			return false;
		//		}

		// false -> we dont use this event, give it to other layers
		return false;
	}

	public void onDraw(Canvas c)
	{
		//Log.e("Navit", "NavitAndroidOverlay -> onDraw");

		Paint paint = new Paint(0);
		paint.setAntiAlias(false);
		paint.setColor(Color.GRAY);

		if (this.draw_bubble)
		{
			if ((System.currentTimeMillis() - this.bubble_showing_since) > this.bubble_max_showing_timespan)
			{
				// bubble has been showing too log, hide it
				this.hide_bubble();

				// next lines are a hack, without it screen will not get updated anymore!
				// next lines are a hack, without it screen will not get updated anymore!
				// down
				Message msg = new Message();
				Bundle b = new Bundle();
				b.putInt("Callback", 21);
				b.putInt("x", 1);
				b.putInt("y", 1);
				msg.setData(b);
				Navit.N_NavitGraphics.callback_handler.sendMessage(msg);

				// move
				msg = new Message();
				b = new Bundle();
				b.putInt("Callback", 23);
				b.putInt("x", 1 + 10);
				b.putInt("y", 1);
				msg.setData(b);
				Navit.N_NavitGraphics.callback_handler.sendMessage(msg);

				// move
				msg = new Message();
				b = new Bundle();
				b.putInt("Callback", 23);
				b.putInt("x", 1 - 10);
				b.putInt("y", 1);
				msg.setData(b);
				Navit.N_NavitGraphics.callback_handler.sendMessage(msg);

				// up
				msg = new Message();
				b = new Bundle();
				b.putInt("Callback", 22);
				b.putInt("x", 1);
				b.putInt("y", 1);
				msg.setData(b);
				Navit.N_NavitGraphics.callback_handler.sendMessage(msg);
				// next lines are a hack, without it screen will not get updated anymore!
				// next lines are a hack, without it screen will not get updated anymore!
			}
		}

		if (this.draw_bubble)
		{
			//Log.e("Navit", "NavitAndroidOverlay -> onDraw -> bubble");

			int dx = 20;
			int dy = -100;
			Paint bubble_paint = new Paint(0);

			// yellow-ish funny lines
			int lx = 15;
			int ly = 15;
			bubble_paint.setStyle(Style.FILL);
			bubble_paint.setAntiAlias(true);
			bubble_paint.setStrokeWidth(8);
			bubble_paint.setColor(Color.parseColor("#FFF8C6"));
			c.drawLine(this.bubble_001.x + dx, this.bubble_001.y + dy + 60 - ly, this.bubble_001.x,
					this.bubble_001.y, bubble_paint);
			c.drawLine(this.bubble_001.x + dx + lx, this.bubble_001.y + dy + 60, this.bubble_001.x,
					this.bubble_001.y, bubble_paint);

			// draw black funny lines to target
			bubble_paint.setStyle(Style.STROKE);
			bubble_paint.setAntiAlias(true);
			bubble_paint.setStrokeWidth(3);
			bubble_paint.setColor(Color.parseColor("#000000"));
			c.drawLine(this.bubble_001.x + dx, this.bubble_001.y + dy + 60 - ly, this.bubble_001.x,
					this.bubble_001.y, bubble_paint);
			c.drawLine(this.bubble_001.x + dx + lx, this.bubble_001.y + dy + 60, this.bubble_001.x,
					this.bubble_001.y, bubble_paint);


			// filled rect yellow-ish
			bubble_paint.setStyle(Style.FILL);
			bubble_paint.setStrokeWidth(0);
			bubble_paint.setAntiAlias(false);
			bubble_paint.setColor(Color.parseColor("#FFF8C6"));
			RectF box_rect = new RectF(this.bubble_001.x + dx, this.bubble_001.y + dy,
					this.bubble_001.x + 150 + dx, this.bubble_001.y + 60 + dy);
			int rx = 20;
			int ry = 20;
			c.drawRoundRect(box_rect, rx, ry, bubble_paint);

			// black outlined rect
			bubble_paint.setStyle(Style.STROKE);
			bubble_paint.setStrokeWidth(3);
			bubble_paint.setAntiAlias(true);
			bubble_paint.setColor(Color.parseColor("#000000"));
			c.drawRoundRect(box_rect, rx, ry, bubble_paint);

			int inner_dx = 30;
			int inner_dy = 36;
			bubble_paint.setAntiAlias(true);
			bubble_paint.setStyle(Style.FILL);
			bubble_paint.setTextSize(20);
			bubble_paint.setStrokeWidth(3);
			bubble_paint.setColor(Color.parseColor("#3b3131"));
			c.drawText("drive here", this.bubble_001.x + dx + inner_dx, this.bubble_001.y + dy
					+ inner_dy, bubble_paint);
		}

		// test, draw a grey rectangle on top layer!
		// c.drawRect(10, 10, 300, 200, paint);
	}
}
