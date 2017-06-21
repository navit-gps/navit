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

   <xsl:template match="/config/navit/osd[1]">
      <osd enabled="yes" type="gps_status" x="0" y="0" w="50" h="50"/>
      <xsl:text>&#x0A; 		</xsl:text>
      <osd enabled="no" type="text" label="${{vehicle.position_sats_used}}/${{vehicle.position_qual}}" x="0" y="0" w="50" h="50" background_color="#00000058" font_size="400"/>
      <xsl:text>&#x0A; 		</xsl:text>
      <osd enabled="yes" type="text" label="${{vehicle.position_speed}}" x="50" y="0" w="150" h="50" background_color="#00000058" font_size="400"/>
      <xsl:text>&#x0A; 		</xsl:text>
      <osd enabled="yes" type="text" label="${{navigation.item.destination_length[named]}}" x="200" y="0" w="240" h="50" background_color="#00000058" font_size="400"/>
      <xsl:text>&#x0A; 		</xsl:text>
      <osd enabled="yes" type="navigation_next_turn" x="-100" y="0" w="100" h="100" icon_w="90" icon_h="80" background_color="#00000058"/>
      <xsl:text>&#x0A; 		</xsl:text>
		<osd enabled="yes" type="button" x="-96" y="-96" w="96" h="96" command="zoom_in()" src="zoom_in" use_overlay="true"/>
      <xsl:text>&#x0A; 		</xsl:text>
      <osd enabled="yes" type="text" label="${{navigation.item[1].length[named]}}" x="-100" y="100" w="100" h="50" background_color="#00000058" font_size="400"/>
      <xsl:text>&#x0A; 		</xsl:text>
		<osd enabled="yes" type="button" x="0" y="-96" w="96" h="96" command="zoom_out()" src="zoom_out" use_overlay="true"/>
      <xsl:text>&#x0A; 		</xsl:text>
      <xsl:copy>
         <xsl:apply-templates select="@*"/>
         <xsl:apply-templates select="node()"/>
      </xsl:copy>
   </xsl:template>

</xsl:stylesheet>
