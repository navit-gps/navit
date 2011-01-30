<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
        <xsl:template match="/config/navit/osd[1]">
		<xsl:variable name="OSD_SIZE_">
			<xsl:choose>
				<xsl:when test="string-length($OSD_SIZE)"><xsl:value-of select="$OSD_SIZE"/></xsl:when>
				<xsl:otherwise>1</xsl:otherwise>
			</xsl:choose>
		</xsl:variable>
		<xsl:variable name="OSD_FACTOR_">
			<xsl:choose>
				<xsl:when test="string-length($OSD_FACTOR)"><xsl:value-of select="$OSD_FACTOR"/></xsl:when>
				<xsl:otherwise>1</xsl:otherwise>
			</xsl:choose>
		</xsl:variable>
		<osd type="compass" enabled="yes" x="{round(-60*number($OSD_SIZE_))}" y="{round(-80*number($OSD_SIZE_))}" w="{round(60*number($OSD_SIZE_))}" h="{round(80*number($OSD_SIZE_))}" font_size="{round(200*number($OSD_SIZE_))}" osd_configuration="1"/>
		<xsl:text>&#x0A;        </xsl:text>
		<osd type="text" label="${{navigation.item.destination_length[named]}}\n${{navigation.item.destination_time[arrival]}}" x="{round(-60*number($OSD_SIZE_))}" y="0" w="{round(60*number($OSD_SIZE_))}" h="{round(40*number($OSD_SIZE_))}" font_size="{round(200*number($OSD_SIZE_))}" osd_configuration="1"/>
		<xsl:text>&#x0A;        </xsl:text>
		<osd type="navigation_next_turn" x="0" y="{round(-60*number($OSD_SIZE_))}" w="{round(60*number($OSD_SIZE_))}" h="{round(40*number($OSD_SIZE_))}" icon_src="%s_wh_{round(44*number($OSD_SIZE_))}_{round(44*number($OSD_SIZE_))}.png" osd_configuration="1"/>
		<xsl:text>&#x0A;        </xsl:text>
		<osd type="text" label="${{navigation.item[1].length[named]}}" x="0" y="{round(-20*number($OSD_SIZE_))}" w="{round(60*number($OSD_SIZE_))}" h="{round(20*number($OSD_SIZE_))}" font_size="{round(200*number($OSD_SIZE_))}" osd_configuration="1"/>
		<xsl:text>&#x0A;        </xsl:text>
		<osd type="button" src="gui_zoom_in_{round(48*number($OSD_SIZE_))}_{round(48*number($OSD_SIZE_))}.png" command="zoom_in()" x="0" y="0" osd_configuration="1"/>
		<xsl:text>&#x0A;        </xsl:text>
		<osd type="button" src="gui_zoom_out_{round(48*number($OSD_SIZE_))}_{round(48*number($OSD_SIZE_))}.png" command="zoom_out()" x="0" y="{ round(round(53*number($OSD_SIZE_))*number($OSD_FACTOR_))}" osd_configuration="1"/>
		<xsl:text>&#x0A;        </xsl:text>
                <xsl:copy><xsl:copy-of select="@*|node()"/></xsl:copy>
        </xsl:template>
</xsl:transform>
