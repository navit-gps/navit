<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
	<xsl:template match="/config/navit/gui[@type!='gtk']">
                <xsl:copy><xsl:copy-of select="@*"/><xsl:attribute name="enabled">no</xsl:attribute></xsl:copy>
                <xsl:copy><xsl:copy-of select="node()"/></xsl:copy>
	</xsl:template>
	<xsl:template match="/config/navit/gui[@type='gtk']">
                <xsl:copy><xsl:copy-of select="@*"/><xsl:attribute name="enabled">yes</xsl:attribute></xsl:copy>
                <xsl:copy><xsl:copy-of select="node()"/></xsl:copy>
	</xsl:template>
</xsl:transform>
