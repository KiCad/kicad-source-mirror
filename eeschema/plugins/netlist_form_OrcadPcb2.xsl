<?xml version="1.0" encoding="ISO-8859-1"?>
<!--XSL style sheet to EESCHEMA Generic Netlist Format to CADSTAR netlist format
    Copyright (C) 2010, SoftPLC Corporation.
    GPL v2.

    How to use:
        https://lists.launchpad.net/kicad-developers/msg05157.html
-->

<!DOCTYPE xsl:stylesheet [
  <!ENTITY nl  "&#xd;&#xa;"> <!--new line CR, LF -->
]>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="text" omit-xml-declaration="yes" indent="no"/>

<!-- Netlist header -->
<xsl:template match="/export">
    <xsl:text>( { EESchema Netlist Version 1.1  </xsl:text>
    <xsl:apply-templates select="design/date"/>  <!-- Generate line .TIM <time> -->
    <xsl:apply-templates select="design/tool"/>  <!-- Generate line .APP <eeschema version> -->
    <xsl:text>}&nl;</xsl:text>
    <xsl:apply-templates select="components/comp"/>  <!-- Generate list of components -->
    <xsl:text>)&nl;*&nl;</xsl:text>
</xsl:template>

 <!-- Generate id in header like "eeschema (2010-08-17 BZR 2450)-unstable" -->
<xsl:template match="tool">
    <xsl:apply-templates/>
</xsl:template>

 <!-- Generate date in header like "20/08/2010 10:45:33" -->
<xsl:template match="date">
    <xsl:apply-templates/>
    <xsl:text>&nl;</xsl:text>
</xsl:template>

<!-- for each component -->
<xsl:template match="comp">
    <xsl:text> ( </xsl:text>
    <xsl:choose>
        <xsl:when test = "tstamp != '' ">
            <xsl:apply-templates select="tstamp"/>
        </xsl:when>
        <xsl:otherwise>
            <xsl:text>00000000</xsl:text>
        </xsl:otherwise>
    </xsl:choose>
    <xsl:text> </xsl:text>
    <xsl:choose>
        <xsl:when test = "footprint != '' ">
            <xsl:apply-templates select="footprint"/>
        </xsl:when>
        <xsl:otherwise>
            <xsl:text>$noname</xsl:text>
        </xsl:otherwise>
    </xsl:choose>
    <xsl:text> </xsl:text>
    <xsl:value-of select="@ref"/>
    <xsl:text> </xsl:text>
    <xsl:choose>
        <xsl:when test = "value != '' ">
            <xsl:apply-templates select="value"/>
        </xsl:when>
        <xsl:otherwise>
            <xsl:text>"~"</xsl:text>
        </xsl:otherwise>
    </xsl:choose>
    <xsl:text>&nl;</xsl:text>
    <xsl:apply-templates select="pins/pin"/>
    <xsl:text> )&nl;</xsl:text>
</xsl:template>

<!-- for each pin in a component -->
<xsl:template match="pin">
    <xsl:text>  (  </xsl:text>
    <xsl:value-of select="@num"/>
    <xsl:text> = </xsl:text>
    <xsl:choose>
        <xsl:when test = "@netname != '' ">
            <xsl:apply-templates select="@netname"/>
        </xsl:when>
        <xsl:otherwise>
            <xsl:text>?</xsl:text>
        </xsl:otherwise>
    </xsl:choose>
    <xsl:text> )&nl;</xsl:text>
</xsl:template>

</xsl:stylesheet>
