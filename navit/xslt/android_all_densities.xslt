<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
        <xsl:output method="xml" doctype-system="navit.dtd" cdata-section-elements="gui"/>
		<xsl:variable name="OSD_FACTOR_">
			<xsl:choose>
				<xsl:when test="string-length($OSD_FACTOR)"><xsl:value-of select="$OSD_FACTOR"/></xsl:when>
				<xsl:otherwise>1</xsl:otherwise>
			</xsl:choose>
		</xsl:variable>
		<xsl:variable name="CAR_FACTOR_">
			<xsl:choose>
				<xsl:when test="string-length($CAR_FACTOR)"><xsl:value-of select="$CAR_FACTOR"/></xsl:when>
				<xsl:otherwise>1</xsl:otherwise>
			</xsl:choose>
		</xsl:variable>
	<xsl:include href="default_plugins.xslt"/>
	<!--<xsl:include href="map_sdcard_navitmap_bin.xslt"/>-->
	<xsl:include href="map_secondary_sdcard_navitmap_bin.xslt"/>
	<xsl:include href="osd_android_minimum.xslt"/>
        <xsl:template match="/config/plugins/plugin[1]" priority="100">
		<plugin path="$NAVIT_PREFIX/lib/libgraphics_android.so" ondemand="no"/>
		<xsl:text>&#x0A;        </xsl:text>
		<plugin path="$NAVIT_PREFIX/lib/libvehicle_android.so" ondemand="no"/>
		<xsl:text>&#x0A;        </xsl:text>
		<plugin path="$NAVIT_PREFIX/lib/libspeech_android.so" ondemand="no"/>
		<xsl:text>&#x0A;        </xsl:text>
		<xsl:next-match/>
        </xsl:template>
        <xsl:template match="/config/navit/graphics">
                <graphics type="android" />
        </xsl:template>
	<!-- make gui fonts bigger -->
        <xsl:template match="/config/navit/gui[2]">
                <xsl:copy><xsl:copy-of select="@*[not(name()='font_size')]"/>
			<xsl:attribute name="font_size"><xsl:value-of select="$MENU_VALUE_FONTSIZE"/></xsl:attribute>
			<xsl:attribute name="icon_xs"><xsl:value-of select="$MENU_VALUE_ICONS_XS"/></xsl:attribute>
			<xsl:attribute name="icon_s"><xsl:value-of select="$MENU_VALUE_ICONS_S"/></xsl:attribute>
			<xsl:attribute name="icon_l"><xsl:value-of select="$MENU_VALUE_ICONS_L"/></xsl:attribute>
			<xsl:attribute name="spacing"><xsl:value-of select="$MENU_VALUE_SPACING"/></xsl:attribute>
		<xsl:apply-templates/></xsl:copy>
	</xsl:template>
	<!-- make gui fonts bigger -->
	<!-- after map drag jump to position, initial zoom -->
        <xsl:template match="/config/navit[1]">
                <xsl:copy><xsl:copy-of select="@*"/>
			<!--<xsl:attribute name="timeout">0</xsl:attribute>-->
			<xsl:attribute name="zoom">32</xsl:attribute>
		<xsl:apply-templates/></xsl:copy>
	</xsl:template>
	<!-- after map drag jump to position, initial zoom -->
        <xsl:template match="/config/navit/vehicle[1]">
                <xsl:copy><xsl:copy-of select="@*[not(name()='gpsd_query')]"/>
			<xsl:attribute name="source">android:</xsl:attribute>
			<xsl:attribute name="follow">1</xsl:attribute>
		<xsl:apply-templates/></xsl:copy>
	</xsl:template>
	<!-- add new default look-and-feel for android -->
        <xsl:template match="/config/navit/layout[1]">
<layout name="Android-Car" color="#fef9ee" font="Liberation Sans">
		<xsl:text>&#x0A;        </xsl:text>
        <cursor w="{round(30*number($CAR_FACTOR_)*number($OSD_FACTOR_))}" h="{round(32*number($CAR_FACTOR_)*number($OSD_FACTOR_))}">
		<xsl:text>&#x0A;        </xsl:text>
                <itemgra speed_range="-2">
                        <polyline color="#00BC00" radius="{round(0*number($CAR_FACTOR_)*number($OSD_FACTOR_))}" width="{round(3*number($CAR_FACTOR_)*number($OSD_FACTOR_))}">
                                <coord x="0" y="0"/>
                        </polyline>
                        <circle color="#008500" radius="{round(6*number($CAR_FACTOR_)*number($OSD_FACTOR_))}" width="{round(2*number($CAR_FACTOR_)*number($OSD_FACTOR_))}">
                                <coord x="0" y="0"/>
                        </circle>
                        <circle color="#00BC00" radius="{round(9*number($CAR_FACTOR_)*number($OSD_FACTOR_))}" width="{round(2*number($CAR_FACTOR_)*number($OSD_FACTOR_))}">
                                <coord x="0" y="0"/>
                        </circle>
                        <circle color="#008500" radius="{round(13*number($CAR_FACTOR_)*number($OSD_FACTOR_))}" width="{round(2*number($CAR_FACTOR_)*number($OSD_FACTOR_))}">
                                <coord x="0" y="0"/>
                        </circle>
                </itemgra>
		<xsl:text>&#x0A;        </xsl:text>
                <itemgra speed_range="3-">
                        <polygon color="#00000066">
                                <coord x="{round(-14*number($CAR_FACTOR_)*number($OSD_FACTOR_))}" y="{round(-18*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                                <coord x="0" y="{round(8*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                                <coord x="{round(14*number($CAR_FACTOR_)*number($OSD_FACTOR_))}" y="{round(-18*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                                <coord x="0" y="{round(-8*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                                <coord x="{round(-14*number($CAR_FACTOR_)*number($OSD_FACTOR_))}" y="{round(-18*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                        </polygon>
                        <polygon color="#008500">
                                <coord x="{round(-14*number($CAR_FACTOR_)*number($OSD_FACTOR_))}" y="{round(-12*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                                <coord x="0" y="{round(14*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                                <coord x="0" y="{round(-2*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                                <coord x="{round(-14*number($CAR_FACTOR_)*number($OSD_FACTOR_))}" y="{round(-12*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                        </polygon>
                        <polygon color="#00BC00">
                                <coord x="{round(14*number($CAR_FACTOR_)*number($OSD_FACTOR_))}" y="{round(-12*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                                <coord x="0" y="{round(14*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                                <coord x="0" y="{round(-2*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                                <coord x="{round(14*number($CAR_FACTOR_)*number($OSD_FACTOR_))}" y="{round(-12*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                        </polygon>
                        <polyline color="#008500" width="2">
                                <coord x="{round(-14*number($CAR_FACTOR_)*number($OSD_FACTOR_))}" y="{round(-12*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                                <coord x="0" y="{round(14*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                                <coord x="0" y="{round(-2*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                                <coord x="{round(-14*number($CAR_FACTOR_)*number($OSD_FACTOR_))}" y="{round(-12*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                        </polyline>
                        <polyline color="#008500" width="2">
                                <coord x="{round(14*number($CAR_FACTOR_)*number($OSD_FACTOR_))}" y="{round(-12*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                                <coord x="0" y="{round(14*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                                <coord x="0" y="{round(-2*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                                <coord x="{round(14*number($CAR_FACTOR_)*number($OSD_FACTOR_))}" y="{round(-12*number($CAR_FACTOR_)*number($OSD_FACTOR_))}"/>
                        </polyline>
                </itemgra>
		<xsl:text>&#x0A;        </xsl:text>
        </cursor>
		<xsl:text>&#x0A;        </xsl:text>
		<!-- android layout -->
		<!-- android layout -->
		<!-- android layout -->
			<layer name="polygons">
				<itemgra item_types="poly_wood" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polygon color="#8ec78d"/>
					<text text_size="5"/>
				</itemgra>

				<!-- ocean -->
				<itemgra item_types="poly_water_tiled">
					<polygon color="#82c8ea"/>
				</itemgra>
				<!-- ocean -->

				<!-- rivers with text -->
				<itemgra item_types="poly_water" order="{round(10-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polygon color="#82c8ea"/>
					<polyline color="#5096b8"/>
					<text text_size="5"/>
				</itemgra>
				<!-- rivers with text -->


				<itemgra item_types="poly_flats,poly_scrub,poly_military_zone,poly_marine,plantation,tundra" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polygon color="#a0a0a0"/>
					<text text_size="5"/>
				</itemgra>
				<itemgra item_types="poly_park" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polygon color="#7cc334"/>
					<text text_size="5"/>
				</itemgra>
				<itemgra item_types="poly_pedestrian" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="13"/>
					<polyline color="#dddddd" width="9"/>
					<polygon color="#dddddd"/>
				</itemgra>
				<itemgra item_types="poly_pedestrian" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="18"/>
					<polyline color="#dddddd" width="14"/>
					<polygon color="#dddddd"/>
				</itemgra>
				<itemgra item_types="poly_pedestrian" order="{round(16-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="21"/>
					<polyline color="#dddddd" width="17"/>
					<polygon color="#dddddd"/>
				</itemgra>
				<itemgra item_types="poly_pedestrian" order="{round(17-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="25"/>
					<polyline color="#dddddd" width="21"/>
					<polygon color="#dddddd"/>
				</itemgra>
				<itemgra item_types="poly_pedestrian" order="{round(18-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="40"/>
					<polyline color="#dddddd" width="34"/>
					<polygon color="#dddddd"/>
				</itemgra>
				<itemgra item_types="poly_airport" order="{round(5-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polygon color="#a0a0a0"/>
				</itemgra>

				<!--
				<itemgra item_types="poly_sport,poly_sports_pitch" order="0-">
					<polygon color="#4af04f"/>
				</itemgra>
				<itemgra item_types="poly_industry,poly_place" order="0-">
					<polygon color="#e6e6e6"/>
				</itemgra>
				-->

				<!--
				<itemgra item_types="poly_street_1" order="{round(8-number($LAYOUT_001_ORDER_DELTA_1))}-{round(13-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polygon color="#ffffff"/>
					<polyline color="#d2d2d2" width="1"/>
				</itemgra>
				<itemgra item_types="poly_street_1" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}-{round(16-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polygon color="#ffffff"/>
					<polyline color="#d2d2d2" width="2"/>
				</itemgra>
				<itemgra item_types="poly_street_1" order="{round(17-number($LAYOUT_001_ORDER_DELTA_1))}-{round(18-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polygon color="#ffffff"/>
					<polyline color="#d2d2d2" width="3"/>
				</itemgra>
				-->


				<!--
				<itemgra item_types="poly_street_2" order="{round(7-number($LAYOUT_001_ORDER_DELTA_1))}-{round(12-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polygon color="#fefc8c"/>
					<polyline color="#c0c0c0" width="1"/>
				</itemgra>
				<itemgra item_types="poly_street_2" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-{round(16-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polygon color="#fefc8c"/>
					<polyline color="#c0c0c0" width="2"/>
				</itemgra>
				<itemgra item_types="poly_street_2" order="{round(17-number($LAYOUT_001_ORDER_DELTA_1))}-{round(18-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polygon color="#fefc8c"/>
					<polyline color="#c0c0c0" width="3"/>
				</itemgra>
				-->

				<!--
				<itemgra item_types="poly_street_3" order="{round(7-number($LAYOUT_001_ORDER_DELTA_1))}-{round(11-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polygon color="#fefc8c"/>
					<polyline color="#a0a0a0" width="1"/>
				</itemgra>
				<itemgra item_types="poly_street_3" order="{round(12-number($LAYOUT_001_ORDER_DELTA_1))}-{round(15-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polygon color="#fefc8c"/>
					<polyline color="#a0a0a0" width="2"/>
				</itemgra>
				<itemgra item_types="poly_street_3" order="{round(16-number($LAYOUT_001_ORDER_DELTA_1))}-{round(18-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polygon color="#fefc8c"/>
					<polyline color="#a0a0a0" width="3"/>
				</itemgra>
				-->



				<!--
				<itemgra item_types="water_line" order="0-">
					<polyline color="#5096b8" width="2"/>
					<text text_size="5"/>
				</itemgra>
				-->

				<!--
				<itemgra item_types="water_river" order="0-{round(5-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#82c8ea" width="2"/>
				</itemgra>
				-->
				<itemgra item_types="water_river" order="{round(6-number($LAYOUT_001_ORDER_DELTA_1))}-{round(7-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#82c8ea" width="3"/>
				</itemgra>
				<itemgra item_types="water_river" order="{round(8-number($LAYOUT_001_ORDER_DELTA_1))}-{round(9-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#82c8ea" width="4"/>
				</itemgra>
				<itemgra item_types="water_river" order="{round(10-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#82c8ea" width="4"/>
					<text text_size="10"/>
				</itemgra>
				<itemgra item_types="water_canal" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#82c8ea" width="3"/>
					<text text_size="10"/>
				</itemgra>
				<itemgra item_types="water_stream" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#82c8ea" width="2"/>
					<text text_size="7"/>
				</itemgra>
				<itemgra item_types="water_drain" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#82c8ea" width="1"/>
					<text text_size="5"/>
				</itemgra>
				<itemgra item_types="poly_apron" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polygon color="#d0d0d0"/>
				</itemgra>
				<itemgra item_types="poly_terminal" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polygon color="#e3c6a6"/>
				</itemgra>
				<itemgra item_types="poly_car_parking" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polygon color="#e7cf87"/>
				</itemgra>
				<itemgra item_types="poly_building" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polygon color="#b6a6a6"/>
				</itemgra>
				<itemgra item_types="rail" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#696969" width="3"/>
					<polyline color="#ffffff" width="1" dash="5,5"/>
				</itemgra>
				<itemgra item_types="ferry" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#000000" width="1" dash="10"/>
				</itemgra>


				<itemgra item_types="border_country" order="0-{round(4-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#9b1199" width="8" />
				</itemgra>
				<itemgra item_types="border_country" order="{round(5-number($LAYOUT_001_ORDER_DELTA_1))}-{round(7-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#9b1199" width="12" />
				</itemgra>
				<itemgra item_types="border_country" order="{round(8-number($LAYOUT_001_ORDER_DELTA_1))}-{round(12-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#9b1199" width="20" dash="10,5"/>
				</itemgra>
				<itemgra item_types="border_country" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#9b1199" width="30" dash="10,5,2,1"/>
				</itemgra>


			</layer>
			<layer name="streets">

				<!-- route -->
				<itemgra item_types="street_route" order="{round(2-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#69e068" width="8"/>
				</itemgra>
				<itemgra item_types="street_route" order="{round(3-number($LAYOUT_001_ORDER_DELTA_1))}-{round(5-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#69e068" width="10"/>
				</itemgra>
				<itemgra item_types="street_route" order="{round(6-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#69e068" width="12"/>
				</itemgra>
				<itemgra item_types="street_route" order="{round(7-number($LAYOUT_001_ORDER_DELTA_1))}-{round(8-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#69e068" width="16"/>
				</itemgra>
				<itemgra item_types="street_route" order="{round(9-number($LAYOUT_001_ORDER_DELTA_1))}-{round(10-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#69e068" width="20"/>
				</itemgra>
				<itemgra item_types="street_route" order="{round(11-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#69e068" width="28"/>
				</itemgra>
				<itemgra item_types="street_route" order="{round(12-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#69e068" width="32"/>
				</itemgra>
				<itemgra item_types="street_route" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#69e068" width="52"/>
				</itemgra>
				<itemgra item_types="street_route" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#69e068" width="64"/>
				</itemgra>
				<itemgra item_types="street_route" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#69e068" width="68"/>
				</itemgra>
				<itemgra item_types="street_route" order="{round(16-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#69e068" width="132"/>
				</itemgra>
				<itemgra item_types="street_route" order="{round(17-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#69e068" width="268"/>
				</itemgra>
				<itemgra item_types="street_route" order="{round(18-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#69e068" width="530"/>
				</itemgra>
				<!-- route -->



				<itemgra item_types="selected_line" order="{round(2-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ba00b8" width="4"/>
				</itemgra>
				<itemgra item_types="selected_line" order="{round(3-number($LAYOUT_001_ORDER_DELTA_1))}-{round(5-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ba00b8" width="8"/>
				</itemgra>
				<itemgra item_types="selected_line" order="{round(6-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ba00b8" width="10"/>
				</itemgra>
				<itemgra item_types="selected_line" order="{round(7-number($LAYOUT_001_ORDER_DELTA_1))}-{round(8-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ba00b8" width="16"/>
				</itemgra>
				<itemgra item_types="selected_line" order="{round(9-number($LAYOUT_001_ORDER_DELTA_1))}-{round(10-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ba00b8" width="20"/>
				</itemgra>
				<itemgra item_types="selected_line" order="{round(11-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ba00b8" width="28"/>
				</itemgra>
				<itemgra item_types="selected_line" order="{round(12-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ba00b8" width="32"/>
				</itemgra>
				<itemgra item_types="selected_line" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ba00b8" width="52"/>
				</itemgra>
				<itemgra item_types="selected_line" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ba00b8" width="64"/>
				</itemgra>
				<itemgra item_types="selected_line" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ba00b8" width="68"/>
				</itemgra>
				<itemgra item_types="selected_line" order="{round(16-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ba00b8" width="132"/>
				</itemgra>
				<itemgra item_types="selected_line" order="{round(17-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ba00b8" width="268"/>
				</itemgra>
				<itemgra item_types="selected_line" order="{round(18-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ba00b8" width="530"/>
				</itemgra>
				<itemgra item_types="forest_way_1" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#0070c0" width="6"/>
				</itemgra>
				<itemgra item_types="forest_way_2" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#ff0000" width="3"/>
				</itemgra>
				<itemgra item_types="forest_way_3" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#ff0000" width="1" dash="2,4"/>
				</itemgra>
				<itemgra item_types="forest_way_4" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#119a2e" width="1" dash="4,10"/>
				</itemgra>
				<itemgra item_types="street_nopass" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#000000" width="1"/>
				</itemgra>
				<itemgra item_types="track_paved" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#000000" width="1"/>
				</itemgra>
				<itemgra item_types="track_gravelled" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ffffff" width="4" dash="4,8"/>
					<polyline color="#800000" width="2" dash="4,8"/>
				</itemgra>
				<itemgra item_types="track_gravelled" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}-{round(16-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ffffff" width="5" dash="5,10"/>
					<polyline color="#800000" width="3" dash="5,10"/>
				</itemgra>
				<itemgra item_types="track_gravelled" order="{round(17-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#ffffff" width="7" dash="7,15"/>
					<polyline color="#800000" width="5" dash="7,15"/>
				</itemgra>
				<itemgra item_types="track_unpaved" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#000000" width="1"/>
				</itemgra>
				<itemgra item_types="bridleway" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#000000" width="1"/>
				</itemgra>
				<itemgra item_types="cycleway" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#000000" width="1"/>
				</itemgra>
				<itemgra item_types="lift_cable_car" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#696969" width="1" dash="5"/>
				</itemgra>
				<itemgra item_types="lift_chair" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#696969" width="1" dash="5"/>
				</itemgra>
				<itemgra item_types="lift_drag" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#696969" width="1" dash="5"/>
				</itemgra>
				<itemgra item_types="footway" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}-{round(16-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ffffff" width="5" dash="5,10"/>
					<polyline color="#ff0000" width="3" dash="5,10"/>
				</itemgra>
				<itemgra item_types="footway" order="{round(17-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#ffffff" width="7" dash="7,15"/>
					<polyline color="#ff0000" width="5" dash="7,15"/>
				</itemgra>
				<itemgra item_types="steps" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#000000" width="1"/>
				</itemgra>
				<itemgra item_types="street_pedestrian,living_street" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="13"/>
					<polyline color="#dddddd" width="9"/>
				</itemgra>
				<itemgra item_types="street_pedestrian,living_street" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="18"/>
					<polyline color="#dddddd" width="14"/>
				</itemgra>
				<itemgra item_types="street_pedestrian,living_street" order="{round(16-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="21"/>
					<polyline color="#dddddd" width="17"/>
				</itemgra>
				<itemgra item_types="street_pedestrian,living_street" order="{round(17-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="25"/>
					<polyline color="#dddddd" width="21"/>
				</itemgra>
				<itemgra item_types="street_pedestrian,living_street" order="{round(18-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="40"/>
					<polyline color="#dddddd" width="34"/>
				</itemgra>
				<itemgra item_types="street_service" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="7"/>
					<polyline color="#fefefe" width="5"/>
				</itemgra>
				<itemgra item_types="street_service" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="8"/>
					<polyline color="#fefefe" width="6"/>
				</itemgra>
				<itemgra item_types="street_service" order="{round(16-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="9"/>
					<polyline color="#fefefe" width="7"/>
				</itemgra>
				<itemgra item_types="street_service" order="{round(17-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="10"/>
					<polyline color="#fefefe" width="8"/>
				</itemgra>
				<itemgra item_types="street_service" order="{round(18-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="11"/>
					<polyline color="#fefefe" width="9"/>
				</itemgra>
				<itemgra item_types="street_parking_lane" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="6"/>
					<polyline color="#fefefe" width="4"/>
				</itemgra>
				<itemgra item_types="street_parking_lane" order="{round(16-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="7"/>
					<polyline color="#fefefe" width="5"/>
				</itemgra>
				<itemgra item_types="street_parking_lane" order="{round(17-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="8"/>
					<polyline color="#fefefe" width="6"/>
				</itemgra>
				<itemgra item_types="street_parking_lane" order="{round(18-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="9"/>
					<polyline color="#fefefe" width="7"/>
				</itemgra>

				<!-- very small "white" street -->
				<itemgra item_types="street_0,street_1_city,street_1_land" order="{round(12-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="1"/>
				</itemgra>
				<itemgra item_types="street_0,street_1_city,street_1_land" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-{round(14-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="17"/>
					<polyline color="#ffffff" width="13"/>
				</itemgra>
				<itemgra item_types="street_0,street_1_city,street_1_land" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="18"/>
					<polyline color="#ffffff" width="14"/>
				</itemgra>
				<itemgra item_types="street_0,street_1_city,street_1_land" order="{round(16-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="30"/>
					<polyline color="#ffffff" width="26"/>
				</itemgra>
				<itemgra item_types="street_0,street_1_city,street_1_land" order="{round(17-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="67"/>
					<polyline color="#ffffff" width="61"/>
				</itemgra>
				<itemgra item_types="street_0,street_1_city,street_1_land" order="{round(18-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#d2d2d2" width="132"/>
					<polyline color="#ffffff" width="126"/>
				</itemgra>


				<!-- small -->
				<itemgra item_types="street_2_city,street_2_land,ramp" order="{round(11-number($LAYOUT_001_ORDER_DELTA_1))}-{round(12-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#c0c0c0" width="9"/>
					<polyline color="#fefc8c" width="7"/>
				</itemgra>
				<itemgra item_types="street_2_city,street_2_land,ramp" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-{round(14-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#c0c0c0" width="13"/>
					<polyline color="#fefc8c" width="11"/>
				</itemgra>
				<itemgra item_types="street_2_city,street_2_land,ramp" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#c0c0c0" width="19"/>
					<polyline color="#fefc8c" width="15"/>
				</itemgra>
				<itemgra item_types="street_2_city,street_2_land,ramp" order="{round(16-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#c0c0c0" width="30"/>
					<polyline color="#fefc8c" width="26"/>
				</itemgra>
				<itemgra item_types="street_2_city,street_2_land,ramp" order="{round(17-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#c0c0c0" width="63"/>
					<polyline color="#fefc8c" width="57"/>
				</itemgra>
				<itemgra item_types="street_2_city,street_2_land,ramp" order="{round(18-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#c0c0c0" width="100"/>
					<polyline color="#fefc8c" width="90"/>
				</itemgra>


				<!-- little bigger -->
				<!--
				<itemgra item_types="street_3_city,street_3_land,roundabout" order="{round(10-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#e0e0e0" width="2"/>
				</itemgra>
				-->
				<itemgra item_types="street_3_city,street_3_land,roundabout" order="{round(11-number($LAYOUT_001_ORDER_DELTA_1))}-{round(12-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#a0a0a0" width="9"/>
					<polyline color="#fefc8c" width="7"/>
				</itemgra>
				<itemgra item_types="street_3_city,street_3_land,roundabout" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-{round(14-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#a0a0a0" width="21"/>
					<polyline color="#fefc8c" width="17"/>
				</itemgra>
				<itemgra item_types="street_3_city,street_3_land,roundabout" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#a0a0a0" width="25"/>
					<polyline color="#fefc8c" width="21"/>
				</itemgra>
				<itemgra item_types="street_3_city,street_3_land,roundabout" order="{round(16-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#a0a0a0" width="40"/>
					<polyline color="#fefc8c" width="34"/>
				</itemgra>
				<itemgra item_types="street_3_city,street_3_land,roundabout" order="{round(17-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#a0a0a0" width="79"/>
					<polyline color="#fefc8c" width="73"/>
				</itemgra>
				<itemgra item_types="street_3_city,street_3_land,roundabout" order="{round(18-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#a0a0a0" width="156"/>
					<polyline color="#fefc8c" width="150"/>
				</itemgra>


				<!-- big -->
				<itemgra item_types="street_4_city,street_4_land,street_n_lanes" order="{round(2-number($LAYOUT_001_ORDER_DELTA_1))}-{round(6-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#f8dc79" width="4"/>
				</itemgra>
				<itemgra item_types="street_4_city,street_4_land,street_n_lanes" order="{round(7-number($LAYOUT_001_ORDER_DELTA_1))}-{round(8-number($LAYOUT_001_ORDER_DELTA_1))}">
					<!--<polyline color="#404040" width="3"/>-->
					<polyline color="#f8dc79" width="4"/>
				</itemgra>
				<itemgra item_types="street_4_city,street_4_land,street_n_lanes" order="{round(9-number($LAYOUT_001_ORDER_DELTA_1))}">
					<!--<polyline color="#000000" width="5"/>-->
					<polyline color="#f8dc79" width="5"/>
				</itemgra>
				<itemgra item_types="street_4_city,street_4_land,street_n_lanes" order="{round(10-number($LAYOUT_001_ORDER_DELTA_1))}">
					<!--<polyline color="#000000" width="6"/>-->
					<polyline color="#f8dc79" width="6"/>
				</itemgra>
				<itemgra item_types="street_4_city,street_4_land,street_n_lanes" order="{round(11-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#000000" width="9"/>
					<polyline color="#f8dc79" width="7"/>
				</itemgra>
				<itemgra item_types="street_4_city,street_4_land,street_n_lanes" order="{round(12-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#000000" width="13"/>
					<polyline color="#f8dc79" width="9"/>
				</itemgra>
				<itemgra item_types="street_4_city,street_4_land,street_n_lanes" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#000000" width="18"/>
					<polyline color="#f8dc79" width="14"/>
				</itemgra>
				<itemgra item_types="street_4_city,street_4_land,street_n_lanes" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#000000" width="21"/>
					<polyline color="#f8dc79" width="17"/>
				</itemgra>
				<itemgra item_types="street_4_city,street_4_land,street_n_lanes" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#000000" width="24"/>
					<polyline color="#f8dc79" width="20"/>
				</itemgra>
				<itemgra item_types="street_4_city,street_4_land,street_n_lanes" order="{round(16-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#000000" width="39"/>
					<polyline color="#f8dc79" width="33"/>
				</itemgra>
				<itemgra item_types="street_4_city,street_4_land,street_n_lanes" order="{round(17-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#000000" width="78"/>
					<polyline color="#f8dc79" width="72"/>
				</itemgra>
				<itemgra item_types="street_4_city,street_4_land,street_n_lanes" order="{round(18-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#000000" width="156"/>
					<polyline color="#f8dc79" width="150"/>
				</itemgra>


				<!-- autobahn / highway -->
				<itemgra item_types="highway_city,highway_land" order="0-{round(1-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#fc843b" width="1"/>
				</itemgra>
				<itemgra item_types="highway_city,highway_land" order="{round(2-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#fc843b" width="3"/>
				</itemgra>
				<itemgra item_types="highway_city,highway_land" order="{round(3-number($LAYOUT_001_ORDER_DELTA_1))}-{round(5-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#fc843b" width="6"/>
					<!--<polyline color="#fc843b" width="3"/>-->
					<!--<polyline color="#fc843b" width="1"/>-->
				</itemgra>
				<itemgra item_types="highway_city,highway_land" order="{round(6-number($LAYOUT_001_ORDER_DELTA_1))}">
					<!--<polyline color="#a8aab3" width="4"/>-->
					<polyline color="#fc843b" width="6"/>
				</itemgra>
				<itemgra item_types="highway_city,highway_land" order="{round(7-number($LAYOUT_001_ORDER_DELTA_1))}-{round(8-number($LAYOUT_001_ORDER_DELTA_1))}">
					<!--<polyline color="#a8aab3" width="7"/>-->
					<polyline color="#fc843b" width="6"/>
					<!--<polyline color="#a8aab3" width="1"/>-->
				</itemgra>
				<itemgra item_types="highway_city,highway_land" order="{round(9-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#fc843b" width="7"/>
				</itemgra>
				<itemgra item_types="highway_city,highway_land" order="{round(10-number($LAYOUT_001_ORDER_DELTA_1))}">
					<!--<polyline color="#a8aab3" width="9"/>-->
					<polyline color="#fc843b" width="9"/>
					<!--<polyline color="#a8aab3" width="1"/>-->
				</itemgra>
				<itemgra item_types="highway_city,highway_land" order="{round(11-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#a8aab3" width="13"/>
					<polyline color="#fc843b" width="9"/>
					<polyline color="#a8aab3" width="1"/>
				</itemgra>
				<itemgra item_types="highway_city,highway_land" order="{round(12-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#a8aab3" width="15"/>
					<polyline color="#fc843b" width="10"/>
					<polyline color="#a8aab3" width="1"/>
				</itemgra>
				<itemgra item_types="highway_city,highway_land" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#a8aab3" width="25"/>
					<polyline color="#fc843b" width="17"/>
					<polyline color="#a8aab3" width="1"/>
				</itemgra>
				<itemgra item_types="highway_city,highway_land" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#a8aab3" width="31"/>
					<polyline color="#fc843b" width="24"/>
					<polyline color="#a8aab3" width="1"/>
				</itemgra>
				<itemgra item_types="highway_city,highway_land" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#a8aab3" width="33"/>
					<polyline color="#fc843b" width="27"/>
					<polyline color="#a8aab3" width="1"/>
				</itemgra>
				<itemgra item_types="highway_city,highway_land" order="{round(16-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#a8aab3" width="65"/>
					<polyline color="#fc843b" width="59"/>
					<polyline color="#a8aab3" width="1"/>
				</itemgra>
				<itemgra item_types="highway_city,highway_land" order="{round(17-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#a8aab3" width="133"/>
					<polyline color="#fc843b" width="127"/>
					<polyline color="#a8aab3" width="1"/>
				</itemgra>
				<itemgra item_types="highway_city,highway_land" order="{round(18-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#a8aab3" width="264"/>
					<polyline color="#fc843b" width="258"/>
					<polyline color="#a8aab3" width="1"/>
				</itemgra>





				<itemgra item_types="highway_exit_label" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<circle color="#000000" radius="3" text_size="7"/>
				</itemgra>


				<itemgra item_types="highway_city,highway_land,street_4_city,street_4_land,street_n_lanes" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-{round(18-number($LAYOUT_001_ORDER_DELTA_1))}">
					<text text_size="8"/>
				</itemgra>
				<itemgra item_types="street_2_city,street_2_land,street_3_city,street_3_land,ramp" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-{round(18-number($LAYOUT_001_ORDER_DELTA_1))}">
					<text text_size="9"/>
				</itemgra>
				<itemgra item_types="street_nopass,street_0,street_1_city,street_1_land" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-{round(18-number($LAYOUT_001_ORDER_DELTA_1))}">
					<text text_size="9"/>
				</itemgra>

				<itemgra item_types="traffic_distortion" order="{round(7-number($LAYOUT_001_ORDER_DELTA_1))}-{round(8-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ff9000" width="8"/>
				</itemgra>
				<itemgra item_types="traffic_distortion" order="{round(9-number($LAYOUT_001_ORDER_DELTA_1))}-{round(10-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ff9000" width="10"/>
				</itemgra>
				<itemgra item_types="traffic_distortion" order="{round(11-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ff9000" width="14"/>
				</itemgra>
				<itemgra item_types="traffic_distortion" order="{round(12-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ff9000" width="16"/>
				</itemgra>
				<itemgra item_types="traffic_distortion" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ff9000" width="26"/>
				</itemgra>
				<itemgra item_types="traffic_distortion" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ff9000" width="32"/>
				</itemgra>
				<itemgra item_types="traffic_distortion" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ff9000" width="34"/>
				</itemgra>
				<itemgra item_types="traffic_distortion" order="{round(16-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ff9000" width="66"/>
				</itemgra>
				<itemgra item_types="traffic_distortion" order="{round(17-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ff9000" width="134"/>
				</itemgra>
				<itemgra item_types="traffic_distortion" order="{round(18-number($LAYOUT_001_ORDER_DELTA_1))}">
					<polyline color="#ff9000" width="265"/>
				</itemgra>
			</layer>
			<layer name="polylines">
				<itemgra item_types="rail_tram" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#606060" width="2"/>
				</itemgra>
			</layer>
			<layer name="labels">
				<itemgra item_types="house_number" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}-">
						<circle color="#000000" radius="3" text_size="7"/>
				</itemgra>


				<itemgra item_types="town_label,district_label,town_label_0e0,town_label_1e0,town_label_2e0,town_label_5e0,town_label_1e1,town_label_2e1,town_label_5e1,town_label_1e2,town_label_2e2,town_label_5e2,district_label_0e0,district_label_1e0,district_label_2e0,district_label_5e0,district_label_1e1,district_label_2e1,district_label_5e1,district_label_1e2,district_label_2e2,district_label_5e2" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<circle color="#000000" radius="3" text_size="7"/>
				</itemgra>


				<itemgra item_types="district_label_1e3,district_label_2e3,district_label_5e3" order="{round(12-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<circle color="#000000" radius="3" text_size="7"/>
				</itemgra>
				<itemgra item_types="town_label_1e3,place_label" order="{round(10-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<circle color="#000000" radius="3" text_size="10"/>
				</itemgra>
				<itemgra item_types="district_label_1e4,district_label_2e4,district_label_5e4" order="{round(11-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<circle color="#000000" radius="3" text_size="7"/>
				</itemgra>
				<itemgra item_types="town_label_2e3" order="{round(11-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<circle color="#000000" radius="3" text_size="10"/>
				</itemgra>
				<itemgra item_types="town_label_5e3,town_label_1e4,town_label_2e4,town_label_5e4" order="{round(11-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<circle color="#000000" radius="3" text_size="15"/>
				</itemgra>
				<itemgra item_types="district_label_1e5,district_label_2e5,district_label_5e5" order="{round(10-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<circle color="#000000" radius="3" text_size="7"/>
				</itemgra>
				<itemgra item_types="town_label_2e3" order="{round(10-number($LAYOUT_001_ORDER_DELTA_1))}">
					<circle color="#000000" radius="3" text_size="7"/>
				</itemgra>
				<itemgra item_types="district_label_1e6,district_label_2e6,district_label_5e6,district_label_1e7" order="{round(9-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<circle color="#000000" radius="3" text_size="7"/>
				</itemgra>
				<itemgra item_types="town_label_5e3" order="{round(9-number($LAYOUT_001_ORDER_DELTA_1))}-{round(10-number($LAYOUT_001_ORDER_DELTA_1))}">
					<circle color="#000000" radius="3" text_size="10"/>
				</itemgra>
				<itemgra item_types="town_label_1e4" order="{round(8-number($LAYOUT_001_ORDER_DELTA_1))}-{round(9-number($LAYOUT_001_ORDER_DELTA_1))}">
					<circle color="#000000" radius="3" text_size="10"/>
				</itemgra>
				<itemgra item_types="town_label_2e4,town_label_5e4" order="{round(7-number($LAYOUT_001_ORDER_DELTA_1))}-{round(8-number($LAYOUT_001_ORDER_DELTA_1))}">
					<circle color="#000000" radius="3" text_size="10"/>
				</itemgra>
				<itemgra item_types="town_label_1e5,town_label_2e5,town_label_5e5" order="{round(5-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<circle color="#000000" radius="3" text_size="15"/>
				</itemgra>
				<itemgra item_types="town_label_1e6,town_label_2e6,town_label_5e6,town_label_1e7" order="{round(5-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<circle color="#000000" radius="3" text_size="20"/>
				</itemgra>
				<itemgra item_types="town_label_1e5,town_label_2e5,town_label_5e5" order="{round(4-number($LAYOUT_001_ORDER_DELTA_1))}">
					<circle color="#000000" radius="3" text_size="10"/>
				</itemgra>
				<itemgra item_types="town_label_1e6,town_label_2e6,town_label_5e6,town_label_1e7" order="{round(4-number($LAYOUT_001_ORDER_DELTA_1))}">
					<circle color="#000000" radius="3" text_size="15"/>
				</itemgra>
				<itemgra item_types="town_label_1e6,town_label_2e6,town_label_5e6,town_label_1e7" order="0-{round(3-number($LAYOUT_001_ORDER_DELTA_1))}">
					<circle color="#000000" radius="3" text_size="10"/>
				</itemgra>
			</layer>


			<layer name="Internal">
				<itemgra item_types="track" order="{round(7-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#3f3f3f" width="1"/>
				</itemgra>
				<itemgra item_types="track_tracked" order="{round(7-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#3f3fff" width="3"/>
				</itemgra>
				<itemgra item_types="rg_segment" order="{round(12-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<polyline color="#FF089C" width="1"/>
					<arrows color="#FF089C" width="1"/>
				</itemgra>
				<itemgra item_types="rg_point" order="{round(12-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<circle color="#FF089C" radius="10" text_size="7"/>
				</itemgra>
				<itemgra item_types="nav_left_1" order="0-">
					<icon src="nav_left_1_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_left_2" order="0-">
					<icon src="nav_left_2_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_left_3" order="0-">
					<icon src="nav_left_3_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_right_1" order="0-">
					<icon src="nav_right_1_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_right_2" order="0-">
					<icon src="nav_right_2_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_right_3" order="0-">
					<icon src="nav_right_3_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_straight" order="0-">
					<icon src="nav_straight_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_turnaround" order="0-">
					<icon src="nav_turnaround_left_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_roundabout_l1" order="0-">
					<icon src="nav_roundabout_l1_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_roundabout_r1" order="0-">
					<icon src="nav_roundabout_r1_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_roundabout_l2" order="0-">
					<icon src="nav_roundabout_l2_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_roundabout_r2" order="0-">
					<icon src="nav_roundabout_r2_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_roundabout_l3" order="0-">
					<icon src="nav_roundabout_l3_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_roundabout_r3" order="0-">
					<icon src="nav_roundabout_r3_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_roundabout_l4" order="0-">
					<icon src="nav_roundabout_l4_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_roundabout_r4" order="0-">
					<icon src="nav_roundabout_r4_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_roundabout_l5" order="0-">
					<icon src="nav_roundabout_l5_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_roundabout_r5" order="0-">
					<icon src="nav_roundabout_r5_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_roundabout_l6" order="0-">
					<icon src="nav_roundabout_l6_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_roundabout_r6" order="0-">
					<icon src="nav_roundabout_r6_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_roundabout_l7" order="0-">
					<icon src="nav_roundabout_l7_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_roundabout_r7" order="0-">
					<icon src="nav_roundabout_r7_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_roundabout_l8" order="0-">
					<icon src="nav_roundabout_l8_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_roundabout_r8" order="0-">
					<icon src="nav_roundabout_r8_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="route_end" order="0-">
					<icon src="nav_destination_bk.png" w="32" h="32"/>
				</itemgra>
				<itemgra item_types="nav_none" order="0-">
					<icon src="unknown.png"/>
				</itemgra>
				<itemgra item_types="announcement" order="{round(7-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<icon src="gui_sound_32_32.png"/>
					<circle color="#FF089C" radius="10" text_size="7"/>
				</itemgra>
			</layer>


			<layer name="POI Symbols" active="1">
				<itemgra item_types="mini_roundabout" order="{round(12-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<icon src="mini_roundabout.png"/>
				</itemgra>
				<itemgra item_types="turning_circle" order="{round(12-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<icon src="mini_roundabout.png"/>
				</itemgra>
				<itemgra item_types="poi_airport" order="{round(6-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<icon src="airport.png"/>
				</itemgra>
				<itemgra item_types="poi_fuel" order="{round(12-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<icon src="fuel.png"/>
				</itemgra>
				<itemgra item_types="poi_bridge" order="{round(12-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<icon src="bridge.png"/>
				</itemgra>
				<itemgra item_types="highway_exit" order="{round(12-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<icon src="exit.png"/>
				</itemgra>
				<itemgra item_types="poi_auto_club" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<icon src="auto_club.png"/>
				</itemgra>
				<itemgra item_types="poi_bank" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<icon src="bank.png"/>
				</itemgra>
				<itemgra item_types="poi_danger_area" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<icon src="danger_16_16.png"/>
				</itemgra>
				<itemgra item_types="poi_forbidden_area" order="{round(13-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<icon src="forbiden_area.png"/>
				</itemgra>
				<itemgra item_types="poi_tunnel" order="{round(12-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<icon src="tunnel.png"/>
				</itemgra>
				<itemgra item_types="traffic_signals" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<icon src="traffic_signals.png"/>
				</itemgra>
			</layer>
			<layer name="POI Labels" active="1">
				<itemgra item_types="poi_airport,town_ghost,poi_hotel,poi_car_parking,poi_car_dealer_parts,poi_fuel,poi_shopping,poi_attraction,poi_cafe,poi_bar,highway_exit,poi_camp_rv,poi_museum_history,poi_hospital,poi_dining,poi_fastfood,poi_police,poi_autoservice,poi_bank,poi_bus_station,poi_bus_stop,poi_business_service,poi_car_rent,poi_church,poi_cinema,poi_concert,poi_drinking_water,poi_emergency,poi_fair,poi_fish,poi_government_building,poi_hotspring,poi_information,poi_justice,poi_landmark,poi_library,poi_mall,poi_manmade_feature,poi_marine,poi_marine_type,poi_mark,poi_oil_field,poi_peak,poi_personal_service,poi_pharmacy,poi_post_office,poi_public_office,poi_rail_halt,poi_rail_station,poi_rail_tram_stop,poi_repair_service,poi_resort,poi_restaurant,poi_restricted_area,poi_sailing,poi_scenic_area,poi_school,poi_service,poi_shop_retail,poi_skiing,poi_social_service,poi_sport,poi_stadium,poi_swimming,poi_theater,poi_townhall,poi_trail,poi_truck_stop,poi_tunnel,poi_worship,poi_wrecker,poi_zoo,poi_biergarten,poi_castle,poi_ruins,poi_memorial,poi_monument,poi_shelter,poi_fountain,poi_viewpoint,vehicle" order="{round(15-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<circle color="#606060" radius="0" width="0" text_size="10"/>
				</itemgra>
				<!--
				<itemgra item_types="poi_custom0,poi_custom1,poi_custom2,poi_custom3,poi_custom4,poi_custom5,poi_custom6,poi_custom7,poi_custom8,poi_custom9,poi_customa,poi_customb,poi_customc,poi_customd,poi_custome,poi_customf" order="{round(14-number($LAYOUT_001_ORDER_DELTA_1))}-">
					<circle color="#606060" radius="0" width="0" text_size="10"/>
				</itemgra>
				-->
			</layer>


			<layer name="Android-POI-Icons-full" active="0">
				<itemgra item_types="mini_roundabout" order="12-">
					<icon src="mini_roundabout.png"/>
				</itemgra>
				<itemgra item_types="turning_circle" order="12-">
					<icon src="mini_roundabout.png"/>
				</itemgra>
				<itemgra item_types="poi_airport" order="5-">
					<icon src="airport.png"/>
				</itemgra>
				<itemgra item_types="town_ghost" order="10-">
					<icon src="ghost_town.png"/>
				</itemgra>
				<itemgra item_types="poi_hotel" order="12-">
					<icon src="hotel.png"/>
				</itemgra>
				<itemgra item_types="poi_car_parking" order="11-">
					<icon src="parking.png"/>
				</itemgra>
				<itemgra item_types="poi_car_dealer_parts" order="12-">
					<icon src="car_dealer.png"/>
				</itemgra>
				<itemgra item_types="poi_fuel" order="12-">
					<icon src="fuel.png"/>
				</itemgra>
				<itemgra item_types="poi_shopping" order="12-">
					<icon src="shopping.png"/>
				</itemgra>
				<itemgra item_types="poi_attraction" order="11-">
					<icon src="attraction.png"/>
				</itemgra>
				<itemgra item_types="poi_cafe" order="12-">
					<icon src="cafe.png"/>
				</itemgra>
				<itemgra item_types="poi_bar" order="12-">
					<icon src="bar.png"/>
				</itemgra>
				<itemgra item_types="poi_bridge" order="12-">
					<icon src="bridge.png"/>
				</itemgra>
				<itemgra item_types="highway_exit" order="11-">
					<icon src="exit.png"/>
				</itemgra>
				<itemgra item_types="poi_camp_rv" order="12-">
					<icon src="camping.png"/>
				</itemgra>
				<itemgra item_types="poi_museum_history" order="12-">
					<icon src="museum.png"/>
				</itemgra>
				<itemgra item_types="poi_hospital" order="12-">
					<icon src="hospital.png"/>
				</itemgra>
				<itemgra item_types="poi_dining" order="12-">
					<icon src="dining.png"/>
				</itemgra>
				<itemgra item_types="poi_fastfood" order="12-">
					<icon src="fastfood.png"/>
				</itemgra>
				<itemgra item_types="poi_police" order="12-">
					<icon src="police.png"/>
				</itemgra>
				<itemgra item_types="poi_auto_club" order="12-">
					<icon src="auto_club.png"/>
				</itemgra>
				<itemgra item_types="poi_autoservice" order="12-">
					<icon src="car_dealer.png"/>
				</itemgra>
				<itemgra item_types="poi_bank" order="12-">
					<icon src="bank.png"/>
				</itemgra>
				<itemgra item_types="poi_bay" order="12-">
					<icon src="bay.png"/>
				</itemgra>
				<itemgra item_types="poi_bend" order="12-">
					<icon src="bend.png"/>
				</itemgra>
				<itemgra item_types="poi_boat_ramp" order="12-">
					<icon src="boat_ramp.png"/>
				</itemgra>
				<itemgra item_types="poi_border_station" order="12-">
					<icon src="border_station.png"/>
				</itemgra>
				<itemgra item_types="poi_bowling" order="12-">
					<icon src="bowling.png"/>
				</itemgra>
				<itemgra item_types="poi_bus_station" order="11-">
					<icon src="bus.png"/>
				</itemgra>
				<itemgra item_types="poi_bus_stop" order="12-">
					<icon src="bus.png"/>
				</itemgra>
				<itemgra item_types="poi_business_service" order="12-">
					<icon src="bussines_service.png"/>
				</itemgra>
				<itemgra item_types="poi_car_rent" order="12-">
					<icon src="car_rent.png"/>
				</itemgra>
				<itemgra item_types="poi_car_wash" order="12-">
					<icon src="car_wash.png"/>
				</itemgra>
				<itemgra item_types="poi_casino" order="12-">
					<icon src="casino.png"/>
				</itemgra>
				<itemgra item_types="poi_cemetery" order="12-">
					<icon src="cemetery.png"/>
				</itemgra>
				<itemgra item_types="poi_church" order="11-">
					<icon src="church.png"/>
				</itemgra>
				<itemgra item_types="poi_cinema" order="12-">
					<icon src="cinema.png"/>
				</itemgra>
				<itemgra item_types="poi_communication" order="12-">
					<icon src="communication.png"/>
				</itemgra>
				<itemgra item_types="poi_concert" order="12-">
					<icon src="concert.png"/>
				</itemgra>
				<itemgra item_types="poi_cove" order="12-">
					<icon src="cove.png"/>
				</itemgra>
				<itemgra item_types="poi_crossing" order="12-">
					<icon src="crossing.png"/>
				</itemgra>
				<itemgra item_types="poi_dam" order="12-">
					<icon src="dam.png"/>
				</itemgra>
				<itemgra item_types="poi_danger_area" order="12-">
					<icon src="danger_16_16.png"/>
				</itemgra>
				<itemgra item_types="poi_danger_sea_wreck" order="12-">
					<icon src="dangerous.png"/>
				</itemgra>
				<itemgra item_types="poi_daymark" order="12-">
					<icon src="daymark.png"/>
				</itemgra>
				<itemgra item_types="poi_diving" order="12-">
					<icon src="diving.png"/>
				</itemgra>
				<itemgra item_types="poi_drinking_water" order="13-">
					<icon src="drinking_water.png"/>
				</itemgra>
				<itemgra item_types="poi_emergency" order="12-">
					<icon src="emergency.png"/>
				</itemgra>
				<itemgra item_types="poi_fair" order="12-">
					<icon src="fair.png"/>
				</itemgra>
				<itemgra item_types="poi_firebrigade" order="12-">
					<icon src="firebrigade.png"/>
				</itemgra>
				<itemgra item_types="poi_fish" order="9-">
					<icon src="fish.png"/>
				</itemgra>
				<itemgra item_types="poi_forbidden_area" order="12-">
					<icon src="forbiden_area.png"/>
				</itemgra>
				<itemgra item_types="poi_shop_gps" order="15-">
					<icon src="garmin.png"/>
				</itemgra>
				<itemgra item_types="poi_golf" order="12-">
					<icon src="golf.png"/>
				</itemgra>
				<itemgra item_types="poi_government_building" order="12-">
					<icon src="goverment_building.png"/>
				</itemgra>
				<itemgra item_types="poi_height" order="12-">
					<icon src="height.png"/>
				</itemgra>
				<itemgra item_types="poi_heliport" order="8-">
					<icon src="heliport.png"/>
				</itemgra>
				<itemgra item_types="poi_hotspring" order="12-">
					<icon src="hotspring.png"/>
				</itemgra>
				<itemgra item_types="poi_icesport" order="12-">
					<icon src="icesport.png"/>
				</itemgra>
				<itemgra item_types="poi_information" order="12-">
					<icon src="information.png"/>
				</itemgra>
				<itemgra item_types="poi_justice" order="12-">
					<icon src="justice.png"/>
				</itemgra>
				<itemgra item_types="poi_landmark" order="11-">
					<icon src="landmark.png"/>
				</itemgra>
				<itemgra item_types="poi_levee" order="12-">
					<icon src="levee.png"/>
				</itemgra>
				<itemgra item_types="poi_level_crossing" order="11-">
					<icon src="level_crossing.png"/>
				</itemgra>
				<itemgra item_types="poi_library" order="12-">
					<icon src="library.png"/>
				</itemgra>
				<itemgra item_types="poi_locale" order="12-">
					<icon src="locale.png"/>
				</itemgra>
				<itemgra item_types="poi_loudspeaker" order="12-">
					<icon src="loudspeaker.png"/>
				</itemgra>
				<itemgra item_types="poi_mall" order="12-">
					<icon src="mall.png"/>
				</itemgra>
				<itemgra item_types="poi_manmade_feature" order="12-">
					<icon src="manmade_feature.png"/>
				</itemgra>
				<itemgra item_types="poi_marine" order="12-">
					<icon src="marine.png"/>
				</itemgra>
				<itemgra item_types="poi_marine_type" order="12-">
					<icon src="marine_type.png"/>
				</itemgra>
				<itemgra item_types="poi_mark" order="12-">
					<icon src="mark.png"/>
				</itemgra>
				<itemgra item_types="poi_military" order="11-">
					<icon src="military.png"/>
				</itemgra>
				<itemgra item_types="poi_mine" order="12-">
					<icon src="mine.png"/>
				</itemgra>
				<itemgra item_types="poi_nondangerous" order="12-">
					<icon src="nondangerous.png"/>
				</itemgra>
				<itemgra item_types="poi_oil_field" order="12-">
					<icon src="oil_field.png"/>
				</itemgra>
				<itemgra item_types="poi_peak" order="7-">
					<icon src="peak.png"/>
				</itemgra>
				<itemgra item_types="poi_personal_service" order="12-">
					<icon src="personal_service.png"/>
				</itemgra>
				<itemgra item_types="poi_pharmacy" order="12-">
					<icon src="pharmacy.png"/>
				</itemgra>
				<itemgra item_types="poi_post_office,poi_post_box" order="13-">
					<icon src="post.png"/>
				</itemgra>
				<itemgra item_types="poi_public_office" order="12-">
					<icon src="public_office.png"/>
				</itemgra>
				<itemgra item_types="poi_rail_halt" order="11-">
					<circle color="#ff0000" radius="3" width="3"/>
					<circle color="#000000" radius="5" width="2" text_size="8"/>
				</itemgra>
				<itemgra item_types="poi_rail_station" order="9-">
					<circle color="#ff0000" radius="3" width="3"/>
					<circle color="#000000" radius="6" width="2" text_size="8"/>
				</itemgra>
				<itemgra item_types="poi_rail_tram_stop" order="10-11">
					<circle color="#ff0000" radius="2" width="2"/>
				</itemgra>
				<itemgra item_types="poi_rail_tram_stop" order="12-">
					<circle color="#ff0000" radius="3" width="3"/>
					<circle color="#606060" radius="5" width="2" text_size="8"/>
				</itemgra>
				<itemgra item_types="poi_repair_service" order="12-">
					<icon src="repair_service.png"/>
				</itemgra>
				<itemgra item_types="poi_resort" order="12-">
					<icon src="resort.png"/>
				</itemgra>
				<itemgra item_types="poi_restaurant" order="12-">
					<icon src="restaurant.png"/>
				</itemgra>
				<itemgra item_types="poi_restricted_area" order="12-">
					<icon src="restricted_area.png"/>
				</itemgra>
				<itemgra item_types="poi_restroom" order="13-">
					<icon src="restroom.png"/>
				</itemgra>
				<itemgra item_types="poi_sailing" order="12-">
					<icon src="sailing.png"/>
				</itemgra>
				<itemgra item_types="poi_scenic_area" order="12-">
					<icon src="scenic_area.png"/>
				</itemgra>
				<itemgra item_types="poi_school" order="12-">
					<icon src="school.png"/>
				</itemgra>
				<itemgra item_types="poi_service" order="12-">
					<icon src="service.png"/>
				</itemgra>
				<itemgra item_types="poi_shop_apparel" order="12-">
					<icon src="shop_apparel.png"/>
				</itemgra>
				<itemgra item_types="poi_shop_computer" order="12-">
					<icon src="shop_computer.png"/>
				</itemgra>
				<itemgra item_types="poi_shop_department" order="12-">
					<icon src="shop_department.png"/>
				</itemgra>
				<itemgra item_types="poi_shop_furniture" order="12-">
					<icon src="shop_furnish.png"/>
				</itemgra>
				<itemgra item_types="poi_shop_grocery" order="12-">
					<icon src="shop_grocery.png"/>
				</itemgra>
				<itemgra item_types="poi_shop_handg" order="12-">
					<icon src="shop_handg.png"/>
				</itemgra>
				<itemgra item_types="poi_shop_merchandise" order="12-">
					<icon src="shop_merchandise.png"/>
				</itemgra>
				<itemgra item_types="poi_shop_retail" order="12-">
					<icon src="shop_retail.png"/>
				</itemgra>
				<itemgra item_types="poi_shower" order="13-">
					<icon src="shower.png"/>
				</itemgra>
				<itemgra item_types="poi_skiing" order="11-">
					<icon src="skiing.png"/>
				</itemgra>
				<itemgra item_types="poi_social_service" order="12-">
					<icon src="social_service.png"/>
				</itemgra>
				<itemgra item_types="poi_sounding" order="12-">
					<icon src="sounding.png"/>
				</itemgra>
				<itemgra item_types="poi_sport" order="12-">
					<icon src="sport.png"/>
				</itemgra>
				<itemgra item_types="poi_stadium" order="11-">
					<icon src="stadium.png"/>
				</itemgra>
				<itemgra item_types="poi_swimming" order="12-">
					<icon src="swimming.png"/>
				</itemgra>
				<itemgra item_types="poi_telephone" order="13-">
					<icon src="telephone.png"/>
				</itemgra>
				<itemgra item_types="poi_theater" order="12-">
					<icon src="theater.png"/>
				</itemgra>
				<itemgra item_types="poi_tide" order="12-">
					<icon src="tide.png"/>
				</itemgra>
				<itemgra item_types="poi_tower" order="13-">
					<icon src="tower.png"/>
				</itemgra>
				<itemgra item_types="poi_townhall" order="12-">
					<icon src="townhall.png"/>
				</itemgra>
				<itemgra item_types="poi_trail" order="12-">
					<icon src="trail.png"/>
				</itemgra>
				<itemgra item_types="poi_truck_stop" order="12-">
					<icon src="truck_stop.png"/>
				</itemgra>
				<itemgra item_types="poi_tunnel" order="12-">
					<icon src="tunnel.png"/>
				</itemgra>
				<itemgra item_types="poi_wine" order="12-">
					<icon src="wine.png"/>
				</itemgra>
				<itemgra item_types="poi_worship" order="8-">
					<icon src="worship.png"/>
				</itemgra>
				<itemgra item_types="poi_wrecker" order="12-">
					<icon src="wrecker.png"/>
				</itemgra>
				<itemgra item_types="poi_zoo" order="9-">
					<icon src="zoo.png"/>
				</itemgra>
				<itemgra item_types="poi_picnic" order="12-">
					<icon src="picnic.png"/>
				</itemgra>
				<itemgra item_types="poi_gc_multi" order="12-">
					<icon src="gc_multi.png"/>
				</itemgra>
				<itemgra item_types="poi_gc_tradi" order="12-">
					<icon src="gc_tradi.png"/>
				</itemgra>
				<itemgra item_types="poi_gc_event" order="12-">
					<icon src="gc_event.png"/>
				</itemgra>
				<itemgra item_types="poi_gc_mystery" order="12-">
					<icon src="gc_mystery.png"/>
				</itemgra>
				<itemgra item_types="poi_gc_question" order="12-">
					<icon src="gc_question.png"/>
				</itemgra>
				<itemgra item_types="poi_gc_stages" order="12-">
					<icon src="gc_stages.png"/>
				</itemgra>
				<itemgra item_types="poi_gc_reference" order="12-">
					<icon src="gc_reference.png"/>
				</itemgra>
				<itemgra item_types="poi_gc_webcam" order="12-">
					<icon src="gc_webcam.png"/>
				</itemgra>
				<itemgra item_types="traffic_signals" order="13-">
					<icon src="traffic_signals.png"/>
				</itemgra>
				<itemgra item_types="poi_wifi" order="13-">
					<icon src="wifi.png"/>
				</itemgra>
				<itemgra item_types="poi_image" order="12-">
					<image/>
				</itemgra>
				<!-- I'm not sure if the following stuff should appear in any layout. Maybe portions should only apply to the bicyle layout. -->
				<itemgra item_types="poi_bench" order="13-">
					<icon src="bench.png"/>
				</itemgra>
				<itemgra item_types="poi_biergarten" order="12-">
					<icon src="biergarten.png"/>
				</itemgra>
				<itemgra item_types="poi_boundary_stone" order="13-">
					<icon src="boundary_stone.png"/>
				</itemgra>
				<itemgra item_types="poi_castle" order="11-">
					<icon src="castle.png"/>
				</itemgra>
				<itemgra item_types="poi_ruins" order="11-">
					<icon src="ruins.png"/>
				</itemgra>
				<itemgra item_types="poi_hunting_stand" order="12-">
					<icon src="hunting_stand.png"/>
				</itemgra>
				<itemgra item_types="poi_memorial" order="12-">
					<icon src="memorial.png"/>
				</itemgra>
				<itemgra item_types="poi_monument" order="12-">
					<icon src="memorial.png"/>
				</itemgra>
				<itemgra item_types="poi_shelter" order="11-">
					<icon src="shelter.png"/>
				</itemgra>
				<itemgra item_types="poi_fountain" order="13-">
					<icon src="fountain.png"/>
				</itemgra>
				<itemgra item_types="poi_potable_water" order="13-">
					<icon src="potable_water.png"/>
				</itemgra>
				<itemgra item_types="poi_toilets" order="12-">
					<icon src="toilets.png"/>
				</itemgra>
				<itemgra item_types="poi_viewpoint" order="10-">
					<icon src="viewpoint.png"/>
				</itemgra>
				<itemgra item_types="tec_common" order="11-">
					<icon src="tec_common.png" w="24" h="24"/>
				</itemgra>
				<itemgra item_types="vehicle" order="0-">
					<icon src="gui_vehicle_16_16.png"/>
				</itemgra>
				<itemgra item_types="vehicle_pedestrian" order="0-">
					<icon src="gui_vehicle_pedestrian_16_16.png"/>
				</itemgra>
				<itemgra item_types="poi_custom0,poi_custom1,poi_custom2,poi_custom3,poi_custom4,poi_custom5,poi_custom6,poi_custom7,poi_custom8,poi_custom9,poi_customa,poi_customb,poi_customc,poi_customd,poi_custome,poi_customf" order="10-">
					<icon src="%s"/>
				</itemgra>
			</layer>
			<layer name="Android-POI-Labels-full" active="0">
				<itemgra item_types="poi_airport,town_ghost,poi_hotel,poi_car_parking,poi_car_dealer_parts,poi_fuel,poi_shopping,poi_attraction,poi_cafe,poi_bar,highway_exit,poi_camp_rv,poi_museum_history,poi_hospital,poi_dining,poi_fastfood,poi_police,poi_autoservice,poi_bank,poi_bus_station,poi_bus_stop,poi_business_service,poi_car_rent,poi_church,poi_cinema,poi_concert,poi_drinking_water,poi_emergency,poi_fair,poi_fish,poi_government_building,poi_hotspring,poi_information,poi_justice,poi_landmark,poi_library,poi_mall,poi_manmade_feature,poi_marine,poi_marine_type,poi_mark,poi_oil_field,poi_peak,poi_personal_service,poi_pharmacy,poi_post_office,poi_public_office,poi_rail_halt,poi_rail_station,poi_rail_tram_stop,poi_repair_service,poi_resort,poi_restaurant,poi_restricted_area,poi_sailing,poi_scenic_area,poi_school,poi_service,poi_shop_retail,poi_skiing,poi_social_service,poi_sport,poi_stadium,poi_swimming,poi_theater,poi_townhall,poi_trail,poi_truck_stop,poi_tunnel,poi_worship,poi_wrecker,poi_zoo,poi_biergarten,poi_castle,poi_ruins,poi_memorial,poi_monument,poi_shelter,poi_fountain,poi_viewpoint,vehicle" order="14-">
					<circle color="#606060" radius="0" width="0" text_size="10"/>
				</itemgra>
				<itemgra item_types="poi_custom0,poi_custom1,poi_custom2,poi_custom3,poi_custom4,poi_custom5,poi_custom6,poi_custom7,poi_custom8,poi_custom9,poi_customa,poi_customb,poi_customc,poi_customd,poi_custome,poi_customf" order="14-">
					<circle color="#606060" radius="0" width="0" text_size="10"/>
				</itemgra>
			</layer>



		<!-- android layout -->
		<!-- android layout -->
		<!-- android layout -->
		<xsl:text>&#x0A;        </xsl:text>
</layout>
		<xsl:text>&#x0A;        </xsl:text>
		<xsl:copy><xsl:copy-of select="@*|node()"/></xsl:copy>
	</xsl:template>
        <xsl:template match="/config/navit/layout[@name='XXRouteXX']">
<xsl:text>&#x0A;        </xsl:text>
		<layout name="Route">
<xsl:text>&#x0A;        </xsl:text>
			<layer name="streets">
<xsl:text>&#x0A;        </xsl:text>
				<itemgra item_types="street_route_occluded" order="0-">
<xsl:text>&#x0A;        </xsl:text>
					<polyline color="#69e068" width="20"/>
<xsl:text>&#x0A;        </xsl:text>
				</itemgra>
<xsl:text>&#x0A;        </xsl:text>
			</layer>
<xsl:text>&#x0A;        </xsl:text>
		</layout>
<xsl:text>&#x0A;        </xsl:text>
	</xsl:template>
	<!-- add new default look-and-feel for android -->
        <xsl:template match="/config/navit/speech">
                <xsl:copy><xsl:copy-of select="@*[not(name()='data')]"/><xsl:attribute name="type">android</xsl:attribute><xsl:apply-templates/></xsl:copy>
	</xsl:template>
        <xsl:template match="@*|node()"><xsl:copy><xsl:apply-templates select="@*|node()"/></xsl:copy></xsl:template>
</xsl:transform>
