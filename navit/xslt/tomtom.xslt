<?xml version="1.0"?>
<xsl:transform xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude" version="1.0">

	<xsl:output method="xml" indent="yes" cdata-section-elements="gui" doctype-system="navit.dtd"/>

	<xsl:template match="/">
		<xsl:apply-templates select="config"/>
	</xsl:template>

	<xsl:template match="config">
		<xsl:copy>
			<xsl:apply-templates select="navit"/>
		</xsl:copy>
	</xsl:template>

	<xsl:template match="navit">
		<xsl:copy>
			<xsl:attribute name="zoom">32</xsl:attribute>
			<xsl:attribute name="tracking">1</xsl:attribute>
			<xsl:attribute name="orientation">-1</xsl:attribute>
			<xsl:attribute name="autozoom_active">1</xsl:attribute>
			<xsl:attribute name="recent_dest">25</xsl:attribute>
			<graphics type="sdl" w="480" h="272" bpp="16" frame="0" flags="1"/>
			<xsl:copy-of select="gui[@type='internal']"/>
			<osd enabled="yes" type="text" x="80" y="0" w="320" h="40" font_size="360" align="0" label="${{tracking.item.street_name}} ${{tracking.item.street_name_systematic}}" background_color="#00008080"/>
			<osd enabled="yes" type="text" x="80" y="232" w="80" h="40" font_size="360" align="0" label="${{navigation.item[1].length[named]}}" background_color="#00008080"/>
			<osd enabled="yes" type="text" x="160" y="232" w="240" h="40" font_size="360" align="0" label="${{vehicle.position_speed}} / ${{tracking.item.route_speed}}" background_color="#00008080"/>
			<osd enabled="yes" type="text" x="400" y="232" w="80" h="40" font_size="360" align="0" label="${{navigation.item.destination_length[named]}}" background_color="#00008080"/>
			<osd enabled="yes" type="text" x="400" y="192" w="80" h="40" font_size="360" align="0" label="${{navigation.item.destination_time[arrival]}}" background_color="#00008080"/>
			<osd enabled="yes" type="compass" x="400" y="80" w="80" h="112" background_color="#00008080" font_size="300"/>
			<osd enabled="yes" type="gps_status" x="0" y="80" w="80" h="40" background_color="#00008080"/>
			<osd enabled="yes" type="speed_warner" x="0" y="120" w="80" h="72" background_color="#00008080" font_size="300"/>
			<osd enabled="yes" type="navigation_next_turn" x="0" y="192" w="80" h="80" icon_w="48" icon_h="48" icon_src="%s_wh_48_48.png" background_color="#00008080"/>
			<osd enabled="yes" type="button" x="400" y="0" command="zoom_in()" src="tomtom_plus_80_80.png"/>
			<osd enabled="yes" type="button" x="0" y="0" command="zoom_out()" src="tomtom_minus_80_80.png"/>
			<vehicle name="Local GPS" profilename="car" enabled="yes" active="yes" follow="1" source="file:/var/run/gpspipe">
			<!-- Navit can write a tracklog in several formats (gpx, nmea or textfile): -->
				<log enabled="no" type="gpx" attr_types="position_time_iso8601,position_direction,position_speed,position_radius" data="/mnt/sdcard/navit/track_%Y%m%d-%%i.gpx" flush_size="1000" flush_time="30"/>
			</vehicle>
			<vehicle name="Demo" profilename="car" enabled="yes" active="no" follow="1" source="demo://" speed="100"/>
			<xsl:copy-of select="tracking"/>

			<xsl:copy-of select="vehicleprofile[@name='car']"/>
			<xsl:copy-of select="vehicleprofile[@name='car_shortest']"/>
			<xsl:copy-of select="vehicleprofile[@name='car_avoid_tolls']"/>
			<xsl:copy-of select="vehicleprofile[@name='bike']"/>
			<xsl:copy-of select="vehicleprofile[@name='pedestrian']"/>

			<xsl:copy-of select="route"/>
			<xsl:copy-of select="navigation"/>

			<xsl:comment>Use espeak.</xsl:comment>
			<speech type="cmdline" data="/mnt/sdcard/navit/bin/espeakdsp -v en '%s'"/>

			<mapset enabled="yes">
				<map type="binfile" enabled="yes" data="$NAVIT_SHAREDIR/maps/*.bin"/>
			</mapset>
			<xsl:copy-of select="layer"/>
			<xsl:copy-of select="layout"/>
		</xsl:copy>
	</xsl:template>
</xsl:transform>
