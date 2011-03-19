<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
   <xsl:template match="/config/navit/osd[1]">
      <xsl:param name="NEXT_TURN_SIZE"><xsl:value-of select="round(12*number($OSD_SIZE)+number($ICON_BIG))"/></xsl:param>
      <xsl:param name="NEXT_TURN_TEXT_HIGHT"><xsl:value-of select="round(20*number($OSD_SIZE))"/></xsl:param>
      
      <osd type="compass" enabled="yes" x="{round(-60*number($OSD_SIZE))}" y="{round(-80*number($OSD_SIZE))}" w="{round(60*number($OSD_SIZE))}" h="{round(80*number($OSD_SIZE))}" font_size="{round(200*number($OSD_SIZE))}" osd_configuration="1"/>
      <xsl:text>&#x0A;        </xsl:text>
      <osd type="text" label="${{navigation.item.destination_length[named]}}\n${{navigation.item.destination_time[arrival]}}" x="{round(-60*number($OSD_SIZE))}" y="0" w="{round(60*number($OSD_SIZE))}" h="{round(40*number($OSD_SIZE))}" font_size="{round(200*number($OSD_SIZE))}" osd_configuration="1"/>
      <xsl:text>&#x0A;        </xsl:text>
      <osd type="navigation_next_turn" x="0" y="{-($NEXT_TURN_SIZE+$NEXT_TURN_TEXT_HIGHT)}" w="{$NEXT_TURN_SIZE+$NEXT_TURN_TEXT_HIGHT}" h="{$NEXT_TURN_SIZE}" icon_src="%s_wh_{$ICON_BIG}_{$ICON_BIG}.png" osd_configuration="1"/>
      <xsl:text>&#x0A;        </xsl:text>
      <osd type="text" label="${{navigation.item[1].length[named]}}" x="0" y="{-$NEXT_TURN_TEXT_HIGHT}" w="{$NEXT_TURN_SIZE+$NEXT_TURN_TEXT_HIGHT}" h="{$NEXT_TURN_TEXT_HIGHT}" font_size="{round(200*number($OSD_SIZE))}" osd_configuration="1"/>
      <xsl:text>&#x0A;        </xsl:text>
      <osd type="button" src="gui_zoom_in_{number($ICON_BIG)}_{number($ICON_BIG)}.png" command="zoom_in()" x="0" y="0" osd_configuration="1"/>
      <xsl:text>&#x0A;        </xsl:text>
      <osd type="button" src="gui_zoom_out_{number($ICON_BIG)}_{number($ICON_BIG)}.png" command="zoom_out()" x="0" y="{round(number($ICON_BIG)+8*number($OSD_SIZE))}" osd_configuration="1"/>
      <xsl:text>&#x0A;        </xsl:text>
      <xsl:copy><xsl:copy-of select="@*|node()"/></xsl:copy>
   </xsl:template>
</xsl:transform>
