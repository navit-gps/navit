<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
   <xsl:template match="/config/navit/osd[last()]">
      <xsl:copy><xsl:copy-of select="@*|node()"/></xsl:copy>
      <xsl:text>&#x0A;        </xsl:text>
      <osd type="button" src="gui_android_menu" command="graphics.menu()" x="{round(-(number($ICON_BIG)+8*number($OSD_SIZE)))}" y="{round(62*number($OSD_SIZE))}" w="{round(number($ICON_MEDIUM))}" h="{round(number($ICON_MEDIUM))}" enable_expression="!has_menu_button" use_overlay="yes"/>
   </xsl:template>
</xsl:transform>
