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
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import android.os.StatFs;
import android.util.Log;
import android.util.Pair;

public class NavitMapDownloader extends Thread
{
	private static class osm_map_values
	{
		String  lon1;
		String  lat1;
		String  lon2;
		String  lat2;
		String  map_name                     = "";
		long    est_size_bytes               = 0;
		int     level                        = 0;


		public osm_map_values(String mapname, String lon_1, String lat_1, String lon_2, String lat_2,
				long bytes_est, int level)
		{
			this.map_name = mapname;
			this.lon1 = lon_1;
			this.lat1 = lat_1;
			this.lon2 = lon_2;
			this.lat2 = lat_2;
			this.est_size_bytes = bytes_est;
			this.level = level;
		}
	}
	//
	// define the maps here
	//
	private static final osm_map_values[] osm_maps = {
		new osm_map_values("Whole Planet","-180","-90","180","90", 7449814676L, 0),
		new osm_map_values("Africa","-30.89","-36.17","61.68","38.40", 280880799L, 0),
		new osm_map_values("Angola","11.4","-18.1","24.2","-5.3", 55850695L, 1),
		new osm_map_values("Burundi","28.9","-4.5","30.9","-2.2", 60694550L, 1),
		new osm_map_values("Canary Islands","-18.69","26.52","-12.79","29.99", 58904212L, 1),
		new osm_map_values("Democratic Republic of the Congo","11.7","-13.6","31.5","5.7", 70168822L, 1),
		new osm_map_values("Ethiopia","32.89","3.33","48.07","14.97", 68113437L, 1),
		new osm_map_values("Guinea","-15.47","7.12","-7.58","12.74", 57466702L, 1),
		new osm_map_values("Ivory Coast","-8.72","4.09","-2.43","10.80", 62652062L, 1),
		new osm_map_values("Kenya","33.8","-5.2","42.4","4.9", 64420106L, 1),
		new osm_map_values("Lesotho","26.9","-30.7","29.6","-28.4", 55533656L, 1),
		new osm_map_values("Liberia","-15.00","-0.73","-7.20","8.65", 55963569L, 1),
		new osm_map_values("Lybia","9.32","19.40","25.54","33.63", 63127225L, 1),
		new osm_map_values("Madagascar","42.25","-26.63","51.20","-11.31", 56681202L, 1),
		new osm_map_values("Nambia+Botswana","11.4","-29.1","29.5","-16.9", 64357926L, 1),
		new osm_map_values("Reunion","55.2","-21.4","55.9","-20.9", 59487456L, 1),
		new osm_map_values("Rwanda","28.8","-2.9","30.9","-1.0", 60654274L, 1),
		new osm_map_values("South Africa+Lesotho","15.93","-36.36","33.65","-22.08", 79904318L, 1),
		new osm_map_values("Tanzania","29.19","-11.87","40.74","-0.88", 65380211L, 1),
		new osm_map_values("Uganda","29.3","-1.6","35.1","4.3", 61521337L, 1),
		new osm_map_values("Asia","23.8","0.1","195.0","82.4", 1185692230L, 0),
		new osm_map_values("Azerbaijan","44.74","38.34","51.69","42.37", 64563990L, 1),
		new osm_map_values("China","67.3","5.3","135.0","54.5", 354889731L, 1),
		new osm_map_values("Cyprus","32.0","34.5","34.9","35.8", 62657503L, 1),
		new osm_map_values("India+Nepal","67.9","5.5","89.6","36.0", 96930647L, 1),
		new osm_map_values("Indonesia","93.7","-17.3","155.5","7.6", 81860551L, 1),
		new osm_map_values("Iran","43.5","24.4","63.6","40.4", 79782055L, 1),
		new osm_map_values("Iraq","38.7","28.5","49.2","37.4", 64053119L, 1),
		new osm_map_values("Israel","33.99","29.8","35.95","33.4", 73981247L, 1),
		new osm_map_values("Japan+Korea+Taiwan","117.6","20.5","151.3","47.1", 451295345L, 1),
		new osm_map_values("Kazakhstan","46.44","40.89","87.36","55.45", 114949353L, 1),
		new osm_map_values("Kyrgyzstan","69.23","39.13","80.33","43.29", 62425757L, 1),
		new osm_map_values("Malasia+Singapore","94.3","-5.9","108.6","6.8", 61142116L, 1),
		new osm_map_values("Mongolia","87.5","41.4","120.3","52.7", 66228736L, 1),
		new osm_map_values("Nambia+Botswana","11.4","-29.1","29.5","-16.9", 64357926L, 1),
		new osm_map_values("Pakistan","60.83","23.28","77.89","37.15", 74060802L, 1),
		new osm_map_values("Philippines","115.58","4.47","127.85","21.60", 71601316L, 1),
		new osm_map_values("Saudi Arabia","33.2","16.1","55.9","33.5", 100651634L, 1),
		new osm_map_values("Thailand","97.5","5.7","105.2","19.7", 68260330L, 1),
		new osm_map_values("Turkey","25.1","35.8","46.4","42.8", 105200911L, 1),
		new osm_map_values("Turkmenistan","51.78","35.07","66.76","42.91", 62188003L, 1),
		new osm_map_values("UAE+Other","51.5","22.6","56.7","26.5", 61873090L, 1),
		new osm_map_values("Australia+Oceania","89.84","-57.39","179.79","7.26", 185657003L, 0),
		new osm_map_values("Australia","110.5","-44.2","154.9","-9.2", 138812990L, 0),
		new osm_map_values("Tasmania","144.0","-45.1","155.3","-24.8", 109169592L, 1),
		new osm_map_values("Victoria+New South Wales","140.7","-39.4","153.7","-26.9", 104820309L, 1),
		new osm_map_values("New Caledonia","157.85","-25.05","174.15","-16.85", 54512722L, 1),
		new osm_map_values("New Zealand","165.2","-47.6","179.1","-33.7", 68221081L, 1),
		new osm_map_values("Europe","-12.97","33.59","34.15","72.10", 3570653608L, 0),
		new osm_map_values("Western Europe","-17.6","34.5","42.9","70.9", 3700019849L, 1),
		new osm_map_values("Austria","9.4","46.32","17.21","49.1", 289080462L, 1),
		new osm_map_values("Azores","-31.62","36.63","-24.67","40.13", 54507108L, 1),
		new osm_map_values("BeNeLux","2.08","48.87","7.78","54.52", 656716695L, 1),
		new osm_map_values("Denmark","7.65","54.32","15.58","58.07", 154275079L, 1),
		new osm_map_values("Faroe Islands","-7.8","61.3","-6.1","62.5", 54931474L, 1),
		new osm_map_values("France","-5.45","42.00","8.44","51.68", 1468741961L, 1),
		new osm_map_values("Alsace","6.79","47.27","8.48","49.17", 144023488L, 2),
		new osm_map_values("Aquitaine","-2.27","42.44","1.50","45.76", 186786072L, 2),
		new osm_map_values("Auvergne","2.01","44.57","4.54","46.85", 118942252L, 2),
		new osm_map_values("Basse-Normandie","-2.09","48.13","1.03","49.98", 111940365L, 2),
		new osm_map_values("Bourgogne","2.80","46.11","5.58","48.45", 114109115L, 2),
		new osm_map_values("Bretagne","-5.58","46.95","-0.96","48.99", 188689862L, 2),
		new osm_map_values("Centre","0.01","46.29","3.18","48.99", 208870488L, 2),
		new osm_map_values("Champagne-Ardenne","3.34","47.53","5.94","50.28", 112266252L, 2),
		new osm_map_values("Corse","8.12","41.32","9.95","43.28", 67997394L, 2),
		new osm_map_values("Franche-Comte","5.20","46.21","7.83","48.07", 131236689L, 2),
		new osm_map_values("Haute-Normandie","-0.15","48.62","1.85","50.18", 90484736L, 2),
		new osm_map_values("Ile-de-France","1.40","48.07","3.61","49.29", 152890366L, 2),
		new osm_map_values("Languedoc-Roussillon","1.53","42.25","4.89","45.02", 168413195L, 2),
		new osm_map_values("Limousin","0.58","44.87","2.66","46.50", 98422724L, 2),
		new osm_map_values("Lorraine","4.84","47.77","7.72","49.73", 137538540L, 2),
		new osm_map_values("Midi-Pyrenees","-0.37","42.18","3.50","45.10", 186740619L, 2),
		new osm_map_values("Nord-pas-de-Calais","1.42","49.92","4.49","51.31", 145320230L, 2),
		new osm_map_values("Pays-de-la-Loire","-2.88","46.20","0.97","48.62", 243736184L, 2),
		new osm_map_values("Picardie","1.25","48.79","4.31","50.43", 163238861L, 2),
		new osm_map_values("Poitou-Charentes","-1.69","45.04","1.26","47.23", 197886714L, 2),
		new osm_map_values("Provence-Alpes-Cote-d-Azur","4.21","42.91","7.99","45.18", 179863755L, 2),
		new osm_map_values("Rhone-Alpes","3.65","44.07","7.88","46.64", 201452039L, 2),
		new osm_map_values("Germany","5.18","46.84","15.47","55.64", 1187298374L, 1),
		new osm_map_values("Baden-Wuerttemberg","7.32","47.14","10.57","49.85", 247149038L, 2),
		new osm_map_values("Bayern","8.92","47.22","13.90","50.62", 306577202L, 2),
		new osm_map_values("Mittelfranken","9.86","48.78","11.65","49.84", 95916401L, 2),
		new osm_map_values("Niederbayern","11.55","47.75","14.12","49.42", 119427776L, 2),
		new osm_map_values("Oberbayern","10.67","47.05","13.57","49.14", 147630851L, 2),
		new osm_map_values("Oberfranken","10.31","49.54","12.49","50.95", 104963024L, 2),
		new osm_map_values("Oberpfalz","11.14","48.71","13.47","50.43", 112413336L, 2),
		new osm_map_values("Schwaben","9.27","47.10","11.36","49.09", 126836560L, 2),
		new osm_map_values("Unterfranken","8.59","49.16","10.93","50.67", 124601596L, 2),
		new osm_map_values("Berlin","13.03","52.28","13.81","52.73", 78189548L, 2),
		new osm_map_values("Brandenburg","11.17","51.30","14.83","53.63", 126821283L, 2),
		new osm_map_values("Bremen","8.43","52.96","9.04","53.66", 69427370L, 2),
		new osm_map_values("Hamburg","9.56","53.34","10.39","53.80", 76388380L, 2),
		new osm_map_values("Hessen","7.72","49.34","10.29","51.71", 155980870L, 2),
		new osm_map_values("Mecklenburg-Vorpommern","10.54","53.05","14.48","55.05", 92107050L, 2),
		new osm_map_values("Niedersachsen","6.40","51.24","11.69","54.22", 288712601L, 2),
		new osm_map_values("Nordrhein-westfalen","5.46","50.26","9.52","52.59", 335383638L, 2),
		new osm_map_values("Rheinland-Pfalz","6.06","48.91","8.56","51.00", 157909942L, 2),
		new osm_map_values("Saarland","6.30","49.06","7.46","49.69", 78579241L, 2),
		new osm_map_values("Sachsen-Anhalt","10.50","50.88","13.26","53.11", 115314663L, 2),
		new osm_map_values("Sachsen","11.82","50.11","15.10","51.73", 134182818L, 2),
		new osm_map_values("Schleswig-Holstein","7.41","53.30","11.98","55.20", 114865543L, 2),
		new osm_map_values("Thueringen","9.81","50.15","12.72","51.70", 112896293L, 2),
		new osm_map_values("Germany+Austria+Switzerland","3.4","44.5","18.6","55.1", 1763000779L, 1),
		new osm_map_values("Iceland","-25.3","62.8","-11.4","67.5", 58803839L, 1),
		new osm_map_values("Ireland","-11.17","51.25","-5.23","55.9", 74456575L, 1),
		new osm_map_values("Italy","6.52","36.38","18.96","47.19", 373215809L, 1),
		new osm_map_values("Spain+Portugal","-11.04","34.87","4.62","44.41", 354839261L, 1),
		new osm_map_values("Mallorca","2.2","38.8","4.7","40.2", 66781797L, 2),
		new osm_map_values("Galicia","-10.0","41.7","-6.3","44.1", 69081612L, 2),
		new osm_map_values("Scandinavia","4.0","54.4","32.1","71.5", 386082513L, 1),
		new osm_map_values("Finland","18.6","59.2","32.3","70.3", 167464389L, 1),
		new osm_map_values("Denmark","7.49","54.33","13.05","57.88", 142017133L, 1),
		new osm_map_values("Switzerland","5.79","45.74","10.59","47.84", 197612725L, 1),
		new osm_map_values("UK","-9.7","49.6","2.2","61.2", 308044592L, 1),
		new osm_map_values("England","-7.80","48.93","2.41","56.14", 331085897L, 1),
		new osm_map_values("Buckinghamshire","-1.19","51.44","-0.43","52.25", 74619627L, 2),
		new osm_map_values("Cambridgeshire","-0.55","51.96","0.56","52.79", 71849188L, 2),
		new osm_map_values("Cumbria","-3.96","53.85","-2.11","55.24", 71699620L, 2),
		new osm_map_values("East_yorkshire_with_hull","-1.16","53.50","0.54","54.26", 68241870L, 2),
		new osm_map_values("Essex","-0.07","51.40","1.36","52.14", 82991499L, 2),
		new osm_map_values("Herefordshire","-3.19","51.78","-2.29","52.45", 66471962L, 2),
		new osm_map_values("Kent","-0.02","50.81","1.65","51.53", 75449128L, 2),
		new osm_map_values("Lancashire","-3.20","53.43","-2.00","54.29", 75096621L, 2),
		new osm_map_values("Leicestershire","-1.65","52.34","-0.61","53.03", 75492394L, 2),
		new osm_map_values("Norfolk","0.10","52.30","2.04","53.41", 71556838L, 2),
		new osm_map_values("Nottinghamshire","-1.39","52.73","-0.62","53.55", 72979826L, 2),
		new osm_map_values("Oxfordshire","-1.77","51.41","-0.82","52.22", 73351886L, 2),
		new osm_map_values("Shropshire","-3.29","52.26","-2.18","53.05", 69144272L, 2),
		new osm_map_values("Somerset","-3.89","50.77","-2.20","51.40", 72098176L, 2),
		new osm_map_values("South_yorkshire","-1.88","53.25","-0.80","53.71", 72594920L, 2),
		new osm_map_values("Suffolk","0.29","51.88","1.81","52.60", 72985880L, 2),
		new osm_map_values("Surrey","-0.90","51.02","0.10","51.52", 79850137L, 2),
		new osm_map_values("Wiltshire","-2.41","50.90","-1.44","51.76", 71244578L, 2),
		new osm_map_values("Scotland","-8.13","54.49","-0.15","61.40", 102111248L, 2),
		new osm_map_values("Wales","-5.56","51.28","-2.60","53.60", 84860075L, 2),
		new osm_map_values("Albania","19.09","39.55","21.12","42.72", 71097966L, 1),
		new osm_map_values("Belarus","23.12","51.21","32.87","56.23", 100471644L, 1),
		new osm_map_values("Russia","27.9","41.5","190.4", "77.6", 508559360L, 1),
		new osm_map_values("Bulgaria","24.7","42.1","24.8","42.1", 62211433L, 1),
		new osm_map_values("Bosnia-Herzegovina","15.69","42.52","19.67","45.32", 75756822L, 1),
		new osm_map_values("Czech Republic","11.91","48.48","19.02","51.17", 288911729L, 1),
		new osm_map_values("Croatia","13.4","42.1","19.4","46.9", 118479986L, 1),
		new osm_map_values("Estonia","21.5","57.5","28.2","59.6", 86149958L, 1),
		new osm_map_values("Greece","28.9","37.8","29.0","37.8", 59191120L, 1),
		new osm_map_values("Crete","23.3","34.5","26.8","36.0", 61121443L, 1),
		new osm_map_values("Hungary","16.08","45.57","23.03","48.39", 129046944L, 1),
		new osm_map_values("Latvia","20.7","55.6","28.3","58.1", 81683354L, 1),
		new osm_map_values("Lithuania","20.9","53.8","26.9","56.5", 77228922L, 1),
		new osm_map_values("Poland","13.6","48.8","24.5","55.0", 331299544L, 1),
		new osm_map_values("Romania","20.3","43.5","29.9","48.4", 150011857L, 1),
		new osm_map_values("North America","-178.1","6.5","-10.4","84.0", 2738147321L, 0),
		new osm_map_values("Alaska","-179.5","49.5","-129","71.6", 72413728L, 1),
		new osm_map_values("Canada","-141.3","41.5","-52.2","70.2", 1125713287L, 1),
		new osm_map_values("Hawaii","-161.07","18.49","-154.45","22.85", 57463829L, 1),
		new osm_map_values("USA (except Alaska and Hawaii)","-125.4","24.3","-66.5","49.3", 2356238167L, 1),
		new osm_map_values("Midwest","-104.11","35.92","-80.46","49.46", 663062321L, 2),
		new osm_map_values("Michigan","-90.47","41.64","-79.00","49.37", 207416918L, 2),
		new osm_map_values("Ohio","-84.87","38.05","-79.85","43.53", 143571732L, 2),
		new osm_map_values("Northeast","-80.58","38.72","-66.83","47.53", 517925445L, 2),
		new osm_map_values("Massachusetts","-73.56","40.78","-68.67","42.94", 159493455L, 2),
		new osm_map_values("Vermont","-73.49","42.68","-71.41","45.07", 74308439L, 2),
		new osm_map_values("Pacific","-180.05","15.87","-129.75","73.04", 78496182L, 2),
		new osm_map_values("South","-106.70","23.98","-71.46","40.70", 1135650708L, 2),
		new osm_map_values("Arkansas","-94.67","32.95","-89.59","36.60", 89637645L, 2),
		new osm_map_values("District of Columbia","-77.17","38.74","-76.86","39.05", 64042148L, 2),
		new osm_map_values("Florida","-88.75","23.63","-77.67","31.05", 118647388L, 2),
		new osm_map_values("Louisiana","-94.09","28.09","-88.62","33.07", 136435773L, 2),
		new osm_map_values("Maryland","-79.54","37.83","-74.99","40.22", 134152161L, 2),
		new osm_map_values("Mississippi","-91.71","29.99","-88.04","35.05", 100291749L, 2),
		new osm_map_values("Oklahoma","-103.41","33.56","-94.38","37.38", 106601625L, 2),
		new osm_map_values("Texas","-106.96","25.62","-92.97","36.58", 220587321L, 2),
		new osm_map_values("Virginia","-83.73","36.49","-74.25","39.52", 218627122L, 2),
		new osm_map_values("West Virginia","-82.70","37.15","-77.66","40.97", 133830267L, 2),
		new osm_map_values("West","-133.11","31.28","-101.99","49.51", 616041200L, 2),
		new osm_map_values("Arizona","-114.88","30.01","-108.99","37.06", 89434673L, 2),
		new osm_map_values("California","-125.94","32.43","-114.08","42.07", 303663259L, 2),
		new osm_map_values("Colorado","-109.11","36.52","-100.41","41.05", 132835514L, 2),
		new osm_map_values("Idaho","-117.30","41.93","-110.99","49.18", 97305030L, 2),
		new osm_map_values("Montana","-116.10","44.31","-102.64","49.74", 93935496L, 2),
		new osm_map_values("New Mexico","-109.10","26.98","-96.07","37.05", 185648327L, 2),
		new osm_map_values("Nevada","-120.2","35.0","-113.8","42.1", 138055868L, 2),
		new osm_map_values("Oregon","-124.8","41.8","-116.3","46.3", 103551459L, 2),
		new osm_map_values("Utah","-114.11","36.95","-108.99","42.05", 78249845L, 2),
		new osm_map_values("Washington State","-125.0","45.5","-116.9","49.0", 100601625L, 2),
		new osm_map_values("South+Middle America","-83.5","-56.3","-30.8","13.7", 204217202L, 0),
		new osm_map_values("Argentina","-73.9","-57.3","-51.6","-21.0", 105910515L, 1),
		new osm_map_values("Argentina+Chile","-77.2","-56.3","-52.7","-16.1", 111585063L, 1),
		new osm_map_values("Bolivia","-70.5","-23.1","-57.3","-9.3", 59215113L, 1),
		new osm_map_values("Brazil","-71.4","-34.7","-32.8","5.4", 127279780L, 1),
		new osm_map_values("Chile","-81.77","-58.50","-65.46","-17.41", 84808355L, 1),
		new osm_map_values("Cuba","-85.3","19.6","-74.0","23.6", 57704852L, 1),
		new osm_map_values("Colombia","-79.1","-4.0","-66.7","12.6", 85701114L, 1),
		new osm_map_values("Ecuador","-82.6","-5.4","-74.4","2.3", 63453353L, 1),
		new osm_map_values("Guyana+Suriname+Guyane Francaise","-62.0","1.0","-51.2","8.9", 57226004L, 1),
		new osm_map_values("Haiti+Republica Dominicana","-74.8","17.3","-68.2","20.1", 63826780L, 1),
		new osm_map_values("Jamaica","-78.6","17.4","-75.9","18.9", 53888545L, 1),
		new osm_map_values("Mexico","-117.6","14.1","-86.4","32.8", 258877491L, 1),
		new osm_map_values("Paraguay","-63.8","-28.1","-53.6","-18.8", 60539032L, 1),
		new osm_map_values("Peru","-82.4","-18.1","-67.5","0.4", 71286591L, 1),
		new osm_map_values("Uruguay","-59.2","-36.5","-51.7","-29.7", 64850903L, 1),
		new osm_map_values("Venezuela","-73.6","0.4","-59.7","12.8", 74521456L, 1)
	};

	private static Pair<List<HashMap<String, String>>, ArrayList<ArrayList<HashMap<String, String>>> >
	                                    MAP_MENU                                = null;

	private Boolean                     stop_me                                 = false;
	private static final int            SOCKET_CONNECT_TIMEOUT                  = 60000;			// 60 secs.
	private static final int            SOCKET_READ_TIMEOUT                     = 120000;			// 120 secs.
	private static final int            MAP_WRITE_FILE_BUFFER                   = 1024 * 64;
	private static final int            MAP_WRITE_MEM_BUFFER                    = 1024 * 64;
	private static final int            MAP_READ_FILE_BUFFER                    = 1024 * 64;
	private static final int            UPDATE_PROGRESS_EVERY_CYCLE             = 8;
	private static final int            MAX_RETRIES                             = 5;
	private static final String         MAP_FILENAME_PRI                        = "navitmap.bin";
	private static final String         MAP_FILENAME_NUM                        = "navitmap_%03d.bin";
	private static final String         MAP_FILENAME_PATH                       = Navit.MAP_FILENAME_PATH;
	private static final String         TAG                                     = "NavitMapDownloader";
	private static final String         MAP_BULLETPOINT                        = " * ";
	
	private osm_map_values              map_values;
	private int                         map_slot;
	private int                         dialog_num;
	
	public static final int             EXIT_SUCCESS                            = 0;
	public static final int             EXIT_RECOVERABLE_ERROR                  = 1;
	public static final int             EXIT_UNRECOVERABLE_ERROR                = 2;
	
	public static int                   retry_counter                           = 0;

	public void run()
	{
		stop_me = false;
		int exit_code;
		retry_counter = 0;

		Log.v(TAG, "map_num3=" + this.map_slot);

		NavitDialogs.sendDialogMessage( NavitDialogs.MSG_PROGRESS_BAR
				, Navit.get_text("Mapdownload"), Navit.get_text("downloading") + ": " + map_values.map_name
				, NavitDialogs.DIALOG_MAPDOWNLOAD, 20 , 0);

		do
		{
			try
			{
				Thread.sleep(10 + retry_counter * 1000);
			} catch (InterruptedException e1)	{}
		} while ( ( exit_code = download_osm_map(map_values, map_slot)) == EXIT_RECOVERABLE_ERROR
				&& retry_counter++ < MAX_RETRIES
				&& !stop_me);

		if (exit_code == EXIT_SUCCESS)
		{
			NavitDialogs.sendDialogMessage( NavitDialogs.MSG_TOAST
					, null, map_values.map_name + " " + Navit.get_text("ready")
					, dialog_num , 0 , 0);

			Log.d(TAG, "success");
		}
		
		if (exit_code == EXIT_SUCCESS || stop_me )
		{
			NavitDialogs.sendDialogMessage( NavitDialogs.MSG_REMOVE_PROGRESS_BAR, null, null, dialog_num 
						, exit_code , 0 );
		}
	}

	public void stop_thread()
	{
		stop_me = true;
		Log.d(TAG, "stop_me -> true");
	}

	public NavitMapDownloader(int map_id, int dialog_num, int map_slot)
	{
		this.map_values = osm_maps[map_id];
		this.map_slot = map_slot;
	}
	
	public static Pair<List<HashMap<String, String>>, ArrayList<ArrayList<HashMap<String, String>>> > getMenu() {
		
		if (MAP_MENU != null)
		{
			return MAP_MENU;
		}
		ArrayList<HashMap<String, String>> resultGroups = new ArrayList<HashMap<String, String>>();
		ArrayList<ArrayList<HashMap<String, String>>> resultChilds = new ArrayList<ArrayList<HashMap<String, String>>>();
		ArrayList<HashMap<String, String>> secList = new ArrayList<HashMap<String, String>>();

		for (int currentMapIndex = 0; currentMapIndex < osm_maps.length; currentMapIndex++)
		{
			if (osm_maps[currentMapIndex].level == 0)
			{
				if (secList.size() > 0)
				{
					resultChilds.add(secList);
				}
				secList = new ArrayList<HashMap<String, String>>();
				HashMap<String, String> m = new HashMap<String, String>();
				m.put( "map_name", osm_maps[currentMapIndex].map_name);
				resultGroups.add( m );
			}

			HashMap<String, String> child = new HashMap<String, String>();
			child.put("map_name", (osm_maps[currentMapIndex].level > 1 ? MAP_BULLETPOINT : "")
					+ osm_maps[currentMapIndex].map_name
					+ " "
					+ (osm_maps[currentMapIndex].est_size_bytes / 1024 / 1024) + "MB");
			child.put("map_index", String.valueOf(currentMapIndex));

			secList.add(child);
		}
		resultChilds.add(secList);
		
		MAP_MENU = new Pair<List<HashMap<String, String>>, ArrayList<ArrayList<HashMap<String, String>>> >(resultGroups, resultChilds);
		return MAP_MENU;
	}

	public int download_osm_map(osm_map_values map_values, int map_number)
	{
		int exit_code = EXIT_SUCCESS;
		boolean resume = false;
		HttpURLConnection c = null;
		BufferedOutputStream buf = null;
		BufferedInputStream bif = null;
		File outputFile = null;
		long already_read = 0;
		long real_size_bytes = 0;

		String fileName = map_values.map_name + ".tmp";

		try
		{
			outputFile = new File(MAP_FILENAME_PATH, fileName);

			long old_download_size = outputFile.length();
			long start_timestamp = System.nanoTime();

			outputFile.mkdir();
			URL url = null;
			if (old_download_size > 0)
			{
				try
				{
					ObjectInputStream infoStream = new ObjectInputStream(new FileInputStream(MAP_FILENAME_PATH + fileName + ".info"));
					String resume_proto = infoStream.readUTF();
					infoStream.readUTF(); // read the host name (unused for now)
					String resume_file = infoStream.readUTF();
					infoStream.close();
					// looks like the same file, try to resume
					Log.v(TAG, "Try to resume download");
					resume = true;
					url = new URL(resume_proto + "://" + "maps.navit-project.org" + resume_file);
				} catch (Exception e) {
					File file = new File(MAP_FILENAME_PATH + fileName + ".info");
					file.delete();
				}
			}
				
			if (url == null)
			{
				url = new URL("http://maps.navit-project.org/api/map/?bbox=" + map_values.lon1 + ","
						+ map_values.lat1 + "," + map_values.lon2 + "," + map_values.lat2);
			}
			
			Log.v(TAG, "connect to " + url.toString());
//			URL url = new URL("http://192.168.2.101:8080/zweibruecken.bin");
			c = (HttpURLConnection) url.openConnection();
			c.setRequestMethod("GET");
			c.setDoOutput(true);
			c.setReadTimeout(SOCKET_READ_TIMEOUT);
			c.setConnectTimeout(SOCKET_CONNECT_TIMEOUT);

			if ( resume )
			{
				c.setRequestProperty("Range", "bytes=" + old_download_size + "-");
				already_read = old_download_size;
			}

			real_size_bytes = c.getContentLength();
			long fileTime = c.getLastModified();
			Log.d(TAG, "size: " + real_size_bytes 
					+ ", read: " + already_read
					+ ", timestamp: " + fileTime);
			

			if (!resume)
			{
				outputFile.delete();
				old_download_size = 0;
				File infoFile = new File(MAP_FILENAME_PATH, fileName + ".info");
				ObjectOutputStream infoStream = new ObjectOutputStream(new FileOutputStream(infoFile));
				infoStream.writeUTF(c.getURL().getProtocol());
				infoStream.writeUTF(c.getURL().getHost());
				infoStream.writeUTF(c.getURL().getFile());
				infoStream.close();
			}
			
			Log.v(TAG, "Connection ref: " + c.getURL());
			if ( real_size_bytes <= 0)
				real_size_bytes = map_values.est_size_bytes;
			
			if (!checkFreeSpace(real_size_bytes - already_read))
			{
				return EXIT_UNRECOVERABLE_ERROR;
			}
			
			Log.d(TAG, "real size in bytes: " + real_size_bytes);

			buf = new BufferedOutputStream(new FileOutputStream(outputFile, resume) , MAP_WRITE_FILE_BUFFER);
			bif = new BufferedInputStream(c.getInputStream(), MAP_READ_FILE_BUFFER);

			byte[] buffer = new byte[MAP_WRITE_MEM_BUFFER];
			int len1 = 0;
			int alt_cur = 0;
			String eta_string = "";
			String info;
			float per_second_overall;
			long bytes_remaining = 0;
			int eta_seconds = 0;
			while (!stop_me && (len1 = bif.read(buffer)) != -1)
			{
				already_read += len1;
				if (alt_cur++ % UPDATE_PROGRESS_EVERY_CYCLE == 0)
				{

					if (already_read > real_size_bytes)
					{
						real_size_bytes = already_read;
					}
					
					bytes_remaining = real_size_bytes - already_read;
					per_second_overall = (already_read - old_download_size) / ((System.nanoTime() - start_timestamp) / 1000000000f);
					eta_seconds = (int) (bytes_remaining / per_second_overall);
					if (eta_seconds > 60)
					{
						eta_string = (int) (eta_seconds / 60f) + " m";
					}
					else
					{
						eta_string = eta_seconds + " s";
					}
					info = String.format("%s: %s\n %dMb / %dMb\n %.1f kb/s %s: %s"
							, Navit.get_text("downloading")
							, map_values.map_name
							, already_read / 1024 / 1024
							, real_size_bytes / 1024 / 1024
							, per_second_overall / 1024f
							, Navit.get_text("ETA")
							, eta_string);
					
					if (retry_counter > 0)
					{
						info += "\n Retry " + retry_counter + "/" + MAX_RETRIES;
					}
					Log.e(TAG, "info: " + info);

					NavitDialogs.sendDialogMessage( NavitDialogs.MSG_PROGRESS_BAR
							, Navit.get_text("Mapdownload"), info
							, dialog_num, (int) (real_size_bytes / 1024), (int) (already_read / 1024));
				}
				buf.write(buffer, 0, len1);
			}

			Log.d(TAG, "Connectionerror: " + c.getResponseCode ());

			if (stop_me)
			{
				NavitDialogs.sendDialogMessage( NavitDialogs.MSG_TOAST
								, null, Navit.get_text("Map download aborted!")
								, dialog_num , 0 , 0);
						
				exit_code = EXIT_UNRECOVERABLE_ERROR;
			}
			else if ( already_read < real_size_bytes )
			{
				Log.d(TAG, "Server send only " + already_read + " bytes of " + real_size_bytes);
				exit_code = EXIT_RECOVERABLE_ERROR;
			}
			else
			{
				exit_code = EXIT_SUCCESS;
			}
		}
		catch (IOException e)
		{
			Log.d(TAG, "Error: " + e);
			
			if ( !checkFreeSpace(real_size_bytes - already_read))
			{
				exit_code = EXIT_UNRECOVERABLE_ERROR;
			}
			else
			{
				NavitDialogs.sendDialogMessage( NavitDialogs.MSG_PROGRESS_BAR
						, Navit.get_text("Mapdownload")
						, Navit.get_text("Error downloading map!")
						, dialog_num, (int) (real_size_bytes / 1024), (int) (already_read / 1024));
				exit_code = EXIT_RECOVERABLE_ERROR;
			}
		}
		catch (Exception e)
		{
			NavitDialogs.sendDialogMessage( NavitDialogs.MSG_PROGRESS_BAR
					, Navit.get_text("Mapdownload")
					, Navit.get_text("Error downloading map!")
					, dialog_num, (int) (real_size_bytes / 1024), (int) (already_read / 1024));
			Log.d(TAG, "gerneral Error: " + e);
			exit_code = EXIT_RECOVERABLE_ERROR;
		}

		// always cleanup, as we might get errors when trying to resume
		if (buf!=null && bif!=null)
		{
			try {
				buf.flush();
				buf.close();
	
				bif.close();
			} catch (IOException e) { }
		}

		if (exit_code == EXIT_SUCCESS)
		{
			String final_fileName = MAP_FILENAME_PRI;
			
			if (map_number>0)
			{
				final_fileName = String.format(MAP_FILENAME_NUM, map_number);
			}

			File final_outputFile = new File(MAP_FILENAME_PATH, final_fileName);
			// delete an already final filename, first
			final_outputFile.delete();
			// rename file to final name
			outputFile.renameTo(final_outputFile);
		}
		
		return exit_code;
	}
	
	private boolean checkFreeSpace(long needed_bytes)
	{
		StatFs fsInfo = new StatFs(MAP_FILENAME_PATH);
		
		long free_space = (long)fsInfo.getAvailableBlocks() * fsInfo.getBlockSize();
		
		if ( needed_bytes <= 0 )
			needed_bytes = MAP_WRITE_FILE_BUFFER;

		if (free_space < needed_bytes )
		{
			Log.e(TAG, "Not enough free space. Please free at least " + needed_bytes / 1024 /1024 + "Mb.");
			
			NavitDialogs.sendDialogMessage( NavitDialogs.MSG_PROGRESS_BAR
					, Navit.get_text("Mapdownload")
					, Navit.get_text("Error downloading map!") + "\n" + Navit.get_text("Not enough free space")
					, dialog_num, (int)(needed_bytes / 1024), (int)(free_space / 1024));
			
			return false;
		}
		return true;
	}
}
