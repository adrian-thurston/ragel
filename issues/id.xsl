<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"> 
<xsl:output method="text"/>
<xsl:preserve-space elements="*"/>
<xsl:strip-space elements=""/> 
<xsl:template match="/">
<xsl:for-each select="issues/issue">
	<xsl:value-of select="id"/><xsl:text> 
</xsl:text>
</xsl:for-each>
</xsl:template>
</xsl:stylesheet> 
