<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
        <xsl:output method="xml" doctype-system="navit.dtd" cdata-section-elements="gui"/>
        <xsl:include href="samplemap.xslt"/>
        <xsl:include href="osd_enable_zoom.xslt"/>
        <xsl:include href="speech_win32.xslt"/>
        <xsl:template match="/config/navit/graphics">
                <graphics type="win32" />
        </xsl:template>
        <xsl:template match="@*|node()"><xsl:copy><xsl:apply-templates select="@*|node()"/></xsl:copy></xsl:template>
</xsl:transform>

