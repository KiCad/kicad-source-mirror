/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2016 KiCad Developers, see change_log.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctype.h>

#include <wx/mstream.h>
#include <wx/filename.h>

#include <drawtxt.h>
#include <richio.h>
#include <core/typeinfo.h>

#include <general.h>
#include <lib_field.h>
#include <sch_bus_entry.h>
#include <sch_marker.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_component.h>
#include <sch_text.h>
#include <sch_sheet.h>
#include <sch_bitmap.h>
#include <sch_legacy_plugin.h>
#include <template_fieldnames.h>
#include <class_sch_screen.h>


#define SCH_PARSE_ERROR( text, reader, pos )                       \
    THROW_PARSE_ERROR( wxString::FromUTF8( text ),                 \
                       reader.GetSource(), reader.Line(),          \
                       reader.LineNumber(), pos - reader.Line() )


// Token delimiters.
const char* delims = " \t\r\n";


/**
 * Function strCompare
 *
 * compares \a aString to the string starting at \a aLine and advances the character point to
 * the end of \a String and returns the new pointer position in \a aOutput if it is not NULL.
 *
 * @param aString - A pointer to the string to compare.
 * @param aLine - A pointer to string to begin the comparison.
 * @param aOutput - A pointer to a string pointer to the end of the comparison if not NULL.
 * @return True if \a aString was found starting at \a aLine.  Otherwise false.
 */
static bool strCompare( const char* aString, const char* aLine, const char** aOutput = NULL )
{
    size_t len = strlen( aString );
    bool retv = ( strnicmp( aLine, aString, len ) == 0 ) && isspace( aLine[ len ] );

    if( retv && aOutput )
    {
        const char* tmp = aLine;

        // Move past the end of the token.
        tmp += len;

        // Move to the beginning of the next token.
        while( *tmp && isspace( *tmp ) )
            tmp++;

        *aOutput = tmp;
    }

    return retv;
}


/**
 * Function parseInt
 *
 * parses an ASCII integer string with possible leading whitespace into
 * an integer and updates the pointer at \a aOutput if it is not NULL, just
 * like "man strtol()".
 *
 * @param aReader - The line reader used to generate exception throw information.
 * @param aLine - A pointer the current position in a string.
 * @param aOutput - The pointer to a string pointer to copy the string pointer position when
 *                  the parsing is complete.
 * @return A valid integer value.
 * @throws An #IO_ERROR on an unexpected end of line.
 * @throws A #PARSE_ERROR if the parsed token is not a valid integer.
 */
static int parseInt( FILE_LINE_READER& aReader, const char* aLine, const char** aOutput = NULL )
{
    if( !*aLine )
        THROW_IO_ERROR( _( "unexpected end of line" ) );

    // Clear errno before calling strtol() in case some other crt call set it.
    errno = 0;

    long retv = strtol( aLine, (char**) aOutput, 10 );

    // Make sure no error occurred when calling strtol().
    if( errno == ERANGE )
        SCH_PARSE_ERROR( "invalid integer value", aReader, aLine );

    // strtol does not strip off whitespace before the next token.
    if( aOutput )
    {
        const char* next = *aOutput;

        while( *next && isspace( *next ) )
            next++;

        *aOutput = next;
    }

    return (int) retv;
}


/**
 * Function parseHex
 *
 * parses an ASCII hex integer string with possible leading whitespace into
 * a long integer and updates the pointer at \a aOutput if it is not NULL, just
 * like "man strtol".
 *
 * @param aReader - The line reader used to generate exception throw information.
 * @param aLine - A pointer the current position in a string.
 * @param aOutput - The pointer to a string pointer to copy the string pointer position when
 *                  the parsing is complete.
 * @return A valid integer value.
 * @throws An #IO_ERROR on an unexpected end of line.
 * @throws A #PARSE_ERROR if the parsed token is not a valid integer.
 */
static unsigned long parseHex( FILE_LINE_READER& aReader, const char* aLine,
                               const char** aOutput = NULL )
{
    if( !*aLine )
        THROW_IO_ERROR( _( "unexpected end of line" ) );

    unsigned long retv;

    // Clear errno before calling strtoul() in case some other crt call set it.
    errno = 0;
    retv = strtoul( aLine, (char**) aOutput, 16 );

    // Make sure no error occurred when calling strtoul().
    if( errno == ERANGE )
        SCH_PARSE_ERROR( "invalid hexadecimal number", aReader, aLine );

    // Strip off whitespace before the next token.
    if( aOutput )
    {
        // const char* next = aLine + strlen( token );

        const char* next = *aOutput;

        while( *next && isspace( *next ) )
            next++;

        *aOutput = next;
    }

    return retv;
}


/**
 * Function parseDouble
 *
 * parses an ASCII point string with possible leading whitespace into a double precision
 * floating point number and  updates the pointer at \a aOutput if it is not NULL, just
 * like "man strtod".
 *
 * @param aReader - The line reader used to generate exception throw information.
 * @param aLine - A pointer the current position in a string.
 * @param aOutput - The pointer to a string pointer to copy the string pointer position when
 *                  the parsing is complete.
 * @return A valid double value.
 * @throws An #IO_ERROR on an unexpected end of line.
 * @throws A #PARSE_ERROR if the parsed token is not a valid integer.
 */
static double parseDouble( FILE_LINE_READER& aReader, const char* aLine,
                           const char** aOutput = NULL )
{
    if( !*aLine )
        THROW_IO_ERROR( _( "unexpected end of line" ) );

    // Clear errno before calling strtod() in case some other crt call set it.
    errno = 0;

    double retv = strtod( aLine, (char**) aOutput );

    // Make sure no error occurred when calling strtod().
    if( errno == ERANGE )
        SCH_PARSE_ERROR( "invalid floating point number", aReader, aLine );

    // strtod does not strip off whitespace before the next token.
    if( aOutput )
    {
        const char* next = *aOutput;

        while( *next && isspace( *next ) )
            next++;

        *aOutput = next;
    }

    return retv;
}


/**
 * Function parseChar
 *
 * parses a single ASCII character and updates the pointer at \a aOutput if it is not NULL.
 *
 * @param aReader - The line reader used to generate exception throw information.
 * @param aCurrentToken - A pointer the current position in a string.
 * @param aNextToken - The pointer to a string pointer to copy the string pointer position when
 *                     the parsing is complete.
 * @return A valid ASCII character.
 * @throws An #IO_ERROR on an unexpected end of line.
 * @throws A #PARSE_ERROR if the parsed token is not a a single character token.
 */
static char parseChar( FILE_LINE_READER& aReader, const char* aCurrentToken,
                       const char** aNextToken = NULL )
{
    while( *aCurrentToken && isspace( *aCurrentToken ) )
        aCurrentToken++;

    if( !*aCurrentToken )
        SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, aCurrentToken );

    if( *( aCurrentToken + 1 ) != ' ' )
        SCH_PARSE_ERROR( _( "expected single character token" ), aReader, aCurrentToken );

    if( aNextToken )
    {
        const char* next = aCurrentToken + 2;

        while( *next && isspace( *next ) )
            next++;

        *aNextToken = next;
    }

    return *aCurrentToken;
}


/**
 * Function parseUnquotedString.
 *
 * parses an unquoted utf8 string and updates the pointer at \a aOutput if it is not NULL.
 *
 * The parsed string must be a continuous string with no white space.
 *
 * @param aString - A reference to the parsed string.
 * @param aReader - The line reader used to generate exception throw information.
 * @param aCurrentToken - A pointer the current position in a string.
 * @param aNextToken - The pointer to a string pointer to copy the string pointer position when
 *                     the parsing is complete.
 * @param aCanBeEmpty - True if the parsed string is optional.  False if it is mandatory.
 * @throws An #IO_ERROR on an unexpected end of line.
 * @throws A #PARSE_ERROR if the \a aCanBeEmpty is false and no string was parsed.
 */
static void parseUnquotedString( wxString& aString, FILE_LINE_READER& aReader,
                                 const char* aCurrentToken, const char** aNextToken = NULL,
                                 bool aCanBeEmpty = false )
{
    if( !*aCurrentToken )
        THROW_IO_ERROR( _( "unexpected end of line" ) );

    const char* tmp = aCurrentToken;

    while( *tmp && isspace( *tmp ) )
        tmp++;

    if( !*tmp )
    {
        if( aCanBeEmpty )
            return;
        else
            THROW_IO_ERROR( _( "unexpected end of line" ) );
    }

    std::string utf8;

    while( *tmp && !isspace( *tmp ) )
        utf8 += *tmp++;

    aString = FROM_UTF8( utf8.c_str() );

    if( aString.IsEmpty() && !aCanBeEmpty )
        SCH_PARSE_ERROR( _( "expected unquoted string" ), aReader, aCurrentToken );

    if( aNextToken )
    {
        const char* next = tmp;

        while( *next && isspace( *next ) )
            next++;

        *aNextToken = next;
    }
}


/**
 * Function parseQuotedString.
 *
 * parses an quoted ASCII utf8 and updates the pointer at \a aOutput if it is not NULL.
 *
 * The parsed string must be contained within a single line.  There are no multi-line
 * quoted strings in the legacy schematic file format.
 *
 * @param aString - A reference to the parsed string.
 * @param aReader - The line reader used to generate exception throw information.
 * @param aCurrentToken - A pointer the current position in a string.
 * @param aNextToken - The pointer to a string pointer to copy the string pointer position when
 *                     the parsing is complete.
 * @param aCanBeEmpty - True if the parsed string is optional.  False if it is mandatory.
 * @throws An #IO_ERROR on an unexpected end of line.
 * @throws A #PARSE_ERROR if the \a aCanBeEmpty is false and no string was parsed.
 */
static void parseQuotedString( wxString& aString, FILE_LINE_READER& aReader,
                               const char* aCurrentToken, const char** aNextToken = NULL,
                               bool aCanBeEmpty = false )
{
    if( !*aCurrentToken )
    {
        if( aCanBeEmpty )
            return;
        else
            THROW_IO_ERROR( _( "unexpected end of line" ) );
    }

    const char* tmp = aCurrentToken;

    while( *tmp && isspace( *tmp ) )
        tmp++;

    if( !*tmp )
    {
        if( aCanBeEmpty )
            return;
        else
            THROW_IO_ERROR( _( "unexpected end of line" ) );
    }

    // Verify opening quote.
    if( *tmp != '"' )
        THROW_IO_ERROR( _( "expecting opening quote" ) );

    tmp++;

    std::string utf8;     // utf8 without escapes and quotes.

    // Fetch everything up to closing quote.
    while( *tmp )
    {
        if( *tmp == '\\' )
        {
            tmp++;

            if( !*tmp )
                THROW_IO_ERROR( _( "unexpected end of line" ) );

            // Do not copy the escape byte if it is followed by \ or "
            if( *tmp != '"' && *tmp != '\\' )
                    utf8 += '\\';

            utf8 += *tmp;
        }
        else if( *tmp == '"' )  // Closing double quote.
        {
            break;
        }
        else
        {
            utf8 += *tmp;
        }

        tmp++;
    }

    aString = FROM_UTF8( utf8.c_str() );

    if( aString.IsEmpty() && !aCanBeEmpty )
        SCH_PARSE_ERROR( _( "expected quoted string" ), aReader, aCurrentToken );

    if( *tmp && *tmp != '"' )
        SCH_PARSE_ERROR( _( "no closing quote for string found" ), aReader, tmp );

    // Move past the closing quote.
    tmp++;

    if( aNextToken )
    {
        const char* next = tmp;

        while( *next && *next == ' ' )
            next++;

        *aNextToken = next;
    }
}


SCH_LEGACY_PLUGIN::SCH_LEGACY_PLUGIN()
{
    init( NULL );
}


void SCH_LEGACY_PLUGIN::init( KIWAY* aKiway, const PROPERTIES* aProperties )
{
    m_version = 0;
    m_rootSheet = NULL;
    m_props = aProperties;
    m_kiway = aKiway;
}


SCH_SHEET* SCH_LEGACY_PLUGIN::Load( const wxString& aFileName, KIWAY* aKiway,
                                    SCH_SHEET* aAppendToMe, const PROPERTIES* aProperties )
{
    wxASSERT( !aFileName || aKiway != NULL );

    LOCALE_IO   toggle;     // toggles on, then off, the C locale.
    SCH_SHEET*  sheet;

    wxFileName fn = aFileName;

    // Unfortunately child sheet file names the legacy schematic file format are not fully
    // qualified and are always appended to the project path.  The aFileName attribute must
    // always be an absolute path so the project path can be used for load child sheet files.
    wxASSERT( fn.IsAbsolute() );

    m_path = fn.GetPath();

    init( aKiway, aProperties );

    if( aAppendToMe == NULL )
    {
        // Clean up any allocated memory if an exception occurs loading the schematic.
        std::auto_ptr< SCH_SHEET > newSheet( new SCH_SHEET );
        newSheet->SetFileName( aFileName );
        m_rootSheet = newSheet.get();
        loadHierarchy( newSheet.get() );

        // If we got here, the schematic loaded successfully.
        sheet = newSheet.release();
    }
    else
    {
        m_rootSheet = aAppendToMe->GetRootSheet();
        wxASSERT( m_rootSheet != NULL );
        sheet = aAppendToMe;
        loadHierarchy( sheet );
    }

    return sheet;
}


// Everything below this comment is recursive.  Modify with care.

void SCH_LEGACY_PLUGIN::loadHierarchy( SCH_SHEET* aSheet )
{
    SCH_SCREEN* screen = NULL;

    if( !aSheet->GetScreen() )
    {
        m_rootSheet->SearchHierarchy( aSheet->GetFileName(), &screen );

        if( screen )
        {
            aSheet->SetScreen( screen );

            // Do not need to load the sub-sheets - this has already been done.
        }
        else
        {
            aSheet->SetScreen( new SCH_SCREEN( m_kiway ) );

            wxFileName fileName = aSheet->GetFileName();

            if( !fileName.IsAbsolute() )
                fileName.SetPath( m_path );

            aSheet->GetScreen()->SetFileName( fileName.GetFullPath() );
            loadFile( fileName.GetFullPath(), aSheet->GetScreen() );

            EDA_ITEM* item = aSheet->GetScreen()->GetDrawItems();

            while( item )
            {
                if( item->Type() == SCH_SHEET_T )
                {
                    SCH_SHEET* sheet = (SCH_SHEET*) item;

                    // Set the parent to aSheet.  This effectively creates a method to find
                    // the root sheet from any sheet so a pointer to the root sheet does not
                    // need to be stored globally.  Note: this is not the same as a hierarchy.
                    // Complex hierarchies can have multiple copies of a sheet.  This only
                    // provides a simple tree to find the root sheet.
                    sheet->SetParent( aSheet );

                    // Recursion starts here.
                    loadHierarchy( sheet );
                }

                item = item->Next();
            }
        }
    }
}


void SCH_LEGACY_PLUGIN::loadFile( const wxString& aFileName, SCH_SCREEN* aScreen )
{
    FILE_LINE_READER reader( aFileName );

    loadHeader( reader, aScreen );

    while( reader.ReadLine() )
    {
        char* line = reader.Line();

        while( *line && *line == ' ' )
            line++;

        // Either an object will be loaded properly or the file load will fail and raise
        // an exception.
        if( strCompare( "$Descr", line ) )
            loadPageSettings( reader, aScreen );
        else if( strCompare( "$Comp", line ) )
            aScreen->Append( loadComponent( reader ) );
        else if( strCompare( "$Sheet", line ) )
            aScreen->Append( loadSheet( reader ) );
        else if( strCompare( "$Bitmap", line ) )
            aScreen->Append( loadBitmap( reader ) );
        else if( strCompare( "Connection", line ) )
            aScreen->Append( loadJunction( reader ) );
        else if( strCompare( "NoConn", line ) )
            aScreen->Append( loadNoConnect( reader ) );
        else if( strCompare( "Wire", line ) )
            aScreen->Append( loadWire( reader ) );
        else if( strCompare( "Entry", line ) )
            aScreen->Append( loadBusEntry( reader ) );
        else if( strCompare( "Text", line ) )
            aScreen->Append( loadText( reader ) );
        else if( strCompare( "$EndSCHEMATC", line ) )
            return;
    }


    THROW_IO_ERROR( "'$EndSCHEMATC' not found" );
}


void SCH_LEGACY_PLUGIN::loadHeader( FILE_LINE_READER& aReader, SCH_SCREEN* aScreen )
{
    const char* line = aReader.ReadLine();

    if( !strCompare( "Eeschema Schematic File Version", line, &line ) )
    {
        m_error.Printf( _( "'%s' does not appear to be an Eeschema file" ),
                        GetChars( aScreen->GetFileName() ) );
        THROW_IO_ERROR( m_error );
    }

    // get the file version here.
    m_version = parseInt( aReader, line, &line );

    // The next lines are the lib list section, and are mainly comments, like:
    // LIBS:power
    // the lib list is not used, but is in schematic file just in case.
    // It is usually not empty, but we accept empty list.
    // If empty, there is a legacy section, not used
    // EELAYER i j
    // and the last line is
    // EELAYER END
    // Skip all lines until the end of header "EELAYER END" is found
    while( aReader.ReadLine() )
    {
        line = aReader.Line();

        while( *line == ' ' )
            line++;

        if( strCompare( "EELAYER END", line ) )
            return;
    }

    THROW_IO_ERROR( _( "Missing 'EELAYER END'" ) );
}


void SCH_LEGACY_PLUGIN::loadPageSettings( FILE_LINE_READER& aReader, SCH_SCREEN* aScreen )
{
    wxASSERT( aScreen != NULL );

    wxString    buf;
    const char* line = aReader.Line();

    PAGE_INFO   pageInfo;
    TITLE_BLOCK tb;

    wxCHECK_RET( strCompare( "$Descr", line, &line ), "Invalid sheet description" );

    parseUnquotedString( buf, aReader, line, &line );

    if( !pageInfo.SetType( buf ) )
        SCH_PARSE_ERROR( _( "invalid page size" ), aReader, line );

    if( buf == PAGE_INFO::Custom )
    {
        pageInfo.SetWidthMils( parseInt( aReader, line, &line ) );
        pageInfo.SetHeightMils( parseInt( aReader, line, &line ) );
    }
    else
    {
        wxString orientation;

        // Non custom size, set portrait if its present.  Can be empty string which defaults
        // to landscape.
        parseUnquotedString( orientation, aReader, line, &line, true );

        if( orientation == "portrait" )
            pageInfo.SetPortrait( true );
    }

    aScreen->SetPageSettings( pageInfo );

    while( line != NULL )
    {
        buf.clear();

        if( !aReader.ReadLine() )
            SCH_PARSE_ERROR( _( "unexpected end of file" ), aReader, line );

        line = aReader.Line();

        if( strCompare( "Sheet", line, &line ) )
        {
            aScreen->m_ScreenNumber = parseInt( aReader, line, &line );
            aScreen->m_NumberOfScreens = parseInt( aReader, line, &line );
        }
        else if( strCompare( "Title", line, &line ) )
        {
            parseQuotedString( buf, aReader, line, &line, true );
            tb.SetTitle( buf );
        }
        else if( strCompare( "Date", line, &line ) )
        {
            parseQuotedString( buf, aReader, line, &line, true );
            tb.SetDate( buf );
        }
        else if( strCompare( "Rev", line, &line ) )
        {
            parseQuotedString( buf, aReader, line, &line, true );
            tb.SetRevision( buf );
        }
        else if( strCompare( "Comp", line, &line ) )
        {
            parseQuotedString( buf, aReader, line, &line, true );
            tb.SetCompany( buf );
        }
        else if( strCompare( "Comment1", line, &line ) )
        {
            parseQuotedString( buf, aReader, line, &line, true );
            tb.SetComment1( buf );
        }
        else if( strCompare( "Comment2", line, &line ) )
        {
            parseQuotedString( buf, aReader, line, &line, true );
            tb.SetComment2( buf );
        }
        else if( strCompare( "Comment3", line, &line ) )
        {
            parseQuotedString( buf, aReader, line, &line, true );
            tb.SetComment3( buf );
        }
        else if( strCompare( "Comment4", line, &line ) )
        {
            parseQuotedString( buf, aReader, line, &line, true );
            tb.SetComment4( buf );
        }
        else if( strCompare( "$EndDescr", line ) )
        {
            aScreen->SetTitleBlock( tb );
            return;
        }
    }

    SCH_PARSE_ERROR( _( "missing 'EndDescr'" ), aReader, line );
}


SCH_SHEET* SCH_LEGACY_PLUGIN::loadSheet( FILE_LINE_READER& aReader )
{
    std::auto_ptr< SCH_SHEET > sheet( new SCH_SHEET() );

    sheet->SetTimeStamp( GetNewTimeStamp() );

    const char* line = aReader.ReadLine();

    while( line != NULL )
    {
        if( strCompare( "S", line, &line ) )        // Sheet dimensions.
        {
            wxPoint position;

            position.x = parseInt( aReader, line, &line );
            position.y = parseInt( aReader, line, &line );
            sheet->SetPosition( position );

            wxSize  size;

            size.SetWidth( parseInt( aReader, line, &line ) );
            size.SetHeight( parseInt( aReader, line, &line ) );
            sheet->SetSize( size );
        }
        else if( strCompare( "U", line, &line ) )   // Sheet time stamp.
        {
            sheet->SetTimeStamp( parseHex( aReader, line ) );
        }
        else if( *line == 'F' )                     // Sheet field.
        {
            line++;

            wxString text;
            int size;
            int fieldId = parseInt( aReader, line, &line );

            if( fieldId == 0 || fieldId == 1 )      // Sheet name and file name.
            {
                parseQuotedString( text, aReader, line, &line );
                size = parseInt( aReader, line, &line );

                if( fieldId == 0 )
                {
                    sheet->SetName( text );
                    sheet->SetSheetNameSize( size );
                }
                else
                {
                    sheet->SetFileName( text );
                    sheet->SetFileNameSize( size );
                }
            }
            else                                   // Sheet pin.
            {
                std::auto_ptr< SCH_SHEET_PIN > sheetPin( new SCH_SHEET_PIN( sheet.get() ) );

                sheetPin->SetNumber( fieldId );

                // Can be empty fields.
                parseQuotedString( text, aReader, line, &line, true );

                sheetPin->SetText( text );

                if( line == NULL )
                    THROW_IO_ERROR( _( "unexpected end of line" ) );

                switch( parseChar( aReader, line, &line ) )
                {
                case 'I':
                    sheetPin->SetShape( NET_INPUT );
                    break;

                case 'O':
                    sheetPin->SetShape( NET_OUTPUT );
                    break;

                case 'B':
                    sheetPin->SetShape( NET_BIDI );
                    break;

                case 'T':
                    sheetPin->SetShape( NET_TRISTATE );
                    break;

                case 'U':
                    sheetPin->SetShape( NET_UNSPECIFIED );
                    break;
                default:
                    SCH_PARSE_ERROR( _( "invalid sheet pin type" ), aReader, line );
                }

                switch( parseChar( aReader, line, &line ) )
                {
                case 'R': /* pin on right side */
                    sheetPin->SetEdge( SCH_SHEET_PIN::SHEET_RIGHT_SIDE );
                    break;

                case 'T': /* pin on top side */
                    sheetPin->SetEdge( SCH_SHEET_PIN::SHEET_TOP_SIDE );
                    break;

                case 'B': /* pin on bottom side */
                    sheetPin->SetEdge( SCH_SHEET_PIN::SHEET_BOTTOM_SIDE );
                    break;

                case 'L': /* pin on left side */
                    sheetPin->SetEdge( SCH_SHEET_PIN::SHEET_LEFT_SIDE );
                    break;
                default:
                    SCH_PARSE_ERROR( _( "invalid sheet pin side" ), aReader, line );
                }

                wxPoint position;

                position.x = parseInt( aReader, line, &line );
                position.y = parseInt( aReader, line, &line );
                sheetPin->SetPosition( position );

                size = parseInt( aReader, line, &line );

                sheetPin->SetSize( wxSize( size, size ) );

                sheet->AddPin( sheetPin.release() );
            }
        }
        else if( strCompare( "$EndSheet", line ) )
            return sheet.release();

        line = aReader.ReadLine();
    }

    SCH_PARSE_ERROR( _( "missing '$EndSheet`" ), aReader, line );

    return NULL;  // Prevents compiler warning.  Should never get here.
}


SCH_BITMAP* SCH_LEGACY_PLUGIN::loadBitmap( FILE_LINE_READER& aReader )
{
    std::auto_ptr< SCH_BITMAP > bitmap( new SCH_BITMAP );

    const char* line = aReader.Line();

    wxASSERT( strCompare( "$Bitmap", line, &line ) );

    line = aReader.ReadLine();

    while( line != NULL )
    {
        if( strCompare( "Pos", line, &line ) )
        {
            wxPoint position;

            position.x = parseInt( aReader, line, &line );
            position.y = parseInt( aReader, line, &line );
            bitmap->SetPosition( position );
        }
        else if( strCompare( "Scale", line, &line ) )
        {
            /// @todo Make m_scale private and add accessors.
            bitmap->m_Image->m_Scale = parseDouble( aReader, line, &line );
        }
        else if( strCompare( "Data", line, &line ) )
        {
            wxMemoryOutputStream stream;

            while( line )
            {
                if( !aReader.ReadLine() )
                    SCH_PARSE_ERROR( _( "Unexpected end of file" ), aReader, line );

                line = aReader.Line();

                if( strCompare( "EndData", line ) )
                {
                    // all the PNG date is read.
                    // We expect here m_image and m_bitmap are void
                    wxImage* image = new wxImage();
                    wxMemoryInputStream istream( stream );
                    image->LoadFile( istream, wxBITMAP_TYPE_PNG );
                    bitmap->m_Image->SetImage( image );
                    bitmap->m_Image->SetBitmap( new wxBitmap( *image ) );
                    break;
                }

                // Read PNG data, stored in hexadecimal,
                // each byte = 2 hexadecimal digits and a space between 2 bytes
                // and put it in memory stream buffer
                int len = strlen( line );

                for( ; len > 0 && !isspace( *line ); len -= 3, line += 3 )
                {
                    int value = 0;

                    if( sscanf( line, "%X", &value ) == 1 )
                        stream.PutC( (char) value );
                    else
                        THROW_IO_ERROR( "invalid PNG data" );
                }
            }

            if( line == NULL )
                THROW_IO_ERROR( _( "unexpected end of file" ) );
        }
        else if( strCompare( "$EndBitmap", line ) )
            return bitmap.release();

        line = aReader.ReadLine();
    }

    THROW_IO_ERROR( _( "unexpected end of file" ) );
}


SCH_JUNCTION* SCH_LEGACY_PLUGIN::loadJunction( FILE_LINE_READER& aReader )
{
    std::auto_ptr< SCH_JUNCTION > junction( new SCH_JUNCTION );

    const char* line = aReader.Line();

    wxASSERT( strCompare( "Connection", line, &line ) );

    wxString name;

    parseUnquotedString( name, aReader, line, &line );

    wxPoint position;

    position.x = parseInt( aReader, line, &line );
    position.y = parseInt( aReader, line, &line );
    junction->SetPosition( position );

    return junction.release();
}


SCH_NO_CONNECT* SCH_LEGACY_PLUGIN::loadNoConnect( FILE_LINE_READER& aReader )
{
    std::auto_ptr< SCH_NO_CONNECT > no_connect( new SCH_NO_CONNECT );

    const char* line = aReader.Line();

    wxASSERT( strCompare( "NoConn", line, &line ) );

    wxString name;

    parseUnquotedString( name, aReader, line, &line );

    wxPoint position;

    position.x = parseInt( aReader, line, &line );
    position.y = parseInt( aReader, line, &line );
    no_connect->SetPosition( position );

    return no_connect.release();
}


SCH_LINE* SCH_LEGACY_PLUGIN::loadWire( FILE_LINE_READER& aReader )
{
    std::auto_ptr< SCH_LINE > wire( new SCH_LINE );

    const char* line = aReader.Line();

    wxASSERT( strCompare( "Wire", line, &line ) );

    if( strCompare( "Wire", line, &line ) )
        wire->SetLayer( LAYER_WIRE );
    else if( strCompare( "Bus", line, &line ) )
        wire->SetLayer( LAYER_BUS );
    else if( strCompare( "Notes", line, &line ) )
        wire->SetLayer( LAYER_NOTES );
    else
        SCH_PARSE_ERROR( "invalid line type", aReader, line );

    if( !strCompare( "Line", line, &line ) )
        SCH_PARSE_ERROR( "invalid wire definition", aReader, line );

    line = aReader.ReadLine();

    wxPoint begin, end;

    begin.x = parseInt( aReader, line, &line );
    begin.y = parseInt( aReader, line, &line );
    end.x = parseInt( aReader, line, &line );
    end.y = parseInt( aReader, line, &line );

    wire->SetStartPoint( begin );
    wire->SetEndPoint( end );

    return wire.release();
}


SCH_BUS_ENTRY_BASE* SCH_LEGACY_PLUGIN::loadBusEntry( FILE_LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxASSERT( strCompare( "Entry", line, &line ) );

    std::auto_ptr< SCH_BUS_ENTRY_BASE > busEntry;

    if( strCompare( "Wire", line, &line ) )
    {
        busEntry.reset( new SCH_BUS_WIRE_ENTRY );

        if( !strCompare( "Line", line, &line ) )
            SCH_PARSE_ERROR( "invalid bus entry definition expected 'Line'", aReader, line );
    }
    else if( strCompare( "Bus", line, &line ) )
    {
        busEntry.reset( new SCH_BUS_BUS_ENTRY );

        if( !strCompare( "Bus", line, &line ) )
            SCH_PARSE_ERROR( "invalid bus entry definition expected 'Bus'", aReader, line );
    }
    else
        SCH_PARSE_ERROR( "invalid bus entry type", aReader, line );

    line = aReader.ReadLine();

    wxPoint pos;
    wxSize size;

    pos.x = parseInt( aReader, line, &line );
    pos.y = parseInt( aReader, line, &line );
    size.x = parseInt( aReader, line, &line );
    size.y = parseInt( aReader, line, &line );

    size.x -= pos.x;
    size.y -= pos.y;

    busEntry->SetPosition( pos );
    busEntry->SetSize( size );

    return busEntry.release();
}


SCH_TEXT* SCH_LEGACY_PLUGIN::loadText( FILE_LINE_READER& aReader )
{
    const char*   line = aReader.Line();

    wxASSERT( strCompare( "Text", line, &line ) );

    std::auto_ptr< SCH_TEXT> text;

    if( strCompare( "Notes", line, &line ) )
        text.reset( new SCH_TEXT );
    else if( strCompare( "Label", line, &line ) )
        text.reset( new SCH_LABEL );
    else if( strCompare( "GLabel", line, &line ) )
        text.reset( new SCH_GLOBALLABEL );
    else if( strCompare( "HLabel", line, &line ) )
        text.reset( new SCH_HIERLABEL );
    else
        SCH_PARSE_ERROR( "unknown Text type", aReader, line );

    // Parse the parameters common to all text objects.
    wxPoint position;

    position.x = parseInt( aReader, line, &line );
    position.y = parseInt( aReader, line, &line );
    text->SetPosition( position );
    text->SetOrientation( parseInt( aReader, line, &line ) );

    int size = parseInt( aReader, line, &line );

    text->SetSize( wxSize( size, size ) );

    // Parse the global and hierarchical label type.
    if( text->Type() == SCH_HIERARCHICAL_LABEL_T || text->Type() == SCH_GLOBAL_LABEL_T )
    {
        if( strCompare( SheetLabelType[NET_INPUT], line, &line ) )
            text->SetShape( NET_INPUT );
        else if( strCompare( SheetLabelType[NET_OUTPUT], line, &line ) )
            text->SetShape( NET_OUTPUT );
        else if( strCompare( SheetLabelType[NET_BIDI], line, &line ) )
            text->SetShape( NET_BIDI );
        else if( strCompare( SheetLabelType[NET_TRISTATE], line, &line ) )
            text->SetShape( NET_TRISTATE );
        else if( strCompare( SheetLabelType[NET_UNSPECIFIED], line, &line ) )
            text->SetShape( NET_UNSPECIFIED );
        else
            SCH_PARSE_ERROR( _( "invalid label type" ), aReader, line );
    }

    // Parse the italics indicator.
    if( strCompare( "Italic", line, &line ) )
        text->SetItalic( true );
    else if( !strCompare( "~", line, &line ) )
        SCH_PARSE_ERROR( _( "expected 'Italics' or '~'" ), aReader, line );

    int thickness = parseInt( aReader, line );

    text->SetBold( thickness != 0 );
    text->SetThickness( thickness != 0 ? GetPenSizeForBold( size ) : 0 );

    // Read the text string for the text.
    char* tmp = aReader.ReadLine();

    tmp = strtok( tmp, "\r\n" );
    wxString val = FROM_UTF8( tmp );

    for( ; ; )
    {
        int i = val.find( wxT( "\\n" ) );

        if( i == wxNOT_FOUND )
            break;

        val.erase( i, 2 );
        val.insert( i, wxT( "\n" ) );
    }

    text->SetText( val );

    return text.release();
}


SCH_COMPONENT* SCH_LEGACY_PLUGIN::loadComponent( FILE_LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxASSERT( strCompare( "$Comp", line, &line ) );

    std::auto_ptr< SCH_COMPONENT > component( new SCH_COMPONENT() );

    line = aReader.ReadLine();

    while( line != NULL )
    {
        if( strCompare( "L", line, &line ) )
        {
            wxString libName;

            parseUnquotedString( libName, aReader, line, &line );
            libName.Replace( "~", " " );
            component->SetPartName( libName );

            wxString refDesignator;

            parseUnquotedString( refDesignator, aReader, line, &line );
            refDesignator.Replace( "~", " " );

            wxString prefix = refDesignator;

            while( prefix.Length() )
            {
                if( ( prefix.Last() < '0' || prefix.Last() > '9') && prefix.Last() != '?' )
                    break;

                prefix.RemoveLast();
            }

            // Avoid a prefix containing trailing/leading spaces
            prefix.Trim( true );
            prefix.Trim( false );

            if( prefix.IsEmpty() )
                component->SetPrefix( wxString( "U" ) );
            else
                component->SetPrefix( prefix );
        }
        else if( strCompare( "U", line, &line ) )
        {
            component->SetUnit( parseInt( aReader, line, &line ) );
            component->SetConvert( parseInt( aReader, line, &line ) );
            component->SetTimeStamp( parseHex( aReader, line, &line ) );
        }
        else if( strCompare( "P", line, &line ) )
        {
            wxPoint pos;

            pos.x = parseInt( aReader, line, &line );
            pos.y = parseInt( aReader, line, &line );
            component->SetPosition( pos );
        }
        else if( strCompare( "AR", line, &line ) )
        {
            const char* strCompare = "Path=";
            int         len = strlen( strCompare );

            if( strnicmp( strCompare, line, len ) != 0 )
                SCH_PARSE_ERROR( "missing 'Path=' token", aReader, line );

            line += len;
            wxString path, reference, unit;

            parseQuotedString( path, aReader, line, &line );

            strCompare = "Ref=";
            len = strlen( strCompare );

            if( strnicmp( strCompare, line, len ) != 0 )
                SCH_PARSE_ERROR( "missing 'Ref=' token", aReader, line );

            line+= len;
            parseQuotedString( reference, aReader, line, &line );

            strCompare = "Part=";
            len = strlen( strCompare );

            if( strnicmp( strCompare, line, len ) != 0 )
                SCH_PARSE_ERROR( "missing 'Part=' token", aReader, line );

            line+= len;
            parseQuotedString( unit, aReader, line, &line );

            long tmp;

            if( !unit.ToLong( &tmp, 10 ) )
                SCH_PARSE_ERROR( "expected integer value", aReader, line );

            if( tmp < 0 || tmp > 26 )
                SCH_PARSE_ERROR( "unit value out of range", aReader, line );

            component->AddHierarchicalReference( path, reference, (int)tmp );
            component->GetField( REFERENCE )->SetText( reference );

        }
        else if( strCompare( "F", line, &line ) )
        {
            int index = parseInt( aReader, line, &line );

            wxString text, name, textAttrs;

            parseQuotedString( text, aReader, line, &line, true );

            char orientation = parseChar( aReader, line, &line );
            wxPoint pos;
            pos.x = parseInt( aReader, line, &line );
            pos.y = parseInt( aReader, line, &line );
            int size = parseInt( aReader, line, &line );
            int attributes = parseHex( aReader, line, &line );
            char hjustify = parseChar( aReader, line, &line );

            parseUnquotedString( textAttrs, aReader, line, &line );

            // The name of the field is optional.
            parseQuotedString( name, aReader, line, &line, true );

            if( name.IsEmpty() )
                name = TEMPLATE_FIELDNAME::GetDefaultFieldName( index );

            if( index >= component->GetFieldCount() )
            {
                // The first MANDATOR_FIELDS _must_ be constructed within
                // the SCH_COMPONENT constructor.  This assert is simply here
                // to guard against a change in that constructor.
                wxASSERT( component->GetFieldCount() >= MANDATORY_FIELDS );

                // Ignore the _supplied_ fieldNdx.  It is not important anymore
                // if within the user defined fields region (i.e. >= MANDATORY_FIELDS).
                // We freely renumber the index to fit the next available field slot.
                index = component->GetFieldCount();  // new has this index after insertion

                SCH_FIELD field( wxPoint( 0, 0 ), -1, component.get(), name );
                component->AddField( field );
            }
            else
            {
                component->GetField( index )->SetName( name );
            }

            component->GetField( index )->SetText( text );

            if( orientation == 'H' )
                component->GetField( index )->SetOrientation( TEXT_ORIENT_HORIZ );
            else if( orientation == 'V' )
                component->GetField( index )->SetOrientation( TEXT_ORIENT_VERT );
            else
                SCH_PARSE_ERROR( _( "component field orientation must be H or V" ), aReader, line );

            if( hjustify == 'L' )
                component->GetField( index )->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            else if( hjustify == 'R' )
                component->GetField( index )->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
            else if( hjustify != 'C' )
                SCH_PARSE_ERROR( _( "component field text horizontal justification must be "
                                    "L, R, or C" ), aReader, line );

            component->GetField( index )->SetTextPosition( pos );
            component->GetField( index )->SetAttributes( attributes );
            component->GetField( index )->SetSize( wxSize( size, size ) );

            // We are guaranteed to have a least one character here for older file formats
            // otherwise an exception would have been raised..
            if( textAttrs[0] == 'T' )
                component->GetField( index )->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
            else if( textAttrs[0] == 'B' )
                component->GetField( index )->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
            else if( textAttrs[0] != 'C' )
                SCH_PARSE_ERROR( _( "component field text vertical justification must be "
                                    "B, T, or C" ), aReader, line );

            // Newer file formats include the bold and italics text attribute.
            if( textAttrs.Length() != 3 )
                SCH_PARSE_ERROR( _( "component field text attributes must be 3 characters wide" ),
                                 aReader, line );

            if( textAttrs[1] == 'I' )
                component->GetField( index )->SetItalic( true );
            else if( textAttrs[1] != 'N' )
                SCH_PARSE_ERROR( _( "component field text italics indicator must be I or N" ),
                                 aReader, line );

            if( textAttrs[2] == 'B' )
                component->GetField( index )->SetBold( true );
            else if( textAttrs[2] != 'N' )
                SCH_PARSE_ERROR( _( "component field text bold indicator must be B or N" ),
                                 aReader, line );
        }
        else if( strCompare( "$EndComp", line ) )
            return component.release();
        else
        {
            // There are two lines that begin with a tab or spaces that includes a line with the
            // redundant position information and the transform matrix settings.

            // Parse the redundant position information just the same to check for formatting
            // errors.
            parseInt( aReader, line, &line );    // Always 1.
            parseInt( aReader, line, &line );    // The X coordinate.
            parseInt( aReader, line, &line );    // The Y coordinate.

            line = aReader.ReadLine();

            TRANSFORM transform;

            transform.x1 = parseInt( aReader, line, &line );

            if( transform.x1 < -1 || transform.x1 > 1 )
                SCH_PARSE_ERROR( _( "invalid component X1 transform value" ), aReader, line );

            transform.y1 = parseInt( aReader, line, &line );

            if( transform.y1 < -1 || transform.y1 > 1 )
                SCH_PARSE_ERROR( _( "invalid component Y1 transform value" ), aReader, line );

            transform.x2 = parseInt( aReader, line, &line );

            if( transform.x2 < -1 || transform.x2 > 1 )
                SCH_PARSE_ERROR( _( "invalid component X2 transform value" ), aReader, line );

            transform.y2 = parseInt( aReader, line, &line );

            if( transform.y2 < -1 || transform.y2 > 1 )
                SCH_PARSE_ERROR( _( "invalid component Y2 transform value" ), aReader, line );

            component->SetTransform( transform );
        }

        line = aReader.ReadLine();
    }

    SCH_PARSE_ERROR( "invalid component line", aReader, line );

    return NULL;  // Prevents compiler warning.  Should never get here.
}
