<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
   <xsl:template match="/config/navit/layout/cursor">
      <xsl:param name="OLD_W"><xsl:value-of select="@w"/></xsl:param>
      <xsl:param name="OLD_H"><xsl:value-of select="@h"/></xsl:param>
      <xsl:copy>
         <xsl:copy-of select="@*"/>
         <xsl:attribute name="w"><xsl:value-of select="round($OLD_W*number($OSD_SIZE))"/></xsl:attribute>
         <xsl:attribute name="h"><xsl:value-of select="round($OLD_H*number($OSD_SIZE))"/></xsl:attribute>
         <xsl:apply-templates/>
      </xsl:copy>
   </xsl:template>
   <xsl:template match="/config/navit/layout/cursor/itemgra/circle">
      <xsl:param name="OLD_RADIUS"><xsl:value-of select="@radius"/></xsl:param>
      <xsl:param name="OLD_WIDTH"><xsl:value-of select="@width"/></xsl:param>
      <xsl:copy>
         <xsl:copy-of select="@*"/>
         <xsl:attribute name="radius"><xsl:value-of select="round($OLD_RADIUS*number($OSD_SIZE))"/></xsl:attribute>
         <xsl:attribute name="width"><xsl:value-of select="round($OLD_WIDTH*number($OSD_SIZE))"/></xsl:attribute>
         <xsl:apply-templates/>
      </xsl:copy>
   </xsl:template>
   <xsl:template match="/config/navit/layout/cursor/itemgra/polyline">
      <xsl:param name="OLD_WIDTH"><xsl:value-of select="@width"/></xsl:param>
      <xsl:copy>
         <xsl:copy-of select="@*"/>
         <xsl:attribute name="width"><xsl:value-of select="round($OLD_WIDTH*number($OSD_SIZE))"/></xsl:attribute>
         <xsl:apply-templates/>
      </xsl:copy>
   </xsl:template>
   <xsl:template match="/config/navit/layout/cursor/itemgra/*/coord">
      <xsl:param name="OLD_X"><xsl:value-of select="@x"/></xsl:param>
      <xsl:param name="OLD_Y"><xsl:value-of select="@y"/></xsl:param>
      <xsl:copy>
         <xsl:copy-of select="@*"/>
         <xsl:attribute name="x"><xsl:value-of select="round(number($OLD_X)*number($OSD_SIZE))"/></xsl:attribute>
         <xsl:attribute name="y"><xsl:value-of select="round(number($OLD_Y)*number($OSD_SIZE))"/></xsl:attribute>
         <xsl:apply-templates/>
      </xsl:copy>
   </xsl:template>
</xsl:transform>
