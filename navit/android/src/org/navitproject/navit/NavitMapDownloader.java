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

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.text.DecimalFormat;
import java.text.NumberFormat;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class NavitMapDownloader
{
	public static class osm_map_values
	{
		String	lon1;
		String	lat1;
		String	lon2;
		String	lat2;
		String	map_name								= "";
		long		est_size_bytes						= 0;
		String	est_size_bytes_human_string	= "";
		String	text_for_select_list				= "";

		public osm_map_values(String mapname, String lon_1, String lat_1, String lon_2, String lat_2,
				long bytes_est)
		{
			this.map_name = mapname;
			this.lon1 = lon_1;
			this.lat1 = lat_1;
			this.lon2 = lon_2;
			this.lat2 = lat_2;
			this.est_size_bytes = bytes_est;
			this.est_size_bytes_human_string = " ca. "
					+ (int) ((float) (this.est_size_bytes) / 1024f / 1024f) + "MB";
			this.text_for_select_list = this.map_name + " " + this.est_size_bytes_human_string;
		}
	}

	/*
	 * coords for predefined maps:
	 * 
	 * <a onclick='predefined(5.18,46.84,15.47,55.64)'>Germany</a>
	 * <a onclick='predefined(2.08,48.87,7.78,54.52)'>BeNeLux</a>
	 * <a onclick='predefined(9.4,46.32,17.21,49.1)'>Austria</a> 209 MBytes = 219152384 Bytes
	 */
	//
	// define the maps here
	//                                                          name     , lon1 , lat1   , lon2   , lat2, est. size in bytes
	/*
	 * public static osm_map_values austria = new osm_map_values("Austria",
	 * "9.4", "46.32", "17.21",
	 * "49.1", 222000000);
	 * public static osm_map_values benelux = new osm_map_values("BeNeLux",
	 * "2.08", "48.87", "7.78",
	 * "54.52", 530000000);
	 * public static osm_map_values germany = new osm_map_values("Germany",
	 * "5.18", "46.84", "15.47",
	 * "55.64", 943000000);
	 */


	public static osm_map_values		Whole_Planet									= new osm_map_values(
																											"Whole Planet",
																											"-180", "-90",
																											"180", "90",
																											5985878379L);
	public static osm_map_values		Africa											= new osm_map_values(
																											"Africa", "-20.8",
																											"-35.2", "52.5",
																											"37.4", 180836389L);
	public static osm_map_values		Angola											= new osm_map_values(
																											"Angola", "11.4",
																											"-18.1", "24.2",
																											"-5.3", 56041641L);
	public static osm_map_values		Burundi											= new osm_map_values(
																											"Burundi", "28.9",
																											"-4.5", "30.9",
																											"-2.2", 56512924L);
	public static osm_map_values		Democratic_Republic_of_the_Congo			= new osm_map_values(
																											"Democratic Republic of the Congo",
																											"11.7", "-13.6",
																											"31.5", "5.7",
																											65026791L);
	public static osm_map_values		Kenya												= new osm_map_values(
																											"Kenya", "33.8",
																											"-5.2", "42.4",
																											"4.9", 58545273L);
	public static osm_map_values		Lesotho											= new osm_map_values(
																											"Lesotho", "26.9",
																											"-30.7", "29.6",
																											"-28.4", 54791041L);
	public static osm_map_values		Madagascar										= new osm_map_values(
																											"Madagascar",
																											"43.0", "-25.8",
																											"50.8", "-11.8",
																											56801099L);
	public static osm_map_values		Nambia_Botswana								= new osm_map_values(
																											"Nambia+Botswana",
																											"11.4", "-29.1",
																											"29.5", "-16.9",
																											61807049L);
	public static osm_map_values		Reunion											= new osm_map_values(
																											"Reunion", "55.2",
																											"-21.4", "55.9",
																											"-20.9", 58537419L);
	public static osm_map_values		Rwanda											= new osm_map_values(
																											"Rwanda", "28.8",
																											"-2.9", "30.9",
																											"-1.0", 56313710L);
	public static osm_map_values		South_Africa									= new osm_map_values(
																											"South Africa",
																											"15.6", "-35.2",
																											"33.3", "-21.9",
																											73545245L);
	public static osm_map_values		Uganda											= new osm_map_values(
																											"Uganda", "29.3",
																											"-1.6", "35.1",
																											"4.3", 57376589L);
	public static osm_map_values		Asia												= new osm_map_values(
																											"Asia", "23.8",
																											"0.1", "195.0",
																											"82.4", 797725952L);
	public static osm_map_values		China												= new osm_map_values(
																											"China", "67.3",
																											"5.3", "135.0",
																											"54.5", 259945160L);
	public static osm_map_values		Cyprus											= new osm_map_values(
																											"Cyprus", "32.0",
																											"34.5", "34.9",
																											"35.8", 58585278L);
	public static osm_map_values		India_Nepal										= new osm_map_values(
																											"India+Nepal",
																											"67.9", "5.5",
																											"89.6", "36.0",
																											82819344L);
	public static osm_map_values		Indonesia										= new osm_map_values(
																											"Indonesia",
																											"93.7", "-17.3",
																											"155.5", "7.6",
																											74648081L);
	public static osm_map_values		Iran												= new osm_map_values(
																											"Iran", "43.5",
																											"24.4", "63.6",
																											"40.4", 69561312L);
	public static osm_map_values		Iraq												= new osm_map_values(
																											"Iraq", "38.7",
																											"28.5", "49.2",
																											"37.4", 59146383L);
	public static osm_map_values		Israel											= new osm_map_values(
																											"Israel", "33.99",
																											"29.8", "35.95",
																											"33.4", 65065351L);
	public static osm_map_values		Japan_Korea_Taiwan							= new osm_map_values(
																											"Japan+Korea+Taiwan",
																											"117.6", "20.5",
																											"151.3", "47.1",
																											305538751L);
	public static osm_map_values		Malasia_Singapore								= new osm_map_values(
																											"Malasia+Singapore",
																											"94.3", "-5.9",
																											"108.6", "6.8",
																											58849792L);
	public static osm_map_values		Mongolia											= new osm_map_values(
																											"Mongolia",
																											"87.5", "41.4",
																											"120.3", "52.7",
																											60871187L);
	public static osm_map_values		Thailand											= new osm_map_values(
																											"Thailand",
																											"97.5", "5.7",
																											"105.2", "19.7",
																											62422864L);
	public static osm_map_values		Turkey											= new osm_map_values(
																											"Turkey", "25.1",
																											"35.8", "46.4",
																											"42.8", 81758047L);
	public static osm_map_values		UAE_Other										= new osm_map_values(
																											"UAE+Other",
																											"51.5", "22.6",
																											"56.7", "26.5",
																											57419510L);
	public static osm_map_values		Australia										= new osm_map_values(
																											"Australia",
																											"110.5", "-44.2",
																											"154.9", "-9.2",
																											128502185L);
	public static osm_map_values		Tasmania											= new osm_map_values(
																											"Tasmania",
																											"144.0", "-45.1",
																											"155.3", "-24.8",
																											103573989L);
	public static osm_map_values		Victoria_New_South_Wales					= new osm_map_values(
																											"Victoria+New South Wales",
																											"140.7", "-39.4",
																											"153.7", "-26.9",
																											99307594L);
	public static osm_map_values		New_Zealand										= new osm_map_values(
																											"New Zealand",
																											"165.2", "-47.6",
																											"179.1", "-33.7",
																											64757454L);
	public static osm_map_values		Europe											= new osm_map_values(
																											"Europe",
																											"-12.97", "33.59",
																											"34.15", "72.10",
																											2753910015L);
	public static osm_map_values		Western_Europe									= new osm_map_values(
																											"Western Europe",
																											"-17.6", "34.5",
																											"42.9", "70.9",
																											2832986851L);
	public static osm_map_values		Austria											= new osm_map_values(
																											"Austria", "9.4",
																											"46.32", "17.21",
																											"49.1", 222359992L);
	public static osm_map_values		BeNeLux											= new osm_map_values(
																											"BeNeLux", "2.08",
																											"48.87", "7.78",
																											"54.52",
																											533865194L);
	public static osm_map_values		Faroe_Islands									= new osm_map_values(
																											"Faroe Islands",
																											"-7.8", "61.3",
																											"-6.1", "62.5",
																											54526101L);
	public static osm_map_values		France											= new osm_map_values(
																											"France", "-5.45",
																											"42.00", "8.44",
																											"51.68",
																											1112047845L);
	public static osm_map_values		Germany											= new osm_map_values(
																											"Germany", "5.18",
																											"46.84", "15.47",
																											"55.64",
																											944716238L);
	public static osm_map_values		Bavaria											= new osm_map_values(
																											"Bavaria", "10.3",
																											"47.8", "13.6",
																											"49.7", 131799419L);
	public static osm_map_values		Saxonia											= new osm_map_values(
																											"Saxonia", "11.8",
																											"50.1", "15.0",
																											"51.7", 112073909L);
	public static osm_map_values		Germany_Austria_Switzerland				= new osm_map_values(
																											"Germany+Austria+Switzerland",
																											"3.4", "44.5",
																											"18.6", "55.1",
																											1385785353L);
	public static osm_map_values		Iceland											= new osm_map_values(
																											"Iceland",
																											"-25.3", "62.8",
																											"-11.4", "67.5",
																											57281405L);
	public static osm_map_values		Ireland											= new osm_map_values(
																											"Ireland",
																											"-11.17", "51.25",
																											"-5.23", "55.9",
																											70186936L);
	public static osm_map_values		Italy												= new osm_map_values(
																											"Italy", "6.52",
																											"36.38", "18.96",
																											"47.19",
																											291401314L);
	public static osm_map_values		Spain_Portugal									= new osm_map_values(
																											"Spain+Portugal",
																											"-11.04", "34.87",
																											"4.62", "44.41",
																											292407746L);
	public static osm_map_values		Mallorca											= new osm_map_values(
																											"Mallorca", "2.2",
																											"38.8", "4.7",
																											"40.2", 59700600L);
	public static osm_map_values		Galicia											= new osm_map_values(
																											"Galicia",
																											"-10.0", "41.7",
																											"-6.3", "44.1",
																											64605237L);
	public static osm_map_values		Scandinavia										= new osm_map_values(
																											"Scandinavia",
																											"4.0", "54.4",
																											"32.1", "71.5",
																											299021928L);
	public static osm_map_values		Finland											= new osm_map_values(
																											"Finland", "18.6",
																											"59.2", "32.3",
																											"70.3", 128871467L);
	public static osm_map_values		Denmark											= new osm_map_values(
																											"Denmark", "7.49",
																											"54.33", "13.05",
																											"57.88",
																											120025875L);
	public static osm_map_values		Switzerland										= new osm_map_values(
																											"Switzerland",
																											"5.79", "45.74",
																											"10.59", "47.84",
																											162616817L);
	public static osm_map_values		UK													= new osm_map_values(
																											"UK", "-9.7",
																											"49.6", "2.2",
																											"61.2", 245161510L);
	public static osm_map_values		Bulgaria											= new osm_map_values(
																											"Bulgaria",
																											"24.7", "42.1",
																											"24.8", "42.1",
																											56607427L);
	public static osm_map_values		Czech_Republic									= new osm_map_values(
																											"Czech Republic",
																											"11.91", "48.48",
																											"19.02", "51.17",
																											234138824L);
	public static osm_map_values		Croatia											= new osm_map_values(
																											"Croatia", "13.4",
																											"42.1", "19.4",
																											"46.9", 99183280L);
	public static osm_map_values		Estonia											= new osm_map_values(
																											"Estonia", "21.5",
																											"57.5", "28.2",
																											"59.6", 79276178L);
	public static osm_map_values		Greece											= new osm_map_values(
																											"Greece", "28.9",
																											"37.8", "29.0",
																											"37.8", 55486527L);
	public static osm_map_values		Crete												= new osm_map_values(
																											"Crete", "23.3",
																											"34.5", "26.8",
																											"36.0", 57032630L);
	public static osm_map_values		Hungary											= new osm_map_values(
																											"Hungary",
																											"16.08", "45.57",
																											"23.03", "48.39",
																											109831319L);
	public static osm_map_values		Latvia											= new osm_map_values(
																											"Latvia", "20.7",
																											"55.6", "28.3",
																											"58.1", 71490706L);
	public static osm_map_values		Lithuania										= new osm_map_values(
																											"Lithuania",
																											"20.9", "53.8",
																											"26.9", "56.5",
																											67992457L);
	public static osm_map_values		Poland											= new osm_map_values(
																											"Poland", "13.6",
																											"48.8", "24.5",
																											"55.0", 266136768L);
	public static osm_map_values		Romania											= new osm_map_values(
																											"Romania", "20.3",
																											"43.5", "29.9",
																											"48.4", 134525863L);
	public static osm_map_values		North_America									= new osm_map_values(
																											"North America",
																											"-178.1", "6.5",
																											"-10.4", "84.0",
																											2477309662L);
	public static osm_map_values		Alaska											= new osm_map_values(
																											"Alaska",
																											"-179.5", "49.5",
																											"-129", "71.6",
																											72320027L);
	public static osm_map_values		Canada											= new osm_map_values(
																											"Canada",
																											"-141.3", "41.5",
																											"-52.2", "70.2",
																											937813467L);
	public static osm_map_values		Hawaii											= new osm_map_values(
																											"Hawaii",
																											"-161.07",
																											"18.49",
																											"-154.45",
																											"22.85", 57311788L);
	public static osm_map_values		USA__except_Alaska_and_Hawaii_			= new osm_map_values(
																											"USA (except Alaska and Hawaii)",
																											"-125.4", "24.3",
																											"-66.5", "49.3",
																											2216912004L);
	public static osm_map_values		Nevada											= new osm_map_values(
																											"Nevada",
																											"-120.2", "35.0",
																											"-113.8", "42.1",
																											136754975L);
	public static osm_map_values		Oregon											= new osm_map_values(
																											"Oregon",
																											"-124.8", "41.8",
																											"-116.3", "46.3",
																											101627308L);
	public static osm_map_values		Washington_State								= new osm_map_values(
																											"Washington State",
																											"-125.0", "45.5",
																											"-116.9", "49.0",
																											98178877L);
	public static osm_map_values		South_Middle_America							= new osm_map_values(
																											"South+Middle America",
																											"-83.5", "-56.3",
																											"-30.8", "13.7",
																											159615197L);
	public static osm_map_values		Argentina										= new osm_map_values(
																											"Argentina",
																											"-73.9", "-57.3",
																											"-51.6", "-21.0",
																											87516152L);
	public static osm_map_values		Argentina_Chile								= new osm_map_values(
																											"Argentina+Chile",
																											"-77.2", "-56.3",
																											"-52.7", "-16.1",
																											91976696L);
	public static osm_map_values		Bolivia											= new osm_map_values(
																											"Bolivia",
																											"-70.5", "-23.1",
																											"-57.3", "-9.3",
																											58242168L);
	public static osm_map_values		Brazil											= new osm_map_values(
																											"Brazil", "-71.4",
																											"-34.7", "-32.8",
																											"5.4", 105527899L);
	public static osm_map_values		Cuba												= new osm_map_values(
																											"Cuba", "-85.3",
																											"19.6", "-74.0",
																											"23.6", 56608942L);
	public static osm_map_values		Colombia											= new osm_map_values(
																											"Colombia",
																											"-79.1", "-4.0",
																											"-66.7", "12.6",
																											78658454L);
	public static osm_map_values		Ecuador											= new osm_map_values(
																											"Ecuador",
																											"-82.6", "-5.4",
																											"-74.4", "2.3",
																											61501914L);
	public static osm_map_values		Guyana_Suriname_Guyane_Francaise			= new osm_map_values(
																											"Guyana+Suriname+Guyane Francaise",
																											"-62.0", "1.0",
																											"-51.2", "8.9",
																											57040689L);
	public static osm_map_values		Haiti_Republica_Dominicana					= new osm_map_values(
																											"Haiti+Republica Dominicana",
																											"-74.8", "17.3",
																											"-68.2", "20.1",
																											63528584L);
	public static osm_map_values		Jamaica											= new osm_map_values(
																											"Jamaica",
																											"-78.6", "17.4",
																											"-75.9", "18.9",
																											53958307L);
	public static osm_map_values		Mexico											= new osm_map_values(
																											"Mexico",
																											"-117.6", "14.1",
																											"-86.4", "32.8",
																											251108617L);
	public static osm_map_values		Paraguay											= new osm_map_values(
																											"Paraguay",
																											"-63.8", "-28.1",
																											"-53.6", "-18.8",
																											57188715L);
	public static osm_map_values		Peru												= new osm_map_values(
																											"Peru", "-82.4",
																											"-18.1", "-67.5",
																											"0.4", 65421441L);
	public static osm_map_values		Uruguay											= new osm_map_values(
																											"Uruguay",
																											"-59.2", "-36.5",
																											"-51.7", "-29.7",
																											63542225L);
	public static osm_map_values		Venezuela										= new osm_map_values(
																											"Venezuela",
																											"-73.6", "0.4",
																											"-59.7", "12.8",
																											64838882L);

	public static osm_map_values[]	OSM_MAPS											= new osm_map_values[]{
			Whole_Planet, Africa, Angola, Burundi, Democratic_Republic_of_the_Congo, Kenya, Lesotho,
			Madagascar, Nambia_Botswana, Reunion, Rwanda, South_Africa, Uganda, Asia, China, Cyprus,
			India_Nepal, Indonesia, Iran, Iraq, Israel, Japan_Korea_Taiwan, Malasia_Singapore,
			Mongolia, Nambia_Botswana, Thailand, Turkey, UAE_Other, Australia, Tasmania,
			Victoria_New_South_Wales, New_Zealand, Europe, Western_Europe, Austria, BeNeLux,
			Faroe_Islands, France, Germany, Bavaria, Saxonia, Germany_Austria_Switzerland, Iceland,
			Ireland, Italy, Spain_Portugal, Mallorca, Galicia, Scandinavia, Finland, Denmark,
			Switzerland, UK, Bulgaria, Czech_Republic, Croatia, Estonia, Greece, Crete, Hungary,
			Latvia, Lithuania, Poland, Romania, North_America, Alaska, Canada, Hawaii,
			USA__except_Alaska_and_Hawaii_, Nevada, Oregon, Washington_State, South_Middle_America,
			Argentina, Argentina_Chile, Bolivia, Brazil, Cuba, Colombia, Ecuador,
			Guyana_Suriname_Guyane_Francaise, Haiti_Republica_Dominicana, Jamaica, Mexico, Paraguay,
			Peru, Uruguay, Venezuela														};
	public static String[]				OSM_MAP_NAME_LIST								= new String[]{

			Whole_Planet.map_name, Africa.map_name, Angola.map_name, Burundi.map_name,
			Democratic_Republic_of_the_Congo.map_name, Kenya.map_name, Lesotho.map_name,
			Madagascar.map_name, Nambia_Botswana.map_name, Reunion.map_name, Rwanda.map_name,
			South_Africa.map_name, Uganda.map_name, Asia.map_name, China.map_name, Cyprus.map_name,
			India_Nepal.map_name, Indonesia.map_name, Iran.map_name, Iraq.map_name, Israel.map_name,
			Japan_Korea_Taiwan.map_name, Malasia_Singapore.map_name, Mongolia.map_name,
			Nambia_Botswana.map_name, Thailand.map_name, Turkey.map_name, UAE_Other.map_name,
			Australia.map_name, Tasmania.map_name, Victoria_New_South_Wales.map_name,
			New_Zealand.map_name, Europe.map_name, Western_Europe.map_name, Austria.map_name,
			BeNeLux.map_name, Faroe_Islands.map_name, France.map_name, Germany.map_name,
			Bavaria.map_name, Saxonia.map_name, Germany_Austria_Switzerland.map_name,
			Iceland.map_name, Ireland.map_name, Italy.map_name, Spain_Portugal.map_name,
			Mallorca.map_name, Galicia.map_name, Scandinavia.map_name, Finland.map_name,
			Denmark.map_name, Switzerland.map_name, UK.map_name, Bulgaria.map_name,
			Czech_Republic.map_name, Croatia.map_name, Estonia.map_name, Greece.map_name,
			Crete.map_name, Hungary.map_name, Latvia.map_name, Lithuania.map_name, Poland.map_name,
			Romania.map_name, North_America.map_name, Alaska.map_name, Canada.map_name,
			Hawaii.map_name, USA__except_Alaska_and_Hawaii_.map_name, Nevada.map_name,
			Oregon.map_name, Washington_State.map_name, South_Middle_America.map_name,
			Argentina.map_name, Argentina_Chile.map_name, Bolivia.map_name, Brazil.map_name,
			Cuba.map_name, Colombia.map_name, Ecuador.map_name,
			Guyana_Suriname_Guyane_Francaise.map_name, Haiti_Republica_Dominicana.map_name,
			Jamaica.map_name, Mexico.map_name, Paraguay.map_name, Peru.map_name, Uruguay.map_name,
			Venezuela.map_name																};

	public static String[]				OSM_MAP_NAME_LIST_inkl_SIZE_ESTIMATE	= new String[]{
			Whole_Planet.text_for_select_list, Africa.text_for_select_list,
			Angola.text_for_select_list, Burundi.text_for_select_list,
			Democratic_Republic_of_the_Congo.text_for_select_list, Kenya.text_for_select_list,
			Lesotho.text_for_select_list, Madagascar.text_for_select_list,
			Nambia_Botswana.text_for_select_list, Reunion.text_for_select_list,
			Rwanda.text_for_select_list, South_Africa.text_for_select_list,
			Uganda.text_for_select_list, Asia.text_for_select_list, China.text_for_select_list,
			Cyprus.text_for_select_list, India_Nepal.text_for_select_list,
			Indonesia.text_for_select_list, Iran.text_for_select_list, Iraq.text_for_select_list,
			Israel.text_for_select_list, Japan_Korea_Taiwan.text_for_select_list,
			Malasia_Singapore.text_for_select_list, Mongolia.text_for_select_list,
			Nambia_Botswana.text_for_select_list, Thailand.text_for_select_list,
			Turkey.text_for_select_list, UAE_Other.text_for_select_list,
			Australia.text_for_select_list, Tasmania.text_for_select_list,
			Victoria_New_South_Wales.text_for_select_list, New_Zealand.text_for_select_list,
			Europe.text_for_select_list, Western_Europe.text_for_select_list,
			Austria.text_for_select_list, BeNeLux.text_for_select_list,
			Faroe_Islands.text_for_select_list, France.text_for_select_list,
			Germany.text_for_select_list, Bavaria.text_for_select_list, Saxonia.text_for_select_list,
			Germany_Austria_Switzerland.text_for_select_list, Iceland.text_for_select_list,
			Ireland.text_for_select_list, Italy.text_for_select_list,
			Spain_Portugal.text_for_select_list, Mallorca.text_for_select_list,
			Galicia.text_for_select_list, Scandinavia.text_for_select_list,
			Finland.text_for_select_list, Denmark.text_for_select_list,
			Switzerland.text_for_select_list, UK.text_for_select_list, Bulgaria.text_for_select_list,
			Czech_Republic.text_for_select_list, Croatia.text_for_select_list,
			Estonia.text_for_select_list, Greece.text_for_select_list, Crete.text_for_select_list,
			Hungary.text_for_select_list, Latvia.text_for_select_list, Lithuania.text_for_select_list,
			Poland.text_for_select_list, Romania.text_for_select_list,
			North_America.text_for_select_list, Alaska.text_for_select_list,
			Canada.text_for_select_list, Hawaii.text_for_select_list,
			USA__except_Alaska_and_Hawaii_.text_for_select_list, Nevada.text_for_select_list,
			Oregon.text_for_select_list, Washington_State.text_for_select_list,
			South_Middle_America.text_for_select_list, Argentina.text_for_select_list,
			Argentina_Chile.text_for_select_list, Bolivia.text_for_select_list,
			Brazil.text_for_select_list, Cuba.text_for_select_list, Colombia.text_for_select_list,
			Ecuador.text_for_select_list, Guyana_Suriname_Guyane_Francaise.text_for_select_list,
			Haiti_Republica_Dominicana.text_for_select_list, Jamaica.text_for_select_list,
			Mexico.text_for_select_list, Paraguay.text_for_select_list, Peru.text_for_select_list,
			Uruguay.text_for_select_list, Venezuela.text_for_select_list		};

	public Boolean							stop_me											= false;
	static final int						SOCKET_CONNECT_TIMEOUT						= 6000;
	static final int						SOCKET_READ_TIMEOUT							= 6000;
	static final int						MAP_WRITE_FILE_BUFFER						= 1024 * 64;
	static final int						MAP_WRITE_MEM_BUFFER							= 1024 * 64;
	static final int						MAP_READ_FILE_BUFFER							= 1024 * 64;
	static final int						UPDATE_PROGRESS_EVERY_CYCLE				= 8;

	static final String					DOWNLOAD_FILENAME								= "navitmap.tmp";
	static final String					MAP_FILENAME_PRI								= "navitmap.bin";
	static final String					MAP_FILENAME_SEC								= "navitmap_002.bin";
	static final String					MAP_FILENAME_PATH								= Navit.MAP_FILENAME_PATH;


	public class ProgressThread extends Thread
	{
		Handler			mHandler;
		osm_map_values	map_values;
		int				map_num;
		int				my_dialog_num;

		ProgressThread(Handler h, osm_map_values map_values, int map_num2)
		{
			this.mHandler = h;
			this.map_values = map_values;
			this.map_num = map_num2;
			if (this.map_num == Navit.MAP_NUM_PRIMARY)
			{
				this.my_dialog_num = Navit.MAPDOWNLOAD_PRI_DIALOG;
			}
			else if (this.map_num == Navit.MAP_NUM_SECONDARY)
			{
				this.my_dialog_num = Navit.MAPDOWNLOAD_SEC_DIALOG;
			}
		}

		public void run()
		{
			stop_me = false;
			int exit_code = download_osm_map(mHandler, map_values, this.map_num);

			// ok, remove dialog
			Message msg = mHandler.obtainMessage();
			Bundle b = new Bundle();
			msg.what = 0;
			b.putInt("dialog_num", this.my_dialog_num);
			b.putInt("exit_code", exit_code);
			msg.setData(b);
			mHandler.sendMessage(msg);
		}

		public void stop_thread()
		{
			stop_me = true;
			Log.d("NavitMapDownloader", "stop_me -> true");
		}
	}

	public Navit	navit_jmain	= null;

	public NavitMapDownloader(Navit main)
	{
		this.navit_jmain = main;
	}

	public int download_osm_map(Handler handler, osm_map_values map_values, int map_num3)
	{
		int exit_code = 1;

		//Log.v("NavitMapDownloader", "map_num3=" + map_num3);
		int my_dialog_num = 0;
		if (map_num3 == Navit.MAP_NUM_PRIMARY)
		{
			my_dialog_num = Navit.MAPDOWNLOAD_PRI_DIALOG;
			//Log.v("NavitMapDownloader", "PRI");
		}
		else if (map_num3 == Navit.MAP_NUM_SECONDARY)
		{
			my_dialog_num = Navit.MAPDOWNLOAD_SEC_DIALOG;
			//Log.v("NavitMapDownloader", "SEC");
		}
		//Log.v("NavitMapDownloader", "map_num3=" + map_num3);

		Message msg = handler.obtainMessage();
		Bundle b = new Bundle();
		msg.what = 1;
		b.putInt("max", 20); // use a dummy number here
		b.putInt("cur", 0);
		b.putInt("dialog_num", my_dialog_num);
		b.putString("title", "Mapdownload");
		b.putString("text", "downloading: " + map_values.map_name);
		msg.setData(b);
		handler.sendMessage(msg);
		try
		{
			// little pause here
			Thread.sleep(10);
		}
		catch (InterruptedException e1)
		{
		}

		// output filename
		String PATH = MAP_FILENAME_PATH;
		String fileName = DOWNLOAD_FILENAME;
		String final_fileName = "xxx";
		//Log.v("NavitMapDownloader", "map_num3=" + map_num3);
		if (map_num3 == Navit.MAP_NUM_SECONDARY)
		{
			final_fileName = this.MAP_FILENAME_SEC;
		}
		else if (map_num3 == Navit.MAP_NUM_PRIMARY)
		{
			final_fileName = this.MAP_FILENAME_PRI;
		}
		// output path for output filename
		// String PATH = Environment.getExternalStorageDirectory() + "/download/";

		try
		{
			URL url = new URL("http://maps.navit-project.org/api/map/?bbox=" + map_values.lon1 + ","
					+ map_values.lat1 + "," + map_values.lon2 + "," + map_values.lat2);
			HttpURLConnection c = (HttpURLConnection) url.openConnection();
			c.setRequestMethod("GET");
			c.setDoOutput(true);
			c.setReadTimeout(SOCKET_READ_TIMEOUT);
			c.setConnectTimeout(SOCKET_CONNECT_TIMEOUT);
			long real_size_bytes = c.getContentLength();
			c.connect();

			Log.d("NavitMapDownloader", "real size in bytes: " + real_size_bytes);
			if (real_size_bytes > 20)
			{
				// change the estimated filesize to reported filesize
				map_values.est_size_bytes = real_size_bytes;
			}
			Log.d("NavitMapDownloader", "size in bytes: " + map_values.est_size_bytes);

			File file = new File(PATH);
			File outputFile = new File(file, fileName);
			File final_outputFile = new File(file, final_fileName);
			// tests have shown that deleting the file first is sometimes faster -> so we delete it (who knows)
			outputFile.delete();
			// seems this command overwrites the output file anyway
			FileOutputStream fos = new FileOutputStream(outputFile);
			BufferedOutputStream buf = new BufferedOutputStream(fos, MAP_WRITE_FILE_BUFFER); // buffer

			InputStream is = c.getInputStream();
			BufferedInputStream bif = new BufferedInputStream(is, MAP_READ_FILE_BUFFER); // buffer

			byte[] buffer = new byte[MAP_WRITE_MEM_BUFFER]; // buffer
			int len1 = 0;
			int already_read = 0;
			int alt = UPDATE_PROGRESS_EVERY_CYCLE; // show progress about every xx cylces
			int alt_cur = 0;
			String kbytes_per_second = "";
			long start_timestamp = System.currentTimeMillis();
			NumberFormat formatter = new DecimalFormat("00000.0");
			String eta_string = "";
			float per_second_overall = 0f;
			long bytes_remaining = 0;
			int eta_seconds = 0;
			while ((len1 = bif.read(buffer)) != -1)
			{
				if (stop_me)
				{
					// ok we need to be stopped! close all files and end
					buf.flush();
					buf.close();
					fos.close();
					bif.close();
					is.close();
					c.disconnect();
					return 2;
				}
				already_read = already_read + len1;
				alt_cur++;
				if (alt_cur > alt)
				{
					alt_cur = 0;

					msg = handler.obtainMessage();
					b = new Bundle();
					msg.what = 1;
					b.putInt("max", (int) (map_values.est_size_bytes / 1024));
					b.putInt("cur", (int) (already_read / 1024));
					b.putInt("dialog_num", my_dialog_num);
					b.putString("title", "Map download");
					per_second_overall = (float) already_read
							/ (float) ((System.currentTimeMillis() - start_timestamp) / 1000);
					kbytes_per_second = formatter.format((per_second_overall / 1024f));
					bytes_remaining = map_values.est_size_bytes - already_read;
					eta_seconds = (int) ((float) bytes_remaining / (float) per_second_overall);
					if (eta_seconds > 60)
					{
						eta_string = (int) (eta_seconds / 60f) + " m";
					}
					else
					{
						eta_string = eta_seconds + " s";
					}
					b.putString("text", "downloading: " + map_values.map_name + "\n" + " "
							+ (int) (already_read / 1024f / 1024f) + "Mb / "
							+ (int) (map_values.est_size_bytes / 1024f / 1024f) + "Mb" + "\n" + " "
							+ kbytes_per_second + "kb/s" + " ETA: " + eta_string + "\n" +
							"Map data CC-BY-SA by OpenStreetMap");
					msg.setData(b);
					handler.sendMessage(msg);
					//					try
					//					{
					//						// little pause here
					//						Thread.sleep(20);
					//					}
					//					catch (InterruptedException e1)
					//					{
					//					}
				}
				buf.write(buffer, 0, len1);
			}
			buf.flush();

			buf.close();
			fos.close();

			bif.close();
			is.close();

			c.disconnect();

			// delete an already final filename, first
			final_outputFile.delete();
			// rename file to final name
			outputFile.renameTo(final_outputFile);
		}
		catch (IOException e)
		{
			msg = handler.obtainMessage();
			b = new Bundle();
			msg.what = 2;
			b.putInt("dialog_num", my_dialog_num);
			b.putString("text", "Error downloading map!");
			msg.setData(b);
			handler.sendMessage(msg);

			Log.d("NavitMapDownloader", "Error: " + e);
			exit_code = 3;
		}
		catch (Exception e)
		{
			msg = handler.obtainMessage();
			b = new Bundle();
			msg.what = 2;
			b.putInt("dialog_num", my_dialog_num);
			b.putString("text", "Error downloading map!");
			msg.setData(b);
			handler.sendMessage(msg);

			Log.d("NavitMapDownloader", "gerneral Error: " + e);
			exit_code = 4;
		}

		msg = handler.obtainMessage();
		b = new Bundle();
		msg.what = 1;
		b.putInt("max", (int) (map_values.est_size_bytes / 1024));
		b.putInt("cur", (int) (map_values.est_size_bytes / 1024));
		b.putInt("dialog_num", my_dialog_num);
		b.putString("title", "Mapdownload");
		b.putString("text", map_values.map_name + " ready");
		msg.setData(b);
		handler.sendMessage(msg);


		Log.d("NavitMapDownloader", "success");
		exit_code = 0;
		return exit_code;
	}
}
