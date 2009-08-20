<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
        <xsl:output method="xml" doctype-system="navit.dtd" />
	<xsl:include href="default_plugins.xslt"/>
	<xsl:include href="gui_internal.xslt"/>
	<xsl:include href="map_sdcard_navitmap_bin.xslt"/>
        <xsl:template match="/config/plugins/plugin[1]" priority="100">
		<plugin path="$NAVIT_PREFIX/lib/libgraphics_android.so" ondemand="no"/>
		<xsl:text>&#x0A;        </xsl:text>
		<plugin path="$NAVIT_PREFIX/lib/libvehicle_android.so" ondemand="no"/>
		<xsl:text>&#x0A;        </xsl:text>
		<plugin path="$NAVIT_PREFIX/lib/libspeech_android.so" enabled="yes" ondemand="no"/>
		<xsl:text>&#x0A;        </xsl:text>
		<xsl:next-match/>
        </xsl:template>
        <xsl:template match="/config/navit/graphics">
                <graphics type="android" />
        </xsl:template>
        <xsl:template match="/config/navit/osd[1]">
		<osd type="compass" enabled="yes" x="-60" y="-80"/>
		<xsl:text>&#x0A;        </xsl:text>
		<osd type="text" label="${{navigation.item.destination_length[named]}}\n${{navigation.item.destination_time[arrival]}}" x="-60" y="0" w="60" h="40"/>
		<xsl:text>&#x0A;        </xsl:text>
		<osd type="navigation_next_turn" x="0" y="-60" w="60" h="40" icon_src="%s_wh_44_44.png"/>
		<xsl:text>&#x0A;        </xsl:text>
		<osd type="text" label="${{navigation.item[1].length[named]}}" x="0" y="-20"/>
		<xsl:text>&#x0A;        </xsl:text>
		<osd type="button" src="gui_zoom_in_64_64.png" command="zoom_in()" x="0" y="0"/>
		<xsl:text>&#x0A;        </xsl:text>
		<osd type="button" src="gui_zoom_out_64_64.png" command="zoom_out()" x="0" y="70"/>
		<xsl:text>&#x0A;        </xsl:text>
                <xsl:copy><xsl:copy-of select="@*|node()"/></xsl:copy>
        </xsl:template>
        <xsl:template match="/config/navit/vehicle[1]">
                <xsl:copy><xsl:copy-of select="@*[not(name()='gpsd_query')]"/>
			<xsl:attribute name="source">android:</xsl:attribute>
			<xsl:attribute name="follow">1</xsl:attribute>
		<xsl:apply-templates/></xsl:copy>
	</xsl:template>
        <xsl:template match="/config/navit/speech">
                <xsl:copy><xsl:copy-of select="@*[not(name()='data')]"/><xsl:attribute name="type">android</xsl:attribute><xsl:apply-templates/></xsl:copy>
	</xsl:template>
        <xsl:template match="@*|node()"><xsl:copy><xsl:apply-templates select="@*|node()"/></xsl:copy></xsl:template>
</xsl:transform>
