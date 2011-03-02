<?xml version="1.0" encoding="ISO-8859-1"?>

<!--XSL style sheet that takes EESCHEMA's Generic Netlist Format as input and
    outputs a simple BOM in CSV format.  Feel free to enhance this and submit
    patches.

    How to use:
        Eeschema.pdf: chapter 14
-->

<!DOCTYPE xsl:stylesheet [
  <!ENTITY nl  "&#xd;&#xa;"> <!--new line CR, LF -->
]>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="text" omit-xml-declaration="yes" indent="no"/>

<!-- for each component -->
<xsl:template match="libpart">

<!--
    <xsl:value-of select="count(//comp/libsource/@part[@part])"/><xsl:text>,</xsl:text>
-->

    <xsl:value-of select="@part"/><xsl:text>,</xsl:text>
    <xsl:value-of select="description"/><xsl:text>,</xsl:text>
    <xsl:value-of select="@lib"/>

    <xsl:text>&nl;</xsl:text>
</xsl:template>


<xsl:template match="/export">
    <xsl:text>qty,partname,description,lib&nl;</xsl:text>
    <xsl:apply-templates select="libparts/libpart"/>
</xsl:template>


</xsl:stylesheet>
