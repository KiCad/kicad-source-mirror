/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <set>

#include <wx/mstream.h>
#include <wx/filename.h>
#include <wx/textfile.h>
#include <wx/tokenzr.h>

#include <pgm_base.h>
#include <gr_text.h>
#include <kiway.h>
#include <kicad_string.h>
#include <locale_io.h>
#include <richio.h>
#include <core/typeinfo.h>
#include <properties.h>
#include <trace_helpers.h>

#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_symbol.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_no_connect.h>
#include <sch_text.h>
#include <sch_sheet.h>
#include <sch_bitmap.h>
#include <bus_alias.h>
#include <sch_plugins/legacy/sch_legacy_plugin.h>
#include <template_fieldnames.h>
#include <sch_screen.h>
#include <schematic.h>
#include <class_library.h>
#include <lib_arc.h>
#include <lib_bezier.h>
#include <lib_circle.h>
#include <lib_field.h>
#include <lib_pin.h>
#include <lib_polyline.h>
#include <lib_rectangle.h>
#include <lib_text.h>
#include <eeschema_id.h>       // for MAX_UNIT_COUNT_PER_PACKAGE definition
#include <symbol_lib_table.h>  // for PropPowerSymsOnly definintion.
#include <tool/selection.h>
#include <default_values.h>    // For some default values


#define Mils2Iu( x ) Mils2iu( x )


// Must be the first line of part library document (.dcm) files.
#define DOCFILE_IDENT     "EESchema-DOCLIB  Version 2.0"

#define SCH_PARSE_ERROR( text, reader, pos )                         \
    THROW_PARSE_ERROR( text, reader.GetSource(), reader.Line(),      \
                       reader.LineNumber(), pos - reader.Line() )


// Token delimiters.
const char* delims = " \t\r\n";

// Tokens to read/save graphic lines style
#define T_STYLE "style"
#define T_COLOR "rgb"          // cannot be modifed (used by wxWidgets)
#define T_COLORA "rgba"        // cannot be modifed (used by wxWidgets)
#define T_WIDTH "width"


static bool is_eol( char c )
{
    //        The default file eol character used internally by KiCad.
    //        |
    //        |            Possible eol if someone edited the file by hand on certain platforms.
    //        |            |
    //        |            |           May have gone past eol with strtok().
    //        |            |           |
    if( c == '\n' || c == '\r' || c == 0 )
        return true;

    return false;
}


/**
 * Compare \a aString to the string starting at \a aLine and advances the character point to
 * the end of \a String and returns the new pointer position in \a aOutput if it is not NULL.
 *
 * @param aString - A pointer to the string to compare.
 * @param aLine - A pointer to string to begin the comparison.
 * @param aOutput - A pointer to a string pointer to the end of the comparison if not NULL.
 * @return true if \a aString was found starting at \a aLine.  Otherwise false.
 */
static bool strCompare( const char* aString, const char* aLine, const char** aOutput = NULL )
{
    size_t len = strlen( aString );
    bool retv = ( strncasecmp( aLine, aString, len ) == 0 ) &&
                ( isspace( aLine[ len ] ) || aLine[ len ] == 0 );

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
 * Parse an ASCII integer string with possible leading whitespace into
 * an integer and updates the pointer at \a aOutput if it is not NULL, just
 * like "man strtol()".
 *
 * @param aReader - The line reader used to generate exception throw information.
 * @param aLine - A pointer the current position in a string.
 * @param aOutput - The pointer to a string pointer to copy the string pointer position when
 *                  the parsing is complete.
 * @return A valid integer value.
 * @throw An #IO_ERROR on an unexpected end of line.
 * @throw A #PARSE_ERROR if the parsed token is not a valid integer.
 */
static int parseInt( LINE_READER& aReader, const char* aLine, const char** aOutput = NULL )
{
    if( !*aLine )
        SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, aLine );

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
 * Parse an ASCII hex integer string with possible leading whitespace into
 * a long integer and updates the pointer at \a aOutput if it is not NULL, just
 * like "man strtoll".
 *
 * @param aReader - The line reader used to generate exception throw information.
 * @param aLine - A pointer the current position in a string.
 * @param aOutput - The pointer to a string pointer to copy the string pointer position when
 *                  the parsing is complete.
 * @return A valid uint32_t value.
 * @throw IO_ERROR on an unexpected end of line.
 * @throw PARSE_ERROR if the parsed token is not a valid integer.
 */
static uint32_t parseHex( LINE_READER& aReader, const char* aLine, const char** aOutput = NULL )
{
    if( !*aLine )
        SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, aLine );

    // Due to some issues between some files created by a 64 bits version and those
    // created by a 32 bits version, we use here a temporary at least 64 bits storage:
    unsigned long long retv;

    // Clear errno before calling strtoull() in case some other crt call set it.
    errno = 0;
    retv = strtoull( aLine, (char**) aOutput, 16 );

    // Make sure no error occurred when calling strtoull().
    if( errno == ERANGE )
        SCH_PARSE_ERROR( "invalid hexadecimal number", aReader, aLine );

    // Strip off whitespace before the next token.
    if( aOutput )
    {
        const char* next = *aOutput;

        while( *next && isspace( *next ) )
            next++;

        *aOutput = next;
    }

    return (uint32_t)retv;
}


/**
 * Parses an ASCII point string with possible leading whitespace into a double precision
 * floating point number and  updates the pointer at \a aOutput if it is not NULL, just
 * like "man strtod".
 *
 * @param aReader - The line reader used to generate exception throw information.
 * @param aLine - A pointer the current position in a string.
 * @param aOutput - The pointer to a string pointer to copy the string pointer position when
 *                  the parsing is complete.
 * @return A valid double value.
 * @throw IO_ERROR on an unexpected end of line.
 * @throw PARSE_ERROR if the parsed token is not a valid integer.
 */
static double parseDouble( LINE_READER& aReader, const char* aLine,
                           const char** aOutput = NULL )
{
    if( !*aLine )
        SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, aLine );

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
 * Parse a single ASCII character and updates the pointer at \a aOutput if it is not NULL.
 *
 * @param aReader - The line reader used to generate exception throw information.
 * @param aCurrentToken - A pointer the current position in a string.
 * @param aNextToken - The pointer to a string pointer to copy the string pointer position when
 *                     the parsing is complete.
 * @return A valid ASCII character.
 * @throw IO_ERROR on an unexpected end of line.
 * @throw PARSE_ERROR if the parsed token is not a a single character token.
 */
static char parseChar( LINE_READER& aReader, const char* aCurrentToken,
                       const char** aNextToken = NULL )
{
    while( *aCurrentToken && isspace( *aCurrentToken ) )
        aCurrentToken++;

    if( !*aCurrentToken )
        SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, aCurrentToken );

    if( !isspace( *( aCurrentToken + 1 ) ) )
        SCH_PARSE_ERROR( "expected single character token", aReader, aCurrentToken );

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
 * Parse an unquoted utf8 string and updates the pointer at \a aOutput if it is not NULL.
 *
 * The parsed string must be a continuous string with no white space.
 *
 * @param aString - A reference to the parsed string.
 * @param aReader - The line reader used to generate exception throw information.
 * @param aCurrentToken - A pointer the current position in a string.
 * @param aNextToken - The pointer to a string pointer to copy the string pointer position when
 *                     the parsing is complete.
 * @param aCanBeEmpty - True if the parsed string is optional.  False if it is mandatory.
 * @throw IO_ERROR on an unexpected end of line.
 * @throw PARSE_ERROR if the \a aCanBeEmpty is false and no string was parsed.
 */
static void parseUnquotedString( wxString& aString, LINE_READER& aReader,
                                 const char* aCurrentToken, const char** aNextToken = NULL,
                                 bool aCanBeEmpty = false )
{
    if( !*aCurrentToken )
    {
        if( aCanBeEmpty )
            return;
        else
            SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, aCurrentToken );
    }

    const char* tmp = aCurrentToken;

    while( *tmp && isspace( *tmp ) )
        tmp++;

    if( !*tmp )
    {
        if( aCanBeEmpty )
            return;
        else
            SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, aCurrentToken );
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
 * Parse an quoted ASCII utf8 and updates the pointer at \a aOutput if it is not NULL.
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
 * @throw IO_ERROR on an unexpected end of line.
 * @throw PARSE_ERROR if the \a aCanBeEmpty is false and no string was parsed.
 */
static void parseQuotedString( wxString& aString, LINE_READER& aReader,
                               const char* aCurrentToken, const char** aNextToken = NULL,
                               bool aCanBeEmpty = false )
{
    if( !*aCurrentToken )
    {
        if( aCanBeEmpty )
            return;
        else
            SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, aCurrentToken );
    }

    const char* tmp = aCurrentToken;

    while( *tmp && isspace( *tmp ) )
        tmp++;

    if( !*tmp )
    {
        if( aCanBeEmpty )
            return;
        else
            SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, aCurrentToken );
    }

    // Verify opening quote.
    if( *tmp != '"' )
        SCH_PARSE_ERROR( "expecting opening quote", aReader, aCurrentToken );

    tmp++;

    std::string utf8;     // utf8 without escapes and quotes.

    // Fetch everything up to closing quote.
    while( *tmp )
    {
        if( *tmp == '\\' )
        {
            tmp++;

            if( !*tmp )
                SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, aCurrentToken );

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
        SCH_PARSE_ERROR( "expected quoted string", aReader, aCurrentToken );

    if( *tmp && *tmp != '"' )
        SCH_PARSE_ERROR( "no closing quote for string found", aReader, tmp );

    // Move past the closing quote.
    tmp++;

    if( aNextToken )
    {
        const char* next = tmp;

        while( *next == ' ' )
            next++;

        *aNextToken = next;
    }
}


/**
 * A cache assistant for the part library portion of the #SCH_PLUGIN API, and only for the
 * #SCH_LEGACY_PLUGIN, so therefore is private to this implementation file, i.e. not placed
 * into a header.
 */
class SCH_LEGACY_PLUGIN_CACHE
{
    static int      m_modHash;      // Keep track of the modification status of the library.

    wxString        m_fileName;     // Absolute path and file name.
    wxFileName      m_libFileName;  // Absolute path and file name is required here.
    wxDateTime      m_fileModTime;
    LIB_PART_MAP    m_symbols;      // Map of names of #LIB_PART pointers.
    bool            m_isWritable;
    bool            m_isModified;
    int             m_versionMajor;
    int             m_versionMinor;
    SCH_LIB_TYPE    m_libType;      // Is this cache a component or symbol library.

    void                  loadHeader( FILE_LINE_READER& aReader );
    static void           loadAliases( std::unique_ptr<LIB_PART>& aPart, LINE_READER& aReader,
                                       LIB_PART_MAP* aMap = nullptr );
    static void           loadField( std::unique_ptr<LIB_PART>& aPart, LINE_READER& aReader );
    static void           loadDrawEntries( std::unique_ptr<LIB_PART>& aPart, LINE_READER& aReader,
                                           int aMajorVersion, int aMinorVersion );
    static void           loadFootprintFilters( std::unique_ptr<LIB_PART>& aPart,
                                                LINE_READER& aReader );
    void                  loadDocs();
    static LIB_ARC*       loadArc( std::unique_ptr<LIB_PART>& aPart, LINE_READER& aReader );
    static LIB_CIRCLE*    loadCircle( std::unique_ptr<LIB_PART>& aPart, LINE_READER& aReader );
    static LIB_TEXT*      loadText( std::unique_ptr<LIB_PART>& aPart, LINE_READER& aReader,
                                    int aMajorVersion, int aMinorVersion );
    static LIB_RECTANGLE* loadRectangle( std::unique_ptr<LIB_PART>& aPart,
                                         LINE_READER& aReader );
    static LIB_PIN*       loadPin( std::unique_ptr<LIB_PART>& aPart, LINE_READER& aReader );
    static LIB_POLYLINE*  loadPolyLine( std::unique_ptr<LIB_PART>& aPart, LINE_READER& aReader );
    static LIB_BEZIER*    loadBezier( std::unique_ptr<LIB_PART>& aPart, LINE_READER& aReader );

    static FILL_TYPE      parseFillMode( LINE_READER& aReader, const char* aLine,
                                         const char** aOutput );

    friend SCH_LEGACY_PLUGIN;

public:
    SCH_LEGACY_PLUGIN_CACHE( const wxString& aLibraryPath );
    ~SCH_LEGACY_PLUGIN_CACHE();

    int GetModifyHash() const { return m_modHash; }

    // Most all functions in this class throw IO_ERROR exceptions.  There are no
    // error codes nor user interface calls from here, nor in any SCH_PLUGIN objects.
    // Catch these exceptions higher up please.

    void Load();

    // If m_libFileName is a symlink follow it to the real source file
    wxFileName GetRealFile() const;

    wxDateTime GetLibModificationTime();

    bool IsFile( const wxString& aFullPathAndFileName ) const;

    bool IsFileChanged() const;

    void SetModified( bool aModified = true ) { m_isModified = aModified; }

    wxString GetLogicalName() const { return m_libFileName.GetName(); }

    void SetFileName( const wxString& aFileName ) { m_libFileName = aFileName; }

    wxString GetFileName() const { return m_libFileName.GetFullPath(); }

    static LIB_PART* LoadPart( LINE_READER& aReader, int aMajorVersion, int aMinorVersion,
                               LIB_PART_MAP* aMap = nullptr );
};


SCH_LEGACY_PLUGIN::SCH_LEGACY_PLUGIN()
{
    init( NULL );
}


SCH_LEGACY_PLUGIN::~SCH_LEGACY_PLUGIN()
{
    delete m_cache;
}


void SCH_LEGACY_PLUGIN::init( SCHEMATIC* aSchematic, const PROPERTIES* aProperties )
{
    m_version   = 0;
    m_rootSheet = nullptr;
    m_props     = aProperties;
    m_schematic = aSchematic;
    m_cache     = nullptr;
    m_out       = nullptr;
}


SCH_SHEET* SCH_LEGACY_PLUGIN::Load( const wxString& aFileName, SCHEMATIC* aSchematic,
                                    SCH_SHEET* aAppendToMe, const PROPERTIES* aProperties )
{
    wxASSERT( !aFileName || aSchematic != NULL );

    LOCALE_IO   toggle;     // toggles on, then off, the C locale.
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
            m_path = aSchematic->Prj().GetProjectPath();

        wxLogTrace( traceSchLegacyPlugin, "m_Normalized append path \"%s\".", m_path );
    }
    else
    {
        m_path = aSchematic->Prj().GetProjectPath();
    }

    m_currentPath.push( m_path );
    init( aSchematic, aProperties );

    if( aAppendToMe == NULL )
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
        wxCHECK_MSG( aSchematic->IsValid(), nullptr, "Can't append to a schematic with no root!" );
        m_rootSheet = &aSchematic->Root();
        sheet = aAppendToMe;
        loadHierarchy( sheet );
    }

    wxASSERT( m_currentPath.size() == 1 );  // only the project path should remain

    return sheet;
}


// Everything below this comment is recursive.  Modify with care.

void SCH_LEGACY_PLUGIN::loadHierarchy( SCH_SHEET* aSheet )
{
    SCH_SCREEN* screen = NULL;

    if( !aSheet->GetScreen() )
    {
        // SCH_SCREEN objects store the full path and file name where the SCH_SHEET object only
        // stores the file name and extension.  Add the project path to the file name and
        // extension to compare when calling SCH_SHEET::SearchHierarchy().
        wxFileName fileName = aSheet->GetFileName();
        fileName.SetExt( "sch" );

        if( !fileName.IsAbsolute() )
            fileName.MakeAbsolute( m_currentPath.top() );

        // Save the current path so that it gets restored when decending and ascending the
        // sheet hierarchy which allows for sheet schematic files to be nested in folders
        // relative to the last path a schematic was loaded from.
        wxLogTrace( traceSchLegacyPlugin, "Saving path    \"%s\"", m_currentPath.top() );
        m_currentPath.push( fileName.GetPath() );
        wxLogTrace( traceSchLegacyPlugin, "Current path   \"%s\"", m_currentPath.top() );
        wxLogTrace( traceSchLegacyPlugin, "Loading        \"%s\"", fileName.GetFullPath() );

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

            for( auto aItem : aSheet->GetScreen()->Items().OfType( SCH_SHEET_T ) )
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


void SCH_LEGACY_PLUGIN::loadFile( const wxString& aFileName, SCH_SCREEN* aScreen )
{
    FILE_LINE_READER reader( aFileName );

    loadHeader( reader, aScreen );

    LoadContent( reader, aScreen, m_version );

    // Unfortunately schematic files prior to version 2 are not terminated with $EndSCHEMATC
    // so checking for it's existance will fail so just exit here and take our chances. :(
    if( m_version > 1 )
    {
        char* line = reader.Line();

        while( *line == ' ' )
            line++;

        if( !strCompare( "$EndSCHEMATC", line ) )
            THROW_IO_ERROR( "'$EndSCHEMATC' not found" );
    }
}


void SCH_LEGACY_PLUGIN::LoadContent( LINE_READER& aReader, SCH_SCREEN* aScreen, int version )
{
    m_version = version;

    // We cannot safely load content without a set root level.
    wxCHECK_RET( m_rootSheet,
            "Cannot call SCH_LEGACY_PLUGIN::LoadContent() without setting root sheet." );

    while( aReader.ReadLine() )
    {
        char* line = aReader.Line();

        while( *line == ' ' )
            line++;

        // Either an object will be loaded properly or the file load will fail and raise
        // an exception.
        if( strCompare( "$Descr", line ) )
            loadPageSettings( aReader, aScreen );
        else if( strCompare( "$Comp", line ) )
            aScreen->Append( loadComponent( aReader ) );
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
        else if( strCompare( "$EndSCHEMATC", line ) )
            return;
        else
            SCH_PARSE_ERROR( "unrecognized token", aReader, line );
    }
}


void SCH_LEGACY_PLUGIN::loadHeader( LINE_READER& aReader, SCH_SCREEN* aScreen )
{
    const char* line = aReader.ReadLine();

    if( !line || !strCompare( "Eeschema Schematic File Version", line, &line ) )
    {
        m_error.Printf(
                _( "\"%s\" does not appear to be an Eeschema file" ), aScreen->GetFileName() );
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


void SCH_LEGACY_PLUGIN::loadPageSettings( LINE_READER& aReader, SCH_SCREEN* aScreen )
{
    wxASSERT( aScreen != NULL );

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

    if( buf == PAGE_INFO::Custom )
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

    while( line != NULL )
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


SCH_SHEET* SCH_LEGACY_PLUGIN::loadSheet( LINE_READER& aReader )
{
    std::unique_ptr<SCH_SHEET> sheet = std::make_unique<SCH_SHEET>();

    const char* line = aReader.ReadLine();

    while( line != NULL )
    {
        if( strCompare( "S", line, &line ) )        // Sheet dimensions.
        {
            wxPoint position;

            position.x = Mils2Iu( parseInt( aReader, line, &line ) );
            position.y = Mils2Iu( parseInt( aReader, line, &line ) );
            sheet->SetPosition( position );

            wxSize  size;

            size.SetWidth( Mils2Iu( parseInt( aReader, line, &line ) ) );
            size.SetHeight( Mils2Iu( parseInt( aReader, line, &line ) ) );
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
            int fieldId = parseInt( aReader, line, &line );

            if( fieldId == 0 || fieldId == 1 )      // Sheet name and file name.
            {
                parseQuotedString( text, aReader, line, &line );
                size = Mils2Iu( parseInt( aReader, line, &line ) );

                SCH_FIELD& field = sheet->GetFields()[ fieldId ];
                field.SetText( text );
                field.SetTextSize( wxSize( size, size ) );
            }
            else                                   // Sheet pin.
            {
                // Use a unique_ptr so that we clean up in the case of a throw
                std::unique_ptr<SCH_SHEET_PIN> sheetPin = std::make_unique<SCH_SHEET_PIN>( sheet.get() );

                sheetPin->SetNumber( fieldId );

                // Can be empty fields.
                parseQuotedString( text, aReader, line, &line, true );

                sheetPin->SetText( text );

                if( line == NULL )
                    THROW_IO_ERROR( _( "unexpected end of line" ) );

                switch( parseChar( aReader, line, &line ) )
                {
                case 'I': sheetPin->SetShape( PINSHEETLABEL_SHAPE::PS_INPUT );       break;
                case 'O': sheetPin->SetShape( PINSHEETLABEL_SHAPE::PS_OUTPUT );      break;
                case 'B': sheetPin->SetShape( PINSHEETLABEL_SHAPE::PS_BIDI );        break;
                case 'T': sheetPin->SetShape( PINSHEETLABEL_SHAPE::PS_TRISTATE );    break;
                case 'U': sheetPin->SetShape( PINSHEETLABEL_SHAPE::PS_UNSPECIFIED ); break;
                default:  SCH_PARSE_ERROR( "invalid sheet pin type", aReader, line );
                }

                switch( parseChar( aReader, line, &line ) )
                {
                case 'R':  sheetPin->SetEdge( SHEET_RIGHT_SIDE );  break;
                case 'T':  sheetPin->SetEdge( SHEET_TOP_SIDE );    break;
                case 'B':  sheetPin->SetEdge( SHEET_BOTTOM_SIDE ); break;
                case 'L':  sheetPin->SetEdge( SHEET_LEFT_SIDE );   break;
                default:
                    SCH_PARSE_ERROR( "invalid sheet pin side", aReader, line );
                }

                wxPoint position;

                position.x = Mils2Iu( parseInt( aReader, line, &line ) );
                position.y = Mils2Iu( parseInt( aReader, line, &line ) );
                sheetPin->SetPosition( position );

                size = Mils2Iu( parseInt( aReader, line, &line ) );

                sheetPin->SetTextSize( wxSize( size, size ) );

                sheet->AddPin( sheetPin.release() );
            }
        }
        else if( strCompare( "$EndSheet", line ) )
        {
            sheet->AutoplaceFields( /* aScreen */ NULL, /* aManual */ false );
            return sheet.release();
        }

        line = aReader.ReadLine();
    }

    SCH_PARSE_ERROR( "missing '$EndSheet`", aReader, line );

    return NULL;  // Prevents compiler warning.  Should never get here.
}


SCH_BITMAP* SCH_LEGACY_PLUGIN::loadBitmap( LINE_READER& aReader )
{
    std::unique_ptr<SCH_BITMAP> bitmap = std::make_unique<SCH_BITMAP>();

    const char* line = aReader.Line();

    wxCHECK( strCompare( "$Bitmap", line, &line ), NULL );

    line = aReader.ReadLine();

    while( line != NULL )
    {
        if( strCompare( "Pos", line, &line ) )
        {
            wxPoint position;

            position.x = Mils2Iu( parseInt( aReader, line, &line ) );
            position.y = Mils2Iu( parseInt( aReader, line, &line ) );
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

            bitmap->GetImage()->SetScale( scalefactor );
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
                    bitmap->GetImage()->SetImage( image );
                    bitmap->GetImage()->SetBitmap( new wxBitmap( *image ) );
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


SCH_JUNCTION* SCH_LEGACY_PLUGIN::loadJunction( LINE_READER& aReader )
{
    std::unique_ptr<SCH_JUNCTION> junction = std::make_unique<SCH_JUNCTION>();

    const char* line = aReader.Line();

    wxCHECK( strCompare( "Connection", line, &line ), NULL );

    wxString name;

    parseUnquotedString( name, aReader, line, &line );

    wxPoint position;

    position.x = Mils2Iu( parseInt( aReader, line, &line ) );
    position.y = Mils2Iu( parseInt( aReader, line, &line ) );
    junction->SetPosition( position );

    return junction.release();
}


SCH_NO_CONNECT* SCH_LEGACY_PLUGIN::loadNoConnect( LINE_READER& aReader )
{
    std::unique_ptr<SCH_NO_CONNECT> no_connect = std::make_unique<SCH_NO_CONNECT>();

    const char* line = aReader.Line();

    wxCHECK( strCompare( "NoConn", line, &line ), NULL );

    wxString name;

    parseUnquotedString( name, aReader, line, &line );

    wxPoint position;

    position.x = Mils2Iu( parseInt( aReader, line, &line ) );
    position.y = Mils2Iu( parseInt( aReader, line, &line ) );
    no_connect->SetPosition( position );

    return no_connect.release();
}


SCH_LINE* SCH_LEGACY_PLUGIN::loadWire( LINE_READER& aReader )
{
    std::unique_ptr<SCH_LINE> wire = std::make_unique<SCH_LINE>();

    const char* line = aReader.Line();

    wxCHECK( strCompare( "Wire", line, &line ), NULL );

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
            int size = Mils2Iu( parseInt( aReader, line, &line ) );
            wire->SetLineWidth( size );
        }
        else if( buf == T_STYLE )
        {
            parseUnquotedString( buf, aReader, line, &line );
            PLOT_DASH_TYPE style = SCH_LINE::GetLineStyleByName( buf );
            wire->SetLineStyle( style );
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

    wxPoint begin, end;

    begin.x = Mils2Iu( parseInt( aReader, line, &line ) );
    begin.y = Mils2Iu( parseInt( aReader, line, &line ) );
    end.x = Mils2Iu( parseInt( aReader, line, &line ) );
    end.y = Mils2Iu( parseInt( aReader, line, &line ) );

    wire->SetStartPoint( begin );
    wire->SetEndPoint( end );

    return wire.release();
}


SCH_BUS_ENTRY_BASE* SCH_LEGACY_PLUGIN::loadBusEntry( LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxCHECK( strCompare( "Entry", line, &line ), NULL );

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
        SCH_PARSE_ERROR( "invalid bus entry type", aReader, line );

    line = aReader.ReadLine();

    wxPoint pos;
    wxSize size;

    pos.x = Mils2Iu( parseInt( aReader, line, &line ) );
    pos.y = Mils2Iu( parseInt( aReader, line, &line ) );
    size.x = Mils2Iu( parseInt( aReader, line, &line ) );
    size.y = Mils2Iu( parseInt( aReader, line, &line ) );

    size.x -= pos.x;
    size.y -= pos.y;

    busEntry->SetPosition( pos );
    busEntry->SetSize( size );

    return busEntry.release();
}

// clang-format off
const std::map<PINSHEETLABEL_SHAPE, const char*> sheetLabelNames
{
    { PINSHEETLABEL_SHAPE::PS_INPUT,       "Input" },
    { PINSHEETLABEL_SHAPE::PS_OUTPUT,      "Output" },
    { PINSHEETLABEL_SHAPE::PS_BIDI,        "BiDi" },
    { PINSHEETLABEL_SHAPE::PS_TRISTATE,    "3State" },
    { PINSHEETLABEL_SHAPE::PS_UNSPECIFIED, "UnSpc" },
};
// clang-format on


SCH_TEXT* SCH_LEGACY_PLUGIN::loadText( LINE_READER& aReader )
{
    const char*   line = aReader.Line();

    wxCHECK( strCompare( "Text", line, &line ), NULL );

    std::unique_ptr<SCH_TEXT> text;

    if( strCompare( "Notes", line, &line ) )
        text.reset( new SCH_TEXT );
    else if( strCompare( "Label", line, &line ) )
        text.reset( new SCH_LABEL );
    else if( strCompare( "HLabel", line, &line ) )
        text.reset( new SCH_HIERLABEL );
    else if( strCompare( "GLabel", line, &line ) )
    {
        // Prior to version 2, the SCH_GLOBALLABEL object did not exist.
        if( m_version == 1 )
            text = std::make_unique<SCH_HIERLABEL>();
        else
            text = std::make_unique<SCH_GLOBALLABEL>();
    }
    else
        SCH_PARSE_ERROR( "unknown Text type", aReader, line );

    // Parse the parameters common to all text objects.
    wxPoint position;

    position.x = Mils2Iu( parseInt( aReader, line, &line ) );
    position.y = Mils2Iu( parseInt( aReader, line, &line ) );
    text->SetPosition( position );

    int spinStyle = parseInt( aReader, line, &line );

    // Sadly we store the orientation of hierarchical and global labels using a different
    // int encoding than that for local labels:
    //                   Global      Local
    // Left justified      0           2
    // Up                  1           3
    // Right justified     2           0
    // Down                3           1
    // So we must flip it as the enum is setup with the "global" numbering
    if( text->Type() != SCH_GLOBAL_LABEL_T && text->Type() != SCH_HIER_LABEL_T )
    {
        if( spinStyle == 0 )
            spinStyle = 2;
        else if( spinStyle == 2 )
            spinStyle = 0;
    }

    text->SetLabelSpinStyle( (LABEL_SPIN_STYLE::SPIN) spinStyle );

    int size = Mils2Iu( parseInt( aReader, line, &line ) );

    text->SetTextSize( wxSize( size, size ) );

    // Parse the global and hierarchical label type.
    if( text->Type() == SCH_HIER_LABEL_T || text->Type() == SCH_GLOBAL_LABEL_T )
    {
        auto resultIt = std::find_if( sheetLabelNames.begin(), sheetLabelNames.end(),
                [ &line ]( const auto& it )
                {
                    return strCompare( it.second, line, &line );
                } );

        if( resultIt != sheetLabelNames.end() )
            text->SetShape( resultIt->first );
        else
            SCH_PARSE_ERROR( "invalid label type", aReader, line );
    }

    int penWidth = 0;

    // The following tokens do not exist in version 1 schematic files,
    // and not always in version 2 for HLabels and GLabels
    if( m_version > 1 )
    {
        if( m_version > 2 || *line >= ' ' )
        {
            if( strCompare( "Italic", line, &line ) )
                text->SetItalic( true );
            else if( !strCompare( "~", line, &line ) )
                SCH_PARSE_ERROR( _( "expected 'Italics' or '~'" ), aReader, line );
        }

        // The penWidth token does not exist in older versions of the schematic file format
        // so calling parseInt will be made only if the EOL is not reached.
        if( *line >= ' ' )
            penWidth = parseInt( aReader, line, &line );
    }

    text->SetBold( penWidth != 0 );
    text->SetTextThickness( penWidth != 0 ? GetPenSizeForBold( size ) : 0 );

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


SCH_COMPONENT* SCH_LEGACY_PLUGIN::loadComponent( LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxCHECK( strCompare( "$Comp", line, &line ), NULL );

    std::unique_ptr<SCH_COMPONENT> component = std::make_unique<SCH_COMPONENT>();

    line = aReader.ReadLine();

    while( line != NULL )
    {
        if( strCompare( "L", line, &line ) )
        {
            wxString libName;
            size_t pos = 2;                               // "X" plus ' ' space character.
            wxString utf8Line = wxString::FromUTF8( line );
            wxStringTokenizer tokens( utf8Line, " \r\n\t" );

            if( tokens.CountTokens() < 2 )
                THROW_PARSE_ERROR( "invalid symbol library definition", aReader.GetSource(),
                                   aReader.Line(), aReader.LineNumber(), pos );

            libName = tokens.GetNextToken();
            libName.Replace( "~", " " );

            LIB_ID libId;

            // Prior to schematic version 4, library IDs did not have a library nickname so
            // parsing the symbol name with LIB_ID::Parse() would break symbol library links
            // that contained '/' and ':' characters.
            if( m_version > 3 )
                libId.Parse( libName, true );
            else
                libId.SetLibItemName( libName, false );

            component->SetLibId( libId );

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
                component->SetPrefix( wxString( "U" ) );
            else
                component->SetPrefix( prefix );
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
                    m_rootSheet->GetScreen()->SetModify();
            }

            component->SetUnit( unit );

            // Same can also happen with the convert parameter
            int convert = parseInt( aReader, line, &line );

            if( convert == 0 )
            {
                convert = 1;

                // Set the file as modified so the user can be warned.
                if( m_rootSheet->GetScreen() )
                    m_rootSheet->GetScreen()->SetModify();
            }

            component->SetConvert( convert );

            wxString text;
            parseUnquotedString( text, aReader, line, &line );

            if( text != "00000000" )
                const_cast<KIID&>( component->m_Uuid ) = KIID( text );
        }
        else if( strCompare( "P", line, &line ) )
        {
            wxPoint pos;

            pos.x = Mils2Iu( parseInt( aReader, line, &line ) );
            pos.y = Mils2Iu( parseInt( aReader, line, &line ) );
            component->SetPosition( pos );
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

            // Note: AR path excludes root sheet, but includes component.  Normalize to
            // internal format by shifting everything down one and adding the root sheet.
            KIID_PATH path( pathStr );

            if( path.size() > 0 )
            {
                for( size_t i = path.size() - 1; i > 0; --i )
                    path[i] = path[i-1];

                path[0] = m_rootSheet->m_Uuid;
            }
            else
                path.push_back( m_rootSheet->m_Uuid );

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

            component->AddHierarchicalReference( path, reference, (int)tmp );
            component->GetField( REFERENCE_FIELD )->SetText( reference );

        }
        else if( strCompare( "F", line, &line ) )
        {
            int index = parseInt( aReader, line, &line );

            wxString text, name;

            parseQuotedString( text, aReader, line, &line, true );

            char orientation = parseChar( aReader, line, &line );
            wxPoint pos;
            pos.x = Mils2Iu( parseInt( aReader, line, &line ) );
            pos.y = Mils2Iu( parseInt( aReader, line, &line ) );
            int size = Mils2Iu( parseInt( aReader, line, &line ) );
            int attributes = parseHex( aReader, line, &line );

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

                SCH_FIELD field( wxPoint( 0, 0 ), index, component.get(), name );
                component->AddField( field );
            }

            SCH_FIELD& field = component->GetFields()[index];

            // Prior to version 2 of the schematic file format, none of the following existed.
            if( m_version > 1 )
            {
                wxString textAttrs;
                char hjustify = parseChar( aReader, line, &line );

                parseUnquotedString( textAttrs, aReader, line, &line );

                // The name of the field is optional.
                parseQuotedString( name, aReader, line, &line, true );

                if( hjustify == 'L' )
                    field.SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                else if( hjustify == 'R' )
                    field.SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
                else if( hjustify != 'C' )
                    SCH_PARSE_ERROR( "component field text horizontal justification must be "
                                     "L, R, or C", aReader, line );

                // We are guaranteed to have a least one character here for older file formats
                // otherwise an exception would have been raised..
                if( textAttrs[0] == 'T' )
                    field.SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
                else if( textAttrs[0] == 'B' )
                    field.SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
                else if( textAttrs[0] != 'C' )
                    SCH_PARSE_ERROR( "component field text vertical justification must be "
                                     "B, T, or C", aReader, line );

                // Newer file formats include the bold and italics text attribute.
                if( textAttrs.Length() > 1 )
                {
                    if( textAttrs.Length() != 3 )
                        SCH_PARSE_ERROR( _( "component field text attributes must be 3 characters wide" ),
                                         aReader, line );

                    if( textAttrs[1] == 'I' )
                        field.SetItalic( true );
                    else if( textAttrs[1] != 'N' )
                        SCH_PARSE_ERROR( "component field text italics indicator must be I or N",
                                         aReader, line );

                    if( textAttrs[2] == 'B' )
                        field.SetBold( true );
                    else if( textAttrs[2] != 'N' )
                        SCH_PARSE_ERROR( "component field text bold indicator must be B or N",
                                         aReader, line );
                }
            }

            field.SetText( text );
            field.SetTextPos( pos );
            field.SetVisible( !attributes );
            field.SetTextSize( wxSize( size, size ) );

            if( orientation == 'H' )
                field.SetTextAngle( TEXT_ANGLE_HORIZ );
            else if( orientation == 'V' )
                field.SetTextAngle( TEXT_ANGLE_VERT );
            else
                SCH_PARSE_ERROR( "component field orientation must be H or V", aReader, line );

            if( name.IsEmpty() )
                name = TEMPLATE_FIELDNAME::GetDefaultFieldName( index );

            field.SetName( name );
        }
        else if( strCompare( "$EndComp", line ) )
        {
            // Ensure all flags (some are set by previous initializations) are reset:
            component->ClearFlags();
            return component.release();
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
                SCH_PARSE_ERROR( "invalid component X1 transform value", aReader, line );

            transform.y1 = parseInt( aReader, line, &line );

            if( transform.y1 < -1 || transform.y1 > 1 )
                SCH_PARSE_ERROR( "invalid component Y1 transform value", aReader, line );

            transform.x2 = parseInt( aReader, line, &line );

            if( transform.x2 < -1 || transform.x2 > 1 )
                SCH_PARSE_ERROR( "invalid component X2 transform value", aReader, line );

            transform.y2 = parseInt( aReader, line, &line );

            if( transform.y2 < -1 || transform.y2 > 1 )
                SCH_PARSE_ERROR( "invalid component Y2 transform value", aReader, line );

            component->SetTransform( transform );
        }

        line = aReader.ReadLine();
    }

    SCH_PARSE_ERROR( "invalid component line", aReader, line );

    return NULL;  // Prevents compiler warning.  Should never get here.
}


std::shared_ptr<BUS_ALIAS> SCH_LEGACY_PLUGIN::loadBusAlias( LINE_READER& aReader,
                                                            SCH_SCREEN* aScreen )
{
    auto busAlias = std::make_shared<BUS_ALIAS>( aScreen );
    const char* line = aReader.Line();

    wxCHECK( strCompare( "BusAlias", line, &line ), NULL );

    wxString buf;
    parseUnquotedString( buf, aReader, line, &line );
    busAlias->SetName( buf );

    while( *line != '\0' )
    {
        buf.clear();
        parseUnquotedString( buf, aReader, line, &line, true );
        if( buf.Len() > 0 )
        {
            busAlias->AddMember( buf );
        }
    }

    return busAlias;
}


int SCH_LEGACY_PLUGIN_CACHE::m_modHash = 1;     // starts at 1 and goes up


SCH_LEGACY_PLUGIN_CACHE::SCH_LEGACY_PLUGIN_CACHE( const wxString& aFullPathAndFileName ) :
    m_fileName( aFullPathAndFileName ),
    m_libFileName( aFullPathAndFileName ),
    m_isWritable( true ),
    m_isModified( false )
{
    m_versionMajor = -1;
    m_versionMinor = -1;
    m_libType = SCH_LIB_TYPE::LT_EESCHEMA;
}


SCH_LEGACY_PLUGIN_CACHE::~SCH_LEGACY_PLUGIN_CACHE()
{
    // When the cache is destroyed, all of the alias objects on the heap should be deleted.
    for( auto& symbol : m_symbols )
        delete symbol.second;

    m_symbols.clear();
}


// If m_libFileName is a symlink follow it to the real source file
wxFileName SCH_LEGACY_PLUGIN_CACHE::GetRealFile() const
{
    wxFileName fn( m_libFileName );

#ifndef __WINDOWS__
    if( fn.Exists( wxFILE_EXISTS_SYMLINK ) )
    {
        char buffer[ PATH_MAX + 1 ];
        ssize_t pathLen = readlink( TO_UTF8( fn.GetFullPath() ), buffer, PATH_MAX );

        if( pathLen > 0 )
        {
            buffer[ pathLen ] = '\0';
            fn.Assign( fn.GetPath() + wxT( "/" ) + wxString::FromUTF8( buffer ) );
            fn.Normalize();
        }
    }
#endif

    return fn;
}


wxDateTime SCH_LEGACY_PLUGIN_CACHE::GetLibModificationTime()
{
    wxFileName fn = GetRealFile();

    // update the writable flag while we have a wxFileName, in a network this
    // is possibly quite dynamic anyway.
    m_isWritable = fn.IsFileWritable();

    return fn.GetModificationTime();
}


bool SCH_LEGACY_PLUGIN_CACHE::IsFile( const wxString& aFullPathAndFileName ) const
{
    return m_fileName == aFullPathAndFileName;
}


bool SCH_LEGACY_PLUGIN_CACHE::IsFileChanged() const
{
    wxFileName fn = GetRealFile();

    if( m_fileModTime.IsValid() && fn.IsOk() && fn.FileExists() )
        return fn.GetModificationTime() != m_fileModTime;

    return false;
}


void SCH_LEGACY_PLUGIN_CACHE::Load()
{
    if( !m_libFileName.FileExists() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Library file \"%s\" not found." ),
                                          m_libFileName.GetFullPath() ) );
    }

    wxCHECK_RET( m_libFileName.IsAbsolute(),
                 wxString::Format( "Cannot use relative file paths in legacy plugin to "
                                   "open library \"%s\".", m_libFileName.GetFullPath() ) );

    wxLogTrace( traceSchLegacyPlugin, "Loading legacy symbol file \"%s\"",
                m_libFileName.GetFullPath() );

    FILE_LINE_READER reader( m_libFileName.GetFullPath() );

    if( !reader.ReadLine() )
        THROW_IO_ERROR( _( "unexpected end of file" ) );

    const char* line = reader.Line();

    if( !strCompare( "EESchema-LIBRARY Version", line, &line ) )
    {
        // Old .sym files (which are libraries with only one symbol, used to store and reuse shapes)
        // EESchema-LIB Version x.x SYMBOL. They are valid files.
        if( !strCompare( "EESchema-LIB Version", line, &line ) )
            SCH_PARSE_ERROR( "file is not a valid component or symbol library file", reader, line );
    }

    m_versionMajor = parseInt( reader, line, &line );

    if( *line != '.' )
        SCH_PARSE_ERROR( "invalid file version formatting in header", reader, line );

    line++;

    m_versionMinor = parseInt( reader, line, &line );

    if( m_versionMajor < 1 || m_versionMinor < 0 || m_versionMinor > 99 )
        SCH_PARSE_ERROR( "invalid file version in header", reader, line );

    // Check if this is a symbol library which is the same as a component library but without
    // any alias, documentation, footprint filters, etc.
    if( strCompare( "SYMBOL", line, &line ) )
    {
        // Symbol files add date and time stamp info to the header.
        m_libType = SCH_LIB_TYPE::LT_SYMBOL;

        /// @todo Probably should check for a valid date and time stamp even though it's not used.
    }
    else
    {
        m_libType = SCH_LIB_TYPE::LT_EESCHEMA;
    }

    while( reader.ReadLine() )
    {
        line = reader.Line();

        if( *line == '#' || isspace( *line ) )  // Skip comments and blank lines.
            continue;

        // Headers where only supported in older library file formats.
        if( m_libType == SCH_LIB_TYPE::LT_EESCHEMA && strCompare( "$HEADER", line ) )
            loadHeader( reader );

        if( strCompare( "DEF", line ) )
        {
            // Read one DEF/ENDDEF part entry from library:
            LIB_PART* part = LoadPart( reader, m_versionMajor, m_versionMinor, &m_symbols );

            m_symbols[ part->GetName() ] = part;
        }
    }

    ++m_modHash;

    // Remember the file modification time of library file when the
    // cache snapshot was made, so that in a networked environment we will
    // reload the cache as needed.
    m_fileModTime = GetLibModificationTime();

    if( USE_OLD_DOC_FILE_FORMAT( m_versionMajor, m_versionMinor ) )
        loadDocs();
}


void SCH_LEGACY_PLUGIN_CACHE::loadDocs()
{
    const char* line;
    wxString    text;
    wxString    aliasName;
    wxFileName  fn = m_libFileName;
    LIB_PART*   symbol = NULL;;

    fn.SetExt( DOC_EXT );

    // Not all libraries will have a document file.
    if( !fn.FileExists() )
        return;

    if( !fn.IsFileReadable() )
        THROW_IO_ERROR( wxString::Format( _( "user does not have permission to read library "
                                             "document file \"%s\"" ), fn.GetFullPath() ) );

    FILE_LINE_READER reader( fn.GetFullPath() );

    line = reader.ReadLine();

    if( !line )
        THROW_IO_ERROR( _( "symbol document library file is empty" ) );

    if( !strCompare( DOCFILE_IDENT, line, &line ) )
        SCH_PARSE_ERROR( "invalid document library file version formatting in header",
                         reader, line );

    while( reader.ReadLine() )
    {
        line = reader.Line();

        if( *line == '#' )    // Comment line.
            continue;

        if( !strCompare( "$CMP", line, &line ) != 0 )
            SCH_PARSE_ERROR( "$CMP command expected", reader, line );

        aliasName = wxString::FromUTF8( line );
        aliasName.Trim();
        aliasName = LIB_ID::FixIllegalChars( aliasName );

        LIB_PART_MAP::iterator it = m_symbols.find( aliasName );

        if( it == m_symbols.end() )
            wxLogWarning( "Symbol '%s' not found in library:\n\n"
                          "'%s'\n\nat line %d offset %d", aliasName, fn.GetFullPath(),
                          reader.LineNumber(), (int) (line - reader.Line() ) );
        else
            symbol = it->second;

        // Read the curent alias associated doc.
        // if the alias does not exist, just skip the description
        // (Can happen if a .dcm is not synchronized with the corresponding .lib file)
        while( reader.ReadLine() )
        {
            line = reader.Line();

            if( !line )
                SCH_PARSE_ERROR( "unexpected end of file", reader, line );

            if( strCompare( "$ENDCMP", line, &line ) )
                break;

            text = FROM_UTF8( line + 2 );
            // Remove spaces at eol, and eol chars:
            text = text.Trim();

            switch( line[0] )
            {
            case 'D':
                if( symbol )
                    symbol->SetDescription( text );
                break;

            case 'K':
                if( symbol )
                    symbol->SetKeyWords( text );
                break;

            case 'F':
                if( symbol )
                    symbol->GetFieldById( DATASHEET_FIELD )->SetText( text );
                break;

            case 0:
            case '\n':
            case '\r':
            case '#':
                // Empty line or commment
                break;

            default:
                SCH_PARSE_ERROR( "expected token in symbol definition", reader, line );
            }
        }
    }
}


void SCH_LEGACY_PLUGIN_CACHE::loadHeader( FILE_LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxASSERT( strCompare( "$HEADER", line, &line ) );

    while( aReader.ReadLine() )
    {
        line = (char*) aReader;

        // The time stamp saved in old library files is not used or saved in the latest
        // library file version.
        if( strCompare( "TimeStamp", line, &line ) )
            continue;
        else if( strCompare( "$ENDHEADER", line, &line ) )
            return;
    }

    SCH_PARSE_ERROR( "$ENDHEADER not found", aReader, line );
}


LIB_PART* SCH_LEGACY_PLUGIN_CACHE::LoadPart( LINE_READER& aReader, int aMajorVersion,
                                             int aMinorVersion, LIB_PART_MAP* aMap )
{
    const char* line = aReader.Line();

    while( *line == '#' )
        aReader.ReadLine();

    if( !strCompare( "DEF", line, &line ) )
        SCH_PARSE_ERROR( "invalid symbol definition", aReader, line );

    long num;
    size_t pos = 4;                               // "DEF" plus the first space.
    wxString utf8Line = wxString::FromUTF8( line );
    wxStringTokenizer tokens( utf8Line, " \r\n\t" );

    if( tokens.CountTokens() < 8 )
        SCH_PARSE_ERROR( "invalid symbol definition", aReader, line );

    // Read DEF line:
    std::unique_ptr<LIB_PART> part = std::make_unique<LIB_PART>( wxEmptyString );

    wxString name, prefix, tmp;

    name = tokens.GetNextToken();
    pos += name.size() + 1;

    prefix = tokens.GetNextToken();
    pos += prefix.size() + 1;

    tmp = tokens.GetNextToken();
    pos += tmp.size() + 1;                        // NumOfPins, unused.

    tmp = tokens.GetNextToken();                  // Pin name offset.

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin offset", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    part->SetPinNameOffset( Mils2Iu( (int)num ) );

    tmp = tokens.GetNextToken();                  // Show pin numbers.

    if( !( tmp == "Y" || tmp == "N") )
        THROW_PARSE_ERROR( "expected Y or N", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    part->SetShowPinNumbers( ( tmp == "N" ) ? false : true );

    tmp = tokens.GetNextToken();                  // Show pin names.

    if( !( tmp == "Y" || tmp == "N") )
        THROW_PARSE_ERROR( "expected Y or N", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    part->SetShowPinNames( ( tmp == "N" ) ? false : true );

    tmp = tokens.GetNextToken();                  // Number of units.

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid unit count", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    part->SetUnitCount( (int)num );

    // Ensure m_unitCount is >= 1.  Could be read as 0 in old libraries.
    if( part->GetUnitCount() < 1 )
        part->SetUnitCount( 1 );

    // Copy part name and prefix.

    // The root alias is added to the alias list by SetName() which is called by SetText().
    if( name.IsEmpty() )
    {
        part->SetName( "~" );
    }
    else if( name[0] != '~' )
    {
        part->SetName( name );
    }
    else
    {
        part->SetName( name.Right( name.Length() - 1 ) );
        part->GetValueField().SetVisible( false );
    }

    // Don't set the library alias, this is determined by the symbol library table.
    part->SetLibId( LIB_ID( wxEmptyString, part->GetName() ) );

    LIB_FIELD& reference = part->GetReferenceField();

    if( prefix == "~" )
    {
        reference.Empty();
        reference.SetVisible( false );
    }
    else
    {
        reference.SetText( prefix );
    }

    // In version 2.2 and earlier, this parameter was a '0' which was just a place holder.
    // The was no concept of interchangeable multiple unit symbols.
    if( LIB_VERSION( aMajorVersion, aMinorVersion ) > 0
     && LIB_VERSION( aMajorVersion, aMinorVersion ) <= LIB_VERSION( 2, 2 ) )
    {
        // Nothing needs to be set since the default setting for symbols with multiple
        // units were never interchangeable.  Just parse the 0 an move on.
        tmp = tokens.GetNextToken();
        pos += tmp.size() + 1;
    }
    else
    {
        tmp = tokens.GetNextToken();

        if( tmp == "L" )
            part->LockUnits( true );
        else if( tmp == "F" || tmp == "0" )
            part->LockUnits( false );
        else
            THROW_PARSE_ERROR( "expected L, F, or 0", aReader.GetSource(), aReader.Line(),
                               aReader.LineNumber(), pos );

        pos += tmp.size() + 1;
    }

    // There is the optional power component flag.
    if( tokens.HasMoreTokens() )
    {
        tmp = tokens.GetNextToken();

        if( tmp == "P" )
            part->SetPower();
        else if( tmp == "N" )
            part->SetNormal();
        else
            THROW_PARSE_ERROR( "expected P or N", aReader.GetSource(), aReader.Line(),
                               aReader.LineNumber(), pos );
    }

    line = aReader.ReadLine();

    // Read lines until "ENDDEF" is found.
    while( line )
    {
        if( *line == '#' )                                  // Comment
            ;
        else if( strCompare( "Ti", line, &line ) )          // Modification date is ignored.
            continue;
        else if( strCompare( "ALIAS", line, &line ) )       // Aliases
            loadAliases( part, aReader, aMap );
        else if( *line == 'F' )                             // Fields
            loadField( part, aReader );
        else if( strCompare( "DRAW", line, &line ) )        // Drawing objects.
            loadDrawEntries( part, aReader, aMajorVersion, aMinorVersion );
        else if( strCompare( "$FPLIST", line, &line ) )     // Footprint filter list
            loadFootprintFilters( part, aReader );
        else if( strCompare( "ENDDEF", line, &line ) )      // End of part description
        {
            return part.release();
        }

        line = aReader.ReadLine();
    }

    SCH_PARSE_ERROR( "missing ENDDEF", aReader, line );
}


void SCH_LEGACY_PLUGIN_CACHE::loadAliases( std::unique_ptr<LIB_PART>& aPart,
                                           LINE_READER&               aReader,
                                           LIB_PART_MAP*              aMap )
{
    wxString newAliasName;
    const char* line = aReader.Line();

    wxCHECK_RET( strCompare( "ALIAS", line, &line ), "Invalid ALIAS section" );

    wxString utf8Line = wxString::FromUTF8( line );
    wxStringTokenizer tokens( utf8Line, " \r\n\t" );

    // Parse the ALIAS list.
    while( tokens.HasMoreTokens() )
    {
        newAliasName = tokens.GetNextToken();

        if( aMap )
        {
            LIB_PART* newPart = new LIB_PART( newAliasName );

            // Inherit the parent mandatory field attributes.
            for( int id = 0; id < MANDATORY_FIELDS; ++id )
            {
                LIB_FIELD* field = newPart->GetFieldById( id );

                // the MANDATORY_FIELDS are exactly that in RAM.
                wxASSERT( field );

                LIB_FIELD* parentField = aPart->GetFieldById( id );

                wxASSERT( parentField );

                *field = *parentField;

                if( id == VALUE_FIELD )
                    field->SetText( newAliasName );

                field->SetParent( newPart );
            }

            newPart->SetParent( aPart.get() );

            // This will prevent duplicate aliases.
            (*aMap)[ newPart->GetName() ] = newPart;
        }
    }
}


void SCH_LEGACY_PLUGIN_CACHE::loadField( std::unique_ptr<LIB_PART>& aPart,
                                         LINE_READER&               aReader )
{
    const char* line = aReader.Line();

    wxCHECK_RET( *line == 'F', "Invalid field line" );

    wxString    text;
    int         id;

    if( sscanf( line + 1, "%d", &id ) != 1 || id < 0 )
        SCH_PARSE_ERROR( "invalid field ID", aReader, line + 1 );

    LIB_FIELD* field;

    if( id >= 0 && id < MANDATORY_FIELDS )
    {
        field = aPart->GetFieldById( id );

        // this will fire only if somebody broke a constructor or editor.
        // MANDATORY_FIELDS are always present in ram resident components, no
        // exceptions, and they always have their names set, even fixed fields.
        wxASSERT( field );
    }
    else
    {
        field = new LIB_FIELD( aPart.get(), id );
        aPart->AddDrawItem( field, false );
    }

    // Skip to the first double quote.
    while( *line != '"' && *line != 0 )
        line++;

    if( *line == 0 )
        SCH_PARSE_ERROR( _( "unexpected end of line" ), aReader, line );

    parseQuotedString( text, aReader, line, &line, true );

    field->SetText( text );

    // Doctor the *.lib file field which has a "~" in blank fields.  New saves will
    // not save like this.
    if( text.size() == 1 && text[0] == '~' )
        field->SetText( wxEmptyString );
    else
        field->SetText( text );

    wxPoint pos;

    pos.x = Mils2Iu( parseInt( aReader, line, &line ) );
    pos.y = Mils2Iu( parseInt( aReader, line, &line ) );
    field->SetPosition( pos );

    wxSize textSize;

    textSize.x = textSize.y = Mils2Iu( parseInt( aReader, line, &line ) );
    field->SetTextSize( textSize );

    char textOrient = parseChar( aReader, line, &line );

    if( textOrient == 'H' )
        field->SetTextAngle( TEXT_ANGLE_HORIZ );
    else if( textOrient == 'V' )
        field->SetTextAngle( TEXT_ANGLE_VERT );
    else
        SCH_PARSE_ERROR( "invalid field text orientation parameter", aReader, line );

    char textVisible = parseChar( aReader, line, &line );

    if( textVisible == 'V' )
        field->SetVisible( true );
    else if ( textVisible == 'I' )
        field->SetVisible( false );
    else
        SCH_PARSE_ERROR( "invalid field text visibility parameter", aReader, line );

    // It may be technically correct to use the library version to determine if the field text
    // attributes are present.  If anyone knows if that is valid and what version that would be,
    // please change this to test the library version rather than an EOL or the quoted string
    // of the field name.
    if( *line != 0 && *line != '"' )
    {
        char textHJustify = parseChar( aReader, line, &line );

        if( textHJustify == 'C' )
            field->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        else if( textHJustify == 'L' )
            field->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        else if( textHJustify == 'R' )
            field->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        else
            SCH_PARSE_ERROR( "invalid field text horizontal justification", aReader, line );

        wxString attributes;

        parseUnquotedString( attributes, aReader, line, &line );

        size_t attrSize = attributes.size();

        if( !(attrSize == 3 || attrSize == 1 ) )
            SCH_PARSE_ERROR( "invalid field text attributes size", aReader, line );

        switch( (wxChar) attributes[0] )
        {
        case 'C': field->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER ); break;
        case 'B': field->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM ); break;
        case 'T': field->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );    break;
        default:  SCH_PARSE_ERROR( "invalid field text vertical justification", aReader, line );
        }

        if( attrSize == 3 )
        {
            wxChar attr_1 = attributes[1];
            wxChar attr_2 = attributes[2];

            if( attr_1 == 'I' )        // Italic
                field->SetItalic( true );
            else if( attr_1 != 'N' )   // No italics is default, check for error.
                SCH_PARSE_ERROR( "invalid field text italic parameter", aReader, line );

            if ( attr_2 == 'B' )       // Bold
                field->SetBold( true );
            else if( attr_2 != 'N' )   // No bold is default, check for error.
                SCH_PARSE_ERROR( "invalid field text bold parameter", aReader, line );
        }
    }

    // Fields in RAM must always have names.
    if( id >= 0 && id < MANDATORY_FIELDS )
    {
        // Fields in RAM must always have names, because we are trying to get
        // less dependent on field ids and more dependent on names.
        // Plus assumptions are made in the field editors.
        field->m_name = TEMPLATE_FIELDNAME::GetDefaultFieldName( id );

        // Ensure the VALUE field = the part name (can be not the case
        // with malformed libraries: edited by hand, or converted from other tools)
        if( id == VALUE_FIELD )
            field->SetText( aPart->GetName() );
    }
    else
    {
        parseQuotedString( field->m_name, aReader, line, &line, true );  // Optional.
    }
}


void SCH_LEGACY_PLUGIN_CACHE::loadDrawEntries( std::unique_ptr<LIB_PART>& aPart,
                                               LINE_READER&               aReader,
                                               int                        aMajorVersion,
                                               int                        aMinorVersion )
{
    const char* line = aReader.Line();

    wxCHECK_RET( strCompare( "DRAW", line, &line ), "Invalid DRAW section" );

    line = aReader.ReadLine();

    while( line )
    {
        if( strCompare( "ENDDRAW", line, &line ) )
        {
            aPart->GetDrawItems().sort();
            return;
        }

        switch( line[0] )
        {
        case 'A':    // Arc
            aPart->AddDrawItem( loadArc( aPart, aReader ), false );
            break;

        case 'C':    // Circle
            aPart->AddDrawItem( loadCircle( aPart, aReader ), false );
            break;

        case 'T':    // Text
            aPart->AddDrawItem( loadText( aPart, aReader, aMajorVersion, aMinorVersion ), false );
            break;

        case 'S':    // Square
            aPart->AddDrawItem( loadRectangle( aPart, aReader ), false );
            break;

        case 'X':    // Pin Description
            aPart->AddDrawItem( loadPin( aPart, aReader ), false );
            break;

        case 'P':    // Polyline
            aPart->AddDrawItem( loadPolyLine( aPart, aReader ), false );
            break;

        case 'B':    // Bezier Curves
            aPart->AddDrawItem( loadBezier( aPart, aReader ), false );
            break;

        case '#':    // Comment
        case '\n':   // Empty line
        case '\r':
        case 0:
            break;

        default:
            SCH_PARSE_ERROR( "undefined DRAW entry", aReader, line );
        }

        line = aReader.ReadLine();
    }

    SCH_PARSE_ERROR( "file ended prematurely loading component draw element", aReader, line );
}


FILL_TYPE SCH_LEGACY_PLUGIN_CACHE::parseFillMode( LINE_READER& aReader, const char* aLine,
                                               const char** aOutput )
{
    switch ( parseChar( aReader, aLine, aOutput ) )
    {
    case 'F': return FILL_TYPE::FILLED_SHAPE;
    case 'f': return FILL_TYPE::FILLED_WITH_BG_BODYCOLOR;
    case 'N': return FILL_TYPE::NO_FILL;
    default:  SCH_PARSE_ERROR( "invalid fill type, expected f, F, or N", aReader, aLine );
    }

    // This will never be reached but quiets the compiler warnings
    return FILL_TYPE::NO_FILL;
}


LIB_ARC* SCH_LEGACY_PLUGIN_CACHE::loadArc( std::unique_ptr<LIB_PART>& aPart,
                                           LINE_READER&               aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "A", line, &line ), NULL, "Invalid LIB_ARC definition" );

    LIB_ARC* arc = new LIB_ARC( aPart.get() );

    wxPoint center;

    center.x = Mils2Iu( parseInt( aReader, line, &line ) );
    center.y = Mils2Iu( parseInt( aReader, line, &line ) );

    arc->SetPosition( center );
    arc->SetRadius( Mils2Iu( parseInt( aReader, line, &line ) ) );

    int angle1 = parseInt( aReader, line, &line );
    int angle2 = parseInt( aReader, line, &line );

    NORMALIZE_ANGLE_POS( angle1 );
    NORMALIZE_ANGLE_POS( angle2 );
    arc->SetFirstRadiusAngle( angle1 );
    arc->SetSecondRadiusAngle( angle2 );

    arc->SetUnit( parseInt( aReader, line, &line ) );
    arc->SetConvert( parseInt( aReader, line, &line ) );
    arc->SetWidth( Mils2Iu( parseInt( aReader, line, &line ) ) );

    // Old libraries (version <= 2.2) do not have always this FILL MODE param
    // when fill mode is no fill (default mode).
    if( *line != 0 )
        arc->SetFillMode( parseFillMode( aReader, line, &line ) );

    // Actual Coordinates of arc ends are read from file
    if( *line != 0 )
    {
        wxPoint arcStart, arcEnd;

        arcStart.x = Mils2Iu( parseInt( aReader, line, &line ) );
        arcStart.y = Mils2Iu( parseInt( aReader, line, &line ) );
        arcEnd.x = Mils2Iu( parseInt( aReader, line, &line ) );
        arcEnd.y = Mils2Iu( parseInt( aReader, line, &line ) );

        arc->SetStart( arcStart );
        arc->SetEnd( arcEnd );
    }
    else
    {
        // Actual Coordinates of arc ends are not read from file
        // (old library), calculate them
        wxPoint arcStart( arc->GetRadius(), 0 );
        wxPoint arcEnd( arc->GetRadius(), 0 );

        RotatePoint( &arcStart.x, &arcStart.y, -angle1 );
        arcStart += arc->GetPosition();
        arc->SetStart( arcStart );
        RotatePoint( &arcEnd.x, &arcEnd.y, -angle2 );
        arcEnd += arc->GetPosition();
        arc->SetEnd( arcEnd );
    }

    return arc;
}


LIB_CIRCLE* SCH_LEGACY_PLUGIN_CACHE::loadCircle( std::unique_ptr<LIB_PART>& aPart,
                                                 LINE_READER&               aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "C", line, &line ), NULL, "Invalid LIB_CIRCLE definition" );

    LIB_CIRCLE* circle = new LIB_CIRCLE( aPart.get() );

    wxPoint center;

    center.x = Mils2Iu( parseInt( aReader, line, &line ) );
    center.y = Mils2Iu( parseInt( aReader, line, &line ) );

    circle->SetPosition( center );
    circle->SetRadius( Mils2Iu( parseInt( aReader, line, &line ) ) );
    circle->SetUnit( parseInt( aReader, line, &line ) );
    circle->SetConvert( parseInt( aReader, line, &line ) );
    circle->SetWidth( Mils2Iu( parseInt( aReader, line, &line ) ) );

    if( *line != 0 )
        circle->SetFillMode( parseFillMode( aReader, line, &line ) );

    return circle;
}


LIB_TEXT* SCH_LEGACY_PLUGIN_CACHE::loadText( std::unique_ptr<LIB_PART>& aPart,
                                             LINE_READER&               aReader,
                                             int                        aMajorVersion,
                                             int                        aMinorVersion )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "T", line, &line ), NULL, "Invalid LIB_TEXT definition" );

    LIB_TEXT* text = new LIB_TEXT( aPart.get() );

    text->SetTextAngle( (double) parseInt( aReader, line, &line ) );

    wxPoint center;

    center.x = Mils2Iu( parseInt( aReader, line, &line ) );
    center.y = Mils2Iu( parseInt( aReader, line, &line ) );
    text->SetPosition( center );

    wxSize size;

    size.x = size.y = Mils2Iu( parseInt( aReader, line, &line ) );
    text->SetTextSize( size );
    text->SetVisible( !parseInt( aReader, line, &line ) );
    text->SetUnit( parseInt( aReader, line, &line ) );
    text->SetConvert( parseInt( aReader, line, &line ) );

    wxString str;

    // If quoted string loading fails, load as not quoted string.
    if( *line == '"' )
        parseQuotedString( str, aReader, line, &line );
    else
    {
        parseUnquotedString( str, aReader, line, &line );

        // In old libs, "spaces" are replaced by '~' in unquoted strings:
        str.Replace( "~", " " );
    }

    if( !str.IsEmpty() )
    {
        // convert two apostrophes back to double quote
        str.Replace( "''", "\"" );
    }

    text->SetText( str );

    // Here things are murky and not well defined.  At some point it appears the format
    // was changed to add text properties.  However rather than add the token to the end of
    // the text definition, it was added after the string and no mention if the file
    // verion was bumped or not so this code make break on very old component libraries.
    //
    // Update: apparently even in the latest version this can be different so added a test
    //         for end of line before checking for the text properties.
    if( LIB_VERSION( aMajorVersion, aMinorVersion ) > 0
     && LIB_VERSION( aMajorVersion, aMinorVersion ) > LIB_VERSION( 2, 0 ) && !is_eol( *line ) )
    {
        if( strCompare( "Italic", line, &line ) )
            text->SetItalic( true );
        else if( !strCompare( "Normal", line, &line ) )
            SCH_PARSE_ERROR( "invalid text stype, expected 'Normal' or 'Italic'", aReader, line );

        if( parseInt( aReader, line, &line ) > 0 )
            text->SetBold( true );

        // Some old libaries version > 2.0 do not have these options for text justification:
        if( !is_eol( *line ) )
        {
            switch( parseChar( aReader, line, &line ) )
            {
            case 'L': text->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );   break;
            case 'C': text->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER ); break;
            case 'R': text->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );  break;
            default: SCH_PARSE_ERROR( "invalid horizontal text justication; expected L, C, or R",
                                      aReader, line );
            }

            switch( parseChar( aReader, line, &line ) )
            {
            case 'T': text->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );    break;
            case 'C': text->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER ); break;
            case 'B': text->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM ); break;
            default: SCH_PARSE_ERROR( "invalid vertical text justication; expected T, C, or B",
                                      aReader, line );
            }
        }
    }

    return text;
}


LIB_RECTANGLE* SCH_LEGACY_PLUGIN_CACHE::loadRectangle( std::unique_ptr<LIB_PART>& aPart,
                                                       LINE_READER&               aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "S", line, &line ), NULL, "Invalid LIB_RECTANGLE definition" );

    LIB_RECTANGLE* rectangle = new LIB_RECTANGLE( aPart.get() );

    wxPoint pos;

    pos.x = Mils2Iu( parseInt( aReader, line, &line ) );
    pos.y = Mils2Iu( parseInt( aReader, line, &line ) );
    rectangle->SetPosition( pos );

    wxPoint end;

    end.x = Mils2Iu( parseInt( aReader, line, &line ) );
    end.y = Mils2Iu( parseInt( aReader, line, &line ) );
    rectangle->SetEnd( end );

    rectangle->SetUnit( parseInt( aReader, line, &line ) );
    rectangle->SetConvert( parseInt( aReader, line, &line ) );
    rectangle->SetWidth( Mils2Iu( parseInt( aReader, line, &line ) ) );

    if( *line != 0 )
        rectangle->SetFillMode( parseFillMode( aReader, line, &line ) );

    return rectangle;
}


LIB_PIN* SCH_LEGACY_PLUGIN_CACHE::loadPin( std::unique_ptr<LIB_PART>& aPart,
                                           LINE_READER&               aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "X", line, &line ), NULL, "Invalid LIB_PIN definition" );

    wxString name;
    wxString number;

    size_t pos = 2;                               // "X" plus ' ' space character.
    wxString tmp;
    wxString utf8Line = wxString::FromUTF8( line );
    wxStringTokenizer tokens( utf8Line, " \r\n\t" );

    if( tokens.CountTokens() < 11 )
        SCH_PARSE_ERROR( "invalid pin definition", aReader, line );

    tmp = tokens.GetNextToken();
    name = tmp;
    pos += tmp.size() + 1;

    tmp = tokens.GetNextToken();
    number = tmp ;
    pos += tmp.size() + 1;

    long num;
    wxPoint position;

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin X coordinate", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    position.x = Mils2Iu( (int) num );

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin Y coordinate", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    position.y = Mils2Iu( (int) num );

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin length", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    int length = Mils2Iu( (int) num );


    tmp = tokens.GetNextToken();

    if( tmp.size() > 1 )
        THROW_PARSE_ERROR( "invalid pin orientation", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    int orientation = tmp[0];

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin number text size", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    int numberTextSize = Mils2Iu( (int) num );

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin name text size", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    int nameTextSize = Mils2Iu( (int) num );

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin unit", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    int unit = (int) num;

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin alternate body type", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    int convert = (int) num;

    tmp = tokens.GetNextToken();

    if( tmp.size() != 1 )
        THROW_PARSE_ERROR( "invalid pin type", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    char type = tmp[0];
    ELECTRICAL_PINTYPE pinType;

    switch( type )
    {
    case 'I': pinType = ELECTRICAL_PINTYPE::PT_INPUT;         break;
    case 'O': pinType = ELECTRICAL_PINTYPE::PT_OUTPUT;        break;
    case 'B': pinType = ELECTRICAL_PINTYPE::PT_BIDI;          break;
    case 'T': pinType = ELECTRICAL_PINTYPE::PT_TRISTATE;      break;
    case 'P': pinType = ELECTRICAL_PINTYPE::PT_PASSIVE;       break;
    case 'U': pinType = ELECTRICAL_PINTYPE::PT_UNSPECIFIED;   break;
    case 'W': pinType = ELECTRICAL_PINTYPE::PT_POWER_IN;      break;
    case 'w': pinType = ELECTRICAL_PINTYPE::PT_POWER_OUT;     break;
    case 'C': pinType = ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR; break;
    case 'E': pinType = ELECTRICAL_PINTYPE::PT_OPENEMITTER;   break;
    case 'N': pinType = ELECTRICAL_PINTYPE::PT_NC;            break;
    default:
        THROW_PARSE_ERROR( "unknown pin type", aReader.GetSource(), aReader.Line(),
                aReader.LineNumber(), pos );
    }


    LIB_PIN* pin = new LIB_PIN( aPart.get(), name, number, orientation, pinType, length,
            nameTextSize, numberTextSize, convert, position, unit );

    // Optional
    if( tokens.HasMoreTokens() )       /* Special Symbol defined */
    {
        tmp = tokens.GetNextToken();

        enum
        {
            INVERTED        = 1 << 0,
            CLOCK           = 1 << 1,
            LOWLEVEL_IN     = 1 << 2,
            LOWLEVEL_OUT    = 1 << 3,
            FALLING_EDGE    = 1 << 4,
            NONLOGIC        = 1 << 5
        };

        int flags = 0;

        for( int j = tmp.size(); j > 0; )
        {
            switch( tmp[--j].GetValue() )
            {
            case '~': break;
            case 'N': pin->SetVisible( false ); break;
            case 'I': flags |= INVERTED;        break;
            case 'C': flags |= CLOCK;           break;
            case 'L': flags |= LOWLEVEL_IN;     break;
            case 'V': flags |= LOWLEVEL_OUT;    break;
            case 'F': flags |= FALLING_EDGE;    break;
            case 'X': flags |= NONLOGIC;        break;
            default: THROW_PARSE_ERROR( "invalid pin attribut", aReader.GetSource(),
                                        aReader.Line(), aReader.LineNumber(), pos );
            }

            pos += 1;
        }

        switch( flags )
        {
        case 0:                   pin->SetShape( GRAPHIC_PINSHAPE::LINE );               break;
        case INVERTED:            pin->SetShape( GRAPHIC_PINSHAPE::INVERTED );           break;
        case CLOCK:               pin->SetShape( GRAPHIC_PINSHAPE::CLOCK );              break;
        case INVERTED | CLOCK:    pin->SetShape( GRAPHIC_PINSHAPE::INVERTED_CLOCK );     break;
        case LOWLEVEL_IN:         pin->SetShape( GRAPHIC_PINSHAPE::INPUT_LOW );          break;
        case LOWLEVEL_IN | CLOCK: pin->SetShape( GRAPHIC_PINSHAPE::CLOCK_LOW );          break;
        case LOWLEVEL_OUT:        pin->SetShape( GRAPHIC_PINSHAPE::OUTPUT_LOW );         break;
        case FALLING_EDGE:        pin->SetShape( GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK ); break;
        case NONLOGIC:            pin->SetShape( GRAPHIC_PINSHAPE::NONLOGIC );           break;
        default:
            SCH_PARSE_ERROR( "pin attributes do not define a valid pin shape", aReader, line );
        }
    }

    return pin;
}


LIB_POLYLINE* SCH_LEGACY_PLUGIN_CACHE::loadPolyLine( std::unique_ptr<LIB_PART>& aPart,
                                                     LINE_READER&               aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "P", line, &line ), NULL, "Invalid LIB_POLYLINE definition" );

    LIB_POLYLINE* polyLine = new LIB_POLYLINE( aPart.get() );

    int points = parseInt( aReader, line, &line );
    polyLine->SetUnit( parseInt( aReader, line, &line ) );
    polyLine->SetConvert( parseInt( aReader, line, &line ) );
    polyLine->SetWidth( Mils2Iu( parseInt( aReader, line, &line ) ) );
    polyLine->Reserve( points );

    wxPoint pt;

    for( int i = 0; i < points; i++ )
    {
        pt.x = Mils2Iu( parseInt( aReader, line, &line ) );
        pt.y = Mils2Iu( parseInt( aReader, line, &line ) );
        polyLine->AddPoint( pt );
    }

    if( *line != 0 )
        polyLine->SetFillMode( parseFillMode( aReader, line, &line ) );

    return polyLine;
}


LIB_BEZIER* SCH_LEGACY_PLUGIN_CACHE::loadBezier( std::unique_ptr<LIB_PART>& aPart,
                                                 LINE_READER&               aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "B", line, &line ), NULL, "Invalid LIB_BEZIER definition" );

    LIB_BEZIER* bezier = new LIB_BEZIER( aPart.get() );

    int points = parseInt( aReader, line, &line );
    bezier->SetUnit( parseInt( aReader, line, &line ) );
    bezier->SetConvert( parseInt( aReader, line, &line ) );
    bezier->SetWidth( Mils2Iu( parseInt( aReader, line, &line ) ) );

    wxPoint pt;
    bezier->Reserve( points );

    for( int i = 0; i < points; i++ )
    {
        pt.x = Mils2Iu( parseInt( aReader, line, &line ) );
        pt.y = Mils2Iu( parseInt( aReader, line, &line ) );
        bezier->AddPoint( pt );
    }

    if( *line != 0 )
        bezier->SetFillMode( parseFillMode( aReader, line, &line ) );

    return bezier;
}


void SCH_LEGACY_PLUGIN_CACHE::loadFootprintFilters( std::unique_ptr<LIB_PART>& aPart,
                                                    LINE_READER&               aReader )
{
    const char* line = aReader.Line();

    wxCHECK_RET( strCompare( "$FPLIST", line, &line ), "Invalid footprint filter list" );

    line = aReader.ReadLine();

    wxArrayString footprintFilters;

    while( line )
    {
        if( strCompare( "$ENDFPLIST", line, &line ) )
        {
            aPart->SetFPFilters( footprintFilters );
            return;
        }

        wxString footprint;

        parseUnquotedString( footprint, aReader, line, &line );
        footprintFilters.Add( footprint );
        line = aReader.ReadLine();
    }

    SCH_PARSE_ERROR( "file ended prematurely while loading footprint filters", aReader, line );
}


void SCH_LEGACY_PLUGIN::cacheLib( const wxString& aLibraryFileName )
{
    if( !m_cache || !m_cache->IsFile( aLibraryFileName ) || m_cache->IsFileChanged() )
    {
        // a spectacular episode in memory management:
        delete m_cache;
        m_cache = new SCH_LEGACY_PLUGIN_CACHE( aLibraryFileName );

        // Because m_cache is rebuilt, increment PART_LIBS::s_modify_generation
        // to modify the hash value that indicate component to symbol links
        // must be updated.
        PART_LIBS::s_modify_generation++;

        if( !isBuffering( m_props ) )
            m_cache->Load();
    }
}


bool SCH_LEGACY_PLUGIN::isBuffering( const PROPERTIES* aProperties )
{
    return ( aProperties && aProperties->Exists( SCH_LEGACY_PLUGIN::PropBuffering ) );
}


int SCH_LEGACY_PLUGIN::GetModifyHash() const
{
    if( m_cache )
        return m_cache->GetModifyHash();

    // If the cache hasn't been loaded, it hasn't been modified.
    return 0;
}


void SCH_LEGACY_PLUGIN::EnumerateSymbolLib( wxArrayString&    aSymbolNameList,
                                            const wxString&   aLibraryPath,
                                            const PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    m_props = aProperties;

    bool powerSymbolsOnly = ( aProperties &&
                              aProperties->find( SYMBOL_LIB_TABLE::PropPowerSymsOnly ) != aProperties->end() );
    cacheLib( aLibraryPath );

    const LIB_PART_MAP& symbols = m_cache->m_symbols;

    for( LIB_PART_MAP::const_iterator it = symbols.begin();  it != symbols.end();  ++it )
    {
        if( !powerSymbolsOnly || it->second->IsPower() )
            aSymbolNameList.Add( it->first );
    }
}


void SCH_LEGACY_PLUGIN::EnumerateSymbolLib( std::vector<LIB_PART*>& aSymbolList,
                                            const wxString&   aLibraryPath,
                                            const PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    m_props = aProperties;

    bool powerSymbolsOnly = ( aProperties &&
                              aProperties->find( SYMBOL_LIB_TABLE::PropPowerSymsOnly ) != aProperties->end() );
    cacheLib( aLibraryPath );

    const LIB_PART_MAP& symbols = m_cache->m_symbols;

    for( LIB_PART_MAP::const_iterator it = symbols.begin();  it != symbols.end();  ++it )
    {
        if( !powerSymbolsOnly || it->second->IsPower() )
            aSymbolList.push_back( it->second );
    }
}


LIB_PART* SCH_LEGACY_PLUGIN::LoadSymbol( const wxString& aLibraryPath, const wxString& aSymbolName,
                                         const PROPERTIES* aProperties )
{
    LOCALE_IO toggle;     // toggles on, then off, the C locale.

    m_props = aProperties;

    cacheLib( aLibraryPath );

    LIB_PART_MAP::const_iterator it = m_cache->m_symbols.find( aSymbolName );

    if( it == m_cache->m_symbols.end() )
        return nullptr;

    return it->second;
}


bool SCH_LEGACY_PLUGIN::DeleteSymbolLib( const wxString& aLibraryPath,
                                         const PROPERTIES* aProperties )
{
    wxFileName fn = aLibraryPath;

    if( !fn.FileExists() )
        return false;

    // Some of the more elaborate wxRemoveFile() crap puts up its own wxLog dialog
    // we don't want that.  we want bare metal portability with no UI here.
    if( wxRemove( aLibraryPath ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "library \"%s\" cannot be deleted" ),
                                          aLibraryPath.GetData() ) );
    }

    if( m_cache && m_cache->IsFile( aLibraryPath ) )
    {
        delete m_cache;
        m_cache = 0;
    }

    return true;
}


bool SCH_LEGACY_PLUGIN::CheckHeader( const wxString& aFileName )
{
    // Open file and check first line
    wxTextFile tempFile;

    tempFile.Open( aFileName );
    wxString firstline;
    // read the first line
    firstline = tempFile.GetFirstLine();
    tempFile.Close();

    return firstline.StartsWith( "EESchema" );
}


bool SCH_LEGACY_PLUGIN::IsSymbolLibWritable( const wxString& aLibraryPath )
{
    // Writing legacy symbol libraries is deprecated.
    return false;
}


LIB_PART* SCH_LEGACY_PLUGIN::ParsePart( LINE_READER& reader, int aMajorVersion,
                                        int aMinorVersion )
{
    return SCH_LEGACY_PLUGIN_CACHE::LoadPart( reader, aMajorVersion, aMinorVersion );
}


const char* SCH_LEGACY_PLUGIN::PropBuffering = "buffering";
const char* SCH_LEGACY_PLUGIN::PropNoDocFile = "no_doc_file";
