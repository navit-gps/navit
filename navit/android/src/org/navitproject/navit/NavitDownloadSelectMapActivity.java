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

import java.util.HashMap;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ExpandableListActivity;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.ExpandableListView;
import android.widget.RelativeLayout;
import android.widget.SimpleExpandableListAdapter;
import android.widget.TextView;

public class NavitDownloadSelectMapActivity extends ExpandableListActivity {

	private NavitMapDownloader.MapMenu   mapMenu                                = null;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		mapMenu = NavitMapDownloader.getMenu();

		SimpleExpandableListAdapter adapter =
		        new SimpleExpandableListAdapter(this,
		        		mapMenu.groupList,
		                android.R.layout.simple_expandable_list_item_1,
		                new String[] { "category_name" },
		                new int[] { android.R.id.text1 },
		                mapMenu.childList,
		                android.R.layout.simple_expandable_list_item_1, 
		                new String[] { "map_name" },
		                new int[] { android.R.id.text1 }
		        );
		setListAdapter(adapter);
		setTitle(String.valueOf(NavitMapDownloader.getFreeSpace()/1024/1024) + "MB available");
	}

	@Override
	public boolean onChildClick(ExpandableListView parent, View v, int groupPosition, int childPosition, long id) {
		super.onChildClick(parent, v, groupPosition, childPosition, id);
		Log.d("Navit", "p:" + groupPosition + ", child_pos:" + childPosition);
		HashMap<String, String> map = mapMenu.childList.get(groupPosition).get(childPosition);

		String map_index = map.get("map_index");
		if (map_index != null) {
			Intent resultIntent = new Intent();
			resultIntent.putExtra("map_index", Integer.parseInt(map_index));
			setResult(Activity.RESULT_OK, resultIntent);
			finish();
		}
		else
		{
			// ask user if to delete this map
			askForMapDeletion(map.get("map_location"));
		}
		return true;
	}
	
	private void askForMapDeletion(final String map_location)
	{
		AlertDialog.Builder deleteMapBox = new AlertDialog.Builder(this);
		deleteMapBox.setTitle(getString(R.string.map_delete)); // TRANS
		deleteMapBox.setCancelable(true);
		final TextView message = new TextView(this);
		message.setFadingEdgeLength(20);
		message.setVerticalFadingEdgeEnabled(true);
		RelativeLayout.LayoutParams layoutParams = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.FILL_PARENT, RelativeLayout.LayoutParams.FILL_PARENT);

		message.setLayoutParams(layoutParams);
		NavitMap maptoDelete = new NavitMap(map_location);
		message.setText(maptoDelete.mapName + " " + String.valueOf(maptoDelete.size()/1024/1024)+ "MB");
		deleteMapBox.setView(message);

		// TRANS
		deleteMapBox.setPositiveButton(getString(R.string.yes), new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface arg0, int arg1) {
				Log.e("Navit", "Delete Map");
				Message msg = Message.obtain(Navit.N_NavitGraphics.callback_handler, NavitGraphics.msg_type.CLB_DELETE_MAP.ordinal());
				Bundle b = new Bundle();
				b.putString("title", map_location);
				msg.setData(b);
				msg.sendToTarget();
				finish();
			}
		});

		// TRANS
		deleteMapBox.setNegativeButton(getString(R.string.no), new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface arg0, int arg1) {
				Log.e("Navit", "don't delete map");
			}
		});
		deleteMapBox.show();
	}
}
