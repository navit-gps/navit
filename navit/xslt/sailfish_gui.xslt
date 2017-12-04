<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 xmlns:xi="http://www.w3.org/2001/XInclude">

	<xsl:output method="xml" indent="yes" cdata-section-elements="gui" doctype-system="navit.dtd"/>

   <!-- Default rule: copy all -->
   <xsl:template match="node()|@*">
      <xsl:copy>
         <xsl:apply-templates select="node()|@*"/>
      </xsl:copy>
   </xsl:template>

   <xsl:template match="/config/navit">
      <xsl:copy>
         <xsl:apply-templates select="@*"/>
         <xsl:attribute name="drag_bitmap">1</xsl:attribute>
         <xsl:apply-templates select="node()"/>
      </xsl:copy>
   </xsl:template>

   <xsl:template match="/config/navit/gui[@type='internal']">
      <xsl:copy>
         <xsl:apply-templates select="@*"/>
         <xsl:attribute name="font_size">350</xsl:attribute>
         <xsl:attribute name="icon_xs">32</xsl:attribute>
         <xsl:attribute name="icon_s">64</xsl:attribute>
         <xsl:attribute name="icon_l">96</xsl:attribute>
         <xsl:attribute name="enabled">yes</xsl:attribute>
         <xsl:apply-templates select="node()"/>
      </xsl:copy>
   </xsl:template>

</xsl:stylesheet>
