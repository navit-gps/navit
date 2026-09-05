/*
 * Navit, a modular navigation system. Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if
 * not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

package org.navitproject.navit;

import static org.navitproject.navit.NavitAppConfig.getTstring;

import android.location.Location;
import android.os.Bundle;
import android.os.Message;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;
import java.lang.String;
import java.io.InputStreamReader;
import java.io.BufferedReader;



/*
 * @author hoehnp
 * @author rikky
 *
 */
public class NavitMapDownloader extends Thread {

    // removed since not available in github-actions-mapserver
    static int [] removed = new int[]{R.string.korea, R.string.uae_other, R.string.tasmania, R.string.victoria, R.string.new_south_wales, R.string.new_caledonia,
            R.string.mittelfranken, R.string.oberfranken, R.string.unterfranken, R.string.oberbayern, R.string.niederbayern, R.string.oberpfalz, R.string.schwaben,
            R.string.mallorca, R.string.galicia, R.string.wiltshire, R.string.surrey, R.string.suffolk, R.string.south_yorkshire, R.string.somerset,
            R.string.shropshire, R.string.oxfordshire, R.string.nottinghamshire, R.string.norfolk, R.string.leicestershire, R.string.lancashire,
            R.string.kent, R.string.herefordshire, R.string.essex, R.string.east_yorkshire_with_hull, R.string.cumbria, R.string.cambridgeshire, R.string.buckinghamshire,
            R.string.crete, R.string.midwest, R.string.pacific, R.string.south, R.string.west, R.string.guyana
    };
    
    static String [] africa = new String[]{"africa-algeria", "africa-angola", "africa-benin", "africa-botswana", "africa-burkina-faso", "africa-burundi", "africa-cameroon", "africa-canary-islands", "africa-cape-verde", "africa-central-african-republic", "africa-chad", "africa-comores", "africa-congo-brazzaville", "africa-congo-democratic-republic", "africa-djibouti", "africa-egypt", "africa-equatorial-guinea", "africa-eritrea", "africa-ethiopia", "africa-gabon", "africa-ghana", "africa-guinea", "africa-guinea-bissau", "africa-ivory-coast", "africa-kenya", "africa-lesotho", "africa-liberia", "africa-libya", "africa-madagascar", "africa-malawi", "africa-mali", "africa-mauritania", "africa-mauritius", "africa-morocco", "africa-mozambique", "africa-namibia", "africa-niger", "africa-nigeria", "africa-rwanda", "africa-saint-helena-ascension-and-tristan-da-cunha", "africa-sao-tome-and-principe", "africa-senegal-and-gambia", "africa-seychelles", "africa-sierra-leone", "africa-somalia", "africa-south-africa", "africa-south-sudan", "africa-sudan", "africa-swaziland", "africa-tanzania", "africa-togo", "africa-tunisia", "africa-uganda", "africa-zambia", "africa-zimbabwe"};

    static String[] asia = new String[]{"asia-afghanistan", "asia-armenia", "asia-azerbaijan", "asia-bangladesh", "asia-bhutan", "asia-cambodia", "asia-china", "asia-gcc-states", "asia-india", "asia-indonesia", "asia-iran", "asia-iraq", "asia-israel-and-palestine", "asia-japan", "asia-jordan", "asia-kazakhstan", "asia-kyrgyzstan", "asia-laos", "asia-lebanon", "asia-malaysia-singapore-brunei", "asia-maldives", "asia-mongolia", "asia-myanmar", "asia-nepal", "asia-north-korea", "asia-pakistan", "asia-philippines", "asia-south-korea", "asia-sri-lanka", "asia-syria", "asia-taiwan", "asia-tajikistan", "asia-thailand", "asia-turkmenistan", "asia-uzbekistan", "asia-vietnam", "asia-yemen"};

    static String [] australia_oceania = new String[]{"australia-oceania-american-oceania", "australia-oceania-australia", "australia-oceania-cook-islands", "australia-oceania-fiji", "australia-oceania-ile-de-clipperton", "australia-oceania-kiribati", "australia-oceania-marshall-islands", "australia-oceania-micronesia", "australia-oceania-nauru", "australia-oceania-new-caledonia", "australia-oceania-new-zealand", "australia-oceania-niue", "australia-oceania-palau", "australia-oceania-papua-new-guinea", "australia-oceania-pitcairn-islands", "australia-oceania-polynesie-francaise", "australia-oceania-samoa", "australia-oceania-solomon-islands", "australia-oceania-tokelau", "australia-oceania-tonga", "australia-oceania-tuvalu", "australia-oceania-vanuatu", "australia-oceania-wallis-et-futuna"};



    static String[] europe = new String[]{"europe-albania", "europe-andorra", "europe-austria", "europe-azores", "europe-belarus", "europe-belgium", "europe-bosnia_herzegovina", "europe-bulgaria", "europe-croatia", "europe-cyprus", "europe-czech_republic", "europe-denmark", "europe-estonia", "europe-faroe_islands", "europe-finland", "europe-france-alsace", "europe-france-aquitaine", "europe-france-auvergne", "europe-france-basse-normandie", "europe-france-bourgogne", "europe-france-bretagne", "europe-france-centre", "europe-france-champagne-ardenne", "europe-france-corse", "europe-france-franche-comte", "europe-france-guadeloupe", "europe-france-guyane", "europe-france-haute-normandie", "europe-france-ile-de-france", "europe-france-languedoc-roussillon", "europe-france-limousin", "europe-france-lorraine", "europe-france-martinique", "europe-france-mayotte", "europe-france-midi-pyrenees", "europe-france-nord-pas-de-calais", "europe-france-pays-de-la-loire", "europe-france-picardie", "europe-france-poitou-charentes", "europe-france-provence-alpes-cote-d-azur", "europe-france-reunion", "europe-france-rhone-alpes", "europe-georgia", "europe-germany-baden-wuerttemberg", "europe-germany-bayern", "europe-germany-berlin", "europe-germany-brandenburg", "europe-germany-bremen", "europe-germany-hamburg", "europe-germany-hessen", "europe-germany-mecklenburg-vorpommern", "europe-germany-niedersachsen", "europe-germany-nordrhein-westfalen", "europe-germany-rheinland-pfalz", "europe-germany-saarland", "europe-germany-sachsen-anhalt", "europe-germany-sachsen", "europe-germany-schleswig-holstein", "europe-germany-thueringen", "europe-greece", "europe-guernsey_jersey", "europe-hungary", "europe-iceland", "europe-ireland_and_northern_ireland", "europe-isle_of_man", "europe-italy-centro", "europe-italy-isole", "europe-italy-nord-est", "europe-italy-nord-ovest", "europe-italy-sud", "europe-kosovo", "europe-latvia", "europe-liechtenstein", "europe-lithuania", "europe-luxembourg", "europe-macedonia", "europe-malta", "europe-moldova", "europe-monaco", "europe-montenegro", "europe-netherlands-drenthe", "europe-netherlands-flevoland", "europe-netherlands-friesland", "europe-netherlands-gelderland", "europe-netherlands-groningen", "europe-netherlands-limburg", "europe-netherlands-noord-brabant", "europe-netherlands-noord-holland", "europe-netherlands-overijssel", "europe-netherlands-utrecht", "europe-netherlands-zeeland", "europe-netherlands-zuid-holland", "europe-norway", "europe-poland-dolnoslaskie", "europe-poland-kujawsko-pomorskie", "europe-poland-lodzkie", "europe-poland-lubelskie", "europe-poland-lubuskie", "europe-poland-malopolskie", "europe-poland-mazowieckie", "europe-poland-opolskie", "europe-poland-podkarpackie", "europe-poland-podlaskie", "europe-poland-pomorskie", "europe-poland-slaskie", "europe-poland-swietokrzyskie", "europe-poland-warminsko-mazurskie", "europe-poland-wielkopolskie", "europe-poland-zachodniopomorskie", "europe-portugal", "europe-romania", "europe-serbia", "europe-slovakia", "europe-slovenia", "europe-spain", "europe-sweden", "europe-switzerland", "europe-turkey", "europe-united-kingdom-england", "europe-united-kingdom-scotland", "europe-united-kingdom-wales", "europe-ukraine"};

    static String[] scandinavia = new String[]{"europe-sweden", "europe-danmark", "europe-norway"};

    static String[] north_america = new String[]{"north-america-canada-alberta", "north-america-canada-british-columbia", "north-america-canada-manitoba", "north-america-canada-new-brunswick", "north-america-canada-newfoundland-and-labrador", "north-america-canada-northwest-territories", "north-america-canada-nova-scotia", "north-america-canada-nunavut", "north-america-canada-ontario", "north-america-canada-prince-edward-island", "north-america-canada-quebec", "north-america-canada-saskatchewan", "north-america-canada-yukon", "north-america-greenland", "north-america-mexico", "north-america-us-alabama", "north-america-us-alaska", "north-america-us-arizona", "north-america-us-arkansas", "north-america-us-california", "north-america-us-colorado", "north-america-us-connecticut", "north-america-us-delaware", "north-america-us-district-of-columbia", "north-america-us-florida", "north-america-us-georgia", "north-america-us-hawaii", "north-america-us-idaho", "north-america-us-illinois", "north-america-us-indiana", "north-america-us-iowa", "north-america-us-kansas", "north-america-us-kentucky", "north-america-us-louisiana", "north-america-us-maine", "north-america-us-maryland", "north-america-us-massachusetts", "north-america-us-michigan", "north-america-us-minnesota", "north-america-us-mississippi", "north-america-us-missouri", "north-america-us-montana", "north-america-us-nebraska", "north-america-us-nevada", "north-america-us-new-hampshire", "north-america-us-new-jersey", "north-america-us-new-mexico", "north-america-us-new-york", "north-america-us-north-carolina", "north-america-us-north-dakota", "north-america-us-ohio", "north-america-us-oklahoma", "north-america-us-oregon", "north-america-us-pennsylvania", "north-america-us-puerto-rico", "north-america-us-rhode-island", "north-america-us-south-carolina", "north-america-us-south-dakota", "north-america-us-tennessee", "north-america-us-texas", "north-america-us-utah", "north-america-us-vermont", "north-america-us-virginia", "north-america-us-washington", "north-america-us-west-virginia", "north-america-us-wisconsin", "north-america-us-wyoming"};

    static String[] south_middle_america = new String[]{"south-america-argentina", "south-america-bolivia", "south-america-brazil-centro_oeste", "south-america-brazil-nordeste", "south-america-brazil-norte", "south-america-brazil-sudeste", "south-america-brazil-sul", "south-america-chile", "south-america-colombia", "south-america-ecuador", "south-america-paraguay", "south-america-peru", "south-america-suriname", "south-america-uruguay", "south-america-venezuela", "central-america-bahamas", "central-america-belize", "central-america-costa_rica", "central-america-cuba", "central-america-el-salvador", "central-america-guatemala", "central-america-haiti-and-domrep", "central-america-honduras", "central-america-jamaica", "central-america-nicaragua"};

    static String[] france = new String[]{"europe-france-alsace", "europe-france-aquitaine", "europe-france-auvergne", "europe-france-basse-normandie", "europe-france-bourgogne", "europe-france-bretagne", "europe-france-centre", "europe-france-champagne-ardenne", "europe-france-corse", "europe-france-franche-comte", "europe-france-guadeloupe", "europe-france-guyane", "europe-france-haute-normandie", "europe-france-ile-de-france", "europe-france-languedoc-roussillon", "europe-france-limousin", "europe-france-lorraine", "europe-france-martinique", "europe-france-mayotte", "europe-france-midi-pyrenees", "europe-france-nord-pas-de-calais", "europe-france-pays-de-la-loire", "europe-france-picardie", "europe-france-poitou-charentes", "europe-france-provence-alpes-cote-d-azur", "europe-france-reunion", "europe-france-rhone-alpes"};

    static String[] germany = new String[]{"europe-germany-baden-wuerttemberg", "europe-germany-bayern", "europe-germany-berlin", "europe-germany-brandenburg", "europe-germany-bremen", "europe-germany-hamburg", "europe-germany-hessen", "europe-germany-mecklenburg-vorpommern", "europe-germany-niedersachsen", "europe-germany-nordrhein-westfalen", "europe-germany-rheinland-pfalz", "europe-germany-saarland", "europe-germany-sachsen-anhalt", "europe-germany-sachsen", "europe-germany-schleswig-holstein", "europe-germany-thueringen"};

    static String[] united_kingdom = new String[]{"europe-united-kingdom-england", "europe-united-kingdom-scotland", "europe-united-kingdom-wales"};

    static String[] italy = new String[]{"europe-italy-centro", "europe-italy-isole", "europe-italy-nord-est", "europe-italy-nord-ovest", "europe-italy-sud"};

    static String[] netherlands = new String[]{"europe-netherlands-drenthe", "europe-netherlands-flevoland", "europe-netherlands-friesland", "europe-netherlands-gelderland", "europe-netherlands-groningen", "europe-netherlands-limburg", "europe-netherlands-noord-brabant", "europe-netherlands-noord-holland", "europe-netherlands-overijssel", "europe-netherlands-utrecht", "europe-netherlands-zeeland", "europe-netherlands-zuid-holland"};

    static String[] poland = new String[]{"europe-poland-dolnoslaskie", "europe-poland-kujawsko-pomorskie", "europe-poland-lodzkie", "europe-poland-lubelskie", "europe-poland-lubuskie", "europe-poland-malopolskie", "europe-poland-mazowieckie", "europe-poland-opolskie", "europe-poland-podkarpackie", "europe-poland-podlaskie", "europe-poland-pomorskie", "europe-poland-slaskie", "europe-poland-swietokrzyskie", "europe-poland-warminsko-mazurskie", "europe-poland-wielkopolskie", "europe-poland-zachodniopomorskie"};

    static String [] canada = new String[]{"north-america-canada-alberta", "north-america-canada-british-columbia", "north-america-canada-manitoba", "north-america-canada-new-brunswick", "north-america-canada-newfoundland-and-labrador", "north-america-canada-northwest-territories", "north-america-canada-nova-scotia", "north-america-canada-nunavut", "north-america-canada-ontario", "north-america-canada-prince-edward-island", "north-america-canada-quebec", "north-america-canada-saskatchewan", "north-america-canada-yukon"};

    static String[] us = new String[]{"north-america-us-alabama", "north-america-us-arizona", "north-america-us-arkansas", "north-america-us-california", "north-america-us-colorado", "north-america-us-connecticut", "north-america-us-delaware", "north-america-us-district-of-columbia", "north-america-us-florida", "north-america-us-georgia", "north-america-us-idaho", "north-america-us-illinois", "north-america-us-indiana", "north-america-us-iowa", "north-america-us-kansas", "north-america-us-kentucky", "north-america-us-louisiana", "north-america-us-maine", "north-america-us-maryland", "north-america-us-massachusetts", "north-america-us-michigan", "north-america-us-minnesota", "north-america-us-mississippi", "north-america-us-missouri", "north-america-us-montana", "north-america-us-nebraska", "north-america-us-nevada", "north-america-us-new-hampshire", "north-america-us-new-jersey", "north-america-us-new-mexico", "north-america-us-new-york", "north-america-us-north-carolina", "north-america-us-north-dakota", "north-america-us-ohio", "north-america-us-oklahoma", "north-america-us-oregon", "north-america-us-pennsylvania", "north-america-us-puerto-rico", "north-america-us-rhode-island", "north-america-us-south-carolina", "north-america-us-south-dakota", "north-america-us-tennessee", "north-america-us-texas", "north-america-us-utah", "north-america-us-vermont", "north-america-us-virginia", "north-america-us-washington", "north-america-us-west-virginia", "north-america-us-wisconsin", "north-america-us-wyoming"};

    static String[] brazil = new String[]{"south-america-brazil-centro_oeste", "south-america-brazil-nordeste", "south-america-brazil-norte", "south-america-brazil-sudeste", "south-america-brazil-sul"};

    static String[] benelux = new String[]{"europe-luxembourg", "europe-belgium", "europe-netherlands-drenthe", "europe-netherlands-flevoland", "europe-netherlands-friesland", "europe-netherlands-gelderland", "europe-netherlands-groningen", "europe-netherlands-limburg", "europe-netherlands-noord-brabant", "europe-netherlands-noord-holland", "europe-netherlands-overijssel", "europe-netherlands-utrecht", "europe-netherlands-zeeland", "europe-netherlands-zuid-holland"};

    static String[] world = {"africa-algeria", "africa-angola", "africa-benin", "africa-botswana", "africa-burkina-faso", "africa-burundi", "africa-cameroon", "africa-canary-islands", "africa-cape-verde", "africa-central-african-republic", "africa-chad", "africa-comores", "africa-congo-brazzaville", "africa-congo-democratic-republic", "africa-djibouti", "africa-egypt", "africa-equatorial-guinea", "africa-eritrea", "africa-ethiopia", "africa-gabon", "africa-ghana", "africa-guinea", "africa-guinea-bissau", "africa-ivory-coast", "africa-kenya", "africa-lesotho", "africa-liberia", "africa-libya", "africa-madagascar", "africa-malawi", "africa-mali", "africa-mauritania", "africa-mauritius", "africa-morocco", "africa-mozambique", "africa-namibia", "africa-niger", "africa-nigeria", "africa-rwanda", "africa-saint-helena-ascension-and-tristan-da-cunha", "africa-sao-tome-and-principe", "africa-senegal-and-gambia", "africa-seychelles", "africa-sierra-leone", "africa-somalia", "africa-south-africa", "africa-south-sudan", "africa-sudan", "africa-swaziland", "africa-tanzania", "africa-togo", "africa-tunisia", "africa-uganda", "africa-zambia", "africa-zimbabwe", "asia-afghanistan", "asia-armenia", "asia-azerbaijan", "asia-bangladesh", "asia-bhutan", "asia-cambodia", "asia-china", "asia-gcc-states", "asia-india", "asia-indonesia", "asia-iran", "asia-iraq", "asia-israel-and-palestine", "asia-japan", "asia-jordan", "asia-kazakhstan", "asia-kyrgyzstan", "asia-laos", "asia-lebanon", "asia-malaysia-singapore-brunei", "asia-maldives", "asia-mongolia", "asia-myanmar", "asia-nepal", "asia-north-korea", "asia-pakistan", "asia-philippines", "asia-south-korea", "asia-sri-lanka", "asia-syria", "asia-taiwan", "asia-tajikistan", "asia-thailand", "asia-turkmenistan", "asia-uzbekistan", "asia-vietnam", "asia-yemen", "australia-oceania-american-oceania", "australia-oceania-australia", "australia-oceania-cook-islands", "australia-oceania-fiji", "australia-oceania-ile-de-clipperton", "australia-oceania-kiribati", "australia-oceania-marshall-islands", "australia-oceania-micronesia", "australia-oceania-nauru", "australia-oceania-new-caledonia", "australia-oceania-new-zealand", "australia-oceania-niue", "australia-oceania-palau", "australia-oceania-papua-new-guinea", "australia-oceania-pitcairn-islands", "australia-oceania-polynesie-francaise", "australia-oceania-samoa", "australia-oceania-solomon-islands", "australia-oceania-tokelau", "australia-oceania-tonga", "australia-oceania-tuvalu", "australia-oceania-vanuatu", "australia-oceania-wallis-et-futuna", "central-america-bahamas", "central-america-belize", "central-america-costa_rica", "central-america-cuba", "central-america-el-salvador", "central-america-guatemala", "central-america-haiti-and-domrep", "central-america-honduras", "central-america-jamaica", "central-america-nicaragua", "europe-albania", "europe-andorra", "europe-austria", "europe-azores", "europe-belarus", "europe-belgium", "europe-bosnia_herzegovina", "europe-bulgaria", "europe-croatia", "europe-cyprus", "europe-czech_republic", "europe-denmark", "europe-estonia", "europe-faroe_islands", "europe-finland", "europe-france-alsace", "europe-france-aquitaine", "europe-france-auvergne", "europe-france-basse-normandie", "europe-france-bourgogne", "europe-france-bretagne", "europe-france-centre", "europe-france-champagne-ardenne", "europe-france-corse", "europe-france-franche-comte", "europe-france-guadeloupe", "europe-france-guyane", "europe-france-haute-normandie", "europe-france-ile-de-france", "europe-france-languedoc-roussillon", "europe-france-limousin", "europe-france-lorraine", "europe-france-martinique", "europe-france-mayotte", "europe-france-midi-pyrenees", "europe-france-nord-pas-de-calais", "europe-france-pays-de-la-loire", "europe-france-picardie", "europe-france-poitou-charentes", "europe-france-provence-alpes-cote-d-azur", "europe-france-reunion", "europe-france-rhone-alpes", "europe-georgia", "europe-germany-baden-wuerttemberg", "europe-germany-bayern", "europe-germany-berlin", "europe-germany-brandenburg", "europe-germany-bremen", "europe-germany-hamburg", "europe-germany-hessen", "europe-germany-mecklenburg-vorpommern", "europe-germany-niedersachsen", "europe-germany-nordrhein-westfalen", "europe-germany-rheinland-pfalz", "europe-germany-saarland", "europe-germany-sachsen-anhalt", "europe-germany-sachsen", "europe-germany-schleswig-holstein", "europe-germany-thueringen", "europe-greece", "europe-guernsey_jersey", "europe-hungary", "europe-iceland", "europe-ireland_and_northern_ireland", "europe-isle_of_man", "europe-italy-centro", "europe-italy-isole", "europe-italy-nord-est", "europe-italy-nord-ovest", "europe-italy-sud", "europe-kosovo", "europe-latvia", "europe-liechtenstein", "europe-lithuania", "europe-luxembourg", "europe-macedonia", "europe-malta", "europe-moldova", "europe-monaco", "europe-montenegro", "europe-netherlands-drenthe", "europe-netherlands-flevoland", "europe-netherlands-friesland", "europe-netherlands-gelderland", "europe-netherlands-groningen", "europe-netherlands-limburg", "europe-netherlands-noord-brabant", "europe-netherlands-noord-holland", "europe-netherlands-overijssel", "europe-netherlands-utrecht", "europe-netherlands-zeeland", "europe-netherlands-zuid-holland", "europe-norway", "europe-poland-dolnoslaskie", "europe-poland-kujawsko-pomorskie", "europe-poland-lodzkie", "europe-poland-lubelskie", "europe-poland-lubuskie", "europe-poland-malopolskie", "europe-poland-mazowieckie", "europe-poland-opolskie", "europe-poland-podkarpackie", "europe-poland-podlaskie", "europe-poland-pomorskie", "europe-poland-slaskie", "europe-poland-swietokrzyskie", "europe-poland-warminsko-mazurskie", "europe-poland-wielkopolskie", "europe-poland-zachodniopomorskie", "europe-portugal", "europe-romania", "europe-serbia", "europe-slovakia", "europe-slovenia", "europe-spain", "europe-sweden", "europe-switzerland", "europe-turkey", "europe-united-kingdom-england", "europe-united-kingdom-scotland", "europe-united-kingdom-wales", "europe-ukraine", "north-america-canada-alberta", "north-america-canada-british-columbia", "north-america-canada-manitoba", "north-america-canada-new-brunswick", "north-america-canada-newfoundland-and-labrador", "north-america-canada-northwest-territories", "north-america-canada-nova-scotia", "north-america-canada-nunavut", "north-america-canada-ontario", "north-america-canada-prince-edward-island", "north-america-canada-quebec", "north-america-canada-saskatchewan", "north-america-canada-yukon", "north-america-greenland", "north-america-mexico", "north-america-us-alabama", "north-america-us-alaska", "north-america-us-arizona", "north-america-us-arkansas", "north-america-us-california", "north-america-us-colorado", "north-america-us-connecticut", "north-america-us-delaware", "north-america-us-district-of-columbia", "north-america-us-florida", "north-america-us-georgia", "north-america-us-hawaii", "north-america-us-idaho", "north-america-us-illinois", "north-america-us-indiana", "north-america-us-iowa", "north-america-us-kansas", "north-america-us-kentucky", "north-america-us-louisiana", "north-america-us-maine", "north-america-us-maryland", "north-america-us-massachusetts", "north-america-us-michigan", "north-america-us-minnesota", "north-america-us-mississippi", "north-america-us-missouri", "north-america-us-montana", "north-america-us-nebraska", "north-america-us-nevada", "north-america-us-new-hampshire", "north-america-us-new-jersey", "north-america-us-new-mexico", "north-america-us-new-york", "north-america-us-north-carolina", "north-america-us-north-dakota", "north-america-us-ohio", "north-america-us-oklahoma", "north-america-us-oregon", "north-america-us-pennsylvania", "north-america-us-puerto-rico", "north-america-us-rhode-island", "north-america-us-south-carolina", "north-america-us-south-dakota", "north-america-us-tennessee", "north-america-us-texas", "north-america-us-utah", "north-america-us-vermont", "north-america-us-virginia", "north-america-us-washington", "north-america-us-west-virginia", "north-america-us-wisconsin", "north-america-us-wyoming", "south-america-argentina", "south-america-bolivia", "south-america-brazil-centro_oeste", "south-america-brazil-nordeste", "south-america-brazil-norte", "south-america-brazil-sudeste", "south-america-brazil-sul", "south-america-chile", "south-america-colombia", "south-america-ecuador", "south-america-paraguay", "south-america-peru", "south-america-suriname", "south-america-uruguay","south-america-venezuela"};

    static final OsmMapValues[] osm_maps = {

        new OsmMapValues(R.string.whole_planet, "-180", "-90", "180", "90",
                         0, world),
        new OsmMapValues(R.string.africa, "-30.89", "-36.17", "61.68", "38.40",
                         0, africa) ,
        new OsmMapValues(R.string.angola, "11.4", "-18.1", "24.2", "-5.3",
                         1, new String[]{"africa-angola"}),
        new OsmMapValues(R.string.burundi, "28.9", "-4.5", "30.9", "-2.2",
                         1, new String[]{"africa-burundi"}),
        new OsmMapValues(R.string.canary_islands, "-18.69", "26.52", "-12.79", "29.99",
                         1, new String[]{"africa-canary-islands"}),
        new OsmMapValues(R.string.congo, "11.7", "-13.6", "31.5", "5.7",
                         1, new String[]{"africa-congo"}),
        new OsmMapValues(R.string.ethiopia, "32.89", "3.33", "48.07", "14.97",
                         1, new String[]{"africa-ethiopia"}) ,
        new OsmMapValues(R.string.guinea, "-15.47", "7.12", "-7.58", "12.74",
                         1, new String[]{"africa-guinea"}),
        new OsmMapValues(R.string.cotedivoire, "-8.72", "4.09", "-2.43", "10.80",
                         1, new String[]{"africa-ivory-coast"}),
        new OsmMapValues(R.string.kenya, "33.8", "-5.2", "42.4", "4.9",
                         1, new String[]{"africa-kenya"}),
        new OsmMapValues(R.string.lesotho, "26.9", "-30.7", "29.6", "-28.4",
                         1, new String[]{"africa-lesotho"}),
        new OsmMapValues(R.string.liberia, "-15.00", "-0.73", "-7.20", "8.65",
                         1, new String[]{"africa-liberia"}),
        new OsmMapValues(R.string.libya, "9.32", "19.40", "25.54", "33.63",
                         1, new String[]{"africa-lybia"}),
        new OsmMapValues(R.string.madagascar, "42.25", "-26.63", "51.20", "-11.31",
                         1, new String[]{"africa-madagascar"}),
        new OsmMapValues(R.string.namibia,"11.4", "-29.1", "29.5", "-16.9",
                         1, new String[]{"africa-namibia"}),
        new OsmMapValues(R.string.botswana, "11.4", "-29.1", "29.5", "-16.9",
                         1, new String[]{"africa-botswana"}),
        new OsmMapValues(R.string.reunion, "55.2", "-21.4", "55.9", "-20.9",
                         1, new String[]{"africa-reunion"}),
        new OsmMapValues(R.string.rwanda, "28.8", "-2.9", "30.9", "-1.0",
                         1, new String[]{"africa-rwanda"}),
        new OsmMapValues(R.string.south_africa, "15.93", "-36.36", "33.65", "-22.08",
                         1, new String[]{"africa-south-africa"}),
        new OsmMapValues(R.string.tanzania, "29.19", "-11.87", "40.74", "-0.88",
                         1, new String[]{"africa-tanzania"}),
        new OsmMapValues(R.string.uganda, "29.3", "-1.6", "35.1", "4.3",
                         1, new String[]{"africa-uganda"}),
        new OsmMapValues(R.string.asia, "23.8", "0.1", "195.0", "82.4",
                         0, asia),
        new OsmMapValues(R.string.azerbaijan, "44.74", "38.34", "51.69", "42.37",
                         1, new String[]{"asia-azerbaijan"}),
        new OsmMapValues(R.string.china, "67.3", "5.3", "135.0", "54.5",
                         1, new String[]{"asia-china"}),
        new OsmMapValues(R.string.cyprus, "32.0", "34.5", "34.9", "35.8",
                         1, new String[]{"europe-cyprus"}),
        new OsmMapValues(R.string.india, "67.9", "5.5", "89.6", "36.0",
                         1, new String[]{"asia-india"}),
        new OsmMapValues(R.string.nepal, "67.9", "5.5", "89.6", "36.0",
                         1, new String[]{"asia-nepal"}),
        new OsmMapValues(R.string.indonesia, "93.7", "-17.3", "155.5", "7.6",
                         1, new String[]{"asia-indonesia"}),
        new OsmMapValues(R.string.iran, "43.5", "24.4", "63.6", "40.4",
                         1, new String[]{"asia-iran"}),
        new OsmMapValues(R.string.iraq, "38.7", "28.5", "49.2", "37.4",
                         1, new String[]{"asia-iraq"}),
        new OsmMapValues(R.string.israel, "33.99", "29.8", "35.95", "33.4",
                         1, new String[]{"asia-israel"}),
        new OsmMapValues(R.string.japan, "123.6", "25.2", "151.3", "47.1",
                         1, new String[]{"asia-japan"}),
        new OsmMapValues(R.string.kazakhstan, "46.44", "40.89", "87.36", "55.45",
                         1, new String[]{"asia-kazakhstan"}),
        new OsmMapValues(R.string.kyrgyzsyan, "69.23", "39.13", "80.33", "43.29",
                         1, new String[]{"asia-kyrgyzsyan"}),
        new OsmMapValues(R.string.malaysia, "94.3", "-5.9", "108.6", "6.8",
                         1, new String[]{"asia-malaysia"}),
        new OsmMapValues(R.string.mongolia, "87.5", "41.4", "120.3", "52.7",
                         1, new String[]{"asia-mongolia"}),
        new OsmMapValues(R.string.pakistan, "60.83", "23.28", "77.89", "37.15",
                         1, new String[]{"asia-pakistan"}),
        new OsmMapValues(R.string.philippines, "115.58", "4.47", "127.85", "21.60",
                         1, new String[]{"asia-philippines"}),
        new OsmMapValues(R.string.saudi_arabia, "33.2", "16.1", "55.9", "33.5",
                         1, new String[]{"asia-saudi-arabia"}),
        new OsmMapValues(R.string.taiwan, "119.1", "21.5", "122.5", "25.2",
                         1, new String[]{"asia-taiwan"}),
        new OsmMapValues(R.string.thailand, "97.5", "5.7", "105.2", "19.7",
                         1, new String[]{"asia-thailand"}),
        new OsmMapValues(R.string.turkey, "25.1", "35.8", "46.4", "42.8",
                         1, new String[]{"europe-turkey"}),
        new OsmMapValues(R.string.turkmenistan, "51.78", "35.07", "66.76", "42.91",
                         1, new String[]{"asia-turkmenistan"}),
        new OsmMapValues(R.string.australia, R.string.oceania, "89.84", "-57.39", "179.79", "7.26",
                         0, australia_oceania),
        new OsmMapValues(R.string.australia, "110.5", "-44.2", "154.9", "-9.2",
                         0, new String[]{"australia-oceania-australia"}),
        new OsmMapValues(R.string.newzealand, "165.2", "-47.6", "179.1", "-33.7",
                         1, new String[]{"australia-oceania-new-zeeland"}),
        new OsmMapValues(R.string.europe, "-12.97", "33.59", "34.15", "72.10",
                         0, europe),
        new OsmMapValues(R.string.austria, "9.4", "46.32", "17.21", "49.1",
                         1, new String[]{"europe-austria"}),
        new OsmMapValues(R.string.azores, "-31.62", "36.63", "-24.67", "40.13",
                         1, new String[]{"europe-azores"}),
        new OsmMapValues(R.string.belgium, "2.3", "49.5", "6.5", "51.6",
                         1, new String[]{"europe-belgium"}),
        new OsmMapValues(R.string.benelux, "2.08", "48.87", "7.78", "54.52",
                         1, benelux),
        new OsmMapValues(R.string.netherlands, "3.07", "50.75", "7.23", "53.73",
                         1, netherlands),
        new OsmMapValues(R.string.denmark, "7.65", "54.32", "15.58", "58.07",
                         1, new String[]{"europe-denmark"}),
        new OsmMapValues(R.string.faroe_islands, "-7.8", "61.3", "-6.1", "62.5",
                         1, new String[]{"europe-faroe-islands"}),
        new OsmMapValues(R.string.france, "-5.20", "42.20", "8.20", "51.68",
                         1, france),
        new OsmMapValues(R.string.alsace, "6.79", "47.27", "8.48", "49.17",
                         2, new String[]{"europe-france-alsace"}),
        new OsmMapValues(R.string.aquitaine, "-2.27", "42.44", "1.50", "45.76",
                         2, new String[]{"europe-france-aquitaine"}),
        new OsmMapValues(R.string.auvergne, "2.01", "44.57", "4.54", "46.85",
                         2, new String[]{"europe-france-auvergne"}),
        new OsmMapValues(R.string.basse_normandie, "-2.09", "48.13", "1.03", "49.98",
                         2, new String[]{"europe-france-basse-normandie"}),
        new OsmMapValues(R.string.bourgogne, "2.80", "46.11", "5.58", "48.45",
                         2, new String[]{"europe-france-bourgogne"}),
        new OsmMapValues(R.string.bretagne, "-5.58", "46.95", "-0.96", "48.99",
                         2, new String[]{"europe-france-bretagne"}),
        new OsmMapValues(R.string.centre, "0.01", "46.29", "3.18", "48.99",
                         2, new String[]{"europe-france-centre"}),
        new OsmMapValues(R.string.champagne_ardenne, "3.34", "47.53", "5.94", "50.28",
                         2, new String[]{"europe-france-champagne-ardenne"}),
        new OsmMapValues(R.string.corse, "8.12", "41.32", "9.95", "43.28",
                         2, new String[]{"europe-france-corse"}),
        new OsmMapValues(R.string.franche_comte, "5.20", "46.21", "7.83", "48.07",
                         2, new String[]{"europe-france-franche-comte"}),
        new OsmMapValues(R.string.haute_normandie, "-0.15", "48.62", "1.85", "50.18",
                         2, new String[]{"europe-france-haute-normandie"}),
            new OsmMapValues(R.string.ile_de_france, "1.40", "48.07", "3.61", "49.29",
                         2, new String[]{"europe-france-ile-de-france"}),
        new OsmMapValues(R.string.languedoc_roussillon, "1.53", "42.25", "4.89", "45.02",
                         2, new String[]{"europe-france-languedoc-roussillon"}),
        new OsmMapValues(R.string.limousin, "0.58", "44.87", "2.66", "46.50",
                         2, new String[]{"europe-france-limousin"}),
        new OsmMapValues(R.string.lorraine, "4.84", "47.77", "7.72", "49.73",
                         2, new String[]{"europe-france-lorraine"}),
        new OsmMapValues(R.string.midi_pyrenees, "-0.37", "42.18", "3.50", "45.10",
                         2, new String[]{"europe-france-midi-pyrenees"}),
        new OsmMapValues(R.string.nord_pas_de_calais, "1.42", "49.92", "4.49", "51.31",
                         2, new String[]{"europe-france-nord-pas-de-calais"}),
        new OsmMapValues(R.string.pays_de_la_loire, "-2.88", "46.20", "0.97", "48.62",
                         2, new String[]{"europe-france-pays-de-la-loire"}),
        new OsmMapValues(R.string.picardie, "1.25", "48.79", "4.31", "50.43",
                         2, new String[]{"europe-france-picardie"}),
        new OsmMapValues(R.string.poitou_charentes, "-1.69", "45.04", "1.26", "47.23",
                         2, new String[]{"europe-france-poitou-charentes"}),
        new OsmMapValues(R.string.provence_alpes_cote_d_azur, "4.21", "42.91", "7.99", "45.18",
                         2, new String[]{"europe-france-provence-alpes-cote-d-azur"}),
        new OsmMapValues(R.string.rhone_alpes, "3.65", "44.07", "7.88", "46.64",
                         2, new String[]{"europe-france-rhone-alpes"}),
        new OsmMapValues(R.string.luxembourg, "5.7", "49.4", "6.5", "50.2",
                         1, new String[]{"europe-luxembourg"}),
        new OsmMapValues(R.string.germany, "5.18", "46.84", "15.47", "55.64",
                         1, germany),
        new OsmMapValues(R.string.baden_wuerttemberg, "7.32", "47.14", "10.57", "49.85",
                         2, new String[]{"europe-germany-baden-wuerttemberg"}),
        new OsmMapValues(R.string.bayern, "8.92", "47.22", "13.90", "50.62",
                         2, new String[]{"europe-germany-bayern"}),
        new OsmMapValues(R.string.berlin, "13.03", "52.28", "13.81", "52.73",
                         2, new String[]{"europe-germany-berlin"}),
        new OsmMapValues(R.string.brandenburg, "11.17", "51.30", "14.83", "53.63",
                         2, new String[]{"europe-germany-brandenburg"}),
        new OsmMapValues(R.string.bremen, "8.43", "52.96", "9.04", "53.66",
                         2, new String[]{"europe-germany-berlin"}),
        new OsmMapValues(R.string.hamburg, "9.56", "53.34", "10.39", "53.80",
                         2, new String[]{"europe-germany-hamburg"}),
        new OsmMapValues(R.string.hessen, "7.72", "49.34", "10.29", "51.71",
                         2, new String[]{"europe-germany-hessen"}),
        new OsmMapValues(R.string.mecklenburg_vorpommern, "10.54", "53.05", "14.48", "55.05",
                         2, new String[]{"europe-germany-mecklenburg-vorpommern"}),
        new OsmMapValues(R.string.niedersachsen, "6.40", "51.24", "11.69", "54.22",
                         2, new String[]{"europe-germany-niedersachsen"}),
        new OsmMapValues(R.string.nordrhein_westfalen, "5.46", "50.26", "9.52", "52.59",
                         2, new String[]{"europe-germany-nordrhein-westfalen"}),
        new OsmMapValues(R.string.rheinland_pfalz, "6.06", "48.91", "8.56", "51.00",
                         2, new String[]{"europe-germany-rheinland-pfalz"}),
        new OsmMapValues(R.string.saarland, "6.30", "49.06", "7.46", "49.69",
                         2, new String[]{"europe-germany-saarland"}),
        new OsmMapValues(R.string.sachsen_anhalt, "10.50", "50.88", "13.26", "53.11",
                         2, new String[]{"europe-germany-sachsen-anhalt"}),
        new OsmMapValues(R.string.sachsen, "11.82", "50.11", "15.10", "51.73",
                         2, new String[]{"europe-germany-sachsen"}),
        new OsmMapValues(R.string.schleswig_holstein, "7.41", "53.30", "11.98", "55.20",
                         2, new String[]{"europe-germany-schleswig-holstein"}),
        new OsmMapValues(R.string.thueringen, "9.81", "50.15", "12.72", "51.70",
                         2, new String[]{"europe-germany-thueringen"}),
        new OsmMapValues(R.string.iceland, "-25.3", "62.8", "-11.4", "67.5",
                         1, new String[]{"europe-iceland"}),
        new OsmMapValues(R.string.ireland, "-11.17", "51.25", "-5.23", "55.9",
                         1, new String[]{"europe-ireland"}),
        new OsmMapValues(R.string.italy, "6.52", "36.38", "18.96", "47.19",
                         1, italy),
        new OsmMapValues(R.string.portugal, "-11.04", "34.87", "4.62", "44.41",
                         1, new String[]{"europe-portugal"}),
        new OsmMapValues(R.string.spain, "-11.04", "34.87", "4.62", "44.41",
                         1, new String[]{"europe-spain"}),
        new OsmMapValues(R.string.scandinavia, "4.0", "54.4", "32.1", "71.5",
                         1, scandinavia),
        new OsmMapValues(R.string.finland, "18.6", "59.2", "32.3", "70.3",
                         1, new String[]{"europe-finland"}),
        new OsmMapValues(R.string.denmark, "7.49", "54.33", "13.05", "57.88",
                         1, new String[]{"europe-denmark"}),
        new OsmMapValues(R.string.switzerland, "5.79", "45.74", "10.59", "47.84",
                         1, new String[]{"europe-switzerland"}),
        new OsmMapValues(R.string.united_kingdom, "-9.7", "49.6", "2.2", "61.2",
                         1, united_kingdom),
        new OsmMapValues(R.string.england, "-7.80", "48.93", "2.41", "56.14",
                         2, new String[]{"europe-united-kingdom-england"}),
        new OsmMapValues(R.string.scotland, "-8.13", "54.49", "-0.15", "61.40",
                         2, new String[]{"europe-united-kingdom-scotland"}),
        new OsmMapValues(R.string.wales, "-5.56", "51.28", "-2.60", "53.60",
                         2, new String[]{"europe-united-kingdom-wales"}),
        new OsmMapValues(R.string.albania, "19.09", "39.55", "21.12", "42.72",
                         1, new String[]{"europe-albania"}),
        new OsmMapValues(R.string.belarus, "23.12", "51.21", "32.87", "56.23",
                         1, new String[]{"europe-belarus"}),
        new OsmMapValues(R.string.bulgaria, "24.7", "42.1", "24.8", "42.1",
                         1, new String[]{"europe-bulgaria"}),
        new OsmMapValues(R.string.bosnia_and_herzegovina, "15.69", "42.52", "19.67", "45.32",
                         1, new String[]{"europe-bosnia-and-herzegovina"}),
        new OsmMapValues(R.string.czech_republic, "11.91", "48.48", "19.02", "51.17",
                         1, new String[]{"europe-czech-republic"}),
        new OsmMapValues(R.string.croatia, "13.4", "42.1", "19.4", "46.9",
                         1, new String[]{"europe-croatia"}),
        new OsmMapValues(R.string.estonia, "21.5", "57.5", "28.2", "59.6",
                         1, new String[]{"europe-estonia"}),
        new OsmMapValues(R.string.greece, "28.9", "37.8", "29.0", "37.8",
                         1, new String[]{"europe-greece"}),
        new OsmMapValues(R.string.hungary, "16.08", "45.57", "23.03", "48.39",
                         1, new String[]{"europe-hungary"}),
        new OsmMapValues(R.string.latvia, "20.7", "55.6", "28.3", "58.1",
                         1, new String[]{"europe-latvia"}),
        new OsmMapValues(R.string.lithuania, "20.9", "53.8", "26.9", "56.5",
                         1, new String[]{"europe-lithuania"}),
        new OsmMapValues(R.string.poland, "13.6", "48.8", "24.5", "55.0",
                         1, poland),
        new OsmMapValues(R.string.romania, "20.3", "43.5", "29.9", "48.4",
                         1, new String[]{"europe-romania"}),
        new OsmMapValues(R.string.slovakia, "16.8", "47.7", "22.6", "49.7",
                         1, new String[]{"europe-slovakia"}),
        new OsmMapValues(R.string.ukraine, "22.0", "44.3", "40.4", "52.4",
                         1, new String[]{"europe-ukraine"}),
        new OsmMapValues(R.string.north_america, "-178.1", "6.5", "-10.4", "84.0",
                         0, north_america),
        new OsmMapValues(R.string.alaska, "-179.5", "49.5", "-129", "71.6",
                         1, new String[]{"north-america-us-alaska"}),
        new OsmMapValues(R.string.canada, "-141.3", "41.5", "-52.2", "70.2",
                         1, canada),
        new OsmMapValues(R.string.hawaii, "-161.07", "18.49", "-154.45", "22.85",
                         1, new String[]{"north-america-us-hawaii"}),
        new OsmMapValues(R.string.usa+R.string.except_alaska_and_hawaii, "-125.4", "24.3", "-66.5", "49.3",
                         1, us),
        new OsmMapValues(R.string.michigan, "-90.47", "41.64", "-79.00", "49.37",
                         2, new String[]{"north-america-us-michigan"}),
        new OsmMapValues(R.string.ohio, "-84.87", "38.05", "-79.85", "43.53",
                         2, new String[]{"north-america-us-ohio"}),
        new OsmMapValues(R.string.massachusetts, "-73.56", "40.78", "-68.67", "42.94",
                         2, new String[]{"north-america-us-massachusettts"}),
        new OsmMapValues(R.string.vermont, "-73.49", "42.68", "-71.41", "45.07",
                         2, new String[]{"north-america-us-vermont"}),
        new OsmMapValues(R.string.arkansas, "-94.67", "32.95", "-89.59", "36.60",
                         2, new String[]{"north-america-us-arkansas"}),
        new OsmMapValues(R.string.district_of_columbia, "-77.17", "38.74", "-76.86", "39.05",
                         2, new String[]{"north-america-us-district-of-columbia"}),
        new OsmMapValues(R.string.florida, "-88.75", "23.63", "-77.67", "31.05",
                         2, new String[]{"north-america-us-florida"}),
        new OsmMapValues(R.string.louisiana, "-94.09", "28.09", "-88.62", "33.07",
                         2, new String[]{"north-america-us-louisiana"}),
        new OsmMapValues(R.string.maryland, "-79.54", "37.83", "-74.99", "40.22",
                         2, new String[]{"north-america-us-maryland"}),
        new OsmMapValues(R.string.mississippi, "-91.71", "29.99", "-88.04", "35.05",
                         2, new String[]{"north-america-us-mississippi"}),
        new OsmMapValues(R.string.oklahoma, "-103.41", "33.56", "-94.38", "37.38",
                         2, new String[]{"north-america-us-oklahoma"}),
        new OsmMapValues(R.string.texas, "-106.96", "25.62", "-92.97", "36.58",
                         2, new String[]{"north-america-us-texas"}),
        new OsmMapValues(R.string.virginia, "-83.73", "36.49", "-74.25", "39.52",
                         2, new String[]{"north-america-us-virginia"}),
        new OsmMapValues(R.string.west_virginia, "-82.70", "37.15", "-77.66", "40.97",
                         2, new String[]{"north-america-us-west-virginia"}),
        new OsmMapValues(R.string.arizona, "-114.88", "30.01", "-108.99", "37.06",
                         2, new String[]{"north-america-us-arizona"}),
        new OsmMapValues(R.string.california, "-125.94", "32.43", "-114.08", "42.07",
                         2, new String[]{"north-america-us-california"}),
        new OsmMapValues(R.string.colorado, "-109.11", "36.52", "-100.41", "41.05",
                         2, new String[]{"north-america-us-colorado"}),
        new OsmMapValues(R.string.idaho, "-117.30", "41.93", "-110.99", "49.18",
                         2, new String[]{"north-america-us-idaho"}),
        new OsmMapValues(R.string.montana, "-116.10", "44.31", "-102.64", "49.74",
                         2, new String[]{"north-america-us-montana"}),
        new OsmMapValues(R.string.new_mexico, "-109.10", "26.98", "-96.07", "37.05",
                         2, new String[]{"north-america-us-new-mexico"}),
        new OsmMapValues(R.string.nevada, "-120.2", "35.0", "-113.8", "42.1",
                         2, new String[]{"north-america-us-nevada"}),
        new OsmMapValues(R.string.oregon, "-124.8", "41.8", "-116.3", "46.3",
                         2, new String[]{"north-america-us-oregon"}),
        new OsmMapValues(R.string.utah, "-114.11", "36.95", "-108.99", "42.05",
                         2, new String[]{"north-america-us-utah"}),
        new OsmMapValues(R.string.washington_state, "-125.0", "45.5", "-116.9", "49.0",
                         2, new String[]{"north-america-us-washington"}),
        new OsmMapValues(R.string.south_middle_america, "-83.5", "-56.3", "-30.8", "13.7",
                         0, south_middle_america),
        new OsmMapValues(R.string.argentina, "-73.9", "-57.3", "-51.6", "-21.0",
                         1, new String[]{"south-america-argentinia"}),
        new OsmMapValues(R.string.chile, "-77.2", "-56.3", "-52.7", "-16.1",
                         1, new String[]{"south-america-chile"}),
        new OsmMapValues(R.string.bolivia, "-70.5", "-23.1", "-57.3", "-9.3",
                         1, new String[]{"south-america-bolivia"}),
        new OsmMapValues(R.string.brazil, "-71.4", "-34.7", "-32.8", "5.4",
                         1, brazil),
        new OsmMapValues(R.string.cuba, "-85.3", "19.6", "-74.0", "23.6",
                         1, new String[]{"south-america-cuba"}),
        new OsmMapValues(R.string.colombia, "-79.1", "-4.0", "-66.7", "12.6",
                         1, new String[]{"south-america-columbia"}),
        new OsmMapValues(R.string.ecuador, "-82.6", "-5.4", "-74.4", "2.3",
                         1, new String[]{"south-america-ecuador"}),
        new OsmMapValues(R.string.suriname, "-62.0", "1.0", "-51.2", "8.9",
                         1, new String[]{"south-america-suriname"}),
        new OsmMapValues(R.string.guyane_francaise, "-62.0", "1.0", "-51.2", "8.9",
                         1, new String[]{"europe-france-guyane"}),
        new OsmMapValues(R.string.haiti, "-74.8", "17.3", "-68.2", "20.1",
                         1, new String[]{"south-america-haiti"}),
        new OsmMapValues(R.string.dominican_republic, "-74.8", "17.3", "-68.2", "20.1",
                         1, new String[]{"south-america-dominican-republic"}),
        new OsmMapValues(R.string.jamaica, "-78.6", "17.4", "-75.9", "18.9",
                         1, new String[]{"south-america-jamaica"}),
        new OsmMapValues(R.string.mexico, "-117.6", "14.1", "-86.4", "32.8",
                         1, new String[]{"south-america-mexico"}),
        new OsmMapValues(R.string.paraguay, "-63.8", "-28.1", "-53.6", "-18.8",
                         1, new String[]{"south-america-paraguay"}),
        new OsmMapValues(R.string.peru, "-82.4", "-18.1", "-67.5", "0.4",
                         1, new String[]{"south-america-peru"}),
        new OsmMapValues(R.string.uruguay, "-59.2", "-36.5", "-51.7", "-29.7",
                         1, new String[]{"south-america-uruguay"}),
        new OsmMapValues(R.string.venezuela, "-73.6", "0.4", "-59.7", "12.8",
                         1, new String[]{"south-america-venezuela"}) };
    //we should try to resume
    private static final int SOCKET_CONNECT_TIMEOUT = 60000;          // 60 secs.
    private static final int SOCKET_READ_TIMEOUT = 120000;         // 120 secs.
    private static final int MAP_WRITE_FILE_BUFFER = 1024 * 64;
    private static final int MAP_WRITE_MEM_BUFFER = 1024 * 64;
    private static final int MAP_READ_FILE_BUFFER = 1024 * 64;
    private static final int UPDATE_PROGRESS_TIME_NS = 1000 * 1000000; // 1ns=1E-9s
    private static final int MAX_RETRIES = 5;
    private static final String TAG = "NavitMapDownLoader";
    private final OsmMapValues mMapValues;
    private final int mMapId;

    private String mGitHubMetadata;
    private Boolean mStopMe = false;
    private long mUiLastUpdated = -1;
    private Boolean mRetryDownload = false; //Download failed, but
    private int mRetryCounter = 0;

    NavitMapDownloader(int mapId) {
        this.mMapValues = osm_maps[mapId];
        this.mMapId = mapId;

        URL url;
        try {
            url = new URL("https://api.github.com/repositories/384098365/releases/latest");

            InputStream is = url.openStream();
            BufferedReader br = new BufferedReader(new InputStreamReader(is));
            this.mGitHubMetadata = br.readLine();
        } catch (MalformedURLException e) {
            Log.e(TAG, "We failed to create a URL to download the github api file.");
            e.printStackTrace();
            this.mGitHubMetadata = "";
        } catch (IOException e) {
            Log.e(TAG, "We failed to download the github api file.");
            e.printStackTrace();
            this.mGitHubMetadata = "";
        }
    }

    static NavitMap[] getAvailableMaps() {
        class FilterMaps implements FilenameFilter {

            public boolean accept(File dir, String filename) {
                return (filename.endsWith(".bin"));
            }
        }

        NavitMap[] maps = new NavitMap[0];
        File mapDir = new File(Navit.sMapFilenamePath);
        String[] mapFileNames = mapDir.list(new FilterMaps());
        if (mapFileNames != null) {
            maps = new NavitMap[mapFileNames.length];
            for (int mapFileIndex = 0; mapFileIndex < mapFileNames.length; mapFileIndex++) {
                maps[mapFileIndex] = new NavitMap(Navit.sMapFilenamePath,
                                                  mapFileNames[mapFileIndex]);
            }
        }
        return maps;
    }

    @Override
    public void run() {

        Log.v(TAG, "start download " + mMapValues.mMapName);
        updateProgress(0, getMapSize(mMapId),
                       getTstring(R.string.map_downloading) + ": " + mMapValues.mMapName);

        boolean success=false;
	for (int subMapIndex = 0; subMapIndex < this.mMapValues.mSubMaps.length; subMapIndex++) {
            mStopMe = false;
            mRetryCounter = 0;
            do {
                try {
                    Thread.sleep(10 + mRetryCounter * 1000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                mRetryDownload = false;
                success = download_osm_map(subMapIndex);
            } while (!success
                     && mRetryDownload
                     && mRetryCounter < MAX_RETRIES
                     && !mStopMe);

            if (success) {
                toast(mMapValues.mSubMaps[subMapIndex] + " " + getTstring(R.string.map_download_ready));
                getMapInfoFile(subMapIndex).delete();
                Log.d(TAG, "success");
            }

    	}
        if (success || mStopMe) {
            NavitDialogs.sendDialogMessage(NavitDialogs.MSG_MAP_DOWNLOAD_FINISHED,
                    Navit.sMapFilenamePath + mMapValues.mSubMaps[0] + ".bin",
                    null,
                    -1,
                    success ? 1 : 0,
                    mMapId);
        }

        if (success) {
            toast(mMapValues.mMapName + " " + getTstring(R.string.map_download_ready));
        }

    }

    void stop_thread() {
        mStopMe = true;
        Log.d(TAG, "mStopMe -> true");
    }

    private boolean checkFreeSpace(long neededBytes) {
        long freeSpace = NavitUtils.getFreeSpace(Navit.sMapFilenamePath);

        if (neededBytes <= 0) {
            neededBytes = MAP_WRITE_FILE_BUFFER;
        }

        if (freeSpace < neededBytes) {
            String msg;
            Log.e(TAG,
                    "Not enough free space or media not available. Please free at least "
                    + neededBytes / 1024 / 1024 + "Mb.");
            if (freeSpace < 0) {
                msg = getTstring(R.string.map_download_medium_unavailable);
            } else {
                msg = getTstring(R.string.map_download_not_enough_free_space);
            }
            updateProgress(freeSpace, neededBytes,
                           getTstring(R.string.map_download_download_error) + "\n" + msg);
            return false;
        }
        return true;
    }

    private boolean deleteMap(int subMapIndex) {
        File finalOutputFile = getMapFile(subMapIndex);

        if (finalOutputFile.exists()) {
            Message msg = Message.obtain(NavitCallbackHandler.sCallbackHandler,
                                         NavitCallbackHandler.MsgType.CLB_DELETE_MAP.ordinal());
            Bundle b = new Bundle();
            b.putString("title", finalOutputFile.getAbsolutePath());
            msg.setData(b);
            msg.sendToTarget();
            return true;
        }
        return false;
    }

    private boolean download_osm_map(int subMapIndex) {
        long alreadyRead = 0;
        long realSizeBytes;
        boolean resume = true;

        File outputFile = getDestinationFile();
        long oldDownloadSize = outputFile.length();

        URL url = null;
        if (oldDownloadSize > 0) {
            url = readFileInfo(subMapIndex);
        }

        if (url == null) {
            resume = false;
            url = getDownloadURL(subMapIndex);
        }

        URLConnection c = initConnection(url);
        if (c != null) {

            if (resume) {
                c.setRequestProperty("Range", "bytes=" + oldDownloadSize + "-");
                alreadyRead = oldDownloadSize;
            }
            try {
                realSizeBytes = Long.parseLong(c.getHeaderField("Content-Length")) + alreadyRead;
            } catch (Exception e) {
                realSizeBytes = -1;
            }

            long fileTime = c.getLastModified();

            if (!resume) {
                outputFile.delete();
                writeFileInfo(c, realSizeBytes, subMapIndex);
            }

            if (realSizeBytes <= 0) {
                realSizeBytes = getEstSizeBytes(this.mMapId, subMapIndex, this.mGitHubMetadata);
            }

            Log.d(TAG, "size: " + realSizeBytes + ", read: " + alreadyRead + ", timestamp: "
                    + fileTime
                    + ", Connection ref: " + c.getURL());

            if (checkFreeSpace(realSizeBytes - alreadyRead)
                    && downloadData(c, alreadyRead, resume, outputFile, subMapIndex)) {

                File finalOutputFile = getMapFile(subMapIndex);
                // delete an already existing file first
                finalOutputFile.delete();
                // rename file to its final name
                outputFile.renameTo(finalOutputFile);
                return true;
            }
        }
        return false;
    }

    private File getDestinationFile() {
        File outputFile = new File(Navit.sMapFilenamePath, mMapValues.mMapName + ".tmp");
        outputFile.getParentFile().mkdir();
        return outputFile;
    }

    private boolean downloadData(URLConnection c, long alreadyRead, boolean resume, File outputFile, int subMapIndex) {
        boolean success = false;
        BufferedOutputStream buf = getOutputStream(outputFile, resume);
        BufferedInputStream bif = getInputStream(c, subMapIndex);

        if (buf != null && bif != null) {
            success = readData(buf, bif, alreadyRead, subMapIndex);
            // always cleanup, as we might get errors when trying to resume
            try {
                buf.flush();
                buf.close();

                bif.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return success;
    }

    private String getLatestDate() {
        if (this.mGitHubMetadata != "") {
            int ind = this.mGitHubMetadata.indexOf("/tarball/");
            return this.mGitHubMetadata.substring(ind + 9, ind + 19);
        } else {
                Log.e(TAG, "We failed to retrieve the date. ");
                return (String) "";
        }
    }

    private static long getEstSizeBytes(int mapId, int subMapIndex, String githubMetadata) {
            if (subMapIndex < osm_maps[mapId].mSubMaps.length) {
                int ind_dataset = githubMetadata.indexOf(osm_maps[mapId].mSubMaps[subMapIndex]);
                int ind_colon = githubMetadata.indexOf("size", ind_dataset) + 6;
                int ind_comma = githubMetadata.indexOf(",", ind_colon);
                return Math.max(Long.valueOf(githubMetadata.substring(ind_colon, ind_comma)), 0);
            } else {
                return 0;
            }
    }

    private long getMapSize(int mapId) {
        long size = 0;

        for (int subMapIndex = 0; subMapIndex < osm_maps[mapId].mSubMaps.length; subMapIndex++) {
            size += getEstSizeBytes(mapId, subMapIndex, this.mGitHubMetadata);
        }
        return size;
    }

    public static long getMapSize(int mapId, String githubMetadata) {
        long size = 0;

        for (int subMapIndex = 0; subMapIndex < osm_maps[mapId].mSubMaps.length; subMapIndex++) {
            size += getEstSizeBytes(mapId, subMapIndex, githubMetadata);
        }
        return size;
    }

    private URL getDownloadURL(int subMapIndex) {
        URL url=null;
        try {
	    String date = getLatestDate();
	    if (date != "") {
                    url =
                        new URL("https://github.com/navit-gps/gh-actions-mapserver/releases/download/" + date + "/"
                                + mMapValues.mSubMaps[subMapIndex] + "-" + date + ".bin");
                }
        } catch (MalformedURLException e) {
            Log.e(TAG, "We failed to create a URL to " + mMapValues.mMapName);
            e.printStackTrace();
            return null;
        }
        Log.v(TAG, "connect to " + url.toString());
        return url;
    }


    private BufferedInputStream getInputStream(URLConnection c, int subMapIndex) {
        BufferedInputStream bif;
        try {
            bif = new BufferedInputStream(c.getInputStream(), MAP_READ_FILE_BUFFER);
        } catch (FileNotFoundException e) {
            Log.e(TAG, "File not found on server: " + e);
            if (mRetryCounter > 0) {
                getMapInfoFile(subMapIndex).delete();
            }
            enableRetry();
            bif = null;
        } catch (IOException e) {
            Log.e(TAG, "Error reading from server: " + e);
            enableRetry();
            bif = null;
        }
        return bif;
    }

    private File getMapFile(int subMapIndex) {
        return new File(Navit.sMapFilenamePath, mMapValues.mSubMaps[subMapIndex] + ".bin");
    }

    private File getMapInfoFile(int subMapIndex) {
        return new File(Navit.sMapFilenamePath, mMapValues.mSubMaps[subMapIndex] + ".tmp.info");
    }

    private static BufferedOutputStream getOutputStream(File outputFile, boolean resume) {
        BufferedOutputStream buf;
        try {
            buf = new BufferedOutputStream(new FileOutputStream(outputFile, resume), MAP_WRITE_FILE_BUFFER);
        } catch (FileNotFoundException e) {
            Log.e(TAG, "Could not open output file for writing: " + e);
            buf = null;
        }
        return buf;
    }

    private URLConnection initConnection(URL url) {
        HttpURLConnection c;
        try {
            c = (HttpURLConnection) url.openConnection();
            c.setRequestMethod("GET");
        } catch (Exception e) {
            Log.e(TAG, "Failed connecting server: " + e);
            enableRetry();
            return null;
        }

        c.setReadTimeout(SOCKET_READ_TIMEOUT);
        c.setConnectTimeout(SOCKET_CONNECT_TIMEOUT);
        return c;
    }

    private boolean readData(OutputStream buf, InputStream bif, long alreadyRead,
                             int subMapIndex) {
        long startTimestamp = System.nanoTime();
        byte[] buffer = new byte[MAP_WRITE_MEM_BUFFER];
        int len1;
        long startOffset = alreadyRead;
        boolean success = false;


        long totalSize = getEstSizeBytes(this.mMapId, subMapIndex, this.mGitHubMetadata);
        if (totalSize==0) {
            return false;
        }

        try {
            while (!mStopMe && (len1 = bif.read(buffer)) != -1) {
                alreadyRead += len1;

                updateProgress(startTimestamp, startOffset, alreadyRead, totalSize);

                try {
                    buf.write(buffer, 0, len1);
                } catch (IOException e) {
                    Log.d(TAG, "Error: " + e);
                    if (!checkFreeSpace(totalSize - alreadyRead + MAP_WRITE_FILE_BUFFER)) {
                        if (deleteMap(subMapIndex)) {
                            enableRetry();
                        } else {
                            updateProgress(alreadyRead, totalSize,
                                           getTstring(R.string.map_download_download_error) + "\n"
                                           + getTstring(R.string.map_download_not_enough_free_space));
                        }
                    } else {
                        updateProgress(alreadyRead, totalSize,
                                       getTstring(R.string.map_download_error_writing_map));
                    }

                    return false;
                }
            }

            if (mStopMe) {
                //toast(getTstring(R.string.map_download_download_aborted));
            } else if (alreadyRead < totalSize) {
                Log.d(TAG, "Server send only " + alreadyRead + " bytes of " + totalSize);
                enableRetry();
            } else {
                success = true;
            }
        } catch (IOException e) {
            Log.d(TAG, "Error: " + e);

            enableRetry();
            updateProgress(alreadyRead, totalSize,
                           getTstring(R.string.map_download_download_error));
        }

        return success;
    }

    private URL readFileInfo(int subMapIndex) {
        URL url = null;
        try {
            ObjectInputStream infoStream = new ObjectInputStream(
                    new FileInputStream(getMapInfoFile(subMapIndex)));
            infoStream.readUTF(); // read the host name (unused for now)
            String resumeFile = infoStream.readUTF();
            infoStream.close();
            // looks like the same file, try to resume
            Log.v(TAG, "Try to resume download");

            url = new URL("https://" + "maps.navit-project.org" + resumeFile);
        } catch (Exception e) {
            getMapInfoFile(subMapIndex).delete();
        }
        return url;
    }

    private void toast(String message) {
        NavitDialogs.sendDialogMessage(NavitDialogs.MSG_TOAST, null, message, -1, 0, 0);
    }

    private void updateProgress(long startTime, long offsetBytes, long readBytes, long maxBytes) {
        long currentTime = System.nanoTime();

        if ((currentTime > mUiLastUpdated + UPDATE_PROGRESS_TIME_NS) && startTime != currentTime) {
            float perSecondOverall = (readBytes - offsetBytes) / ((currentTime - startTime) / 1000000000f);
            long bytesRemaining = maxBytes - readBytes;
            int etaSeconds = (int) (bytesRemaining / perSecondOverall);

            String etaString;
            if (etaSeconds > 60) {
                etaString = (int) (etaSeconds / 60f) + " m";
            } else {
                etaString = etaSeconds + " s";
            }
            String info = String.format("%s: %s\n %dMb / %dMb\n %.1f kb/s %s: %s",
                                        getTstring(R.string.map_downloading),
                                        mMapValues.mMapName, readBytes / 1024 / 1024, maxBytes / 1024 / 1024,
                                        perSecondOverall / 1024f, getTstring(R.string.map_download_eta),
                                        etaString);

            if (mRetryCounter > 0) {
                info += "\n Retry " + mRetryCounter + "/" + MAX_RETRIES;
            }
            Log.d(TAG, "info: " + info);

            updateProgress(readBytes, maxBytes, info);
            mUiLastUpdated = currentTime;
        }
    }

    private void updateProgress(long positionBytes, long maximumBytes, String infoText) {
        NavitDialogs.sendDialogMessage(NavitDialogs.MSG_PROGRESS_BAR,
                                       getTstring(R.string.map_download_title), infoText,
                                       NavitDialogs.DIALOG_MAPDOWNLOAD, (int) (maximumBytes / 1024),
                                       (int) (positionBytes / 1024));
    }

    private void writeFileInfo(URLConnection c, long sizeInBytes, int subMapIndex) {
        ObjectOutputStream infoStream;
        try {
            infoStream = new ObjectOutputStream(new FileOutputStream(getMapInfoFile(subMapIndex)));
            infoStream.writeUTF(c.getURL().getProtocol());
            infoStream.writeUTF(c.getURL().getHost());
            infoStream.writeUTF(c.getURL().getFile());
            infoStream.writeLong(sizeInBytes);
            infoStream.close();
        } catch (Exception e) {
            Log.e(TAG,
                    "Could not write info file for map download. Resuming will not be possible. ("
                    + e.getMessage() + ")");
        }
    }

    private void enableRetry() {
        mRetryDownload = true;
        mRetryCounter++;
    }


    static class OsmMapValues {

        String mLon1;
        String mLat1;
        String mLon2;
        String mLat2;
        final String mMapName;
	String [] mSubMaps;
        int mLevel;

        private void setMapValues(String lon1, String lat1, String lon2, String lat2, int level) {
            this.mLon1 = lon1;
            this.mLat1 = lat1;
            this.mLon2 = lon2;
            this.mLat2 = lat2;
            this.mLevel = level;
        }

        private OsmMapValues(int id1, String lon1, String lat1, String lon2, String lat2,
                             int level, String[] subMaps) {

            this.mMapName = getTstring(id1);
            setMapValues(lon1, lat1, lon2, lat2, level);
	    this.mSubMaps = subMaps;
        }

	private OsmMapValues(int id1, int id2, String lon1, String lat1, String lon2, String lat2,
                             int level, String[] subMaps) {

            this.mMapName = getTstring(id1) + " + " + getTstring(id2);
            setMapValues(lon1, lat1, lon2, lat2, level);
	    this.mSubMaps = subMaps;
        }


        boolean isInMap(Location location) {

            if (location.getLongitude() < Double.valueOf(this.mLon1)) {
                return false;
            }
            if (location.getLongitude() > Double.valueOf(this.mLon2)) {
                return false;
            }
            if (location.getLatitude() < Double.valueOf(this.mLat1)) {
                return false;
            }
            return !(location.getLatitude() > Double.valueOf(this.mLat2));
        }
    }
}

