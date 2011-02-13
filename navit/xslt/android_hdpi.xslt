<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
        <xsl:output method="xml" doctype-system="navit.dtd" cdata-section-elements="gui"/>
        <xsl:variable name="OSD_SIZE">1.33</xsl:variable>
        <xsl:variable name="OSD_FACTOR">1.5</xsl:variable>
        <xsl:variable name="CAR_FACTOR">2</xsl:variable>
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
	<xsl:include href="map_sdcard_navitmap_bin.xslt"/>
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
			<xsl:attribute name="font_size">470</xsl:attribute>
			<xsl:attribute name="icon_xs">32</xsl:attribute>
			<xsl:attribute name="icon_s">64</xsl:attribute>
			<xsl:attribute name="icon_l">64</xsl:attribute>
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
	<!-- make arrow bigger -->
        <xsl:template match="/config/navit/layout/cursor">
        <cursor w="{round(30*number($CAR_FACTOR_)*number($OSD_FACTOR_))}" h="{round(32*number($CAR_FACTOR_)*number($OSD_FACTOR_))}">
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
        </cursor>
	</xsl:template>
	<!-- make arrow bigger -->
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
