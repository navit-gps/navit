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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import android.app.Activity;
import android.app.ExpandableListActivity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.util.Pair;
import android.view.View;
import android.widget.ExpandableListView;
import android.widget.SimpleExpandableListAdapter;

public class NavitDownloadSelectMapActivity extends ExpandableListActivity {

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		Pair<List<HashMap<String, String>>, ArrayList<ArrayList<HashMap<String, String>>> > listPair = 
			NavitMapDownloader.getMenu();

		SimpleExpandableListAdapter adapter =
		        new SimpleExpandableListAdapter(this,
		                listPair.first,
		                android.R.layout.simple_expandable_list_item_1, // Group itemlayout XML.
		                new String[] { "map_name" }, // the key of group item.
		                new int[] { android.R.id.text1 }, // ID of each group
		                                             // item.-Data under the key
		                                             // goes into this
		                listPair.second, // childData describes second-level
		                                   // entries.
		                android.R.layout.simple_expandable_list_item_1, // Layout for sub-level
		                                    // entries(second level).
		                new String[] { "map_name" }, // Keys in childData maps
		                                             // to display.
		                new int[] { android.R.id.text1 } // Data under the keys
		                                             // above go into these
		                                             // TextViews.
		        );
		setListAdapter(adapter);
		setTitle(String.valueOf(NavitMapDownloader.getFreeSpace()/1024/1024) + "MB available");
	}

	@Override
	public boolean onChildClick(ExpandableListView parent, View v, int groupPosition, int childPosition, long id) {
		super.onChildClick(parent, v, groupPosition, childPosition, id);
		Log.d("Navit", "p:" + groupPosition + ", child_pos:" + childPosition);
		HashMap<String, String> map = NavitMapDownloader.getMenu().second.get(groupPosition).get(childPosition);
		Intent resultIntent = new Intent();
		resultIntent.putExtra("map_index", Integer.parseInt(map.get("map_index")));
		setResult(Activity.RESULT_OK, resultIntent);
		finish();
		return true;
	}
}
