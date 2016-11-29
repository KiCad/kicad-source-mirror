<!--XSL style sheet to convert EESCHEMA XML Partlist Format to CSV BOM Format
    Copyright (C) 2013, Stefan Helmert.
    GPL v2.

    Functionality:
        Generation of csv table with table head of all existing field names
        and correct assigned cell entries

    How to use this is explained in eeschema.pdf chapter 14.  You enter a command line into the
    netlist exporter using a new (custom) tab in the netlist export dialog.  The command is
    similar to
        on Windows:
            xsltproc -o "%O.csv" "C:\Program Files (x86)\KiCad\bin\plugins\bom2csv.xsl" "%I"
        on Linux:
            xsltproc -o "%O.csv" /usr/local/lib/kicad/plugins/bom2csv.xsl "%I"

    Instead of "%O.csv" you can alternatively use "%O" if you will supply your own file extension when
    prompted in the UI.  The double quotes are there to account for the possibility of space(s)
    in the filename.
-->

<!--
    @package
    Generate a Tab delimited list (csv file type).
    One component per line
    Fields are
    Ref,Value, Footprint, Datasheet, Field5, Field4, price
-->

<!DOCTYPE xsl:stylesheet [
  <!ENTITY nl  "&#xd;&#xa;">    <!--new line CR, LF, or LF, your choice -->
]>


<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
    <xsl:output method="text"/>

    <!-- for table head and empty table fields-->
    <xsl:key name="headentr" match="field" use="@name"/>

    <!-- main part -->
    <xsl:template match="/export">
        <xsl:text>Reference, Value, Footprint, Datasheet</xsl:text>

            <!-- find all existing table head entries and list each one once -->
            <xsl:for-each select="components/comp/fields/field[generate-id(.) = generate-id(key('headentr',@name)[1])]">
                <xsl:text>, </xsl:text>
                <xsl:value-of select="@name"/>
            </xsl:for-each>
            <xsl:text>&nl;</xsl:text>

        <!-- all table entries -->
        <xsl:apply-templates select="components/comp"/>
    </xsl:template>

    <!-- the table entries -->
    <xsl:template match="components/comp">
        <xsl:text>"</xsl:text>
        <xsl:value-of select="@ref"/><xsl:text>","</xsl:text>
        <xsl:value-of select="value"/><xsl:text>","</xsl:text>
        <xsl:value-of select="footprint"/><xsl:text>","</xsl:text>
        <xsl:value-of select="datasheet"/><xsl:text>"</xsl:text>
        <xsl:apply-templates select="fields"/>
        <xsl:text>&nl;</xsl:text>
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
