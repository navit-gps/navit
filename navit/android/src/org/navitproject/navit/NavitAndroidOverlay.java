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
import android.graphics.Paint.Style;
import android.graphics.RectF;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.MotionEvent;
import android.widget.ImageView;

public class NavitAndroidOverlay extends ImageView implements Runnable
{
	public Boolean					draw_bubble							= false;
	public static Boolean		confirmed_bubble					= false;
	public static long			bubble_max_showing_timespan	= 8000L; // 8 secs.

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
			Handler handler = new Handler();
			handler.postDelayed(this, bubble_max_showing_timespan);
			NavitAndroidOverlay.confirmed_bubble = false;
			this.draw_bubble = true;
		}
	}

	public Boolean get_show_bubble()
	{
		return this.draw_bubble;
	}

	public void hide_bubble() {
		confirmed_bubble = false;
		draw_bubble = false;
		Message msg = Message.obtain(Navit.N_NavitGraphics.callback_handler,
				NavitGraphics.msg_type.CLB_REDRAW.ordinal());
		msg.sendToTarget();
		postInvalidate();
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

		int x = (int) event.getX();
		int y = (int) event.getY();

		if ((this.draw_bubble) && (!NavitAndroidOverlay.confirmed_bubble))
		{
			// bubble is showing, test if we touch it to confirm destination
			float draw_factor = 1.0f;
			if (Navit.my_display_density.compareTo("mdpi") == 0)
			{
				draw_factor = 1.0f;
			}
			else if (Navit.my_display_density.compareTo("ldpi") == 0)
			{
				draw_factor = 0.7f;
			}
			else if (Navit.my_display_density.compareTo("hdpi") == 0)
			{
				draw_factor = 1.5f;
			}
			int dx = (int) ((20 / 1.5f) * draw_factor);
			int dy = (int) ((-100 / 1.5f) * draw_factor);
			int bubble_size_x = (int) ((150 / 1.5f) * draw_factor);
			int bubble_size_y = (int) ((60 / 1.5f) * draw_factor);
			RectF box_rect = new RectF(this.bubble_001.x + dx, this.bubble_001.y + dy,
					this.bubble_001.x + bubble_size_x + dx, this.bubble_001.y + bubble_size_y + dy);
			if (box_rect.contains(x, y))
			{
				// bubble touched to confirm destination
				NavitAndroidOverlay.confirmed_bubble = true;
				// draw confirmed bubble
				this.postInvalidate();

				// set destination
				Message msg = Message.obtain(Navit.N_NavitGraphics.callback_handler,
						NavitGraphics.msg_type.CLB_SET_DISPLAY_DESTINATION.ordinal());

				Bundle b = new Bundle();
				b.putInt("x", this.bubble_001.x);
				b.putInt("y", this.bubble_001.y);
				msg.setData(b);
				msg.sendToTarget();

				// consume the event
				return true;
			}
		}

		// false -> we dont use this event, give it to other layers
		return false;
	}

	public void onDraw(Canvas c)
	{
		//Log.e("Navit", "NavitAndroidOverlay -> onDraw");

		float draw_factor = 1.0f;
		if (Navit.my_display_density.compareTo("mdpi") == 0)
		{
			draw_factor = 1.0f;
		}
		else if (Navit.my_display_density.compareTo("ldpi") == 0)
		{
			draw_factor = 0.7f;
		}
		else if (Navit.my_display_density.compareTo("hdpi") == 0)
		{
			draw_factor = 1.5f;
		}


		if (this.draw_bubble)
		{
			//Log.e("Navit", "NavitAndroidOverlay -> onDraw -> bubble");

			int dx = (int) ((20 / 1.5f) * draw_factor);
			int dy = (int) ((-100 / 1.5f) * draw_factor);
			Paint bubble_paint = new Paint(0);

			int bubble_size_x = (int) ((150 / 1.5f) * draw_factor);
			int bubble_size_y = (int) ((60 / 1.5f) * draw_factor);

			// yellow-ish funny lines
			int lx = (int) ((15 / 1.5f) * draw_factor);
			int ly = (int) ((15 / 1.5f) * draw_factor);
			bubble_paint.setStyle(Style.FILL);
			bubble_paint.setAntiAlias(true);
			bubble_paint.setStrokeWidth(8 / 1.5f * draw_factor);
			bubble_paint.setColor(Color.parseColor("#FFF8C6"));
			c.drawLine(this.bubble_001.x + dx, this.bubble_001.y + dy + bubble_size_y - ly,
					this.bubble_001.x, this.bubble_001.y, bubble_paint);
			c.drawLine(this.bubble_001.x + dx + lx, this.bubble_001.y + dy + bubble_size_y,
					this.bubble_001.x, this.bubble_001.y, bubble_paint);

			// draw black funny lines to target
			bubble_paint.setStyle(Style.STROKE);
			bubble_paint.setAntiAlias(true);
			bubble_paint.setStrokeWidth(3);
			bubble_paint.setColor(Color.parseColor("#000000"));
			c.drawLine(this.bubble_001.x + dx, this.bubble_001.y + dy + bubble_size_y - ly,
					this.bubble_001.x, this.bubble_001.y, bubble_paint);
			c.drawLine(this.bubble_001.x + dx + lx, this.bubble_001.y + dy + bubble_size_y,
					this.bubble_001.x, this.bubble_001.y, bubble_paint);


			// filled rect yellow-ish
			bubble_paint.setStyle(Style.FILL);
			bubble_paint.setStrokeWidth(0);
			bubble_paint.setAntiAlias(false);
			bubble_paint.setColor(Color.parseColor("#FFF8C6"));
			RectF box_rect = new RectF(this.bubble_001.x + dx, this.bubble_001.y + dy,
					this.bubble_001.x + bubble_size_x + dx, this.bubble_001.y + bubble_size_y + dy);
			int rx = (int) (20 / 1.5f * draw_factor);
			int ry = (int) (20 / 1.5f * draw_factor);
			c.drawRoundRect(box_rect, rx, ry, bubble_paint);

			if (NavitAndroidOverlay.confirmed_bubble)
			{
				// filled red rect (for confirmed bubble)
				//bubble_paint.setStyle(Style.FILL);
				//bubble_paint.setStrokeWidth(0);
				//bubble_paint.setAntiAlias(false);
				bubble_paint.setColor(Color.parseColor("#EC294D"));
				c.drawRoundRect(box_rect, rx, ry, bubble_paint);
			}

			// black outlined rect
			bubble_paint.setStyle(Style.STROKE);
			bubble_paint.setStrokeWidth(3);
			bubble_paint.setAntiAlias(true);
			bubble_paint.setColor(Color.parseColor("#000000"));
			c.drawRoundRect(box_rect, rx, ry, bubble_paint);

			int inner_dx = (int) (30 / 1.5f * draw_factor);
			int inner_dy = (int) (36 / 1.5f * draw_factor);
			bubble_paint.setAntiAlias(true);
			bubble_paint.setStyle(Style.FILL);
			bubble_paint.setTextSize((int) (20 / 1.5f * draw_factor));
			bubble_paint.setStrokeWidth(3);
			bubble_paint.setColor(Color.parseColor("#3b3131"));
			c.drawText(Navit.get_text("drive here"), this.bubble_001.x + dx + inner_dx,
					this.bubble_001.y + dy + inner_dy, bubble_paint);

		}
	}

	public void run() {
		hide_bubble();
	}
}
