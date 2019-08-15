/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2016-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <algorithm>
#include <boost/algorithm/string/join.hpp>

#include <wx/mstream.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <pgm_base.h>
#include <gr_text.h>
#include <kiway.h>
#include <kicad_string.h>
#include <richio.h>
#include <core/typeinfo.h>
#include <properties.h>
#include <trace_helpers.h>

#include <general.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_component.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_no_connect.h>
#include <sch_text.h>
#include <sch_sheet.h>
#include <sch_bitmap.h>
#include <bus_alias.h>
#include <sch_legacy_plugin.h>
#include <template_fieldnames.h>
#include <sch_screen.h>
#include <class_libentry.h>
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
#include <confirm.h>
#include <tool/selection.h>


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
static uint32_t parseHex( LINE_READER& aReader, const char* aLine,
                               const char** aOutput = NULL )
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

        while( *next && *next == ' ' )
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
    LIB_ALIAS_MAP   m_aliases;      // Map of names of LIB_ALIAS pointers.
    bool            m_isWritable;
    bool            m_isModified;
    int             m_versionMajor;
    int             m_versionMinor;
    int             m_libType;      // Is this cache a component or symbol library.

    void                  loadHeader( FILE_LINE_READER& aReader );
    static void           loadAliases( std::unique_ptr<LIB_PART>& aPart, LINE_READER& aReader );
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

    static FILL_T   parseFillMode( LINE_READER& aReader, const char* aLine,
                                          const char** aOutput );
    LIB_ALIAS*      removeAlias( LIB_ALIAS* aAlias );

    void            saveDocFile();
    static void     saveArc( LIB_ARC* aArc, OUTPUTFORMATTER& aFormatter );
    static void     saveBezier( LIB_BEZIER* aBezier, OUTPUTFORMATTER& aFormatter );
    static void     saveCircle( LIB_CIRCLE* aCircle, OUTPUTFORMATTER& aFormatter );
    static void     saveField( LIB_FIELD* aField, OUTPUTFORMATTER& aFormatter );
    static void     savePin( LIB_PIN* aPin, OUTPUTFORMATTER& aFormatter );
    static void     savePolyLine( LIB_POLYLINE* aPolyLine, OUTPUTFORMATTER& aFormatter );
    static void     saveRectangle( LIB_RECTANGLE* aRectangle, OUTPUTFORMATTER& aFormatter );
    static void     saveText( LIB_TEXT* aText, OUTPUTFORMATTER& aFormatter );

    friend SCH_LEGACY_PLUGIN;

public:
    SCH_LEGACY_PLUGIN_CACHE( const wxString& aLibraryPath );
    ~SCH_LEGACY_PLUGIN_CACHE();

    int GetModifyHash() const { return m_modHash; }

    // Most all functions in this class throw IO_ERROR exceptions.  There are no
    // error codes nor user interface calls from here, nor in any SCH_PLUGIN objects.
    // Catch these exceptions higher up please.

    /// Save the entire library to file m_libFileName;
    void Save( bool aSaveDocFile = true );

    void Load();

    void AddSymbol( const LIB_PART* aPart );

    void DeleteAlias( const wxString& aAliasName );

    void DeleteSymbol( const wxString& aAliasName );

    // If m_libFileName is a symlink follow it to the real source file
    wxFileName GetRealFile() const;

    wxDateTime GetLibModificationTime();

    bool IsFile( const wxString& aFullPathAndFileName ) const;

    bool IsFileChanged() const;

    void SetModified( bool aModified = true ) { m_isModified = aModified; }

    wxString GetLogicalName() const { return m_libFileName.GetName(); }

    void SetFileName( const wxString& aFileName ) { m_libFileName = aFileName; }

    wxString GetFileName() const { return m_libFileName.GetFullPath(); }

    static LIB_PART* LoadPart( LINE_READER& aReader, int aMajorVersion, int aMinorVersion );
    static void      SaveSymbol( LIB_PART* aSymbol, OUTPUTFORMATTER& aFormatter );
};


SCH_LEGACY_PLUGIN::SCH_LEGACY_PLUGIN()
{
    init( NULL );
}


SCH_LEGACY_PLUGIN::~SCH_LEGACY_PLUGIN()
{
    delete m_cache;
}


void SCH_LEGACY_PLUGIN::init( KIWAY* aKiway, const PROPERTIES* aProperties )
{
    m_version = 0;
    m_rootSheet = NULL;
    m_props = aProperties;
    m_kiway = aKiway;
    m_cache = NULL;
    m_out = NULL;
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
            m_path = aKiway->Prj().GetProjectPath();

        wxLogTrace( traceSchLegacyPlugin, "Normalized append path \"%s\".", m_path );
    }
    else
    {
        m_path = aKiway->Prj().GetProjectPath();
    }

    m_currentPath.push( m_path );
    init( aKiway, aProperties );

    if( aAppendToMe == NULL )
    {
        // Clean up any allocated memory if an exception occurs loading the schematic.
        std::unique_ptr< SCH_SHEET > newSheet( new SCH_SHEET );
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

            // Do not need to load the sub-sheets - this has already been done.
        }
        else
        {
            aSheet->SetScreen( new SCH_SCREEN( m_kiway ) );
            aSheet->GetScreen()->SetFileName( fileName.GetFullPath() );

            try
            {
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
        m_error.Printf( _( "\"%s\" does not appear to be an Eeschema file" ),
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

    SCH_PARSE_ERROR( "missing 'EndDescr'", aReader, line );
}


SCH_SHEET* SCH_LEGACY_PLUGIN::loadSheet( LINE_READER& aReader )
{
    std::unique_ptr< SCH_SHEET > sheet( new SCH_SHEET() );

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
                std::unique_ptr< SCH_SHEET_PIN > sheetPin( new SCH_SHEET_PIN( sheet.get() ) );

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
                    SCH_PARSE_ERROR( "invalid sheet pin type", aReader, line );
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

                position.x = parseInt( aReader, line, &line );
                position.y = parseInt( aReader, line, &line );
                sheetPin->SetPosition( position );

                size = parseInt( aReader, line, &line );

                sheetPin->SetTextSize( wxSize( size, size ) );

                sheet->AddPin( sheetPin.release() );
            }
        }
        else if( strCompare( "$EndSheet", line ) )
            return sheet.release();

        line = aReader.ReadLine();
    }

    SCH_PARSE_ERROR( "missing '$EndSheet`", aReader, line );

    return NULL;  // Prevents compiler warning.  Should never get here.
}


SCH_BITMAP* SCH_LEGACY_PLUGIN::loadBitmap( LINE_READER& aReader )
{
    std::unique_ptr< SCH_BITMAP > bitmap( new SCH_BITMAP );

    const char* line = aReader.Line();

    wxCHECK( strCompare( "$Bitmap", line, &line ), NULL );

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
    std::unique_ptr< SCH_JUNCTION > junction( new SCH_JUNCTION );

    const char* line = aReader.Line();

    wxCHECK( strCompare( "Connection", line, &line ), NULL );

    wxString name;

    parseUnquotedString( name, aReader, line, &line );

    wxPoint position;

    position.x = parseInt( aReader, line, &line );
    position.y = parseInt( aReader, line, &line );
    junction->SetPosition( position );

    return junction.release();
}


SCH_NO_CONNECT* SCH_LEGACY_PLUGIN::loadNoConnect( LINE_READER& aReader )
{
    std::unique_ptr< SCH_NO_CONNECT > no_connect( new SCH_NO_CONNECT );

    const char* line = aReader.Line();

    wxCHECK( strCompare( "NoConn", line, &line ), NULL );

    wxString name;

    parseUnquotedString( name, aReader, line, &line );

    wxPoint position;

    position.x = parseInt( aReader, line, &line );
    position.y = parseInt( aReader, line, &line );
    no_connect->SetPosition( position );

    return no_connect.release();
}


SCH_LINE* SCH_LEGACY_PLUGIN::loadWire( LINE_READER& aReader )
{
    std::unique_ptr< SCH_LINE > wire( new SCH_LINE );

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
            int size = parseInt( aReader, line, &line );
            wire->SetLineWidth( size );
        }
        else if( buf == T_STYLE )
        {
            parseUnquotedString( buf, aReader, line, &line );
            int style = SCH_LINE::GetLineStyleInternalId( buf );
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

    begin.x = parseInt( aReader, line, &line );
    begin.y = parseInt( aReader, line, &line );
    end.x = parseInt( aReader, line, &line );
    end.y = parseInt( aReader, line, &line );

    wire->SetStartPoint( begin );
    wire->SetEndPoint( end );

    return wire.release();
}


SCH_BUS_ENTRY_BASE* SCH_LEGACY_PLUGIN::loadBusEntry( LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxCHECK( strCompare( "Entry", line, &line ), NULL );

    std::unique_ptr< SCH_BUS_ENTRY_BASE > busEntry;

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


SCH_TEXT* SCH_LEGACY_PLUGIN::loadText( LINE_READER& aReader )
{
    const char*   line = aReader.Line();

    wxCHECK( strCompare( "Text", line, &line ), NULL );

    std::unique_ptr< SCH_TEXT> text;

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
            text.reset( new SCH_HIERLABEL );
        else
            text.reset( new SCH_GLOBALLABEL );
    }
    else
        SCH_PARSE_ERROR( "unknown Text type", aReader, line );

    // Parse the parameters common to all text objects.
    wxPoint position;

    position.x = parseInt( aReader, line, &line );
    position.y = parseInt( aReader, line, &line );
    text->SetPosition( position );
    text->SetLabelSpinStyle( parseInt( aReader, line, &line ) );

    int size = parseInt( aReader, line, &line );

    text->SetTextSize( wxSize( size, size ) );

    // Parse the global and hierarchical label type.
    if( text->Type() == SCH_HIER_LABEL_T || text->Type() == SCH_GLOBAL_LABEL_T )
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
            SCH_PARSE_ERROR( "invalid label type", aReader, line );
    }

    int thickness = 0;

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

        // The thickness token does not exist in older versions of the schematic file format
        // so calling parseInt will be made only if the EOL is not reached.
        if( *line >= ' ' )
            thickness = parseInt( aReader, line, &line );
    }

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


SCH_COMPONENT* SCH_LEGACY_PLUGIN::loadComponent( LINE_READER& aReader )
{
    const char* line = aReader.Line();

    wxCHECK( strCompare( "$Comp", line, &line ), NULL );

    std::unique_ptr< SCH_COMPONENT > component( new SCH_COMPONENT() );

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
                libId.Parse( libName, LIB_ID::ID_SCH, true );
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
                if( m_rootSheet && m_rootSheet->GetScreen() )
                    m_rootSheet->GetScreen()->SetModify();
            }

            component->SetUnit( unit );

            // Same can also happen with the convert parameter
            int convert = parseInt( aReader, line, &line );

            if( convert == 0 )
            {
                convert = 1;

                // Set the file as modified so the user can be warned.
                if( m_rootSheet && m_rootSheet->GetScreen() )
                    m_rootSheet->GetScreen()->SetModify();
            }

            component->SetConvert( convert );

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

            if( strncasecmp( strCompare, line, len ) != 0 )
                SCH_PARSE_ERROR( "missing 'Path=' token", aReader, line );

            line += len;
            wxString path, reference, unit;

            parseQuotedString( path, aReader, line, &line );

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
            component->GetField( REFERENCE )->SetText( reference );

        }
        else if( strCompare( "F", line, &line ) )
        {
            int index = parseInt( aReader, line, &line );

            wxString text, name;

            parseQuotedString( text, aReader, line, &line, true );

            char orientation = parseChar( aReader, line, &line );
            wxPoint pos;
            pos.x = parseInt( aReader, line, &line );
            pos.y = parseInt( aReader, line, &line );
            int size = parseInt( aReader, line, &line );
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

                SCH_FIELD field( wxPoint( 0, 0 ), -1, component.get(), name );
                component->AddField( field );
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
                    component->GetField( index )->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                else if( hjustify == 'R' )
                    component->GetField( index )->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
                else if( hjustify != 'C' )
                    SCH_PARSE_ERROR( "component field text horizontal justification must be "
                                     "L, R, or C", aReader, line );

                // We are guaranteed to have a least one character here for older file formats
                // otherwise an exception would have been raised..
                if( textAttrs[0] == 'T' )
                    component->GetField( index )->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
                else if( textAttrs[0] == 'B' )
                    component->GetField( index )->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
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
                        component->GetField( index )->SetItalic( true );
                    else if( textAttrs[1] != 'N' )
                        SCH_PARSE_ERROR( "component field text italics indicator must be I or N",
                                         aReader, line );

                    if( textAttrs[2] == 'B' )
                        component->GetField( index )->SetBold( true );
                    else if( textAttrs[2] != 'N' )
                        SCH_PARSE_ERROR( "component field text bold indicator must be B or N",
                                         aReader, line );
                }
            }

            component->GetField( index )->SetText( text );
            component->GetField( index )->SetTextPos( pos );
            component->GetField( index )->SetVisible( !attributes );
            component->GetField( index )->SetTextSize( wxSize( size, size ) );

            if( orientation == 'H' )
                component->GetField( index )->SetTextAngle( TEXT_ANGLE_HORIZ );
            else if( orientation == 'V' )
                component->GetField( index )->SetTextAngle( TEXT_ANGLE_VERT );
            else
                SCH_PARSE_ERROR( "component field orientation must be H or V",
                                 aReader, line );

            if( name.IsEmpty() )
                name = TEMPLATE_FIELDNAME::GetDefaultFieldName( index );

            component->GetField( index )->SetName( name );
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
    auto busAlias = std::make_shared< BUS_ALIAS >( aScreen );
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


void SCH_LEGACY_PLUGIN::Save( const wxString& aFileName, SCH_SCREEN* aScreen, KIWAY* aKiway,
                              const PROPERTIES* aProperties )
{
    wxCHECK_RET( aScreen != NULL, "NULL SCH_SCREEN object." );
    wxCHECK_RET( !aFileName.IsEmpty(), "No schematic file name defined." );

    LOCALE_IO   toggle;     // toggles on, then off, the C locale, to write floating point values.

    init( aKiway, aProperties );

    wxFileName fn = aFileName;

    // File names should be absolute.  Don't assume everything relative to the project path
    // works properly.
    wxASSERT( fn.IsAbsolute() );

    FILE_OUTPUTFORMATTER formatter( fn.GetFullPath() );

    m_out = &formatter;     // no ownership

    Format( aScreen );
}


void SCH_LEGACY_PLUGIN::Format( SCH_SCREEN* aScreen )
{
    wxCHECK_RET( aScreen != NULL, "NULL SCH_SCREEN* object." );
    wxCHECK_RET( m_kiway != NULL, "NULL KIWAY* object." );

    // Write the header
    m_out->Print( 0, "%s %s %d\n", "EESchema", SCHEMATIC_HEAD_STRING, EESCHEMA_VERSION );

    // Write the project libraries.
    for( const PART_LIB& lib : *m_kiway->Prj().SchLibs() )
        m_out->Print( 0, "LIBS:%s\n", TO_UTF8( lib.GetName() ) );

    // This section is not used, but written for file compatibility
    m_out->Print( 0, "EELAYER %d %d\n", SCH_LAYER_ID_COUNT, 0 );
    m_out->Print( 0, "EELAYER END\n" );

    /* Write page info, ScreenNumber and NumberOfScreen; not very meaningful for
     * SheetNumber and Sheet Count in a complex hierarchy, but useful in
     * simple hierarchy and flat hierarchy.  Used also to search the root
     * sheet ( ScreenNumber = 1 ) within the files
     */
    const TITLE_BLOCK& tb = aScreen->GetTitleBlock();
    const PAGE_INFO& page = aScreen->GetPageSettings();

    m_out->Print( 0, "$Descr %s %d %d%s\n", TO_UTF8( page.GetType() ),
                  page.GetWidthMils(),
                  page.GetHeightMils(),
                  !page.IsCustom() && page.IsPortrait() ? " portrait" : "" );
    m_out->Print( 0, "encoding utf-8\n" );
    m_out->Print( 0, "Sheet %d %d\n", aScreen->m_ScreenNumber, aScreen->m_NumberOfScreens );
    m_out->Print( 0, "Title %s\n",    EscapedUTF8( tb.GetTitle() ).c_str() );
    m_out->Print( 0, "Date %s\n",     EscapedUTF8( tb.GetDate() ).c_str() );
    m_out->Print( 0, "Rev %s\n",      EscapedUTF8( tb.GetRevision() ).c_str() );
    m_out->Print( 0, "Comp %s\n",     EscapedUTF8( tb.GetCompany() ).c_str() );
    m_out->Print( 0, "Comment1 %s\n", EscapedUTF8( tb.GetComment1() ).c_str() );
    m_out->Print( 0, "Comment2 %s\n", EscapedUTF8( tb.GetComment2() ).c_str() );
    m_out->Print( 0, "Comment3 %s\n", EscapedUTF8( tb.GetComment3() ).c_str() );
    m_out->Print( 0, "Comment4 %s\n", EscapedUTF8( tb.GetComment4() ).c_str() );
    m_out->Print( 0, "$EndDescr\n" );

    for( auto alias : aScreen->GetBusAliases() )
    {
        saveBusAlias( alias );
    }

    for( SCH_ITEM* item = aScreen->GetDrawItems(); item; item = item->Next() )
    {
        switch( item->Type() )
        {
        case SCH_COMPONENT_T:
            saveComponent( static_cast< SCH_COMPONENT* >( item ) );
            break;
        case SCH_BITMAP_T:
            saveBitmap( static_cast< SCH_BITMAP* >( item ) );
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
            wxASSERT( "Unexpected schematic object type in SCH_LEGACY_PLUGIN::Format()" );
        }
    }

    m_out->Print( 0, "$EndSCHEMATC\n" );
}


void SCH_LEGACY_PLUGIN::Format( SELECTION* aSelection, OUTPUTFORMATTER* aFormatter )
{
    m_out = aFormatter;

    for( unsigned i = 0; i < aSelection->GetSize(); ++i )
    {
        SCH_ITEM* item = (SCH_ITEM*) aSelection->GetItem( i );

        switch( item->Type() )
        {
        case SCH_COMPONENT_T:
            saveComponent( static_cast< SCH_COMPONENT* >( item ) );
            break;
        case SCH_BITMAP_T:
            saveBitmap( static_cast< SCH_BITMAP* >( item ) );
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
            wxASSERT( "Unexpected schematic object type in SCH_LEGACY_PLUGIN::Format()" );
        }
    }
}


void SCH_LEGACY_PLUGIN::saveComponent( SCH_COMPONENT* aComponent )
{
    std::string     name1;
    std::string     name2;
    wxArrayString   reference_fields;

    static wxString delimiters( wxT( " " ) );

    // This is redundant with the AR entries below, but it makes the files backwards-compatible.
    if( aComponent->GetPathsAndReferences().GetCount() > 0 )
    {
        reference_fields = wxStringTokenize( aComponent->GetPathsAndReferences()[0], delimiters );
        name1 = toUTFTildaText( reference_fields[1] );
    }
    else
    {
        if( aComponent->GetField( REFERENCE )->GetText().IsEmpty() )
            name1 = toUTFTildaText( aComponent->GetPrefix() );
        else
            name1 = toUTFTildaText( aComponent->GetField( REFERENCE )->GetText() );
    }

    wxString part_name = aComponent->GetLibId().Format();

    if( part_name.size() )
    {
        name2 = toUTFTildaText( part_name );
    }
    else
    {
        name2 = "_NONAME_";
    }

    m_out->Print( 0, "$Comp\n" );
    m_out->Print( 0, "L %s %s\n", name2.c_str(), name1.c_str() );

    // Generate unit number, convert and time stamp
    m_out->Print( 0, "U %d %d %8.8X\n", aComponent->GetUnit(), aComponent->GetConvert(),
                  aComponent->GetTimeStamp() );

    // Save the position
    m_out->Print( 0, "P %d %d\n", aComponent->GetPosition().x, aComponent->GetPosition().y );

    /* If this is a complex hierarchy; save hierarchical references.
     * but for simple hierarchies it is not necessary.
     * the reference inf is already saved
     * this is useful for old Eeschema version compatibility
     */
    if( aComponent->GetPathsAndReferences().GetCount() > 1 )
    {
        for( unsigned int ii = 0; ii <  aComponent->GetPathsAndReferences().GetCount(); ii++ )
        {
            /*format:
             * AR Path="/140/2" Ref="C99"   Part="1"
             * where 140 is the uid of the containing sheet
             * and 2 is the timestamp of this component.
             * (timestamps are actually 8 hex chars)
             * Ref is the conventional component reference for this 'path'
             * Part is the conventional component part selection for this 'path'
             */
            reference_fields = wxStringTokenize( aComponent->GetPathsAndReferences()[ii],
                                                 delimiters );

            m_out->Print( 0, "AR Path=\"%s\" Ref=\"%s\"  Part=\"%s\" \n",
                          TO_UTF8( reference_fields[0] ),
                          TO_UTF8( reference_fields[1] ),
                          TO_UTF8( reference_fields[2] ) );
        }
    }

    // update the ugly field index, which I would like to see go away someday soon.
    for( int i = 0;  i < aComponent->GetFieldCount();  ++i )
        aComponent->GetField( i )->SetId( i );

    // Fixed fields:
    // Save mandatory fields even if they are blank,
    // because the visibility, size and orientation are set from libary editor.
    for( unsigned i = 0;  i < MANDATORY_FIELDS;  ++i )
        saveField( aComponent->GetField( i ) );

    // User defined fields:
    // The *policy* about which user defined fields are part of a symbol is now
    // only in the dialog editors.  No policy should be enforced here, simply
    // save all the user defined fields, they are present because a dialog editor
    // thought they should be.  If you disagree, go fix the dialog editors.
    for( int i = MANDATORY_FIELDS;  i < aComponent->GetFieldCount();  ++i )
        saveField( aComponent->GetField( i ) );

    // Unit number, position, box ( old standard )
    m_out->Print( 0, "\t%-4d %-4d %-4d\n", aComponent->GetUnit(), aComponent->GetPosition().x,
                  aComponent->GetPosition().y );

    TRANSFORM transform = aComponent->GetTransform();

    m_out->Print( 0, "\t%-4d %-4d %-4d %-4d\n",
                  transform.x1, transform.y1, transform.x2, transform.y2 );
    m_out->Print( 0, "$EndComp\n" );
}


void SCH_LEGACY_PLUGIN::saveField( SCH_FIELD* aField )
{
    char hjustify = 'C';

    if( aField->GetHorizJustify() == GR_TEXT_HJUSTIFY_LEFT )
        hjustify = 'L';
    else if( aField->GetHorizJustify() == GR_TEXT_HJUSTIFY_RIGHT )
        hjustify = 'R';

    char vjustify = 'C';

    if( aField->GetVertJustify() == GR_TEXT_VJUSTIFY_BOTTOM )
        vjustify = 'B';
    else if( aField->GetVertJustify() == GR_TEXT_VJUSTIFY_TOP )
        vjustify = 'T';

    m_out->Print( 0, "F %d %s %c %-3d %-3d %-3d %4.4X %c %c%c%c",
                  aField->GetId(),
                  EscapedUTF8( aField->GetText() ).c_str(),     // wraps in quotes too
                  aField->GetTextAngle() == TEXT_ANGLE_HORIZ ? 'H' : 'V',
                  aField->GetLibPosition().x, aField->GetLibPosition().y,
                  aField->GetTextWidth(),
                  !aField->IsVisible(),
                  hjustify, vjustify,
                  aField->IsItalic() ? 'I' : 'N',
                  aField->IsBold() ? 'B' : 'N' );

    // Save field name, if the name is user definable
    if( aField->GetId() >= FIELD1 )
    {
        m_out->Print( 0, " %s", EscapedUTF8( aField->GetName() ).c_str() );
    }

    m_out->Print( 0, "\n" );
}


void SCH_LEGACY_PLUGIN::saveBitmap( SCH_BITMAP* aBitmap )
{
    wxCHECK_RET( aBitmap != NULL, "SCH_BITMAP* is NULL" );

    const wxImage* image = aBitmap->GetImage()->GetImageData();

    wxCHECK_RET( image != NULL, "wxImage* is NULL" );

    m_out->Print( 0, "$Bitmap\n" );
    m_out->Print( 0, "Pos %-4d %-4d\n", aBitmap->GetPosition().x, aBitmap->GetPosition().y );
    m_out->Print( 0, "Scale %f\n", aBitmap->GetImage()->GetScale() );
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


void SCH_LEGACY_PLUGIN::saveSheet( SCH_SHEET* aSheet )
{
    wxCHECK_RET( aSheet != NULL, "SCH_SHEET* is NULL" );

    m_out->Print( 0, "$Sheet\n" );
    m_out->Print( 0, "S %-4d %-4d %-4d %-4d\n",
                  aSheet->GetPosition().x, aSheet->GetPosition().y,
                  aSheet->GetSize().x, aSheet->GetSize().y );

    m_out->Print( 0, "U %8.8X\n", aSheet->GetTimeStamp() );

    if( !aSheet->GetName().IsEmpty() )
        m_out->Print( 0, "F0 %s %d\n", EscapedUTF8( aSheet->GetName() ).c_str(),
                      aSheet->GetSheetNameSize() );

    if( !aSheet->GetFileName().IsEmpty() )
        m_out->Print( 0, "F1 %s %d\n", EscapedUTF8( aSheet->GetFileName() ).c_str(),
                      aSheet->GetFileNameSize() );

    for( const SCH_SHEET_PIN& pin : aSheet->GetPins() )
    {
        int type, side;

        if( pin.GetText().IsEmpty() )
            break;

        switch( pin.GetEdge() )
        {
        default:
        case SHEET_LEFT_SIDE:   side = 'L'; break;
        case SHEET_RIGHT_SIDE:  side = 'R'; break;
        case SHEET_TOP_SIDE:    side = 'T'; break;
        case SHEET_BOTTOM_SIDE: side = 'B'; break;
        }

        switch( pin.GetShape() )
        {
        case NET_INPUT:       type = 'I'; break;
        case NET_OUTPUT:      type = 'O'; break;
        case NET_BIDI:        type = 'B'; break;
        case NET_TRISTATE:    type = 'T'; break;
        default:
        case NET_UNSPECIFIED: type = 'U'; break;
        }

        m_out->Print( 0, "F%d %s %c %c %-3d %-3d %-3d\n", pin.GetNumber(),
                      EscapedUTF8( pin.GetText() ).c_str(),     // supplies wrapping quotes
                      type, side, pin.GetPosition().x, pin.GetPosition().y,
                      pin.GetTextWidth() );
    }

    m_out->Print( 0, "$EndSheet\n" );
}


void SCH_LEGACY_PLUGIN::saveJunction( SCH_JUNCTION* aJunction )
{
    wxCHECK_RET( aJunction != NULL, "SCH_JUNCTION* is NULL" );

    m_out->Print( 0, "Connection ~ %-4d %-4d\n",
                  aJunction->GetPosition().x, aJunction->GetPosition().y );
}


void SCH_LEGACY_PLUGIN::saveNoConnect( SCH_NO_CONNECT* aNoConnect )
{
    wxCHECK_RET( aNoConnect != NULL, "SCH_NOCONNECT* is NULL" );

    m_out->Print( 0, "NoConn ~ %-4d %-4d\n", aNoConnect->GetPosition().x,
                  aNoConnect->GetPosition().y );
}


void SCH_LEGACY_PLUGIN::saveBusEntry( SCH_BUS_ENTRY_BASE* aBusEntry )
{
    wxCHECK_RET( aBusEntry != NULL, "SCH_BUS_ENTRY_BASE* is NULL" );

    if( aBusEntry->GetLayer() == LAYER_WIRE )
        m_out->Print( 0, "Entry Wire Line\n\t%-4d %-4d %-4d %-4d\n",
                      aBusEntry->GetPosition().x, aBusEntry->GetPosition().y,
                      aBusEntry->m_End().x, aBusEntry->m_End().y );
    else
        m_out->Print( 0, "Entry Bus Bus\n\t%-4d %-4d %-4d %-4d\n",
                      aBusEntry->GetPosition().x, aBusEntry->GetPosition().y,
                      aBusEntry->m_End().x, aBusEntry->m_End().y );
}


void SCH_LEGACY_PLUGIN::saveLine( SCH_LINE* aLine )
{
    wxCHECK_RET( aLine != NULL, "SCH_LINE* is NULL" );

    const char* layer = "Notes";
    const char* width = "Line";

    if( aLine->GetLayer() == LAYER_WIRE )
        layer = "Wire";
    else if( aLine->GetLayer() == LAYER_BUS )
        layer = "Bus";

    m_out->Print( 0, "Wire %s %s", layer, width );

    // Write line style (width, type, color) only for non default values
    if( aLine->GetLayer() == LAYER_NOTES )
    {
        if( aLine->GetPenSize() != aLine->GetDefaultWidth() )
            m_out->Print( 0, " %s %d", T_WIDTH, aLine->GetLineSize() );

        if( aLine->GetLineStyle() != aLine->GetDefaultStyle() )
            m_out->Print( 0, " %s %s", T_STYLE, SCH_LINE::GetLineStyleName( aLine->GetLineStyle() ) );

        if( aLine->GetLineColor() != aLine->GetDefaultColor() )
            m_out->Print( 0, " %s",
                TO_UTF8( aLine->GetLineColor().ToColour().GetAsString( wxC2S_CSS_SYNTAX ) ) );
    }

    m_out->Print( 0, "\n" );

    m_out->Print( 0, "\t%-4d %-4d %-4d %-4d",
                  aLine->GetStartPoint().x, aLine->GetStartPoint().y,
                  aLine->GetEndPoint().x, aLine->GetEndPoint().y );

    m_out->Print( 0, "\n");
}


void SCH_LEGACY_PLUGIN::saveText( SCH_TEXT* aText )
{
    wxCHECK_RET( aText != NULL, "SCH_TEXT* is NULL" );

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

        m_out->Print( 0, "Text %s %-4d %-4d %-4d %-4d %s %d\n%s\n", textType,
                      aText->GetPosition().x, aText->GetPosition().y,
                      aText->GetLabelSpinStyle(),
                      aText->GetTextWidth(),
                      italics, aText->GetThickness(), TO_UTF8( text ) );
    }
    else if( layer == LAYER_GLOBLABEL || layer == LAYER_HIERLABEL )
    {
        textType = ( layer == LAYER_GLOBLABEL ) ? "GLabel" : "HLabel";

        m_out->Print( 0, "Text %s %-4d %-4d %-4d %-4d %s %s %d\n%s\n", textType,
                      aText->GetPosition().x, aText->GetPosition().y,
                      aText->GetLabelSpinStyle(),
                      aText->GetTextWidth(),
                      SheetLabelType[aText->GetShape()],
                      italics,
                      aText->GetThickness(), TO_UTF8( text ) );
    }
}


void SCH_LEGACY_PLUGIN::saveBusAlias( std::shared_ptr<BUS_ALIAS> aAlias )
{
    wxCHECK_RET( aAlias != NULL, "BUS_ALIAS* is NULL" );

    wxString members = boost::algorithm::join( aAlias->Members(), " " );

    m_out->Print( 0, "BusAlias %s %s\n",
                  TO_UTF8( aAlias->GetName() ), TO_UTF8( members ) );
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
    m_libType = LIBRARY_TYPE_EESCHEMA;
}


SCH_LEGACY_PLUGIN_CACHE::~SCH_LEGACY_PLUGIN_CACHE()
{
    // When the cache is destroyed, all of the alias objects on the heap should be deleted.
    for( LIB_ALIAS_MAP::iterator it = m_aliases.begin();  it != m_aliases.end();  ++it )
    {
        wxLogTrace( traceSchLegacyPlugin, wxT( "Removing alias %s from library %s." ),
                    GetChars( it->second->GetName() ), GetChars( GetLogicalName() ) );
        LIB_PART* part = it->second->GetPart();
        LIB_ALIAS* alias = it->second;
        delete alias;

        // When the last alias of a part is destroyed, the part is no longer required and it
        // too is destroyed.
        if( part && part->GetAliasCount() == 0 )
            delete part;
    }

    m_aliases.clear();
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


LIB_ALIAS* SCH_LEGACY_PLUGIN_CACHE::removeAlias( LIB_ALIAS* aAlias )
{
    wxCHECK_MSG( aAlias != NULL, NULL, "NULL pointer cannot be removed from library." );

    LIB_ALIAS_MAP::iterator it = m_aliases.find( aAlias->GetName() );

    if( it == m_aliases.end() )
        return NULL;

    // If the entry pointer doesn't match the name it is mapped to in the library, we
    // have done something terribly wrong.
    wxCHECK_MSG( *it->second == aAlias, NULL,
                 "Pointer mismatch while attempting to remove alias entry <" + aAlias->GetName() +
                 "> from library cache <" + m_libFileName.GetName() + ">." );

    LIB_ALIAS*  alias = aAlias;
    LIB_PART*   part = alias->GetPart();

    alias = part->RemoveAlias( alias );

    if( !alias )
    {
        delete part;

        if( m_aliases.size() > 1 )
        {
            LIB_ALIAS_MAP::iterator next = it;
            next++;

            if( next == m_aliases.end() )
                next = m_aliases.begin();

            alias = next->second;
        }
    }

    m_aliases.erase( it );
    m_isModified = true;
    ++m_modHash;
    return alias;
}


void SCH_LEGACY_PLUGIN_CACHE::AddSymbol( const LIB_PART* aPart )
{
    // aPart is cloned in PART_LIB::AddPart().  The cache takes ownership of aPart.
    wxArrayString aliasNames = aPart->GetAliasNames();

    for( size_t i = 0; i < aliasNames.size(); i++ )
    {
        LIB_ALIAS_MAP::iterator it = m_aliases.find( aliasNames[i] );

        if( it != m_aliases.end() )
            removeAlias( it->second );

        LIB_ALIAS* alias = const_cast< LIB_PART* >( aPart )->GetAlias( aliasNames[i] );

        wxASSERT_MSG( alias != NULL, "No alias <" + aliasNames[i] + "> found in symbol <" +
                      aPart->GetName() +">." );

        m_aliases[ aliasNames[i] ] = alias;
    }

    m_isModified = true;
    ++m_modHash;
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
        m_libType = LIBRARY_TYPE_SYMBOL;

        /// @todo Probably should check for a valid date and time stamp even though it's not used.
    }
    else
    {
        m_libType = LIBRARY_TYPE_EESCHEMA;
    }

    while( reader.ReadLine() )
    {
        line = reader.Line();

        if( *line == '#' || isspace( *line ) )  // Skip comments and blank lines.
            continue;

        // Headers where only supported in older library file formats.
        if( m_libType == LIBRARY_TYPE_EESCHEMA && strCompare( "$HEADER", line ) )
            loadHeader( reader );

        if( strCompare( "DEF", line ) )
        {
            // Read one DEF/ENDDEF part entry from library:
            LIB_PART * part = LoadPart( reader, m_versionMajor, m_versionMinor );

            // Add aliases to cache
            for( size_t ii = 0; ii < part->GetAliasCount(); ++ii )
            {
                LIB_ALIAS* alias = part->GetAlias( ii );
                const wxString& aliasName = alias->GetName();

                // This section seems to do a similar job as checkForDuplicates, so
                // I'm not sure checkForDuplicates needs to be preserved.
                auto it = m_aliases.find( aliasName );

                if( it != m_aliases.end() )
                {
                    // Find a new name for the alias
                    wxString newName;
                    int idx = 0;
                    LIB_ALIAS_MAP::const_iterator jt;

                    do
                    {
                        newName = wxString::Format( "%s_%d", aliasName, idx );
                        jt = m_aliases.find( newName );
                        ++idx;
                    }
                    while( jt != m_aliases.end() );

                    wxLogWarning( "Symbol name conflict in library:\n%s\n"
                                  "'%s' has been renamed to '%s'",
                                  m_fileName, aliasName, newName );

                    if( alias->IsRoot() )
                        part->SetName( newName );
                    else
                        alias->SetName( newName );

                    m_aliases[newName] = alias;
                }
                else
                {
                    m_aliases[aliasName] = alias;
                }
            }
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
    LIB_ALIAS*  alias = NULL;;

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
        aliasName = LIB_ID::FixIllegalChars( aliasName, LIB_ID::ID_SCH );

        LIB_ALIAS_MAP::iterator it = m_aliases.find( aliasName );

        if( it == m_aliases.end() )
            wxLogWarning( "Alias '%s' not found in library:\n\n"
                          "'%s'\n\nat line %d offset %d", aliasName, fn.GetFullPath(),
                          reader.LineNumber(), (int) (line - reader.Line() ) );
        else
            alias = it->second;

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
                if( alias )
                    alias->SetDescription( text );
                break;

            case 'K':
                if( alias )
                    alias->SetKeyWords( text );
                break;

            case 'F':
                if( alias )
                    alias->SetDocFileName( text );
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
                                             int aMinorVersion )
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
    std::unique_ptr< LIB_PART > part( new LIB_PART( wxEmptyString ) );

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
    part->SetPinNameOffset( (int)num );

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

    // There are some code paths in SetText() that do not set the root alias to the
    // alias list so add it here if it didn't get added by SetText().
    if( !part->HasAlias( part->GetName() ) )
        part->AddAlias( part->GetName() );

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
        if( *line == '#' )                               // Comment
            ;
        else if( strCompare( "Ti", line, &line ) )       // Modification date is ignored.
            continue;
        else if( strCompare( "ALIAS", line, &line ) )    // Aliases
            loadAliases( part, aReader );
        else if( *line == 'F' )                          // Fields
            loadField( part, aReader );
        else if( strCompare( "DRAW", line, &line ) )     // Drawing objects.
            loadDrawEntries( part, aReader, aMajorVersion, aMinorVersion );
        else if( strCompare( "$FPLIST", line, &line ) )  // Footprint filter list
            loadFootprintFilters( part, aReader );
        else if( strCompare( "ENDDEF", line, &line ) )   // End of part description
        {
            return part.release();
        }

        line = aReader.ReadLine();
    }

    SCH_PARSE_ERROR( "missing ENDDEF", aReader, line );
}


#if 0
bool SCH_LEGACY_PLUGIN_CACHE::checkForDuplicates( wxString& aAliasName )
{
    wxCHECK_MSG( !aAliasName.IsEmpty(), false, "alias name cannot be empty" );

    // The alias name is not a duplicate so don't change it.
    if( m_aliases.find( aAliasName ) == m_aliases.end() )
        return false;

    int dupCounter = 1;
    wxString newAlias = aAliasName;

    // If the alias is already loaded, the library is broken.  It may have been possible in
    // the past that this could happen so we assign a new alias name to prevent any conflicts
    // rather than throw an exception.
    while( m_aliases.find( newAlias ) != m_aliases.end() )
    {
        newAlias = aAliasName << dupCounter;
        dupCounter++;
    }

    aAliasName = newAlias;

    return true;
}
#endif


void SCH_LEGACY_PLUGIN_CACHE::loadAliases( std::unique_ptr<LIB_PART>& aPart,
                                           LINE_READER&                 aReader )
{
    wxString newAlias;
    const char* line = aReader.Line();

    wxCHECK_RET( strCompare( "ALIAS", line, &line ), "Invalid ALIAS section" );

    wxString utf8Line = wxString::FromUTF8( line );
    wxStringTokenizer tokens( utf8Line, " \r\n\t" );

    // Parse the ALIAS list.
    while( tokens.HasMoreTokens() )
    {
        newAlias = tokens.GetNextToken();
        aPart->AddAlias( newAlias );
    }
}


void SCH_LEGACY_PLUGIN_CACHE::loadField( std::unique_ptr<LIB_PART>& aPart,
                                         LINE_READER&                 aReader )
{
    const char* line = aReader.Line();

    wxCHECK_RET( *line == 'F', "Invalid field line" );

    wxString    text;
    int         id;

    if( sscanf( line + 1, "%d", &id ) != 1 || id < 0 )
        SCH_PARSE_ERROR( "invalid field ID", aReader, line + 1 );

    LIB_FIELD* field;

    if( (unsigned) id < MANDATORY_FIELDS )
    {
        field = aPart->GetField( id );

        // this will fire only if somebody broke a constructor or editor.
        // MANDATORY_FIELDS are always present in ram resident components, no
        // exceptions, and they always have their names set, even fixed fields.
        wxASSERT( field );
    }
    else
    {
        field = new LIB_FIELD( aPart.get(), id );
        aPart->AddDrawItem( field );
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

    pos.x = parseInt( aReader, line, &line );
    pos.y = parseInt( aReader, line, &line );
    field->SetPosition( pos );

    wxSize textSize;

    textSize.x = textSize.y = parseInt( aReader, line, &line );
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
    if( (unsigned) id < MANDATORY_FIELDS )
    {
        // Fields in RAM must always have names, because we are trying to get
        // less dependent on field ids and more dependent on names.
        // Plus assumptions are made in the field editors.
        field->m_name = TEMPLATE_FIELDNAME::GetDefaultFieldName( id );

        // Ensure the VALUE field = the part name (can be not the case
        // with malformed libraries: edited by hand, or converted from other tools)
        if( id == VALUE )
            field->SetText( aPart->GetName() );
    }
    else
    {
        parseQuotedString( field->m_name, aReader, line, &line, true );  // Optional.
    }
}


void SCH_LEGACY_PLUGIN_CACHE::loadDrawEntries( std::unique_ptr<LIB_PART>& aPart,
                                               LINE_READER&                 aReader,
                                               int                          aMajorVersion,
                                               int                          aMinorVersion )
{
    const char* line = aReader.Line();

    wxCHECK_RET( strCompare( "DRAW", line, &line ), "Invalid DRAW section" );

    line = aReader.ReadLine();

    while( line )
    {
        if( strCompare( "ENDDRAW", line, &line ) )
            return;

        switch( line[0] )
        {
        case 'A':    // Arc
            aPart->AddDrawItem( loadArc( aPart, aReader ) );
            break;

        case 'C':    // Circle
            aPart->AddDrawItem( loadCircle( aPart, aReader ) );
            break;

        case 'T':    // Text
            aPart->AddDrawItem( loadText( aPart, aReader, aMajorVersion, aMinorVersion ) );
            break;

        case 'S':    // Square
            aPart->AddDrawItem( loadRectangle( aPart, aReader ) );
            break;

        case 'X':    // Pin Description
            aPart->AddDrawItem( loadPin( aPart, aReader ) );
            break;

        case 'P':    // Polyline
            aPart->AddDrawItem( loadPolyLine( aPart, aReader ) );
            break;

        case 'B':    // Bezier Curves
            aPart->AddDrawItem( loadBezier( aPart, aReader ) );
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


FILL_T SCH_LEGACY_PLUGIN_CACHE::parseFillMode( LINE_READER& aReader, const char* aLine,
                                               const char** aOutput )
{
    switch( parseChar( aReader, aLine, aOutput ) )
    {
    case 'F': return FILLED_SHAPE;
    case 'f': return FILLED_WITH_BG_BODYCOLOR;
    case 'N': return NO_FILL;
    default: SCH_PARSE_ERROR( "invalid fill type, expected f, F, or N", aReader, aLine );
    }
}


LIB_ARC* SCH_LEGACY_PLUGIN_CACHE::loadArc( std::unique_ptr<LIB_PART>& aPart,
                                           LINE_READER&                 aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "A", line, &line ), NULL, "Invalid LIB_ARC definition" );

    LIB_ARC* arc = new LIB_ARC( aPart.get() );

    wxPoint center;

    center.x = parseInt( aReader, line, &line );
    center.y = parseInt( aReader, line, &line );

    arc->SetPosition( center );
    arc->SetRadius( parseInt( aReader, line, &line ) );

    int angle1 = parseInt( aReader, line, &line );
    int angle2 = parseInt( aReader, line, &line );

    NORMALIZE_ANGLE_POS( angle1 );
    NORMALIZE_ANGLE_POS( angle2 );
    arc->SetFirstRadiusAngle( angle1 );
    arc->SetSecondRadiusAngle( angle2 );

    arc->SetUnit( parseInt( aReader, line, &line ) );
    arc->SetConvert( parseInt( aReader, line, &line ) );
    arc->SetWidth( parseInt( aReader, line, &line ) );

    // Old libraries (version <= 2.2) do not have always this FILL MODE param
    // when fill mode is no fill (default mode).
    if( *line != 0 )
        arc->SetFillMode( parseFillMode( aReader, line, &line ) );

    // Actual Coordinates of arc ends are read from file
    if( *line != 0 )
    {
        wxPoint arcStart, arcEnd;

        arcStart.x = parseInt( aReader, line, &line );
        arcStart.y = parseInt( aReader, line, &line );
        arcEnd.x = parseInt( aReader, line, &line );
        arcEnd.y = parseInt( aReader, line, &line );

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
                                                 LINE_READER&                 aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "C", line, &line ), NULL, "Invalid LIB_CIRCLE definition" );

    LIB_CIRCLE* circle = new LIB_CIRCLE( aPart.get() );

    wxPoint center;

    center.x = parseInt( aReader, line, &line );
    center.y = parseInt( aReader, line, &line );

    circle->SetPosition( center );
    circle->SetRadius( parseInt( aReader, line, &line ) );
    circle->SetUnit( parseInt( aReader, line, &line ) );
    circle->SetConvert( parseInt( aReader, line, &line ) );
    circle->SetWidth( parseInt( aReader, line, &line ) );

    if( *line != 0 )
        circle->SetFillMode( parseFillMode( aReader, line, &line ) );

    return circle;
}


LIB_TEXT* SCH_LEGACY_PLUGIN_CACHE::loadText( std::unique_ptr<LIB_PART>& aPart,
                                             LINE_READER&                 aReader,
                                             int                          aMajorVersion,
                                             int                          aMinorVersion )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "T", line, &line ), NULL, "Invalid LIB_TEXT definition" );

    LIB_TEXT* text = new LIB_TEXT( aPart.get() );

    text->SetTextAngle( (double) parseInt( aReader, line, &line ) );

    wxPoint center;

    center.x = parseInt( aReader, line, &line );
    center.y = parseInt( aReader, line, &line );
    text->SetPosition( center );

    wxSize size;

    size.x = size.y = parseInt( aReader, line, &line );
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
                                                       LINE_READER&                 aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "S", line, &line ), NULL, "Invalid LIB_RECTANGLE definition" );

    LIB_RECTANGLE* rectangle = new LIB_RECTANGLE( aPart.get() );

    wxPoint pos;

    pos.x = parseInt( aReader, line, &line );
    pos.y = parseInt( aReader, line, &line );
    rectangle->SetPosition( pos );

    wxPoint end;

    end.x = parseInt( aReader, line, &line );
    end.y = parseInt( aReader, line, &line );
    rectangle->SetEnd( end );

    rectangle->SetUnit( parseInt( aReader, line, &line ) );
    rectangle->SetConvert( parseInt( aReader, line, &line ) );
    rectangle->SetWidth( parseInt( aReader, line, &line ) );

    if( *line != 0 )
        rectangle->SetFillMode( parseFillMode( aReader, line, &line ) );

    return rectangle;
}


LIB_PIN* SCH_LEGACY_PLUGIN_CACHE::loadPin( std::unique_ptr<LIB_PART>& aPart,
                                           LINE_READER&                 aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "X", line, &line ), NULL, "Invalid LIB_PIN definition" );

    LIB_PIN* pin = new LIB_PIN( aPart.get() );

    size_t pos = 2;                               // "X" plus ' ' space character.
    wxString tmp;
    wxString utf8Line = wxString::FromUTF8( line );
    wxStringTokenizer tokens( utf8Line, " \r\n\t" );

    if( tokens.CountTokens() < 11 )
        SCH_PARSE_ERROR( "invalid pin definition", aReader, line );

    pin->m_name = tokens.GetNextToken();
    pos += pin->m_name.size() + 1;
    pin->m_number = tokens.GetNextToken();
    pos += pin->m_number.size() + 1;

    long num;
    wxPoint position;

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin X coordinate", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    position.x = (int) num;

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin Y coordinate", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    position.y = (int) num;
    pin->m_position = position;

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin length", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    pin->m_length = (int) num;


    tmp = tokens.GetNextToken();

    if( tmp.size() > 1 )
        THROW_PARSE_ERROR( "invalid pin orientation", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    pin->m_orientation = tmp[0];

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin number text size", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    pin->m_numTextSize = (int) num;

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin name text size", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    pin->m_nameTextSize = (int) num;

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin unit", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    pin->m_Unit = (int) num;

    tmp = tokens.GetNextToken();

    if( !tmp.ToLong( &num ) )
        THROW_PARSE_ERROR( "invalid pin alternate body type", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    pin->m_Convert = (int) num;

    tmp = tokens.GetNextToken();

    if( tmp.size() != 1 )
        THROW_PARSE_ERROR( "invalid pin type", aReader.GetSource(), aReader.Line(),
                           aReader.LineNumber(), pos );

    pos += tmp.size() + 1;
    char type = tmp[0];

    wxString attributes;

    switch( type )
    {
    case 'I': pin->m_type = PIN_INPUT;         break;
    case 'O': pin->m_type = PIN_OUTPUT;        break;
    case 'B': pin->m_type = PIN_BIDI;          break;
    case 'T': pin->m_type = PIN_TRISTATE;      break;
    case 'P': pin->m_type = PIN_PASSIVE;       break;
    case 'U': pin->m_type = PIN_UNSPECIFIED;   break;
    case 'W': pin->m_type = PIN_POWER_IN;      break;
    case 'w': pin->m_type = PIN_POWER_OUT;     break;
    case 'C': pin->m_type = PIN_OPENCOLLECTOR; break;
    case 'E': pin->m_type = PIN_OPENEMITTER;   break;
    case 'N': pin->m_type = PIN_NC;            break;
    default: THROW_PARSE_ERROR( "unknown pin type", aReader.GetSource(),
                                aReader.Line(), aReader.LineNumber(), pos );
    }

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
            case 'N': pin->m_attributes |= PIN_INVISIBLE; break;
            case 'I': flags |= INVERTED;     break;
            case 'C': flags |= CLOCK;        break;
            case 'L': flags |= LOWLEVEL_IN;  break;
            case 'V': flags |= LOWLEVEL_OUT; break;
            case 'F': flags |= FALLING_EDGE; break;
            case 'X': flags |= NONLOGIC;     break;
            default: THROW_PARSE_ERROR( "invalid pin attribut", aReader.GetSource(),
                                        aReader.Line(), aReader.LineNumber(), pos );
            }

            pos += 1;
        }

        switch( flags )
        {
        case 0:                   pin->m_shape = PINSHAPE_LINE;               break;
        case INVERTED:            pin->m_shape = PINSHAPE_INVERTED;           break;
        case CLOCK:               pin->m_shape = PINSHAPE_CLOCK;              break;
        case INVERTED | CLOCK:    pin->m_shape = PINSHAPE_INVERTED_CLOCK;     break;
        case LOWLEVEL_IN:         pin->m_shape = PINSHAPE_INPUT_LOW;          break;
        case LOWLEVEL_IN | CLOCK: pin->m_shape = PINSHAPE_CLOCK_LOW;          break;
        case LOWLEVEL_OUT:        pin->m_shape = PINSHAPE_OUTPUT_LOW;         break;
        case FALLING_EDGE:        pin->m_shape = PINSHAPE_FALLING_EDGE_CLOCK; break;
        case NONLOGIC:            pin->m_shape = PINSHAPE_NONLOGIC;           break;
        default: SCH_PARSE_ERROR( "pin attributes do not define a valid pin shape", aReader, line );
        }
    }

    return pin;
}


LIB_POLYLINE* SCH_LEGACY_PLUGIN_CACHE::loadPolyLine( std::unique_ptr<LIB_PART>& aPart,
                                                     LINE_READER&                 aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "P", line, &line ), NULL, "Invalid LIB_POLYLINE definition" );

    LIB_POLYLINE* polyLine = new LIB_POLYLINE( aPart.get() );

    int points = parseInt( aReader, line, &line );
    polyLine->SetUnit( parseInt( aReader, line, &line ) );
    polyLine->SetConvert( parseInt( aReader, line, &line ) );
    polyLine->SetWidth( parseInt( aReader, line, &line ) );
    polyLine->Reserve( points );

    wxPoint pt;

    for( int i = 0; i < points; i++ )
    {
        pt.x = parseInt( aReader, line, &line );
        pt.y = parseInt( aReader, line, &line );
        polyLine->AddPoint( pt );
    }

    if( *line != 0 )
        polyLine->SetFillMode( parseFillMode( aReader, line, &line ) );

    return polyLine;
}


LIB_BEZIER* SCH_LEGACY_PLUGIN_CACHE::loadBezier( std::unique_ptr<LIB_PART>& aPart,
                                                 LINE_READER&                 aReader )
{
    const char* line = aReader.Line();

    wxCHECK_MSG( strCompare( "B", line, &line ), NULL, "Invalid LIB_BEZIER definition" );

    LIB_BEZIER* bezier = new LIB_BEZIER( aPart.get() );

    int points = parseInt( aReader, line, &line );
    bezier->SetUnit( parseInt( aReader, line, &line ) );
    bezier->SetConvert( parseInt( aReader, line, &line ) );
    bezier->SetWidth( parseInt( aReader, line, &line ) );

    wxPoint pt;
    bezier->Reserve( points );

    for( int i = 0; i < points; i++ )
    {
        pt.x = parseInt( aReader, line, &line );
        pt.y = parseInt( aReader, line, &line );
        bezier->AddPoint( pt );
    }

    if( *line != 0 )
        bezier->SetFillMode( parseFillMode( aReader, line, &line ) );

    return bezier;
}


void SCH_LEGACY_PLUGIN_CACHE::loadFootprintFilters( std::unique_ptr<LIB_PART>& aPart,
                                                    LINE_READER&                 aReader )
{
    const char* line = aReader.Line();

    wxCHECK_RET( strCompare( "$FPLIST", line, &line ), "Invalid footprint filter list" );

    line = aReader.ReadLine();

    while( line )
    {
        if( strCompare( "$ENDFPLIST", line, &line ) )
            return;

        wxString footprint;

        parseUnquotedString( footprint, aReader, line, &line );
        aPart->GetFootprints().Add( footprint );
        line = aReader.ReadLine();
    }

    SCH_PARSE_ERROR( "file ended prematurely while loading footprint filters", aReader, line );
}


void SCH_LEGACY_PLUGIN_CACHE::Save( bool aSaveDocFile )
{
    if( !m_isModified )
        return;

    // Write through symlinks, don't replace them
    wxFileName fn = GetRealFile();

    std::unique_ptr< FILE_OUTPUTFORMATTER > formatter( new FILE_OUTPUTFORMATTER( fn.GetFullPath() ) );
    formatter->Print( 0, "%s %d.%d\n", LIBFILE_IDENT, LIB_VERSION_MAJOR, LIB_VERSION_MINOR );
    formatter->Print( 0, "#encoding utf-8\n");

    for( LIB_ALIAS_MAP::iterator it = m_aliases.begin();  it != m_aliases.end();  it++ )
    {
        if( !it->second->IsRoot() )
            continue;

        SaveSymbol( it->second->GetPart(), *formatter.get() );
    }

    formatter->Print( 0, "#\n#End Library\n" );
    formatter.reset();

    m_fileModTime = fn.GetModificationTime();
    m_isModified = false;

    if( aSaveDocFile )
        saveDocFile();
}


void SCH_LEGACY_PLUGIN_CACHE::SaveSymbol( LIB_PART* aSymbol,
                                          OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aSymbol, "Invalid LIB_PART pointer." );

    LIB_FIELD&  value = aSymbol->GetValueField();

    // First line: it s a comment (component name for readers)
    aFormatter.Print( 0, "#\n# %s\n#\n", TO_UTF8( value.GetText() ) );

    // Save data
    aFormatter.Print( 0, "DEF" );
    aFormatter.Print( 0, " %s", TO_UTF8( value.GetText() ) );

    LIB_FIELD& reference = aSymbol->GetReferenceField();

    if( !reference.GetText().IsEmpty() )
    {
        aFormatter.Print( 0, " %s", TO_UTF8( reference.GetText() ) );
    }
    else
    {
        aFormatter.Print( 0, " ~" );
    }

    aFormatter.Print( 0, " %d %d %c %c %d %c %c\n",
                      0, aSymbol->GetPinNameOffset(),
                      aSymbol->ShowPinNumbers() ? 'Y' : 'N',
                      aSymbol->ShowPinNames() ? 'Y' : 'N',
                      aSymbol->GetUnitCount(), aSymbol->UnitsLocked() ? 'L' : 'F',
                      aSymbol->IsPower() ? 'P' : 'N' );

    timestamp_t dateModified = aSymbol->GetDateLastEdition();

    if( dateModified != 0 )
    {
        int sec  = dateModified & 63;
        int min  = ( dateModified >> 6 ) & 63;
        int hour = ( dateModified >> 12 ) & 31;
        int day  = ( dateModified >> 17 ) & 31;
        int mon  = ( dateModified >> 22 ) & 15;
        int year = ( dateModified >> 26 ) + 1990;

        aFormatter.Print( 0, "Ti %d/%d/%d %d:%d:%d\n", year, mon, day, hour, min, sec );
    }

    LIB_FIELDS fields;
    aSymbol->GetFields( fields );

    // Mandatory fields:
    // may have their own save policy so there is a separate loop for them.
    // Empty fields are saved, because the user may have set visibility,
    // size and orientation
    for( int i = 0;  i < MANDATORY_FIELDS;  ++i )
    {
        saveField( &fields[i], aFormatter );
    }

    // User defined fields:
    // may have their own save policy so there is a separate loop for them.

    int fieldId = MANDATORY_FIELDS;     // really wish this would go away.

    for( unsigned i = MANDATORY_FIELDS; i < fields.size(); ++i )
    {
        // There is no need to save empty fields, i.e. no reason to preserve field
        // names now that fields names come in dynamically through the template
        // fieldnames.
        if( !fields[i].GetText().IsEmpty() )
        {
            fields[i].SetId( fieldId++ );
            saveField( &fields[i], aFormatter );
        }
    }

    // Save the alias list: a line starting by "ALIAS".  The first alias is the root
    // and has the same name as the component.  In the old library file format this
    // alias does not get added to the alias list.
    if( aSymbol->GetAliasCount() > 1 )
    {
        wxArrayString aliases = aSymbol->GetAliasNames();

        aFormatter.Print( 0, "ALIAS" );

        for( unsigned i = 1; i < aliases.size(); i++ )
        {
            aFormatter.Print( 0, " %s", TO_UTF8( aliases[i] ) );
        }

        aFormatter.Print( 0, "\n" );
    }

    wxArrayString footprints = aSymbol->GetFootprints();

    // Write the footprint filter list
    if( footprints.GetCount() != 0 )
    {
        aFormatter.Print( 0, "$FPLIST\n" );

        for( unsigned i = 0; i < footprints.GetCount(); i++ )
        {
            aFormatter.Print( 0, " %s\n", TO_UTF8( footprints[i] ) );
        }

        aFormatter.Print( 0, "$ENDFPLIST\n" );
    }

    // Save graphics items (including pins)
    if( !aSymbol->GetDrawItems().empty() )
    {
        // Sort the draw items in order to editing a file editing by hand.
        aSymbol->GetDrawItems().sort();

        aFormatter.Print( 0, "DRAW\n" );

        for( LIB_ITEM& item : aSymbol->GetDrawItems() )
        {
            switch( item.Type() )
            {
            case LIB_FIELD_T:              // Fields have already been saved above.
                continue;

            case LIB_ARC_T:
                saveArc( (LIB_ARC*) &item, aFormatter );
                break;

            case LIB_BEZIER_T:
                saveBezier( (LIB_BEZIER*) &item, aFormatter );
                break;

            case LIB_CIRCLE_T:
                saveCircle( ( LIB_CIRCLE* ) &item, aFormatter );
                break;

            case LIB_PIN_T:
                savePin( (LIB_PIN* ) &item, aFormatter );
                break;

            case LIB_POLYLINE_T:
                savePolyLine( ( LIB_POLYLINE* ) &item, aFormatter );
                break;

            case LIB_RECTANGLE_T:
                saveRectangle( ( LIB_RECTANGLE* ) &item, aFormatter );
                break;

            case LIB_TEXT_T:
                saveText( ( LIB_TEXT* ) &item, aFormatter );
                break;

            default:
                ;
            }
        }

        aFormatter.Print( 0, "ENDDRAW\n" );
    }

    aFormatter.Print( 0, "ENDDEF\n" );
}


void SCH_LEGACY_PLUGIN_CACHE::saveArc( LIB_ARC* aArc,
                                       OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aArc && aArc->Type() == LIB_ARC_T, "Invalid LIB_ARC object." );

    int x1 = aArc->GetFirstRadiusAngle();

    if( x1 > 1800 )
        x1 -= 3600;

    int x2 = aArc->GetSecondRadiusAngle();

    if( x2 > 1800 )
        x2 -= 3600;

    aFormatter.Print( 0, "A %d %d %d %d %d %d %d %d %c %d %d %d %d\n",
                       aArc->GetPosition().x, aArc->GetPosition().y,
                       aArc->GetRadius(), x1, x2, aArc->GetUnit(), aArc->GetConvert(),
                       aArc->GetWidth(), fill_tab[aArc->GetFillMode()],
                       aArc->GetStart().x, aArc->GetStart().y,
                       aArc->GetEnd().x, aArc->GetEnd().y );
}


void SCH_LEGACY_PLUGIN_CACHE::saveBezier( LIB_BEZIER* aBezier,
                                          OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aBezier && aBezier->Type() == LIB_BEZIER_T, "Invalid LIB_BEZIER object." );

    aFormatter.Print( 0, "B %u %d %d %d", (unsigned)aBezier->GetPoints().size(),
                      aBezier->GetUnit(), aBezier->GetConvert(), aBezier->GetWidth() );

    for( const auto& pt : aBezier->GetPoints() )
        aFormatter.Print( 0, " %d %d", pt.x, pt.y );

    aFormatter.Print( 0, " %c\n", fill_tab[aBezier->GetFillMode()] );
}


void SCH_LEGACY_PLUGIN_CACHE::saveCircle( LIB_CIRCLE* aCircle,
                                          OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aCircle && aCircle->Type() == LIB_CIRCLE_T, "Invalid LIB_CIRCLE object." );

    aFormatter.Print( 0, "C %d %d %d %d %d %d %c\n",
                      aCircle->GetPosition().x, aCircle->GetPosition().y,
                      aCircle->GetRadius(), aCircle->GetUnit(), aCircle->GetConvert(),
                      aCircle->GetWidth(), fill_tab[aCircle->GetFillMode()] );
}


void SCH_LEGACY_PLUGIN_CACHE::saveField( LIB_FIELD* aField,
                                         OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aField && aField->Type() == LIB_FIELD_T, "Invalid LIB_FIELD object." );

    int      hjustify, vjustify;
    int      id = aField->GetId();
    wxString text = aField->GetText();

    hjustify = 'C';

    if( aField->GetHorizJustify() == GR_TEXT_HJUSTIFY_LEFT )
        hjustify = 'L';
    else if( aField->GetHorizJustify() == GR_TEXT_HJUSTIFY_RIGHT )
        hjustify = 'R';

    vjustify = 'C';

    if( aField->GetVertJustify() == GR_TEXT_VJUSTIFY_BOTTOM )
        vjustify = 'B';
    else if( aField->GetVertJustify() == GR_TEXT_VJUSTIFY_TOP )
        vjustify = 'T';

    aFormatter.Print( 0, "F%d %s %d %d %d %c %c %c %c%c%c",
                      id,
                      EscapedUTF8( text ).c_str(),       // wraps in quotes
                      aField->GetTextPos().x, aField->GetTextPos().y, aField->GetTextWidth(),
                      aField->GetTextAngle() == 0 ? 'H' : 'V',
                      aField->IsVisible() ? 'V' : 'I',
                      hjustify, vjustify,
                      aField->IsItalic() ? 'I' : 'N',
                      aField->IsBold() ? 'B' : 'N' );

    /* Save field name, if necessary
     * Field name is saved only if it is not the default name.
     * Just because default name depends on the language and can change from
     * a country to another
     */
    wxString defName = TEMPLATE_FIELDNAME::GetDefaultFieldName( id );

    if( id >= FIELD1 && !aField->m_name.IsEmpty() && aField->m_name != defName )
        aFormatter.Print( 0, " %s", EscapedUTF8( aField->m_name ).c_str() );

    aFormatter.Print( 0, "\n" );
}


void SCH_LEGACY_PLUGIN_CACHE::savePin( LIB_PIN* aPin,
                                       OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aPin && aPin->Type() == LIB_PIN_T, "Invalid LIB_PIN object." );

    int      Etype;

    switch( aPin->GetType() )
    {
    default:
    case PIN_INPUT:
        Etype = 'I';
        break;

    case PIN_OUTPUT:
        Etype = 'O';
        break;

    case PIN_BIDI:
        Etype = 'B';
        break;

    case PIN_TRISTATE:
        Etype = 'T';
        break;

    case PIN_PASSIVE:
        Etype = 'P';
        break;

    case PIN_UNSPECIFIED:
        Etype = 'U';
        break;

    case PIN_POWER_IN:
        Etype = 'W';
        break;

    case PIN_POWER_OUT:
        Etype = 'w';
        break;

    case PIN_OPENCOLLECTOR:
        Etype = 'C';
        break;

    case PIN_OPENEMITTER:
        Etype = 'E';
        break;

    case PIN_NC:
        Etype = 'N';
        break;
    }

    if( !aPin->GetName().IsEmpty() )
        aFormatter.Print( 0, "X %s", TO_UTF8( aPin->GetName() ) );
    else
        aFormatter.Print( 0, "X ~" );

    aFormatter.Print( 0, " %s %d %d %d %c %d %d %d %d %c",
                      aPin->GetNumber().IsEmpty() ? "~" : TO_UTF8( aPin->GetNumber() ),
                      aPin->GetPosition().x, aPin->GetPosition().y,
                      (int) aPin->GetLength(), (int) aPin->GetOrientation(),
                      aPin->GetNumberTextSize(), aPin->GetNameTextSize(),
                      aPin->GetUnit(), aPin->GetConvert(), Etype );

    if( aPin->GetShape() || !aPin->IsVisible() )
        aFormatter.Print( 0, " " );

    if( !aPin->IsVisible() )
        aFormatter.Print( 0, "N" );

    switch( aPin->GetShape() )
    {
    case PINSHAPE_LINE:
        break;

    case PINSHAPE_INVERTED:
        aFormatter.Print( 0, "I" );
        break;

    case PINSHAPE_CLOCK:
        aFormatter.Print( 0, "C" );
        break;

    case PINSHAPE_INVERTED_CLOCK:
        aFormatter.Print( 0, "IC" );
        break;

    case PINSHAPE_INPUT_LOW:
        aFormatter.Print( 0, "L" );
        break;

    case PINSHAPE_CLOCK_LOW:
        aFormatter.Print( 0, "CL" );
        break;

    case PINSHAPE_OUTPUT_LOW:
        aFormatter.Print( 0, "V" );
        break;

    case PINSHAPE_FALLING_EDGE_CLOCK:
        aFormatter.Print( 0, "F" );
        break;

    case PINSHAPE_NONLOGIC:
        aFormatter.Print( 0, "X" );
        break;

    default:
        assert( !"Invalid pin shape" );
    }

    aFormatter.Print( 0, "\n" );

    aPin->ClearFlags( IS_CHANGED );
}


void SCH_LEGACY_PLUGIN_CACHE::savePolyLine( LIB_POLYLINE* aPolyLine,
                                            OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aPolyLine && aPolyLine->Type() == LIB_POLYLINE_T, "Invalid LIB_POLYLINE object." );

    int ccount = aPolyLine->GetCornerCount();

    aFormatter.Print( 0, "P %d %d %d %d", ccount, aPolyLine->GetUnit(), aPolyLine->GetConvert(),
                      aPolyLine->GetWidth() );

    for( const auto& pt : aPolyLine->GetPolyPoints() )
    {
        aFormatter.Print( 0, " %d %d", pt.x, pt.y );
    }

    aFormatter.Print( 0, " %c\n", fill_tab[aPolyLine->GetFillMode()] );
}


void SCH_LEGACY_PLUGIN_CACHE::saveRectangle( LIB_RECTANGLE* aRectangle,
                                             OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aRectangle && aRectangle->Type() == LIB_RECTANGLE_T,
                 "Invalid LIB_RECTANGLE object." );

    aFormatter.Print( 0, "S %d %d %d %d %d %d %d %c\n",
                      aRectangle->GetPosition().x, aRectangle->GetPosition().y,
                      aRectangle->GetEnd().x, aRectangle->GetEnd().y,
                      aRectangle->GetUnit(), aRectangle->GetConvert(),
                      aRectangle->GetWidth(), fill_tab[aRectangle->GetFillMode()] );
}


void SCH_LEGACY_PLUGIN_CACHE::saveText( LIB_TEXT* aText,
                                        OUTPUTFORMATTER& aFormatter )
{
    wxCHECK_RET( aText && aText->Type() == LIB_TEXT_T, "Invalid LIB_TEXT object." );

    wxString text = aText->GetText();

    if( text.Contains( wxT( " " ) ) || text.Contains( wxT( "~" ) ) || text.Contains( wxT( "\"" ) ) )
    {
        // convert double quote to similar-looking two apostrophes
        text.Replace( wxT( "\"" ), wxT( "''" ) );
        text = wxT( "\"" ) + text + wxT( "\"" );
    }

    aFormatter.Print( 0, "T %g %d %d %d %d %d %d %s", aText->GetTextAngle(),
                      aText->GetTextPos().x, aText->GetTextPos().y,
                      aText->GetTextWidth(), !aText->IsVisible(),
                      aText->GetUnit(), aText->GetConvert(), TO_UTF8( text ) );

    aFormatter.Print( 0, " %s %d", aText->IsItalic() ? "Italic" : "Normal", aText->IsBold() );

    char hjustify = 'C';

    if( aText->GetHorizJustify() == GR_TEXT_HJUSTIFY_LEFT )
        hjustify = 'L';
    else if( aText->GetHorizJustify() == GR_TEXT_HJUSTIFY_RIGHT )
        hjustify = 'R';

    char vjustify = 'C';

    if( aText->GetVertJustify() == GR_TEXT_VJUSTIFY_BOTTOM )
        vjustify = 'B';
    else if( aText->GetVertJustify() == GR_TEXT_VJUSTIFY_TOP )
        vjustify = 'T';

    aFormatter.Print( 0, " %c %c\n", hjustify, vjustify );
}


void SCH_LEGACY_PLUGIN_CACHE::saveDocFile()
{
    wxFileName fileName = m_libFileName;

    fileName.SetExt( DOC_EXT );
    FILE_OUTPUTFORMATTER formatter( fileName.GetFullPath() );

    formatter.Print( 0, "%s\n", DOCFILE_IDENT );

    for( LIB_ALIAS_MAP::iterator it = m_aliases.begin();  it != m_aliases.end();  it++ )
    {
        wxString description =  it->second->GetDescription();
        wxString keyWords = it->second->GetKeyWords();
        wxString docFileName = it->second->GetDocFileName();

        if( description.IsEmpty() && keyWords.IsEmpty() && docFileName.IsEmpty() )
            continue;

        formatter.Print( 0, "#\n$CMP %s\n", TO_UTF8( it->second->GetName() ) );

        if( !description.IsEmpty() )
            formatter.Print( 0, "D %s\n", TO_UTF8( description ) );

        if( !keyWords.IsEmpty() )
            formatter.Print( 0, "K %s\n", TO_UTF8( keyWords ) );

        if( !docFileName.IsEmpty() )
            formatter.Print( 0, "F %s\n", TO_UTF8( docFileName ) );

        formatter.Print( 0, "$ENDCMP\n" );
    }

    formatter.Print( 0, "#\n#End Doc Library\n" );
}


void SCH_LEGACY_PLUGIN_CACHE::DeleteAlias( const wxString& aAliasName )
{
    LIB_ALIAS_MAP::iterator it = m_aliases.find( aAliasName );

    if( it == m_aliases.end() )
        THROW_IO_ERROR( wxString::Format( _( "library %s does not contain an alias %s" ),
                                          m_libFileName.GetFullName(), aAliasName ) );

    LIB_ALIAS*  alias = it->second;
    LIB_PART*   part = alias->GetPart();

    alias = part->RemoveAlias( alias );

    if( !alias )
    {
        delete part;

        if( m_aliases.size() > 1 )
        {
            LIB_ALIAS_MAP::iterator next = it;
            next++;

            if( next == m_aliases.end() )
                next = m_aliases.begin();

            alias = next->second;
        }
    }

    m_aliases.erase( it );
    ++m_modHash;
    m_isModified = true;
}


void SCH_LEGACY_PLUGIN_CACHE::DeleteSymbol( const wxString& aAliasName )
{
    LIB_ALIAS_MAP::iterator it = m_aliases.find( aAliasName );

    if( it == m_aliases.end() )
        THROW_IO_ERROR( wxString::Format( _( "library %s does not contain an alias %s" ),
                                          m_libFileName.GetFullName(), aAliasName ) );

    LIB_ALIAS*  alias = it->second;
    LIB_PART*   part = alias->GetPart();

    wxArrayString aliasNames = part->GetAliasNames();

    // Deleting all of the aliases deletes the symbol from the library.
    for( size_t i = 0;  i < aliasNames.Count(); i++ )
        DeleteAlias( aliasNames[i] );
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


bool SCH_LEGACY_PLUGIN::writeDocFile( const PROPERTIES* aProperties )
{
    std::string propName( SCH_LEGACY_PLUGIN::PropNoDocFile );

    if( aProperties && aProperties->find( propName ) != aProperties->end() )
        return false;

    return true;
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


void SCH_LEGACY_PLUGIN::EnumerateSymbolLib( wxArrayString&    aAliasNameList,
                                            const wxString&   aLibraryPath,
                                            const PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    m_props = aProperties;

    bool powerSymbolsOnly = ( aProperties &&
                              aProperties->find( SYMBOL_LIB_TABLE::PropPowerSymsOnly ) != aProperties->end() );
    cacheLib( aLibraryPath );

    const LIB_ALIAS_MAP& aliases = m_cache->m_aliases;

    for( LIB_ALIAS_MAP::const_iterator it = aliases.begin();  it != aliases.end();  ++it )
    {
        if( !powerSymbolsOnly || it->second->GetPart()->IsPower() )
            aAliasNameList.Add( it->first );
    }
}


void SCH_LEGACY_PLUGIN::EnumerateSymbolLib( std::vector<LIB_ALIAS*>& aAliasList,
                                            const wxString&   aLibraryPath,
                                            const PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    m_props = aProperties;

    bool powerSymbolsOnly = ( aProperties &&
                              aProperties->find( SYMBOL_LIB_TABLE::PropPowerSymsOnly ) != aProperties->end() );
    cacheLib( aLibraryPath );

    const LIB_ALIAS_MAP& aliases = m_cache->m_aliases;

    for( LIB_ALIAS_MAP::const_iterator it = aliases.begin();  it != aliases.end();  ++it )
    {
        if( !powerSymbolsOnly || it->second->GetPart()->IsPower() )
            aAliasList.push_back( it->second );
    }
}


LIB_ALIAS* SCH_LEGACY_PLUGIN::LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                                          const PROPERTIES* aProperties )
{
    LOCALE_IO toggle;     // toggles on, then off, the C locale.

    m_props = aProperties;

    cacheLib( aLibraryPath );

    LIB_ALIAS_MAP::const_iterator it = m_cache->m_aliases.find( aAliasName );

    if( it == m_cache->m_aliases.end() )
        return NULL;

    return it->second;
}


void SCH_LEGACY_PLUGIN::SaveSymbol( const wxString& aLibraryPath, const LIB_PART* aSymbol,
                                    const PROPERTIES* aProperties )
{
    m_props = aProperties;

    cacheLib( aLibraryPath );

    m_cache->AddSymbol( aSymbol );

    if( !isBuffering( aProperties ) )
        m_cache->Save( writeDocFile( aProperties ) );
}


void SCH_LEGACY_PLUGIN::DeleteAlias( const wxString& aLibraryPath, const wxString& aAliasName,
                                     const PROPERTIES* aProperties )
{
    m_props = aProperties;

    cacheLib( aLibraryPath );

    m_cache->DeleteAlias( aAliasName );

    if( !isBuffering( aProperties ) )
        m_cache->Save( writeDocFile( aProperties ) );
}


void SCH_LEGACY_PLUGIN::DeleteSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                                      const PROPERTIES* aProperties )
{
    m_props = aProperties;

    cacheLib( aLibraryPath );

    m_cache->DeleteSymbol( aAliasName );

    if( !isBuffering( aProperties ) )
        m_cache->Save( writeDocFile( aProperties ) );
}


void SCH_LEGACY_PLUGIN::CreateSymbolLib( const wxString& aLibraryPath,
                                         const PROPERTIES* aProperties )
{
    if( wxFileExists( aLibraryPath ) )
    {
        THROW_IO_ERROR( wxString::Format(
            _( "symbol library \"%s\" already exists, cannot create a new library" ),
            aLibraryPath.GetData() ) );
    }

    LOCALE_IO toggle;

    m_props = aProperties;

    delete m_cache;
    m_cache = new SCH_LEGACY_PLUGIN_CACHE( aLibraryPath );
    m_cache->SetModified();
    m_cache->Save( writeDocFile( aProperties ) );
    m_cache->Load();    // update m_writable and m_mod_time
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


void SCH_LEGACY_PLUGIN::SaveLibrary( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    if( !m_cache )
        m_cache = new SCH_LEGACY_PLUGIN_CACHE( aLibraryPath );

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
    return wxFileName::IsFileWritable( aLibraryPath );
}


LIB_PART* SCH_LEGACY_PLUGIN::ParsePart( LINE_READER& reader, int aMajorVersion,
                                        int aMinorVersion )
{
    return SCH_LEGACY_PLUGIN_CACHE::LoadPart( reader, aMajorVersion, aMinorVersion );
}


void SCH_LEGACY_PLUGIN::FormatPart( LIB_PART* part, OUTPUTFORMATTER & formatter )
{
    SCH_LEGACY_PLUGIN_CACHE::SaveSymbol( part, formatter );
}



const char* SCH_LEGACY_PLUGIN::PropBuffering = "buffering";
const char* SCH_LEGACY_PLUGIN::PropNoDocFile = "no_doc_file";
