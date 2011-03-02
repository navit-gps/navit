package org.navitproject.navit;

import java.util.HashMap;

import android.util.Log;

public class NavitTextTranslations
{
	static String															main_language									= "en";
	static String															sub_language									= "EN";
	static String															fallback_language								= "en";
	static String															fallback_sub_language						= "EN";
	private static HashMap<String, HashMap<String, String>>	Navit_text_lookup								= new HashMap<String, HashMap<String, String>>();


	// this part will be removed *******************
	// this part will be removed *******************
	// this part will be removed *******************
	// space !!
	static final String													m													= " ";

	static final String													NAVIT_JAVA_MENU_download_first_map_en	= "download first map";
	static final String													NAVIT_JAVA_MENU_download_first_map_fr	= "télécharchez 1ere carte";
	static final String													NAVIT_JAVA_MENU_download_first_map_nl	= "download eerste kaart";
	static final String													NAVIT_JAVA_MENU_download_first_map_de	= "1te karte runterladen";

	static final String													INFO_BOX_TITLE_en								= "Welcome to Navit";
	static final String													INFO_BOX_TITLE_fr								= "Bienvenue chez Navit";
	static final String													INFO_BOX_TITLE_nl								= "Welkom bij Navit";
	static final String													INFO_BOX_TITLE_de								= "Willkommen bei Navit";

	static final String													INFO_BOX_TEXT_en								= m
																																			+ "You are running Navit for the first time!\n\n"
																																			+ m
																																			+ "To start select \""
																																			+ NAVIT_JAVA_MENU_download_first_map_en
																																			+ "\"\n"
																																			+ m
																																			+ "from the menu, and download a map\n"
																																			+ m
																																			+ "for your current Area.\n"
																																			+ m
																																			+ "This will download a large file, so please\n"
																																			+ m
																																			+ "make sure you have a flatrate or similar!\n\n"
																																			+ m
																																			+ "Mapdata:\n"
																																			+ m
																																			+ "CC-BY-SA OpenStreetMap Project\n\n"
																																			+ m
																																			+ "For more information on Navit\n"
																																			+ m
																																			+ "visit our Website\n"
																																			+ m
																																			+ "http://wiki.navit-project.org/\n"
																																			+ "\n"
																																			+ m
																																			+ "      Have fun using Navit.";
	static final String													INFO_BOX_TEXT_fr								= m

																																			+ "Vous exécutez Navit pour la première fois\n\n"
																																			+ m
																																			+ "Pour commencer, sélectionnez \n \""

																																			+ NAVIT_JAVA_MENU_download_first_map_fr
																																			+ "\"\n"
																																			+ m

																																			+ "du menu et télechargez une carte\n de votre région.\n"
																																			+ m

																																			+ "Les cartes sont volumineux, donc\n il est préférable d'avoir une connection\n internet illimitée!\n\n"
																																			+ m
																																			+ "Cartes:\n"
																																			+ m
																																			+ "CC-BY-SA OpenStreetMap Project\n\n"
																																			+ m
																																			+ "Pour plus d'infos sur Navit\n"
																																			+ m
																																			+ "visitez notre site internet\n"
																																			+ m

																																			+ "http://wiki.navit-project.org/\n"
																																			+ "\n"
																																			+ m
																																			+ "      Amusez vous avec Navit.";
	static final String													INFO_BOX_TEXT_de								= m
																																			+ "Sie starten Navit zum ersten Mal!\n\n"
																																			+ m
																																			+ "Zum loslegen im Menu \""
																																			+ NAVIT_JAVA_MENU_download_first_map_en
																																			+ "\"\n"
																																			+ m
																																			+ "auswählen und Karte für die\n"
																																			+ m
																																			+ "gewünschte Region downloaden.\n"
																																			+ m
																																			+ "Die Kartendatei ist sehr gross,\n"
																																			+ m
																																			+ "bitte flatrate oder ähnliches aktivieren!\n\n"
																																			+ m
																																			+ "Kartendaten:\n"
																																			+ m
																																			+ "CC-BY-SA OpenStreetMap Project\n\n"
																																			+ m
																																			+ "Für mehr Infos zu Navit\n"
																																			+ m
																																			+ "bitte die Website besuchen\n"
																																			+ m
																																			+ "http://wiki.navit-project.org/\n"
																																			+ "\n"
																																			+ m
																																			+ "      Viel Spaß mit Navit.";
	static final String													INFO_BOX_TEXT_nl								= m

																																			+ "U voert Navit voor de eerste keer uit.\n\n"
																																			+ m
																																			+ "Om te beginnen, selecteer  \n \""

																																			+ NAVIT_JAVA_MENU_download_first_map_nl
																																			+ "\"\n"
																																			+ m

																																			+ "uit het menu en download een kaart\n van je regio.\n"
																																			+ m

																																			+ "De kaarten zijn groot,\n het is dus aangeraden om een \n ongelimiteerde internetverbinding te hebben!\n\n"
																																			+ m
																																			+ "Kaartdata:\n"
																																			+ m
																																			+ "CC-BY-SA OpenStreetMap Project\n\n"
																																			+ m

																																			+ "Voor meer info over Navit\n"
																																			+ m
																																			+ "bezoek onze site\n"
																																			+ m

																																			+ "http://wiki.navit-project.org/\n"
																																			+ "\n"
																																			+ m
																																			+ "      Nog veel plezier met Navit.";

	static final String													NAVIT_JAVA_MENU_MOREINFO_en				= "More info";
	static final String													NAVIT_JAVA_MENU_MOREINFO_fr				= "plus d'infos";
	static final String													NAVIT_JAVA_MENU_MOREINFO_nl				= "meer info";
	static final String													NAVIT_JAVA_MENU_MOREINFO_de				= "Mehr infos";

	static final String													NAVIT_JAVA_MENU_ZOOMIN_en					= "zoom in";
	static final String													NAVIT_JAVA_MENU_ZOOMIN_fr					= "zoom-avant";
	static final String													NAVIT_JAVA_MENU_ZOOMIN_nl					= "inzoomen";
	static final String													NAVIT_JAVA_MENU_ZOOMIN_de					= "zoom in";

	static final String													NAVIT_JAVA_MENU_ZOOMOUT_en					= "zoom out";
	static final String													NAVIT_JAVA_MENU_ZOOMOUT_fr					= "zoom-arrière";
	static final String													NAVIT_JAVA_MENU_ZOOMOUT_nl					= "uitzoomen";
	static final String													NAVIT_JAVA_MENU_ZOOMOUT_de					= "zoom out";

	static final String													NAVIT_JAVA_MENU_EXIT_en						= "Exit Navit";
	static final String													NAVIT_JAVA_MENU_EXIT_fr						= "quittez Navit";
	static final String													NAVIT_JAVA_MENU_EXIT_nl						= "Navit afsluiten";
	static final String													NAVIT_JAVA_MENU_EXIT_de						= "Navit Beenden";

	static final String													NAVIT_JAVA_MENU_TOGGLE_POI_en				= "toggle POI";
	static final String													NAVIT_JAVA_MENU_TOGGLE_POI_fr				= "POI on/off";
	static final String													NAVIT_JAVA_MENU_TOGGLE_POI_nl				= "POI aan/uit";
	static final String													NAVIT_JAVA_MENU_TOGGLE_POI_de				= "POI ein/aus";

	static final String													NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE_en	= "drive here";
	static final String													NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE_fr	= "conduisez";
	static final String													NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE_nl	= "Ga naar hier";
	static final String													NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE_de	= "Ziel setzen";

	static final String													NAVIT_JAVA_MENU_download_second_map_en	= "download 2nd map";
	static final String													NAVIT_JAVA_MENU_download_second_map_fr	= "télécharchez 2ème carte";
	static final String													NAVIT_JAVA_MENU_download_second_map_nl	= "download 2de kaart";
	static final String													NAVIT_JAVA_MENU_download_second_map_de	= "2te karte runterladen";

	// default values
	static String															NAVIT_JAVA_MENU_download_first_map		= NAVIT_JAVA_MENU_download_first_map_en;
	static String															NAVIT_JAVA_MENU_download_second_map		= NAVIT_JAVA_MENU_download_second_map_en;
	static String															INFO_BOX_TITLE									= INFO_BOX_TITLE_en;
	static String															INFO_BOX_TEXT									= INFO_BOX_TEXT_en;
	static String															NAVIT_JAVA_MENU_MOREINFO					= NAVIT_JAVA_MENU_MOREINFO_en;
	static String															NAVIT_JAVA_MENU_ZOOMIN						= NAVIT_JAVA_MENU_ZOOMIN_en;
	static String															NAVIT_JAVA_MENU_ZOOMOUT						= NAVIT_JAVA_MENU_ZOOMOUT_en;
	static String															NAVIT_JAVA_MENU_EXIT							= NAVIT_JAVA_MENU_EXIT_en;
	static String															NAVIT_JAVA_MENU_TOGGLE_POI					= NAVIT_JAVA_MENU_TOGGLE_POI_en;
	static String															NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE		= NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE_en;
	// this part will be removed *******************
	// this part will be removed *******************
	// this part will be removed *******************


	public static void init()
	{
		Log.e("NavitTextTranslations", "initializing translated text ...");
		String k = null;
		String[] v = null;

		k = "exit navit";
		v = new String[]{"en", "Exit Navit", "de", "Navit beenden", "nl", "Navit afsluiten", "fr",
				"Quittez Navit"};
		p(k, v);

		k = "zoom in";
		v = new String[]{"en", "Zoom in", "fr", "Zoom-avant"};
		p(k, v);

		k = "zoom out";
		v = new String[]{"en", "Zoom out", "fr", "Zoom-arrière", "nl", "Zoom uit"};
		p(k, v);

		k = "address search";
		v = new String[]{"en", "Address search", "de", "Adresse suchen", "nl", "Zoek adres", "fr",
				"Cherchez adresse"};
		p(k, v);

		Log.e("NavitTextTranslations", "... ready");
	}

	private static void p(String key, String[] values)
	{
		HashMap<String, String> t = null;
		t = new HashMap<String, String>();
		Log.e("NavitTextTranslations", "trying: " + key);
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

		Log.e("NavitTextTranslations", "lookup L:" + main_language + " T:" + in);
		try
		{
			out = Navit_text_lookup.get(in).get(main_language);
		}
		catch (Exception e)
		{
			// most likely there is not translation yet
			Log.e("NavitTextTranslations", "lookup: exception");
			out = null;
		}

		if (out == null)
		{
			// always return a string for output (use fallback language)
			Log.e("NavitTextTranslations", "using default language");
			try
			{
				out = Navit_text_lookup.get(in).get(fallback_language);
			}
			catch (Exception e)
			{
				Log.e("NavitTextTranslations", "using default language: exception");
				// most likely there is not translation yet
				out = null;
			}
		}

		if (out == null)
		{
			// if we still dont have any text, use the ".mo" file and call the c-function gettext(in)
			out = NavitGraphics.getLocalizedString(in);
			Log.e("NavitTextTranslations", "return the value from gettext() = " + out);
		}
		return out;
	}

}
