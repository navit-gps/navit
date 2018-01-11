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

   <!-- Force all png icons to svg and add w and h if not exist -->
   <xsl:template match="icon">
      <xsl:copy>
         <xsl:apply-templates select="@*"/>
         <xsl:if test="@src[substring(., string-length()-3)='.png']|@src[substring(., string-length()-3)='.xpm']">
         <xsl:attribute name="src">
            <xsl:value-of select="concat(substring(@src,1, string-length(@src)-3),'svg')"/>
         </xsl:attribute>
         </xsl:if>
         <xsl:if test="not(@h)">
            <xsl:attribute name="h">15</xsl:attribute>
         </xsl:if>
         <xsl:if test="not(@w)">
            <xsl:attribute name="w">15</xsl:attribute>
         </xsl:if>
         <xsl:apply-templates select="node()"/>
      </xsl:copy>
   </xsl:template>

</xsl:stylesheet>
