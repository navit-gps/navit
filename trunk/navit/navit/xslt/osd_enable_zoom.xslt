<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
	<xsl:template match="/config/navit/osd[@type='button'][@command='zoom_in()' or @command='zoom_out()']">
		<xsl:copy><xsl:copy-of select="@*"/><xsl:attribute name="enabled">yes</xsl:attribute><xsl:apply-templates/></xsl:copy>
	</xsl:template>
</xsl:transform>
