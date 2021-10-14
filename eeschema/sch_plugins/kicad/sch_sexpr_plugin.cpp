/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

// For some reason wxWidgets is built with wxUSE_BASE64 unset so expose the wxWidgets
// base64 code.
#define wxUSE_BASE64 1
#include <wx/base64.h>
#include <wx/log.h>
#include <wx/mstream.h>
#include <advanced_config.h>
#include <trace_helpers.h>
#include <locale_io.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_symbol.h>
#include <sch_edit_frame.h>       // SYMBOL_ORIENTATION_T
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_text.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <schematic.h>
#include <sch_plugins/kicad/sch_sexpr_plugin.h>
#include <sch_screen.h>
#include <symbol_library.h>
#include <lib_shape.h>
#include <lib_field.h>
#include <lib_pin.h>
#include <lib_text.h>
#include <eeschema_id.h>       // for MAX_UNIT_COUNT_PER_PACKAGE definition
#include <sch_file_versions.h>
#include <schematic_lexer.h>
#include <sch_plugins/kicad/sch_sexpr_parser.h>
#include <symbol_lib_table.h>  // for PropPowerSymsOnly definition.
#include <ee_selection.h>
#include <string_utils.h>
#include <wx_filename.h>       // for ::ResolvePossibleSymlinks()
#include <progress_reporter.h>


using namespace TSCHEMATIC_T;


#define SCH_PARSE_ERROR( text, reader, pos )                         \
    THROW_PARSE_ERROR( text, reader.GetSource(), reader.Line(),      \
                       reader.LineNumber(), pos - reader.Line() )


static const char* emptyString = "";

/**
 * Fill token formatting helper.
 */
static void formatFill( OUTPUTFORMATTER* aFormatter, int aNestLevel, FILL_T aFillMode,
                        const COLOR4D& aFillColor )
{
    const char* fillType;

    switch( aFillMode )
    {
    default:
    case FILL_T::NO_FILL:                  fillType = "none";       break;
    case FILL_T::FILLED_SHAPE:             fillType = "outline";    break;
    case FILL_T::FILLED_WITH_BG_BODYCOLOR: fillType = "background"; break;
    }

    aFormatter->Print( aNestLevel, "(fill (type %s))", fillType );
}


static const char* getPinElectricalTypeToken( ELECTRICAL_PINTYPE aType )
{
    switch( aType )
    {
    case ELECTRICAL_PINTYPE::PT_INPUT:
        return SCHEMATIC_LEXER::TokenName( T_input );

    case ELECTRICAL_PINTYPE::PT_OUTPUT:
        return SCHEMATIC_LEXER::TokenName( T_output );

    case ELECTRICAL_PINTYPE::PT_BIDI:
        return SCHEMATIC_LEXER::TokenName( T_bidirectional );

    case ELECTRICAL_PINTYPE::PT_TRISTATE:
        return SCHEMATIC_LEXER::TokenName( T_tri_state );

    case ELECTRICAL_PINTYPE::PT_PASSIVE:
        return SCHEMATIC_LEXER::TokenName( T_passive );

    case ELECTRICAL_PINTYPE::PT_NIC:
        return SCHEMATIC_LEXER::TokenName( T_free );

    case ELECTRICAL_PINTYPE::PT_UNSPECIFIED:
        return SCHEMATIC_LEXER::TokenName( T_unspecified );

    case ELECTRICAL_PINTYPE::PT_POWER_IN:
        return SCHEMATIC_LEXER::TokenName( T_power_in );

    case ELECTRICAL_PINTYPE::PT_POWER_OUT:
        return SCHEMATIC_LEXER::TokenName( T_power_out );

    case ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR:
        return SCHEMATIC_LEXER::TokenName( T_open_collector );

    case ELECTRICAL_PINTYPE::PT_OPENEMITTER:
        return SCHEMATIC_LEXER::TokenName( T_open_emitter );

    case ELECTRICAL_PINTYPE::PT_NC:
        return SCHEMATIC_LEXER::TokenName( T_no_connect );

    default:
        wxFAIL_MSG( "Missing symbol library pin connection type" );
    }

    return emptyString;
}


static const char* getPinShapeToken( GRAPHIC_PINSHAPE aShape )
{
    switch( aShape )
    {
    case GRAPHIC_PINSHAPE::LINE:
        return SCHEMATIC_LEXER::TokenName( T_line );

    case GRAPHIC_PINSHAPE::INVERTED:
        return SCHEMATIC_LEXER::TokenName( T_inverted );

    case GRAPHIC_PINSHAPE::CLOCK:
        return SCHEMATIC_LEXER::TokenName( T_clock );

    case GRAPHIC_PINSHAPE::INVERTED_CLOCK:
        return SCHEMATIC_LEXER::TokenName( T_inverted_clock );

    case GRAPHIC_PINSHAPE::INPUT_LOW:
        return SCHEMATIC_LEXER::TokenName( T_input_low );

    case GRAPHIC_PINSHAPE::CLOCK_LOW:
        return SCHEMATIC_LEXER::TokenName( T_clock_low );

    case GRAPHIC_PINSHAPE::OUTPUT_LOW:
        return SCHEMATIC_LEXER::TokenName( T_output_low );

    case GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK:
        return SCHEMATIC_LEXER::TokenName( T_edge_clock_high );

    case GRAPHIC_PINSHAPE::NONLOGIC:
        return SCHEMATIC_LEXER::TokenName( T_non_logic );

    default:
        wxFAIL_MSG( "Missing symbol library pin shape type" );
    }

    return emptyString;
}


static float getPinAngle( int aOrientation )
{
    switch( aOrientation )
    {
    case PIN_RIGHT: return 0.0;
    case PIN_LEFT:  return 180.0;
    case PIN_UP:    return 90.0;
    case PIN_DOWN:  return 270.0;
    default:        wxFAIL_MSG( "Missing symbol library pin orientation type" ); return 0.0;
    }
}


static const char* getSheetPinShapeToken( PINSHEETLABEL_SHAPE aShape )
{
    switch( aShape )
    {
    case PINSHEETLABEL_SHAPE::PS_INPUT:       return SCHEMATIC_LEXER::TokenName( T_input );
    case PINSHEETLABEL_SHAPE::PS_OUTPUT:      return SCHEMATIC_LEXER::TokenName( T_output );
    case PINSHEETLABEL_SHAPE::PS_BIDI:        return SCHEMATIC_LEXER::TokenName( T_bidirectional );
    case PINSHEETLABEL_SHAPE::PS_TRISTATE:    return SCHEMATIC_LEXER::TokenName( T_tri_state );
    case PINSHEETLABEL_SHAPE::PS_UNSPECIFIED: return SCHEMATIC_LEXER::TokenName( T_passive );
    default:         wxFAIL;                  return SCHEMATIC_LEXER::TokenName( T_passive );
    }
}


static double getSheetPinAngle( SHEET_SIDE aSide )
{
    double retv;

    switch( aSide )
    {
    case SHEET_SIDE::UNDEFINED:
    case SHEET_SIDE::LEFT:     retv = 180.0; break;
    case SHEET_SIDE::RIGHT:    retv = 0.0;   break;
    case SHEET_SIDE::TOP:      retv = 90.0;  break;
    case SHEET_SIDE::BOTTOM:   retv = 270.0; break;
    default:   wxFAIL;         retv = 0.0;   break;
    }

    return retv;
}


static wxString getLineStyleToken( PLOT_DASH_TYPE aStyle )
{
    wxString token;

    switch( aStyle )
    {
    case PLOT_DASH_TYPE::DASH:     token = "dash";      break;
    case PLOT_DASH_TYPE::DOT:      token = "dot";       break;
    case PLOT_DASH_TYPE::DASHDOT:  token = "dash_dot";  break;
    case PLOT_DASH_TYPE::SOLID:    token = "solid";     break;
    case PLOT_DASH_TYPE::DEFAULT:  token = "default";   break;
    }

    return token;
}


static const char* getTextTypeToken( KICAD_T aType )
{
    switch( aType )
    {
    case SCH_TEXT_T:          return SCHEMATIC_LEXER::TokenName( T_text );
    case SCH_LABEL_T:         return SCHEMATIC_LEXER::TokenName( T_label );
    case SCH_GLOBAL_LABEL_T:  return SCHEMATIC_LEXER::TokenName( T_global_label );
    case SCH_HIER_LABEL_T:    return SCHEMATIC_LEXER::TokenName( T_hierarchical_label );
    default:     wxFAIL;      return SCHEMATIC_LEXER::TokenName( T_text );
    }
}


/**
 * Write stroke definition to \a aFormatter.
 *
 * This only writes the stroke definition if \a aWidth, \a aStyle and \a aColor are
 * not the default setting or are not defined.
 *
 * @param aFormatter A pointer to the #OUTPUTFORMATTER object to write to.
 * @param aNestLevel The nest level to indent the stroke definition.
 * @param aWidth The stroke line width in internal units.
 * @param aStyle The stroke line style.
 * @param aColor The stroke line color.
 */
static void formatStroke( OUTPUTFORMATTER* aFormatter, int aNestLevel,
                          const STROKE_PARAMS& aStroke )
{
    wxASSERT( aFormatter != nullptr );

    aFormatter->Print( aNestLevel, "(stroke (width %s) (type %s) (color %d %d %d %s))",
                       FormatInternalUnits( aStroke.GetWidth() ).c_str(),
                       TO_UTF8( getLineStyleToken( aStroke.GetPlotStyle() ) ),
                       KiROUND( aStroke.GetColor().r * 255.0 ),
                       KiROUND( aStroke.GetColor().g * 255.0 ),
                       KiROUND( aStroke.GetColor().b * 255.0 ),
                       Double2Str( aStroke.GetColor().a ).c_str() );
}


static void formatArc( OUTPUTFORMATTER* aFormatter, int aNestLevel, EDA_SHAPE* aArc, int x1, int x2,
                       const STROKE_PARAMS& aStroke, FILL_T aFillMode, const COLOR4D& aFillColor,
                       KIID aUuid = niluuid )
{
    if( x1 > 1800 )
        x1 -= 3600;

    if( x2 > 1800 )
        x2 -= 3600;

    aFormatter->Print( aNestLevel, "(arc (start %s) (mid %s) (end %s)\n",
                       FormatInternalUnits( aArc->GetStart() ).c_str(),
                       FormatInternalUnits( aArc->GetArcMid() ).c_str(),
                       FormatInternalUnits( aArc->GetEnd() ).c_str() );

    formatStroke( aFormatter, aNestLevel + 1, aStroke );
    aFormatter->Print( 0, "\n" );
    formatFill( aFormatter, aNestLevel + 1, aFillMode, aFillColor );
    aFormatter->Print( 0, "\n" );

    if( aUuid != niluuid )
        aFormatter->Print( aNestLevel + 1, "(uuid %s)\n", TO_UTF8( aUuid.AsString() ) );

    aFormatter->Print( aNestLevel, ")\n" );
}


static void formatCircle( OUTPUTFORMATTER* aFormatter, int aNestLevel, EDA_SHAPE* aCircle,
                          const STROKE_PARAMS& aStroke, FILL_T aFillMode, const COLOR4D& aFillColor,
                          KIID aUuid = niluuid )
{
    aFormatter->Print( aNestLevel, "(circle (center %s %s) (radius %s) (stroke (width %s)) ",
                       FormatInternalUnits( aCircle->GetStart().x ).c_str(),
                       FormatInternalUnits( aCircle->GetStart().y ).c_str(),
                       FormatInternalUnits( aCircle->GetRadius() ).c_str(),
                       FormatInternalUnits( aCircle->GetWidth() ).c_str() );

    formatStroke( aFormatter, aNestLevel + 1, aStroke );
    aFormatter->Print( 0, "\n" );
    formatFill( aFormatter, aNestLevel + 1, aFillMode, aFillColor );
    aFormatter->Print( 0, "\n" );

    if( aUuid != niluuid )
        aFormatter->Print( aNestLevel + 1, "(uuid %s)\n", TO_UTF8( aUuid.AsString() ) );

    aFormatter->Print( aNestLevel, ")\n" );
}


static void formatRect( OUTPUTFORMATTER* aFormatter, int aNestLevel, EDA_SHAPE* aRect,
                        const STROKE_PARAMS& aStroke, FILL_T aFillMode, const COLOR4D& aFillColor,
                        KIID aUuid = niluuid )
{
    aFormatter->Print( aNestLevel, "(rectangle (start %s %s) (end %s %s)\n",
                       FormatInternalUnits( aRect->GetStart().x ).c_str(),
                       FormatInternalUnits( aRect->GetStart().y ).c_str(),
                       FormatInternalUnits( aRect->GetEnd().x ).c_str(),
                       FormatInternalUnits( aRect->GetEnd().y ).c_str() );
    formatStroke( aFormatter, aNestLevel + 1, aStroke );
    aFormatter->Print( 0, "\n" );
    formatFill( aFormatter, aNestLevel + 1, aFillMode, aFillColor );
    aFormatter->Print( 0, "\n" );

    if( aUuid != niluuid )
        aFormatter->Print( aNestLevel + 1, "(uuid %s)\n", TO_UTF8( aUuid.AsString() ) );

    aFormatter->Print( aNestLevel, ")\n" );
}


static void formatBezier( OUTPUTFORMATTER* aFormatter, int aNestLevel, EDA_SHAPE* aBezier,
                          const STROKE_PARAMS& aStroke, FILL_T aFillMode, const COLOR4D& aFillColor,
                          KIID aUuid = niluuid )
{
    aFormatter->Print( aNestLevel, "(bezier (pts " );

    for( const wxPoint& pt : { aBezier->GetStart(), aBezier->GetBezierC1(),
                               aBezier->GetBezierC2(), aBezier->GetEnd() } )
    {
        aFormatter->Print( 0, " (xy %s %s)",
                           FormatInternalUnits( pt.x ).c_str(),
                           FormatInternalUnits( pt.y ).c_str() );
    }

    aFormatter->Print( 0, ")\n" );  // Closes pts token on same line.

    formatStroke( aFormatter, aNestLevel + 1, aStroke );
    aFormatter->Print( 0, "\n" );
    formatFill( aFormatter, aNestLevel + 1, aFillMode, aFillColor );
    aFormatter->Print( 0, "\n" );

    if( aUuid != niluuid )
        aFormatter->Print( aNestLevel + 1, "(uuid %s)\n", TO_UTF8( aUuid.AsString() ) );

    aFormatter->Print( aNestLevel, ")\n" );
}


static void formatPoly( OUTPUTFORMATTER* aFormatter, int aNestLevel, EDA_SHAPE* aPolyLine,
                        const STROKE_PARAMS& aStroke, FILL_T aFillMode, const COLOR4D& aFillColor,
                        KIID aUuid = niluuid )
{
    int newLine = 0;
    int lineCount = 1;
    aFormatter->Print( aNestLevel, "(polyline\n" );
    aFormatter->Print( aNestLevel + 1, "(pts" );

    for( const VECTOR2I& pt : aPolyLine->GetPolyShape().Outline( 0 ).CPoints() )
    {
        if( newLine == 4 || !ADVANCED_CFG::GetCfg().m_CompactSave )
        {
            aFormatter->Print( 0, "\n" );
            aFormatter->Print( aNestLevel + 2, "(xy %s %s)",
                               FormatInternalUnits( pt.x ).c_str(),
                               FormatInternalUnits( pt.y ).c_str() );
            newLine = 0;
            lineCount += 1;
        }
        else
        {
            aFormatter->Print( 0, " (xy %s %s)",
                               FormatInternalUnits( pt.x ).c_str(),
                               FormatInternalUnits( pt.y ).c_str() );
        }

        newLine += 1;
    }

    if( lineCount == 1 )
    {
        aFormatter->Print( 0, ")\n" );  // Closes pts token on same line.
    }
    else
    {
        aFormatter->Print( 0, "\n" );
        aFormatter->Print( aNestLevel + 1, ")\n" );  // Closes pts token with multiple lines.
    }

    formatStroke( aFormatter, aNestLevel + 1, aStroke );
    aFormatter->Print( 0, "\n" );
    formatFill( aFormatter, aNestLevel + 1, aFillMode, aFillColor );
    aFormatter->Print( 0, "\n" );

    if( aUuid != niluuid )
        aFormatter->Print( aNestLevel + 1, "(uuid %s)\n", TO_UTF8( aUuid.AsString() ) );

    aFormatter->Print( aNestLevel, ")\n" );
}


/**
 * A cache assistant for the symbol library portion of the #SCH_PLUGIN API, and only for the
 * #SCH_SEXPR_PLUGIN, so therefore is private to this implementation file, i.e. not placed
 * into a header.
 */
class SCH_SEXPR_PLUGIN_CACHE
{
    static int      m_modHash;      // Keep track of the modification status of the library.

    wxString        m_fileName;     // Absolute path and file name.
    wxFileName      m_libFileName;  // Absolute path and file name is required here.
    wxDateTime      m_fileModTime;
    LIB_SYMBOL_MAP  m_symbols;      // Map of names of #LIB_SYMBOL pointers.
    bool            m_isWritable;
    bool            m_isModified;
    int             m_versionMajor;
    SCH_LIB_TYPE    m_libType;      // Is this cache a symbol or symbol library.

    LIB_SYMBOL* removeSymbol( LIB_SYMBOL* aAlias );

    static void saveSymbolDrawItem( LIB_ITEM* aItem, OUTPUTFORMATTER& aFormatter,
                                    int aNestLevel );
    static void saveField( LIB_FIELD* aField, OUTPUTFORMATTER& aFormatter, int& aNextFreeFieldId,
                           int aNestLevel );
    static void savePin( LIB_PIN* aPin, OUTPUTFORMATTER& aFormatter, int aNestLevel = 0 );
    static void saveText( LIB_TEXT* aText, OUTPUTFORMATTER& aFormatter, int aNestLevel = 0 );

    static void saveDcmInfoAsFields( LIB_SYMBOL* aSymbol, OUTPUTFORMATTER& aFormatter,
                                     int& aNextFreeFieldId, int aNestLevel );

    friend SCH_SEXPR_PLUGIN;

public:
    SCH_SEXPR_PLUGIN_CACHE( const wxString& aLibraryPath );
    ~SCH_SEXPR_PLUGIN_CACHE();

    int GetModifyHash() const { return m_modHash; }

    // Most all functions in this class throw IO_ERROR exceptions.  There are no
    // error codes nor user interface calls from here, nor in any SCH_PLUGIN objects.
    // Catch these exceptions higher up please.

    /// Save the entire library to file m_libFileName;
    void Save();

    void Load();

    void AddSymbol( const LIB_SYMBOL* aSymbol );

    void DeleteSymbol( const wxString& aName );

    // If m_libFileName is a symlink follow it to the real source file
    wxFileName GetRealFile() const;

    wxDateTime GetLibModificationTime();

    bool IsFile( const wxString& aFullPathAndFileName ) const;

    bool IsFileChanged() const;

    void SetModified( bool aModified = true ) { m_isModified = aModified; }

    wxString GetLogicalName() const { return m_libFileName.GetName(); }

    void SetFileName( const wxString& aFileName ) { m_libFileName = aFileName; }

    wxString GetFileName() const { return m_libFileName.GetFullPath(); }

    static void SaveSymbol( LIB_SYMBOL* aSymbol, OUTPUTFORMATTER& aFormatter,
                            int aNestLevel = 0, const wxString& aLibName = wxEmptyString );
};


SCH_SEXPR_PLUGIN::SCH_SEXPR_PLUGIN() :
    m_progressReporter( nullptr )
{
    init( nullptr );
}


SCH_SEXPR_PLUGIN::~SCH_SEXPR_PLUGIN()
{
    delete m_cache;
}


void SCH_SEXPR_PLUGIN::init( SCHEMATIC* aSchematic, const PROPERTIES* aProperties )
{
    m_version         = 0;
    m_rootSheet       = nullptr;
    m_schematic       = aSchematic;
    m_cache           = nullptr;
    m_out             = nullptr;
    m_nextFreeFieldId = 100; // number arbitrarily > MANDATORY_FIELDS or SHEET_MANDATORY_FIELDS
}


SCH_SHEET* SCH_SEXPR_PLUGIN::Load( const wxString& aFileName, SCHEMATIC* aSchematic,
                                   SCH_SHEET* aAppendToMe, const PROPERTIES* aProperties )
{
    wxASSERT( !aFileName || aSchematic != nullptr );

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

        wxLogTrace( traceSchLegacyPlugin, "Normalized append path \"%s\".", m_path );
    }
    else
    {
        m_path = aSchematic->Prj().GetProjectPath();
    }

    m_currentPath.push( m_path );
    init( aSchematic, aProperties );

    if( aAppendToMe == nullptr )
    {
        // Clean up any allocated memory if an exception occurs loading the schematic.
        std::unique_ptr<SCH_SHEET> newSheet = std::make_unique<SCH_SHEET>( aSchematic );

        wxFileName relPath( aFileName );
        // Do not use wxPATH_UNIX as option in MakeRelativeTo(). It can create incorrect
        // relative paths on Windows, because paths have a disk identifier (C:, D: ...)
        relPath.MakeRelativeTo( aSchematic->Prj().GetProjectPath() );

        newSheet->SetFileName( relPath.GetFullPath() );
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

    m_currentPath.pop(); // Clear the path stack for next call to Load

    return sheet;
}


// Everything below this comment is recursive.  Modify with care.

void SCH_SEXPR_PLUGIN::loadHierarchy( SCH_SHEET* aSheet )
{
    SCH_SCREEN* screen = nullptr;

    if( !aSheet->GetScreen() )
    {
        // SCH_SCREEN objects store the full path and file name where the SCH_SHEET object only
        // stores the file name and extension.  Add the project path to the file name and
        // extension to compare when calling SCH_SHEET::SearchHierarchy().
        wxFileName fileName = aSheet->GetFileName();

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
            aSheet->GetScreen()->SetParent( m_schematic );
            // Do not need to load the sub-sheets - this has already been done.
        }
        else
        {
            aSheet->SetScreen( new SCH_SCREEN( m_schematic ) );
            aSheet->GetScreen()->SetFileName( fileName.GetFullPath() );

            try
            {
                loadFile( fileName.GetFullPath(), aSheet );
            }
            catch( const IO_ERROR& ioe )
            {
                // If there is a problem loading the root sheet, there is no recovery.
                if( aSheet == m_rootSheet )
                    throw;

                // For all subsheets, queue up the error message for the caller.
                if( !m_error.IsEmpty() )
                    m_error += "\n";

                m_error += ioe.What();
            }

            aSheet->GetScreen()->SetFileReadOnly( !fileName.IsFileWritable() );
            aSheet->GetScreen()->SetFileExists( true );

            // This was moved out of the try{} block so that any sheets definitionsthat
            // the plugin fully parsed before the exception was raised will be loaded.
            for( SCH_ITEM* aItem : aSheet->GetScreen()->Items().OfType( SCH_SHEET_T ) )
            {
                wxCHECK2( aItem->Type() == SCH_SHEET_T, /* do nothing */ );
                SCH_SHEET* sheet = static_cast<SCH_SHEET*>( aItem );

                // Recursion starts here.
                loadHierarchy( sheet );
            }
        }

        m_currentPath.pop();
        wxLogTrace( traceSchLegacyPlugin, "Restoring path \"%s\"", m_currentPath.top() );
    }
}


void SCH_SEXPR_PLUGIN::loadFile( const wxString& aFileName, SCH_SHEET* aSheet )
{
    FILE_LINE_READER reader( aFileName );

    size_t lineCount = 0;

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_ERROR( ( "Open cancelled by user." ) );

        while( reader.ReadLine() )
            lineCount++;

        reader.Rewind();
    }

    SCH_SEXPR_PARSER parser( &reader, m_progressReporter, lineCount );

    parser.ParseSchematic( aSheet );
}


void SCH_SEXPR_PLUGIN::LoadContent( LINE_READER& aReader, SCH_SHEET* aSheet, int aFileVersion )
{
    wxCHECK( aSheet, /* void */ );

    LOCALE_IO toggle;
    SCH_SEXPR_PARSER parser( &aReader );

    parser.ParseSchematic( aSheet, true, aFileVersion );
}


void SCH_SEXPR_PLUGIN::Save( const wxString& aFileName, SCH_SHEET* aSheet, SCHEMATIC* aSchematic,
                             const PROPERTIES* aProperties )
{
    wxCHECK_RET( aSheet != nullptr, "NULL SCH_SHEET object." );
    wxCHECK_RET( !aFileName.IsEmpty(), "No schematic file name defined." );

    LOCALE_IO   toggle;     // toggles on, then off, the C locale, to write floating point values.

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


void SCH_SEXPR_PLUGIN::Format( SCH_SHEET* aSheet )
{
    wxCHECK_RET( aSheet != nullptr, "NULL SCH_SHEET* object." );
    wxCHECK_RET( m_schematic != nullptr, "NULL SCHEMATIC* object." );

    SCH_SCREEN* screen = aSheet->GetScreen();

    wxCHECK( screen, /* void */ );

    m_out->Print( 0, "(kicad_sch (version %d) (generator eeschema)\n\n",
                  SEXPR_SCHEMATIC_FILE_VERSION );

    m_out->Print( 1, "(uuid %s)\n\n", TO_UTF8( screen->m_uuid.AsString() ) );

    screen->GetPageSettings().Format( m_out, 1, 0 );
    m_out->Print( 0, "\n" );
    screen->GetTitleBlock().Format( m_out, 1, 0 );

    // Save cache library.
    m_out->Print( 1, "(lib_symbols\n" );

    for( auto libSymbol : screen->GetLibSymbols() )
        SCH_SEXPR_PLUGIN_CACHE::SaveSymbol( libSymbol.second, *m_out, 2, libSymbol.first );

    m_out->Print( 1, ")\n\n" );

    for( const auto& alias : screen->GetBusAliases() )
    {
        saveBusAlias( alias, 1 );
    }

    // Enforce item ordering
    auto cmp = []( const SCH_ITEM* a, const SCH_ITEM* b )
               {
                   return *a < *b;
               };

    std::multiset<SCH_ITEM*, decltype( cmp )> save_map( cmp );

    for( SCH_ITEM* item : screen->Items() )
        save_map.insert( item );

    KICAD_T itemType = TYPE_NOT_INIT;
    SCH_LAYER_ID layer = SCH_LAYER_ID_START;

    for( SCH_ITEM* item : save_map )
    {
        if( itemType != item->Type() )
        {
            itemType = item->Type();

            if( itemType != SCH_SYMBOL_T
              && itemType != SCH_JUNCTION_T
              && itemType != SCH_SHEET_T )
                m_out->Print( 0, "\n" );
        }

        switch( item->Type() )
        {
        case SCH_SYMBOL_T:
            m_out->Print( 0, "\n" );
            saveSymbol( static_cast<SCH_SYMBOL*>( item ), nullptr, 1 );
            break;

        case SCH_BITMAP_T:
            saveBitmap( static_cast<SCH_BITMAP*>( item ), 1 );
            break;

        case SCH_SHEET_T:
            m_out->Print( 0, "\n" );
            saveSheet( static_cast<SCH_SHEET*>( item ), 1 );
            break;

        case SCH_JUNCTION_T:
            saveJunction( static_cast<SCH_JUNCTION*>( item ), 1 );
            break;

        case SCH_NO_CONNECT_T:
            saveNoConnect( static_cast<SCH_NO_CONNECT*>( item ), 1 );
            break;

        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_BUS_BUS_ENTRY_T:
            saveBusEntry( static_cast<SCH_BUS_ENTRY_BASE*>( item ), 1 );
            break;

        case SCH_LINE_T:
            if( layer != item->GetLayer() )
            {
                if( layer == SCH_LAYER_ID_START )
                {
                    layer = item->GetLayer();
                }
                else
                {
                    layer = item->GetLayer();
                    m_out->Print( 0, "\n" );
                }
            }

            saveLine( static_cast<SCH_LINE*>( item ), 1 );
            break;

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
            saveText( static_cast<SCH_TEXT*>( item ), 1 );
            break;

        default:
            wxASSERT( "Unexpected schematic object type in SCH_SEXPR_PLUGIN::Format()" );
        }
    }

    // If this is the root sheet, save all of the sheet paths.
    if( aSheet->IsRootSheet() )
    {
        SCH_SHEET_LIST sheetPaths( aSheet, true );

        SCH_REFERENCE_LIST symbolInstances;

        for( const SCH_SHEET_PATH& sheetPath : sheetPaths )
            sheetPath.GetSymbols( symbolInstances, true, true );

        symbolInstances.SortByReferenceOnly();

        saveInstances( sheetPaths.GetSheetInstances(), symbolInstances.GetSymbolInstances(), 1 );
    }
    else
    {
        // Schematic files (SCH_SCREEN objects) can be shared so we have to save and restore
        // symbol and sheet instance data even if the file being saved is not the root sheet
        // because it is possible that the file is the root sheet of another project.
        saveInstances( screen->m_sheetInstances, screen->m_symbolInstances, 1 );
    }

    m_out->Print( 0, ")\n" );
}


void SCH_SEXPR_PLUGIN::Format( EE_SELECTION* aSelection, SCH_SHEET_PATH* aSelectionPath,
                               SCH_SHEET_LIST* aFullSheetHierarchy,
                               OUTPUTFORMATTER* aFormatter )
{
    wxCHECK( aSelection && aSelectionPath && aFullSheetHierarchy && aFormatter, /* void */ );

    LOCALE_IO toggle;

    m_out = aFormatter;

    size_t i;
    SCH_ITEM* item;
    std::map<wxString, LIB_SYMBOL*> libSymbols;
    SCH_SCREEN* screen = aSelection->GetScreen();

    for( i = 0; i < aSelection->GetSize(); ++i )
    {
        item = dynamic_cast<SCH_ITEM*>( aSelection->GetItem( i ) );

        wxCHECK2( item, continue );

        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item );

        wxCHECK2( symbol, continue );

        wxString libSymbolLookup = symbol->GetLibId().Format().wx_str();

        if( !symbol->UseLibIdLookup() )
            libSymbolLookup = symbol->GetSchSymbolLibraryName();

        auto it = screen->GetLibSymbols().find( libSymbolLookup );

        if( it != screen->GetLibSymbols().end() )
            libSymbols[ libSymbolLookup ] = it->second;
    }

    if( !libSymbols.empty() )
    {
        m_out->Print( 0, "(lib_symbols\n" );

        for( auto libSymbol : libSymbols )
            SCH_SEXPR_PLUGIN_CACHE::SaveSymbol( libSymbol.second, *m_out, 1, libSymbol.first );

        m_out->Print( 0, ")\n\n" );
    }

    // Store the selected sheets instance information
    SCH_SHEET_LIST selectedSheets;
    selectedSheets.push_back( *aSelectionPath ); // Include the "root" of the selection

    SCH_REFERENCE_LIST selectedSymbols;

    for( i = 0; i < aSelection->GetSize(); ++i )
    {
        item = (SCH_ITEM*) aSelection->GetItem( i );

        switch( item->Type() )
        {
        case SCH_SYMBOL_T:
            saveSymbol( static_cast<SCH_SYMBOL*>( item ), aSelectionPath, 0 );

            aSelectionPath->AppendSymbol( selectedSymbols, static_cast<SCH_SYMBOL*>( item ),
                                          true, true );
            break;

        case SCH_BITMAP_T:
            saveBitmap( static_cast< SCH_BITMAP* >( item ), 0 );
            break;

        case SCH_SHEET_T:
            saveSheet( static_cast< SCH_SHEET* >( item ), 0 );

            {
                SCH_SHEET_PATH subSheetPath = *aSelectionPath;
                subSheetPath.push_back( static_cast<SCH_SHEET*>( item ) );

                aFullSheetHierarchy->GetSheetsWithinPath( selectedSheets, subSheetPath );
                aFullSheetHierarchy->GetSymbolsWithinPath( selectedSymbols, subSheetPath, true,
                                                          true );
            }

            break;

        case SCH_JUNCTION_T:
            saveJunction( static_cast< SCH_JUNCTION* >( item ), 0 );
            break;

        case SCH_NO_CONNECT_T:
            saveNoConnect( static_cast< SCH_NO_CONNECT* >( item ), 0 );
            break;

        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_BUS_BUS_ENTRY_T:
            saveBusEntry( static_cast< SCH_BUS_ENTRY_BASE* >( item ), 0 );
            break;

        case SCH_LINE_T:
            saveLine( static_cast< SCH_LINE* >( item ), 0 );
            break;

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
            saveText( static_cast< SCH_TEXT* >( item ), 0 );
            break;

        default:
            wxASSERT( "Unexpected schematic object type in SCH_SEXPR_PLUGIN::Format()" );
        }
    }

    // Make all instance information relative to the selection path
    KIID_PATH selectionPath = aSelectionPath->PathWithoutRootUuid();

    selectedSheets.SortByPageNumbers();
    std::vector<SCH_SHEET_INSTANCE> sheetinstances = selectedSheets.GetSheetInstances();

    for( SCH_SHEET_INSTANCE& sheetInstance : sheetinstances )
    {
        wxASSERT_MSG( sheetInstance.m_Path.MakeRelativeTo( selectionPath ),
                      "Sheet is not inside the selection path?" );
    }


    selectedSymbols.SortByReferenceOnly();
    std::vector<SYMBOL_INSTANCE_REFERENCE> symbolInstances = selectedSymbols.GetSymbolInstances();

    for( SYMBOL_INSTANCE_REFERENCE& symbolInstance : symbolInstances )
    {
        wxASSERT_MSG( symbolInstance.m_Path.MakeRelativeTo( selectionPath ),
                      "Symbol is not inside the selection path?" );
    }

    saveInstances( sheetinstances, symbolInstances, 0 );
}


void SCH_SEXPR_PLUGIN::saveSymbol( SCH_SYMBOL* aSymbol, SCH_SHEET_PATH* aSheetPath,
                                   int aNestLevel )
{
    wxCHECK_RET( aSymbol != nullptr && m_out != nullptr, "" );

    std::string     libName;
    wxArrayString   reference_fields;

    static wxString delimiters( wxT( " " ) );

    wxString symbol_name = aSymbol->GetLibId().Format();

    if( symbol_name.size() )
    {
        libName = toUTFTildaText( symbol_name );
    }
    else
    {
        libName = "_NONAME_";
    }

    double angle;
    int orientation = aSymbol->GetOrientation() & ~( SYM_MIRROR_X | SYM_MIRROR_Y );

    if( orientation == SYM_ORIENT_90 )
        angle = 90.0;
    else if( orientation == SYM_ORIENT_180 )
        angle = 180.0;
    else if( orientation == SYM_ORIENT_270 )
        angle = 270.0;
    else
        angle = 0.0;

    m_out->Print( aNestLevel, "(symbol" );

    if( !aSymbol->UseLibIdLookup() )
    {
        m_out->Print( 0, " (lib_name %s)",
                      m_out->Quotew( aSymbol->GetSchSymbolLibraryName() ).c_str() );
    }

    m_out->Print( 0, " (lib_id %s) (at %s %s %s)",
                  m_out->Quotew( aSymbol->GetLibId().Format().wx_str() ).c_str(),
                  FormatInternalUnits( aSymbol->GetPosition().x ).c_str(),
                  FormatInternalUnits( aSymbol->GetPosition().y ).c_str(),
                  FormatAngle( angle * 10.0 ).c_str() );

    bool mirrorX = aSymbol->GetOrientation() & SYM_MIRROR_X;
    bool mirrorY = aSymbol->GetOrientation() & SYM_MIRROR_Y;

    if( mirrorX || mirrorY )
    {
        m_out->Print( 0, " (mirror" );

        if( mirrorX )
            m_out->Print( 0, " x" );

        if( mirrorY )
            m_out->Print( 0, " y" );

        m_out->Print( 0, ")" );
    }

    int unit = -1;

    if( !( aSymbol->GetInstanceReferences().size() > 1 ) )
        unit = aSymbol->GetUnit();
    else if( aSheetPath != nullptr )
        unit = aSymbol->GetUnitSelection( aSheetPath );
    if ( unit >= 0 )
        m_out->Print( 0, " (unit %d)", unit );

    if( aSymbol->GetConvert() == LIB_ITEM::LIB_CONVERT::DEMORGAN )
        m_out->Print( 0, " (convert %d)", aSymbol->GetConvert() );

    m_out->Print( 0, "\n" );

    m_out->Print( aNestLevel + 1, "(in_bom %s)", ( aSymbol->GetIncludeInBom() ) ? "yes" : "no" );
    m_out->Print( 0, " (on_board %s)", ( aSymbol->GetIncludeOnBoard() ) ? "yes" : "no" );

    if( aSymbol->GetFieldsAutoplaced() != FIELDS_AUTOPLACED_NO )
        m_out->Print( 0, " (fields_autoplaced)" );

    m_out->Print( 0, "\n" );

    m_out->Print( aNestLevel + 1, "(uuid %s)\n", TO_UTF8( aSymbol->m_Uuid.AsString() ) );

    m_nextFreeFieldId = MANDATORY_FIELDS;

    for( SCH_FIELD& field : aSymbol->GetFields() )
    {
        saveField( &field, aNestLevel + 1 );
    }

    for( const SCH_PIN* pin : aSymbol->GetPins() )
    {
        if( pin->GetAlt().IsEmpty() )
        {
            m_out->Print( aNestLevel + 1, "(pin %s (uuid %s))\n",
                          m_out->Quotew( pin->GetNumber() ).c_str(),
                          TO_UTF8( pin->m_Uuid.AsString() ) );
        }
        else
        {
            m_out->Print( aNestLevel + 1, "(pin %s (uuid %s) (alternate %s))\n",
                          m_out->Quotew( pin->GetNumber() ).c_str(),
                          TO_UTF8( pin->m_Uuid.AsString() ),
                          m_out->Quotew( pin->GetAlt() ).c_str() );
        }
    }

    m_out->Print( aNestLevel, ")\n" );
}


void SCH_SEXPR_PLUGIN::saveField( SCH_FIELD* aField, int aNestLevel )
{
    wxCHECK_RET( aField != nullptr && m_out != nullptr, "" );

    wxString fieldName = aField->GetName();

    // For some reason (bug in legacy parser?) the field ID for non-mandatory fields is -1 so
    // check for this in order to correctly use the field name.
    if( aField->GetParent()->Type() == SCH_SYMBOL_T )
    {
        if( aField->GetId() >= 0 && aField->GetId() < MANDATORY_FIELDS )
            fieldName = TEMPLATE_FIELDNAME::GetDefaultFieldName( aField->GetId(), false );
    }
    else if( aField->GetParent()->Type() == SCH_SHEET_T )
    {
        if( aField->GetId() >= 0 && aField->GetId() < SHEET_MANDATORY_FIELDS )
            fieldName = SCH_SHEET::GetDefaultFieldName( aField->GetId() );
    }

    if( aField->GetId() == -1 /* undefined ID */ )
    {
        aField->SetId( m_nextFreeFieldId );
        m_nextFreeFieldId += 1;
    }
    else if( aField->GetId() >= m_nextFreeFieldId )
    {
        m_nextFreeFieldId = aField->GetId() + 1;
    }

    m_out->Print( aNestLevel, "(property %s %s (id %d) (at %s %s %s)",
                  m_out->Quotew( fieldName ).c_str(),
                  m_out->Quotew( aField->GetText() ).c_str(),
                  aField->GetId(),
                  FormatInternalUnits( aField->GetPosition().x ).c_str(),
                  FormatInternalUnits( aField->GetPosition().y ).c_str(),
                  FormatAngle( aField->GetTextAngleDegrees() * 10.0 ).c_str() );

    if( !aField->IsDefaultFormatting()
      || ( aField->GetTextHeight() != Mils2iu( DEFAULT_SIZE_TEXT ) ) )
    {
        m_out->Print( 0, "\n" );
        aField->Format( m_out, aNestLevel, 0 );
        m_out->Print( aNestLevel, ")\n" );   // Closes property token with font effects.
    }
    else
    {
        m_out->Print( 0, ")\n" );            // Closes property token without font effects.
    }
}


void SCH_SEXPR_PLUGIN::saveBitmap( SCH_BITMAP* aBitmap, int aNestLevel )
{
    wxCHECK_RET( aBitmap != nullptr && m_out != nullptr, "" );

    const wxImage* image = aBitmap->GetImage()->GetImageData();

    wxCHECK_RET( image != nullptr, "wxImage* is NULL" );

    m_out->Print( aNestLevel, "(image (at %s %s)",
                  FormatInternalUnits( aBitmap->GetPosition().x ).c_str(),
                  FormatInternalUnits( aBitmap->GetPosition().y ).c_str() );

    if( aBitmap->GetImage()->GetScale() != 1.0 )
        m_out->Print( 0, " (scale %g)", aBitmap->GetImage()->GetScale() );

    m_out->Print( 0, "\n" );

    m_out->Print( aNestLevel + 1, "(uuid %s)\n", TO_UTF8( aBitmap->m_Uuid.AsString() ) );

    m_out->Print( aNestLevel + 1, "(data" );

    wxMemoryOutputStream stream;

    image->SaveFile( stream, wxBITMAP_TYPE_PNG );

    // Write binary data in hexadecimal form (ASCII)
    wxStreamBuffer* buffer = stream.GetOutputStreamBuffer();
    wxString out = wxBase64Encode( buffer->GetBufferStart(), buffer->GetBufferSize() );

    // Apparently the MIME standard character width for base64 encoding is 76 (unconfirmed)
    // so use it in a vein attempt to be standard like.
#define MIME_BASE64_LENGTH 76

    size_t first = 0;

    while( first < out.Length() )
    {
        m_out->Print( 0, "\n" );
        m_out->Print( aNestLevel + 2, "%s", TO_UTF8( out( first, MIME_BASE64_LENGTH ) ) );
        first += MIME_BASE64_LENGTH;
    }

    m_out->Print( 0, "\n" );
    m_out->Print( aNestLevel + 1, ")\n" );  // Closes data token.
    m_out->Print( aNestLevel, ")\n" );      // Closes image token.
}


void SCH_SEXPR_PLUGIN::saveSheet( SCH_SHEET* aSheet, int aNestLevel )
{
    wxCHECK_RET( aSheet != nullptr && m_out != nullptr, "" );

    m_out->Print( aNestLevel, "(sheet (at %s %s) (size %s %s)",
                  FormatInternalUnits( aSheet->GetPosition().x ).c_str(),
                  FormatInternalUnits( aSheet->GetPosition().y ).c_str(),
                  FormatInternalUnits( aSheet->GetSize().GetWidth() ).c_str(),
                  FormatInternalUnits( aSheet->GetSize().GetHeight() ).c_str() );

    if( aSheet->GetFieldsAutoplaced() != FIELDS_AUTOPLACED_NO )
        m_out->Print( 0, " (fields_autoplaced)" );

    m_out->Print( 0, "\n" );

    STROKE_PARAMS stroke( aSheet->GetBorderWidth(), PLOT_DASH_TYPE::SOLID,
                          aSheet->GetBorderColor() );

    stroke.SetWidth( aSheet->GetBorderWidth() );
    formatStroke( m_out, aNestLevel + 1, stroke );

    m_out->Print( 0, "\n" );

    m_out->Print( aNestLevel + 1, "(fill (color %d %d %d %0.4f))\n",
                  KiROUND( aSheet->GetBackgroundColor().r * 255.0 ),
                  KiROUND( aSheet->GetBackgroundColor().g * 255.0 ),
                  KiROUND( aSheet->GetBackgroundColor().b * 255.0 ),
                  aSheet->GetBackgroundColor().a );

    m_out->Print( aNestLevel + 1, "(uuid %s)\n", TO_UTF8( aSheet->m_Uuid.AsString() ) );

    m_nextFreeFieldId = SHEET_MANDATORY_FIELDS;

    for( SCH_FIELD& field : aSheet->GetFields() )
    {
        saveField( &field, aNestLevel + 1 );
    }

    for( const SCH_SHEET_PIN* pin : aSheet->GetPins() )
    {
        m_out->Print( aNestLevel + 1, "(pin %s %s (at %s %s %s)\n",
                      EscapedUTF8( pin->GetText() ).c_str(),
                      getSheetPinShapeToken( pin->GetShape() ),
                      FormatInternalUnits( pin->GetPosition().x ).c_str(),
                      FormatInternalUnits( pin->GetPosition().y ).c_str(),
                      FormatAngle( getSheetPinAngle( pin->GetEdge() ) * 10.0 ).c_str() );

        pin->Format( m_out, aNestLevel + 1, 0 );

        m_out->Print( aNestLevel + 2, "(uuid %s)\n", TO_UTF8( pin->m_Uuid.AsString() ) );

        m_out->Print( aNestLevel + 1, ")\n" );  // Closes pin token.
    }

    m_out->Print( aNestLevel, ")\n" );          // Closes sheet token.
}


void SCH_SEXPR_PLUGIN::saveJunction( SCH_JUNCTION* aJunction, int aNestLevel )
{
    wxCHECK_RET( aJunction != nullptr && m_out != nullptr, "" );

    m_out->Print( aNestLevel, "(junction (at %s %s) (diameter %s) (color %d %d %d %s))\n",
                  FormatInternalUnits( aJunction->GetPosition().x ).c_str(),
                  FormatInternalUnits( aJunction->GetPosition().y ).c_str(),
                  FormatInternalUnits( aJunction->GetDiameter() ).c_str(),
                  KiROUND( aJunction->GetColor().r * 255.0 ),
                  KiROUND( aJunction->GetColor().g * 255.0 ),
                  KiROUND( aJunction->GetColor().b * 255.0 ),
                  Double2Str( aJunction->GetColor().a ).c_str() );
}


void SCH_SEXPR_PLUGIN::saveNoConnect( SCH_NO_CONNECT* aNoConnect, int aNestLevel )
{
    wxCHECK_RET( aNoConnect != nullptr && m_out != nullptr, "" );

    m_out->Print( aNestLevel, "(no_connect (at %s %s) (uuid %s))\n",
                  FormatInternalUnits( aNoConnect->GetPosition().x ).c_str(),
                  FormatInternalUnits( aNoConnect->GetPosition().y ).c_str(),
                  TO_UTF8( aNoConnect->m_Uuid.AsString() ) );
}


void SCH_SEXPR_PLUGIN::saveBusEntry( SCH_BUS_ENTRY_BASE* aBusEntry, int aNestLevel )
{
    wxCHECK_RET( aBusEntry != nullptr && m_out != nullptr, "" );

    // Bus to bus entries are converted to bus line segments.
    if( aBusEntry->GetClass() == "SCH_BUS_BUS_ENTRY" )
    {
        SCH_LINE busEntryLine( aBusEntry->GetPosition(), LAYER_BUS );

        busEntryLine.SetEndPoint( aBusEntry->GetEnd() );
        saveLine( &busEntryLine, aNestLevel );
    }
    else
    {
        m_out->Print( aNestLevel, "(bus_entry (at %s %s) (size %s %s)\n",
                      FormatInternalUnits( aBusEntry->GetPosition().x ).c_str(),
                      FormatInternalUnits( aBusEntry->GetPosition().y ).c_str(),
                      FormatInternalUnits( aBusEntry->GetSize().GetWidth() ).c_str(),
                      FormatInternalUnits( aBusEntry->GetSize().GetHeight() ).c_str() );

        formatStroke( m_out, aNestLevel + 1, aBusEntry->GetStroke() );

        m_out->Print( 0, "\n" );

        m_out->Print( aNestLevel + 1, "(uuid %s)\n", TO_UTF8( aBusEntry->m_Uuid.AsString() ) );

        m_out->Print( aNestLevel, ")\n" );
    }
}


void SCH_SEXPR_PLUGIN::saveLine( SCH_LINE* aLine, int aNestLevel )
{
    wxCHECK_RET( aLine != nullptr && m_out != nullptr, "" );

    wxString lineType;

    STROKE_PARAMS line_stroke = aLine->GetStroke();

    switch( aLine->GetLayer() )
    {
    case LAYER_BUS:     lineType = "bus";       break;
    case LAYER_WIRE:    lineType = "wire";      break;
    case LAYER_NOTES:
    default:            lineType = "polyline";  break;
    }

    m_out->Print( aNestLevel, "(%s (pts (xy %s %s) (xy %s %s))\n",
                  TO_UTF8( lineType ),
                  FormatInternalUnits( aLine->GetStartPoint().x ).c_str(),
                  FormatInternalUnits( aLine->GetStartPoint().y ).c_str(),
                  FormatInternalUnits( aLine->GetEndPoint().x ).c_str(),
                  FormatInternalUnits( aLine->GetEndPoint().y ).c_str() );

    formatStroke( m_out, aNestLevel + 1, line_stroke );
    m_out->Print( 0, "\n" );

    m_out->Print( aNestLevel + 1, "(uuid %s)\n", TO_UTF8( aLine->m_Uuid.AsString() ) );

    m_out->Print( aNestLevel, ")\n" );
}


void SCH_SEXPR_PLUGIN::saveText( SCH_TEXT* aText, int aNestLevel )
{
    wxCHECK_RET( aText != nullptr && m_out != nullptr, "" );

    double angle;

    switch( aText->GetLabelSpinStyle() )
    {
    case LABEL_SPIN_STYLE::RIGHT:    angle = 0.0;    break;
    case LABEL_SPIN_STYLE::UP:       angle = 90.0;   break;
    case LABEL_SPIN_STYLE::LEFT:     angle = 180.0;  break;
    case LABEL_SPIN_STYLE::BOTTOM:   angle = 270.0;  break;
    default:      wxFAIL;            angle = 0.0;    break;
    }

    m_out->Print( aNestLevel, "(%s %s",
                  getTextTypeToken( aText->Type() ),
                  m_out->Quotew( aText->GetText() ).c_str() );

    if( ( aText->Type() == SCH_GLOBAL_LABEL_T ) || ( aText->Type() == SCH_HIER_LABEL_T ) )
        m_out->Print( 0, " (shape %s)", getSheetPinShapeToken( aText->GetShape() ) );

    if( aText->GetText().Length() < 50 )
    {
        m_out->Print( 0, " (at %s %s %s)",
                      FormatInternalUnits( aText->GetPosition().x ).c_str(),
                      FormatInternalUnits( aText->GetPosition().y ).c_str(),
                      FormatAngle( angle * 10.0 ).c_str() );
    }
    else
    {
        m_out->Print( 0, "\n" );
        m_out->Print( aNestLevel + 1, "(at %s %s %s)",
                      FormatInternalUnits( aText->GetPosition().x ).c_str(),
                      FormatInternalUnits( aText->GetPosition().y ).c_str(),
                      FormatAngle( aText->GetTextAngle() ).c_str() );
    }

    if( aText->GetFieldsAutoplaced() != FIELDS_AUTOPLACED_NO )
        m_out->Print( 0, " (fields_autoplaced)" );

    m_out->Print( 0, "\n" );
    aText->Format( m_out, aNestLevel, 0 );

    m_out->Print( aNestLevel + 1, "(uuid %s)\n", TO_UTF8( aText->m_Uuid.AsString() ) );

    if( ( aText->Type() == SCH_GLOBAL_LABEL_T ) )
    {
        SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( aText );
        saveField( label->GetIntersheetRefs(), aNestLevel + 1 );
    }

    m_out->Print( aNestLevel, ")\n" );   // Closes text token.
}


void SCH_SEXPR_PLUGIN::saveBusAlias( std::shared_ptr<BUS_ALIAS> aAlias, int aNestLevel )
{
    wxCHECK_RET( aAlias != nullptr, "BUS_ALIAS* is NULL" );

    wxString members;

    for( auto member : aAlias->Members() )
    {
        if( members.IsEmpty() )
            members = m_out->Quotew( member );
        else
            members += " " + m_out->Quotew( member );
    }

    m_out->Print( aNestLevel, "(bus_alias %s (members %s))\n",
                  m_out->Quotew( aAlias->GetName() ).c_str(),
                  TO_UTF8( members ) );
}


void SCH_SEXPR_PLUGIN::saveInstances( const std::vector<SCH_SHEET_INSTANCE>& aSheets,
                                      const std::vector<SYMBOL_INSTANCE_REFERENCE>& aSymbols,
                                      int aNestLevel )
{
    if( aSheets.size() )
    {
        m_out->Print( 0, "\n" );
        m_out->Print( aNestLevel, "(sheet_instances\n" );

        for( const SCH_SHEET_INSTANCE& instance : aSheets )
        {
            wxString path = instance.m_Path.AsString();

            if( path.IsEmpty() )
                path = wxT( "/" ); // Root path

            m_out->Print( aNestLevel + 1, "(path %s (page %s))\n",
                          m_out->Quotew( path ).c_str(),
                          m_out->Quotew( instance.m_PageNumber ).c_str() );
        }

        m_out->Print( aNestLevel, ")\n" ); // Close sheet instances token.
    }

    if( aSymbols.size() )
    {
        m_out->Print( 0, "\n" );
        m_out->Print( aNestLevel, "(symbol_instances\n" );

        for( const SYMBOL_INSTANCE_REFERENCE& instance : aSymbols )
        {
            m_out->Print( aNestLevel + 1, "(path %s\n",
                          m_out->Quotew( instance.m_Path.AsString() ).c_str() );
            m_out->Print( aNestLevel + 2, "(reference %s) (unit %d) (value %s) (footprint %s)\n",
                          m_out->Quotew( instance.m_Reference ).c_str(),
                          instance.m_Unit,
                          m_out->Quotew( instance.m_Value ).c_str(),
                          m_out->Quotew( instance.m_Footprint ).c_str() );
            m_out->Print( aNestLevel + 1, ")\n" );
        }

        m_out->Print( aNestLevel, ")\n" ); // Close symbol instances token.
    }
}


int SCH_SEXPR_PLUGIN_CACHE::m_modHash = 1;     // starts at 1 and goes up


SCH_SEXPR_PLUGIN_CACHE::SCH_SEXPR_PLUGIN_CACHE( const wxString& aFullPathAndFileName ) :
    m_fileName( aFullPathAndFileName ),
    m_libFileName( aFullPathAndFileName ),
    m_isWritable( true ),
    m_isModified( false )
{
    m_versionMajor = -1;
    m_libType      = SCH_LIB_TYPE::LT_EESCHEMA;
}


SCH_SEXPR_PLUGIN_CACHE::~SCH_SEXPR_PLUGIN_CACHE()
{
    // When the cache is destroyed, all of the alias objects on the heap should be deleted.
    for( LIB_SYMBOL_MAP::iterator it = m_symbols.begin();  it != m_symbols.end();  ++it )
        delete it->second;

    m_symbols.clear();
}


// If m_libFileName is a symlink follow it to the real source file
wxFileName SCH_SEXPR_PLUGIN_CACHE::GetRealFile() const
{
    wxFileName fn( m_libFileName );
    WX_FILENAME::ResolvePossibleSymlinks( fn );
    return fn;
}


wxDateTime SCH_SEXPR_PLUGIN_CACHE::GetLibModificationTime()
{
    wxFileName fn = GetRealFile();

    // update the writable flag while we have a wxFileName, in a network this
    // is possibly quite dynamic anyway.
    m_isWritable = fn.IsFileWritable();

    return fn.GetModificationTime();
}


bool SCH_SEXPR_PLUGIN_CACHE::IsFile( const wxString& aFullPathAndFileName ) const
{
    return m_fileName == aFullPathAndFileName;
}


bool SCH_SEXPR_PLUGIN_CACHE::IsFileChanged() const
{
    wxFileName fn = GetRealFile();

    if( m_fileModTime.IsValid() && fn.IsOk() && fn.FileExists() )
        return fn.GetModificationTime() != m_fileModTime;

    return false;
}


LIB_SYMBOL* SCH_SEXPR_PLUGIN_CACHE::removeSymbol( LIB_SYMBOL* aSymbol )
{
    wxCHECK_MSG( aSymbol != nullptr, nullptr, "NULL pointer cannot be removed from library." );

    LIB_SYMBOL* firstChild = nullptr;
    LIB_SYMBOL_MAP::iterator it = m_symbols.find( aSymbol->GetName() );

    if( it == m_symbols.end() )
        return nullptr;

    // If the entry pointer doesn't match the name it is mapped to in the library, we
    // have done something terribly wrong.
    wxCHECK_MSG( *it->second == aSymbol, nullptr,
                 "Pointer mismatch while attempting to remove alias entry <" + aSymbol->GetName() +
                 "> from library cache <" + m_libFileName.GetName() + ">." );

    // If the symbol is a root symbol used by other symbols find the first alias that uses
    // the root symbol and make it the new root.
    if( aSymbol->IsRoot() )
    {
        for( auto entry : m_symbols )
        {
            if( entry.second->IsAlias()
              && entry.second->GetParent().lock() == aSymbol->SharedPtr() )
            {
                firstChild = entry.second;
                break;
            }
        }

        if( firstChild )
        {
            for( LIB_ITEM& drawItem : aSymbol->GetDrawItems() )
            {
                if( drawItem.Type() == LIB_FIELD_T )
                {
                    LIB_FIELD& field = static_cast<LIB_FIELD&>( drawItem );

                    if( firstChild->FindField( field.GetCanonicalName() ) )
                        continue;
                }

                LIB_ITEM* newItem = (LIB_ITEM*) drawItem.Clone();
                drawItem.SetParent( firstChild );
                firstChild->AddDrawItem( newItem );
            }

            // Reparent the remaining aliases.
            for( auto entry : m_symbols )
            {
                if( entry.second->IsAlias()
                  && entry.second->GetParent().lock() == aSymbol->SharedPtr() )
                    entry.second->SetParent( firstChild );
            }
        }
    }

    m_symbols.erase( it );
    delete aSymbol;
    m_isModified = true;
    ++m_modHash;
    return firstChild;
}


void SCH_SEXPR_PLUGIN_CACHE::AddSymbol( const LIB_SYMBOL* aSymbol )
{
    // aSymbol is cloned in SYMBOL_LIB::AddSymbol().  The cache takes ownership of aSymbol.
    wxString name = aSymbol->GetName();
    LIB_SYMBOL_MAP::iterator it = m_symbols.find( name );

    if( it != m_symbols.end() )
    {
        removeSymbol( it->second );
    }

    m_symbols[ name ] = const_cast< LIB_SYMBOL* >( aSymbol );
    m_isModified = true;
    ++m_modHash;
}


void SCH_SEXPR_PLUGIN_CACHE::Load()
{
    if( !m_libFileName.FileExists() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Library file '%s' not found." ),
                                          m_libFileName.GetFullPath() ) );
    }

    wxCHECK_RET( m_libFileName.IsAbsolute(),
                 wxString::Format( "Cannot use relative file paths in sexpr plugin to "
                                   "open library '%s'.", m_libFileName.GetFullPath() ) );

    // The current locale must use period as the decimal point.
    wxCHECK2( wxLocale::GetInfo( wxLOCALE_DECIMAL_POINT, wxLOCALE_CAT_NUMBER ) == ".",
              LOCALE_IO toggle );

    wxLogTrace( traceSchLegacyPlugin, "Loading sexpr symbol library file '%s'",
                m_libFileName.GetFullPath() );

    FILE_LINE_READER reader( m_libFileName.GetFullPath() );

    SCH_SEXPR_PARSER parser( &reader );

    parser.ParseLib( m_symbols );
    ++m_modHash;

    // Remember the file modification time of library file when the
    // cache snapshot was made, so that in a networked environment we will
    // reload the cache as needed.
    m_fileModTime = GetLibModificationTime();
}


void SCH_SEXPR_PLUGIN_CACHE::Save()
{
    if( !m_isModified )
        return;

    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    // Write through symlinks, don't replace them.
    wxFileName fn = GetRealFile();

    auto formatter = std::make_unique<FILE_OUTPUTFORMATTER>( fn.GetFullPath() );

    formatter->Print( 0, "(kicad_symbol_lib (version %d) (generator kicad_symbol_editor)\n",
                      SEXPR_SYMBOL_LIB_FILE_VERSION );

    for( auto parent : m_symbols )
    {
        // Save the root symbol first so alias can inherit from them.
        if( parent.second->IsRoot() )
        {
            SaveSymbol( parent.second, *formatter.get(), 1 );

            // Save all of the aliases associated with the current root symbol.
            for( auto alias : m_symbols )
            {
                if( !alias.second->IsAlias() )
                    continue;

                std::shared_ptr<LIB_SYMBOL> aliasParent = alias.second->GetParent().lock();

                if( aliasParent.get() != parent.second )
                    continue;

                SaveSymbol( alias.second, *formatter.get(), 1 );
            }
        }
    }

    formatter->Print( 0, ")\n" );

    formatter.reset();

    m_fileModTime = fn.GetModificationTime();
    m_isModified = false;
}


void SCH_SEXPR_PLUGIN_CACHE::SaveSymbol( LIB_SYMBOL* aSymbol, OUTPUTFORMATTER& aFormatter,
                                         int aNestLevel, const wxString& aLibName )
{
    wxCHECK_RET( aSymbol, "Invalid LIB_SYMBOL pointer." );

    // The current locale must use period as the decimal point.
    wxCHECK2( wxLocale::GetInfo( wxLOCALE_DECIMAL_POINT, wxLOCALE_CAT_NUMBER ) == ".",
              LOCALE_IO toggle );

    int nextFreeFieldId = MANDATORY_FIELDS;
    std::vector<LIB_FIELD*> fields;
    std::string name = aFormatter.Quotew( aSymbol->GetLibId().Format().wx_str() );
    std::string unitName = aSymbol->GetLibId().GetLibItemName();

    if( !aLibName.IsEmpty() )
    {
        name = aFormatter.Quotew( aLibName );

        LIB_ID unitId;

        wxCHECK2( unitId.Parse( aLibName ) < 0, /* do nothing */ );

        unitName = unitId.GetLibItemName();
    }

    if( aSymbol->IsRoot() )
    {
        aFormatter.Print( aNestLevel, "(symbol %s", name.c_str() );

        if( aSymbol->IsPower() )
            aFormatter.Print( 0, " (power)" );

        // TODO: add uuid token here.

        // TODO: add anchor position token here.

        if( !aSymbol->ShowPinNumbers() )
            aFormatter.Print( 0, " (pin_numbers hide)" );

        if( aSymbol->GetPinNameOffset() != Mils2iu( DEFAULT_PIN_NAME_OFFSET )
          || !aSymbol->ShowPinNames() )
        {
            aFormatter.Print( 0, " (pin_names" );

            if( aSymbol->GetPinNameOffset() != Mils2iu( DEFAULT_PIN_NAME_OFFSET ) )
                aFormatter.Print( 0, " (offset %s)",
                                  FormatInternalUnits( aSymbol->GetPinNameOffset() ).c_str() );

            if( !aSymbol->ShowPinNames() )
                aFormatter.Print( 0, " hide" );

            aFormatter.Print( 0, ")" );
        }

        aFormatter.Print( 0, " (in_bom %s)", ( aSymbol->GetIncludeInBom() ) ? "yes" : "no" );
        aFormatter.Print( 0, " (on_board %s)", ( aSymbol->GetIncludeOnBoard() ) ? "yes" : "no" );

        // TODO: add atomic token here.

        // TODO: add required token here."

        aFormatter.Print( 0, "\n" );

        aSymbol->GetFields( fields );

        for( LIB_FIELD* field : fields )
            saveField( field, aFormatter, nextFreeFieldId, aNestLevel + 1 );

        // @todo At some point in the future the lock status (all units interchangeable) should
        // be set deterministically.  For now a custom lock property is used to preserve the
        // locked flag state.
        if( aSymbol->UnitsLocked() )
        {
            LIB_FIELD locked( -1, "ki_locked" );
            saveField( &locked, aFormatter, nextFreeFieldId, aNestLevel + 1 );
        }

        saveDcmInfoAsFields( aSymbol, aFormatter, nextFreeFieldId, aNestLevel );

        // Save the draw items grouped by units.
        std::vector<LIB_SYMBOL_UNITS> units = aSymbol->GetUnitDrawItems();
        std::sort( units.begin(), units.end(),
                   []( const LIB_SYMBOL_UNITS& a, const LIB_SYMBOL_UNITS& b )
                   {
                        if( a.m_unit == b.m_unit )
                            return a.m_convert < b.m_convert;

                        return a.m_unit < b.m_unit;
                   } );

        for( auto unit : units )
        {
            // Add quotes and escape chars like ") to the UTF8 unitName string
            name = aFormatter.Quotes( unitName );
            name.pop_back();    // Remove last char: the quote ending the string.

            aFormatter.Print( aNestLevel + 1, "(symbol %s_%d_%d\"\n",
                              name.c_str(), unit.m_unit, unit.m_convert );

            for( auto item : unit.m_items )
                saveSymbolDrawItem( item, aFormatter, aNestLevel + 2 );

            aFormatter.Print( aNestLevel + 1, ")\n" );
        }
    }
    else
    {
        std::shared_ptr<LIB_SYMBOL> parent = aSymbol->GetParent().lock();

        wxASSERT( parent );

        aFormatter.Print( aNestLevel, "(symbol %s (extends %s)\n",
                          name.c_str(),
                          aFormatter.Quotew( parent->GetName() ).c_str() );

        aSymbol->GetFields( fields );

        for( LIB_FIELD* field : fields )
            saveField( field, aFormatter, nextFreeFieldId, aNestLevel + 1 );

        saveDcmInfoAsFields( aSymbol, aFormatter, nextFreeFieldId, aNestLevel );
    }

    aFormatter.Print( aNestLevel, ")\n" );
}


void SCH_SEXPR_PLUGIN_CACHE::saveDcmInfoAsFields( LIB_SYMBOL* aSymbol, OUTPUTFORMATTER& aFormatter,
                                                  int& aNextFreeFieldId, int aNestLevel )
{
    wxCHECK_RET( aSymbol, "Invalid LIB_SYMBOL pointer." );

    if( !aSymbol->GetKeyWords().IsEmpty() )
    {
        LIB_FIELD keywords( -1, wxString( "ki_keywords" ) );
        keywords.SetVisible( false );
        keywords.SetText( aSymbol->GetKeyWords() );
        saveField( &keywords, aFormatter, aNextFreeFieldId, aNestLevel + 1 );
    }

    if( !aSymbol->GetDescription().IsEmpty() )
    {
        LIB_FIELD description( -1, wxString( "ki_description" ) );
        description.SetVisible( false );
        description.SetText( aSymbol->GetDescription() );
        saveField( &description, aFormatter, aNextFreeFieldId, aNestLevel + 1 );
    }

    wxArrayString fpFilters = aSymbol->GetFPFilters();

    if( !fpFilters.IsEmpty() )
    {
        wxString tmp;

        for( auto filter : fpFilters )
        {
            // Spaces are not handled in fp filter names so escape spaces if any
            wxString curr_filter = EscapeString( filter, ESCAPE_CONTEXT::CTX_NO_SPACE );

            if( tmp.IsEmpty() )
                tmp = curr_filter;
            else
                tmp += " " + curr_filter;
        }

        LIB_FIELD description( -1, wxString( "ki_fp_filters" ) );
        description.SetVisible( false );
        description.SetText( tmp );
        saveField( &description, aFormatter, aNextFreeFieldId, aNestLevel + 1 );
    }
}


void SCH_SEXPR_PLUGIN_CACHE::saveSymbolDrawItem( LIB_ITEM* aItem, OUTPUTFORMATTER& aFormatter,
                                                 int aNestLevel )
{
    wxCHECK_RET( aItem, "Invalid LIB_ITEM pointer." );

    switch( aItem->Type() )
    {
    case LIB_SHAPE_T:
    {
        LIB_SHAPE*    shape = static_cast<LIB_SHAPE*>( aItem );
        STROKE_PARAMS stroke;
        FILL_T        fillMode = shape->GetFillType();

        stroke.SetWidth( shape->GetWidth() );

        switch( shape->GetShape() )
        {
        case SHAPE_T::ARC:
            int x1;
            int x2;

            shape->CalcArcAngles( x1, x2 );

            formatArc( &aFormatter, aNestLevel, shape, x1, x2, stroke, fillMode, COLOR4D()  );
            break;

        case SHAPE_T::CIRCLE:
            formatCircle( &aFormatter, aNestLevel, shape, stroke, fillMode, COLOR4D() );
            break;

        case SHAPE_T::RECT:
            formatRect( &aFormatter, aNestLevel, shape, stroke, fillMode, COLOR4D() );
            break;

        case SHAPE_T::BEZIER:
            formatBezier(&aFormatter, aNestLevel, shape, stroke, fillMode, COLOR4D() );
            break;

        case SHAPE_T::POLY:
            formatPoly( &aFormatter, aNestLevel, shape, stroke, fillMode, COLOR4D() );
            break;

        default:
            UNIMPLEMENTED_FOR( shape->SHAPE_T_asString() );
        }

        break;
    }

    case LIB_PIN_T:
        savePin( (LIB_PIN* ) aItem, aFormatter, aNestLevel );
        break;

    case LIB_TEXT_T:
        saveText( ( LIB_TEXT* ) aItem, aFormatter, aNestLevel );
        break;

    default:
        UNIMPLEMENTED_FOR( aItem->GetClass() );
    }
}


void SCH_SEXPR_PLUGIN_CACHE::saveField( LIB_FIELD* aField, OUTPUTFORMATTER& aFormatter,
                                        int& aNextFreeFieldId, int aNestLevel  )
{
    wxCHECK_RET( aField && aField->Type() == LIB_FIELD_T, "Invalid LIB_FIELD object." );

    wxString fieldName = aField->GetName();

    if( aField->GetId() == -1 /* undefined ID */ )
    {
        aField->SetId( aNextFreeFieldId );
        aNextFreeFieldId += 1;
    }
    else if( aField->GetId() >= aNextFreeFieldId )
    {
        aNextFreeFieldId = aField->GetId() + 1;
    }

    if( aField->GetId() >= 0 && aField->GetId() < MANDATORY_FIELDS )
        fieldName = TEMPLATE_FIELDNAME::GetDefaultFieldName( aField->GetId(), false );

    aFormatter.Print( aNestLevel, "(property %s %s (id %d) (at %s %s %g)\n",
                      aFormatter.Quotew( fieldName ).c_str(),
                      aFormatter.Quotew( aField->GetText() ).c_str(),
                      aField->GetId(),
                      FormatInternalUnits( aField->GetPosition().x ).c_str(),
                      FormatInternalUnits( aField->GetPosition().y ).c_str(),
                      static_cast<double>( aField->GetTextAngle() ) / 10.0 );

    aField->Format( &aFormatter, aNestLevel, 0 );
    aFormatter.Print( aNestLevel, ")\n" );
}


void SCH_SEXPR_PLUGIN_CACHE::savePin( LIB_PIN* aPin, OUTPUTFORMATTER& aFormatter, int aNestLevel )
{
    wxCHECK_RET( aPin && aPin->Type() == LIB_PIN_T, "Invalid LIB_PIN object." );

    aPin->ClearFlags( IS_CHANGED );

    aFormatter.Print( aNestLevel, "(pin %s %s (at %s %s %s) (length %s)",
                      getPinElectricalTypeToken( aPin->GetType() ),
                      getPinShapeToken( aPin->GetShape() ),
                      FormatInternalUnits( aPin->GetPosition().x ).c_str(),
                      FormatInternalUnits( aPin->GetPosition().y ).c_str(),
                      FormatAngle( getPinAngle( aPin->GetOrientation() ) * 10.0 ).c_str(),
                      FormatInternalUnits( aPin->GetLength() ).c_str() );

    if( !aPin->IsVisible() )
        aFormatter.Print( 0, " hide\n" );
    else
        aFormatter.Print( 0, "\n" );

    // This follows the EDA_TEXT effects formatting for future expansion.
    aFormatter.Print( aNestLevel + 1, "(name %s (effects (font (size %s %s))))\n",
                      aFormatter.Quotew( aPin->GetName() ).c_str(),
                      FormatInternalUnits( aPin->GetNameTextSize() ).c_str(),
                      FormatInternalUnits( aPin->GetNameTextSize() ).c_str() );

    aFormatter.Print( aNestLevel + 1, "(number %s (effects (font (size %s %s))))\n",
                      aFormatter.Quotew( aPin->GetNumber() ).c_str(),
                      FormatInternalUnits( aPin->GetNumberTextSize() ).c_str(),
                      FormatInternalUnits( aPin->GetNumberTextSize() ).c_str() );


    for( const std::pair<const wxString, LIB_PIN::ALT>& alt : aPin->GetAlternates() )
    {
        aFormatter.Print( aNestLevel + 1, "(alternate %s %s %s)\n",
                          aFormatter.Quotew( alt.second.m_Name ).c_str(),
                          getPinElectricalTypeToken( alt.second.m_Type ),
                          getPinShapeToken( alt.second.m_Shape ) );
    }

    aFormatter.Print( aNestLevel, ")\n" );
}


void SCH_SEXPR_PLUGIN_CACHE::saveText( LIB_TEXT* aText, OUTPUTFORMATTER& aFormatter,
                                       int aNestLevel )
{
    wxCHECK_RET( aText && aText->Type() == LIB_TEXT_T, "Invalid LIB_TEXT object." );

    aFormatter.Print( aNestLevel, "(text %s (at %s %s %g)\n",
                      aFormatter.Quotew( aText->GetText() ).c_str(),
                      FormatInternalUnits( aText->GetPosition().x ).c_str(),
                      FormatInternalUnits( aText->GetPosition().y ).c_str(),
                      aText->GetTextAngle() );
    aText->Format( &aFormatter, aNestLevel, 0 );
    aFormatter.Print( aNestLevel, ")\n" );
}


void SCH_SEXPR_PLUGIN_CACHE::DeleteSymbol( const wxString& aSymbolName )
{
    LIB_SYMBOL_MAP::iterator it = m_symbols.find( aSymbolName );

    if( it == m_symbols.end() )
        THROW_IO_ERROR( wxString::Format( _( "library %s does not contain a symbol named %s" ),
                                          m_libFileName.GetFullName(), aSymbolName ) );

    LIB_SYMBOL* symbol = it->second;

    if( symbol->IsRoot() )
    {
        LIB_SYMBOL* rootSymbol = symbol;

        // Remove the root symbol and all its children.
        m_symbols.erase( it );

        LIB_SYMBOL_MAP::iterator it1 = m_symbols.begin();

        while( it1 != m_symbols.end() )
        {
            if( it1->second->IsAlias()
              && it1->second->GetParent().lock() == rootSymbol->SharedPtr() )
            {
                delete it1->second;
                it1 = m_symbols.erase( it1 );
            }
            else
            {
                it1++;
            }
        }

        delete rootSymbol;
    }
    else
    {
        // Just remove the alias.
        m_symbols.erase( it );
        delete symbol;
    }

    ++m_modHash;
    m_isModified = true;
}


void SCH_SEXPR_PLUGIN::cacheLib( const wxString& aLibraryFileName, const PROPERTIES* aProperties )
{
    if( !m_cache || !m_cache->IsFile( aLibraryFileName ) || m_cache->IsFileChanged() )
    {
        // a spectacular episode in memory management:
        delete m_cache;
        m_cache = new SCH_SEXPR_PLUGIN_CACHE( aLibraryFileName );

        // Because m_cache is rebuilt, increment SYMBOL_LIBS::s_modify_generation
        // to modify the hash value that indicate symbol to symbol links
        // must be updated.
        SYMBOL_LIBS::IncrementModifyGeneration();

        if( !isBuffering( aProperties ) )
            m_cache->Load();
    }
}


bool SCH_SEXPR_PLUGIN::isBuffering( const PROPERTIES* aProperties )
{
    return ( aProperties && aProperties->Exists( SCH_SEXPR_PLUGIN::PropBuffering ) );
}


int SCH_SEXPR_PLUGIN::GetModifyHash() const
{
    if( m_cache )
        return m_cache->GetModifyHash();

    // If the cache hasn't been loaded, it hasn't been modified.
    return 0;
}


void SCH_SEXPR_PLUGIN::EnumerateSymbolLib( wxArrayString&    aSymbolNameList,
                                           const wxString&   aLibraryPath,
                                           const PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    bool powerSymbolsOnly = ( aProperties &&
                              aProperties->find( SYMBOL_LIB_TABLE::PropPowerSymsOnly ) != aProperties->end() );

    cacheLib( aLibraryPath, aProperties );

    const LIB_SYMBOL_MAP& symbols = m_cache->m_symbols;

    for( LIB_SYMBOL_MAP::const_iterator it = symbols.begin();  it != symbols.end();  ++it )
    {
        if( !powerSymbolsOnly || it->second->IsPower() )
            aSymbolNameList.Add( it->first );
    }
}


void SCH_SEXPR_PLUGIN::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                           const wxString&   aLibraryPath,
                                           const PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    bool powerSymbolsOnly = ( aProperties &&
                              aProperties->find( SYMBOL_LIB_TABLE::PropPowerSymsOnly ) != aProperties->end() );

    cacheLib( aLibraryPath, aProperties );

    const LIB_SYMBOL_MAP& symbols = m_cache->m_symbols;

    for( LIB_SYMBOL_MAP::const_iterator it = symbols.begin();  it != symbols.end();  ++it )
    {
        if( !powerSymbolsOnly || it->second->IsPower() )
            aSymbolList.push_back( it->second );
    }
}


LIB_SYMBOL* SCH_SEXPR_PLUGIN::LoadSymbol( const wxString& aLibraryPath, const wxString& aSymbolName,
                                          const PROPERTIES* aProperties )
{
    LOCALE_IO toggle;     // toggles on, then off, the C locale.

    cacheLib( aLibraryPath, aProperties );

    LIB_SYMBOL_MAP::const_iterator it = m_cache->m_symbols.find( aSymbolName );

    if( it == m_cache->m_symbols.end() )
        return nullptr;

    return it->second;
}


void SCH_SEXPR_PLUGIN::SaveSymbol( const wxString& aLibraryPath, const LIB_SYMBOL* aSymbol,
                                   const PROPERTIES* aProperties )
{
    LOCALE_IO toggle;     // toggles on, then off, the C locale.

    cacheLib( aLibraryPath, aProperties );

    m_cache->AddSymbol( aSymbol );

    if( !isBuffering( aProperties ) )
        m_cache->Save();
}


void SCH_SEXPR_PLUGIN::DeleteSymbol( const wxString& aLibraryPath, const wxString& aSymbolName,
                                     const PROPERTIES* aProperties )
{
    LOCALE_IO toggle;     // toggles on, then off, the C locale.

    cacheLib( aLibraryPath, aProperties );

    m_cache->DeleteSymbol( aSymbolName );

    if( !isBuffering( aProperties ) )
        m_cache->Save();
}


void SCH_SEXPR_PLUGIN::CreateSymbolLib( const wxString& aLibraryPath,
                                        const PROPERTIES* aProperties )
{
    if( wxFileExists( aLibraryPath ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "Symbol library '%s' already exists." ),
                                          aLibraryPath.GetData() ) );
    }

    LOCALE_IO toggle;

    delete m_cache;
    m_cache = new SCH_SEXPR_PLUGIN_CACHE( aLibraryPath );
    m_cache->SetModified();
    m_cache->Save();
    m_cache->Load();    // update m_writable and m_mod_time
}


bool SCH_SEXPR_PLUGIN::DeleteSymbolLib( const wxString& aLibraryPath,
                                        const PROPERTIES* aProperties )
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


void SCH_SEXPR_PLUGIN::SaveLibrary( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    if( !m_cache )
        m_cache = new SCH_SEXPR_PLUGIN_CACHE( aLibraryPath );

    wxString oldFileName = m_cache->GetFileName();

    if( !m_cache->IsFile( aLibraryPath ) )
    {
        m_cache->SetFileName( aLibraryPath );
    }

    // This is a forced save.
    m_cache->SetModified();
    m_cache->Save();
    m_cache->SetFileName( oldFileName );
}


bool SCH_SEXPR_PLUGIN::CheckHeader( const wxString& aFileName )
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


bool SCH_SEXPR_PLUGIN::IsSymbolLibWritable( const wxString& aLibraryPath )
{
    wxFileName fn( aLibraryPath );

    return ( fn.FileExists() && fn.IsFileWritable() ) || fn.IsDirWritable();
}


LIB_SYMBOL* SCH_SEXPR_PLUGIN::ParseLibSymbol( LINE_READER& aReader, int aFileVersion )
{
    LOCALE_IO toggle;     // toggles on, then off, the C locale.
    LIB_SYMBOL_MAP map;
    SCH_SEXPR_PARSER parser( &aReader );

    parser.NeedLEFT();
    parser.NextTok();

    return parser.ParseSymbol( map, aFileVersion );
}


void SCH_SEXPR_PLUGIN::FormatLibSymbol( LIB_SYMBOL* symbol, OUTPUTFORMATTER & formatter )
{

    LOCALE_IO toggle;     // toggles on, then off, the C locale.
    SCH_SEXPR_PLUGIN_CACHE::SaveSymbol( symbol, formatter );
}


const char* SCH_SEXPR_PLUGIN::PropBuffering = "buffering";
