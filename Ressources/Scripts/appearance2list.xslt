<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="text" omit-xml-declaration="yes" />
<xsl:template match="/">
<xsl:for-each select="Characteristic/Value/Field/Enumerations/Enumeration">    { <xsl:value-of select="@key"/>, "<xsl:value-of select="@value"/>" },
</xsl:for-each>
</xsl:template>
</xsl:stylesheet>
