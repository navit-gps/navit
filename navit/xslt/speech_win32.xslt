<?xml version="1.0"?>
<xsl:transform version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude">
        <xsl:template match="/config/navit/speech">
                <speech type="cmdline" data="PowerShell -Command &quot;Add-Type –AssemblyName System.Speech; (New-Object System.Speech.Synthesis.SpeechSynthesizer).Speak(%s);&quot;"/>
        </xsl:template>
</xsl:transform>
