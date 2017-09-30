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

   <xsl:template match="/config/navit/mapset[1]">
		<mapset enabled="yes">
         <xsl:text>&#x0A; 			</xsl:text>
			<map type="binfile" enabled="yes" active="no" data="/usr/share/harbour-navit/maps/osm_bbox_11.3,47.9,11.7,48.2.bin"/>
         <xsl:text>&#x0A; 			</xsl:text>
			<map type="binfile" enabled="yes" data="~/Maps/map.navit.bin"/>
         <xsl:text>&#x0A; 			</xsl:text>
			<map type="binfile" enabled="yes" active="no" name="map.navit.heightlines.bin" data="~/Maps/map.navit.heightlines.bin"/>
         <xsl:text>&#x0A; 		</xsl:text>
		</mapset>
      <xsl:text>&#x0A; 		</xsl:text>
      <xsl:copy>
         <xsl:apply-templates select="@*"/>
         <xsl:apply-templates select="node()"/>
      </xsl:copy>
   </xsl:template>

</xsl:stylesheet>
