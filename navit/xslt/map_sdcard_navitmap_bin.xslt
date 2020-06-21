<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
	<xsl:template match="/config/navit/mapset/xi:include">
		<maps type="binfile" data="$NAVIT_USER_DATADIR/*.bin" />
		<xsl:text>&#x0A;                        </xsl:text>
		<maps type="binfile" data="/sdcard/Download/osm_bbox_*.bin" />
	</xsl:template>
</xsl:transform>
