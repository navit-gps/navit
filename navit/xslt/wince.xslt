<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
	<xsl:param name="OSD_SIZE" select="1"/>
	<xsl:param name="ICON_SMALL" select="24"/>
	<xsl:param name="ICON_MEDIUM" select="24"/>
	<xsl:param name="ICON_BIG" select="48"/>
	<xsl:param name="OSD_USE_OVERLAY">yes</xsl:param>

	<xsl:output method="xml" doctype-system="navit.dtd" cdata-section-elements="gui"/>
	<xsl:include href="osd_minimum.xslt"/>
	<xsl:template match="/config/navit/vehicle[@enabled='yes']">
		<xsl:copy>
			<xsl:copy-of select="@*[name() != 'gpsd_query']"/>
				<xsl:attribute name="source">wince:COM2:</xsl:attribute>
				<xsl:attribute name="baudrate">4800</xsl:attribute>
			<xsl:apply-templates/>
		</xsl:copy>
	</xsl:template>
	<xsl:template match="@*|node()"><xsl:copy><xsl:apply-templates select="@*|node()"/></xsl:copy></xsl:template>
</xsl:transform>
