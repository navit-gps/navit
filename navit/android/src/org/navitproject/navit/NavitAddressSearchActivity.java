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


import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Locale;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.TextView;


public class NavitAddressSearchActivity extends Activity
{
	private EditText     address_string;
	private CheckBox     pm_checkbox;
	private String       mCountry;
	private ImageButton  mCountryButton;

	public RelativeLayout	NavitAddressSearchActivity_layout;

	private int getDrawableID(String resourceName)
	{
		int drawableId = 0;
		try {
			Class<?> res = R.drawable.class;
			Field field = res.getField(resourceName);
			drawableId = field.getInt(null);
		}
		catch (Exception e) {
			Log.e("NavitAddressSearch", "Failure to get drawable id.", e);
		}
		return drawableId;
	}

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		getWindow().setFlags(WindowManager.LayoutParams.FLAG_BLUR_BEHIND,
				WindowManager.LayoutParams.FLAG_BLUR_BEHIND);
		LinearLayout panel = new LinearLayout(this);
		panel.setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
		panel.setOrientation(LinearLayout.VERTICAL);

		// address: label and text field
		SharedPreferences settings = getSharedPreferences(Navit.NAVIT_PREFS, MODE_PRIVATE);
		mCountry = settings.getString("DefaultCountry", null);

		if (mCountry == null)
		{
			Locale defaultLocale = Locale.getDefault();
			mCountry = defaultLocale.getCountry().toLowerCase(defaultLocale);
			SharedPreferences.Editor edit_settings = settings.edit();
			edit_settings.putString("DefaultCountry", mCountry);
			edit_settings.commit();
		}

		mCountryButton = new ImageButton(this);

		mCountryButton.setImageResource(getDrawableID("country_" + mCountry + "_32_32"));

		mCountryButton.setOnClickListener(new OnClickListener()
		{
			public void onClick(View v)
			{
				requestCountryDialog();
			}
		});

		// address: label and text field
		TextView addr_view = new TextView(this);
		addr_view.setText(Navit.get_text("Enter Destination")); //TRANS
		addr_view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20f);
		addr_view.setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT,
				LayoutParams.WRAP_CONTENT));
		addr_view.setPadding(4, 4, 4, 4);

		// partial match checkbox
		pm_checkbox = new CheckBox(this);
		pm_checkbox.setText(Navit.get_text("partial match")); //TRANS
		pm_checkbox.setChecked(false);
		pm_checkbox.setGravity(Gravity.CENTER);


		// search button
		final Button btnSearch = new Button(this);
		btnSearch.setText(Navit.get_text("Search")); //TRANS
		btnSearch.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT,
				LayoutParams.WRAP_CONTENT));
		btnSearch.setGravity(Gravity.CENTER);
		btnSearch.setOnClickListener(new OnClickListener()
		{
			public void onClick(View v)
			{
				executeDone();
			}
		});

		Bundle extras = getIntent().getExtras();

		if ( extras != null )
		{
			String title = extras.getString("title");
			String partial = extras.getString("partial_match");
			String address = extras.getString("address_string");

			if ( title != null && title.length() > 0)
				this.setTitle(title);

			if (partial != null && partial.length() > 0)
				pm_checkbox.setChecked(partial.equals("1"));

			address_string = new EditText(this);
			if (address != null)
				address_string.setText(address);
		}

		LinearLayout searchSettingsLayout = new LinearLayout(this);
		searchSettingsLayout.setOrientation(LinearLayout.HORIZONTAL);

		searchSettingsLayout.addView(mCountryButton);
		searchSettingsLayout.addView(pm_checkbox);
		panel.addView(addr_view);
		panel.addView(address_string);
		panel.addView(searchSettingsLayout);
		panel.addView(btnSearch);

		setContentView(panel);
	}

	private void requestCountryDialog()
	{
		final String [][]all_countries = NavitGraphics.GetAllCountries();

		Comparator<String[]> country_comperator = new Comparator<String[]>(){
			@Override
			public int compare(String[] object1, String[] object2) {
				return object1[1].compareTo(object2[1]);
			}};

		Arrays.sort(all_countries, country_comperator );

		AlertDialog.Builder mapModeChooser = new AlertDialog.Builder(this);
		// ToDo also show icons and country code
		String []country_name = new String[all_countries.length];

		for (int country_index = 0; country_index < all_countries.length; country_index++)
		{
			country_name[country_index] = all_countries[country_index][1];
		}

		mapModeChooser.setItems(country_name, new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int item) {
				SharedPreferences settings = getSharedPreferences(Navit.NAVIT_PREFS, MODE_PRIVATE);
				mCountry = all_countries[item][0];
				SharedPreferences.Editor edit_settings = settings.edit();
				edit_settings.putString("DefaultCountry", mCountry);
				edit_settings.commit();

				mCountryButton.setImageResource(getDrawableID("country_" + mCountry + "_32_32"));
		 	}
		});

		mapModeChooser.show();
	}

	private void executeDone()
	{
		Intent resultIntent = new Intent();
		resultIntent.putExtra("address_string", address_string.getText().toString());
		resultIntent.putExtra("country", mCountry);
		if (pm_checkbox.isChecked())
		{
			resultIntent.putExtra("partial_match", "1");
		}
		else
		{
			resultIntent.putExtra("partial_match", "0");
		}
		setResult(Activity.RESULT_OK, resultIntent);
		finish();
	}
}
