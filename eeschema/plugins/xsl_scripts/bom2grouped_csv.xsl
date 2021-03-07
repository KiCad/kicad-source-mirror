<!--XSL style sheet to convert EESCHEMA XML Partlist Format to grouped CSV BOM Format
    Copyright (C) 2014, Wolf Walter.
    Copyright (C) 2013, Stefan Helmert.
    Copyright (C) 2018, Kicad developers.
    GPL v2.

	Functionality:
		Generation of Digi-Key ordering system compatible BOM

    How to use this is explained in eeschema.pdf chapter 14.
-->
<!--
    @package
    Output: CSV (comma-separated)
    Grouped By: Value, Footprint
    Sorted By: Ref
    Fields: Reference, Quantity, Value, Footprint, Datasheet, all additional symbol fields

    Command line:
    xsltproc -o "%O.csv" "pathToFile/bom2grouped_csv.xsl" "%I"
-->


<!DOCTYPE xsl:stylesheet [
  <!ENTITY nl  "&#xd;&#xa;">    <!--new line CR, LF, or LF, your choice -->
]>


<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="text"/>

	<xsl:variable name="digits" select="'1234567890'" />

	<!-- for matching grouping of footprint and value combination -->
	<xsl:key name="partTypeByValueAndFootprint" match="comp" use="concat(footprint, '-', value)" />

	<!-- for table head and empty table fields-->
	<xsl:key name="headentr" match="field" use="@name"/>

	    <!-- main part -->
	<xsl:template match="/export">
	    <xsl:text>Reference, Quantity, Value, Footprint, Datasheet</xsl:text>

	    <!-- find all existing table head entries and list each one once -->
	    <xsl:for-each select="components/comp/fields/field[generate-id(.) = generate-id(key('headentr',@name)[1])]">
		<xsl:text>, </xsl:text>
		<xsl:value-of select="@name"/>
	    </xsl:for-each>

	    <!-- all table entries -->
	    <xsl:apply-templates select="components"/>
	</xsl:template>

	<xsl:template match="components">
	    <!-- for Muenchian grouping of footprint and value combination -->
	    <xsl:for-each select="comp[count(. | key('partTypeByValueAndFootprint', concat(footprint, '-', value))[1]) = 1]">
		<xsl:sort select="@ref" />
		<xsl:text>&nl;</xsl:text>
		<!-- list of all references -->
		<xsl:for-each select="key('partTypeByValueAndFootprint', concat(footprint, '-', value))">
			<!-- strip non-digits from reference and sort based on remaining number -->
			<xsl:sort select="translate(@ref, translate(@ref, $digits, ''), '')" data-type="number" />
			<xsl:value-of select="@ref"/><xsl:text> </xsl:text>
		</xsl:for-each><xsl:text>,</xsl:text>
		<!-- quantity of parts with same footprint and value -->
		<xsl:value-of select="count(key('partTypeByValueAndFootprint', concat(footprint, '-', value)))"/><xsl:text>,</xsl:text>
        <xsl:text>"</xsl:text>
		<xsl:value-of select="value"/><xsl:text>","</xsl:text>
		<xsl:value-of select="footprint"/><xsl:text>","</xsl:text>
		<xsl:value-of select="datasheet"/><xsl:text>"</xsl:text>
		<xsl:apply-templates select="fields"/>
	    </xsl:for-each>
	</xsl:template>

	 <!-- table entries with dynamic table head -->
	<xsl:template match="fields">

	    <!-- remember current fields section -->
	    <xsl:variable name="fieldvar" select="field"/>

	    <!-- for all existing head entries -->
	    <xsl:for-each select="/export/components/comp/fields/field[generate-id(.) = generate-id(key('headentr',@name)[1])]">
		<xsl:variable name="allnames" select="@name"/>
		<xsl:text>,"</xsl:text>

		<!-- for all field entries in the remembered fields section -->
		<xsl:for-each select="$fieldvar">

		    <!-- only if this field entry exists in this fields section -->
		    <xsl:if test="@name=$allnames">
			<!-- content of the field -->
			<xsl:value-of select="."/>
		    </xsl:if>
		    <!--
			If it does not exist, use an empty cell in output for this row.
			Every non-blank entry is assigned to its proper column.
		    -->
                </xsl:for-each>

                <xsl:text>"</xsl:text>
	    </xsl:for-each>
	</xsl:template>

 </xsl:stylesheet>
