<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
	<xsl:param name="OSD_SIZE" select="2"/>
	<xsl:param name="ICON_SMALL" select="48"/>
	<xsl:param name="ICON_MEDIUM" select="48"/>
	<xsl:param name="ICON_BIG" select="96"/>
	<xsl:param name="OSD_USE_OVERLAY">yes</xsl:param>

        <xsl:output method="xml" doctype-system="navit.dtd" cdata-section-elements="gui"/>
	<xsl:include href="navit_drag_bitmap.xslt"/>
	<xsl:include href="osd_minimum.xslt"/>
        <xsl:template match="@*|node()"><xsl:copy><xsl:apply-templates select="@*|node()"/></xsl:copy></xsl:template>
</xsl:transform>
