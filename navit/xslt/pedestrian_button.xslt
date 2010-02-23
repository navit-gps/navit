<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
        <xsl:output method="xml" doctype-system="navit.dtd" cdata-section-elements="gui"/>
        <xsl:template match="/config/navit/gui[@type='internal']">
		<xsl:copy><xsl:copy-of select="@*"/>
		<xsl:value-of select="substring-before(text(),'&lt;/html&gt;')"/>
		<xsl:text>
		<![CDATA[
			<a name='Plugins'>
			<img src='gui_settings' onclick='plugin=new plugin(path="/data/data/org.navitproject.navit/lib/libplugin_pedestrian.so")'><text>Pedestrian</text></img>
			</a>
			</html>
		]]>
		</xsl:text>
		</xsl:copy>
        </xsl:template>
	<xsl:template match="@*|node()"><xsl:copy><xsl:apply-templates select="@*|node()"/></xsl:copy></xsl:template>
</xsl:transform>
