<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
	<xsl:template match="/config/navit/vehicle[@name='Local GPS']">
                <xsl:copy><xsl:copy-of select="@*[not(name()='data')]"/><xsl:attribute name="source">null:</xsl:attribute><xsl:apply-templates/></xsl:copy>
	</xsl:template>
</xsl:transform>
