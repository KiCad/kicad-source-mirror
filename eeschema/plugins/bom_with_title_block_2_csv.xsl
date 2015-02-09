<!--
    @package
    EESCHEMA BOM plugin. Creates BOM CSV files from the project net file.
    Based on Stefan Helmert bom2csv.xsl
    
    Arthur: Ronald Sousa HashDefineElectronics.com
    
    Usage:
        on Windows:
            xsltproc -o "%O.csv" "C:\Program Files (x86)\KiCad\bin\plugins\bom2csv.xsl" "%I"
        on Linux:
            xsltproc -o "%O.csv" /usr/local/lib/kicad/plugins/bom2csv.xsl "%I"
    
    Ouput Example:
        Kicad Rev:  working director and file source
        Generated Date: date this file was generated
        Document Title: the project tile
        Company: the project company
        Revision: the project revision
        Issue Date: project issue date
        Comment: This is comment 1
        Comment: This is comment 2
        Comment: This is comment 3
        Comment: This is comment 4

        Reference, Value, Fields[n], Library, Library Ref
        U1, PIC32MX, Fields[n], KicadLib, PIC
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
			
        <xsl:text>Source: </xsl:text><xsl:value-of  select="design/source"/><xsl:text>&nl;</xsl:text>
        <xsl:text>Kicad Rev: </xsl:text><xsl:value-of  select="design/tool"/><xsl:text>&nl;</xsl:text>
        <xsl:text>Generated Date: </xsl:text><xsl:value-of  select="design/generatedDate"/><xsl:text>&nl;</xsl:text>
        <xsl:text>&nl;</xsl:text>
        <xsl:text>Document Title: </xsl:text><xsl:value-of  select="design/title"/><xsl:text>&nl;</xsl:text>
        <xsl:text>Company: </xsl:text><xsl:value-of  select="design/company"/><xsl:text>&nl;</xsl:text>
        <xsl:text>Revision: </xsl:text><xsl:value-of  select="design/revision"/><xsl:text>&nl;</xsl:text>
        <xsl:text>Issue Date: </xsl:text><xsl:value-of  select="design/issueDate"/><xsl:text>&nl;</xsl:text>

        <xsl:choose>
            <xsl:when test="design/comment1 !=''">
            <xsl:text>Comment: </xsl:text><xsl:value-of  select="design/comment1"/><xsl:text>&nl;</xsl:text>
            </xsl:when>
        </xsl:choose>

        <xsl:choose>
            <xsl:when test="design/comment2 !=''">
            <xsl:text>Comment: </xsl:text><xsl:value-of  select="design/comment2"/><xsl:text>&nl;</xsl:text>
            </xsl:when>
        </xsl:choose>

        <xsl:choose>
            <xsl:when test="design/comment3 !=''">
            <xsl:text>Comment: </xsl:text><xsl:value-of  select="design/comment3"/><xsl:text>&nl;</xsl:text>
            </xsl:when>
        </xsl:choose>

        <xsl:choose>
            <xsl:when test="design/comment4 !=''">
            <xsl:text>Comment: </xsl:text><xsl:value-of  select="design/comment4"/><xsl:text>&nl;</xsl:text>
            </xsl:when>
        </xsl:choose>

		<xsl:text>&nl;</xsl:text>
			
		<!-- Output table header -->
        <xsl:text>Reference, Value, </xsl:text>
        <xsl:for-each select="components/comp/fields/field[generate-id(.) = generate-id(key('headentr',@name)[1])]">
            <xsl:value-of select="@name"/>
            <xsl:text>, </xsl:text>
        </xsl:for-each>
        <xsl:text>Library, Library Ref</xsl:text>
        <xsl:text>&nl;</xsl:text>

        <!-- all table entries -->
        <xsl:apply-templates select="components/comp"/>
    </xsl:template>

    <!-- the table entries -->
    <xsl:template match="components/comp">
        <xsl:value-of select="@ref"/><xsl:text>,</xsl:text>
        <xsl:value-of select="value"/><xsl:text>,</xsl:text>
        <xsl:apply-templates select="fields"/>
        <xsl:apply-templates select="libsource"/>
        <xsl:text>&nl;</xsl:text>
    </xsl:template>

    <!-- the library selection -->
    <xsl:template match="libsource">
        <xsl:value-of select="@lib"/><xsl:text>,</xsl:text>
        <xsl:value-of select="@part"/>
    </xsl:template>

    <!-- table entries with dynamic table head -->
    <xsl:template match="fields">

        <!-- remember current fields section -->
        <xsl:variable name="fieldvar" select="field"/>

        <!-- for all existing head entries -->
        <xsl:for-each select="/export/components/comp/fields/field[generate-id(.) = generate-id(key('headentr',@name)[1])]">
            <xsl:variable name="allnames" select="@name"/>

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
            <xsl:text>,</xsl:text>
        </xsl:for-each>
    </xsl:template>

 </xsl:stylesheet>
