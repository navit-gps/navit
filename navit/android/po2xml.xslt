<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:po2xml="http://example.com/namespace" exclude-result-prefixes="po2xml">
<xsl:param name="po_file"/>
<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:function name="po2xml:escape">
	<xsl:param name="str" />
	<xsl:sequence select="replace($str,concat('(',codepoints-to-string(39),')'),'\\$1')" />
</xsl:function>

<xsl:template match="/">
	<resources>
	<xsl:variable name="r" select="." />
		<xsl:for-each select="tokenize(replace(unparsed-text($po_file),'\r',''), '\n+\n')">
			<xsl:analyze-string regex="(.*\n)?msgid &quot;(.*)&quot;\nmsgstr &quot;(.*)&quot;$" select="." flags="s">
				<xsl:matching-substring>
					<xsl:variable name="comment" select="replace(regex-group(1),'#','')" />
					<xsl:variable name="msgid" select="po2xml:escape(replace(regex-group(2),'&quot;\n&quot;','','s'))" />
					<xsl:variable name="msgstr" select="po2xml:escape(replace(regex-group(3),'&quot;\n&quot;','','s'))" />
					<xsl:variable name="resname" select="$r/resources/string[text()=$msgid]/@name" />
					<xsl:choose>
						<xsl:when test="$msgid!='' and $msgstr!='' and $resname!=''">
							<xsl:choose>
								<xsl:when test="$comment!=''">
									<xsl:comment select="$comment"/>
								</xsl:when>
							</xsl:choose>
							<string>
								<xsl:attribute name='name'><xsl:value-of select="$resname"/></xsl:attribute>
								<xsl:value-of select="$msgstr"/>
							</string>
						</xsl:when>
					</xsl:choose>
			</xsl:matching-substring>
			</xsl:analyze-string>
		</xsl:for-each>
	</resources>
</xsl:template>
</xsl:transform>

