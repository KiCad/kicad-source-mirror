/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
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

#include <fmt/format.h>

#include <wx/log.h>
#include <wx/mstream.h>

#include <base_units.h>
#include <bitmap_base.h>
#include <build_version.h>
#include <sch_selection.h>
#include <font/fontconfig.h>
#include <io/kicad/kicad_io_utils.h>
#include <libraries/symbol_library_adapter.h>
#include <progress_reporter.h>
#include <schematic.h>
#include <schematic_lexer.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_edit_frame.h>       // SYMBOL_ORIENTATION_T
#include <sch_group.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr_common.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr_lib_cache.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr_parser.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_pin.h>
#include <sch_rule_area.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_symbol.h>
#include <sch_table.h>
#include <sch_tablecell.h>
#include <sch_text.h>
#include <sch_textbox.h>
#include <string_utils.h>
#include <trace_helpers.h>
#include <reporter.h>

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


void SCH_IO_KICAD_SEXPR::init( SCHEMATIC* aSchematic,
                               const std::map<std::string, UTF8>* aProperties )
{
    m_version   = 0;
    m_appending = false;
    m_rootSheet = nullptr;
    m_schematic = aSchematic;
    m_cache     = nullptr;
    m_out       = nullptr;
}


SCH_SHEET* SCH_IO_KICAD_SEXPR::LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                                  SCH_SHEET* aAppendToMe,
                                                  const std::map<std::string, UTF8>* aProperties )
{
    wxASSERT( !aFileName || aSchematic != nullptr );

    SCH_SHEET*  sheet;

    wxFileName fn = aFileName;

    // Collect the font substitution warnings (RAII - automatically reset on scope exit)
    FONTCONFIG_REPORTER_SCOPE fontconfigScope( &LOAD_INFO_REPORTER::GetInstance() );

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
            m_path = aSchematic->Project().GetProjectPath();

        wxLogTrace( traceSchPlugin, "Normalized append path \"%s\".", m_path );
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

        wxFileName relPath( aFileName );

        // Do not use wxPATH_UNIX as option in MakeRelativeTo(). It can create incorrect
        // relative paths on Windows, because paths have a disk identifier (C:, D: ...)
        relPath.MakeRelativeTo( aSchematic->Project().GetProjectPath() );

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
            THROW_IO_ERROR( _( "Open canceled by user." ) );

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

    SCH_IO_KICAD_SEXPR_PARSER parser( &aReader );

    parser.ParseSchematic( aSheet, true, aFileVersion );
}


void SCH_IO_KICAD_SEXPR::SaveSchematicFile( const wxString& aFileName, SCH_SHEET* aSheet,
                                            SCHEMATIC*             aSchematic,
                                            const std::map<std::string, UTF8>* aProperties )
{
    wxCHECK_RET( aSheet != nullptr, "NULL SCH_SHEET object." );
    wxCHECK_RET( !aFileName.IsEmpty(), "No schematic file name defined." );

    wxString sanityResult = aSheet->GetScreen()->GroupsSanityCheck();

    if( sanityResult != wxEmptyString && m_queryUserCallback )
    {
        if( !m_queryUserCallback( _( "Internal Group Data Error" ), wxICON_ERROR,
                                  wxString::Format( _( "Please report this bug.  Error validating group "
                                                       "structure: %s\n\nSave anyway?" ),
                                                    sanityResult ),
                                  _( "Save Anyway" ) ) )
        {
            return;
        }
    }

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

    SCH_SHEET_LIST sheets = m_schematic->Hierarchy();
    SCH_SCREEN* screen = aSheet->GetScreen();

    wxCHECK( screen, /* void */ );

    // If we've requested to embed the fonts in the schematic, do so.
    // Otherwise, clear the embedded fonts from the schematic.  Embedded
    // fonts will be used if available
    if( m_schematic->GetAreFontsEmbedded() )
        m_schematic->EmbedFonts();
    else
        m_schematic->GetEmbeddedFiles()->ClearEmbeddedFonts();

    m_out->Print( "(kicad_sch (version %d) (generator \"eeschema\") (generator_version %s)",
                  SEXPR_SCHEMATIC_FILE_VERSION,
                  m_out->Quotew( GetMajorMinorVersion() ).c_str() );

    KICAD_FORMAT::FormatUuid( m_out, screen->m_uuid );

    screen->GetPageSettings().Format( m_out );
    screen->GetTitleBlock().Format( m_out );

    // Save cache library.
    m_out->Print( "(lib_symbols" );

    for( const auto& [ libItemName, libSymbol ] : screen->GetLibSymbols() )
        SCH_IO_KICAD_SEXPR_LIB_CACHE::SaveSymbol( libSymbol, *m_out, libItemName );

    m_out->Print( ")" );

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

    for( SCH_ITEM* item : save_map )
    {
        switch( item->Type() )
        {
        case SCH_SYMBOL_T:
            saveSymbol( static_cast<SCH_SYMBOL*>( item ), *m_schematic, sheets, false );
            break;

        case SCH_BITMAP_T:
            saveBitmap( static_cast<SCH_BITMAP&>( *item ) );
            break;

        case SCH_SHEET_T:
            saveSheet( static_cast<SCH_SHEET*>( item ), sheets );
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

        case SCH_SHAPE_T:
            saveShape( static_cast<SCH_SHAPE*>( item ) );
            break;

        case SCH_RULE_AREA_T:
            saveRuleArea( static_cast<SCH_RULE_AREA*>( item ) );
            break;

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_DIRECTIVE_LABEL_T:
            saveText( static_cast<SCH_TEXT*>( item ) );
            break;

        case SCH_TEXTBOX_T:
            saveTextBox( static_cast<SCH_TEXTBOX*>( item ) );
            break;

        case SCH_TABLE_T:
            saveTable( static_cast<SCH_TABLE*>( item ) );
            break;

        case SCH_GROUP_T:
            saveGroup( static_cast<SCH_GROUP*>( item ) );
            break;

        default:
            wxASSERT( "Unexpected schematic object type in SCH_IO_KICAD_SEXPR::Format()" );
        }
    }

    if( aSheet->HasRootInstance() )
    {
        std::vector< SCH_SHEET_INSTANCE> instances;

        instances.emplace_back( aSheet->GetRootInstance() );
        saveInstances( instances );

        KICAD_FORMAT::FormatBool( m_out, "embedded_fonts", m_schematic->GetAreFontsEmbedded() );

        // Save any embedded files
        if( !m_schematic->GetEmbeddedFiles()->IsEmpty() )
            m_schematic->WriteEmbeddedFiles( *m_out, true );
    }

    m_out->Print( ")" );
}


void SCH_IO_KICAD_SEXPR::Format( SCH_SELECTION* aSelection, SCH_SHEET_PATH* aSelectionPath,
                                 SCHEMATIC& aSchematic, OUTPUTFORMATTER* aFormatter,
                                 bool aForClipboard )
{
    wxCHECK( aSelection && aSelectionPath && aFormatter, /* void */ );

    SCH_SHEET_LIST sheets = aSchematic.Hierarchy();

    m_schematic = &aSchematic;
    m_out = aFormatter;

    std::map<wxString, LIB_SYMBOL*> libSymbols;
    SCH_SCREEN*                     screen = aSelection->GetScreen();
    std::set<SCH_TABLE*>            promotedTables;

    for( EDA_ITEM* item : *aSelection )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        wxString libSymbolLookup = symbol->GetLibId().Format().wx_str();

        if( !symbol->UseLibIdLookup() )
            libSymbolLookup = symbol->GetSchSymbolLibraryName();

        auto it = screen->GetLibSymbols().find( libSymbolLookup );

        if( it != screen->GetLibSymbols().end() )
            libSymbols[ libSymbolLookup ] = it->second;
    }

    if( !libSymbols.empty() )
    {
        m_out->Print( "(lib_symbols" );

        for( const auto& [name, libSymbol] : libSymbols )
            SCH_IO_KICAD_SEXPR_LIB_CACHE::SaveSymbol( libSymbol, *m_out, name, false );

        m_out->Print( ")" );
    }

    for( EDA_ITEM* edaItem : *aSelection )
    {
        if( !edaItem->IsSCH_ITEM() )
            continue;

        SCH_ITEM* item = static_cast<SCH_ITEM*>( edaItem );

        switch( item->Type() )
        {
        case SCH_SYMBOL_T:
            saveSymbol( static_cast<SCH_SYMBOL*>( item ), aSchematic, sheets, aForClipboard, aSelectionPath );
            break;

        case SCH_BITMAP_T:
            saveBitmap( static_cast<SCH_BITMAP&>( *item ) );
            break;

        case SCH_SHEET_T:
            saveSheet( static_cast<SCH_SHEET*>( item ), sheets );
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

        case SCH_SHAPE_T:
            saveShape( static_cast<SCH_SHAPE*>( item ) );
            break;

        case SCH_RULE_AREA_T:
            saveRuleArea( static_cast<SCH_RULE_AREA*>( item ) );
            break;

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_DIRECTIVE_LABEL_T:
            saveText( static_cast<SCH_TEXT*>( item ) );
            break;

        case SCH_TEXTBOX_T:
            saveTextBox( static_cast<SCH_TEXTBOX*>( item ) );
            break;

        case SCH_TABLECELL_T:
        {
            SCH_TABLE* table = static_cast<SCH_TABLE*>( item->GetParent() );

            if( promotedTables.count( table ) )
                break;

            table->SetFlags( SKIP_STRUCT );
            saveTable( table );
            table->ClearFlags( SKIP_STRUCT );
            promotedTables.insert( table );
            break;
        }

        case SCH_TABLE_T:
            item->ClearFlags( SKIP_STRUCT );
            saveTable( static_cast<SCH_TABLE*>( item ) );
            break;

        case SCH_GROUP_T:
            saveGroup( static_cast<SCH_GROUP*>( item ) );
            break;

        default:
            wxASSERT( "Unexpected schematic object type in SCH_IO_KICAD_SEXPR::Format()" );
        }
    }
}


void SCH_IO_KICAD_SEXPR::saveSymbol( SCH_SYMBOL* aSymbol, const SCHEMATIC& aSchematic,
                                     const SCH_SHEET_LIST& aSheetList, bool aForClipboard,
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

    m_out->Print( "(symbol" );

    if( !aSymbol->UseLibIdLookup() )
    {
        m_out->Print( "(lib_name %s)",
                      m_out->Quotew( aSymbol->GetSchSymbolLibraryName() ).c_str() );
    }

    m_out->Print( "(lib_id %s) (at %s %s %s)",
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
        m_out->Print( "(mirror %s %s)",
                      mirrorX ? "x" : "",
                      mirrorY ? "y" : "" );
    }

    // The symbol unit is always set to the ordianal instance regardless of the current sheet
    // instance to prevent file churn.
    SCH_SYMBOL_INSTANCE ordinalInstance;

    ordinalInstance.m_Reference = aSymbol->GetPrefix();

    const SCH_SCREEN* parentScreen = static_cast<const SCH_SCREEN*>( aSymbol->GetParent() );

    wxASSERT( parentScreen );

    if( parentScreen && m_schematic )
    {
        std::optional<SCH_SHEET_PATH> ordinalPath =
                m_schematic->Hierarchy().GetOrdinalPath( parentScreen );

        // Design blocks are saved from a temporary sheet & screen which will not be found in
        // the schematic, and will therefore have no ordinal path.
        // wxASSERT( ordinalPath );

        if( ordinalPath )
            aSymbol->GetInstance( ordinalInstance, ordinalPath->Path() );
        else if( aSymbol->GetInstances().size() )
            ordinalInstance = aSymbol->GetInstances()[0];
    }

    int unit = ordinalInstance.m_Unit;

    if( aForClipboard && aRelativePath )
    {
        SCH_SYMBOL_INSTANCE unitInstance;

        if( aSymbol->GetInstance( unitInstance, aRelativePath->Path() ) )
            unit = unitInstance.m_Unit;
    }

    m_out->Print( "(unit %d)", unit );
    m_out->Print( "(body_style %d)", aSymbol->GetBodyStyle() );

    KICAD_FORMAT::FormatBool( m_out, "exclude_from_sim", aSymbol->GetExcludedFromSim() );
    KICAD_FORMAT::FormatBool( m_out, "in_bom", !aSymbol->GetExcludedFromBOM() );
    KICAD_FORMAT::FormatBool( m_out, "on_board", !aSymbol->GetExcludedFromBoard() );
    KICAD_FORMAT::FormatBool( m_out, "in_pos_files", !aSymbol->GetExcludedFromPosFiles() );
    KICAD_FORMAT::FormatBool( m_out, "dnp", aSymbol->GetDNP() );

    AUTOPLACE_ALGO fieldsAutoplaced = aSymbol->GetFieldsAutoplaced();

    if( fieldsAutoplaced == AUTOPLACE_AUTO || fieldsAutoplaced == AUTOPLACE_MANUAL )
        KICAD_FORMAT::FormatBool( m_out, "fields_autoplaced", true );

    KICAD_FORMAT::FormatUuid( m_out, aSymbol->m_Uuid );

    std::vector<SCH_FIELD*> orderedFields;
    aSymbol->GetFields( orderedFields, false );

    for( SCH_FIELD* field : orderedFields )
    {
        FIELD_T  id = field->GetId();
        wxString value = field->GetText();

        if( !aForClipboard && aSymbol->GetInstances().size() )
        {
            // The instance fields are always set to the default instance regardless of the
            // sheet instance to prevent file churn.
            if( id == FIELD_T::REFERENCE )
                field->SetText( ordinalInstance.m_Reference );
        }
        else if( aForClipboard && aSymbol->GetInstances().size() && aRelativePath
               && ( id == FIELD_T::REFERENCE ) )
        {
            SCH_SYMBOL_INSTANCE instance;

            if( aSymbol->GetInstance( instance, aRelativePath->Path() ) )
                field->SetText( instance.m_Reference );
        }

        try
        {
            saveField( field );
        }
        catch( ... )
        {
            // Restore the changed field text on write error.
            if( id == FIELD_T::REFERENCE )
                field->SetText( value );

            throw;
        }

        if( id == FIELD_T::REFERENCE )
            field->SetText( value );
    }

    for( const std::unique_ptr<SCH_PIN>& pin : aSymbol->GetRawPins() )
    {
        // There was a bug introduced somewhere in the original alternated pin code that would
        // set the alternate pin to the default pin name which caused a number of library symbol
        // comparison issues.  Clearing the alternate pin resolves this issue.
        if( pin->GetAlt().IsEmpty() || ( pin->GetAlt() == pin->GetBaseName() ) )
        {
            m_out->Print( "(pin %s", m_out->Quotew( pin->GetNumber() ).c_str() );
            KICAD_FORMAT::FormatUuid( m_out, pin->m_Uuid );
            m_out->Print( ")" );
        }
        else
        {
            m_out->Print( "(pin %s", m_out->Quotew( pin->GetNumber() ).c_str() );
            KICAD_FORMAT::FormatUuid( m_out, pin->m_Uuid );
            m_out->Print( "(alternate %s))", m_out->Quotew( pin->GetAlt() ).c_str() );
        }
    }

    if( !aSymbol->GetInstances().empty() )
    {
        std::map<KIID, std::vector<SCH_SYMBOL_INSTANCE>> projectInstances;

        m_out->Print( "(instances" );

        wxString projectName;
        KIID     rootSheetUuid = aSchematic.Root().m_Uuid;

        for( const SCH_SYMBOL_INSTANCE& inst : aSymbol->GetInstances() )
        {
            // Zero length KIID_PATH objects are not valid and will cause a crash below.
            wxCHECK2( inst.m_Path.size(), continue );

            // If the instance data is part of this design but no longer has an associated sheet
            // path, don't save it.  This prevents large amounts of orphaned instance data for the
            // current project from accumulating in the schematic files.
            //
            // The root sheet UUID can be niluuid for the virtual root. In that case, instance
            // paths may include the virtual root, but SCH_SHEET_PATH::Path() skips it. We need
            // to normalize the path by removing the virtual root before comparison.
            KIID_PATH pathToCheck = inst.m_Path;
            bool hasVirtualRoot = false;

            // If root is virtual (niluuid) and path starts with virtual root, strip it
            if( rootSheetUuid == niluuid && !pathToCheck.empty() && pathToCheck[0] == niluuid )
            {
                if( pathToCheck.size() > 1 )
                {
                    pathToCheck.erase( pathToCheck.begin() );
                    hasVirtualRoot = true;
                }
                else
                {
                    // Path only contains virtual root, skip it
                    continue;
                }
            }

            // Check if this instance is orphaned (no matching sheet path)
            // For virtual root, we check if the first real sheet matches one of the top-level sheets
            // For non-virtual root, we check if it matches the root sheet UUID
            bool belongsToThisProject = hasVirtualRoot || pathToCheck[0] == rootSheetUuid;
            bool isOrphaned = belongsToThisProject && !aSheetList.GetSheetPathByKIIDPath( pathToCheck );

            // Keep all instance data when copying to the clipboard.  They may be needed on paste.
            if( !aForClipboard && isOrphaned )
                continue;

            // Group by project - use the first real sheet KIID (after stripping virtual root)
            KIID projectKey = pathToCheck[0];
            auto it = projectInstances.find( projectKey );

            if( it == projectInstances.end() )
                projectInstances[ projectKey ] = { inst };
            else
                it->second.emplace_back( inst );
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

            m_out->Print( "(project %s", m_out->Quotew( projectName ).c_str() );

            for( const SCH_SYMBOL_INSTANCE& instance : instances )
            {
                wxString path;
                KIID_PATH tmp = instance.m_Path;

                if( aForClipboard && aRelativePath )
                    tmp.MakeRelativeTo( aRelativePath->Path() );

                path = tmp.AsString();

                m_out->Print( "(path %s (reference %s) (unit %d)",
                              m_out->Quotew( path ).c_str(),
                              m_out->Quotew( instance.m_Reference ).c_str(),
                              instance.m_Unit );

                if( !instance.m_Variants.empty() )
                {
                    for( const auto&[name, variant] : instance.m_Variants )
                    {
                        m_out->Print( "(variant (name %s)", m_out->Quotew( name ).c_str() );

                        if( variant.m_DNP != aSymbol->GetDNP() )
                            KICAD_FORMAT::FormatBool( m_out, "dnp", variant.m_DNP );

                        if( variant.m_ExcludedFromSim != aSymbol->GetExcludedFromSim() )
                            KICAD_FORMAT::FormatBool( m_out, "exclude_from_sim", variant.m_ExcludedFromSim );

                        if( variant.m_ExcludedFromBOM != aSymbol->GetExcludedFromBOM() )
                            KICAD_FORMAT::FormatBool( m_out, "in_bom", variant.m_ExcludedFromBOM );

                        if( variant.m_ExcludedFromBoard != aSymbol->GetExcludedFromBoard() )
                            KICAD_FORMAT::FormatBool( m_out, "on_board", !variant.m_ExcludedFromBoard );

                        if( variant.m_ExcludedFromPosFiles != aSymbol->GetExcludedFromPosFiles() )
                            KICAD_FORMAT::FormatBool( m_out, "in_pos_files", !variant.m_ExcludedFromPosFiles );

                        for( const auto&[fname, fvalue] : variant.m_Fields )
                        {
                            m_out->Print( "(field (name %s) (value %s))",
                                          m_out->Quotew( fname ).c_str(), m_out->Quotew( fvalue ).c_str() );
                        }

                        m_out->Print( ")" );  // Closes `variant` token.
                    }
                }

                m_out->Print( ")" );  // Closes `path` token.
            }

            m_out->Print( ")" );  // Closes `project`.
        }

        m_out->Print( ")" );  // Closes `instances`.
    }

    m_out->Print( ")" );  // Closes `symbol`.
}


void SCH_IO_KICAD_SEXPR::saveField( SCH_FIELD* aField )
{
    wxCHECK_RET( aField != nullptr && m_out != nullptr, "" );

    wxString fieldName;

    if( aField->IsMandatory() )
        fieldName = aField->GetCanonicalName();
    else
        fieldName = aField->GetName();

    m_out->Print( "(property %s %s %s (at %s %s %s)",
                  aField->IsPrivate() ? "private" : "",
                  m_out->Quotew( fieldName ).c_str(),
                  m_out->Quotew( aField->GetText() ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aField->GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aField->GetPosition().y ).c_str(),
                  EDA_UNIT_UTILS::FormatAngle( aField->GetTextAngle() ).c_str() );

    if( !aField->IsVisible() )
        KICAD_FORMAT::FormatBool( m_out, "hide", true );

    KICAD_FORMAT::FormatBool( m_out, "show_name", aField->IsNameShown() );

    KICAD_FORMAT::FormatBool( m_out, "do_not_autoplace", !aField->CanAutoplace() );

    if( !aField->IsDefaultFormatting()
            || ( aField->GetTextHeight() != schIUScale.MilsToIU( DEFAULT_SIZE_TEXT ) ) )
    {
        aField->Format( m_out, 0 );
    }

    m_out->Print( ")" );            // Closes `property` token
}


void SCH_IO_KICAD_SEXPR::saveBitmap( const SCH_BITMAP& aBitmap )
{
    wxCHECK_RET( m_out != nullptr, "" );

    const REFERENCE_IMAGE& refImage = aBitmap.GetReferenceImage();
    const BITMAP_BASE&     bitmapBase = refImage.GetImage();

    const wxImage* image = bitmapBase.GetImageData();

    wxCHECK_RET( image != nullptr, "wxImage* is NULL" );

    m_out->Print( "(image (at %s %s)",
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       refImage.GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       refImage.GetPosition().y ).c_str() );

    double scale = refImage.GetImageScale();

    // 20230121 or older file format versions assumed 300 image PPI at load/save.
    // Let's keep compatibility by changing image scale.
    if( SEXPR_SCHEMATIC_FILE_VERSION <= 20230121 )
        scale = scale * 300.0 / bitmapBase.GetPPI();

    if( scale != 1.0 )
        m_out->Print( "%s", fmt::format("(scale {:g})", refImage.GetImageScale()).c_str() );

    KICAD_FORMAT::FormatUuid( m_out, aBitmap.m_Uuid );

    wxMemoryOutputStream stream;
    bitmapBase.SaveImageData( stream );

    KICAD_FORMAT::FormatStreamData( *m_out, *stream.GetOutputStreamBuffer() );

    m_out->Print( ")" );        // Closes image token.
}


void SCH_IO_KICAD_SEXPR::saveSheet( SCH_SHEET* aSheet, const SCH_SHEET_LIST& aSheetList )
{
    wxCHECK_RET( aSheet != nullptr && m_out != nullptr, "" );

    m_out->Print( "(sheet (at %s %s) (size %s %s)",
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aSheet->GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aSheet->GetPosition().y ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aSheet->GetSize().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aSheet->GetSize().y ).c_str() );

    KICAD_FORMAT::FormatBool( m_out, "exclude_from_sim", aSheet->GetExcludedFromSim() );
    KICAD_FORMAT::FormatBool( m_out, "in_bom", !aSheet->GetExcludedFromBOM() );
    KICAD_FORMAT::FormatBool( m_out, "on_board", !aSheet->GetExcludedFromBoard() );
    KICAD_FORMAT::FormatBool( m_out, "dnp", aSheet->GetDNP() );

    AUTOPLACE_ALGO fieldsAutoplaced = aSheet->GetFieldsAutoplaced();

    if( fieldsAutoplaced == AUTOPLACE_AUTO || fieldsAutoplaced == AUTOPLACE_MANUAL )
        KICAD_FORMAT::FormatBool( m_out, "fields_autoplaced", true );

    STROKE_PARAMS stroke( aSheet->GetBorderWidth(), LINE_STYLE::SOLID, aSheet->GetBorderColor() );

    stroke.SetWidth( aSheet->GetBorderWidth() );
    stroke.Format( m_out, schIUScale );

    m_out->Print( "(fill (color %d %d %d %s))",
                  KiROUND( aSheet->GetBackgroundColor().r * 255.0 ),
                  KiROUND( aSheet->GetBackgroundColor().g * 255.0 ),
                  KiROUND( aSheet->GetBackgroundColor().b * 255.0 ),
                  FormatDouble2Str( aSheet->GetBackgroundColor().a ).c_str() );

    KICAD_FORMAT::FormatUuid( m_out, aSheet->m_Uuid );

    for( SCH_FIELD& field : aSheet->GetFields() )
        saveField( &field );

    for( const SCH_SHEET_PIN* pin : aSheet->GetPins() )
    {
        m_out->Print( "(pin %s %s (at %s %s %s)",
                      EscapedUTF8( pin->GetText() ).c_str(),
                      getSheetPinShapeToken( pin->GetShape() ),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           pin->GetPosition().x ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           pin->GetPosition().y ).c_str(),
                      EDA_UNIT_UTILS::FormatAngle( getSheetPinAngle( pin->GetSide() ) ).c_str() );

        KICAD_FORMAT::FormatUuid( m_out, pin->m_Uuid );

        pin->Format( m_out, 0 );

        m_out->Print( ")" );  // Closes pin token.
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
        m_out->Print( "(instances" );

        KIID lastProjectUuid;
        KIID rootSheetUuid = m_schematic->Root().m_Uuid;
        bool inProjectClause = false;

        for( size_t i = 0; i < sheetInstances.size(); i++ )
        {
            // If the instance data is part of this design but no longer has an associated sheet
            // path, don't save it.  This prevents large amounts of orphaned instance data for the
            // current project from accumulating in the schematic files.
            //
            // Keep all instance data when copying to the clipboard.  It may be needed on paste.
            if( ( sheetInstances[i].m_Path[0] == rootSheetUuid )
                    && !aSheetList.GetSheetPathByKIIDPath( sheetInstances[i].m_Path, false ) )
            {
                if( inProjectClause && ( ( i + 1 == sheetInstances.size() )
                        || lastProjectUuid != sheetInstances[i+1].m_Path[0] ) )
                {
                    m_out->Print( ")" );  // Closes `project` token.
                    inProjectClause = false;
                }

                continue;
            }

            if( lastProjectUuid != sheetInstances[i].m_Path[0] )
            {
                wxString projectName;

                if( sheetInstances[i].m_Path[0] == rootSheetUuid )
                    projectName = m_schematic->Project().GetProjectName();
                else
                    projectName = sheetInstances[i].m_ProjectName;

                lastProjectUuid = sheetInstances[i].m_Path[0];
                m_out->Print( "(project %s", m_out->Quotew( projectName ).c_str() );
                inProjectClause = true;
            }

            wxString path = sheetInstances[i].m_Path.AsString();

            m_out->Print( "(path %s (page %s)",
                          m_out->Quotew( path ).c_str(),
                          m_out->Quotew( sheetInstances[i].m_PageNumber ).c_str() );

            if( !sheetInstances[i].m_Variants.empty() )
            {
                for( const auto&[name, variant] : sheetInstances[i].m_Variants )
                {
                    m_out->Print( "(variant (name %s)", m_out->Quotew( name ).c_str() );

                    if( variant.m_DNP != aSheet->GetDNP() )
                        KICAD_FORMAT::FormatBool( m_out, "dnp", variant.m_DNP );

                    if( variant.m_ExcludedFromSim != aSheet->GetExcludedFromSim() )
                        KICAD_FORMAT::FormatBool( m_out, "exclude_from_sim", variant.m_ExcludedFromSim );

                    if( variant.m_ExcludedFromBOM != aSheet->GetExcludedFromBOM() )
                        KICAD_FORMAT::FormatBool( m_out, "in_bom", variant.m_ExcludedFromBOM );

                    for( const auto&[fname, fvalue] : variant.m_Fields )
                    {
                        m_out->Print( "(field (name %s) (value %s))",
                                      m_out->Quotew( fname ).c_str(), m_out->Quotew( fvalue ).c_str() );
                    }

                    m_out->Print( ")" );  // Closes `variant` token.
                }
            }

            m_out->Print( ")" );     // Closes `path` token.

            if( inProjectClause && ( ( i + 1 == sheetInstances.size() )
                    || lastProjectUuid != sheetInstances[i+1].m_Path[0] ) )
            {
                m_out->Print( ")" );  // Closes `project` token.
                inProjectClause = false;
            }
        }

        m_out->Print( ")" );          // Closes `instances` token.
    }

    m_out->Print( ")" );              // Closes sheet token.
}


void SCH_IO_KICAD_SEXPR::saveJunction( SCH_JUNCTION* aJunction )
{
    wxCHECK_RET( aJunction != nullptr && m_out != nullptr, "" );

    m_out->Print( "(junction (at %s %s) (diameter %s) (color %d %d %d %s)",
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
    m_out->Print( ")" );
}


void SCH_IO_KICAD_SEXPR::saveNoConnect( SCH_NO_CONNECT* aNoConnect )
{
    wxCHECK_RET( aNoConnect != nullptr && m_out != nullptr, "" );

    m_out->Print( "(no_connect (at %s %s)",
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aNoConnect->GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aNoConnect->GetPosition().y ).c_str() );

    KICAD_FORMAT::FormatUuid( m_out, aNoConnect->m_Uuid );
    m_out->Print( ")" );
}


void SCH_IO_KICAD_SEXPR::saveBusEntry( SCH_BUS_ENTRY_BASE* aBusEntry )
{
    wxCHECK_RET( aBusEntry != nullptr && m_out != nullptr, "" );

    // Bus to bus entries are converted to bus line segments.
    if( aBusEntry->GetClass() == "SCH_BUS_BUS_ENTRY" )
    {
        SCH_LINE busEntryLine( aBusEntry->GetPosition(), LAYER_BUS );

        busEntryLine.SetEndPoint( aBusEntry->GetEnd() );
        saveLine( &busEntryLine );
        return;
    }

    m_out->Print( "(bus_entry (at %s %s) (size %s %s)",
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aBusEntry->GetPosition().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aBusEntry->GetPosition().y ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aBusEntry->GetSize().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aBusEntry->GetSize().y ).c_str() );

    aBusEntry->GetStroke().Format( m_out, schIUScale );
    KICAD_FORMAT::FormatUuid( m_out, aBusEntry->m_Uuid );
    m_out->Print( ")" );
}


void SCH_IO_KICAD_SEXPR::saveShape( SCH_SHAPE* aShape )
{
    wxCHECK_RET( aShape != nullptr && m_out != nullptr, "" );

    switch( aShape->GetShape() )
    {
    case SHAPE_T::ARC:
        formatArc( m_out, aShape, false, aShape->GetStroke(), aShape->GetFillMode(),
                   aShape->GetFillColor(), false, aShape->m_Uuid );
        break;

    case SHAPE_T::CIRCLE:
        formatCircle( m_out, aShape, false, aShape->GetStroke(), aShape->GetFillMode(),
                      aShape->GetFillColor(), false, aShape->m_Uuid );
        break;

    case SHAPE_T::RECTANGLE:
        formatRect( m_out, aShape, false, aShape->GetStroke(), aShape->GetFillMode(),
                    aShape->GetFillColor(), false, aShape->m_Uuid );
        break;

    case SHAPE_T::BEZIER:
        formatBezier( m_out, aShape, false, aShape->GetStroke(), aShape->GetFillMode(),
                      aShape->GetFillColor(), false, aShape->m_Uuid );
        break;

    case SHAPE_T::POLY:
        formatPoly( m_out, aShape, false, aShape->GetStroke(), aShape->GetFillMode(),
                    aShape->GetFillColor(), false, aShape->m_Uuid );
        break;

    default:
        UNIMPLEMENTED_FOR( aShape->SHAPE_T_asString() );
    }
}


void SCH_IO_KICAD_SEXPR::saveRuleArea( SCH_RULE_AREA* aRuleArea )
{
    wxCHECK_RET( aRuleArea != nullptr && m_out != nullptr, "" );

    m_out->Print( "(rule_area " );

    KICAD_FORMAT::FormatBool( m_out, "exclude_from_sim", aRuleArea->GetExcludedFromSim() );
    KICAD_FORMAT::FormatBool( m_out, "in_bom", !aRuleArea->GetExcludedFromBOM() );
    KICAD_FORMAT::FormatBool( m_out, "on_board", !aRuleArea->GetExcludedFromBoard() );
    KICAD_FORMAT::FormatBool( m_out, "dnp", aRuleArea->GetDNP() );

    saveShape( aRuleArea );

    m_out->Print( ")" );
}


void SCH_IO_KICAD_SEXPR::saveLine( SCH_LINE* aLine )
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

    m_out->Print( "(%s (pts (xy %s %s) (xy %s %s))",
                  TO_UTF8( lineType ),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aLine->GetStartPoint().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aLine->GetStartPoint().y ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aLine->GetEndPoint().x ).c_str(),
                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                       aLine->GetEndPoint().y ).c_str() );

    line_stroke.Format( m_out, schIUScale );
    KICAD_FORMAT::FormatUuid( m_out, aLine->m_Uuid );
    m_out->Print( ")" );
}


void SCH_IO_KICAD_SEXPR::saveText( SCH_TEXT* aText )
{
    wxCHECK_RET( aText != nullptr && m_out != nullptr, "" );

    // Note: label is nullptr SCH_TEXT, but not for SCH_LABEL_XXX,
    SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( aText );

    m_out->Print( "(%s %s",
                  getTextTypeToken( aText->Type() ),
                  m_out->Quotew( aText->GetText() ).c_str() );

    if( aText->Type() == SCH_TEXT_T )
        KICAD_FORMAT::FormatBool( m_out, "exclude_from_sim", aText->GetExcludedFromSim() );

    if( aText->Type() == SCH_DIRECTIVE_LABEL_T )
    {
        SCH_DIRECTIVE_LABEL* flag = static_cast<SCH_DIRECTIVE_LABEL*>( aText );

        m_out->Print( "(length %s)",
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
            m_out->Print( "(shape %s)", getSheetPinShapeToken( label->GetShape() ) );
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

    m_out->Print( "(at %s %s %s)",
                    EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                        aText->GetPosition().x ).c_str(),
                    EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                        aText->GetPosition().y ).c_str(),
                    EDA_UNIT_UTILS::FormatAngle( angle ).c_str() );

    if( label && !label->GetFields().empty() )
    {
        AUTOPLACE_ALGO fieldsAutoplaced = label->GetFieldsAutoplaced();

        if( fieldsAutoplaced == AUTOPLACE_AUTO || fieldsAutoplaced == AUTOPLACE_MANUAL )
            KICAD_FORMAT::FormatBool( m_out, "fields_autoplaced", true );
    }

    aText->EDA_TEXT::Format( m_out, 0 );
    KICAD_FORMAT::FormatUuid( m_out, aText->m_Uuid );

    if( label )
    {
        for( SCH_FIELD& field : label->GetFields() )
            saveField( &field );
    }

    m_out->Print( ")" );   // Closes text token.
}


void SCH_IO_KICAD_SEXPR::saveTextBox( SCH_TEXTBOX* aTextBox )
{
    wxCHECK_RET( aTextBox != nullptr && m_out != nullptr, "" );

    m_out->Print( "(%s %s",
                  aTextBox->Type() == SCH_TABLECELL_T ? "table_cell" : "text_box",
                  m_out->Quotew( aTextBox->GetText() ).c_str() );

    KICAD_FORMAT::FormatBool( m_out, "exclude_from_sim", aTextBox->GetExcludedFromSim() );

    VECTOR2I pos = aTextBox->GetStart();
    VECTOR2I size = aTextBox->GetEnd() - pos;

    m_out->Print( "(at %s %s %s) (size %s %s) (margins %s %s %s %s)",
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
        m_out->Print( "(span %d %d)", cell->GetColSpan(), cell->GetRowSpan() );

    if( aTextBox->Type() != SCH_TABLECELL_T )
        aTextBox->GetStroke().Format( m_out, schIUScale );

    formatFill( m_out, aTextBox->GetFillMode(), aTextBox->GetFillColor() );
    aTextBox->EDA_TEXT::Format( m_out, 0 );
    KICAD_FORMAT::FormatUuid( m_out, aTextBox->m_Uuid );
    m_out->Print( ")" );
}


void SCH_IO_KICAD_SEXPR::saveTable( SCH_TABLE* aTable )
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

    m_out->Print( "(table (column_count %d)", aTable->GetColCount() );

    m_out->Print( "(border" );
    KICAD_FORMAT::FormatBool( m_out, "external", aTable->StrokeExternal() );
    KICAD_FORMAT::FormatBool( m_out, "header", aTable->StrokeHeaderSeparator() );

    if( aTable->StrokeExternal() || aTable->StrokeHeaderSeparator() )
        aTable->GetBorderStroke().Format( m_out, schIUScale );

    m_out->Print( ")" );               // Close `border` token.

    m_out->Print( "(separators" );
    KICAD_FORMAT::FormatBool( m_out, "rows", aTable->StrokeRows() );
    KICAD_FORMAT::FormatBool( m_out, "cols", aTable->StrokeColumns() );

    if( aTable->StrokeRows() || aTable->StrokeColumns() )
        aTable->GetSeparatorsStroke().Format( m_out, schIUScale );

    m_out->Print( ")" );               // Close `separators` token.

    m_out->Print( "(column_widths" );

    for( int col = 0; col < aTable->GetColCount(); ++col )
    {
        m_out->Print( " %s",
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTable->GetColWidth( col ) ).c_str() );
    }

    m_out->Print( ")" );

    m_out->Print( "(row_heights" );

    for( int row = 0; row < aTable->GetRowCount(); ++row )
    {
        m_out->Print( " %s",
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTable->GetRowHeight( row ) ).c_str() );
    }

    m_out->Print( ")" );

    KICAD_FORMAT::FormatUuid( m_out, aTable->m_Uuid );

    m_out->Print( "(cells" );

    for( SCH_TABLECELL* cell : aTable->GetCells() )
        saveTextBox( cell );

    m_out->Print( ")" );        // Close `cells` token.
    m_out->Print( ")" );        // Close `table` token.

    if( aTable->GetFlags() & SKIP_STRUCT )
        delete aTable;
}


void SCH_IO_KICAD_SEXPR::saveGroup( SCH_GROUP* aGroup )
{
    // Don't write empty groups
    if( aGroup->GetItems().empty() )
        return;

    m_out->Print( "(group %s", m_out->Quotew( aGroup->GetName() ).c_str() );

    KICAD_FORMAT::FormatUuid( m_out, aGroup->m_Uuid );

    if( aGroup->IsLocked() )
        KICAD_FORMAT::FormatBool( m_out, "locked", true );

    if( aGroup->HasDesignBlockLink() )
        m_out->Print( "(lib_id \"%s\")", aGroup->GetDesignBlockLibId().Format().c_str() );

    wxArrayString memberIds;

    for( EDA_ITEM* member : aGroup->GetItems() )
        memberIds.Add( member->m_Uuid.AsString() );

    memberIds.Sort();

    m_out->Print( "(members" );

    for( const wxString& memberId : memberIds )
        m_out->Print( " %s", m_out->Quotew( memberId ).c_str() );

    m_out->Print( ")" ); // Close `members` token.
    m_out->Print( ")" ); // Close `group` token.
}


void SCH_IO_KICAD_SEXPR::saveInstances( const std::vector<SCH_SHEET_INSTANCE>& aInstances )
{
    if( aInstances.size() )
    {
        m_out->Print( "(sheet_instances" );

        for( const SCH_SHEET_INSTANCE& instance : aInstances )
        {
            wxString path = instance.m_Path.AsString();

            if( path.IsEmpty() )
                path = wxT( "/" ); // Root path

            m_out->Print( "(path %s (page %s))",
                          m_out->Quotew( path ).c_str(),
                          m_out->Quotew( instance.m_PageNumber ).c_str() );
        }

        m_out->Print( ")" );    // Close sheet instances token.
    }
}


void SCH_IO_KICAD_SEXPR::cacheLib( const wxString& aLibraryFileName,
                                   const std::map<std::string, UTF8>* aProperties )
{
    // Suppress font substitution warnings (RAII - automatically restored on scope exit)
    FONTCONFIG_REPORTER_SCOPE fontconfigScope( nullptr );

    if( !m_cache || !m_cache->IsFile( aLibraryFileName ) || m_cache->IsFileChanged() )
    {
        int  oldModifyHash = 1;
        bool isNewCache = false;

        if( m_cache )
            oldModifyHash = m_cache->m_modHash;
        else
            isNewCache = true;

        // a spectacular episode in memory management:
        delete m_cache;
        m_cache = new SCH_IO_KICAD_SEXPR_LIB_CACHE( aLibraryFileName );

        if( !isBuffering( aProperties ) || isNewCache )
        {
            m_cache->Load();
            m_cache->m_modHash = oldModifyHash + 1;
        }
    }
}


bool SCH_IO_KICAD_SEXPR::isBuffering( const std::map<std::string, UTF8>* aProperties )
{
    return ( aProperties && aProperties->contains( SCH_IO_KICAD_SEXPR::PropBuffering ) );
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
                                             const std::map<std::string, UTF8>* aProperties )
{
    bool powerSymbolsOnly = ( aProperties && aProperties->contains( SYMBOL_LIBRARY_ADAPTER::PropPowerSymsOnly ) );

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
                                             const std::map<std::string, UTF8>* aProperties )
{
    bool powerSymbolsOnly = ( aProperties && aProperties->contains( SYMBOL_LIBRARY_ADAPTER::PropPowerSymsOnly ) );

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
                                            const std::map<std::string, UTF8>* aProperties )
{
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
                                     const std::map<std::string, UTF8>* aProperties )
{
    cacheLib( aLibraryPath, aProperties );

    m_cache->AddSymbol( aSymbol );

    if( !isBuffering( aProperties ) )
        m_cache->Save();
}


void SCH_IO_KICAD_SEXPR::DeleteSymbol( const wxString& aLibraryPath, const wxString& aSymbolName,
                                       const std::map<std::string, UTF8>* aProperties )
{
    cacheLib( aLibraryPath, aProperties );

    m_cache->DeleteSymbol( aSymbolName );

    if( !isBuffering( aProperties ) )
        m_cache->Save();
}


void SCH_IO_KICAD_SEXPR::CreateLibrary( const wxString& aLibraryPath,
                                        const std::map<std::string, UTF8>* aProperties )
{
    wxFileName fn( aLibraryPath );

    if( !fn.IsDir() )
    {
        if( fn.FileExists() )
            THROW_IO_ERROR( wxString::Format( _( "Symbol library file '%s' already exists." ), fn.GetFullPath() ) );
    }
    else
    {
        if( fn.DirExists() )
            THROW_IO_ERROR( wxString::Format( _( "Symbol library path '%s' already exists." ), fn.GetPath() ) );
    }

    delete m_cache;
    m_cache = new SCH_IO_KICAD_SEXPR_LIB_CACHE( aLibraryPath );
    m_cache->SetModified();
    m_cache->Save();
    m_cache->Load();    // update m_writable and m_timestamp
}


bool SCH_IO_KICAD_SEXPR::DeleteLibrary( const wxString& aLibraryPath,
                                        const std::map<std::string, UTF8>* aProperties )
{
    wxFileName fn = aLibraryPath;

    if( !fn.FileExists() )
        return false;

    // Some of the more elaborate wxRemoveFile() crap puts up its own wxLog dialog
    // we don't want that.  we want bare metal portability with no UI here.
    if( !fn.IsDir() )
    {
        if( wxRemove( aLibraryPath ) )
            THROW_IO_ERROR( wxString::Format( _( "Symbol library file '%s' cannot be deleted." ),
                                              aLibraryPath.GetData() ) );
    }
    else
    {
        // This may be overly agressive.  Perhaps in the future we should remove all of the *.kicad_sym
        // files and only delete the folder if it's empty.
        if( !fn.Rmdir( wxPATH_RMDIR_RECURSIVE ) )
            THROW_IO_ERROR( wxString::Format( _( "Symbol library folder '%s' cannot be deleted." ),
                                              fn.GetPath() ) );
    }

    if( m_cache && m_cache->IsFile( aLibraryPath ) )
    {
        delete m_cache;
        m_cache = nullptr;
    }

    return true;
}


void SCH_IO_KICAD_SEXPR::SaveLibrary( const wxString& aLibraryPath,
                                      const std::map<std::string, UTF8>* aProperties )
{
    if( !m_cache )
        m_cache = new SCH_IO_KICAD_SEXPR_LIB_CACHE( aLibraryPath );

    wxString oldFileName = m_cache->GetFileName();

    if( !m_cache->IsFile( aLibraryPath ) )
        m_cache->SetFileName( aLibraryPath );

    // This is a forced save.
    m_cache->SetModified();
    m_cache->Save();
    m_cache->SetFileName( oldFileName );
}


bool SCH_IO_KICAD_SEXPR::CanReadLibrary( const wxString& aLibraryPath ) const
{
    if( !SCH_IO::CanReadLibrary( aLibraryPath ) )
        return false;

    // Above just checks for proper extension; now check that it actually exists

    wxFileName fn( aLibraryPath );
    return fn.IsOk() && fn.FileExists();
}


bool SCH_IO_KICAD_SEXPR::IsLibraryWritable( const wxString& aLibraryPath )
{
    wxFileName fn( aLibraryPath );

    if( fn.FileExists() )
        return fn.IsFileWritable();

    return fn.IsDirWritable();
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
    SCH_IO_KICAD_SEXPR_LIB_CACHE::SaveSymbol( symbol, formatter );
}


const char* SCH_IO_KICAD_SEXPR::PropBuffering = "buffering";
