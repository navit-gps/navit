package org.navitproject.navit;

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

import java.util.Iterator;

import android.app.Activity;
import android.app.ListActivity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.ListView;

public class NavitAddressResultListActivity extends ListActivity
{

	private int			selected_id	= -1;
	private Boolean	is_empty		= true;
	public String[]	result_list	= new String[]{"loading results ..."};

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		//Log.e("Navit", "all ok");

		Navit.Navit_Address_Result_Struct tmp = new Navit.Navit_Address_Result_Struct();

		Log.e("Navit", "########### full result count: "
				+ Navit.NavitAddressResultList_foundItems.size());

		// show "town names" as results only when we dont have any street names in resultlist
		if ((Navit.search_results_streets > 0) || (Navit.search_results_streets_hn > 0))
		{
			// clear out towns from result list
			for (Iterator<Navit.Navit_Address_Result_Struct> k = Navit.NavitAddressResultList_foundItems
					.iterator(); k.hasNext();)
			{
				tmp = k.next();
				if (tmp.result_type.equals("TWN"))
				{
					k.remove();
				}
			}
		}

		Log.e("Navit", "########### final result count: "
				+ Navit.NavitAddressResultList_foundItems.size());

		this.result_list = new String[Navit.NavitAddressResultList_foundItems.size()];
		int j = 0;
		for (Iterator<Navit.Navit_Address_Result_Struct> i = Navit.NavitAddressResultList_foundItems
				.iterator(); i.hasNext();)
		{
			tmp = i.next();
			this.result_list[j] = tmp.addr;
			j++;
		}

		ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,
				android.R.layout.simple_list_item_1, result_list);
		setListAdapter(adapter);
		is_empty = true;
	}

	public void add_item_(String item)
	{
		if (item == null)
		{
			// empty item?
			return;
		}

		if (this.is_empty)
		{
			// clear dummy text, and add this item
			this.result_list = new String[1];
			this.result_list[0] = item;
		}
		else
		{
			// add the item to the end of the list
			String[] tmp_list = this.result_list;
			this.result_list = new String[tmp_list.length + 1];
			for (int i = 0; i < tmp_list.length; i = i + 1)
			{
				this.result_list[i] = tmp_list[i];
			}
			this.result_list[tmp_list.length] = item;
		}
		ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,
				android.R.layout.simple_list_item_1, result_list);
		setListAdapter(adapter);
		this.is_empty = false;
	}

	@Override
	protected void onListItemClick(ListView l, View v, int position, long id)
	{
		super.onListItemClick(l, v, position, id);
		this.selected_id = position;
		Log.e("Navit", "p:" + position);
		Log.e("Navit", "i:" + id);

		// close this activity
		executeDone();
	}

	//	@Override
	//	public void onBackPressed()
	//	{
	//		executeDone();
	//		super.onBackPressed();
	//	}

	private void executeDone()
	{
		Intent resultIntent = new Intent();
		resultIntent.putExtra("selected_id", String.valueOf(this.selected_id));
		setResult(Activity.RESULT_OK, resultIntent);
		finish();
	}

}
