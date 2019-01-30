<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 xmlns:xi="http://www.w3.org/2001/XInclude">
	<xsl:param name="OSD_SIZE" select="2"/>
	<xsl:param name="ICON_SMALL" select="32"/>
	<xsl:param name="ICON_MEDIUM" select="64"/>
	<xsl:param name="ICON_BIG" select="96"/>
	<xsl:param name="OSD_USE_OVERLAY">yes</xsl:param>

	<xsl:output method="xml" indent="yes" cdata-section-elements="gui" doctype-system="navit.dtd"/>
	<xsl:include href="osd_minimum.xslt"/>

   <!-- Default rule: copy all -->
   <xsl:template match="node()|@*">
      <xsl:copy>
         <xsl:apply-templates select="node()|@*"/>
      </xsl:copy>
   </xsl:template>

</xsl:stylesheet>
