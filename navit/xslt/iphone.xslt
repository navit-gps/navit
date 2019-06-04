<?xml version="1.0"?>
<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
   <xsl:param name="OSD_SIZE" select="1.33"/>
   <xsl:param name="ICON_SMALL" select="32"/>
   <xsl:param name="ICON_MEDIUM" select="32"/>
   <xsl:param name="ICON_BIG" select="64"/>
   <xsl:param name="OSD_USE_OVERLAY">yes</xsl:param>

   <xsl:output method="xml" doctype-system="navit.dtd" cdata-section-elements="gui"/>
   <xsl:include href="osd_minimum.xslt"/>
   <xsl:template match="/config/navit/graphics">
      <graphics type="cocoa" />
   </xsl:template>
   <xsl:template match="/config/navit[1]">
      <xsl:copy>
         <xsl:copy-of select="@*"/>
         <xsl:attribute name="timeout">86400</xsl:attribute>
         <xsl:apply-templates/>
      </xsl:copy>
   </xsl:template>
   <xsl:template match="/config/navit/vehicle[1]">
      <xsl:copy><xsl:copy-of select="@*[not(name()='gpsd_query')]"/>
      <xsl:attribute name="source">iphone:</xsl:attribute>
      <xsl:attribute name="follow">1</xsl:attribute>
      <xsl:apply-templates/></xsl:copy>
   </xsl:template>
   <xsl:template match="/config/navit/speech">
      <xsl:copy>
         <xsl:copy-of select="@*[not(name()='data')]"/>
         <xsl:attribute name="type">iphone</xsl:attribute>
         <xsl:apply-templates/>
      </xsl:copy>
   </xsl:template>
   <xsl:template match="/config/navit/mapset/xi:include">
      <map type="binfile" enabled="yes" data="/var/mobile/navit/navitmap.bin" />
      <map type="binfile" enabled="yes" data="../Documents/navitmap.bin" />
   </xsl:template>
   <xsl:template match="@*|node()">
      <xsl:copy><xsl:apply-templates select="@*|node()"/></xsl:copy>
   </xsl:template>
</xsl:transform>
