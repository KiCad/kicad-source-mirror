/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>
#include <boost/algorithm/string/join.hpp>
#include <cctype>
#include <mutex>
#include <set>

#include <wx/mstream.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/textfile.h>
#include <wx/tokenzr.h>
#include <wx_filename.h>       // For ::ResolvePossibleSymlinks()

#include <bitmap_base.h>
#include <fmt/format.h>
#include <kiway.h>
#include <string_utils.h>
#include <richio.h>
#include <trace_helpers.h>
#include <trigo.h>
#include <progress_reporter.h>
#include <general.h>
#include <gr_text.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_symbol.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_no_connect.h>
#include <sch_text.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <bus_alias.h>
#include <io/io_utils.h>
#include <sch_io/kicad_legacy/sch_io_kicad_legacy_lib_cache.h>
#include <sch_io/kicad_legacy/sch_io_kicad_legacy.h>
#include <sch_io/kicad_legacy/sch_io_kicad_legacy_helpers.h>
#include <sch_screen.h>
#include <schematic.h>
#include <libraries/legacy_symbol_library.h>
#include <libraries/symbol_library_adapter.h>
#include <eeschema_id.h>       // for MAX_UNIT_COUNT_PER_PACKAGE definition
#include <tool/selection.h>
#include <wildcards_and_files_ext.h>


// Tokens to read/save graphic lines style
#define T_STYLE "style"
#define T_COLOR "rgb"          // cannot be modified (used by wxWidgets)
#define T_COLORA "rgba"        // cannot be modified (used by wxWidgets)
#define T_WIDTH "width"


SCH_IO_KICAD_LEGACY::SCH_IO_KICAD_LEGACY() : SCH_IO( wxS( "Eeschema legacy" ) ),
    m_appending( false ),
    m_lineReader( nullptr ),
    m_lastProgressLine( 0 ),
    m_lineCount( 0 )
{
    init( nullptr );
}


SCH_IO_KICAD_LEGACY::~SCH_IO_KICAD_LEGACY()
{
    delete m_cache;
}


void SCH_IO_KICAD_LEGACY::init( SCHEMATIC* aSchematic, const std::map<std::string, UTF8>* aProperties )
{
    m_version   = 0;
    m_rootSheet = nullptr;
    m_currentSheet = nullptr;
    m_schematic = aSchematic;
    m_cache     = nullptr;
    m_out       = nullptr;
}


void SCH_IO_KICAD_LEGACY::checkpoint()
{
    const unsigned PROGRESS_DELTA = 250;

    if( m_progressReporter )
    {
        unsigned curLine = m_lineReader->LineNumber();

        if( curLine > m_lastProgressLine + PROGRESS_DELTA )
        {
            m_progressReporter->SetCurrentProgress( ( (double) curLine )
                                                            / std::max( 1U, m_lineCount ) );

            if( !m_progressReporter->KeepRefreshing() )
                THROW_IO_ERROR( _( "Open canceled by user." ) );

            m_lastProgressLine = curLine;
        }
    }
}


SCH_SHEET* SCH_IO_KICAD_LEGACY::LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                                   SCH_SHEET*             aAppendToMe,
                                                   const std::map<std::string, UTF8>* aProperties )
{
    wxASSERT( !aFileName || aSchematic != nullptr );

    SCH_SHEET*  sheet;
    wxFileName fn = aFileName;

    // Unfortunately child sheet file names the legacy schematic file format are not fully
    // qualified and are always appended to the project path.  The aFileName attribute must
    // always be an absolute path so the project path can be used for load child sheet files.
    wxASSERT( fn.IsAbsolute() );

    if( aAppendToMe )
    {
        wxLogTrace( traceSchLegacyPlugin, "Append \"%s\" to sheet \"%s\".",
                    aFileName, aAppendToMe->GetFileName() );

        wxFileName normedFn = aAppendToMe->GetFileName();

        if( !normedFn.IsAbsolute() )
        {
            if( aFileName.Right( normedFn.GetFullPath().Length() ) == normedFn.GetFullPath() )
                m_path = aFileName.Left( aFileName.Length() - normedFn.GetFullPath().Length() );
        }

        if( m_path.IsEmpty() )
            m_path = aSchematic->Project().GetProjectPath();

        wxLogTrace( traceSchLegacyPlugin, "Normalized append path \"%s\".", m_path );
    }
    else
    {
        m_path = aSchematic->Project().GetProjectPath();
    }

    m_currentPath.push( m_path );
    init( aSchematic, aProperties );

    if( aAppendToMe == nullptr )
    {
        // Clean up any allocated memory if an exception occurs loading the schematic.
        std::unique_ptr<SCH_SHEET> newSheet = std::make_unique<SCH_SHEET>( aSchematic );
        newSheet->SetFileName( aFileName );
        m_rootSheet = newSheet.get();
        loadHierarchy( newSheet.get() );

        // If we got here, the schematic loaded successfully.
        sheet = newSheet.release();
        m_rootSheet = nullptr;         // Quiet Coverity warning.
    }
    else
    {
        m_appending = true;
        wxCHECK_MSG( aSchematic->IsValid(), nullptr, "Can't append to a schematic with no root!" );
        m_rootSheet = &aSchematic->Root();
        sheet = aAppendToMe;
        loadHierarchy( sheet );
    }

    wxASSERT( m_currentPath.size() == 1 );  // only the project path should remain

    return sheet;
}


// Everything below this comment is recursive.  Modify with care.

void SCH_IO_KICAD_LEGACY::loadHierarchy( SCH_SHEET* aSheet )
{
    SCH_SCREEN* screen = nullptr;

    m_currentSheet = aSheet;

    if( !aSheet->GetScreen() )
    {
        // SCH_SCREEN objects store the full path and file name where the SCH_SHEET object only
        // stores the file name and extension.  Add the project path to the file name and
        // extension to compare when calling SCH_SHEET::SearchHierarchy().
        wxFileName fileName = aSheet->GetFileName();
        fileName.SetExt( "sch" );

        if( !fileName.IsAbsolute() )
            fileName.MakeAbsolute( m_currentPath.top() );

        // Save the current path so that it gets restored when descending and ascending the
        // sheet hierarchy which allows for sheet schematic files to be nested in folders
        // relative to the last path a schematic was loaded from.
        wxLogTrace( traceSchLegacyPlugin, "Saving path    '%s'", m_currentPath.top() );
        m_currentPath.push( fileName.GetPath() );
        wxLogTrace( traceSchLegacyPlugin, "Current path   '%s'", m_currentPath.top() );
        wxLogTrace( traceSchLegacyPlugin, "Loading        '%s'", fileName.GetFullPath() );

        m_rootSheet->SearchHierarchy( fileName.GetFullPath(), &screen );

        if( screen )
        {
            aSheet->SetScreen( screen );
            screen->SetParent( m_schematic );
            // Do not need to load the sub-sheets - this has already been done.
        }
        else
        {
            aSheet->SetScreen( new SCH_SCREEN( m_schematic ) );
            aSheet->GetScreen()->SetFileName( fileName.GetFullPath() );

            if( aSheet == m_rootSheet )
                const_cast<KIID&>( aSheet->m_Uuid ) = aSheet->GetScreen()->GetUuid();

            try
            {
                loadFile( fileName.GetFullPath(), aSheet->GetScreen() );
            }
            catch( const IO_ERROR& ioe )
            {
                // If there is a problem loading the root sheet, there is no recovery.
                if( aSheet == m_rootSheet )
                    throw( ioe );

                // For all subsheets, queue up the error message for the caller.
                if( !m_error.IsEmpty() )
                    m_error += "\n";

                m_error += ioe.What();
            }

            aSheet->GetScreen()->SetFileReadOnly( !fileName.IsFileWritable() );
            aSheet->GetScreen()->SetFileExists( true );

            for( SCH_ITEM* aItem : aSheet->GetScreen()->Items().OfType( SCH_SHEET_T ) )
            {
                wxCHECK2( aItem->Type() == SCH_SHEET_T, continue );
                auto sheet = static_cast<SCH_SHEET*>( aItem );

                // Set the parent to aSheet.  This effectively creates a method to find
                // the root sheet from any sheet so a pointer to the root sheet does not
                // need to be stored globally.  Note: this is not the same as a hierarchy.
                // Complex hierarchies can have multiple copies of a sheet.  This only
                // provides a simple tree to find the root sheet.
                sheet->SetParent( aSheet );

                // Recursion starts here.
                loadHierarchy( sheet );
            }
        }

        m_currentPath.pop();
        wxLogTrace( traceSchLegacyPlugin, "Restoring path \"%s\"", m_currentPath.top() );
    }
}


void SCH_IO_KICAD_LEGACY::loadFile( const wxString& aFileName, SCH_SCREEN* aScreen )
{
    FILE_LINE_READER reader( aFileName );

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_ERROR( _( "Open canceled by user." ) );

        m_lineReader = &reader;
        m_lineCount = 0;

        while( reader.ReadLine() )
            m_lineCount++;

        reader.Rewind();
    }

    loadHeader( reader, aScreen );

    LoadContent( reader, aScreen, m_version );

    // Unfortunately schematic files prior to version 2 are not terminated with $EndSCHEMATC
    // so checking for its existance will fail so just exit here and take our chances. :(
    if( m_version > 1 )
    {
        char* line = reader.Line();

        while( *line == ' ' )
            line++;

        if( !strCompare( "$EndSCHEMATC", line ) )
            THROW_IO_ERROR( "'$EndSCHEMATC' not found" );
    }
}


void SCH_IO_KICAD_LEGACY::LoadContent( LINE_READER& aReader, SCH_SCREEN* aScreen, int version )
{
    m_version = version;

    // We cannot safely load content without a set root level.
    wxCHECK_RET( m_rootSheet,
            "Cannot call SCH_IO_KICAD_LEGACY::LoadContent() without setting root sheet." );

    while( aReader.ReadLine() )
    {
        checkpoint();

        char* line = aReader.Line();

        while( *line == ' ' )
            line++;

        // Either an object will be loaded properly or the file load will fail and raise
        // an exception.
        if( strCompare( "$Descr", line ) )
            loadPageSettings( aReader, aScreen );
        else if( strCompare( "$Comp", line ) )
            aScreen->Append( loadSymbol( aReader ) );
        else if( strCompare( "$Sheet", line ) )
            aScreen->Append( loadSheet( aReader ) );
        else if( strCompare( "$Bitmap", line ) )
            aScreen->Append( loadBitmap( aReader ) );
        else if( strCompare( "Connection", line ) )
            aScreen->Append( loadJunction( aReader ) );
        else if( strCompare( "NoConn", line ) )
            aScreen->Append( loadNoConnect( aReader ) );
        else if( strCompare( "Wire", line ) )
            aScreen->Append( loadWire( aReader ) );
        else if( strCompare( "Entry", line ) )
            aScreen->Append( loadBusEntry( aReader ) );
        else if( strCompare( "Text", line ) )
            aScreen->Append( loadText( aReader ) );
        else if( strCompare( "BusAlias", line ) )
            aScreen->AddBusAlias( loadBusAlias( aReader, aScreen ) );
        else if( strCompare( "Kmarq", line ) )
            continue; // Ignore legacy (until 2009) ERC marker entry
        else if( strCompare( "$EndSCHEMATC", line ) )
            return;
        else
            SCH_PARSE_ERROR( "unrecognized token", aReader, line );
    }
}


void SCH_IO_KICAD_LEGACY::loadHeader( LINE_READER& aReader, SCH_SCREEN* aScreen )
{
    const char* line = aReader.ReadLine();

    if( !line || !strCompare( "Eeschema Schematic File Version", line, &line ) )
    {
        m_error.Printf( _( "'%s' does not appear to be an Eeschema file." ),
                        aScreen->GetFileName() );
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
        checkpoint();

        line = aReader.Line();

        while( *line == ' ' )
            line++;

        if( strCompare( "EELAYER END", line ) )
            return;
    }

    THROW_IO_ERROR( _( "Missing 'EELAYER END'" ) );
}


void SCH_IO_KICAD_LEGACY::loadPageSettings( LINE_READER& aReader, SCH_SCREEN* aScreen )
{
    wxASSERT( aScreen != nullptr );

    wxString    buf;
    const char* line = aReader.Line();

    PAGE_INFO   pageInfo;
    TITLE_BLOCK tb;

    wxCHECK_RET( strCompare( "$Descr", line, &line ), "Invalid sheet description" );

    parseUnquotedString( buf, aReader, line, &line );

    if( !pageInfo.SetType( buf ) )
        SCH_PARSE_ERROR( "invalid page size", aReader, line );

    int pagew = parseInt( aReader, line, &line );
    int pageh = parseInt( aReader, line, &line );

    if( pageInfo.GetType() == PAGE_SIZE_TYPE::User )
    {
        pageInfo.SetWidthMils( pagew );
        pageInfo.SetHeightMils( pageh );
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

    while( line != nullptr )
    {
        buf.clear();

        if( !aReader.ReadLine() )
            SCH_PARSE_ERROR( _( "unexpected end of file" ), aReader, line );

        line = aReader.Line();

        if( strCompare( "Sheet", line, &line ) )
        {
            aScreen->SetVirtualPageNumber( parseInt( aReader, line, &line ) );
            aScreen->SetPageCount( parseInt( aReader, line, &line ) );
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
            tb.SetComment( 0, buf );
        }
        else if( strCompare( "Comment2", line, &line ) )
        {
            parseQuotedString( buf, aReader, line, &line, true );
            tb.SetComment( 1, buf );
        }
        else if( strCompare( "Comment3", line, &line ) )
        {
            parseQuotedString( buf, aReader, line, &line, true );
            tb.SetComment( 2, buf );
        }
        else if( strCompare( "Comment4", line, &line ) )
        {
            parseQuotedString( buf, aReader, line, &line, true );
            tb.SetComment( 3, buf );
        }
        else if( strCompare( "Comment5", line, &line ) )
        {
            parseQuotedString( buf, aReader, line, &line, true );
            tb.SetComment( 4, buf );
        }
        else if( strCompare( "Comment6", line, &line ) )
        {
            parseQuotedString( buf, aReader, line, &line, true );
            tb.SetComment( 5, buf );
        }
        else if( strCompare( "Comment7", line, &line ) )
        {
            parseQuotedString( buf, aReader, line, &line, true );
            tb.SetComment( 6, buf );
        }
        else if( strCompare( "Comment8", line, &line ) )
        {
            parseQuotedString( buf, aReader, line, &line, true );
            tb.SetComment( 7, buf );
        }
        else if( strCompare( "Comment9", line, &line ) )
        {
            parseQuotedString( buf, aReader, line, &line, true );
            tb.SetComment( 8, buf );
        }
        else if( strCompare( "$EndDescr", line ) )
        {
            aScreen->SetTitleBlock( tb );
            return;
        }
    }

    SCH_PARSE_ERROR( "missing 'EndDescr'", aReader, line );
}


SCH_SHEET* SCH_IO_KICAD_LEGACY::loadSheet( LINE_READER& aReader )
{
    std::unique_ptr<SCH_SHEET> sheet = std::make_unique<SCH_SHEET>();

    const char* line = aReader.ReadLine();

    while( line != nullptr )
    {
        if( strCompare( "S", line, &line ) )        // Sheet dimensions.
        {
            VECTOR2I position;

            position.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
            position.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
            sheet->SetPosition( position );

            VECTOR2I size;

            size.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
            size.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
            sheet->SetSize( size );
        }
        else if( strCompare( "U", line, &line ) )   // Sheet UUID.
        {
            wxString text;
            parseUnquotedString( text, aReader, line );

            if( text != "00000000" )
                const_cast<KIID&>( sheet->m_Uuid ) = KIID( text );
        }
        else if( *line == 'F' )                     // Sheet field.
        {
            line++;

            wxString text;
            int size;
            int legacy_field_id = parseInt( aReader, line, &line );

            if( legacy_field_id == 0 || legacy_field_id == 1 )      // Sheet name and file name.
            {
                parseQuotedString( text, aReader, line, &line );
                size = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );

                SCH_FIELD* field = sheet->GetField( legacy_field_id == 0 ? FIELD_T::SHEET_NAME
                                                                         : FIELD_T::SHEET_FILENAME );
                field->SetText( text );
                field->SetTextSize( VECTOR2I( size, size ) );
            }
            else                                   // Sheet pin.
            {
                // Use a unique_ptr so that we clean up in the case of a throw
                std::unique_ptr<SCH_SHEET_PIN> sheetPin = std::make_unique<SCH_SHEET_PIN>( sheet.get() );

                sheetPin->SetNumber( legacy_field_id );

                // Can be empty fields.
                parseQuotedString( text, aReader, line, &line, true );

                sheetPin->SetText( ConvertToNewOverbarNotation( text ) );

                if( line == nullptr )
                    THROW_IO_ERROR( _( "unexpected end of line" ) );

                switch( parseChar( aReader, line, &line ) )
                {
                case 'I': sheetPin->SetShape( LABEL_FLAG_SHAPE::L_INPUT );       break;
                case 'O': sheetPin->SetShape( LABEL_FLAG_SHAPE::L_OUTPUT );      break;
                case 'B': sheetPin->SetShape( LABEL_FLAG_SHAPE::L_BIDI );        break;
                case 'T': sheetPin->SetShape( LABEL_FLAG_SHAPE::L_TRISTATE );    break;
                case 'U': sheetPin->SetShape( LABEL_FLAG_SHAPE::L_UNSPECIFIED ); break;
                default:  SCH_PARSE_ERROR( "invalid sheet pin type", aReader, line );
                }

                switch( parseChar( aReader, line, &line ) )
                {
                case 'R': sheetPin->SetSide( SHEET_SIDE::RIGHT );  break;
                case 'T': sheetPin->SetSide( SHEET_SIDE::TOP );    break;
                case 'B': sheetPin->SetSide( SHEET_SIDE::BOTTOM ); break;
                case 'L': sheetPin->SetSide( SHEET_SIDE::LEFT );   break;
                default:  SCH_PARSE_ERROR( "invalid sheet pin side", aReader, line );
                }

                VECTOR2I position;

                position.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
                position.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
                sheetPin->SetPosition( position );

                size = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );

                sheetPin->SetTextSize( VECTOR2I( size, size ) );

                sheet->AddPin( sheetPin.release() );
            }
        }
        else if( strCompare( "$EndSheet", line ) )
        {
            sheet->AutoplaceFields( nullptr, AUTOPLACE_AUTO );
            return sheet.release();
        }

        line = aReader.ReadLine();
    }

    SCH_PARSE_ERROR( "missing '$EndSheet`", aReader, line );

    return nullptr;  // Prevents compiler warning.  Should never get here.
}


SCH_BITMAP* SCH_IO_KICAD_LEGACY::loadBitmap( LINE_READER& aReader )
{
    std::unique_ptr<SCH_BITMAP> bitmap = std::make_unique<SCH_BITMAP>();
    REFERENCE_IMAGE&            refImage = bitmap->GetReferenceImage();

    const char* line = aReader.Line();

    wxCHECK( strCompare( "$Bitmap", line, &line ), nullptr );

    line = aReader.ReadLine();

    while( line != nullptr )
    {
        if( strCompare( "Pos", line, &line ) )
        {
            VECTOR2I position;

            position.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
            position.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
            bitmap->SetPosition( position );
        }
        else if( strCompare( "Scale", line, &line ) )
        {
            auto scalefactor = parseDouble( aReader, line, &line );

            // Prevent scalefactor values that cannot be displayed.
            // In the case of a bad value, we accept that the image might be mis-scaled
            // rather than removing the full image.  Users can then edit the scale factor in
            // Eeschema to the appropriate value
            if( !std::isnormal( scalefactor ) )
                scalefactor = 1.0;

            refImage.SetImageScale( scalefactor );
        }
        else if( strCompare( "Data", line, &line ) )
        {
            wxMemoryBuffer buffer;

            while( line )
            {
                if( !aReader.ReadLine() )
                    SCH_PARSE_ERROR( _( "Unexpected end of file" ), aReader, line );

                line = aReader.Line();

                if( strCompare( "EndData", line ) )
                {
                    // all the PNG date is read.
                    refImage.ReadImageFile( buffer );

                    // Legacy file formats assumed 300 image PPI at load.
                    const BITMAP_BASE& bitmapImage = refImage.GetImage();
                    refImage.SetImageScale( refImage.GetImageScale() * bitmapImage.GetPPI()
                                            / 300.0 );
                    break;
                }

                // Read PNG data, stored in hexadecimal,
                // each byte = 2 hexadecimal digits and a space between 2 bytes
                // and put it in memory stream buffer
                // Note:
                // Some old files created bu the V4 schematic versions have a extra
                // "$EndBitmap" at the end of the hexadecimal data. (Probably due to
                // a bug). So discard it
                int len = strlen( line );

                for( ; len > 0 && !isspace( *line ) && '$' != *line; len -= 3, line += 3 )
                {
                    int value = 0;

                    if( sscanf( line, "%X", &value ) == 1 )
                        buffer.AppendByte( (char) value );
                    else
                        THROW_IO_ERROR( "invalid PNG data" );
                }
            }

            if( line == nullptr )
                THROW_IO_ERROR( _( "unexpected end of file" ) );
        }
        else if( strCompare( "$EndBitmap", line ) )
        {
            return bitmap.release();
        }

        line = aReader.ReadLine();
    }

    THROW_IO_ERROR( _( "unexpected end of file" ) );
}


SCH_JUNCTION* SCH_IO_KICAD_LEGACY::loadJunction( LINE_READER& aReader )
{
    std::unique_ptr<SCH_JUNCTION> junction = std::make_unique<SCH_JUNCTION>();

    const char* line = aReader.Line();

    wxCHECK( strCompare( "Connection", line, &line ), nullptr );

    wxString name;

    parseUnquotedString( name, aReader, line, &line );

    VECTOR2I position;

    position.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    position.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    junction->SetPosition( position );

    return junction.release();
}


SCH_NO_CONNECT* SCH_IO_KICAD_LEGACY::loadNoConnect( LINE_READER& aReader )
{
    std::unique_ptr<SCH_NO_CONNECT> no_connect = std::make_unique<SCH_NO_CONNECT>();

    const char* line = aReader.Line();

    wxCHECK( strCompare( "NoConn", line, &line ), nullptr );

    wxString name;

    parseUnquotedString( name, aReader, line, &line );

    VECTOR2I position;

    position.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    position.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    no_connect->SetPosition( position );

    return no_connect.release();
}


SCH_LINE* SCH_IO_KICAD_LEGACY::loadWire( LINE_READER& aReader )
{
    std::unique_ptr<SCH_LINE> wire = std::make_unique<SCH_LINE>();

    const char* line = aReader.Line();

    wxCHECK( strCompare( "Wire", line, &line ), nullptr );

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

    // The default graphical line style was Dashed.
    if( wire->GetLayer() == LAYER_NOTES )
        wire->SetLineStyle( LINE_STYLE::DASH );

    // Since Sept 15, 2017, a line style is alloved (width, style, color)
    // Only non default values are stored
    while( !is_eol( *line ) )
    {
        wxString buf;

        parseUnquotedString( buf, aReader, line, &line );

        if( buf == ")" )
            continue;

        else if( buf == T_WIDTH )
        {
            int size = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
            wire->SetLineWidth( size );
        }
        else if( buf == T_STYLE )
        {
            parseUnquotedString( buf, aReader, line, &line );

            if( buf == wxT( "solid" ) )
                wire->SetLineStyle( LINE_STYLE::SOLID );
            else if( buf == wxT( "dashed" ) )
                wire->SetLineStyle( LINE_STYLE::DASH );
            else if( buf == wxT( "dash_dot" ) )
                wire->SetLineStyle( LINE_STYLE::DASHDOT );
            else if( buf == wxT( "dotted" ) )
                wire->SetLineStyle( LINE_STYLE::DOT );
        }
        else    // should be the color parameter.
        {
            // The color param is something like rgb(150, 40, 191)
            // and because there is no space between ( and 150
            // the first param is inside buf.
            // So break keyword and the first param into 2 separate strings.
            wxString prm, keyword;
            keyword = buf.BeforeLast( '(', &prm );

            if( ( keyword == T_COLOR ) || ( keyword == T_COLORA ) )
            {
                long color[4] = { 0 };

                int ii = 0;

                if( !prm.IsEmpty() )
                {
                    prm.ToLong( &color[ii] );
                    ii++;
                }

                int prm_count = ( keyword == T_COLORA ) ? 4 : 3;

                // fix opacity to 1.0 or 255, when not exists in file
                color[3] = 255;

                for(; ii < prm_count && !is_eol( *line ); ii++ )
                {
                    color[ii] = parseInt( aReader, line, &line );

                    // Skip the separator between values
                    if( *line == ',' || *line == ' ')
                        line++;
                }

                wire->SetLineColor( color[0]/255.0, color[1]/255.0, color[2]/255.0,color[3]/255.0 );
            }
        }
    }

    // Read the segment en points coordinates:
    line = aReader.ReadLine();

    VECTOR2I begin, end;

    begin.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    begin.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    end.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    end.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );

    wire->SetStartPoint( begin );
    wire->SetEndPoint( end );

    return wire.release();
}


SCH_BUS_ENTRY_BASE* SCH_IO_KICAD_LEGACY::loadBusEntry( LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxCHECK( strCompare( "Entry", line, &line ), nullptr );

    std::unique_ptr<SCH_BUS_ENTRY_BASE> busEntry;

    if( strCompare( "Wire", line, &line ) )
    {
        busEntry = std::make_unique<SCH_BUS_WIRE_ENTRY>();

        if( !strCompare( "Line", line, &line ) )
            SCH_PARSE_ERROR( "invalid bus entry definition expected 'Line'", aReader, line );
    }
    else if( strCompare( "Bus", line, &line ) )
    {
        busEntry = std::make_unique<SCH_BUS_BUS_ENTRY>();

        if( !strCompare( "Bus", line, &line ) )
            SCH_PARSE_ERROR( "invalid bus entry definition expected 'Bus'", aReader, line );
    }
    else
    {
        SCH_PARSE_ERROR( "invalid bus entry type", aReader, line );
    }

    line = aReader.ReadLine();

    VECTOR2I pos;
    VECTOR2I size;

    pos.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    pos.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    size.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    size.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );

    size.x -= pos.x;
    size.y -= pos.y;

    busEntry->SetPosition( pos );
    busEntry->SetSize( size );

    return busEntry.release();
}


// clang-format off
const std::map<LABEL_FLAG_SHAPE, const char*> sheetLabelNames
{
    { LABEL_FLAG_SHAPE::L_INPUT,       "Input" },
    { LABEL_FLAG_SHAPE::L_OUTPUT,      "Output" },
    { LABEL_FLAG_SHAPE::L_BIDI,        "BiDi" },
    { LABEL_FLAG_SHAPE::L_TRISTATE,    "3State" },
    { LABEL_FLAG_SHAPE::L_UNSPECIFIED, "UnSpc" },
};
// clang-format on


SCH_TEXT* SCH_IO_KICAD_LEGACY::loadText( LINE_READER& aReader )
{
    const char* line = aReader.Line();
    KICAD_T     textType = TYPE_NOT_INIT;

    wxCHECK( strCompare( "Text", line, &line ), nullptr );

    if( strCompare( "Notes", line, &line ) )
    {
        textType = SCH_TEXT_T;
    }
    else if( strCompare( "Label", line, &line ) )
    {
        textType = SCH_LABEL_T;
    }
    else if( strCompare( "HLabel", line, &line ) )
    {
        textType = SCH_HIER_LABEL_T;
    }
    else if( strCompare( "GLabel", line, &line ) )
    {
        // Prior to version 2, the SCH_GLOBALLABEL object did not exist.
        if( m_version == 1 )
            textType = SCH_HIER_LABEL_T;
        else
            textType = SCH_GLOBAL_LABEL_T;
    }
    else
    {
        SCH_PARSE_ERROR( "unknown Text type", aReader, line );
    }

    VECTOR2I position;

    position.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
    position.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );

    std::unique_ptr<SCH_TEXT> text;

    switch( textType )
    {
    case SCH_TEXT_T:         text.reset( new SCH_TEXT( position ) );        break;
    case SCH_LABEL_T:        text.reset( new SCH_LABEL( position ) );       break;
    case SCH_HIER_LABEL_T:   text.reset( new SCH_HIERLABEL( position ) );   break;
    case SCH_GLOBAL_LABEL_T: text.reset( new SCH_GLOBALLABEL( position ) ); break;
    default:                                                                break;
    }

    int spinStyle = parseInt( aReader, line, &line );

    // Sadly we store the orientation of hierarchical and global labels using a different
    // int encoding than that for local labels:
    //                   Global      Local
    // Left justified      0           2
    // Up                  1           3
    // Right justified     2           0
    // Down                3           1
    // So we must flip it as the enum is setup with the "global" numbering
    if( textType != SCH_GLOBAL_LABEL_T && textType != SCH_HIER_LABEL_T  )
    {
        if( spinStyle == 0 )
            spinStyle = 2;
        else if( spinStyle == 2 )
            spinStyle = 0;
    }

    int size = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );

    text->SetTextSize( VECTOR2I( size, size ) );

    if( textType == SCH_LABEL_T || textType == SCH_HIER_LABEL_T || textType == SCH_GLOBAL_LABEL_T )
    {
        SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( text.get() );

        label->SetSpinStyle( static_cast<SPIN_STYLE::SPIN>( spinStyle ) );

        // Parse the global and hierarchical label type.
        if( textType == SCH_HIER_LABEL_T || textType == SCH_GLOBAL_LABEL_T )
        {
            auto resultIt = std::find_if( sheetLabelNames.begin(), sheetLabelNames.end(),
                    [ &line ]( const auto& it )
                    {
                        return strCompare( it.second, line, &line );
                    } );

            if( resultIt != sheetLabelNames.end() )
                label->SetShape( resultIt->first );
            else
                SCH_PARSE_ERROR( "invalid label type", aReader, line );
        }
    }
    else if( textType == SCH_TEXT_T )
    {
        switch( spinStyle )
        {
        case SPIN_STYLE::RIGHT:            // Horiz Normal Orientation
            text->SetTextAngle( ANGLE_HORIZONTAL );
            text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
            break;

        case SPIN_STYLE::UP:               // Vert Orientation UP
            text->SetTextAngle( ANGLE_VERTICAL );
            text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
            break;

        case SPIN_STYLE::LEFT:             // Horiz Orientation - Right justified
            text->SetTextAngle( ANGLE_HORIZONTAL );
            text->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
            break;

        case SPIN_STYLE::BOTTOM:           //  Vert Orientation BOTTOM
            text->SetTextAngle( ANGLE_VERTICAL );
            text->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
            break;
        }

        text->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
    }

    int penWidth = 0;

    // The following tokens do not exist in version 1 schematic files,
    // and not always in version 2 for HLabels and GLabels
    if( m_version > 1 )
    {
        if( m_version > 2 || *line >= ' ' )
        {
            if( strCompare( "Italic", line, &line ) )
                text->SetItalicFlag( true );
            else if( !strCompare( "~", line, &line ) )
                SCH_PARSE_ERROR( _( "expected 'Italics' or '~'" ), aReader, line );
        }

        // The penWidth token does not exist in older versions of the schematic file format
        // so calling parseInt will be made only if the EOL is not reached.
        if( *line >= ' ' )
            penWidth = parseInt( aReader, line, &line );
    }

    text->SetBoldFlag( penWidth != 0 );
    text->SetTextThickness( penWidth != 0 ? GetPenSizeForBold( size ) : 0 );

    // Read the text string for the text.
    char* tmp = aReader.ReadLine();

    tmp = strtok( tmp, "\r\n" );
    wxString val = From_UTF8( tmp );

    for( ; ; )
    {
        size_t i = val.find( wxT( "\\n" ) );

        if( i == wxString::npos )
            break;

        val.erase( i, 2 );
        val.insert( i, wxT( "\n" ) );
    }

    text->SetText( ConvertToNewOverbarNotation( val ) );

    return text.release();
}


SCH_SYMBOL* SCH_IO_KICAD_LEGACY::loadSymbol( LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxCHECK( strCompare( "$Comp", line, &line ), nullptr );

    std::unique_ptr<SCH_SYMBOL> symbol = std::make_unique<SCH_SYMBOL>();

    line = aReader.ReadLine();

    while( line != nullptr )
    {
        if( strCompare( "L", line, &line ) )
        {
            wxString libName;
            size_t pos = 2;                               // "X" plus ' ' space character.
            wxString utf8Line = wxString::FromUTF8( line );
            wxStringTokenizer tokens( utf8Line, " \t\r\n" );

            if( tokens.CountTokens() < 2 )
            {
                THROW_PARSE_ERROR( "invalid symbol library definition", aReader.GetSource(),
                                   aReader.Line(), aReader.LineNumber(), pos );
            }

            libName = tokens.GetNextToken();
            libName.Replace( "~", " " );

            LIB_ID libId;

            // Prior to schematic version 4, library IDs did not have a library nickname so
            // parsing the symbol name with LIB_ID::Parse() would break symbol library links
            // that contained '/' and ':' characters.
            if( m_version > 3 )
                libId.Parse( libName, true );
            else
                libId.SetLibItemName( libName );

            symbol->SetLibId( libId );

            wxString refDesignator = tokens.GetNextToken();

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
                symbol->SetPrefix( wxString( "U" ) );
            else
                symbol->SetPrefix( prefix );
        }
        else if( strCompare( "U", line, &line ) )
        {
            // This fixes a potentially buggy files caused by unit being set to zero which
            // causes netlist issues.  See https://bugs.launchpad.net/kicad/+bug/1677282.
            int unit = parseInt( aReader, line, &line );

            if( unit == 0 )
            {
                unit = 1;

                // Set the file as modified so the user can be warned.
                if( m_rootSheet->GetScreen() )
                    m_rootSheet->GetScreen()->SetContentModified();
            }

            symbol->SetUnit( unit );

            // Same can also happen with the body style ("convert") parameter
            int bodyStyle = parseInt( aReader, line, &line );

            if( bodyStyle == 0 )
            {
                bodyStyle = 1;

                // Set the file as modified so the user can be warned.
                if( m_rootSheet->GetScreen() )
                    m_rootSheet->GetScreen()->SetContentModified();
            }

            symbol->SetBodyStyle( bodyStyle );

            wxString text;
            parseUnquotedString( text, aReader, line, &line );

            if( text != "00000000" )
                const_cast<KIID&>( symbol->m_Uuid ) = KIID( text );
        }
        else if( strCompare( "P", line, &line ) )
        {
            VECTOR2I pos;

            pos.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
            pos.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
            symbol->SetPosition( pos );
        }
        else if( strCompare( "AR", line, &line ) )
        {
            const char* strCompare = "Path=";
            int         len = strlen( strCompare );

            if( strncasecmp( strCompare, line, len ) != 0 )
                SCH_PARSE_ERROR( "missing 'Path=' token", aReader, line );

            line += len;
            wxString pathStr, reference, unit;

            parseQuotedString( pathStr, aReader, line, &line );

            // Note: AR path excludes root sheet, but includes symbol.  Drop the symbol ID
            // since it's already defined in the symbol itself.
            KIID_PATH path( pathStr );

            if( path.size() > 0 )
                path.pop_back();

            // In the new file format, the root schematic UUID is used as the virtual SCH_SHEET
            // UUID so we need to prefix it to the symbol path so the symbol instance paths
            // get saved with the root schematic UUID.
            if( !m_appending )
                path.insert( path.begin(), m_rootSheet->GetScreen()->GetUuid() );

            strCompare = "Ref=";
            len = strlen( strCompare );

            if( strncasecmp( strCompare, line, len ) != 0 )
                SCH_PARSE_ERROR( "missing 'Ref=' token", aReader, line );

            line+= len;
            parseQuotedString( reference, aReader, line, &line );

            strCompare = "Part=";
            len = strlen( strCompare );

            if( strncasecmp( strCompare, line, len ) != 0 )
                SCH_PARSE_ERROR( "missing 'Part=' token", aReader, line );

            line+= len;
            parseQuotedString( unit, aReader, line, &line );

            long tmp;

            if( !unit.ToLong( &tmp, 10 ) )
                SCH_PARSE_ERROR( "expected integer value", aReader, line );

            if( tmp < 0 || tmp > MAX_UNIT_COUNT_PER_PACKAGE )
                SCH_PARSE_ERROR( "unit value out of range", aReader, line );

            symbol->AddHierarchicalReference( path, reference, (int)tmp );
            symbol->GetField( FIELD_T::REFERENCE )->SetText( reference );
        }
        else if( strCompare( "F", line, &line ) )
        {
            int legacy_field_id = parseInt( aReader, line, &line );

            wxString text, name;

            parseQuotedString( text, aReader, line, &line, true );

            char orientation = parseChar( aReader, line, &line );
            VECTOR2I pos;
            pos.x = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
            pos.y = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );

            // Y got inverted in symbol coordinates
            pos.y = -( pos.y - symbol->GetY() ) + symbol->GetY();

            int size = schIUScale.MilsToIU( parseInt( aReader, line, &line ) );
            int attributes = parseHex( aReader, line, &line );

            SCH_FIELD* field;

            // Map fixed legacy IDs
            switch( legacy_field_id )
            {
            case 0:  field = symbol->GetField( FIELD_T::REFERENCE ); break;
            case 1:  field = symbol->GetField( FIELD_T::VALUE );     break;
            case 2:  field = symbol->GetField( FIELD_T::FOOTPRINT ); break;
            case 3:  field = symbol->GetField( FIELD_T::DATASHEET ); break;

            default:
                field = symbol->AddField( SCH_FIELD( symbol.get(), FIELD_T::USER ) );
                break;
            }

            // Prior to version 2 of the schematic file format, none of the following existed.
            if( m_version > 1 )
            {
                wxString textAttrs;
                char hjustify = parseChar( aReader, line, &line );

                parseUnquotedString( textAttrs, aReader, line, &line );

                // The name of the field is optional.
                parseQuotedString( name, aReader, line, &line, true );

                if( hjustify == 'L' )
                    field->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
                else if( hjustify == 'R' )
                    field->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
                else if( hjustify != 'C' )
                    SCH_PARSE_ERROR( "symbol field text horizontal justification must be "
                                     "L, R, or C", aReader, line );

                // We are guaranteed to have a least one character here for older file formats
                // otherwise an exception would have been raised..
                if( textAttrs[0] == 'T' )
                    field->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
                else if( textAttrs[0] == 'B' )
                    field->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
                else if( textAttrs[0] != 'C' )
                    SCH_PARSE_ERROR( "symbol field text vertical justification must be "
                                     "B, T, or C", aReader, line );

                // Newer file formats include the bold and italics text attribute.
                if( textAttrs.Length() > 1 )
                {
                    if( textAttrs.Length() != 3 )
                    {
                        SCH_PARSE_ERROR( _( "symbol field text attributes must be 3 characters wide" ),
                                         aReader, line );
                    }

                    if( textAttrs[1] == 'I' )
                    {
                        field->SetItalicFlag( true );
                    }
                    else if( textAttrs[1] != 'N' )
                    {
                        SCH_PARSE_ERROR( "symbol field text italics indicator must be I or N",
                                         aReader, line );
                    }

                    if( textAttrs[2] == 'B' )
                    {
                        field->SetBoldFlag( true );
                    }
                    else if( textAttrs[2] != 'N' )
                    {
                        SCH_PARSE_ERROR( "symbol field text bold indicator must be B or N",
                                         aReader, line );
                    }
                }
            }

            field->SetText( text );
            field->SetTextPos( pos );
            field->SetVisible( !attributes );
            field->SetTextSize( VECTOR2I( size, size ) );

            if( orientation == 'H' )
                field->SetTextAngle( ANGLE_HORIZONTAL );
            else if( orientation == 'V' )
                field->SetTextAngle( ANGLE_VERTICAL );
            else
                SCH_PARSE_ERROR( "symbol field orientation must be H or V", aReader, line );

            if( name.IsEmpty() )
            {
                if( field->IsMandatory() )
                    name = GetCanonicalFieldName( field->GetId() );
                else
                    name = GetUserFieldName( legacy_field_id, !DO_TRANSLATE );
            }

            field->SetName( name );
        }
        else if( strCompare( "$EndComp", line ) )
        {
            if( !m_appending )
            {
                if( m_currentSheet == m_rootSheet )
                {
                    KIID_PATH path;
                    path.push_back( m_rootSheet->GetScreen()->GetUuid() );

                    SCH_SYMBOL_INSTANCE instance;
                    instance.m_Path = path;
                    instance.m_Reference = symbol->GetField( FIELD_T::REFERENCE )->GetText();
                    instance.m_Unit = symbol->GetUnit();
                    symbol->AddHierarchicalReference( instance );
                }
                else
                {
                    for( const SCH_SYMBOL_INSTANCE& instance : symbol->GetInstances() )
                    {
                        SCH_SYMBOL_INSTANCE tmpInstance = instance;
                        symbol->AddHierarchicalReference( tmpInstance );
                    }
                }
            }

            // Ensure all flags (some are set by previous initializations) are reset:
            symbol->ClearFlags();
            return symbol.release();
        }
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
                SCH_PARSE_ERROR( "invalid symbol X1 transform value", aReader, line );

            transform.y1 = -parseInt( aReader, line, &line );

            if( transform.y1 < -1 || transform.y1 > 1 )
                SCH_PARSE_ERROR( "invalid symbol Y1 transform value", aReader, line );

            transform.x2 = parseInt( aReader, line, &line );

            if( transform.x2 < -1 || transform.x2 > 1 )
                SCH_PARSE_ERROR( "invalid symbol X2 transform value", aReader, line );

            transform.y2 = -parseInt( aReader, line, &line );

            if( transform.y2 < -1 || transform.y2 > 1 )
                SCH_PARSE_ERROR( "invalid symbol Y2 transform value", aReader, line );

            symbol->SetTransform( transform );
        }

        line = aReader.ReadLine();
    }

    SCH_PARSE_ERROR( "invalid symbol line", aReader, line );

    return nullptr;  // Prevents compiler warning.  Should never get here.
}


std::shared_ptr<BUS_ALIAS> SCH_IO_KICAD_LEGACY::loadBusAlias( LINE_READER& aReader,
                                                              SCH_SCREEN* aScreen )
{
    // BUS_ALIAS does not take a SCH_SCREEN* in its constructor; create a default instance
    auto busAlias = std::make_shared<BUS_ALIAS>();
    const char* line = aReader.Line();

    wxCHECK( strCompare( "BusAlias", line, &line ), nullptr );

    wxString buf;
    parseUnquotedString( buf, aReader, line, &line );
    busAlias->SetName( buf );

    while( *line != '\0' )
    {
        buf.clear();
        parseUnquotedString( buf, aReader, line, &line, true );

        if( !buf.IsEmpty() )
            busAlias->Members().emplace_back( buf );
    }

    return busAlias;
}


void SCH_IO_KICAD_LEGACY::SaveSchematicFile( const wxString& aFileName, SCH_SHEET* aSheet,
                                             SCHEMATIC*             aSchematic,
                                             const std::map<std::string, UTF8>* aProperties )
{
    wxCHECK_RET( aSheet != nullptr, "NULL SCH_SHEET object." );
    wxCHECK_RET( !aFileName.IsEmpty(), "No schematic file name defined." );

    init( aSchematic, aProperties );

    wxFileName fn = aFileName;

    // File names should be absolute.  Don't assume everything relative to the project path
    // works properly.
    wxASSERT( fn.IsAbsolute() );

    FILE_OUTPUTFORMATTER formatter( fn.GetFullPath() );

    m_out = &formatter;     // no ownership

    Format( aSheet );

    aSheet->GetScreen()->SetFileExists( true );
}


void SCH_IO_KICAD_LEGACY::Format( SCH_SHEET* aSheet )
{
    wxCHECK_RET( aSheet != nullptr, "NULL SCH_SHEET* object." );
    wxCHECK_RET( m_schematic != nullptr, "NULL SCHEMATIC* object." );

    SCH_SCREEN* screen = aSheet->GetScreen();

    wxCHECK( screen, /* void */ );

    // Write the header
    m_out->Print( 0, "%s %s %d\n", "EESchema", SCHEMATIC_HEAD_STRING, EESCHEMA_VERSION );

    // This section is not used, but written for file compatibility
    m_out->Print( 0, "EELAYER %d %d\n", SCH_LAYER_ID_COUNT, 0 );
    m_out->Print( 0, "EELAYER END\n" );

    /* Write page info, ScreenNumber and NumberOfScreen; not very meaningful for
     * SheetNumber and Sheet Count in a complex hierarchy, but useful in
     * simple hierarchy and flat hierarchy.  Used also to search the root
     * sheet ( ScreenNumber = 1 ) within the files
     */
    const TITLE_BLOCK& tb = screen->GetTitleBlock();
    const PAGE_INFO& page = screen->GetPageSettings();

    m_out->Print( 0, "$Descr %s %d %d%s\n", TO_UTF8( page.GetTypeAsString() ),
                  (int)page.GetWidthMils(),
                  (int)page.GetHeightMils(),
                  !page.IsCustom() && page.IsPortrait() ? " portrait" : "" );
    m_out->Print( 0, "encoding utf-8\n" );
    m_out->Print( 0, "Sheet %d %d\n", screen->GetVirtualPageNumber(), screen->GetPageCount() );
    m_out->Print( 0, "Title %s\n",    EscapedUTF8( tb.GetTitle() ).c_str() );
    m_out->Print( 0, "Date %s\n",     EscapedUTF8( tb.GetDate() ).c_str() );
    m_out->Print( 0, "Rev %s\n",      EscapedUTF8( tb.GetRevision() ).c_str() );
    m_out->Print( 0, "Comp %s\n",     EscapedUTF8( tb.GetCompany() ).c_str() );
    m_out->Print( 0, "Comment1 %s\n", EscapedUTF8( tb.GetComment( 0 ) ).c_str() );
    m_out->Print( 0, "Comment2 %s\n", EscapedUTF8( tb.GetComment( 1 ) ).c_str() );
    m_out->Print( 0, "Comment3 %s\n", EscapedUTF8( tb.GetComment( 2 ) ).c_str() );
    m_out->Print( 0, "Comment4 %s\n", EscapedUTF8( tb.GetComment( 3 ) ).c_str() );
    m_out->Print( 0, "Comment5 %s\n", EscapedUTF8( tb.GetComment( 4 ) ).c_str() );
    m_out->Print( 0, "Comment6 %s\n", EscapedUTF8( tb.GetComment( 5 ) ).c_str() );
    m_out->Print( 0, "Comment7 %s\n", EscapedUTF8( tb.GetComment( 6 ) ).c_str() );
    m_out->Print( 0, "Comment8 %s\n", EscapedUTF8( tb.GetComment( 7 ) ).c_str() );
    m_out->Print( 0, "Comment9 %s\n", EscapedUTF8( tb.GetComment( 8 ) ).c_str() );
    m_out->Print( 0, "$EndDescr\n" );

    // Enforce item ordering
    auto cmp = []( const SCH_ITEM* a, const SCH_ITEM* b ) { return *a < *b; };
    std::multiset<SCH_ITEM*, decltype( cmp )> save_map( cmp );

    for( SCH_ITEM* item : screen->Items() )
        save_map.insert( item );


    for( auto& item : save_map )
    {
        switch( item->Type() )
        {
        case SCH_SYMBOL_T:
            saveSymbol( static_cast<SCH_SYMBOL*>( item ) );
            break;
        case SCH_BITMAP_T:
            saveBitmap( static_cast<const SCH_BITMAP&>( *item ) );
            break;
        case SCH_SHEET_T:
            saveSheet( static_cast<SCH_SHEET*>( item ) );
            break;
        case SCH_JUNCTION_T:
            saveJunction( static_cast<SCH_JUNCTION*>( item ) );
            break;
        case SCH_NO_CONNECT_T:
            saveNoConnect( static_cast<SCH_NO_CONNECT*>( item ) );
            break;
        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_BUS_BUS_ENTRY_T:
            saveBusEntry( static_cast<SCH_BUS_ENTRY_BASE*>( item ) );
            break;
        case SCH_LINE_T:
            saveLine( static_cast<SCH_LINE*>( item ) );
            break;
        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
            saveText( static_cast<SCH_TEXT*>( item ) );
            break;
        default:
            wxASSERT( "Unexpected schematic object type in SCH_IO_KICAD_LEGACY::Format()" );
        }
    }

    m_out->Print( 0, "$EndSCHEMATC\n" );
}


void SCH_IO_KICAD_LEGACY::Format( SELECTION* aSelection, OUTPUTFORMATTER* aFormatter )
{
    m_out = aFormatter;

    for( unsigned i = 0; i < aSelection->GetSize(); ++i )
    {
        SCH_ITEM* item = (SCH_ITEM*) aSelection->GetItem( i );

        switch( item->Type() )
        {
        case SCH_SYMBOL_T:
            saveSymbol( static_cast< SCH_SYMBOL* >( item ) );
            break;
        case SCH_BITMAP_T:
            saveBitmap( static_cast< const SCH_BITMAP& >( *item ) );
            break;
        case SCH_SHEET_T:
            saveSheet( static_cast< SCH_SHEET* >( item ) );
            break;
        case SCH_JUNCTION_T:
            saveJunction( static_cast< SCH_JUNCTION* >( item ) );
            break;
        case SCH_NO_CONNECT_T:
            saveNoConnect( static_cast< SCH_NO_CONNECT* >( item ) );
            break;
        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_BUS_BUS_ENTRY_T:
            saveBusEntry( static_cast< SCH_BUS_ENTRY_BASE* >( item ) );
            break;
        case SCH_LINE_T:
            saveLine( static_cast< SCH_LINE* >( item ) );
            break;
        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
            saveText( static_cast< SCH_TEXT* >( item ) );
            break;
        default:
            wxASSERT( "Unexpected schematic object type in SCH_IO_KICAD_LEGACY::Format()" );
        }
    }
}


void SCH_IO_KICAD_LEGACY::saveSymbol( SCH_SYMBOL* aSymbol )
{
    std::string name1;
    std::string name2;

    // This is redundant with the AR entries below, but it makes the files backwards-compatible.
    if( aSymbol->GetInstances().size() > 0 )
    {
        const SCH_SYMBOL_INSTANCE& instance = aSymbol->GetInstances()[0];
        name1 = toUTFTildaText( instance.m_Reference );
    }
    else
    {
        if( aSymbol->GetField( FIELD_T::REFERENCE )->GetText().IsEmpty() )
            name1 = toUTFTildaText( aSymbol->GetPrefix() );
        else
            name1 = toUTFTildaText( aSymbol->GetField( FIELD_T::REFERENCE )->GetText() );
    }

    wxString symbol_name = aSymbol->GetLibId().Format();

    if( symbol_name.size() )
    {
        name2 = toUTFTildaText( symbol_name );
    }
    else
    {
        name2 = "_NONAME_";
    }

    m_out->Print( 0, "$Comp\n" );
    m_out->Print( 0, "L %s %s\n", name2.c_str(), name1.c_str() );

    // Generate unit number, conversion and timestamp
    m_out->Print( 0, "U %d %d %8.8X\n",
                  aSymbol->GetUnit(),
                  aSymbol->GetBodyStyle(),
                  aSymbol->m_Uuid.AsLegacyTimestamp() );

    // Save the position
    m_out->Print( 0, "P %d %d\n",
                  schIUScale.IUToMils( aSymbol->GetPosition().x ),
                  schIUScale.IUToMils( aSymbol->GetPosition().y ) );

    /* If this is a complex hierarchy; save hierarchical references.
     * but for simple hierarchies it is not necessary.
     * the reference inf is already saved
     * this is useful for old Eeschema version compatibility
     */
    if( aSymbol->GetInstances().size() > 1 )
    {
        for( const SCH_SYMBOL_INSTANCE& instance : aSymbol->GetInstances() )
        {
            /*format:
             * AR Path="/140/2" Ref="C99"   Part="1"
             * where 140 is the uid of the containing sheet and 2 is the timestamp of this symbol.
             * (timestamps are actually 8 hex chars)
             * Ref is the conventional symbol reference designator for this 'path'
             * Part is the conventional symbol unit selection for this 'path'
             */
            wxString path = "/";

            // Skip root sheet
            for( int i = 1; i < (int) instance.m_Path.size(); ++i )
                path += instance.m_Path[i].AsLegacyTimestampString() + "/";

            m_out->Print( 0, "AR Path=\"%s\" Ref=\"%s\"  Part=\"%d\" \n",
                          TO_UTF8( path + aSymbol->m_Uuid.AsLegacyTimestampString() ),
                          TO_UTF8( instance.m_Reference ),
                          instance.m_Unit );
        }
    }

    // NB: FieldIDs in legacy libraries must be consecutive, and include user fields
    int legacy_field_id = 0;

    for( SCH_FIELD& field : aSymbol->GetFields() )
        saveField( &field, legacy_field_id++ );

    // Unit number, position, box ( old standard )
    m_out->Print( 0, "\t%-4d %-4d %-4d\n", aSymbol->GetUnit(),
                  schIUScale.IUToMils( aSymbol->GetPosition().x ),
                  schIUScale.IUToMils( aSymbol->GetPosition().y ) );

    TRANSFORM transform = aSymbol->GetTransform();

    m_out->Print( 0, "\t%-4d %-4d %-4d %-4d\n",
                  transform.x1, transform.y1, transform.x2, transform.y2 );
    m_out->Print( 0, "$EndComp\n" );
}


void SCH_IO_KICAD_LEGACY::saveField( SCH_FIELD* aField, int aLegacyId )
{
    char hjustify = 'C';

    if( aField->GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
        hjustify = 'L';
    else if( aField->GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
        hjustify = 'R';

    char vjustify = 'C';

    if( aField->GetVertJustify() == GR_TEXT_V_ALIGN_BOTTOM )
        vjustify = 'B';
    else if( aField->GetVertJustify() == GR_TEXT_V_ALIGN_TOP )
        vjustify = 'T';

    m_out->Print( 0, "F %d %s %c %-3d %-3d %-3d %4.4X %c %c%c%c",
                  aLegacyId,
                  EscapedUTF8( aField->GetText() ).c_str(),     // wraps in quotes too
                  aField->GetTextAngle().IsHorizontal() ? 'H' : 'V',
                  schIUScale.IUToMils( aField->GetLibPosition().x ),
                  schIUScale.IUToMils( aField->GetLibPosition().y ),
                  schIUScale.IUToMils( aField->GetTextWidth() ),
                  !aField->IsVisible(),
                  hjustify, vjustify,
                  aField->IsItalic() ? 'I' : 'N',
                  aField->IsBold() ? 'B' : 'N' );

    // Save field name, if the name is user definable
    if( !aField->IsMandatory() )
        m_out->Print( 0, " %s", EscapedUTF8( aField->GetName() ).c_str() );

    m_out->Print( 0, "\n" );
}


void SCH_IO_KICAD_LEGACY::saveBitmap( const SCH_BITMAP& aBitmap )
{
    const REFERENCE_IMAGE& refImage = aBitmap.GetReferenceImage();

    const wxImage* image = refImage.GetImage().GetImageData();

    wxCHECK_RET( image != nullptr, "wxImage* is NULL" );

    m_out->Print( 0, "$Bitmap\n" );
    m_out->Print( 0, "Pos %-4d %-4d\n",
                  schIUScale.IUToMils( aBitmap.GetPosition().x ),
                  schIUScale.IUToMils( aBitmap.GetPosition().y ) );
    m_out->Print( "%s", fmt::format("Scale {:g}\n", refImage.GetImageScale()).c_str() );
    m_out->Print( 0, "Data\n" );

    wxMemoryOutputStream stream;

    image->SaveFile( stream, wxBITMAP_TYPE_PNG );

    // Write binary data in hexadecimal form (ASCII)
    wxStreamBuffer* buffer = stream.GetOutputStreamBuffer();
    char*           begin  = (char*) buffer->GetBufferStart();

    for( int ii = 0; begin < buffer->GetBufferEnd(); begin++, ii++ )
    {
        if( ii >= 32 )
        {
            ii = 0;

            m_out->Print( 0, "\n" );
        }

        m_out->Print( 0, "%2.2X ", *begin & 0xFF );
    }

    m_out->Print( 0, "\nEndData\n" );
    m_out->Print( 0, "$EndBitmap\n" );
}


void SCH_IO_KICAD_LEGACY::saveSheet( SCH_SHEET* aSheet )
{
    wxCHECK_RET( aSheet != nullptr, "SCH_SHEET* is NULL" );

    m_out->Print( 0, "$Sheet\n" );
    m_out->Print( 0, "S %-4d %-4d %-4d %-4d\n",
                  schIUScale.IUToMils( aSheet->GetPosition().x ),
                  schIUScale.IUToMils( aSheet->GetPosition().y ),
                  schIUScale.IUToMils( aSheet->GetSize().x ),
                  schIUScale.IUToMils( aSheet->GetSize().y ) );

    m_out->Print( 0, "U %8.8X\n", aSheet->m_Uuid.AsLegacyTimestamp() );

    SCH_FIELD* sheetName = aSheet->GetField( FIELD_T::SHEET_NAME );
    SCH_FIELD* fileName = aSheet->GetField( FIELD_T::SHEET_FILENAME );

    if( !sheetName->GetText().IsEmpty() )
    {
        m_out->Print( 0, "F0 %s %d\n",
                      EscapedUTF8( sheetName->GetText() ).c_str(),
                      schIUScale.IUToMils( sheetName->GetTextSize().x ) );
    }

    if( !fileName->GetText().IsEmpty() )
    {
        m_out->Print( 0, "F1 %s %d\n",
                      EscapedUTF8( fileName->GetText() ).c_str(),
                      schIUScale.IUToMils( fileName->GetTextSize().x ) );
    }

    for( const SCH_SHEET_PIN* pin : aSheet->GetPins() )
    {
        int type, side;

        if( pin->GetText().IsEmpty() )
            break;

        switch( pin->GetSide() )
        {
        default:
        case SHEET_SIDE::LEFT:   side = 'L'; break;
        case SHEET_SIDE::RIGHT:  side = 'R'; break;
        case SHEET_SIDE::TOP:    side = 'T'; break;
        case SHEET_SIDE::BOTTOM: side = 'B'; break;
        }

        switch( pin->GetShape() )
        {
        default:
        case LABEL_FLAG_SHAPE::L_UNSPECIFIED: type = 'U'; break;
        case LABEL_FLAG_SHAPE::L_INPUT:       type = 'I'; break;
        case LABEL_FLAG_SHAPE::L_OUTPUT:      type = 'O'; break;
        case LABEL_FLAG_SHAPE::L_BIDI:        type = 'B'; break;
        case LABEL_FLAG_SHAPE::L_TRISTATE:    type = 'T'; break;
        }

        m_out->Print( 0, "F%d %s %c %c %-3d %-3d %-3d\n",
                      pin->GetNumber(),
                      EscapedUTF8( pin->GetText() ).c_str(),     // supplies wrapping quotes
                      type, side, schIUScale.IUToMils( pin->GetPosition().x ),
                      schIUScale.IUToMils( pin->GetPosition().y ),
                      schIUScale.IUToMils( pin->GetTextWidth() ) );
    }

    m_out->Print( 0, "$EndSheet\n" );
}


void SCH_IO_KICAD_LEGACY::saveJunction( SCH_JUNCTION* aJunction )
{
    wxCHECK_RET( aJunction != nullptr, "SCH_JUNCTION* is NULL" );

    m_out->Print( 0, "Connection ~ %-4d %-4d\n",
                  schIUScale.IUToMils( aJunction->GetPosition().x ),
                  schIUScale.IUToMils( aJunction->GetPosition().y ) );
}


void SCH_IO_KICAD_LEGACY::saveNoConnect( SCH_NO_CONNECT* aNoConnect )
{
    wxCHECK_RET( aNoConnect != nullptr, "SCH_NOCONNECT* is NULL" );

    m_out->Print( 0, "NoConn ~ %-4d %-4d\n",
                  schIUScale.IUToMils( aNoConnect->GetPosition().x ),
                  schIUScale.IUToMils( aNoConnect->GetPosition().y ) );
}


void SCH_IO_KICAD_LEGACY::saveBusEntry( SCH_BUS_ENTRY_BASE* aBusEntry )
{
    wxCHECK_RET( aBusEntry != nullptr, "SCH_BUS_ENTRY_BASE* is NULL" );

    if( aBusEntry->GetLayer() == LAYER_WIRE )
    {
        m_out->Print( 0, "Entry Wire Line\n\t%-4d %-4d %-4d %-4d\n",
                      schIUScale.IUToMils( aBusEntry->GetPosition().x ),
                      schIUScale.IUToMils( aBusEntry->GetPosition().y ),
                      schIUScale.IUToMils( aBusEntry->GetEnd().x ),
                      schIUScale.IUToMils( aBusEntry->GetEnd().y ) );
    }
    else
    {
        m_out->Print( 0, "Entry Bus Bus\n\t%-4d %-4d %-4d %-4d\n",
                      schIUScale.IUToMils( aBusEntry->GetPosition().x ),
                      schIUScale.IUToMils( aBusEntry->GetPosition().y ),
                      schIUScale.IUToMils( aBusEntry->GetEnd().x ),
                      schIUScale.IUToMils( aBusEntry->GetEnd().y ) );
    }
}


void SCH_IO_KICAD_LEGACY::saveLine( SCH_LINE* aLine )
{
    wxCHECK_RET( aLine != nullptr, "SCH_LINE* is NULL" );

    const char* layer = "Notes";
    const char* width = "Line";

    if( aLine->GetLayer() == LAYER_WIRE )
        layer = "Wire";
    else if( aLine->GetLayer() == LAYER_BUS )
        layer = "Bus";

    m_out->Print( 0, "Wire %s %s", layer, width );

    // Write line style (width, type, color) only for non default values
    if( aLine->IsGraphicLine() )
    {
        const STROKE_PARAMS& stroke = aLine->GetStroke();

        if( stroke.GetWidth() != 0 )
            m_out->Print( 0, " %s %d", T_WIDTH, schIUScale.IUToMils( stroke.GetWidth() ) );

        m_out->Print( 0, " %s %s",
                      T_STYLE,
                      TO_UTF8( STROKE_PARAMS::GetLineStyleToken( stroke.GetLineStyle() ) ) );

        if( stroke.GetColor() != COLOR4D::UNSPECIFIED )
            m_out->Print( 0, " %s", TO_UTF8( stroke.GetColor().ToCSSString() ) );
    }

    m_out->Print( 0, "\n" );

    m_out->Print( 0, "\t%-4d %-4d %-4d %-4d",
                  schIUScale.IUToMils( aLine->GetStartPoint().x ),
                  schIUScale.IUToMils( aLine->GetStartPoint().y ),
                  schIUScale.IUToMils( aLine->GetEndPoint().x ),
                  schIUScale.IUToMils( aLine->GetEndPoint().y ) );

    m_out->Print( 0, "\n");
}


void SCH_IO_KICAD_LEGACY::saveText( SCH_TEXT* aText )
{
    wxCHECK_RET( aText != nullptr, "SCH_TEXT* is NULL" );

    const char* italics  = "~";
    const char* textType = "Notes";

    if( aText->IsItalic() )
        italics = "Italic";

    wxString text = aText->GetText();

    SCH_LAYER_ID layer = aText->GetLayer();

    if( layer == LAYER_NOTES || layer == LAYER_LOCLABEL )
    {
        if( layer == LAYER_NOTES )
        {
            // For compatibility reasons, the text must be saved in only one text line
            // so replace all EOLs with \\n
            text.Replace( wxT( "\n" ), wxT( "\\n" ) );

            // Here we should have no CR or LF character in line
            // This is not always the case if a multiline text was copied (using a copy/paste
            // function) from a text that uses E.O.L characters that differs from the current
            // EOL format.  This is mainly the case under Linux using LF symbol when copying
            // a text from Windows (using CRLF symbol) so we must just remove the extra CR left
            // (or LF left under MacOSX)
            for( unsigned ii = 0; ii < text.Len();  )
            {
                if( text[ii] == 0x0A || text[ii] == 0x0D )
                    text.erase( ii, 1 );
                else
                    ii++;
            }
        }
        else
        {
            textType = "Label";
        }

        int spinStyle = 0;

        // Local labels must have their spin style inverted for left and right
        if( SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( aText ) )
        {
            spinStyle = static_cast<int>( label->GetSpinStyle() );

            if( spinStyle == 0 )
                spinStyle = 2;
            else if( spinStyle == 2 )
                spinStyle = 0;
        }

        m_out->Print( 0, "Text %s %-4d %-4d %-4d %-4d %s %d\n%s\n", textType,
                      schIUScale.IUToMils( aText->GetPosition().x ),
                      schIUScale.IUToMils( aText->GetPosition().y ),
                      spinStyle,
                      schIUScale.IUToMils( aText->GetTextWidth() ),
                      italics, schIUScale.IUToMils( aText->GetTextThickness() ), TO_UTF8( text ) );
    }
    else if( layer == LAYER_GLOBLABEL || layer == LAYER_HIERLABEL )
    {
        textType = ( layer == LAYER_GLOBLABEL ) ? "GLabel" : "HLabel";

        SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( aText );
        auto            shapeLabelIt = sheetLabelNames.find( label->GetShape() );
        wxCHECK_RET( shapeLabelIt != sheetLabelNames.end(), "Shape not found in names list" );

        m_out->Print( 0, "Text %s %-4d %-4d %-4d %-4d %s %s %d\n%s\n", textType,
                      schIUScale.IUToMils( aText->GetPosition().x ),
                      schIUScale.IUToMils( aText->GetPosition().y ),
                      static_cast<int>( label->GetSpinStyle() ),
                      schIUScale.IUToMils( aText->GetTextWidth() ),
                      shapeLabelIt->second,
                      italics,
                      schIUScale.IUToMils( aText->GetTextThickness() ), TO_UTF8( text ) );
    }
}


void SCH_IO_KICAD_LEGACY::saveBusAlias( std::shared_ptr<BUS_ALIAS> aAlias )
{
    wxCHECK_RET( aAlias != nullptr, "BUS_ALIAS* is NULL" );

    wxString members = boost::algorithm::join( aAlias->Members(), " " );

    m_out->Print( 0, "BusAlias %s %s\n",
                  TO_UTF8( aAlias->GetName() ), TO_UTF8( members ) );
}


void SCH_IO_KICAD_LEGACY::cacheLib( const wxString& aLibraryFileName,
                                    const std::map<std::string, UTF8>* aProperties )
{
    if( !m_cache || !m_cache->IsFile( aLibraryFileName ) || m_cache->IsFileChanged() )
    {
        // a spectacular episode in memory management:
        delete m_cache;
        m_cache = new SCH_IO_KICAD_LEGACY_LIB_CACHE( aLibraryFileName );

        if( !isBuffering( aProperties ) )
            m_cache->Load();
    }
}


bool SCH_IO_KICAD_LEGACY::writeDocFile( const std::map<std::string, UTF8>* aProperties )
{
    std::string propName( SCH_IO_KICAD_LEGACY::PropNoDocFile );

    if( aProperties && aProperties->find( propName ) != aProperties->end() )
        return false;

    return true;
}


bool SCH_IO_KICAD_LEGACY::isBuffering( const std::map<std::string, UTF8>* aProperties )
{
    return ( aProperties && aProperties->contains( SCH_IO_KICAD_LEGACY::PropBuffering ) );
}


int SCH_IO_KICAD_LEGACY::GetModifyHash() const
{
    if( m_cache )
        return m_cache->GetModifyHash();

    // If the cache hasn't been loaded, it hasn't been modified.
    return 0;
}


void SCH_IO_KICAD_LEGACY::EnumerateSymbolLib( wxArrayString&    aSymbolNameList,
                                              const wxString&   aLibraryPath,
                                              const std::map<std::string, UTF8>* aProperties )
{
    bool powerSymbolsOnly = ( aProperties && aProperties->contains( SYMBOL_LIBRARY_ADAPTER::PropPowerSymsOnly ) );

    cacheLib( aLibraryPath, aProperties  );

    const LIB_SYMBOL_MAP& symbols = m_cache->m_symbols;

    for( LIB_SYMBOL_MAP::const_iterator it = symbols.begin();  it != symbols.end();  ++it )
    {
        if( !powerSymbolsOnly || it->second->IsGlobalPower() )
            aSymbolNameList.Add( it->first );
    }
}


void SCH_IO_KICAD_LEGACY::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                              const wxString& aLibraryPath,
                                              const std::map<std::string, UTF8>* aProperties )
{
    bool powerSymbolsOnly = ( aProperties && aProperties->contains( SYMBOL_LIBRARY_ADAPTER::PropPowerSymsOnly ) );

    cacheLib( aLibraryPath, aProperties );

    const LIB_SYMBOL_MAP& symbols = m_cache->m_symbols;

    for( LIB_SYMBOL_MAP::const_iterator it = symbols.begin();  it != symbols.end();  ++it )
    {
        if( !powerSymbolsOnly || it->second->IsGlobalPower() )
            aSymbolList.push_back( it->second );
    }
}


LIB_SYMBOL* SCH_IO_KICAD_LEGACY::LoadSymbol( const wxString& aLibraryPath,
                                             const wxString& aSymbolName,
                                             const std::map<std::string, UTF8>* aProperties )
{
    cacheLib( aLibraryPath, aProperties );

    LIB_SYMBOL_MAP::const_iterator it = m_cache->m_symbols.find( aSymbolName );

    if( it == m_cache->m_symbols.end() )
        return nullptr;

    return it->second;
}


void SCH_IO_KICAD_LEGACY::SaveSymbol( const wxString& aLibraryPath, const LIB_SYMBOL* aSymbol,
                                      const std::map<std::string, UTF8>* aProperties )
{
    cacheLib( aLibraryPath, aProperties );

    m_cache->AddSymbol( aSymbol );

    if( !isBuffering( aProperties ) )
        m_cache->Save( writeDocFile( aProperties ) );
}


void SCH_IO_KICAD_LEGACY::DeleteSymbol( const wxString& aLibraryPath, const wxString& aSymbolName,
                                        const std::map<std::string, UTF8>* aProperties )
{
    cacheLib( aLibraryPath, aProperties );

    m_cache->DeleteSymbol( aSymbolName );

    if( !isBuffering( aProperties ) )
        m_cache->Save( writeDocFile( aProperties ) );
}


void SCH_IO_KICAD_LEGACY::CreateLibrary( const wxString& aLibraryPath,
                                         const std::map<std::string, UTF8>* aProperties )
{
    if( wxFileExists( aLibraryPath ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "Symbol library '%s' already exists." ),
                                          aLibraryPath.GetData() ) );
    }

    delete m_cache;
    m_cache = new SCH_IO_KICAD_LEGACY_LIB_CACHE( aLibraryPath );
    m_cache->SetModified();
    m_cache->Save( writeDocFile( aProperties ) );
    m_cache->Load();    // update m_writable and m_timestamp
}


bool SCH_IO_KICAD_LEGACY::DeleteLibrary( const wxString& aLibraryPath,
                                         const std::map<std::string, UTF8>* aProperties )
{
    wxFileName fn = aLibraryPath;

    if( !fn.FileExists() )
        return false;

    // Some of the more elaborate wxRemoveFile() crap puts up its own wxLog dialog
    // we don't want that.  we want bare metal portability with no UI here.
    if( wxRemove( aLibraryPath ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "Symbol library '%s' cannot be deleted." ),
                                          aLibraryPath.GetData() ) );
    }

    if( m_cache && m_cache->IsFile( aLibraryPath ) )
    {
        delete m_cache;
        m_cache = nullptr;
    }

    return true;
}


void SCH_IO_KICAD_LEGACY::SaveLibrary( const wxString& aLibraryPath,
                                       const std::map<std::string, UTF8>* aProperties )
{
    if( !m_cache )
        m_cache = new SCH_IO_KICAD_LEGACY_LIB_CACHE( aLibraryPath );

    wxString oldFileName = m_cache->GetFileName();

    if( !m_cache->IsFile( aLibraryPath ) )
    {
        m_cache->SetFileName( aLibraryPath );
    }

    // This is a forced save.
    m_cache->SetModified();
    m_cache->Save( writeDocFile( aProperties ) );
    m_cache->SetFileName( oldFileName );
}


bool SCH_IO_KICAD_LEGACY::CanReadSchematicFile( const wxString& aFileName ) const
{
    if( !SCH_IO::CanReadSchematicFile( aFileName ) )
        return false;

    return IO_UTILS::fileStartsWithPrefix( aFileName, wxT( "EESchema" ), true );
}


bool SCH_IO_KICAD_LEGACY::CanReadLibrary( const wxString& aFileName ) const
{
    if( !SCH_IO::CanReadLibrary( aFileName ) )
        return false;

    return IO_UTILS::fileStartsWithPrefix( aFileName, wxT( "EESchema" ), true );
}


bool SCH_IO_KICAD_LEGACY::IsLibraryWritable( const wxString& aLibraryPath )
{
    // Writing legacy symbol libraries is deprecated.
    return false;
}


LIB_SYMBOL* SCH_IO_KICAD_LEGACY::ParsePart( LINE_READER& reader, int aMajorVersion,
                                          int aMinorVersion )
{
    return SCH_IO_KICAD_LEGACY_LIB_CACHE::LoadPart( reader, aMajorVersion, aMinorVersion );
}


void SCH_IO_KICAD_LEGACY::FormatPart( LIB_SYMBOL* symbol, OUTPUTFORMATTER & formatter )
{
    SCH_IO_KICAD_LEGACY_LIB_CACHE::SaveSymbol( symbol, formatter );
}



const char* SCH_IO_KICAD_LEGACY::PropBuffering = "buffering";
const char* SCH_IO_KICAD_LEGACY::PropNoDocFile = "no_doc_file";
