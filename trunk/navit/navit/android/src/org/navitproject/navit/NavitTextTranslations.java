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

import android.util.Log;

public class NavitTextTranslations
{
	static String main_language = "en";
	static String sub_language = "EN";
	static String fallback_language = "en";
	static String fallback_sub_language = "EN";
	private static HashMap<String, HashMap<String, String>>	Navit_text_lookup = new HashMap<String, HashMap<String, String>>();

	public static void init()
	{
		Log.e("NavitTextTranslations", "initializing translated text ...");
		String k = null;
		String[] v = null;

		k = "exit navit";
		v = new String[]{"en", "Exit Navit", "de", "Navit beenden", "nl", "Navit afsluiten", "fr","Quittez Navit"};
		p(k, v);

		k = "zoom in";
		v = new String[]{"en", "Zoom in", "fr", "Zoom-avant"};
		p(k, v);

		k = "zoom out";
		v = new String[]{"en", "Zoom out", "fr", "Zoom-arrière", "nl", "Zoom uit"};
		p(k, v);

		k = "address search";
		v = new String[]{"en", "Address search", "de", "Adresse suchen", "nl", "Zoek adres", "fr","Cherchez adresse"};
		p(k, v);

		k = "Mapdownload";
		v = new String[]{"en", "Mapdownload", "de", "Kartendownload"};
		p(k, v);

		k = "downloading";
		v = new String[]{"en", "downloading"};
		p(k, v);

		k = "Downloaded Maps";
		v = new String[]{"en", "Downloaded Maps", "de", "Heruntergeladene Karten", "nl", "Gedownloade kaarten", "fr","Cartes téléchargées" };
		p(k, v);

		k = "ETA";
		v = new String[]{"en", "ETA", "de", "fertig in"};
		p(k, v);

		k = "Error downloading map!";
		v = new String[]{"en", "Error downloading map!", "de", "Fehler beim Kartendownload"};
		p(k, v);

		k = "ready";
		v = new String[]{"en", "ready", "de", "fertig"};
		p(k, v);

		k = "Ok";
		v = new String[]{"en", "OK"};
		p(k, v);

		k = "No address found";
		v = new String[]{"en", "No address found", "de", "Keine Adresse gefunden"};
		p(k, v);

		k = "Enter: City and Street";
		v = new String[]{"en", "Enter: City, Street", "de", "Stadt und Straße:"};
		p(k, v);

		k = "No search string entered";
		v = new String[]{"en", "No text entered", "de", "Keine Eingabe"};
		p(k, v);

		k = "setting destination to";
		v = new String[]{"en", "Setting destination to:", "de", "neues Fahrziel"};
		p(k, v);

		k = "getting search results";
		v = new String[]{"en", "getting search results", "de", "lade Suchergebnisse"};
		p(k, v);

		k = "searching ...";
		v = new String[]{"en", "searching ...", "de", "Suche läuft ..."};
		p(k, v);

		k = "No Results found!";
		v = new String[]{"en", "No Results found!", "de", "Suche liefert kein Ergebnis!"};
		p(k, v);

		k = "Map data (c) OpenStreetMap contributors, CC-BY-SA";
		v = new String[]{"en", "Map data (c) OpenStreetMap contributors, CC-BY-SA"};
		p(k, v);

		k = "partial match";
		v = new String[]{"en", "partial match", "de", "ungefähr"};
		p(k, v);

		k = "Search";
		v = new String[]{"en", "Search", "de", "suchen"};
		p(k, v);

		k = "drive here";
		v = new String[]{"en", "Route to here", "de", "Ziel setzen"};
		p(k, v);

		k = "loading search results";
		v = new String[]{"en", "Loading search results", "de", "lade Suchergebnisse"};
		p(k, v);

		k = "towns";
		v = new String[]{"en", "Towns", "de", "Städte"};
		p(k, v);

		Log.e("NavitTextTranslations", "... ready");
	}

	private static void p(String key, String[] values)
	{
		HashMap<String, String> t = new HashMap<String, String>();
		//Log.e("NavitTextTranslations", "trying: " + key);
		try
		{
			for (int i = 0; i < (int) (values.length / 2); i++)
			{
				t.put(values[i * 2], values[(i * 2) + 1]);
			}
			Navit_text_lookup.put(key, t);
		}
		catch (Exception e)
		{
			Log.e("NavitTextTranslations", "!!Error in translationkey: " + key);
		}
	}

	public static String get_text(String in)
	{
		String out = null;
		//Log.e("NavitTextTranslations", "lookup L:" + main_language + " T:" + in);
		try
		{
			out = Navit_text_lookup.get(in).get(main_language);
		}
		catch (Exception e)
		{
			// most likely there is not translation yet
			//Log.e("NavitTextTranslations", "lookup: exception");
			out = null;
		}

		if (out == null)
		{
			// always return a string for output (use fallback language)
			//Log.e("NavitTextTranslations", "using default language");
			try
			{
				out = Navit_text_lookup.get(in).get(fallback_language);
			}
			catch (Exception e)
			{
				//Log.e("NavitTextTranslations", "using default language: exception");
				// most likely there is not translation yet
				out = null;
			}
		}

		if (out == null)
		{
			// if we still dont have any text, use the ".mo" file and call the c-function gettext(in)
			out = NavitGraphics.getLocalizedString(in);
			if (out != null)
			{
				HashMap<String, String> langmap = new HashMap<String, String>();
				langmap.put(main_language, out);
				Navit_text_lookup.put(in, langmap);
			}

			//Log.e("NavitTextTranslations", "return the value from gettext() = " + out);
		}
		return out;
	}

}
