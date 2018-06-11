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

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
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
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.TextView;
import android.widget.Toast;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;

public class NavitAddressSearchActivity extends Activity {
    public static final class NavitAddress {
        public NavitAddress(int type, float latitude, float longitude, String address) {
            result_type = type;
            lat = latitude;
            lon = longitude;
            addr = address;
        }

        final int    result_type;
        final float  lat;
        final float  lon;
        final String addr;
    }

    private static final String TAG                         = "NavitAddress";
    private static final int    ADDRESS_RESULT_PROGRESS_MAX = 10;

    private List<NavitAddress> Addresses_found              = null;
    private List<NavitAddress> addresses_shown              = null;
    private String             mAddressString               = "";
    private boolean            mPartialSearch               = false;
    private String             mCountry;
    private ImageButton        mCountryButton;
    private ProgressDialog             search_results_wait          = null;
    public RelativeLayout      NavitAddressSearchActivity_layout;
    private int                search_results_towns           = 0;
    private int                search_results_streets         = 0;
    private int                search_results_streets_hn      = 0;
    private long               search_handle                  = 0;

    // TODO remember settings
    private static String               last_address_search_string = "";
    private static Boolean              last_address_partial_match = false;
    private static String               last_country = "";

    private int getDrawableID(String resourceName) {
        int drawableId = 0;
        try {
            Class<?> res = R.drawable.class;
            Field field = res.getField(resourceName);
            drawableId = field.getInt(null);
        } catch (Exception e) {
            Log.e(TAG, "Failure to get drawable id.", e);
        }
        return drawableId;
    }

    private void setCountryButtonImage() {
        // We have all images stored as drawable_nodpi resources which allows native code to manipulate them
        // without interference with android builtin choosing and scaling system. But that makes us to
        // reinvent the wheel here to show an image in android native interface.
        int[] flag_icon_sizes = {24,32,48,64,96};
        int exact_size, nearest_size;
        exact_size = (int)(Navit.metrics.density*24.0 -.5);
        nearest_size = flag_icon_sizes[0];
        for(int size: flag_icon_sizes) {
            nearest_size = size;
            if(exact_size <= size)
                break;
        }
        mCountryButton.setImageResource(getDrawableID("country_" + mCountry+"_"+nearest_size+"_"+nearest_size));
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Bundle extras = getIntent().getExtras();
        if ( extras != null ) {
            String search_string = extras.getString(("search_string"));
            if (search_string != null) {
                mPartialSearch = true;
                mAddressString = search_string;
                executeSearch();
                return;
            }
        }

        mPartialSearch = last_address_partial_match;
        mAddressString = last_address_search_string;

        getWindow().setFlags(WindowManager.LayoutParams.FLAG_BLUR_BEHIND, WindowManager.LayoutParams.FLAG_BLUR_BEHIND);
        LinearLayout panel = new LinearLayout(this);
        panel.setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
        panel.setOrientation(LinearLayout.VERTICAL);

        // address: label and text field
        SharedPreferences settings = getSharedPreferences(Navit.NAVIT_PREFS, MODE_PRIVATE);
        mCountry = settings.getString(("DefaultCountry"), null);

        if (mCountry == null) {
            Locale defaultLocale = Locale.getDefault();
            mCountry = defaultLocale.getCountry().toLowerCase(defaultLocale);
            SharedPreferences.Editor edit_settings = settings.edit();
            edit_settings.putString("DefaultCountry", mCountry);
            edit_settings.apply();
        }

        mCountryButton = new ImageButton(this);

        setCountryButtonImage();

        mCountryButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                requestCountryDialog();
            }
        });

        // address: label and text field
        TextView addr_view = new TextView(this);
        addr_view.setText(Navit.getInstance().getTstring(R.string.address_enter_destination)); // TRANS
        addr_view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20f);
        addr_view.setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
        addr_view.setPadding(4, 4, 4, 4);

        // partial match checkbox
        final CheckBox checkboxPartialMatch = new CheckBox(this);
        checkboxPartialMatch.setText(Navit.getInstance().getTstring(R.string.address_partial_match)); // TRANS
        checkboxPartialMatch.setChecked(last_address_partial_match);
        checkboxPartialMatch.setGravity(Gravity.CENTER);

        final EditText address_string = new EditText(this);
        address_string.setText(last_address_search_string);
        address_string.setSelectAllOnFocus(true);

        // search button
        final Button btnSearch = new Button(this);
        btnSearch.setText(Navit.getInstance().getTstring(R.string.address_search_button)); // TRANS
        btnSearch.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.WRAP_CONTENT));
        btnSearch.setGravity(Gravity.CENTER);
        btnSearch.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                mPartialSearch = checkboxPartialMatch.isChecked();
                mAddressString = address_string.getText().toString();
                last_address_partial_match = mPartialSearch;
                last_address_search_string = mAddressString;
                executeSearch();
            }
        });

        ListView lastAddresses = new ListView(this);
        NavitAppConfig navitConfig = (NavitAppConfig) getApplicationContext();

        final List<NavitAddress> addresses = navitConfig.getLastAddresses();
        int addressCount = addresses.size();
        if (addressCount > 0) {
            String[] strAddresses = new String[addressCount];
            for (int addrIndex = 0; addrIndex < addressCount; addrIndex++) {
                strAddresses[addrIndex] = addresses.get(addrIndex).addr;
            }
            ArrayAdapter<String> addressList =
                new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1, strAddresses);
            lastAddresses.setAdapter(addressList);
            lastAddresses.setOnItemClickListener(new OnItemClickListener() {
                public void onItemClick(AdapterView<?> arg0, View arg1, int arg2, long arg3) {
                    NavitAddress addressSelected = addresses.get(arg2);
                    Intent resultIntent = new Intent();

                    resultIntent.putExtra("lat", addressSelected.lat);
                    resultIntent.putExtra("lon", addressSelected.lon);
                    resultIntent.putExtra("q", addressSelected.addr);

                    setResult(Activity.RESULT_OK, resultIntent);
                    finish();
                }
            });
        }

        String title = getString(R.string.address_search_title);
        this.setTitle(title);

        LinearLayout searchSettingsLayout = new LinearLayout(this);
        searchSettingsLayout.setOrientation(LinearLayout.HORIZONTAL);

        searchSettingsLayout.addView(mCountryButton);
        searchSettingsLayout.addView(checkboxPartialMatch);
        panel.addView(addr_view);
        panel.addView(address_string);
        panel.addView(searchSettingsLayout);
        panel.addView(btnSearch);
        panel.addView(lastAddresses);

        setContentView(panel);
    }

    private void requestCountryDialog() {
        final String[][] all_countries = NavitGraphics.GetAllCountries();

        Comparator<String[]> country_comperator = new Comparator<String[]>() {
            public int compare(String[] object1, String[] object2) {
                return object1[1].compareTo(object2[1]);
            }
        };

        Arrays.sort(all_countries, country_comperator);

        AlertDialog.Builder mapModeChooser = new AlertDialog.Builder(this);
        // ToDo also show icons and country code
        String[] country_name = new String[all_countries.length];

        for (int country_index = 0; country_index < all_countries.length; country_index++) {
            country_name[country_index] = all_countries[country_index][1];
        }

        mapModeChooser.setItems(country_name, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int item) {
                SharedPreferences settings = getSharedPreferences(Navit.NAVIT_PREFS, MODE_PRIVATE);
                mCountry = all_countries[item][0];
                SharedPreferences.Editor edit_settings = settings.edit();
                edit_settings.putString("DefaultCountry", mCountry);
                edit_settings.apply();

                setCountryButtonImage();
            }
        });

        AlertDialog d=mapModeChooser.create();
        d.getListView().setFastScrollEnabled(true);
        d.show();
    }

    /**
     * start a search on the map
     */
    public void receiveAddress(int type, float latitude, float longitude, String address) {
        Log.e(TAG, "(" + String.valueOf(latitude) + ", " + String.valueOf(longitude) + ") " + address);

        switch (type) {
        case 0:
            search_results_towns++;
            break;
        case 1:
            search_results_streets++;
            break;
        case 2:
            search_results_streets_hn++;
            break;

        }
        search_results_wait.setMessage(Navit.getInstance().getTstring(R.string.address_search_towns) + ":" +
                                       search_results_towns + " "
                                       + Navit.getInstance().getTstring(R.string.address_search_streets) + ":" + search_results_streets + "/"
                                       + search_results_streets_hn);

        search_results_wait.setProgress(Addresses_found.size() % (ADDRESS_RESULT_PROGRESS_MAX + 1));

        Addresses_found.add(new NavitAddress(type, latitude, longitude, address));
    }

    public void finishAddressSearch() {
        if (Addresses_found.isEmpty()) {
            Toast.makeText( getApplicationContext(),getString(R.string.address_search_not_found) + "\n" + mAddressString,
                            Toast.LENGTH_LONG).show(); //TRANS
            setResult(Activity.RESULT_CANCELED);
            finish();
        }
        ListView addressesFound = new ListView(this);
        addressesFound.setFastScrollEnabled(true);
        ArrayAdapter<String> addressList =
            new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1);

        addresses_shown = new ArrayList<NavitAddress>();

        for (NavitAddress currentAddress : Addresses_found) {
            if (currentAddress.result_type != 0 || search_results_streets == 0) {
                addressList.add(currentAddress.addr);
                addresses_shown.add(currentAddress);
            }
        }

        addressesFound.setAdapter(addressList);

        addressesFound.setOnItemClickListener(new OnItemClickListener() {
            public void onItemClick(AdapterView<?> arg0, View arg1, int arg2, long arg3) {
                NavitAddress addressSelected = addresses_shown.get(arg2);
                Intent resultIntent = new Intent();

                resultIntent.putExtra("lat", addressSelected.lat);
                resultIntent.putExtra("lon", addressSelected.lon);
                resultIntent.putExtra("q", addressSelected.addr);

                setResult(Activity.RESULT_OK, resultIntent);
                finish();
            }
        });

        setContentView(addressesFound);
        search_results_wait.dismiss();
    }

    public native long CallbackStartAddressSearch(int partial_match, String country, String s);
    public native void CallbackCancelAddressSearch(long handle);

    @Override
    protected Dialog onCreateDialog(int id) {
        search_results_wait = new ProgressDialog(this);
        search_results_wait.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        search_results_wait.setTitle("Loading search results");
        search_results_wait.setMessage("--");
        search_results_wait.setCancelable(true);
        search_results_wait.setProgress(0);
        search_results_wait.setMax(10);

        Addresses_found = new ArrayList<NavitAddress>();
        search_results_towns = 0;
        search_results_streets = 0;
        search_results_streets_hn = 0;

        search_handle = CallbackStartAddressSearch(mPartialSearch ? 1 : 0, mCountry, mAddressString);

        search_results_wait.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                CallbackCancelAddressSearch(search_handle);
                search_handle = 0;
                search_results_wait.dismiss();
            }
        });
        return search_results_wait;
    }

    private void executeSearch() {
        showDialog(0);
    }
}

