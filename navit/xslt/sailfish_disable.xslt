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

   <!-- Disable all vehicles -->
   <xsl:template match="vehicle">
      <xsl:copy>
         <xsl:apply-templates select="@*"/>
         <xsl:attribute name="enabled">no</xsl:attribute>
         <xsl:apply-templates select="node()"/>
      </xsl:copy>
   </xsl:template>
   <!-- Disable all graphics -->
   <xsl:template match="graphics">
      <xsl:copy>
         <xsl:apply-templates select="@*"/>
         <xsl:attribute name="enabled">no</xsl:attribute>
         <xsl:apply-templates select="node()"/>
      </xsl:copy>
   </xsl:template>
   <!-- Disable all mapsets -->
   <xsl:template match="mapset">
      <xsl:copy>
         <xsl:apply-templates select="@*"/>
         <xsl:attribute name="enabled">no</xsl:attribute>
         <xsl:apply-templates select="node()"/>
      </xsl:copy>
   </xsl:template>
   <!-- Disable all osd -->
   <xsl:template match="osd">
      <xsl:copy>
         <xsl:apply-templates select="@*"/>
         <xsl:attribute name="enabled">no</xsl:attribute>
         <xsl:apply-templates select="node()"/>
      </xsl:copy>
   </xsl:template>
   <!-- Disable all speech -->
   <xsl:template match="speech">
      <xsl:copy>
         <xsl:apply-templates select="@*"/>
         <xsl:attribute name="enabled">no</xsl:attribute>
         <xsl:apply-templates select="node()"/>
      </xsl:copy>
   </xsl:template>

</xsl:stylesheet>
