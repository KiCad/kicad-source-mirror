/*
* This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Oliver Walters
 * Copyright (C) 2017 KiCad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "bom_exporter.h"

/**
 * BOM_FILE_WRITER class (pure virtual)
 */
BOM_FILE_WRITER::BOM_FILE_WRITER() :
    m_includeExtraData( true ),
    m_showRowNumbers( true )
{
}

void BOM_FILE_WRITER::SetHeader( const wxArrayString aHeader )
{
    m_bomHeader = aHeader;
}

void BOM_FILE_WRITER::AddLine( wxArrayString aLine )
{
    m_bomLines.push_back( aLine );
}

/**
 * Function ExtraDataPairs
 * Format extra data for writing to file
 */
wxArrayString BOM_FILE_WRITER::ExtraDataPairs()
{
    wxArrayString pairs;

    if( m_groupCount != m_componentCount )
    {
        pairs.Add( DataPair( _( "Group Count" ), m_groupCount ) );
    }

    pairs.Add( DataPair( _( "Component Count" ), m_componentCount ) );
    pairs.Add( DataPair( _( "Schematic Title" ), m_schematicTitle ) );
    pairs.Add( DataPair( _( "Schematic Date" ), m_schematicDate ) );
    pairs.Add( DataPair( _( "Schematic Version" ), m_schematicVersion ) );
    pairs.Add( DataPair( _( "KiCad Version" ), m_kicadVersion ) );

    return pairs;
}

BOM_CSV_WRITER::BOM_CSV_WRITER( wxChar aDelim ) :
        BOM_FILE_WRITER()
{
    m_delim = aDelim;
}

/**
 * Function WriteToFile
 * Write delimited data to file
 */
bool BOM_CSV_WRITER::WriteToFile( wxFile& aFile )
{

    // Generate table header
    wxString line = wxJoin( EscapeLine( m_bomHeader ), m_delim )+ "\n";

    if( m_showRowNumbers )
    {
        line = m_delim + line;
    }

    if( !aFile.Write( line ))
    {
        return false;
    }

    // Write each line in the file
    for( unsigned int ii=0; ii<m_bomLines.size(); ii++ )
    {
        line = wxJoin( EscapeLine( m_bomLines[ii]), m_delim ) + "\n";

        if( m_showRowNumbers )
        {
            line = wxString::Format( _( "%i" ), ii+1 ) + m_delim + line;
        }

        if( !aFile.Write( line ) )
        {
            return false;
        }
    }

    // Write extra options
    if( m_includeExtraData )
    {
        wxString extra;

        extra += "\n\n";

        for( wxString pair : ExtraDataPairs() )
        {
            extra += pair;
            extra += "\n";
        }

        if( !aFile.Write( extra ) )
            return false;
    }

    // File writing successful
    return true;
}

/**
 * Function DataPair
 * Combine two items into a delimited pair
 */
wxString BOM_CSV_WRITER::DataPair( const wxString& aFirst, const wxString& aSecond )
{
    wxArrayString pair;

    pair.Add( aFirst );
    pair.Add( aSecond );

    return wxJoin( pair, m_delim );
}

/**
 * Function EscapeLine
 * Any values that contain the delimiter are escaped with quotes
 */
wxArrayString BOM_CSV_WRITER::EscapeLine( wxArrayString line )
{
    wxArrayString escaped;

    for( wxString item : line )
    {
        if( item.Contains( m_delim ) &&
            !item.StartsWith( "\"" ) &&
            !item.EndsWith( "\"" ) )
        {
            item = "\"" + item + "\"";
        }

        escaped.Add( item );
    }

    return escaped;
}

BOM_HTML_WRITER::BOM_HTML_WRITER() :
        BOM_FILE_WRITER()
{
    // Strings to check for hyperlinkable text
    m_linkChecks.Add( "http:*" );
    m_linkChecks.Add( "https:* " );
    m_linkChecks.Add( "ftp:*" );
    m_linkChecks.Add( "www.*" );
    m_linkChecks.Add( "*.pdf" );
    m_linkChecks.Add( "*.html" );
    m_linkChecks.Add( "*.htm" );
}

/**
 * Function WriteToFile
 * Write HTML BoM Data
 */
bool BOM_HTML_WRITER::WriteToFile( wxFile& aFile )
{
    // Write HTML header
    if( !aFile.Write( HtmlHeader() ) )
        return false;

    if( m_includeExtraData )
    {
        if( !aFile.Write( ExtraData() ) )
            return false;
    }

    // Table data
    wxString tableTitle = "<h2>";

    tableTitle += _( "Bill of Materials" );
    tableTitle += "</h2>\n";

    if( !aFile.Write( tableTitle ) )
        return false;

    if( !aFile.Write( "<table border=\"1\">\n" ) )
        return false;

    if( !aFile.Write( TableHeader( m_bomHeader ) ) )
        return false;

    // Write each line of the BOM
    for( unsigned int ii=0; ii<m_bomLines.size(); ii++ )
    {
        if( !aFile.Write( TableRow( ii+1, m_bomLines[ii] ) ) )
            return false;
    }

    if( !aFile.Write( "</table>\n" ) )
        return false;

    if( !aFile.Write( HtmlFooter() ) )
        return false;

    return true;
}

wxString BOM_HTML_WRITER::HtmlHeader()
{
    wxString header = wxEmptyString;

    header += "<html>\n<head>\n";
    //TODO - Project title
    header += "<title>";
    header += m_schematicTitle;
    header += "</title>\n";

    header += HtmlMetaTag( "charset", "UTF-8" ) + "\n";

    //TODO - KiCad reference data here

    header += "</head>\n\n";

    header += "<body>\n";

    return header;
}

wxString BOM_HTML_WRITER::HtmlFooter()
{
    wxString footer = wxEmptyString;

    footer += "</body>\n\n";
    footer += "</html>\n";

    return footer;
}

wxString BOM_HTML_WRITER::HtmlMetaTag( const wxString aTitle, const wxString aData )
{
    wxString tag = "<meta name=\"";

    tag += aTitle + "\"";
    tag += " content=\"";
    tag += aData + "\"";

    tag += ">";

    return tag;
}

/**
 * Function ExtraData
 * Write extra project information
 */
wxString BOM_HTML_WRITER::ExtraData()
{
    wxString extra;

    extra += "<h2>";
    extra += _( "Project details" );
    extra += "</h2>\n";

    extra += "<table border=\"1\">\n";

    for( wxString pair : ExtraDataPairs() )
    {
        extra += pair + "\n";
    }

    extra += "</table>\n";

    return extra;
}

/**
 * Function LinkText
 * Automatically detect linkable text, and wrap it in <a> tag
 * @aText - Text to (potentially) link
 */
wxString BOM_HTML_WRITER::LinkText( const wxString& aText )
{
    // Should we provide a link to the text?
    wxString lower = aText.Lower();

    bool found = false;

    for( wxString check : m_linkChecks )
    {
        if( lower.Matches( check ) )
        {
            found = true;
            break;
        }
    }

    if( found )
    {
        wxString link = "<a href=\"";

        link += aText;
        link += "\">";
        link += aText;
        link += "</a>";

        return link;
    }
    else
    {
        return aText;
    }
}

wxString BOM_HTML_WRITER::TableHeader( const wxArrayString& aHeaderData )
{
    wxString header = "<tr>\n";

    if( m_showRowNumbers )
    {
        header += "<th></th>\n";
    }

    for( wxString item : aHeaderData )
    {
        header += "<th align=\"center\">";
        header += item;
        header += "</th>";
        header += "\n";
    }

    header += "</tr>\n";

    return header;
}

wxString BOM_HTML_WRITER::DataPair( const wxString& aFirst, const wxString& aSecond )
{
    wxString html = "<tr>\n";

    html += TableEntry( aFirst ) + "\n";
    html += TableEntry( aSecond ) + "\n";

    html += "</tr>\n";

    return html;
}

/**
 * Function TableRow
 * Generate a single row of BOM data
 * @aRowNum is the number of the row
 * @aRowData is the array of data for the given row
 */
wxString BOM_HTML_WRITER::TableRow( const int& aRowNum, const wxArrayString& aRowData )
{
    wxString row = wxEmptyString;

    row += "<tr>\n";

    if( m_showRowNumbers )
    {
        row += "<td>";
        row += wxString::Format( "%i", aRowNum );
        row += "</td>\n";
    }

    for( wxString data : aRowData )
    {
        row += TableEntry( data );
        row += "\n";
    }

    row += "</tr>\n";

    return row;
}

/**
 * Function TableEntry
 * Wrap a string in <td> tags
 * @aData is the text to be wrapped
 * @aColor is an (optional) HTML background color
 */
wxString BOM_HTML_WRITER::TableEntry( wxString aData, wxString aColor )
{
    wxString cell = "<td align=\"center\"";

    if( !aColor.IsEmpty() )
    {
        cell += " bgcolor=\"" + aColor + "\"";
    }

    cell += ">";

    cell += LinkText( aData );

    cell += "</td>";

    return cell;
}
