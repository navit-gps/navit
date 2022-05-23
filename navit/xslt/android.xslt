<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
   <xsl:param name="OSD_SIZE" select="1.33"/>
   <xsl:param name="ICON_SMALL" select="32"/>
   <xsl:param name="ICON_MEDIUM" select="32"/>
   <xsl:param name="ICON_BIG" select="64"/>
   <xsl:param name="OSD_USE_OVERLAY">yes</xsl:param>

   <xsl:output method="xml" doctype-system="navit.dtd" cdata-section-elements="gui"/>
   <xsl:include href="default_plugins.xslt"/>
   <xsl:include href="map_sdcard_navitmap_bin.xslt"/>
   <xsl:include href="osd_minimum.xslt"/>
   <xsl:include href="osd_android.xslt"/>
   <xsl:include href="cursor_scale.xslt"/>
   <xsl:template match="/config/plugins/plugin[1]" priority="100">
      <plugin path="$NAVIT_PREFIX/lib/lib*.so" ondemand="yes"/>
      <xsl:text>&#x0A;        </xsl:text>
      <plugin path="$NAVIT_PREFIX/lib/libgraphics_android.so" ondemand="no"/>
      <xsl:text>&#x0A;        </xsl:text>
      <plugin path="$NAVIT_PREFIX/lib/libvehicle_android.so" ondemand="no"/>
      <xsl:text>&#x0A;        </xsl:text>
      <plugin path="$NAVIT_PREFIX/lib/libspeech_android.so" ondemand="no"/>
      <xsl:text>&#x0A;        </xsl:text>
      <xsl:next-match/>
   </xsl:template>
   <xsl:template match="/config/navit/graphics">
      <graphics type="android" />
   </xsl:template>
   <xsl:template match="/config/navit/gui[2]">
      <xsl:copy>
         <xsl:copy-of select="@*[not(name()='font_size')]"/>
         <xsl:attribute name="font_size"><xsl:value-of select="round(185*number($OSD_SIZE))"/></xsl:attribute>
         <xsl:attribute name="icon_xs"><xsl:value-of select="number($ICON_SMALL)"/></xsl:attribute>
         <xsl:attribute name="icon_s"><xsl:value-of select="number($ICON_MEDIUM)"/></xsl:attribute>
         <xsl:attribute name="icon_l"><xsl:value-of select="number($ICON_BIG)"/></xsl:attribute>
         <xsl:attribute name="spacing"><xsl:value-of select="round(2*number($OSD_SIZE))"/></xsl:attribute>
	 <xsl:attribute name="keyboard">false</xsl:attribute>
         <xsl:apply-templates/>
      </xsl:copy>
   </xsl:template>
   <xsl:template match="/config/navit/gui[2]/text()">
		<xsl:value-of select="replace(/config/navit/gui[2]/text(),
		    '&lt;a name=''Tools''>&lt;text>Tools&lt;/text>','&lt;a name=''Tools''>&lt;text>Tools&lt;/text>
			&lt;img src=''gui_maps'' onclick=''navit.graphics.map_download_dialog();''>&lt;text>Map download&lt;/text>&lt;/img>
			&lt;img src=''gui_rules'' onclick=''navit.graphics.set_map_location();''>&lt;text>Set map location&lt;/text>&lt;/img>
			&lt;img src=''gui_rules'' onclick=''navit.graphics.backup_restore_dialog();''>&lt;text>Backup / Restore&lt;/text>&lt;/img>')"/>
   </xsl:template>
   <xsl:template match="/config/navit/traffic">
      <traffic type="traff_android"/>
   </xsl:template>

   <xsl:template match="/config/navit[1]">
      <xsl:copy>
         <xsl:copy-of select="@*"/>
         <xsl:attribute name="zoom">32</xsl:attribute>
         <xsl:attribute name="autozoom_active">1</xsl:attribute>
         <xsl:attribute name="timeout">86400</xsl:attribute>
         <xsl:attribute name="drag_bitmap">1</xsl:attribute>
         <xsl:apply-templates/>
      </xsl:copy>
   </xsl:template>
   <xsl:template match="/config/navit/vehicle[1]">
      <xsl:copy><xsl:copy-of select="@*[not(name()='gpsd_query')]"/>
      <xsl:attribute name="source">android:</xsl:attribute>
      <xsl:attribute name="follow">1</xsl:attribute>
      <xsl:apply-templates/></xsl:copy>
   </xsl:template>
   <xsl:template match="/config/navit/speech">
      <xsl:copy>
         <xsl:copy-of select="@*[not(name()='data')]"/>
         <xsl:attribute name="type">android</xsl:attribute>
         <xsl:apply-templates/>
      </xsl:copy>
   </xsl:template>
   <xsl:template match="/config/navit/layout/layer/itemgra/child::*|/config/navit/layer/itemgra/child::*|layout/layer/itemgra/child::*">
      <xsl:copy>
         <xsl:copy-of select="@*[not(name()='text_size') and not(name()='width') and not(name()='radius') and not(name()='w') and not(name()='h') and not(name()='x') and not(name()='y') and not(name()='dash')]"/>
         <xsl:if test="@text_size">
		<xsl:attribute name="text_size"><xsl:value-of select="round(number(@text_size)*number($OSD_SIZE))"/></xsl:attribute>
	 </xsl:if>
         <xsl:if test="@width">
		<xsl:attribute name="width"><xsl:value-of select="round(number(@width)*number($OSD_SIZE))"/></xsl:attribute>
	 </xsl:if>
         <xsl:if test="@radius">
		<xsl:attribute name="radius"><xsl:value-of select="round(number(@radius)*number($OSD_SIZE))"/></xsl:attribute>
	 </xsl:if>
         <xsl:if test="name()='icon'">
		<xsl:attribute name="w"><xsl:value-of select="$ICON_SMALL"/></xsl:attribute>
		<xsl:attribute name="h"><xsl:value-of select="$ICON_SMALL"/></xsl:attribute>
	 </xsl:if>
         <xsl:if test="@w and not(name()='icon')">
		<xsl:attribute name="w"><xsl:value-of select="round(number(@w)*number($OSD_SIZE))"/></xsl:attribute>
	 </xsl:if>
         <xsl:if test="@h and not(name()='icon')">
		<xsl:attribute name="h"><xsl:value-of select="round(number(@h)*number($OSD_SIZE))"/></xsl:attribute>
	 </xsl:if>
         <xsl:apply-templates/>
         <xsl:if test="@x">
		<xsl:attribute name="x"><xsl:value-of select="round(number(@x)*number($OSD_SIZE))"/></xsl:attribute>
	 </xsl:if>
         <xsl:if test="@y">
		<xsl:attribute name="y"><xsl:value-of select="round(number(@y)*number($OSD_SIZE))"/></xsl:attribute>
	 </xsl:if>
         <xsl:if test="@offset">
		<xsl:attribute name="offset"><xsl:value-of select="round(number(@offset)*number($OSD_SIZE))"/></xsl:attribute>
	 </xsl:if>
	 <xsl:if test="@dash">
	 	<xsl:attribute name="dash">
		 	<xsl:for-each select="tokenize(@dash,',')">
		 		<xsl:value-of select="round(number(.)*number($OSD_SIZE))"/>
	 			<xsl:if test="not(position() eq last())"><xsl:text>,</xsl:text></xsl:if>
		 	</xsl:for-each>
 		</xsl:attribute>
	 </xsl:if>
      </xsl:copy>
   </xsl:template>
   <xsl:template match="/config/navit/layout|/layout">
      <xsl:copy>
         <xsl:copy-of select="@*"/>
         <xsl:if test="@name='Car'">
		<xsl:attribute name="active">1</xsl:attribute>
	 </xsl:if>
         <xsl:if test="number($OSD_SIZE)>3">
		<xsl:attribute name="order_delta">-2</xsl:attribute>
	 </xsl:if>
         <xsl:if test="number($OSD_SIZE)>1.4 and 3>=number($OSD_SIZE)">
		<xsl:attribute name="order_delta">-1</xsl:attribute>
	 </xsl:if>
         <xsl:apply-templates/>
      </xsl:copy>
   </xsl:template>
   <xsl:template match="@*|node()">
      <xsl:copy><xsl:apply-templates select="@*|node()"/></xsl:copy>
   </xsl:template>
</xsl:transform>
