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


/*
 * @author rikky
 *
 */
public class NavitMapDownloader extends Thread {

    static final OsmMapValues[] osm_maps = {

        // size estimations updated 2019-10-28
        new OsmMapValues(R.string.whole_planet, "-180", "-90", "180", "90",
                    35471336933L, 0),
        new OsmMapValues(R.string.africa, "-30.89", "-36.17", "61.68", "38.40",
                    3941558472L, 0),
        new OsmMapValues(R.string.angola, "11.4", "-18.1", "24.2", "-5.3",
                    224809554L, 1),
        new OsmMapValues(R.string.burundi, "28.9", "-4.5", "30.9", "-2.2",
                    201346208L, 1),
        new OsmMapValues(R.string.canary_islands, "-18.69", "26.52", "-12.79", "29.99",
                    191823973L, 1),
        new OsmMapValues(R.string.congo, "11.7", "-13.6", "31.5", "5.7",
                    558204116L, 1),
        new OsmMapValues(R.string.ethiopia, "32.89", "3.33", "48.07", "14.97",
                    249153700L, 1),
        new OsmMapValues(R.string.guinea, "-15.47", "7.12", "-7.58", "12.74",
                    290456408L, 1),
        new OsmMapValues(R.string.cotedivoire, "-8.72", "4.09", "-2.43", "10.80",
                    220210996L, 1),
        new OsmMapValues(R.string.kenya, "33.8", "-5.2", "42.4", "4.9",
                    402046406L, 1),
        new OsmMapValues(R.string.lesotho, "26.9", "-30.7", "29.6", "-28.4",
                    250430629L, 1),
        new OsmMapValues(R.string.liberia, "-15.00", "-0.73", "-7.20", "8.65",
                    220130148L, 1),
        new OsmMapValues(R.string.libya, "9.32", "19.40", "25.54", "33.63",
                    202766153L, 1),
        new OsmMapValues(R.string.madagascar, "42.25", "-26.63", "51.20", "-11.31",
                    231793632L, 1),
        new OsmMapValues(R.string.namibia, R.string.botswana, "11.4", "-29.1", "29.5", "-16.9",
                    355183133L, 1),
        new OsmMapValues(R.string.reunion, "55.2", "-21.4", "55.9", "-20.9",
                    170953086L, 1),
        new OsmMapValues(R.string.rwanda, "28.8", "-2.9", "30.9", "-1.0",
                    213213966L, 1),
        new OsmMapValues(R.string.south_africa, R.string.lesotho, "15.93", "-36.36", "33.65", "-22.08",
                    412557227L, 1),
        new OsmMapValues(R.string.tanzania, "29.19", "-11.87", "40.74", "-0.88",
                    659878114L, 1),
        new OsmMapValues(R.string.uganda, "29.3", "-1.6", "35.1", "4.3",
                    424948047L, 1),
        new OsmMapValues(R.string.asia, "23.8", "0.1", "195.0", "82.4",
                    8633755849L, 0),
        new OsmMapValues(R.string.azerbaijan, "44.74", "38.34", "51.69", "42.37",
                    208193716L, 1),
        new OsmMapValues(R.string.china, "67.3", "5.3", "135.0", "54.5",
                    3071225422L, 1),
        new OsmMapValues(R.string.cyprus, "32.0", "34.5", "34.9", "35.8",
                    178895753L, 1),
        new OsmMapValues(R.string.india, R.string.nepal, "67.9", "5.5", "89.6", "36.0",
                    1046746656L, 1),
        new OsmMapValues(R.string.indonesia, "93.7", "-17.3", "155.5", "7.6",
                    1182281967L, 1),
        new OsmMapValues(R.string.iran, "43.5", "24.4", "63.6", "40.4",
                    391459887L, 1),
        new OsmMapValues(R.string.iraq, "38.7", "28.5", "49.2", "37.4",
                    244896621L, 1),
        new OsmMapValues(R.string.israel, "33.99", "29.8", "35.95", "33.4",
                    236037295L, 1),
        new OsmMapValues(R.string.japan, R.string.korea, "123.6", "25.2", "151.3", "47.1",
                    1337474060L, 1),
        new OsmMapValues(R.string.kazakhstan, "46.44", "40.89", "87.36", "55.45",
                    661252271L, 1),
        new OsmMapValues(R.string.kyrgyzsyan, "69.23", "39.13", "80.33", "43.29",
                    215786962L, 1),
        new OsmMapValues(R.string.malaysia, R.string.singapore, "94.3", "-5.9", "108.6", "6.8",
                    339511849L, 1),
        new OsmMapValues(R.string.mongolia, "87.5", "41.4", "120.3", "52.7",
                    231652214L, 1),
        new OsmMapValues(R.string.pakistan, "60.83", "23.28", "77.89", "37.15",
                    316649335L, 1),
        new OsmMapValues(R.string.philippines, "115.58", "4.47", "127.85", "21.60",
                    417733117L, 1),
        new OsmMapValues(R.string.saudi_arabia, "33.2", "16.1", "55.9", "33.5",
                    374101149L, 1),
        new OsmMapValues(R.string.taiwan, "119.1", "21.5", "122.5", "25.2",
                    213310839L, 1),
        new OsmMapValues(R.string.thailand, "97.5", "5.7", "105.2", "19.7",
                    345327944L, 1),
        new OsmMapValues(R.string.turkey, "25.1", "35.8", "46.4", "42.8",
                    547969022L, 1),
        new OsmMapValues(R.string.turkmenistan, "51.78", "35.07", "66.76", "42.91",
                    197833565L, 1),
        new OsmMapValues(R.string.uae_other, "51.5", "22.6", "56.7", "26.5",
                    188885191L, 1),
        new OsmMapValues(R.string.australia, R.string.oceania, "89.84", "-57.39", "179.79", "7.26",
                    1767156679L, 0),
        new OsmMapValues(R.string.australia, "110.5", "-44.2", "154.9", "-9.2",
                    550526915L, 0),
        new OsmMapValues(R.string.tasmania, "144.0", "-45.1", "155.3", "-24.8",
                    377084650L, 1),
        new OsmMapValues(R.string.victoria, R.string.new_south_wales, "140.7", "-39.4", "153.7", "-26.9",
                    364316999L, 1),
        new OsmMapValues(R.string.new_caledonia, "157.85", "-25.05", "174.15", "-16.85",
                    164225513L, 1),
        new OsmMapValues(R.string.newzealand, "165.2", "-47.6", "179.1", "-33.7",
                    350750954L, 1),
        new OsmMapValues(R.string.europe, "-12.97", "33.59", "34.15", "72.10",
                    15902268761L, 0),
        // Is more than 'Europe' from above where 'western europe' should be smaller than europe
        //new OsmMapValues(R.string.western_europe, "-17.6", "34.5", "42.9", "70.9",
        //           16879410713L, 1),
        new OsmMapValues(R.string.austria, "9.4", "46.32", "17.21", "49.1",
                    1184302570L, 1),
        new OsmMapValues(R.string.azores, "-31.62", "36.63", "-24.67", "40.13",
                    171415530L, 1),
        new OsmMapValues(R.string.belgium, "2.3", "49.5", "6.5", "51.6",
                    866194492L, 1),
        new OsmMapValues(R.string.benelux, "2.08", "48.87", "7.78", "54.52",
                    2075973265L, 1),
        new OsmMapValues(R.string.netherlands, "3.07", "50.75", "7.23", "53.73",
                    1388530557L, 1),
        new OsmMapValues(R.string.denmark, "7.65", "54.32", "15.58", "58.07",
                    552080821L, 1),
        new OsmMapValues(R.string.faroe_islands, "-7.8", "61.3", "-6.1", "62.5",
                    162933852L, 1),
        new OsmMapValues(R.string.france, R.string.belgium, R.string.luxembourg,
                    "-5.20", "42.20", "8.20", "51.68",
                    4575442134L, 1),
        new OsmMapValues(R.string.france, R.string.luxembourg,"-5.20", "42.20", "8.20", "51.10",
                    3984216361L, 1),
        new OsmMapValues(R.string.alsace, "6.79", "47.27", "8.48", "49.17",
                    457980795L, 2),
        new OsmMapValues(R.string.aquitaine, "-2.27", "42.44", "1.50", "45.76",
                    590247901L, 2),
        new OsmMapValues(R.string.auvergne, "2.01", "44.57", "4.54", "46.85",
                    404857899L, 2),
        new OsmMapValues(R.string.basse_normandie, "-2.09", "48.13", "1.03", "49.98",
                    350392952L, 2),
        new OsmMapValues(R.string.bourgogne, "2.80", "46.11", "5.58", "48.45",
                    429259997L, 2),
        new OsmMapValues(R.string.bretagne, "-5.58", "46.95", "-0.96", "48.99",
                    506377685L, 2),
        new OsmMapValues(R.string.centre, "0.01", "46.29", "3.18", "48.99",
                    620373774L, 2),
        new OsmMapValues(R.string.champagne_ardenne, "3.34", "47.53", "5.94", "50.28",
                    404110051L, 2),
        new OsmMapValues(R.string.corse, "8.12", "41.32", "9.95", "43.28",
                    196000318L, 2),
        new OsmMapValues(R.string.franche_comte, "5.20", "46.21", "7.83", "48.07",
                    448854594L, 2),
        new OsmMapValues(R.string.haute_normandie, "-0.15", "48.62", "1.85", "50.18",
                    291794192L, 2),
        new OsmMapValues(R.string.ile_de_france, "1.40", "48.07", "3.61", "49.29",
                    404250888L, 2),
        new OsmMapValues(R.string.languedoc_roussillon, "1.53", "42.25", "4.89", "45.02",
                    509739604L, 2),
        new OsmMapValues(R.string.limousin, "0.58", "44.87", "2.66", "46.50",
                    315805128L, 2),
        new OsmMapValues(R.string.lorraine, "4.84", "47.77", "7.72", "49.73",
                    433828147L, 2),
        new OsmMapValues(R.string.midi_pyrenees, "-0.37", "42.18", "3.50", "45.10",
                    609387039L, 2),
        new OsmMapValues(R.string.nord_pas_de_calais, "1.42", "49.92", "4.49", "51.31",
                    480042491L, 2),
        new OsmMapValues(R.string.pays_de_la_loire, "-2.88", "46.20", "0.97", "48.62",
                    638031978L, 2),
        new OsmMapValues(R.string.picardie, "1.25", "48.79", "4.31", "50.43",
                    480802771L, 2),
        new OsmMapValues(R.string.poitou_charentes, "-1.69", "45.04", "1.26", "47.23",
                    459723694L, 2),
        new OsmMapValues(R.string.provence_alpes_cote_d_azur, "4.21", "42.91", "7.99", "45.18",
                    491232080L, 2),
        new OsmMapValues(R.string.rhone_alpes, "3.65", "44.07", "7.88", "46.64",
                    656454467L, 2),
        new OsmMapValues(R.string.luxembourg, "5.7", "49.4", "6.5", "50.2",
                    207870816L, 1),
        new OsmMapValues(R.string.germany, "5.18", "46.84", "15.47", "55.64",
                    4258366162L, 1),
        new OsmMapValues(R.string.baden_wuerttemberg, "7.32", "47.14", "10.57", "49.85",
                    859363389L, 2),
        new OsmMapValues(R.string.bayern, "8.92", "47.22", "13.90", "50.62",
                    1089590692L, 2),
        new OsmMapValues(R.string.mittelfranken, "9.86", "48.78", "11.65", "49.84",
                    278839689L, 2),
        new OsmMapValues(R.string.niederbayern, "11.55", "47.75", "14.12", "49.42",
                    409234523L, 2),
        new OsmMapValues(R.string.oberbayern, "10.67", "47.05", "13.57", "49.14",
                    494705080L, 2),
        new OsmMapValues(R.string.oberfranken, "10.31", "49.54", "12.49", "50.95",
                    320127656L, 2),
        new OsmMapValues(R.string.oberpfalz, "11.14", "48.71", "13.47", "50.43",
                    345835711L, 2),
        new OsmMapValues(R.string.schwaben, "9.27", "47.10", "11.36", "49.09",
                    425231381L, 2),
        new OsmMapValues(R.string.unterfranken, "8.59", "49.16", "10.93", "50.67",
                    407153157L, 2),
        new OsmMapValues(R.string.berlin, "13.03", "52.28", "13.81", "52.73",
                    232299113L, 2),
        new OsmMapValues(R.string.brandenburg, "11.17", "51.30", "14.83", "53.63",
                    429938728L, 2),
        new OsmMapValues(R.string.bremen, "8.43", "52.96", "9.04", "53.66",
                    210480663L, 2),
        new OsmMapValues(R.string.hamburg, "9.56", "53.34", "10.39", "53.80",
                    215777707L, 2),
        new OsmMapValues(R.string.hessen, "7.72", "49.34", "10.29", "51.71",
                    549304606L, 2),
        new OsmMapValues(R.string.mecklenburg_vorpommern, "10.54", "53.05", "14.48", "55.05",
                    298273965L, 2),
        new OsmMapValues(R.string.niedersachsen, "6.40", "51.24", "11.69", "54.22",
                    973888436L, 2),
        new OsmMapValues(R.string.nordrhein_westfalen, "5.46", "50.26", "9.52", "52.59",
                    1121091906L, 2),
        new OsmMapValues(R.string.rheinland_pfalz, "6.06", "48.91", "8.56", "51.00",
                    562406978L, 2),
        new OsmMapValues(R.string.saarland, "6.30", "49.06", "7.46", "49.69",
                    223614951L, 2),
        new OsmMapValues(R.string.sachsen_anhalt, "10.50", "50.88", "13.26", "53.11",
                    380652800L, 2),
        new OsmMapValues(R.string.sachsen, "11.82", "50.11", "15.10", "51.73",
                    452296960L, 2),
        new OsmMapValues(R.string.schleswig_holstein, "7.41", "53.30", "11.98", "55.20",
                    375552150L, 2),
        new OsmMapValues(R.string.thueringen, "9.81", "50.15", "12.72", "51.70",
                    362501525L, 2),
        new OsmMapValues(R.string.iceland, "-25.3", "62.8", "-11.4", "67.5",
                    198263647L, 1),
        new OsmMapValues(R.string.ireland, "-11.17", "51.25", "-5.23", "55.9",
                    326472032L, 1),
        new OsmMapValues(R.string.italy, "6.52", "36.38", "18.96", "47.19",
                    2199756999L, 1),
        new OsmMapValues(R.string.spain, R.string.portugal, "-11.04", "34.87", "4.62", "44.41",
                    1442482175L, 1),
        new OsmMapValues(R.string.mallorca, "2.2", "38.8", "4.7", "40.2",
                    246286923L, 2),
        new OsmMapValues(R.string.galicia, "-10.0", "41.7", "-6.3", "44.1",
                    269729717L, 2),
        new OsmMapValues(R.string.scandinavia, "4.0", "54.4", "32.1", "71.5",
                    2545082361L, 1),
        new OsmMapValues(R.string.finland, "18.6", "59.2", "32.3", "70.3",
                    919020354L, 1),
        new OsmMapValues(R.string.denmark, "7.49", "54.33", "13.05", "57.88",
                    488352770L, 1),
        new OsmMapValues(R.string.switzerland, "5.79", "45.74", "10.59", "47.84",
                    731538216L, 1),
        new OsmMapValues(R.string.united_kingdom, "-9.7", "49.6", "2.2", "61.2",
                    1205714793L, 1),
        new OsmMapValues(R.string.england, "-7.80", "48.93", "2.41", "56.14",
                    1234829532L, 1),
        new OsmMapValues(R.string.buckinghamshire, "-1.19", "51.44", "-0.43", "52.25",
                    216717402L, 2),
        new OsmMapValues(R.string.cambridgeshire, "-0.55", "51.96", "0.56", "52.79",
                    216306889L, 2),
        new OsmMapValues(R.string.cumbria, "-3.96", "53.85", "-2.11", "55.24",
                    227066588L, 2),
        new OsmMapValues(R.string.east_yorkshire_with_hull, "-1.16", "53.50", "0.54", "54.26",
                    214532411L, 2),
        new OsmMapValues(R.string.essex, "-0.07", "51.40", "1.36", "52.14",
                    242803228L, 2),
        new OsmMapValues(R.string.herefordshire, "-3.19", "51.78", "-2.29", "52.45",
                    201746914L, 2),
        new OsmMapValues(R.string.kent, "-0.02", "50.81", "1.65", "51.53",
                    222224232L, 2),
        new OsmMapValues(R.string.lancashire, "-3.20", "53.43", "-2.00", "54.29",
                    231719236L, 2),
        new OsmMapValues(R.string.leicestershire, "-1.65", "52.34", "-0.61", "53.03",
                    231608922L, 2),
        new OsmMapValues(R.string.norfolk, "0.10", "52.30", "2.04", "53.41",
                    220583805L, 2),
        new OsmMapValues(R.string.nottinghamshire, "-1.39", "52.73", "-0.62", "53.55",
                    222001309L, 2),
        new OsmMapValues(R.string.oxfordshire, "-1.77", "51.41", "-0.82", "52.22",
                    215858382L, 2),
        new OsmMapValues(R.string.shropshire, "-3.29", "52.26", "-2.18", "53.05",
                    210489688L, 2),
        new OsmMapValues(R.string.somerset, "-3.89", "50.77", "-2.20", "51.40",
                    221498821L, 2),
        new OsmMapValues(R.string.south_yorkshire, "-1.88", "53.25", "-0.80", "53.71",
                    217566765L, 2),
        new OsmMapValues(R.string.suffolk, "0.29", "51.88", "1.81", "52.60",
                    218109848L, 2),
        new OsmMapValues(R.string.surrey, "-0.90", "51.02", "0.10", "51.52",
                    238058353L, 2),
        new OsmMapValues(R.string.wiltshire, "-2.41", "50.90", "-1.44", "51.76",
                    212963156L, 2),
        new OsmMapValues(R.string.scotland, "-8.13", "54.49", "-0.15", "61.40",
                    372486133L, 2),
        new OsmMapValues(R.string.wales, "-5.56", "51.28", "-2.60", "53.60",
                    281554024L, 2),
        new OsmMapValues(R.string.albania, "19.09", "39.55", "21.12", "42.72",
                    227718134L, 1),
        new OsmMapValues(R.string.belarus, "23.12", "51.21", "32.87", "56.23",
                    475885189L, 1),
        new OsmMapValues(R.string.russian_federation, "27.9", "41.5", "190.4", "77.6",
                    3368130201L, 1),
        new OsmMapValues(R.string.bulgaria, "24.7", "42.1", "24.8", "42.1",
                    162974147L, 1),
        new OsmMapValues(R.string.bosnia_and_herzegovina, "15.69", "42.52", "19.67", "45.32",
                    272871917L, 1),
        new OsmMapValues(R.string.czech_republic, "11.91", "48.48", "19.02", "51.17",
                    1100792845L, 1),
        new OsmMapValues(R.string.croatia, "13.4", "42.1", "19.4", "46.9",
                    718506859L, 1),
        new OsmMapValues(R.string.estonia, "21.5", "57.5", "28.2", "59.6",
                    239147853L, 1),
        new OsmMapValues(R.string.greece, "28.9", "37.8", "29.0", "37.8",
                    165328625L, 1),
        new OsmMapValues(R.string.crete, "23.3", "34.5", "26.8", "36.0",
                    174905198L, 1),
        new OsmMapValues(R.string.hungary, "16.08", "45.57", "23.03", "48.39",
                    499823508L, 1),
        new OsmMapValues(R.string.latvia, "20.7", "55.6", "28.3", "58.1",
                    272931650L, 1),
        new OsmMapValues(R.string.lithuania, "20.9", "53.8", "26.9", "56.5",
                    332328437L, 1),
        new OsmMapValues(R.string.poland, "13.6", "48.8", "24.5", "55.0",
                    1849496038L, 1),
        new OsmMapValues(R.string.romania, "20.3", "43.5", "29.9", "48.4",
                    497623904L, 1),
        new OsmMapValues(R.string.slovakia, "16.8", "47.7", "22.6", "49.7",
                    518251489L, 1),
        new OsmMapValues(R.string.ukraine, "22.0", "44.3", "40.4", "52.4",
                    1105659895L, 1),
        new OsmMapValues(R.string.north_america, "-178.1", "6.5", "-10.4", "84.0",
                    7727821782L, 0),
        new OsmMapValues(R.string.alaska, "-179.5", "49.5", "-129", "71.6",
                    309103093L, 1),
        new OsmMapValues(R.string.canada, "-141.3", "41.5", "-52.2", "70.2",
                    3564630901L, 1),
        new OsmMapValues(R.string.hawaii, "-161.07", "18.49", "-154.45", "22.85",
                    170132156L, 1),
        new OsmMapValues(R.string.usa, R.string.except_alaska_and_hawaii, "-125.4", "24.3", "-66.5", "49.3",
                    5533181627L, 1),
        new OsmMapValues(R.string.midwest, "-104.11", "35.92", "-80.46", "49.46",
                    1565753619L, 2),
        new OsmMapValues(R.string.michigan, "-90.47", "41.64", "-79.00", "49.37",
                    794434442L, 2),
        new OsmMapValues(R.string.ohio, "-84.87", "38.05", "-79.85", "43.53",
                    428282496L, 2),
        new OsmMapValues(R.string.northeast, "-80.58", "38.72", "-66.83", "47.53",
                    1391521085L, 2),
        new OsmMapValues(R.string.massachusetts, "-73.56", "40.78", "-68.67", "42.94",
                    438012615L, 2),
        new OsmMapValues(R.string.vermont, "-73.49", "42.68", "-71.41", "45.07",
                    206957907L, 2),
        new OsmMapValues(R.string.pacific, "-180.05", "15.87", "-129.75", "73.04",
                    309809468L, 2),
        new OsmMapValues(R.string.south, "-106.70", "23.98", "-71.46", "40.70",
                    2466350986L, 2),
        new OsmMapValues(R.string.arkansas, "-94.67", "32.95", "-89.59", "36.60",
                    223752575L, 2),
        new OsmMapValues(R.string.district_of_columbia, "-77.17", "38.74", "-76.86", "39.05",
                    201055647L, 2),
        new OsmMapValues(R.string.florida, "-88.75", "23.63", "-77.67", "31.05",
                    372359367L, 2),
        new OsmMapValues(R.string.louisiana, "-94.09", "28.09", "-88.62", "33.07",
                    290673534L, 2),
        new OsmMapValues(R.string.maryland, "-79.54", "37.83", "-74.99", "40.22",
                    409820479L, 2),
        new OsmMapValues(R.string.mississippi, "-91.71", "29.99", "-88.04", "35.05",
                    246036091L, 2),
        new OsmMapValues(R.string.oklahoma, "-103.41", "33.56", "-94.38", "37.38",
                   265042769L, 2),
        new OsmMapValues(R.string.texas, "-106.96", "25.62", "-92.97", "36.58",
                    625570538L, 2),
        new OsmMapValues(R.string.virginia, "-83.73", "36.49", "-74.25", "39.52",
                    546119530L, 2),
        new OsmMapValues(R.string.west_virginia, "-82.70", "37.15", "-77.66", "40.97",
                    319698480L, 2),
        new OsmMapValues(R.string.west, "-133.11", "31.28", "-101.99", "49.51",
                    1501540635L, 2),
        new OsmMapValues(R.string.arizona, "-114.88", "30.01", "-108.99", "37.06",
                    276838815L, 2),
        new OsmMapValues(R.string.california, "-125.94", "32.43", "-114.08", "42.07",
                    725286705L, 2),
        new OsmMapValues(R.string.colorado, "-109.11", "36.52", "-100.41", "41.05",
                    328730867L, 2),
        new OsmMapValues(R.string.idaho, "-117.30", "41.93", "-110.99", "49.18",
                    241725898L, 2),
        new OsmMapValues(R.string.montana, "-116.10", "44.31", "-102.64", "49.74",
                    246850772L, 2),
        new OsmMapValues(R.string.new_mexico, "-109.10", "26.98", "-96.07", "37.05",
                    542205524L, 2),
        new OsmMapValues(R.string.nevada, "-120.2", "35.0", "-113.8", "42.1",
                    269630259L, 2),
        new OsmMapValues(R.string.oregon, "-124.8", "41.8", "-116.3", "46.3",
                    290552225L, 2),
        new OsmMapValues(R.string.utah, "-114.11", "36.95", "-108.99", "42.05",
                    222549404L, 2),
        new OsmMapValues(R.string.washington_state, "-125.0", "45.5", "-116.9", "49.0",
                    323577562L, 2),
        new OsmMapValues(R.string.south_middle_america, "-83.5", "-56.3", "-30.8", "13.7",
                    1803470489L, 0),
        new OsmMapValues(R.string.argentina, "-73.9", "-57.3", "-51.6", "-21.0",
                    640249385L, 1),
        new OsmMapValues(R.string.argentina, R.string.chile, "-77.2", "-56.3", "-52.7", "-16.1",
                    713424629L, 1),
        new OsmMapValues(R.string.bolivia, "-70.5", "-23.1", "-57.3", "-9.3",
                    257531768L, 1),
        new OsmMapValues(R.string.brazil, "-71.4", "-34.7", "-32.8", "5.4",
                    1275564517L, 1),
        new OsmMapValues(R.string.chile, "-81.77", "-58.50", "-65.46", "-17.41",
                    400742681L, 1),
        new OsmMapValues(R.string.cuba, "-85.3", "19.6", "-74.0", "23.6",
                    190849399L, 1),
        new OsmMapValues(R.string.colombia, "-79.1", "-4.0", "-66.7", "12.6",
                    337606776L, 1),
        new OsmMapValues(R.string.ecuador, "-82.6", "-5.4", "-74.4", "2.3",
                    231183832L, 1),
        new OsmMapValues(R.string.guyana, R.string.suriname, R.string.guyane_francaise,
                    "-62.0", "1.0", "-51.2", "8.9",
                    123000072L, 1),
        new OsmMapValues(R.string.haiti, R.string.dominican_republic, "-74.8", "17.3", "-68.2", "20.1",
                    213013324L, 1),
        new OsmMapValues(R.string.jamaica, "-78.6", "17.4", "-75.9", "18.9",
                    168748968L, 1),
        new OsmMapValues(R.string.mexico, "-117.6", "14.1", "-86.4", "32.8",
                    829539696L, 1),
        new OsmMapValues(R.string.paraguay, "-63.8", "-28.1", "-53.6", "-18.8",
                    262841159L, 1),
        new OsmMapValues(R.string.peru, "-82.4", "-18.1", "-67.5", "0.4",
                    328138464L, 1),
        new OsmMapValues(R.string.uruguay, "-59.2", "-36.5", "-51.7", "-29.7",
                    227796557L, 1),
        new OsmMapValues(R.string.venezuela, "-73.6", "0.4", "-59.7", "12.8",
                    262760703L, 1)
    };
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
    private Boolean mStopMe = false;
    private long mUiLastUpdated = -1;
    private Boolean mRetryDownload = false; //Download failed, but
    private int mRetryCounter = 0;

    NavitMapDownloader(int mapId) {
        this.mMapValues = osm_maps[mapId];
        this.mMapId = mapId;
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
        mStopMe = false;
        mRetryCounter = 0;

        Log.v(TAG, "start download " + mMapValues.mMapName);
        updateProgress(0, mMapValues.mEstSizeBytes,
                getTstring(R.string.map_downloading) + ": " + mMapValues.mMapName);

        boolean success;
        do {
            try {
                Thread.sleep(10 + mRetryCounter * 1000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            mRetryDownload = false;
            success = download_osm_map();
        } while (!success
                && mRetryDownload
                && mRetryCounter < MAX_RETRIES
                && !mStopMe);

        if (success) {
            toast(mMapValues.mMapName + " " + getTstring(R.string.map_download_ready));
            getMapInfoFile().delete();
            Log.d(TAG, "success");
        }

        if (success || mStopMe) {
            NavitDialogs.sendDialogMessage(NavitDialogs.MSG_MAP_DOWNLOAD_FINISHED,
                    Navit.sMapFilenamePath + mMapValues.mMapName + ".bin", null, -1, success ? 1 : 0, mMapId);
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
            Log.e(TAG, "Not enough free space or media not available. Please free at least "
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

    private boolean deleteMap() {
        File finalOutputFile = getMapFile();

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


    private boolean download_osm_map() {
        long alreadyRead = 0;
        long realSizeBytes;
        boolean resume = true;

        File outputFile = getDestinationFile();
        long oldDownloadSize = outputFile.length();

        URL url = null;
        if (oldDownloadSize > 0) {
            url = readFileInfo();
        }

        if (url == null) {
            resume = false;
            url = getDownloadURL();
        }

        // URL url = new URL("http://192.168.2.101:8080/zweibruecken.bin");
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
                writeFileInfo(c, realSizeBytes);
            }

            if (realSizeBytes <= 0) {
                realSizeBytes = mMapValues.mEstSizeBytes;
            }

            Log.d(TAG, "size: " + realSizeBytes + ", read: " + alreadyRead + ", timestamp: "
                    + fileTime
                    + ", Connection ref: " + c.getURL());

            if (checkFreeSpace(realSizeBytes - alreadyRead)
                    && downloadData(c, alreadyRead, realSizeBytes, resume, outputFile)) {

                File finalOutputFile = getMapFile();
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

    private boolean downloadData(URLConnection c, long alreadyRead, long realSizeBytes, boolean resume,
            File outputFile) {
        boolean success = false;
        BufferedOutputStream buf = getOutputStream(outputFile, resume);
        BufferedInputStream bif = getInputStream(c);

        if (buf != null && bif != null) {
            success = readData(buf, bif, alreadyRead, realSizeBytes);
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

    private URL getDownloadURL() {
        URL url;
        try {
            url =
                new URL("http://maps.navit-project.org/api/map/?bbox=" + mMapValues.mLon1 + ","
                        + mMapValues.mLat1
                        + "," + mMapValues.mLon2 + "," + mMapValues.mLat2);
        } catch (MalformedURLException e) {
            Log.e(TAG, "We failed to create a URL to " + mMapValues.mMapName);
            e.printStackTrace();
            return null;
        }
        Log.v(TAG, "connect to " + url.toString());
        return url;
    }


    private BufferedInputStream getInputStream(URLConnection c) {
        BufferedInputStream bif;
        try {
            bif = new BufferedInputStream(c.getInputStream(), MAP_READ_FILE_BUFFER);
        } catch (FileNotFoundException e) {
            Log.e(TAG, "File not found on server: " + e);
            if (mRetryCounter > 0) {
                getMapInfoFile().delete();
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

    private File getMapFile() {
        return new File(Navit.sMapFilenamePath, mMapValues.mMapName + ".bin");
    }

    private File getMapInfoFile() {
        return new File(Navit.sMapFilenamePath, mMapValues.mMapName + ".tmp.info");
    }

    private BufferedOutputStream getOutputStream(File outputFile, boolean resume) {
        BufferedOutputStream buf;
        try {
            buf = new BufferedOutputStream(new FileOutputStream(outputFile, resume),
                    MAP_WRITE_FILE_BUFFER);
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
            long realSizeBytes) {
        long startTimestamp = System.nanoTime();
        byte[] buffer = new byte[MAP_WRITE_MEM_BUFFER];
        int len1;
        long startOffset = alreadyRead;
        boolean success = false;

        try {
            while (!mStopMe && (len1 = bif.read(buffer)) != -1) {
                alreadyRead += len1;
                updateProgress(startTimestamp, startOffset, alreadyRead, realSizeBytes);

                try {
                    buf.write(buffer, 0, len1);
                } catch (IOException e) {
                    Log.d(TAG, "Error: " + e);
                    if (!checkFreeSpace(realSizeBytes - alreadyRead + MAP_WRITE_FILE_BUFFER)) {
                        if (deleteMap()) {
                            enableRetry();
                        } else {
                            updateProgress(alreadyRead, realSizeBytes,
                                    getTstring(R.string.map_download_download_error) + "\n"
                                    + getTstring(R.string.map_download_not_enough_free_space));
                        }
                    } else {
                        updateProgress(alreadyRead, realSizeBytes,
                                getTstring(R.string.map_download_error_writing_map));
                    }

                    return false;
                }
            }

            if (mStopMe) {
                toast(getTstring(R.string.map_download_download_aborted));
            } else if (alreadyRead < realSizeBytes) {
                Log.d(TAG, "Server send only " + alreadyRead + " bytes of " + realSizeBytes);
                enableRetry();
            } else {
                success = true;
            }
        } catch (IOException e) {
            Log.d(TAG, "Error: " + e);

            enableRetry();
            updateProgress(alreadyRead, realSizeBytes,
                    getTstring(R.string.map_download_download_error));
        }

        return success;
    }

    private URL readFileInfo() {
        URL url = null;
        try {
            ObjectInputStream infoStream = new ObjectInputStream(
                    new FileInputStream(getMapInfoFile()));
            infoStream.readUTF(); // read the host name (unused for now)
            String resumeFile = infoStream.readUTF();
            infoStream.close();
            // looks like the same file, try to resume
            Log.v(TAG, "Try to resume download");
            String resumeProto = infoStream.readUTF();
            url = new URL(resumeProto + "://" + "maps.navit-project.org" + resumeFile);
        } catch (Exception e) {
            getMapInfoFile().delete();
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

    private void writeFileInfo(URLConnection c, long sizeInBytes) {
        ObjectOutputStream infoStream;
        try {
            infoStream = new ObjectOutputStream(new FileOutputStream(getMapInfoFile()));
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
        long mEstSizeBytes;
        int mLevel;

        private void setMapValues(String lon1, String lat1, String lon2, String lat2, long bytesEst, int level) {
            this.mLon1 = lon1;
            this.mLat1 = lat1;
            this.mLon2 = lon2;
            this.mLat2 = lat2;
            this.mEstSizeBytes = bytesEst;
            this.mLevel = level;
        }

        private OsmMapValues(int id1, String lon1, String lat1, String lon2, String lat2,
                             long bytesEst, int level) {

            this.mMapName = getTstring(id1);
            setMapValues(lon1, lat1, lon2, lat2, bytesEst, level);
        }

        private OsmMapValues(int id1, int id2, String lon1, String lat1, String lon2, String lat2,
                             long bytesEst, int level) {

            this.mMapName = getTstring(id1) + " + " + getTstring(id2);
            setMapValues(lon1, lat1, lon2, lat2, bytesEst, level);
        }


        private OsmMapValues(int id1, int id2, int id3, String lon1, String lat1, String lon2, String lat2,
                             long bytesEst, int level) {

            mMapName = getTstring(id1) + " + " + getTstring(id2) + " + " + getTstring(id3);
            setMapValues(lon1, lat1, lon2, lat2, bytesEst, level);
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
