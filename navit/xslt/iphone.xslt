<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
   <xsl:param name="OSD_SIZE" select="1.33"/>
   <xsl:param name="ICON_SMALL" select="32"/>
   <xsl:param name="ICON_MEDIUM" select="32"/>
   <xsl:param name="ICON_BIG" select="64"/>
   
   <xsl:output method="xml" doctype-system="navit.dtd" cdata-section-elements="gui"/>
   <xsl:include href="osd_minimum.xslt"/>
   <xsl:template match="/config/navit/graphics">
      <graphics type="cocoa" />
   </xsl:template>
   <xsl:template match="/config/navit/vehicle[1]">
      <xsl:copy><xsl:copy-of select="@*[not(name()='gpsd_query')]"/>
      <xsl:attribute name="source">iphone:</xsl:attribute>
      <xsl:attribute name="follow">1</xsl:attribute>
      <xsl:apply-templates/></xsl:copy>
   </xsl:template>
   <xsl:template match="@*|node()">
      <xsl:copy><xsl:apply-templates select="@*|node()"/></xsl:copy>
   </xsl:template>
</xsl:transform>
