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

   <xsl:template match="/layout[@name='Car' or @name='Car-dark']/cursor">
      <cursor w="57" h="57">
        <xsl:text>&#x0A;				</xsl:text>
        <itemgra speed_range="-2">
        <xsl:text>&#x0A;					</xsl:text>
          <polyline color="#00BC00" radius="0" width="4">
          <xsl:text>&#x0A;						</xsl:text>
            <coord x="0" y="0"/>
            <xsl:text>&#x0A;					</xsl:text>
          </polyline>
          <xsl:text>&#x0A;					</xsl:text>
          <circle color="#008500" radius="9" width="3">
          <xsl:text>&#x0A;						</xsl:text>
            <coord x="0" y="0"/>
            <xsl:text>&#x0A;					</xsl:text>
          </circle>
          <xsl:text>&#x0A;					</xsl:text>
          <circle color="#00BC00" radius="13" width="3">
          <xsl:text>&#x0A;						</xsl:text>
            <coord x="0" y="0"/>
            <xsl:text>&#x0A;					</xsl:text>
          </circle>
          <xsl:text>&#x0A;					</xsl:text>
          <circle color="#008500" radius="18" width="3">
          <xsl:text>&#x0A;						</xsl:text>
            <coord x="0" y="0"/>
            <xsl:text>&#x0A;					</xsl:text>
       	  </circle>
          <xsl:text>&#x0A;				</xsl:text>
        </itemgra>
        <xsl:text>&#x0A;				</xsl:text>
        <itemgra speed_range="3-">
        <xsl:text>&#x0A;					</xsl:text>
          <polygon color="#00000066">
          <xsl:text>&#x0A;						</xsl:text>
            <coord x="-21" y="-27"/>
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="0" y="12"/>
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="21" y="-27"/>
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="0" y="-12"/>
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="-21" y="-27"/>
            <xsl:text>&#x0A;					</xsl:text>
          </polygon>
          <xsl:text>&#x0A;					</xsl:text>
          <polygon color="#008500">
          <xsl:text>&#x0A;						</xsl:text>
            <coord x="-21" y="-18"/>
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="0" y="21"/>
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="0" y="-3"/>
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="-21" y="-18"/>
            <xsl:text>&#x0A;					</xsl:text>
          </polygon>
          <xsl:text>&#x0A;					</xsl:text>
          <polygon color="#00BC00">
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="21" y="-18"/>
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="0" y="21"/>
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="0" y="-3"/>
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="21" y="-18"/>
            <xsl:text>&#x0A;					</xsl:text>
          </polygon>
          <xsl:text>&#x0A;					</xsl:text>
          <polyline color="#008500" width="1">
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="-21" y="-18"/>
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="0" y="21"/>
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="0" y="-3"/>
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="-21" y="-18"/>
            <xsl:text>&#x0A;					</xsl:text>
          </polyline>
          <xsl:text>&#x0A;					</xsl:text>
          <polyline color="#008500" width="1">
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="21" y="-18"/>
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="0" y="21"/>
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="0" y="-3"/>
            <xsl:text>&#x0A;						</xsl:text>
            <coord x="21" y="-18"/>
            <xsl:text>&#x0A;					</xsl:text>
          </polyline>
          <xsl:text>&#x0A;				</xsl:text>
        </itemgra>
        <xsl:text>&#x0A;			</xsl:text>
      </cursor>
   </xsl:template>
</xsl:stylesheet>
