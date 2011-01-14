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
import android.view.MotionEvent;
import android.widget.ImageView;

public class NavitAndroidOverlay extends ImageView
{
	public NavitAndroidOverlay(Context context)
	{
		super(context);
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

		// test, draw a grey rectangle on top layer!
		// c.drawRect(10, 10, 300, 200, paint);
	}
}
