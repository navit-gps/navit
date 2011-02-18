<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
        <xsl:output method="xml" doctype-system="navit.dtd" cdata-section-elements="gui"/>
        <xsl:variable name="OSD_SIZE">1.33</xsl:variable>
        <xsl:variable name="OSD_FACTOR">1</xsl:variable>
        <xsl:variable name="CAR_FACTOR">2</xsl:variable>

	<xsl:variable name="MENU_VALUE_FONTSIZE">200</xsl:variable>
	<!-- icons_xs = selection icons (the green ones) -->
	<!-- icons_s = topmenu and POI-selection-items -->
	<!-- icons_l = menu items -->
	<!-- spacing = spacing between menu items in pixels -->
	<xsl:variable name="MENU_VALUE_ICONS_XS">32</xsl:variable>
	<xsl:variable name="MENU_VALUE_ICONS_S">32</xsl:variable>
	<xsl:variable name="MENU_VALUE_ICONS_L">64</xsl:variable>
	<xsl:variable name="MENU_VALUE_SPACING">3</xsl:variable>

	<xsl:variable name="LAYOUT_001_ORDER_DELTA_1">0</xsl:variable>

	<xsl:include href="android_all_densities.xslt"/>
</xsl:transform>
