<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
        <xsl:template match="/config/plugins/plugin[1]" priority="1" >
		<plugin path="$NAVIT_PREFIX/lib/libgui_internal.so" ondemand="no"/>
		<xsl:text>&#x0A;        </xsl:text>
		<plugin path="$NAVIT_PREFIX/lib/libmap_textfile.so" ondemand="no"/>
		<xsl:text>&#x0A;        </xsl:text>
		<plugin path="$NAVIT_PREFIX/lib/libmap_binfile.so" ondemand="no"/>
		<xsl:text>&#x0A;        </xsl:text>
		<plugin path="$NAVIT_PREFIX/lib/libosd_core.so" ondemand="no"/>
        </xsl:template>
</xsl:transform>
