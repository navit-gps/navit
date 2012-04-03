<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
   <xsl:param name="OSD_SIZE" select="1.33"/>
   <xsl:param name="ICON_SMALL" select="32"/>
   <xsl:param name="ICON_MEDIUM" select="32"/>
   <xsl:param name="ICON_BIG" select="64"/>

   <xsl:output method="xml" doctype-system="navit.dtd" cdata-section-elements="gui"/>
   <xsl:include href="default_plugins.xslt"/>
   <xsl:include href="map_sdcard_navitmap_bin.xslt"/>
   <xsl:include href="osd_minimum.xslt"/>
   <xsl:include href="cursor_scale.xslt"/>
   <xsl:template match="/config/plugins/plugin[1]" priority="100">
      <plugin path="$NAVIT_PREFIX/lib/lib*.so" ondemand="yes"/>
      <xsl:text>&#x0A;        </xsl:text>
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
   <xsl:template match="/config/navit/gui[2]">
      <xsl:copy>
         <xsl:copy-of select="@*[not(name()='font_size')]"/>
         <xsl:attribute name="font_size"><xsl:value-of select="round(185*number($OSD_SIZE))"/></xsl:attribute>
         <xsl:attribute name="icon_xs"><xsl:value-of select="number($ICON_SMALL)"/></xsl:attribute>
         <xsl:attribute name="icon_s"><xsl:value-of select="number($ICON_MEDIUM)"/></xsl:attribute>
         <xsl:attribute name="icon_l"><xsl:value-of select="number($ICON_BIG)"/></xsl:attribute>
         <xsl:attribute name="spacing"><xsl:value-of select="round(2*number($OSD_SIZE))"/></xsl:attribute>
         <xsl:apply-templates/>
      </xsl:copy>
   </xsl:template>
   <xsl:template match="/config/navit[1]">
      <xsl:copy>
         <xsl:copy-of select="@*"/>
         <xsl:attribute name="zoom">32</xsl:attribute>
         <xsl:apply-templates/>
      </xsl:copy>
   </xsl:template>
   <xsl:template match="/config/navit/vehicle[1]">
      <xsl:copy><xsl:copy-of select="@*[not(name()='gpsd_query')]"/>
      <xsl:attribute name="source">android:</xsl:attribute>
      <xsl:attribute name="follow">1</xsl:attribute>
      <xsl:apply-templates/></xsl:copy>
   </xsl:template>
   <xsl:template match="/config/navit/speech">
      <xsl:copy>
         <xsl:copy-of select="@*[not(name()='data')]"/>
         <xsl:attribute name="type">android</xsl:attribute>
         <xsl:apply-templates/>
      </xsl:copy>
   </xsl:template>
   <xsl:template match="/config/navit/layout[@name='Car-Android']">
      <xsl:copy>
         <xsl:copy-of select="@*"/>
         <xsl:attribute name="active">1</xsl:attribute>
         <xsl:apply-templates/>
      </xsl:copy>
   </xsl:template>
   <xsl:template match="@*|node()">
      <xsl:copy><xsl:apply-templates select="@*|node()"/></xsl:copy>
   </xsl:template>
</xsl:transform>
