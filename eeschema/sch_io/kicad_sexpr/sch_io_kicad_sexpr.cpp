/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright (C) 2021-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <base_units.h>
#include <build_version.h>
#include <trace_helpers.h>
#include <locale_io.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_symbol.h>
#include <sch_edit_frame.h>       // SYMBOL_ORIENTATION_T
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_shape.h>
#include <sch_no_connect.h>
#include <sch_text.h>
#include <sch_textbox.h>
#include <sch_table.h>
#include <sch_tablecell.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <schematic.h>
#include <sch_screen.h>
#include <io/kicad/kicad_io_utils.h>
#include <schematic_lexer.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr_parser.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr_lib_cache.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr_common.h>
#include <symbol_lib_table.h>  // for PropPowerSymsOnly definition.
#include <ee_selection.h>
#include <string_utils.h>
#include <wx_filename.h>       // for ::ResolvePossibleSymlinks()
#include <progress_reporter.h>
#include <boost/algorithm/string/join.hpp>

using namespace TSCHEMATIC_T;


#define SCH_PARSE_ERROR( text, reader, pos )                         \
    THROW_PARSE_ERROR( text, reader.GetSource(), reader.Line(),      \
                       reader.LineNumber(), pos - reader.Line() )


SCH_IO_KICAD_SEXPR::SCH_IO_KICAD_SEXPR() : SCH_IO( wxS( "Eeschema s-expression" ) )
{
    init( nullptr );
}


SCH_IO_KICAD_SEXPR::~SCH_IO_KICAD_SEXPR()
{
    delete m_cache;
}


void SCH_IO_KICAD_SEXPR::init( SCHEMATIC* aSchematic, const STRING_UTF8_MAP* aProperties )
{
    m_version         = 0;
    m_appending       = false;
    m_rootSheet       = nullptr;
    m_schematic       = aSchematic;
    m_cache           = nullptr;
    m_out             = nullptr;
    m_nextFreeFieldId = 100; // number arbitrarily > MANDATORY_FIELDS or SHEET_MANDATORY_FIELDS
}


SCH_SHEET* SCH_IO_KICAD_SEXPR::LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                                  SCH_SHEET*             aAppendToMe,
                                                  const STRING_UTF8_MAP* aProperties )
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
        m_appending = true;
        wxLogTrace( traceSchPlugin, "Append \"%s\" to sheet \"%s\".",
                    aFileName, aAppendToMe->GetFileName() );

        wxFileName normedFn = aAppendToMe->GetFileName();

        if( !normedFn.IsAbsolute() )
        {
            if( aFileName.Right( normedFn.GetFullPath().Length() ) == normedFn.GetFullPath() )
                m_path = aFileName.Left( aFileName.Length() - normedFn.GetFullPath().Length() );
        }

        if( m_path.IsEmpty() )
            m_path = aSchematic->Prj().GetProjectPath();

        wxLogTrace( traceSchPlugin, "Normalized append path \"%s\".", m_path );
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
        loadHierarchy( SCH_SHEET_PATH(), newSheet.get() );

        // If we got here, the schematic loaded successfully.
        sheet = newSheet.release();
        m_rootSheet = nullptr;         // Quiet Coverity warning.
    }
    else
    {
        wxCHECK_MSG( aSchematic->IsValid(), nullptr, "Can't append to a schematic with no root!" );
        m_rootSheet = &aSchematic->Root();
        sheet = aAppendToMe;
        loadHierarchy( SCH_SHEET_PATH(), sheet );
    }

    wxASSERT( m_currentPath.size() == 1 );  // only the project path should remain

    m_currentPath.pop(); // Clear the path stack for next call to Load

    return sheet;
}


// Everything below this comment is recursive.  Modify with care.

void SCH_IO_KICAD_SEXPR::loadHierarchy( const SCH_SHEET_PATH& aParentSheetPath, SCH_SHEET* aSheet )
{
    m_currentSheetPath.push_back( aSheet );

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
        wxLogTrace( traceSchPlugin, "Saving path    '%s'", m_currentPath.top() );
        m_currentPath.push( fileName.GetPath() );
        wxLogTrace( traceSchPlugin, "Current path   '%s'", m_currentPath.top() );
        wxLogTrace( traceSchPlugin, "Loading        '%s'", fileName.GetFullPath() );

        SCH_SHEET_PATH ancestorSheetPath = aParentSheetPath;

        while( !ancestorSheetPath.empty() )
        {
            if( ancestorSheetPath.LastScreen()->GetFileName() == fileName.GetFullPath() )
            {
                if( !m_error.IsEmpty() )
                    m_error += "\n";

                m_error += wxString::Format( _( "Could not load sheet '%s' because it already "
                                                "appears as a direct ancestor in the schematic "
                                                "hierarchy." ),
                                             fileName.GetFullPath() );

                fileName = wxEmptyString;

                break;
            }

            ancestorSheetPath.pop_back();
        }

        if( ancestorSheetPath.empty() )
        {
            // Existing schematics could be either in the root sheet path or the current sheet
            // load path so we have to check both.
            if( !m_rootSheet->SearchHierarchy( fileName.GetFullPath(), &screen ) )
                m_currentSheetPath.at( 0 )->SearchHierarchy( fileName.GetFullPath(), &screen );
        }

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

            if( fileName.FileExists() )
            {
                aSheet->GetScreen()->SetFileReadOnly( !fileName.IsFileWritable() );
                aSheet->GetScreen()->SetFileExists( true );
            }
            else
            {
                aSheet->GetScreen()->SetFileReadOnly( !fileName.IsDirWritable() );
                aSheet->GetScreen()->SetFileExists( false );
            }

            SCH_SHEET_PATH currentSheetPath = aParentSheetPath;
            currentSheetPath.push_back( aSheet );

            // This was moved out of the try{} block so that any sheet definitions that
            // the plugin fully parsed before the exception was raised will be loaded.
            for( SCH_ITEM* aItem : aSheet->GetScreen()->Items().OfType( SCH_SHEET_T ) )
            {
                wxCHECK2( aItem->Type() == SCH_SHEET_T, /* do nothing */ );
                SCH_SHEET* sheet = static_cast<SCH_SHEET*>( aItem );

                // Recursion starts here.
                loadHierarchy( currentSheetPath, sheet );
            }
        }

        m_currentPath.pop();
        wxLogTrace( traceSchPlugin, "Restoring path \"%s\"", m_currentPath.top() );
    }

    m_currentSheetPath.pop_back();
}


void SCH_IO_KICAD_SEXPR::loadFile( const wxString& aFileName, SCH_SHEET* aSheet )
{
    FILE_LINE_READER reader( aFileName );

    size_t lineCount = 0;

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_ERROR( _( "Open cancelled by user." ) );

        while( reader.ReadLine() )
            lineCount++;

        reader.Rewind();
    }

    SCH_IO_KICAD_SEXPR_PARSER parser( &reader, m_progressReporter, lineCount, m_rootSheet,
                                      m_appending );

    parser.ParseSchematic( aSheet );
}


void SCH_IO_KICAD_SEXPR::LoadContent( LINE_READER& aReader, SCH_SHEET* aSheet, int aFileVersion )
{
    wxCHECK( aSheet, /* void */ );

    LOCALE_IO toggle;
    SCH_IO_KICAD_SEXPR_PARSER parser( &aReader );

    parser.ParseSchematic( aSheet, true, aFileVersion );
}


void SCH_IO_KICAD_SEXPR::SaveSchematicFile( const wxString& aFileName, SCH_SHEET* aSheet,
                                            SCHEMATIC*             aSchematic,
                                            const STRING_UTF8_MAP* aProperties )
{
    wxCHECK_RET( aSheet != nullptr, "NULL SCH_SHEET object." );
    wxCHECK_RET( !aFileName.IsEmpty(), "No schematic file name defined." );

    LOCALE_IO   toggle;     // toggles on, then off, the C locale, to write floating point values.

    init( aSchematic, aProperties );

    wxFileName fn = aFileName;

    // File names should be absolute.  Don't assume everything relative to the project path
    // works properly.
    wxASSERT( fn.IsAbsolute() );

    PRETTIFIED_FILE_OUTPUTFORMATTER formatter( fn.GetFullPath() );

    m_out = &formatter;     // no ownership

    Format( aSheet );

    if( aSheet->GetScreen() )
        aSheet->GetScreen()->SetFileExists( true );
}


void SCH_IO_KICAD_SEXPR::Format( SCH_SHEET* aSheet )
{
    wxCHECK_RET( aSheet != nullptr, "NULL SCH_SHEET* object." );
    wxCHECK_RET( m_schematic != nullptr, "NULL SCHEMATIC* object." );

    SCH_SCREEN* screen = aSheet->GetScreen();

    wxCHECK( screen, /* void */ );

    m_out->Print( 0, "(kicad_sch (version %d) (generator \"eeschema\") (generator_version \"%s\")\n\n",
                  SEXPR_SCHEMATIC_FILE_VERSION, GetMajorMinorVersion().c_str().AsChar() );

    KICAD_FORMAT::FormatUuid( m_out, screen->m_uuid );

    screen->GetPageSettings().Format( m_out, 1, 0 );
    m_out->Print( 0, "\n" );
    screen->GetTitleBlock().Format( m_out, 1, 0 );

    // Save cache library.
    m_out->Print( 1, "(lib_symbols\n" );

    for( const auto& [ libItemName, libSymbol ] : screen->GetLibSymbols() )
        SCH_IO_KICAD_SEXPR_LIB_CACHE::SaveSymbol( libSymbol, *m_out, 2, libItemName );

    m_out->Print( 1, ")\n\n" );

    for( const std::shared_ptr<BUS_ALIAS>& alias : screen->GetBusAliases() )
        saveBusAlias( alias, 1 );

    // Enforce item ordering
    auto cmp =
            []( const SCH_ITEM* a, const SCH_ITEM* b )
            {
                if( a->Type() != b->Type() )
                    return a->Type() < b->Type();

                return a->m_Uuid < b->m_Uuid;
            };

    std::multiset<SCH_ITEM*, decltype( cmp )> save_map( cmp );

    for( SCH_ITEM* item : screen->Items() )
    {
        // Markers are not saved, so keep them from being considered below
        if( item->Type() != SCH_MARKER_T )
            save_map.insert( item );
    }

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
            {
                m_out->Print( 0, "\n" );
            }
        }

        switch( item->Type() )
        {
        case SCH_SYMBOL_T:
            m_out->Print( 0, "\n" );
            saveSymbol( static_cast<SCH_SYMBOL*>( item ), *m_schematic, 1, false );
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

        case SCH_SHAPE_T:
            saveShape( static_cast<SCH_SHAPE*>( item ), 1 );
            break;

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_DIRECTIVE_LABEL_T:
            saveText( static_cast<SCH_TEXT*>( item ), 1 );
            break;

        case SCH_TEXTBOX_T:
            saveTextBox( static_cast<SCH_TEXTBOX*>( item ), 1 );
            break;

        case SCH_TABLE_T:
            saveTable( static_cast<SCH_TABLE*>( item ), 1 );
            break;

        default:
            wxASSERT( "Unexpected schematic object type in SCH_IO_KICAD_SEXPR::Format()" );
        }
    }

    if( aSheet->HasRootInstance() )
    {
        std::vector< SCH_SHEET_INSTANCE> instances;

        instances.emplace_back( aSheet->GetRootInstance() );
        saveInstances( instances, 1 );
    }

    m_out->Print( 0, ")\n" );
}


void SCH_IO_KICAD_SEXPR::Format( EE_SELECTION* aSelection, SCH_SHEET_PATH* aSelectionPath,
                                 SCHEMATIC& aSchematic, OUTPUTFORMATTER* aFormatter,
                                 bool aForClipboard )
{
    wxCHECK( aSelection && aSelectionPath && aFormatter, /* void */ );

    LOCALE_IO toggle;
    SCH_SHEET_LIST fullHierarchy = aSchematic.GetSheets();

    m_schematic = &aSchematic;
    m_out = aFormatter;

    size_t                          i;
    SCH_ITEM*                       item;
    std::map<wxString, LIB_SYMBOL*> libSymbols;
    SCH_SCREEN*                     screen = aSelection->GetScreen();
    std::set<SCH_TABLE*>            promotedTables;

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

        for( const std::pair<const wxString, LIB_SYMBOL*>& libSymbol : libSymbols )
        {
            SCH_IO_KICAD_SEXPR_LIB_CACHE::SaveSymbol( libSymbol.second, *m_out, 1,
                                                      libSymbol.first );
        }

        m_out->Print( 0, ")\n\n" );
    }

    for( i = 0; i < aSelection->GetSize(); ++i )
    {
        item = (SCH_ITEM*) aSelection->GetItem( i );

        switch( item->Type() )
        {
        case SCH_SYMBOL_T:
            saveSymbol( static_cast<SCH_SYMBOL*>( item ), aSchematic, 0, aForClipboard,
                        aSelectionPath );
            break;

        case SCH_BITMAP_T:
            saveBitmap( static_cast< SCH_BITMAP* >( item ), 0 );
            break;

        case SCH_SHEET_T:
            saveSheet( static_cast< SCH_SHEET* >( item ), 0 );
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

        case SCH_SHAPE_T:
            saveShape( static_cast<SCH_SHAPE*>( item ), 0 );
            break;

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_DIRECTIVE_LABEL_T:
            saveText( static_cast<SCH_TEXT*>( item ), 0 );
            break;

        case SCH_TEXTBOX_T:
            saveTextBox( static_cast<SCH_TEXTBOX*>( item ), 0 );
            break;

        case SCH_TABLECELL_T:
        {
            SCH_TABLE* table = static_cast<SCH_TABLE*>( item->GetParent() );

            if( promotedTables.count( table ) )
                break;

            table->SetFlags( SKIP_STRUCT );
            saveTable( table, 0 );
            table->ClearFlags( SKIP_STRUCT );
            promotedTables.insert( table );
            break;
        }

        case SCH_TABLE_T:
            item->ClearFlags( SKIP_STRUCT );
            saveTable( static_cast<SCH_TABLE*>( item ), 0 );
            break;

        default:
            wxASSERT( "Unexpected schematic object type in SCH_IO_KICAD_SEXPR::Format()" );
        }
    }
}


void SCH_IO_KICAD_SEXPR::saveSymbol( SCH_SYMBOL* aSymbol, const SCHEMATIC& aSchematic,
                                     int aNestLevel, bool aForClipboard,
                                     const SCH_SHEET_PATH* aRelativePath )
{
    wxCHECK_RET( aSymbol != nullptr && m_out != nullptr, "" );

    std::string     libName;

    wxString symbol_name = aSymbol->GetLibId().Format();

    if( symbol_name.size() )
    {
        libName = toUTFTildaText( symbol_name );
    }
    else
    {
        libName = "_NONAME_";
    }

    EDA_ANGLE angle;
    int       orientation = aSymbol->GetOrientation() & ~( SYM_MIRROR_X | SYM_MIRROR_Y );

    if( orientation == SYM_ORIENT_90 )
        angle = ANGLE_90;
    else if( orientation == SYM_ORIENT_180 )
        angle = ANGLE_180;
    else if( orientation == SYM_ORIENT_270 )
        angle = ANGLE_270;
    else
        angle = ANGLE_0;

    m_out->Print( aNestLevel, "(symbol" );

    if( !aSymbol->UseLibIdLookup() )
    {
        m_out->Print( 0, " (lib_name %s)",
                      m_out->Quotew( aSymbol->GetSchSymbolLibraryName() ).c_str() );
    }

    m_out->Print( 0, " (lib_id %s) (at %s %s %s)",
                  m_out->Quotew( aSymbol->GetLibId().Format().wx_str() ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aSymbol->GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aSymbol->GetPosition().y ).c_str(),
                  EDA_UNIT_UTILS::FormatAngle( angle ).c_str() );

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

    // The symbol unit is always set to the first instance regardless of the current sheet
    // instance to prevent file churn.
    int unit = ( aSymbol->GetInstances().size() == 0 ) ?
               aSymbol->GetUnit() :
               aSymbol->GetInstances()[0].m_Unit;

    if( aForClipboard && aRelativePath )
    {
        SCH_SYMBOL_INSTANCE unitInstance;

        if( aSymbol->GetInstance( unitInstance, aRelativePath->Path() ) )
            unit = unitInstance.m_Unit;
    }

    m_out->Print( 0, " (unit %d)", unit );

    if( aSymbol->GetBodyStyle() == BODY_STYLE::DEMORGAN )
        m_out->Print( 0, " (convert %d)", aSymbol->GetBodyStyle() );

    m_out->Print( 0, "\n" );

    m_out->Print( aNestLevel + 1, "(exclude_from_sim %s)",
                  ( aSymbol->GetExcludedFromSim() ) ? "yes" : "no" );
    m_out->Print( 0, " (in_bom %s)", ( aSymbol->GetExcludedFromBOM() ) ? "no" : "yes" );
    m_out->Print( 0, " (on_board %s)", ( aSymbol->GetExcludedFromBoard() ) ? "no" : "yes" );
    m_out->Print( 0, " (dnp %s)", ( aSymbol->GetDNP() ) ? "yes" : "no" );

    if( aSymbol->GetFieldsAutoplaced() != FIELDS_AUTOPLACED_NO )
        m_out->Print( 0, " (fields_autoplaced yes)" );

    m_out->Print( 0, "\n" );

    KICAD_FORMAT::FormatUuid( m_out, aSymbol->m_Uuid );

    m_nextFreeFieldId = MANDATORY_FIELDS;

    for( SCH_FIELD& field : aSymbol->GetFields() )
    {
        int id = field.GetId();
        wxString value = field.GetText();

        if( !aForClipboard && aSymbol->GetInstances().size() )
        {
            // The instance fields are always set to the default instance regardless of the
            // sheet instance to prevent file churn.
            if( id == REFERENCE_FIELD )
            {
                field.SetText( aSymbol->GetInstances()[0].m_Reference );
            }
            else if( id == VALUE_FIELD )
            {
                field.SetText( aSymbol->GetField( VALUE_FIELD )->GetText() );
            }
            else if( id == FOOTPRINT_FIELD )
            {
                field.SetText( aSymbol->GetField( FOOTPRINT_FIELD )->GetText() );
            }
        }
        else if( aForClipboard && aSymbol->GetInstances().size() && aRelativePath
               && ( id == REFERENCE_FIELD ) )
        {
            SCH_SYMBOL_INSTANCE instance;

            if( aSymbol->GetInstance( instance, aRelativePath->Path() ) )
                field.SetText( instance.m_Reference );
        }

        try
        {
            saveField( &field, aNestLevel + 1 );
        }
        catch( ... )
        {
            // Restore the changed field text on write error.
            if( id == REFERENCE_FIELD || id == VALUE_FIELD || id == FOOTPRINT_FIELD )
                field.SetText( value );

            throw;
        }

        if( id == REFERENCE_FIELD || id == VALUE_FIELD || id == FOOTPRINT_FIELD )
            field.SetText( value );
    }

    for( const std::unique_ptr<SCH_PIN>& pin : aSymbol->GetRawPins() )
    {
        if( pin->GetAlt().IsEmpty() )
        {
            m_out->Print( aNestLevel + 1, "(pin %s ", m_out->Quotew( pin->GetNumber() ).c_str() );
            KICAD_FORMAT::FormatUuid( m_out, pin->m_Uuid );
            m_out->Print( aNestLevel + 1, ")\n" );
        }
        else
        {
            m_out->Print( aNestLevel + 1, "(pin %s ", m_out->Quotew( pin->GetNumber() ).c_str() );
            KICAD_FORMAT::FormatUuid( m_out, pin->m_Uuid );
            m_out->Print( aNestLevel + 1, " (alternate %s))\n",
                          m_out->Quotew( pin->GetAlt() ).c_str() );
        }
    }

    if( !aSymbol->GetInstances().empty() )
    {
        std::map<KIID, std::vector<SCH_SYMBOL_INSTANCE>> projectInstances;

        m_out->Print( aNestLevel + 1, "(instances\n" );

        wxString projectName;
        KIID lastProjectUuid;
        KIID rootSheetUuid = aSchematic.Root().m_Uuid;
        SCH_SHEET_LIST fullHierarchy = aSchematic.GetSheets();

        for( const SCH_SYMBOL_INSTANCE& inst : aSymbol->GetInstances() )
        {
            // Zero length KIID_PATH objects are not valid and will cause a crash below.
            wxCHECK2( inst.m_Path.size(), continue );

            // If the instance data is part of this design but no longer has an associated sheet
            // path, don't save it.  This prevents large amounts of orphaned instance data for the
            // current project from accumulating in the schematic files.
            bool isOrphaned = ( inst.m_Path[0] == rootSheetUuid )
                              && !fullHierarchy.GetSheetPathByKIIDPath( inst.m_Path );

            // Keep all instance data when copying to the clipboard.  They may be needed on paste.
            if( !aForClipboard && isOrphaned )
                continue;

            auto it = projectInstances.find( inst.m_Path[0] );

            if( it == projectInstances.end() )
            {
                projectInstances[ inst.m_Path[0] ] = { inst };
            }
            else
            {
                it->second.emplace_back( inst );
            }
        }

        for( auto& [uuid, instances] : projectInstances )
        {
            wxCHECK2( instances.size(), continue );

            // Sort project instances by KIID_PATH.
            std::sort( instances.begin(), instances.end(),
                       []( SCH_SYMBOL_INSTANCE& aLhs, SCH_SYMBOL_INSTANCE& aRhs )
                       {
                           return aLhs.m_Path < aRhs.m_Path;
                       } );

            projectName = instances[0].m_ProjectName;

            m_out->Print( aNestLevel + 2, "(project %s\n",
                          m_out->Quotew( projectName ).c_str() );

            for( const SCH_SYMBOL_INSTANCE& instance : instances )
            {
                wxString path;
                KIID_PATH tmp = instance.m_Path;

                if( aForClipboard && aRelativePath )
                    tmp.MakeRelativeTo( aRelativePath->Path() );

                path = tmp.AsString();

                m_out->Print( aNestLevel + 3, "(path %s\n",
                              m_out->Quotew( path ).c_str() );
                m_out->Print( aNestLevel + 4, "(reference %s) (unit %d)\n",
                              m_out->Quotew( instance.m_Reference ).c_str(),
                              instance.m_Unit );
                m_out->Print( aNestLevel + 3, ")\n" );
            }

            m_out->Print( aNestLevel + 2, ")\n" );  // Closes `project`.
        }

        m_out->Print( aNestLevel + 1, ")\n" );  // Closes `instances`.
    }

    m_out->Print( aNestLevel, ")\n" );      // Closes `symbol`.
}


void SCH_IO_KICAD_SEXPR::saveField( SCH_FIELD* aField, int aNestLevel )
{
    wxCHECK_RET( aField != nullptr && m_out != nullptr, "" );

    wxString fieldName = aField->GetCanonicalName();
    // For some reason (bug in legacy parser?) the field ID for non-mandatory fields is -1 so
    // check for this in order to correctly use the field name.

    if( aField->GetId() == -1 /* undefined ID */ )
    {
        // Replace the default name built by GetCanonicalName() by
        // the field name if exists
        if( !aField->GetName().IsEmpty() )
            fieldName = aField->GetName();

        aField->SetId( m_nextFreeFieldId );
        m_nextFreeFieldId += 1;
    }
    else if( aField->GetId() >= m_nextFreeFieldId )
    {
        m_nextFreeFieldId = aField->GetId() + 1;
    }

    m_out->Print( aNestLevel, "(property %s %s (at %s %s %s)",
                  m_out->Quotew( fieldName ).c_str(),
                  m_out->Quotew( aField->GetText() ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aField->GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aField->GetPosition().y ).c_str(),
                  EDA_UNIT_UTILS::FormatAngle( aField->GetTextAngle() ).c_str() );

    if( aField->IsNameShown() )
        m_out->Print( 0, " (show_name yes)" );

    if( !aField->CanAutoplace() )
        m_out->Print( 0, " (do_not_autoplace yes)" );

    if( !aField->IsDefaultFormatting()
      || ( aField->GetTextHeight() != schIUScale.MilsToIU( DEFAULT_SIZE_TEXT ) ) )
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


void SCH_IO_KICAD_SEXPR::saveBitmap( SCH_BITMAP* aBitmap, int aNestLevel )
{
    wxCHECK_RET( aBitmap != nullptr && m_out != nullptr, "" );

    const wxImage* image = aBitmap->GetImage()->GetImageData();

    wxCHECK_RET( image != nullptr, "wxImage* is NULL" );

    m_out->Print( aNestLevel, "(image (at %s %s)",
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aBitmap->GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aBitmap->GetPosition().y ).c_str() );

    double scale = aBitmap->GetImage()->GetScale();

    // 20230121 or older file format versions assumed 300 image PPI at load/save.
    // Let's keep compatibility by changing image scale.
    if( SEXPR_SCHEMATIC_FILE_VERSION <= 20230121 )
    {
        BITMAP_BASE* bm_image = aBitmap->GetImage();
        scale = scale * 300.0 / bm_image->GetPPI();
    }

    if( scale != 1.0 )
        m_out->Print( 0, " (scale %g)", scale );

    m_out->Print( 0, "\n" );

    KICAD_FORMAT::FormatUuid( m_out, aBitmap->m_Uuid );

    m_out->Print( aNestLevel + 1, "(data" );

    wxString out = wxBase64Encode( aBitmap->GetImage()->GetImageDataBuffer() );

    // Apparently the MIME standard character width for base64 encoding is 76 (unconfirmed)
    // so use it in a vein attempt to be standard like.
#define MIME_BASE64_LENGTH 76

    size_t first = 0;

    while( first < out.Length() )
    {
        m_out->Print( 0, "\n" );
        m_out->Print( aNestLevel + 2, "\"%s\"", TO_UTF8( out( first, MIME_BASE64_LENGTH ) ) );
        first += MIME_BASE64_LENGTH;
    }

    m_out->Print( 0, "\n" );
    m_out->Print( aNestLevel + 1, ")\n" );  // Closes data token.
    m_out->Print( aNestLevel, ")\n" );      // Closes image token.
}


void SCH_IO_KICAD_SEXPR::saveSheet( SCH_SHEET* aSheet, int aNestLevel )
{
    wxCHECK_RET( aSheet != nullptr && m_out != nullptr, "" );

    m_out->Print( aNestLevel, "(sheet (at %s %s) (size %s %s)",
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aSheet->GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aSheet->GetPosition().y ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aSheet->GetSize().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aSheet->GetSize().y ).c_str() );

    if( aSheet->GetFieldsAutoplaced() != FIELDS_AUTOPLACED_NO )
        m_out->Print( 0, " (fields_autoplaced yes)" );

    m_out->Print( 0, "\n" );

    STROKE_PARAMS stroke( aSheet->GetBorderWidth(), LINE_STYLE::SOLID, aSheet->GetBorderColor() );

    stroke.SetWidth( aSheet->GetBorderWidth() );
    stroke.Format( m_out, schIUScale, aNestLevel + 1 );

    m_out->Print( 0, "\n" );

    m_out->Print( aNestLevel + 1, "(fill (color %d %d %d %0.4f))\n",
                  KiROUND( aSheet->GetBackgroundColor().r * 255.0 ),
                  KiROUND( aSheet->GetBackgroundColor().g * 255.0 ),
                  KiROUND( aSheet->GetBackgroundColor().b * 255.0 ),
                  aSheet->GetBackgroundColor().a );

    KICAD_FORMAT::FormatUuid( m_out, aSheet->m_Uuid );

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
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           pin->GetPosition().x ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           pin->GetPosition().y ).c_str(),
                      EDA_UNIT_UTILS::FormatAngle( getSheetPinAngle( pin->GetSide() ) ).c_str() );

        pin->Format( m_out, aNestLevel + 1, 0 );

        KICAD_FORMAT::FormatUuid( m_out, pin->m_Uuid );

        m_out->Print( aNestLevel + 1, ")\n" );  // Closes pin token.
    }

    // Save all sheet instances here except the root sheet instance.
    std::vector< SCH_SHEET_INSTANCE > sheetInstances = aSheet->GetInstances();

    auto it = sheetInstances.begin();

    while( it != sheetInstances.end() )
    {
        if( it->m_Path.size() == 0 )
            it = sheetInstances.erase( it );
        else
            it++;
    }

    if( !sheetInstances.empty() )
    {
        m_out->Print( aNestLevel + 1, "(instances\n" );

        KIID lastProjectUuid;
        KIID rootSheetUuid = m_schematic->Root().m_Uuid;
        SCH_SHEET_LIST fullHierarchy = m_schematic->GetSheets();
        bool project_open = false;

        for( size_t i = 0; i < sheetInstances.size(); i++ )
        {
            // If the instance data is part of this design but no longer has an associated sheet
            // path, don't save it.  This prevents large amounts of orphaned instance data for the
            // current project from accumulating in the schematic files.
            //
            // Keep all instance data when copying to the clipboard.  It may be needed on paste.
            if( ( sheetInstances[i].m_Path[0] == rootSheetUuid )
              && !fullHierarchy.GetSheetPathByKIIDPath( sheetInstances[i].m_Path, false ) )
            {
                if( project_open && ( ( i + 1 == sheetInstances.size() )
                  || lastProjectUuid != sheetInstances[i+1].m_Path[0] ) )
                {
                    m_out->Print( aNestLevel + 2, ")\n" );  // Closes `project` token.
                    project_open = false;
                }

                continue;
            }

            if( lastProjectUuid != sheetInstances[i].m_Path[0] )
            {
                wxString projectName;

                if( sheetInstances[i].m_Path[0] == rootSheetUuid )
                    projectName = m_schematic->Prj().GetProjectName();
                else
                    projectName = sheetInstances[i].m_ProjectName;

                lastProjectUuid = sheetInstances[i].m_Path[0];
                m_out->Print( aNestLevel + 2, "(project %s\n",
                              m_out->Quotew( projectName ).c_str() );
                project_open = true;
            }

            wxString path = sheetInstances[i].m_Path.AsString();

            m_out->Print( aNestLevel + 3, "(path %s (page %s))\n",
                          m_out->Quotew( path ).c_str(),
                          m_out->Quotew( sheetInstances[i].m_PageNumber ).c_str() );

            if( project_open && ( ( i + 1 == sheetInstances.size() )
              || lastProjectUuid != sheetInstances[i+1].m_Path[0] ) )
            {
                m_out->Print( aNestLevel + 2, ")\n" );  // Closes `project` token.
                project_open = false;
            }
        }

        m_out->Print( aNestLevel + 1, ")\n" );  // Closes `instances` token.
    }

    m_out->Print( aNestLevel, ")\n" );          // Closes sheet token.
}


void SCH_IO_KICAD_SEXPR::saveJunction( SCH_JUNCTION* aJunction, int aNestLevel )
{
    wxCHECK_RET( aJunction != nullptr && m_out != nullptr, "" );

    m_out->Print( aNestLevel, "(junction (at %s %s) (diameter %s) (color %d %d %d %s)\n",
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aJunction->GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aJunction->GetPosition().y ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aJunction->GetDiameter() ).c_str(),
                  KiROUND( aJunction->GetColor().r * 255.0 ),
                  KiROUND( aJunction->GetColor().g * 255.0 ),
                  KiROUND( aJunction->GetColor().b * 255.0 ),
                  FormatDouble2Str( aJunction->GetColor().a ).c_str() );

    KICAD_FORMAT::FormatUuid( m_out, aJunction->m_Uuid );

    m_out->Print( aNestLevel, ")\n" );
}


void SCH_IO_KICAD_SEXPR::saveNoConnect( SCH_NO_CONNECT* aNoConnect, int aNestLevel )
{
    wxCHECK_RET( aNoConnect != nullptr && m_out != nullptr, "" );

    m_out->Print( aNestLevel, "(no_connect (at %s %s)",
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aNoConnect->GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aNoConnect->GetPosition().y ).c_str() );
    KICAD_FORMAT::FormatUuid( m_out, aNoConnect->m_Uuid );
    m_out->Print( aNestLevel, ")\n" );
}


void SCH_IO_KICAD_SEXPR::saveBusEntry( SCH_BUS_ENTRY_BASE* aBusEntry, int aNestLevel )
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
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           aBusEntry->GetPosition().x ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           aBusEntry->GetPosition().y ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           aBusEntry->GetSize().x ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           aBusEntry->GetSize().y ).c_str() );

        aBusEntry->GetStroke().Format( m_out, schIUScale, aNestLevel + 1 );

        m_out->Print( 0, "\n" );

        KICAD_FORMAT::FormatUuid( m_out, aBusEntry->m_Uuid );

        m_out->Print( aNestLevel, ")\n" );
    }
}


void SCH_IO_KICAD_SEXPR::saveShape( SCH_SHAPE* aShape, int aNestLevel )
{
    wxCHECK_RET( aShape != nullptr && m_out != nullptr, "" );

    switch( aShape->GetShape() )
    {
    case SHAPE_T::ARC:
        formatArc( m_out, aNestLevel, aShape, false, aShape->GetStroke(), aShape->GetFillMode(),
                   aShape->GetFillColor(), false, aShape->m_Uuid );
        break;

    case SHAPE_T::CIRCLE:
        formatCircle( m_out, aNestLevel, aShape, false, aShape->GetStroke(), aShape->GetFillMode(),
                      aShape->GetFillColor(), false, aShape->m_Uuid );
        break;

    case SHAPE_T::RECTANGLE:
        formatRect( m_out, aNestLevel, aShape, false, aShape->GetStroke(), aShape->GetFillMode(),
                    aShape->GetFillColor(), false, aShape->m_Uuid );
        break;

    case SHAPE_T::BEZIER:
        formatBezier( m_out, aNestLevel, aShape, false, aShape->GetStroke(), aShape->GetFillMode(),
                      aShape->GetFillColor(), false, aShape->m_Uuid );
        break;

    case SHAPE_T::POLY:
        formatPoly( m_out, aNestLevel, aShape, false, aShape->GetStroke(), aShape->GetFillMode(),
                    aShape->GetFillColor(), false, aShape->m_Uuid );
        break;

    default:
        UNIMPLEMENTED_FOR( aShape->SHAPE_T_asString() );
    }
}


void SCH_IO_KICAD_SEXPR::saveLine( SCH_LINE* aLine, int aNestLevel )
{
    wxCHECK_RET( aLine != nullptr && m_out != nullptr, "" );

    wxString lineType;

    STROKE_PARAMS line_stroke = aLine->GetStroke();

    switch( aLine->GetLayer() )
    {
    case LAYER_BUS:     lineType = "bus";       break;
    case LAYER_WIRE:    lineType = "wire";      break;
    case LAYER_NOTES:   lineType = "polyline";  break;
    default:
        UNIMPLEMENTED_FOR( LayerName( aLine->GetLayer() ) );
    }

    m_out->Print( aNestLevel, "(%s (pts (xy %s %s) (xy %s %s))\n",
                  TO_UTF8( lineType ),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aLine->GetStartPoint().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aLine->GetStartPoint().y ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aLine->GetEndPoint().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aLine->GetEndPoint().y ).c_str() );

    line_stroke.Format( m_out, schIUScale, aNestLevel + 1 );
    m_out->Print( 0, "\n" );

    KICAD_FORMAT::FormatUuid( m_out, aLine->m_Uuid );

    m_out->Print( aNestLevel, ")\n" );
}


void SCH_IO_KICAD_SEXPR::saveText( SCH_TEXT* aText, int aNestLevel )
{
    wxCHECK_RET( aText != nullptr && m_out != nullptr, "" );

    // Note: label is nullptr SCH_TEXT, but not for SCH_LABEL_XXX,
    SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( aText );

    m_out->Print( aNestLevel, "(%s %s",
                  getTextTypeToken( aText->Type() ),
                  m_out->Quotew( aText->GetText() ).c_str() );

    if( aText->Type() == SCH_TEXT_T )
    {
        m_out->Print( 0, " (exclude_from_sim %s)\n", aText->GetExcludedFromSim() ? "yes" : "no" );
    }
    else if( aText->Type() == SCH_DIRECTIVE_LABEL_T )
    {
        SCH_DIRECTIVE_LABEL* flag = static_cast<SCH_DIRECTIVE_LABEL*>( aText );

        m_out->Print( 0, " (length %s)",
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           flag->GetPinLength() ).c_str() );
    }

    EDA_ANGLE angle = aText->GetTextAngle();

    if( label )
    {
        if( label->Type() == SCH_GLOBAL_LABEL_T
                || label->Type() == SCH_HIER_LABEL_T
                || label->Type() == SCH_DIRECTIVE_LABEL_T )
        {
            m_out->Print( 0, " (shape %s)", getSheetPinShapeToken( label->GetShape() ) );
        }

        // The angle of the text is always 0 or 90 degrees for readibility reasons,
        // but the item itself can have more rotation (-90 and 180 deg)
        switch( label->GetSpinStyle() )
        {
        default:
        case SPIN_STYLE::LEFT:   angle += ANGLE_180; break;
        case SPIN_STYLE::UP:                         break;
        case SPIN_STYLE::RIGHT:                      break;
        case SPIN_STYLE::BOTTOM: angle += ANGLE_180; break;
        }
    }

    if( aText->GetText().Length() < 50 )
    {
        m_out->Print( 0, " (at %s %s %s)",
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           aText->GetPosition().x ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           aText->GetPosition().y ).c_str(),
                      EDA_UNIT_UTILS::FormatAngle( angle ).c_str() );
    }
    else
    {
        m_out->Print( 0, "\n" );
        m_out->Print( aNestLevel + 1, "(at %s %s %s)",
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           aText->GetPosition().x ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           aText->GetPosition().y ).c_str(),
                      EDA_UNIT_UTILS::FormatAngle( angle ).c_str() );
    }

    if( aText->GetFieldsAutoplaced() != FIELDS_AUTOPLACED_NO )
        m_out->Print( 0, " (fields_autoplaced yes)" );

    m_out->Print( 0, "\n" );
    aText->EDA_TEXT::Format( m_out, aNestLevel, 0 );

    KICAD_FORMAT::FormatUuid( m_out, aText->m_Uuid );

    if( label )
    {
        for( SCH_FIELD& field : label->GetFields() )
            saveField( &field, aNestLevel + 1 );
    }

    m_out->Print( aNestLevel, ")\n" );   // Closes text token.
}


void SCH_IO_KICAD_SEXPR::saveTextBox( SCH_TEXTBOX* aTextBox, int aNestLevel )
{
    wxCHECK_RET( aTextBox != nullptr && m_out != nullptr, "" );

    m_out->Print( aNestLevel, "(%s %s\n",
                  aTextBox->Type() == SCH_TABLECELL_T ? "table_cell" : "text_box",
                  m_out->Quotew( aTextBox->GetText() ).c_str() );

    VECTOR2I pos = aTextBox->GetStart();
    VECTOR2I size = aTextBox->GetEnd() - pos;

    m_out->Print( aNestLevel + 1, "(exclude_from_sim %s) (at %s %s %s) (size %s %s) (margins %s %s %s %s)",
                  aTextBox->GetExcludedFromSim() ? "yes" : "no",
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, pos.x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, pos.y ).c_str(),
                  EDA_UNIT_UTILS::FormatAngle( aTextBox->GetTextAngle() ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, size.x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, size.y ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTextBox->GetMarginLeft() ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTextBox->GetMarginTop() ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTextBox->GetMarginRight() ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTextBox->GetMarginBottom() ).c_str() );

    if( SCH_TABLECELL* cell = dynamic_cast<SCH_TABLECELL*>( aTextBox ) )
        m_out->Print( 0, " (span %d %d)", cell->GetColSpan(), cell->GetRowSpan() );

    m_out->Print( 0, "\n" );

    if( aTextBox->Type() != SCH_TABLECELL_T )
    {
        aTextBox->GetStroke().Format( m_out, schIUScale, aNestLevel + 1 );
        m_out->Print( 0, "\n" );
    }

    formatFill( m_out, aNestLevel + 1, aTextBox->GetFillMode(), aTextBox->GetFillColor() );
    m_out->Print( 0, "\n" );

    aTextBox->EDA_TEXT::Format( m_out, aNestLevel, 0 );

    if( aTextBox->m_Uuid != niluuid )
        KICAD_FORMAT::FormatUuid( m_out, aTextBox->m_Uuid );

    m_out->Print( aNestLevel, ")\n" );
}


void SCH_IO_KICAD_SEXPR::saveTable( SCH_TABLE* aTable, int aNestLevel )
{
    if( aTable->GetFlags() & SKIP_STRUCT )
    {
        aTable = static_cast<SCH_TABLE*>( aTable->Clone() );

        int minCol = aTable->GetColCount();
        int maxCol = -1;
        int minRow = aTable->GetRowCount();
        int maxRow = -1;

        for( int row = 0; row < aTable->GetRowCount(); ++row )
        {
            for( int col = 0; col < aTable->GetColCount(); ++col )
            {
                SCH_TABLECELL* cell = aTable->GetCell( row, col );

                if( cell->IsSelected() )
                {
                    minRow = std::min( minRow, row );
                    maxRow = std::max( maxRow, row );
                    minCol = std::min( minCol, col );
                    maxCol = std::max( maxCol, col );
                }
                else
                {
                    cell->SetFlags( STRUCT_DELETED );
                }
            }
        }

        wxCHECK_MSG( maxCol >= minCol && maxRow >= minRow, /*void*/, wxT( "No selected cells!" ) );

        int destRow = 0;

        for( int row = minRow; row <= maxRow; row++ )
            aTable->SetRowHeight( destRow++, aTable->GetRowHeight( row ) );

        int destCol = 0;

        for( int col = minCol; col <= maxCol; col++ )
            aTable->SetColWidth( destCol++, aTable->GetColWidth( col ) );

        aTable->DeleteMarkedCells();
        aTable->SetColCount( ( maxCol - minCol ) + 1 );
    }

    wxCHECK_RET( aTable != nullptr && m_out != nullptr, "" );

    m_out->Print( aNestLevel, "(table (column_count %d)\n",
                  aTable->GetColCount() );

    m_out->Print( aNestLevel + 1, "(border (external %s) (header %s)",
                  aTable->StrokeExternal() ? "yes" : "no",
                  aTable->StrokeHeader() ? "yes" : "no" );

    if( aTable->StrokeExternal() || aTable->StrokeHeader() )
    {
        m_out->Print( 0, " " );
        aTable->GetBorderStroke().Format( m_out, schIUScale, 0 );
    }

    m_out->Print( 0, ")\n" );

    m_out->Print( aNestLevel + 1, "(separators (rows %s) (cols %s)",
                  aTable->StrokeRows() ? "yes" : "no",
                  aTable->StrokeColumns() ? "yes" : "no" );

    if( aTable->StrokeRows() || aTable->StrokeColumns() )
    {
        m_out->Print( 0, " " );
        aTable->GetSeparatorsStroke().Format( m_out, schIUScale, 0 );
    }

    m_out->Print( 0, ")\n" );               // Close `separators` token.

    m_out->Print( aNestLevel + 1, "(column_widths" );

    for( int col = 0; col < aTable->GetColCount(); ++col )
    {
        m_out->Print( 0, " %s",
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           aTable->GetColWidth( col ) ).c_str() );
    }

    m_out->Print( 0, ")\n" );

    m_out->Print( aNestLevel + 1, "(row_heights" );

    for( int row = 0; row < aTable->GetRowCount(); ++row )
    {
        m_out->Print( 0, " %s",
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           aTable->GetRowHeight( row ) ).c_str() );
    }

    m_out->Print( 0, ")\n" );

    m_out->Print( aNestLevel + 1, "(cells\n" );

    for( SCH_TABLECELL* cell : aTable->GetCells() )
        saveTextBox( cell, aNestLevel + 2 );

    m_out->Print( aNestLevel + 1, ")\n" );  // Close `cells` token.
    m_out->Print( aNestLevel, ")\n" );      // Close `table` token.

    if( aTable->GetFlags() & SKIP_STRUCT )
        delete aTable;
}


void SCH_IO_KICAD_SEXPR::saveBusAlias( std::shared_ptr<BUS_ALIAS> aAlias, int aNestLevel )
{
    wxCHECK_RET( aAlias != nullptr, "BUS_ALIAS* is NULL" );

    wxString members;

    for( const wxString& member : aAlias->Members() )
    {
        if( !members.IsEmpty() )
            members += wxS( " " );

        members += m_out->Quotew( member );
    }

    m_out->Print( aNestLevel, "(bus_alias %s (members %s))\n",
                  m_out->Quotew( aAlias->GetName() ).c_str(),
                  TO_UTF8( members ) );
}


void SCH_IO_KICAD_SEXPR::saveInstances( const std::vector<SCH_SHEET_INSTANCE>& aInstances,
                                        int aNestLevel )
{
    if( aInstances.size() )
    {
        m_out->Print( 0, "\n" );
        m_out->Print( aNestLevel, "(sheet_instances\n" );

        for( const SCH_SHEET_INSTANCE& instance : aInstances )
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
}


void SCH_IO_KICAD_SEXPR::cacheLib( const wxString& aLibraryFileName,
                                   const STRING_UTF8_MAP* aProperties )
{
    if( !m_cache || !m_cache->IsFile( aLibraryFileName ) || m_cache->IsFileChanged() )
    {
        // a spectacular episode in memory management:
        delete m_cache;
        m_cache = new SCH_IO_KICAD_SEXPR_LIB_CACHE( aLibraryFileName );

        if( !isBuffering( aProperties ) )
            m_cache->Load();
    }
}


bool SCH_IO_KICAD_SEXPR::isBuffering( const STRING_UTF8_MAP* aProperties )
{
    return ( aProperties && aProperties->Exists( SCH_IO_KICAD_SEXPR::PropBuffering ) );
}


int SCH_IO_KICAD_SEXPR::GetModifyHash() const
{
    if( m_cache )
        return m_cache->GetModifyHash();

    // If the cache hasn't been loaded, it hasn't been modified.
    return 0;
}


void SCH_IO_KICAD_SEXPR::EnumerateSymbolLib( wxArrayString&    aSymbolNameList,
                                             const wxString&   aLibraryPath,
                                             const STRING_UTF8_MAP* aProperties )
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


void SCH_IO_KICAD_SEXPR::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                             const wxString&   aLibraryPath,
                                             const STRING_UTF8_MAP* aProperties )
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


LIB_SYMBOL* SCH_IO_KICAD_SEXPR::LoadSymbol( const wxString& aLibraryPath,
                                            const wxString& aSymbolName,
                                            const STRING_UTF8_MAP* aProperties )
{
    LOCALE_IO toggle;     // toggles on, then off, the C locale.

    cacheLib( aLibraryPath, aProperties );

    LIB_SYMBOL_MAP::const_iterator it = m_cache->m_symbols.find( aSymbolName );

    // We no longer escape '/' in symbol names, but we used to.
    if( it == m_cache->m_symbols.end() && aSymbolName.Contains( '/' ) )
        it = m_cache->m_symbols.find( EscapeString( aSymbolName, CTX_LEGACY_LIBID ) );

    if( it == m_cache->m_symbols.end() && aSymbolName.Contains( wxT( "{slash}" ) ) )
    {
        wxString unescaped = aSymbolName;
        unescaped.Replace( wxT( "{slash}" ), wxT( "/" ) );
        it = m_cache->m_symbols.find( unescaped );
    }

    if( it == m_cache->m_symbols.end() )
        return nullptr;

    return it->second;
}


void SCH_IO_KICAD_SEXPR::SaveSymbol( const wxString& aLibraryPath, const LIB_SYMBOL* aSymbol,
                                     const STRING_UTF8_MAP* aProperties )
{
    LOCALE_IO toggle;     // toggles on, then off, the C locale.

    cacheLib( aLibraryPath, aProperties );

    m_cache->AddSymbol( aSymbol );

    if( !isBuffering( aProperties ) )
        m_cache->Save();
}


void SCH_IO_KICAD_SEXPR::DeleteSymbol( const wxString& aLibraryPath, const wxString& aSymbolName,
                                       const STRING_UTF8_MAP* aProperties )
{
    LOCALE_IO toggle;     // toggles on, then off, the C locale.

    cacheLib( aLibraryPath, aProperties );

    m_cache->DeleteSymbol( aSymbolName );

    if( !isBuffering( aProperties ) )
        m_cache->Save();
}


void SCH_IO_KICAD_SEXPR::CreateLibrary( const wxString& aLibraryPath,
                                        const STRING_UTF8_MAP* aProperties )
{
    if( wxFileExists( aLibraryPath ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "Symbol library '%s' already exists." ),
                                          aLibraryPath.GetData() ) );
    }

    LOCALE_IO toggle;

    delete m_cache;
    m_cache = new SCH_IO_KICAD_SEXPR_LIB_CACHE( aLibraryPath );
    m_cache->SetModified();
    m_cache->Save();
    m_cache->Load();    // update m_writable and m_mod_time
}


bool SCH_IO_KICAD_SEXPR::DeleteLibrary( const wxString& aLibraryPath,
                                        const STRING_UTF8_MAP* aProperties )
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


void SCH_IO_KICAD_SEXPR::SaveLibrary( const wxString& aLibraryPath,
                                      const STRING_UTF8_MAP* aProperties )
{
    if( !m_cache )
        m_cache = new SCH_IO_KICAD_SEXPR_LIB_CACHE( aLibraryPath );

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


bool SCH_IO_KICAD_SEXPR::IsLibraryWritable( const wxString& aLibraryPath )
{
    wxFileName fn( aLibraryPath );

    return ( fn.FileExists() && fn.IsFileWritable() ) || fn.IsDirWritable();
}


void SCH_IO_KICAD_SEXPR::GetAvailableSymbolFields( std::vector<wxString>& aNames )
{
    if( !m_cache )
        return;

    const LIB_SYMBOL_MAP& symbols = m_cache->m_symbols;

    std::set<wxString> fieldNames;

    for( LIB_SYMBOL_MAP::const_iterator it = symbols.begin();  it != symbols.end();  ++it )
    {
        std::vector<SCH_FIELD*> fields;
        it->second->GetFields( fields );

        for( SCH_FIELD* field : fields )
        {
            if( field->IsMandatory() )
                continue;

            // TODO(JE): enable configurability of this outside database libraries?
            // if( field->ShowInChooser() )
            fieldNames.insert( field->GetName() );
        }
    }

    std::copy( fieldNames.begin(), fieldNames.end(), std::back_inserter( aNames ) );
}


void SCH_IO_KICAD_SEXPR::GetDefaultSymbolFields( std::vector<wxString>& aNames )
{
    GetAvailableSymbolFields( aNames );
}


std::vector<LIB_SYMBOL*> SCH_IO_KICAD_SEXPR::ParseLibSymbols( std::string& aSymbolText,
                                                              std::string  aSource,
                                                              int aFileVersion )
{
    LOCALE_IO      toggle;     // toggles on, then off, the C locale.
    LIB_SYMBOL*    newSymbol = nullptr;
    LIB_SYMBOL_MAP map;

    std::vector<LIB_SYMBOL*>            newSymbols;
    std::unique_ptr<STRING_LINE_READER> reader = std::make_unique<STRING_LINE_READER>( aSymbolText,
                                                                                       aSource );

    do
    {
        SCH_IO_KICAD_SEXPR_PARSER parser( reader.get() );

        newSymbol = parser.ParseSymbol( map, aFileVersion );

        if( newSymbol )
            newSymbols.emplace_back( newSymbol );

        reader.reset( new STRING_LINE_READER( *reader ) );
    }
    while( newSymbol );

    return newSymbols;
}


void SCH_IO_KICAD_SEXPR::FormatLibSymbol( LIB_SYMBOL* symbol, OUTPUTFORMATTER & formatter )
{

    LOCALE_IO toggle;     // toggles on, then off, the C locale.
    SCH_IO_KICAD_SEXPR_LIB_CACHE::SaveSymbol( symbol, formatter );
}


const char* SCH_IO_KICAD_SEXPR::PropBuffering = "buffering";
