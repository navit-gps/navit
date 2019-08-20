<?xml version="1.0"?>
<xsl:transform xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude" version="1.0">
	<xsl:param name="OSD_SIZE" select="1"/>
	<xsl:param name="ICON_SMALL" select="24"/>
	<xsl:param name="ICON_MEDIUM" select="24"/>
	<xsl:param name="ICON_BIG" select="48"/>
	<xsl:param name="OSD_USE_OVERLAY">yes</xsl:param>

	<xsl:output method="xml" indent="yes" cdata-section-elements="gui" doctype-system="navit.dtd"/>
	<xsl:include href="osd_minimum.xslt"/>

	<xsl:template match="/">
		<xsl:apply-templates select="config"/>
		<xsl:apply-templates select="layout"/>
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
			<xsl:copy-of select="xi:include"/>
		</xsl:copy>
	</xsl:template>

	<xsl:template match="layout">
		<xsl:copy>
			<xsl:copy-of select="@*|node()"/>
		</xsl:copy>
	</xsl:template>
</xsl:transform>
