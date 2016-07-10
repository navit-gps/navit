<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
   <xsl:template match="/config/navit/osd[last()]">
      <xsl:copy><xsl:copy-of select="@*|node()"/></xsl:copy>
      <xsl:text>&#x0A;        </xsl:text>
      <osd type="button" src="gui_android_menu_{number($ICON_BIG)}_{number($ICON_BIG)}.png" command="graphics.menu()" x="{round(-60*number($OSD_SIZE))}" y="{round(48*number($OSD_SIZE))}" enable_expression="!has_menu_button" use_overlay="yes"/>
   </xsl:template>
</xsl:transform>
