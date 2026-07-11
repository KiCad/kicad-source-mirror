

#include "csv.h"

#include <wx/txtstrm.h>

#include <rapidcsv/rapidcsv.h>


CSV_WRITER::CSV_WRITER( wxOutputStream& aStream ) :
        m_stream( aStream ), m_delimiter( wxT( "," ) ),
        m_quote( wxT( "\"" ) ), m_lineTerminator( wxT( "\n" ) ),
        m_escape( wxEmptyString )
{
}


void CSV_WRITER::WriteLines( const std::vector<std::vector<wxString>>& aRows )
{
    for( const auto& row : aRows )
    {
        WriteLine( row );
    }
}


void CSV_WRITER::WriteLine( const std::vector<wxString>& cols )
{
    wxString line;

    for( size_t i = 0; i < cols.size(); ++i )
    {
        wxString colVal = cols[i];

        if( i > 0 )
            line += m_delimiter;


        const bool useEscape = m_escape.size();

        if( useEscape )
        {
            colVal.Replace( m_quote, m_escape + m_quote, true );
        }
        else
        {
            colVal.Replace( m_quote, m_quote + m_quote, true );
        }

        // Always quote - if that's a problem, we can only quote if the
        // delimiter (or newlines?) are present in the string.
        colVal = m_quote + colVal + m_quote;


        line += colVal;
    }

    line += m_lineTerminator;

    // Always write text on file using UTF8 encoding
    std::string utf8str( line.utf8_string() );
    m_stream.Write( utf8str.data(), utf8str.length() );
}


bool AutoDecodeCSV( const wxString& aInput, std::vector<std::vector<wxString>>& aData )
{
    // Read the first line to determine the delimiter
    bool trimCells = true;
    bool skipCommentLines = true;
    bool skipEmptyLines = true;
    char commentPrefix = '#';
    char delimiter = ',';

    // Assume if we find a tab, we are dealing with a TSV file
    if( aInput.find( '\t' ) != std::string::npos )
    {
        delimiter = '\t';
    }

    std::istringstream inputStream( aInput.ToStdString() );

    rapidcsv::Document doc( inputStream, rapidcsv::LabelParams( -1, -1 ),
                            rapidcsv::SeparatorParams( delimiter, trimCells ), rapidcsv::ConverterParams(),
                            rapidcsv::LineReaderParams( skipCommentLines, commentPrefix, skipEmptyLines ) );

    // Read the data into aData
    aData.clear();

    size_t rowCount = doc.GetRowCount();
    size_t colCount = doc.GetColumnCount();

    for( size_t i = 0; i < rowCount; ++i )
    {
        std::vector<std::string> docRow = doc.GetRow<std::string>( i );
        std::vector<wxString>&   outRow = aData.emplace_back( colCount );

        for( size_t j = 0; j < std::min( colCount, docRow.size() ); ++j )
            outRow[j] = wxString::FromUTF8( docRow[j] );
    }

    // Anything in the first row?
    return !aData.empty() && aData[0].size() > 0;
}
