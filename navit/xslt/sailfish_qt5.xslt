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

   <xsl:template match="/config/navit/graphics[1]">
      <graphics type="qt5" qt5_platform="wayland" virtual_dpi="245"/>
      <xsl:text>&#x0A; 		</xsl:text>
      <xsl:copy>
         <xsl:apply-templates select="@*"/>
         <xsl:apply-templates select="node()"/>
      </xsl:copy>
   </xsl:template>

   <xsl:template match="/config/navit/vehicle[1]">
		<vehicle name="Qt5" profilename="car" enabled="yes" active="1" source="qt5://"/>
      <xsl:text>&#x0A; 		</xsl:text>
      <xsl:copy>
         <xsl:apply-templates select="@*"/>
         <xsl:apply-templates select="node()"/>
      </xsl:copy>
   </xsl:template>

   <xsl:template match="/config/navit/speech[1]">
		<speech type="qt5_espeak" cps="15"/>
      <xsl:text>&#x0A; 		</xsl:text>
      <xsl:copy>
         <xsl:apply-templates select="@*"/>
         <xsl:apply-templates select="node()"/>
      </xsl:copy>
   </xsl:template>

</xsl:stylesheet>
