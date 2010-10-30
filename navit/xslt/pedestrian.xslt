<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
	<xsl:output method="xml" doctype-system="navit.dtd" cdata-section-elements="gui"/>
	<xsl:template match="/config/navit">
		<xsl:copy><xsl:copy-of select="@*"/><xsl:attribute name="osd_configuration">1</xsl:attribute><xsl:apply-templates/></xsl:copy>
        </xsl:template>
        <xsl:template match="/config/navit/osd[1]">
		<osd type="auxmap" w="160" h="120" x="0" y="0" osd_configuration="2"/>
		<xsl:text>&#x0A;        </xsl:text>
		<osd enabled="yes" type="text" label="${{navigation.item.street_name}}" x="0" y="-40" w="480" h="40" font_size="266" osd_configuration="2"/>
		<xsl:text>&#x0A;        </xsl:text>
		<osd enabled="yes" type="button" x="-96" y="0" command="gui.menu()" src="menu.png" osd_configuration="2"/>
		<xsl:text>&#x0A;        </xsl:text>
		<osd enabled="yes" type="button" x="-192" y="0" command="pedestrian_rocket()" src="rocket.png" osd_configuration="2"/>
		<xsl:text>&#x0A;        </xsl:text>
                <xsl:copy><xsl:copy-of select="@*|node()"/></xsl:copy>
        </xsl:template>
	<xsl:template match="@*|node()"><xsl:copy><xsl:apply-templates select="@*|node()"/></xsl:copy></xsl:template>
</xsl:transform>
