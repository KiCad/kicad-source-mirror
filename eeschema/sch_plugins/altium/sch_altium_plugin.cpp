/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
 * Copyright (C) 2021-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <memory>

#include "altium_parser_sch.h"
#include "sch_shape.h"
#include <plugins/altium/altium_parser.h>
#include <plugins/altium/altium_parser_utils.h>
#include <sch_plugins/altium/sch_altium_plugin.h>

#include <schematic.h>

#include <lib_shape.h>
#include <lib_id.h>
#include <lib_pin.h>
#include <lib_text.h>

#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_symbol.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_screen.h>
#include <sch_label.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_textbox.h>

#include <bezier_curves.h>
#include <compoundfilereader.h>
#include <string_utils.h>
#include <sch_edit_frame.h>
#include <trigo.h>
#include <wildcards_and_files_ext.h>
#include <wx/mstream.h>
#include <wx/log.h>
#include <wx/zstream.h>
#include <wx/wfstream.h>
#include <trigo.h>

// Harness port object itself does not contain color information about itself
// It seems altium is drawing harness ports using these colors
#define HARNESS_PORT_COLOR_DEFAULT_BACKGROUND COLOR4D( 0.92941176470588238, \
                                                       0.94901960784313721, \
                                                       0.98431372549019602, 1.0 )

#define HARNESS_PORT_COLOR_DEFAULT_OUTLINE    COLOR4D( 0.56078431372549020, \
                                                       0.61960784313725492, \
                                                       0.78823529411764703, 1.0 )


static const VECTOR2I GetRelativePosition( const VECTOR2I& aPosition, const SCH_SYMBOL* aSymbol )
{
    TRANSFORM t = aSymbol->GetTransform().InverseTransform();
    return t.TransformCoordinate( aPosition - aSymbol->GetPosition() );
}


static COLOR4D GetColorFromInt( int color )
{
    int red   = color & 0x0000FF;
    int green = ( color & 0x00FF00 ) >> 8;
    int blue  = ( color & 0xFF0000 ) >> 16;

    return COLOR4D().FromCSSRGBA( red, green, blue, 1.0 );
}


static PLOT_DASH_TYPE GetPlotDashType( const ASCH_POLYLINE_LINESTYLE linestyle )
{
    switch( linestyle )
    {
    case ASCH_POLYLINE_LINESTYLE::SOLID: return PLOT_DASH_TYPE::SOLID;
    case ASCH_POLYLINE_LINESTYLE::DASHED: return PLOT_DASH_TYPE::DASH;
    case ASCH_POLYLINE_LINESTYLE::DOTTED: return PLOT_DASH_TYPE::DOT;
    case ASCH_POLYLINE_LINESTYLE::DASH_DOTTED: return PLOT_DASH_TYPE::DASHDOT;
    default: return PLOT_DASH_TYPE::DEFAULT;
    }
}


static void SetSchShapeFillAndColor( const ASCH_SHAPE_INTERFACE& elem, SCH_SHAPE* shape )
{
    shape->SetStroke( STROKE_PARAMS( elem.LineWidth, PLOT_DASH_TYPE::SOLID,
                                     GetColorFromInt( elem.Color ) ) );

    if( !elem.IsSolid )
    {
        shape->SetFillMode( FILL_T::NO_FILL );
    }
    else
    {
        shape->SetFillMode( FILL_T::FILLED_WITH_COLOR );
        shape->SetFillColor( GetColorFromInt( elem.AreaColor ) );
    }
}


static void SetLibShapeFillAndColor( const ASCH_SHAPE_INTERFACE& elem, LIB_SHAPE* shape )
{
    shape->SetStroke( STROKE_PARAMS( elem.LineWidth, PLOT_DASH_TYPE::SOLID ) );

    if( !elem.IsSolid )
    {
        shape->SetFillMode( FILL_T::NO_FILL );
    }
    else if( elem.Color == elem.AreaColor )
    {
        shape->SetFillMode( FILL_T::FILLED_SHAPE );
    }
    else
    {
        shape->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );
    }
}


SCH_ALTIUM_PLUGIN::SCH_ALTIUM_PLUGIN()
{
    m_rootSheet    = nullptr;
    m_schematic    = nullptr;
    m_harnessOwnerIndexOffset = 0;
    m_harnessEntryParent      = 0;

    m_reporter     = &WXLOG_REPORTER::GetInstance();
}


SCH_ALTIUM_PLUGIN::~SCH_ALTIUM_PLUGIN()
{
}


const wxString SCH_ALTIUM_PLUGIN::GetName() const
{
    return "Altium";
}


const wxString SCH_ALTIUM_PLUGIN::GetFileExtension() const
{
    return "SchDoc";
}


const wxString SCH_ALTIUM_PLUGIN::GetLibraryFileExtension() const
{
    return "SchLib";
}


int SCH_ALTIUM_PLUGIN::GetModifyHash() const
{
    return 0;
}


bool SCH_ALTIUM_PLUGIN::CheckHeader( const wxString& aFileName )
{
    // TODO

    return true;
}


wxString SCH_ALTIUM_PLUGIN::getLibName()
{
    if( m_libName.IsEmpty() )
    {
        // Try to come up with a meaningful name
        m_libName = m_schematic->Prj().GetProjectName();

        if( m_libName.IsEmpty() )
        {
            wxFileName fn( m_rootSheet->GetFileName() );
            m_libName = fn.GetName();
        }

        if( m_libName.IsEmpty() )
            m_libName = "noname";

        m_libName += "-altium-import";
        m_libName = LIB_ID::FixIllegalChars( m_libName, true );
    }

    return m_libName;
}


wxFileName SCH_ALTIUM_PLUGIN::getLibFileName()
{
    wxFileName fn( m_schematic->Prj().GetProjectPath(), getLibName(), KiCadSymbolLibFileExtension );

    return fn;
}


SCH_SHEET* SCH_ALTIUM_PLUGIN::Load( const wxString& aFileName, SCHEMATIC* aSchematic,
                                    SCH_SHEET* aAppendToMe, const STRING_UTF8_MAP* aProperties )
{
    wxCHECK( !aFileName.IsEmpty() && aSchematic, nullptr );

    wxFileName fileName( aFileName );
    fileName.SetExt( KiCadSchematicFileExtension );
    m_schematic = aSchematic;

    // Delete on exception, if I own m_rootSheet, according to aAppendToMe
    std::unique_ptr<SCH_SHEET> deleter( aAppendToMe ? nullptr : m_rootSheet );

    if( aAppendToMe )
    {
        wxCHECK_MSG( aSchematic->IsValid(), nullptr, "Can't append to a schematic with no root!" );
        m_rootSheet = &aSchematic->Root();
    }
    else
    {
        m_rootSheet = new SCH_SHEET( aSchematic );
        m_rootSheet->SetFileName( fileName.GetFullPath() );

        aSchematic->SetRoot( m_rootSheet );

        SCH_SHEET_PATH sheetpath;
        sheetpath.push_back( m_rootSheet );

        // We'll update later if we find a pageNumber record for it.
        sheetpath.SetPageNumber( "#" );
    }

    if( !m_rootSheet->GetScreen() )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( m_schematic );
        screen->SetFileName( aFileName );
        m_rootSheet->SetScreen( screen );
        const_cast<KIID&>( m_rootSheet->m_Uuid ) = screen->GetUuid();
    }

    SYMBOL_LIB_TABLE* libTable = m_schematic->Prj().SchSymbolLibTable();

    wxCHECK_MSG( libTable, nullptr, "Could not load symbol lib table." );

    m_pi.set( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

    /// @note No check is being done here to see if the existing symbol library exists so this
    ///       will overwrite the existing one.
    if( !libTable->HasLibrary( getLibName() ) )
    {
        // Create a new empty symbol library.
        m_pi->CreateSymbolLib( getLibFileName().GetFullPath() );
        wxString libTableUri = "${KIPRJMOD}/" + getLibFileName().GetFullName();

        // Add the new library to the project symbol library table.
        libTable->InsertRow( new SYMBOL_LIB_TABLE_ROW( getLibName(), libTableUri,
                                                       wxString( "KiCad" ) ) );

        // Save project symbol library table.
        wxFileName fn( m_schematic->Prj().GetProjectPath(),
                       SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

        // So output formatter goes out of scope and closes the file before reloading.
        {
            FILE_OUTPUTFORMATTER formatter( fn.GetFullPath() );
            libTable->Format( &formatter, 0 );
        }

        // Reload the symbol library table.
        m_schematic->Prj().SetElem( PROJECT::ELEM_SYMBOL_LIB_TABLE, nullptr );
        m_schematic->Prj().SchSymbolLibTable();
    }

    m_sheetPath.push_back( m_rootSheet );

    SCH_SCREEN* rootScreen = m_rootSheet->GetScreen();
    wxCHECK( rootScreen, nullptr );

    SCH_SHEET_INSTANCE sheetInstance;

    sheetInstance.m_Path = m_sheetPath.Path();
    sheetInstance.m_PageNumber = wxT( "#" );

    rootScreen->m_sheetInstances.emplace_back( sheetInstance );

    ParseAltiumSch( aFileName );

    m_pi->SaveLibrary( getLibFileName().GetFullPath() );

    SCH_SCREENS allSheets( m_rootSheet );
    allSheets.UpdateSymbolLinks(); // Update all symbol library links for all sheets.
    allSheets.ClearEditFlags();

    return m_rootSheet;
}


SCH_SCREEN* SCH_ALTIUM_PLUGIN::getCurrentScreen()
{
    return m_sheetPath.LastScreen();
}


SCH_SHEET* SCH_ALTIUM_PLUGIN::getCurrentSheet()
{
    return m_sheetPath.Last();
}


void SCH_ALTIUM_PLUGIN::ParseAltiumSch( const wxString& aFileName )
{
    ALTIUM_COMPOUND_FILE altiumSchFile( aFileName );

    // Load path may be different from the project path.
    wxFileName parentFileName = aFileName;

    try
    {
        ParseStorage( altiumSchFile ); // we need this before parsing the FileHeader
        ParseFileHeader( altiumSchFile );

        // Parse "Additional" because sheet is set up during "FileHeader" parsing.
        ParseAdditional( altiumSchFile );
    }
    catch( const CFB::CFBException& exception )
    {
        THROW_IO_ERROR( exception.what() );
    }
    catch( const std::exception& exc )
    {
        wxLogDebug( wxT( "Unhandled exception in Altium schematic parsers: %s." ), exc.what() );
        throw;
    }

    SCH_SCREEN* currentScreen = getCurrentScreen();
    wxCHECK( currentScreen, /* void */ );

    // Descend the sheet hierarchy.
    for( SCH_ITEM* item : currentScreen->Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SCREEN* loadedScreen = nullptr;
        SCH_SHEET* sheet = dynamic_cast<SCH_SHEET*>( item );

        wxCHECK2( sheet, continue );

        // The assumption is that all of the Altium schematic files will be in the same
        // path as the parent sheet path.
        wxFileName loadAltiumFileName( parentFileName.GetPath(), sheet->GetFileName() );

        if( loadAltiumFileName.GetFullName().IsEmpty() || !loadAltiumFileName.IsFileReadable() )
        {
            wxString msg;

            msg.Printf( _( "The file name for sheet %s is undefined, this is probably an"
                           " Altium signal harness that got converted to a sheet." ),
                        sheet->GetName() );
            m_reporter->Report( msg );
            sheet->SetScreen( new SCH_SCREEN( m_schematic ) );
            continue;
        }

        m_rootSheet->SearchHierarchy( loadAltiumFileName.GetFullPath(), &loadedScreen );

        if( loadedScreen )
        {
            sheet->SetScreen( loadedScreen );
            // Do not need to load the sub-sheets - this has already been done.
        }
        else
        {
            sheet->SetScreen( new SCH_SCREEN( m_schematic ) );
            SCH_SCREEN* screen = sheet->GetScreen();
            sheet->SetName( loadAltiumFileName.GetName() );
            wxCHECK2( screen, continue );

            m_sheetPath.push_back( sheet );
            ParseAltiumSch( loadAltiumFileName.GetFullPath() );

            // Map the loaded Altium file to the project file.
            wxFileName projectFileName = loadAltiumFileName;
            projectFileName.SetPath( m_schematic->Prj().GetProjectPath() );
            projectFileName.SetExt( KiCadSchematicFileExtension );
            sheet->SetFileName( projectFileName.GetFullName() );
            screen->SetFileName( projectFileName.GetFullPath() );

            m_sheetPath.pop_back();
        }
    }
}


void SCH_ALTIUM_PLUGIN::ParseStorage( const ALTIUM_COMPOUND_FILE& aAltiumSchFile )
{
    const CFB::COMPOUND_FILE_ENTRY* file = aAltiumSchFile.FindStream( { "Storage" } );

    if( file == nullptr )
        return;

    ALTIUM_PARSER reader( aAltiumSchFile, file );

    std::map<wxString, wxString> properties = reader.ReadProperties();
    wxString header = ALTIUM_PARSER::ReadString( properties, "HEADER", "" );
    int      weight = ALTIUM_PARSER::ReadInt( properties, "WEIGHT", 0 );

    if( weight < 0 )
        THROW_IO_ERROR( "Storage weight is negative!" );

    for( int i = 0; i < weight; i++ )
    {
        m_altiumStorage.emplace_back( reader );
    }

    if( reader.HasParsingError() )
        THROW_IO_ERROR( "stream was not parsed correctly!" );

    // TODO pointhi: is it possible to have multiple headers in one Storage file? Otherwise
    // throw IO Error.
    if( reader.GetRemainingBytes() != 0 )
    {
        m_reporter->Report( wxString::Format( _( "Storage file not fully parsed "
                                                 "(%d bytes remaining)." ),
                                              reader.GetRemainingBytes() ),
                            RPT_SEVERITY_ERROR );
    }
}


void SCH_ALTIUM_PLUGIN::ParseAdditional( const ALTIUM_COMPOUND_FILE& aAltiumSchFile )
{
    const CFB::COMPOUND_FILE_ENTRY* file = aAltiumSchFile.FindStream( { "Additional" } );

    if( file == nullptr )
        return;

    ALTIUM_PARSER reader( aAltiumSchFile, file );


    if( reader.GetRemainingBytes() <= 0 )
    {
        THROW_IO_ERROR( "Additional section does not contain any data" );
    }
    else
    {
        std::map<wxString, wxString> properties = reader.ReadProperties();

        int               recordId = ALTIUM_PARSER::ReadInt( properties, "RECORD", 0 );
        ALTIUM_SCH_RECORD record = static_cast<ALTIUM_SCH_RECORD>( recordId );

        if( record != ALTIUM_SCH_RECORD::HEADER )
            THROW_IO_ERROR( "Header expected" );
    }

    for( int index = 0; reader.GetRemainingBytes() > 0; index++ )
    {
        std::map<wxString, wxString> properties = reader.ReadProperties();

        int               recordId = ALTIUM_PARSER::ReadInt( properties, "RECORD", 0 );
        ALTIUM_SCH_RECORD record = static_cast<ALTIUM_SCH_RECORD>( recordId );

        // see: https://github.com/vadmium/python-altium/blob/master/format.md
        switch( record )
        {
        case ALTIUM_SCH_RECORD::HARNESS_CONNECTOR:
            ParseHarnessConnector( index, properties );
            break;

        case ALTIUM_SCH_RECORD::HARNESS_ENTRY:
            ParseHarnessEntry( properties );
            break;

        case ALTIUM_SCH_RECORD::HARNESS_TYPE:
            ParseHarnessType( properties );
            break;

        case ALTIUM_SCH_RECORD::SIGNAL_HARNESS:
            ParseSignalHarness( properties );
            break;

        case ALTIUM_SCH_RECORD::BLANKET:
            m_reporter->Report( _( "Blanket not currently supported." ), RPT_SEVERITY_ERROR );
            break;

        default:
            m_reporter->Report( wxString::Format( _( "Unknown or unexpected record ID %d found "
                                                     "inside \"Additional\" section." ),
                                                  recordId ),
                                RPT_SEVERITY_ERROR );
            break;
        }
    }

    // Handle harness Ports
    for( const ASCH_PORT& port : m_altiumHarnessPortsCurrentSheet )
        ParseHarnessPort( port );

    if( reader.HasParsingError() )
        THROW_IO_ERROR( "stream was not parsed correctly!" );

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( "stream is not fully parsed" );
}


void SCH_ALTIUM_PLUGIN::ParseFileHeader( const ALTIUM_COMPOUND_FILE& aAltiumSchFile )
{
    const CFB::COMPOUND_FILE_ENTRY* file = aAltiumSchFile.FindStream( { "FileHeader" } );

    if( file == nullptr )
        THROW_IO_ERROR( "FileHeader not found" );

    ALTIUM_PARSER reader( aAltiumSchFile, file );

    if( reader.GetRemainingBytes() <= 0 )
    {
        THROW_IO_ERROR( "FileHeader does not contain any data" );
    }
    else
    {
        std::map<wxString, wxString> properties = reader.ReadProperties();

        int               recordId = ALTIUM_PARSER::ReadInt( properties, "RECORD", 0 );
        ALTIUM_SCH_RECORD record   = static_cast<ALTIUM_SCH_RECORD>( recordId );

        if( record != ALTIUM_SCH_RECORD::HEADER )
            THROW_IO_ERROR( "Header expected" );
    }

    // Prepare some local variables
    wxCHECK( m_altiumPortsCurrentSheet.empty(), /* void */ );
    wxCHECK( !m_currentTitleBlock, /* void */ );

    m_currentTitleBlock = std::make_unique<TITLE_BLOCK>();

    // index is required to resolve OWNERINDEX
    for( int index = 0; reader.GetRemainingBytes() > 0; index++ )
    {
        std::map<wxString, wxString> properties = reader.ReadProperties();

        int               recordId = ALTIUM_PARSER::ReadInt( properties, "RECORD", 0 );
        ALTIUM_SCH_RECORD record   = static_cast<ALTIUM_SCH_RECORD>( recordId );

        // see: https://github.com/vadmium/python-altium/blob/master/format.md
        switch( record )
        {
        case ALTIUM_SCH_RECORD::HEADER:
            THROW_IO_ERROR( "Header already parsed" );

        case ALTIUM_SCH_RECORD::COMPONENT:
            ParseComponent( index, properties );
            break;

        case ALTIUM_SCH_RECORD::PIN:
            ParsePin( properties );
            break;

        case ALTIUM_SCH_RECORD::IEEE_SYMBOL:
            m_reporter->Report( _( "Record 'IEEE_SYMBOL' not handled." ),
                                RPT_SEVERITY_INFO );
            break;

        case ALTIUM_SCH_RECORD::LABEL:
            ParseLabel( properties );
            break;

        case ALTIUM_SCH_RECORD::BEZIER:
            ParseBezier( properties );
            break;

        case ALTIUM_SCH_RECORD::POLYLINE:
            ParsePolyline( properties );
            break;

        case ALTIUM_SCH_RECORD::POLYGON:
            ParsePolygon( properties );
            break;

        case ALTIUM_SCH_RECORD::ELLIPSE:
            ParseEllipse( properties );
            break;

        case ALTIUM_SCH_RECORD::PIECHART:
            m_reporter->Report( _( "Record 'PIECHART' not handled." ),
                                RPT_SEVERITY_INFO );
            break;

        case ALTIUM_SCH_RECORD::ROUND_RECTANGLE:
            ParseRoundRectangle( properties );
            break;

        case ALTIUM_SCH_RECORD::ELLIPTICAL_ARC:
        case ALTIUM_SCH_RECORD::ARC:
            ParseArc( properties );
            break;

        case ALTIUM_SCH_RECORD::LINE:
            ParseLine( properties );
            break;

        case ALTIUM_SCH_RECORD::RECTANGLE:
            ParseRectangle( properties );
            break;

        case ALTIUM_SCH_RECORD::SHEET_SYMBOL:
            ParseSheetSymbol( index, properties );
            break;

        case ALTIUM_SCH_RECORD::SHEET_ENTRY:
            ParseSheetEntry( properties );
            break;

        case ALTIUM_SCH_RECORD::POWER_PORT:
            ParsePowerPort( properties );
            break;

        case ALTIUM_SCH_RECORD::PORT:
            // Ports are parsed after the sheet was parsed
            // This is required because we need all electrical connection points before placing.
            m_altiumPortsCurrentSheet.emplace_back( properties );
            break;

        case ALTIUM_SCH_RECORD::NO_ERC:
            ParseNoERC( properties );
            break;

        case ALTIUM_SCH_RECORD::NET_LABEL:
            ParseNetLabel( properties );
            break;

        case ALTIUM_SCH_RECORD::BUS:
            ParseBus( properties );
            break;

        case ALTIUM_SCH_RECORD::WIRE:
            ParseWire( properties );
            break;

        case ALTIUM_SCH_RECORD::TEXT_FRAME:
            ParseTextFrame( properties );
            break;

        case ALTIUM_SCH_RECORD::JUNCTION:
            ParseJunction( properties );
            break;

        case ALTIUM_SCH_RECORD::IMAGE:
            ParseImage( properties );
            break;

        case ALTIUM_SCH_RECORD::SHEET:
            ParseSheet( properties );
            break;

        case ALTIUM_SCH_RECORD::SHEET_NAME:
            ParseSheetName( properties );
            break;

        case ALTIUM_SCH_RECORD::FILE_NAME:
            ParseFileName( properties );
            break;

        case ALTIUM_SCH_RECORD::DESIGNATOR:
            ParseDesignator( properties );
            break;

        case ALTIUM_SCH_RECORD::BUS_ENTRY:
            ParseBusEntry( properties );
            break;

        case ALTIUM_SCH_RECORD::TEMPLATE:
            break;

        case ALTIUM_SCH_RECORD::PARAMETER:
            ParseParameter( properties );
            break;

        case ALTIUM_SCH_RECORD::PARAMETER_SET:
            m_reporter->Report( _( "Parameter Set not currently supported." ), RPT_SEVERITY_ERROR );
            break;

        case ALTIUM_SCH_RECORD::IMPLEMENTATION_LIST:
            ParseImplementationList( index, properties );
            break;

        case ALTIUM_SCH_RECORD::IMPLEMENTATION:
            ParseImplementation( properties );
            break;

        case ALTIUM_SCH_RECORD::RECORD_46:
            break;

        case ALTIUM_SCH_RECORD::RECORD_47:
            break;

        case ALTIUM_SCH_RECORD::RECORD_48:
            break;

        case ALTIUM_SCH_RECORD::NOTE:
            ParseNote( properties );
            break;

        case ALTIUM_SCH_RECORD::COMPILE_MASK:
            m_reporter->Report( _( "Compile mask not currently supported." ), RPT_SEVERITY_ERROR );
            break;

        case ALTIUM_SCH_RECORD::RECORD_226:
            break;

        default:
            m_reporter->Report( wxString::Format( _( "Unknown or unexpected record id %d found "
                                                     "inside \"FileHeader\" section." ),
                                                  recordId ),
                                RPT_SEVERITY_ERROR );
            break;
        }

        SCH_ALTIUM_PLUGIN::m_harnessOwnerIndexOffset = index;
    }

    if( reader.HasParsingError() )
        THROW_IO_ERROR( "stream was not parsed correctly!" );

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( "stream is not fully parsed" );

    // assign LIB_SYMBOL -> COMPONENT
    for( std::pair<const int, SCH_SYMBOL*>& symbol : m_symbols )
    {
        auto libSymbolIt = m_libSymbols.find( symbol.first );

        if( libSymbolIt == m_libSymbols.end() )
            THROW_IO_ERROR( "every symbol should have a symbol attached" );

        m_pi->SaveSymbol( getLibFileName().GetFullPath(),
                          new LIB_SYMBOL( *( libSymbolIt->second ) ), m_properties.get() );

        symbol.second->SetLibSymbol( libSymbolIt->second );
    }

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    // Handle title blocks
    screen->SetTitleBlock( *m_currentTitleBlock );
    m_currentTitleBlock.reset();

    // Handle Ports
    for( const ASCH_PORT& port : m_altiumPortsCurrentSheet )
        ParsePort( port );

    m_altiumPortsCurrentSheet.clear();
    m_altiumComponents.clear();
    m_symbols.clear();
    m_libSymbols.clear();

    // Otherwise we cannot save the imported sheet?
    SCH_SHEET* sheet = getCurrentSheet();

    wxCHECK( sheet, /* void */ );

    sheet->SetModified();
}


bool SCH_ALTIUM_PLUGIN::IsComponentPartVisible( int aOwnerindex, int aOwnerpartdisplaymode ) const
{
    const auto& component = m_altiumComponents.find( aOwnerindex );

    if( component == m_altiumComponents.end() )
        return false;

    return component->second.displaymode == aOwnerpartdisplaymode;
}


const ASCH_STORAGE_FILE* SCH_ALTIUM_PLUGIN::GetFileFromStorage( const wxString& aFilename ) const
{
    const ASCH_STORAGE_FILE* nonExactMatch = nullptr;

    for( const ASCH_STORAGE_FILE& file : m_altiumStorage )
    {
        if( file.filename.IsSameAs( aFilename ) )
            return &file;

        if( file.filename.EndsWith( aFilename ) )
            nonExactMatch = &file;
    }

    return nonExactMatch;
}


void SCH_ALTIUM_PLUGIN::ParseComponent( int aIndex,
                                        const std::map<wxString, wxString>& aProperties )
{
    SCH_SHEET* currentSheet = m_sheetPath.Last();
    wxCHECK( currentSheet, /* void */ );

    wxString sheetName = currentSheet->GetName();

    if( sheetName.IsEmpty() )
        sheetName = wxT( "root" );

    ASCH_SYMBOL altiumSymbol( aProperties );

    if( m_altiumComponents.count( aIndex ) )
    {
        const ASCH_SYMBOL& currentSymbol = m_altiumComponents.at( aIndex );

        m_reporter->Report( wxString::Format( _( "Symbol \"%s\" in sheet \"%s\" at index %d "
                                                 "replaced with symbol \"%s\"." ),
                                              currentSymbol.libreference,
                                              sheetName,
                                              aIndex,
                                              altiumSymbol.libreference ),
                            RPT_SEVERITY_ERROR );
    }

    auto pair = m_altiumComponents.insert( { aIndex, altiumSymbol } );
    const ASCH_SYMBOL& elem = pair.first->second;

    // TODO: this is a hack until we correctly apply all transformations to every element
    wxString name = wxString::Format( "%s_%d%s_%s",
                                      sheetName,
                                      elem.orientation,
                                      elem.isMirrored ? "_mirrored" : "",
                                      elem.libreference );
    LIB_ID libId = AltiumToKiCadLibID( getLibName(), name );

    LIB_SYMBOL* ksymbol = new LIB_SYMBOL( wxEmptyString );
    ksymbol->SetName( name );
    ksymbol->SetDescription( elem.componentdescription );
    ksymbol->SetLibId( libId );
    m_libSymbols.insert( { aIndex, ksymbol } );

    // each component has its own symbol for now
    SCH_SYMBOL* symbol = new SCH_SYMBOL();

    symbol->SetPosition( elem.location + m_sheetOffset );

    // TODO: keep it simple for now, and only set position.
    // component->SetOrientation( elem.orientation );
    symbol->SetLibId( libId );
    symbol->SetUnit( std::max( 0, elem.currentpartid ) );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    screen->Append( symbol );

    m_symbols.insert( { aIndex, symbol } );
}


void SCH_ALTIUM_PLUGIN::ParsePin( const std::map<wxString, wxString>& aProperties )
{
    ASCH_PIN elem( aProperties );

    const auto& libSymbolIt = m_libSymbols.find( elem.ownerindex );

    if( libSymbolIt == m_libSymbols.end() )
    {
        // TODO: e.g. can depend on Template (RECORD=39
        m_reporter->Report( wxString::Format( wxT( "Pin's owner (%d) not found." ),
                                              elem.ownerindex ),
                            RPT_SEVERITY_DEBUG );
        return;
    }

    if( !IsComponentPartVisible( elem.ownerindex, elem.ownerpartdisplaymode ) )
        return;

    SCH_SYMBOL* symbol = m_symbols.at( libSymbolIt->first );
    LIB_PIN*    pin = new LIB_PIN( libSymbolIt->second );
    libSymbolIt->second->AddDrawItem( pin );

    pin->SetUnit( std::max( 0, elem.ownerpartid ) );

    pin->SetName( elem.name );
    pin->SetNumber( elem.designator );
    pin->SetLength( elem.pinlength );

    if( !elem.showDesignator )
        pin->SetNumberTextSize( 0 );

    if( !elem.showPinName )
        pin->SetNameTextSize( 0 );

    VECTOR2I pinLocation = elem.location; // the location given is not the connection point!

    switch( elem.orientation )
    {
    case ASCH_RECORD_ORIENTATION::RIGHTWARDS:
        pin->SetOrientation( PIN_ORIENTATION::PIN_LEFT );
        pinLocation.x += elem.pinlength;
        break;

    case ASCH_RECORD_ORIENTATION::UPWARDS:
        pin->SetOrientation( PIN_ORIENTATION::PIN_DOWN );
        pinLocation.y -= elem.pinlength;
        break;

    case ASCH_RECORD_ORIENTATION::LEFTWARDS:
        pin->SetOrientation( PIN_ORIENTATION::PIN_RIGHT );
        pinLocation.x -= elem.pinlength;
        break;

    case ASCH_RECORD_ORIENTATION::DOWNWARDS:
        pin->SetOrientation( PIN_ORIENTATION::PIN_UP );
        pinLocation.y += elem.pinlength;
        break;

    default:
        m_reporter->Report( _( "Pin has unexpected orientation." ), RPT_SEVERITY_WARNING );
        break;
    }

    // TODO: position can be sometimes off a little bit!
    pin->SetPosition( GetRelativePosition( pinLocation + m_sheetOffset, symbol ) );

    // TODO: the following fix is even worse for now?
    // pin->SetPosition( GetRelativePosition( elem.kicadLocation, symbol ) );

    switch( elem.electrical )
    {
    case ASCH_PIN_ELECTRICAL::INPUT:
        pin->SetType( ELECTRICAL_PINTYPE::PT_INPUT );
        break;

    case ASCH_PIN_ELECTRICAL::BIDI:
        pin->SetType( ELECTRICAL_PINTYPE::PT_BIDI );
        break;

    case ASCH_PIN_ELECTRICAL::OUTPUT:
        pin->SetType( ELECTRICAL_PINTYPE::PT_OUTPUT );
        break;

    case ASCH_PIN_ELECTRICAL::OPEN_COLLECTOR:
        pin->SetType( ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR );
        break;

    case ASCH_PIN_ELECTRICAL::PASSIVE:
        pin->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
        break;

    case ASCH_PIN_ELECTRICAL::TRISTATE:
        pin->SetType( ELECTRICAL_PINTYPE::PT_TRISTATE );
        break;

    case ASCH_PIN_ELECTRICAL::OPEN_EMITTER:
        pin->SetType( ELECTRICAL_PINTYPE::PT_OPENEMITTER );
        break;

    case ASCH_PIN_ELECTRICAL::POWER:
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
        break;

    case ASCH_PIN_ELECTRICAL::UNKNOWN:
    default:
        pin->SetType( ELECTRICAL_PINTYPE::PT_UNSPECIFIED );
        m_reporter->Report( _( "Pin has unexpected electrical type." ), RPT_SEVERITY_WARNING );
        break;
    }

    if( elem.symbolOuterEdge == ASCH_PIN_SYMBOL_OUTEREDGE::UNKNOWN )
        m_reporter->Report( _( "Pin has unexpected outer edge type." ), RPT_SEVERITY_WARNING );

    if( elem.symbolInnerEdge == ASCH_PIN_SYMBOL_INNEREDGE::UNKNOWN )
        m_reporter->Report( _( "Pin has unexpected inner edge type." ), RPT_SEVERITY_WARNING );

    if( elem.symbolOuterEdge == ASCH_PIN_SYMBOL_OUTEREDGE::NEGATED )
    {
        switch( elem.symbolInnerEdge )
        {
        case ASCH_PIN_SYMBOL_INNEREDGE::CLOCK:
            pin->SetShape( GRAPHIC_PINSHAPE::INVERTED_CLOCK );
            break;

        default:
            pin->SetShape( GRAPHIC_PINSHAPE::INVERTED );
            break;
        }
    }
    else if( elem.symbolOuterEdge == ASCH_PIN_SYMBOL_OUTEREDGE::LOW_INPUT )
    {
        switch( elem.symbolInnerEdge )
        {
        case ASCH_PIN_SYMBOL_INNEREDGE::CLOCK:
            pin->SetShape( GRAPHIC_PINSHAPE::CLOCK_LOW );
            break;

        default:
            pin->SetShape( GRAPHIC_PINSHAPE::INPUT_LOW );
            break;
        }
    }
    else if( elem.symbolOuterEdge == ASCH_PIN_SYMBOL_OUTEREDGE::LOW_OUTPUT )
    {
        pin->SetShape( GRAPHIC_PINSHAPE::OUTPUT_LOW );
    }
    else
    {
        switch( elem.symbolInnerEdge )
        {
        case ASCH_PIN_SYMBOL_INNEREDGE::CLOCK:
            pin->SetShape( GRAPHIC_PINSHAPE::CLOCK );
            break;

        default:
            pin->SetShape( GRAPHIC_PINSHAPE::LINE ); // nothing to do
            break;
        }
    }
}


void SetTextPositioning( EDA_TEXT* text, ASCH_LABEL_JUSTIFICATION justification,
                         ASCH_RECORD_ORIENTATION orientation )
{
    int       vjustify, hjustify;
    EDA_ANGLE angle = ANGLE_HORIZONTAL;

    switch( justification )
    {
    default:
    case ASCH_LABEL_JUSTIFICATION::UNKNOWN:
    case ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT:
    case ASCH_LABEL_JUSTIFICATION::BOTTOM_CENTER:
    case ASCH_LABEL_JUSTIFICATION::BOTTOM_RIGHT:
        vjustify = GR_TEXT_V_ALIGN_BOTTOM;
        break;

    case ASCH_LABEL_JUSTIFICATION::CENTER_LEFT:
    case ASCH_LABEL_JUSTIFICATION::CENTER_CENTER:
    case ASCH_LABEL_JUSTIFICATION::CENTER_RIGHT:
        vjustify = GR_TEXT_V_ALIGN_CENTER;
        break;

    case ASCH_LABEL_JUSTIFICATION::TOP_LEFT:
    case ASCH_LABEL_JUSTIFICATION::TOP_CENTER:
    case ASCH_LABEL_JUSTIFICATION::TOP_RIGHT:
        vjustify = GR_TEXT_V_ALIGN_TOP;
        break;
    }

    switch( justification )
    {
    default:
    case ASCH_LABEL_JUSTIFICATION::UNKNOWN:
    case ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT:
    case ASCH_LABEL_JUSTIFICATION::CENTER_LEFT:
    case ASCH_LABEL_JUSTIFICATION::TOP_LEFT:
        hjustify = GR_TEXT_H_ALIGN_LEFT;
        break;

    case ASCH_LABEL_JUSTIFICATION::BOTTOM_CENTER:
    case ASCH_LABEL_JUSTIFICATION::CENTER_CENTER:
    case ASCH_LABEL_JUSTIFICATION::TOP_CENTER:
        hjustify = GR_TEXT_H_ALIGN_CENTER;
        break;

    case ASCH_LABEL_JUSTIFICATION::BOTTOM_RIGHT:
    case ASCH_LABEL_JUSTIFICATION::CENTER_RIGHT:
    case ASCH_LABEL_JUSTIFICATION::TOP_RIGHT:
        hjustify = GR_TEXT_H_ALIGN_RIGHT;
        break;
    }

    switch( orientation )
    {
    case ASCH_RECORD_ORIENTATION::RIGHTWARDS:
        angle = ANGLE_HORIZONTAL;
        break;

    case ASCH_RECORD_ORIENTATION::LEFTWARDS:
        vjustify *= -1;
        hjustify *= -1;
        angle = ANGLE_HORIZONTAL;
        break;

    case ASCH_RECORD_ORIENTATION::UPWARDS:
        angle = ANGLE_VERTICAL;
        break;

    case ASCH_RECORD_ORIENTATION::DOWNWARDS:
        vjustify *= -1;
        hjustify *= -1;
        angle = ANGLE_VERTICAL;
        break;
    }

    text->SetVertJustify( static_cast<GR_TEXT_V_ALIGN_T>( vjustify ) );
    text->SetHorizJustify( static_cast<GR_TEXT_H_ALIGN_T>( hjustify ) );
    text->SetTextAngle( angle );
}


void SCH_ALTIUM_PLUGIN::ParseLabel( const std::map<wxString, wxString>& aProperties )
{
    ASCH_LABEL elem( aProperties );

    if( elem.ownerpartid == ALTIUM_COMPONENT_NONE )
    {
        std::map<wxString, wxString> variableMap = {
            { "APPLICATION_BUILDNUMBER", "KICAD_VERSION" },
            { "SHEETNUMBER",  "#"            },
            { "SHEETTOTAL",   "##"           },
            { "TITLE",        "TITLE"        }, // 1:1 maps are sort of useless, but it makes it
            { "REVISION",     "REVISION"     }, // easier to see that the list is complete
            { "DATE",         "ISSUE_DATE"   },
            { "CURRENTDATE",  "CURRENT_DATE" },
            { "COMPANYNAME",  "COMPANY"      },
            { "DOCUMENTNAME", "FILENAME"     },
            { "PROJECTNAME",  "PROJECTNAME"  },
        };

        wxString  kicadText = AltiumSpecialStringsToKiCadVariables( elem.text, variableMap );
        SCH_TEXT* textItem = new SCH_TEXT( elem.location + m_sheetOffset, kicadText );

        SetTextPositioning( textItem, elem.justification, elem.orientation );

        size_t fontId = static_cast<int>( elem.fontId );

        if( m_altiumSheet && fontId > 0 && fontId <= m_altiumSheet->fonts.size() )
        {
            const ASCH_SHEET_FONT& font = m_altiumSheet->fonts.at( fontId - 1 );
            textItem->SetItalic( font.Italic );
            textItem->SetBold( font.Bold );
            textItem->SetTextSize( { font.Size / 2, font.Size / 2 } );
        }

        textItem->SetFlags(IS_NEW );

        SCH_SCREEN* screen = getCurrentScreen();
        wxCHECK( screen, /* void */ );

        screen->Append( textItem );
    }
    else
    {
        const auto& libSymbolIt = m_libSymbols.find( elem.ownerindex );

        if( libSymbolIt == m_libSymbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            m_reporter->Report( wxString::Format( wxT( "Label's owner (%d) not found." ),
                                                  elem.ownerindex ),
                                RPT_SEVERITY_DEBUG );
            return;
        }

        SCH_SYMBOL* symbol = m_symbols.at( libSymbolIt->first );
        LIB_TEXT*   textItem = new LIB_TEXT( libSymbolIt->second );
        libSymbolIt->second->AddDrawItem( textItem );

        textItem->SetUnit( std::max( 0, elem.ownerpartid ) );

        textItem->SetPosition( GetRelativePosition( elem.location + m_sheetOffset, symbol ) );
        textItem->SetText( elem.text );
        SetTextPositioning( textItem, elem.justification, elem.orientation );

        size_t fontId = static_cast<int>( elem.fontId );

        if( m_altiumSheet && fontId > 0 && fontId <= m_altiumSheet->fonts.size() )
        {
            const ASCH_SHEET_FONT& font = m_altiumSheet->fonts.at( fontId - 1 );
            textItem->SetItalic( font.Italic );
            textItem->SetBold( font.Bold );
            textItem->SetTextSize( { font.Size / 2, font.Size / 2 } );
        }
    }
}


void SCH_ALTIUM_PLUGIN::ParseTextFrame( const std::map<wxString, wxString>& aProperties )
{
    ASCH_TEXT_FRAME elem( aProperties );
    AddTextBox( &elem );
}


void SCH_ALTIUM_PLUGIN::ParseNote( const std::map<wxString, wxString>& aProperties )
    {
    ASCH_NOTE elem( aProperties );
    AddTextBox( static_cast<ASCH_TEXT_FRAME*>( &elem ) );

    // TODO: need some sort of property system for storing author....
}


void SCH_ALTIUM_PLUGIN::AddTextBox(const ASCH_TEXT_FRAME *aElem )
    {
    SCH_TEXTBOX* textBox = new SCH_TEXTBOX();

    VECTOR2I sheetTopRight = aElem->TopRight + m_sheetOffset;
    VECTOR2I sheetBottomLeft = aElem->BottomLeft + m_sheetOffset;
    textBox->SetStart( sheetTopRight );
    textBox->SetEnd( sheetBottomLeft );

    textBox->SetText( aElem->Text );

    textBox->SetFillColor( GetColorFromInt( aElem->AreaColor ) );

    if( aElem->IsSolid)
        textBox->SetFillMode( FILL_T::FILLED_WITH_COLOR );
    else
        textBox->SetFilled( false );

    if( aElem->ShowBorder )
        textBox->SetStroke( STROKE_PARAMS( 0, PLOT_DASH_TYPE::DEFAULT,
                                           GetColorFromInt( aElem->BorderColor ) ) );
    else
        textBox->SetStroke( STROKE_PARAMS( -1, PLOT_DASH_TYPE::DEFAULT,
                                           GetColorFromInt( aElem->BorderColor ) ) );

    switch( aElem->Alignment )
    {
    default:
    case ASCH_TEXT_FRAME_ALIGNMENT::LEFT:
        textBox->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;
    case ASCH_TEXT_FRAME_ALIGNMENT::CENTER:
        textBox->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        break;
    case ASCH_TEXT_FRAME_ALIGNMENT::RIGHT:
        textBox->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;
    }

    // JEY TODO: word-wrap once KiCad supports wrapped text.

    size_t fontId = static_cast<int>( aElem->FontID );

    if( m_altiumSheet && fontId > 0 && fontId <= m_altiumSheet->fonts.size() )
    {
        const ASCH_SHEET_FONT& font = m_altiumSheet->fonts.at( fontId - 1 );
        textBox->SetItalic( font.Italic );
        textBox->SetBold( font.Bold );
        textBox->SetTextSize( { font.Size / 2, font.Size / 2 } );
        //textBox->SetFont(  //how to set font, we have a font mane here: ( font.fontname );
    }

    textBox->SetFlags( IS_NEW );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    screen->Append( textBox );
}


void SCH_ALTIUM_PLUGIN::ParseBezier( const std::map<wxString, wxString>& aProperties )
{
    ASCH_BEZIER elem( aProperties );

    if( elem.points.size() < 2 )
    {
        m_reporter->Report( wxString::Format( _( "Bezier has %d control points. At least 2 are "
                                                 "expected." ),
                                              static_cast<int>( elem.points.size() ) ),
                            RPT_SEVERITY_WARNING );
        return;
    }

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    if( elem.ownerpartid == ALTIUM_COMPONENT_NONE )
    {
        for( size_t i = 0; i + 1 < elem.points.size(); i += 3 )
        {
            if( i + 2 == elem.points.size() )
            {
                // special case: single line
                SCH_LINE* line = new SCH_LINE( elem.points.at( i ) + m_sheetOffset,
                                               SCH_LAYER_ID::LAYER_NOTES );

                line->SetEndPoint( elem.points.at( i + 1 ) + m_sheetOffset );
                line->SetStroke( STROKE_PARAMS( elem.lineWidth, PLOT_DASH_TYPE::SOLID ) );

                line->SetFlags( IS_NEW );

                screen->Append( line );
            }
            else
            {
                // simulate Bezier using line segments
                std::vector<VECTOR2I> bezierPoints;
                std::vector<VECTOR2I> polyPoints;

                for( size_t j = i; j < elem.points.size() && j < i + 4; j++ )
                    bezierPoints.push_back( elem.points.at( j ) );

                BEZIER_POLY converter( bezierPoints );
                converter.GetPoly( polyPoints );

                for( size_t k = 0; k + 1 < polyPoints.size(); k++ )
                {
                    SCH_LINE* line = new SCH_LINE( polyPoints.at( k ) + m_sheetOffset,
                                                   SCH_LAYER_ID::LAYER_NOTES );

                    line->SetEndPoint( polyPoints.at( k + 1 ) + m_sheetOffset );
                    line->SetStroke( STROKE_PARAMS( elem.lineWidth, PLOT_DASH_TYPE::SOLID ) );

                    line->SetFlags( IS_NEW );
                    screen->Append( line );
                }
            }
        }
    }
    else
    {
        const auto& libSymbolIt = m_libSymbols.find( elem.ownerindex );

        if( libSymbolIt == m_libSymbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            m_reporter->Report( wxString::Format( wxT( "Bezier's owner (%d) not found." ),
                                                  elem.ownerindex ),
                    RPT_SEVERITY_DEBUG );
            return;
        }

        if( !IsComponentPartVisible( elem.ownerindex, elem.ownerpartdisplaymode ) )
            return;

        SCH_SYMBOL* symbol = m_symbols.at( libSymbolIt->first );

        for( size_t i = 0; i + 1 < elem.points.size(); i += 3 )
        {
            if( i + 2 == elem.points.size() )
            {
                // special case: single line
                LIB_SHAPE* line = new LIB_SHAPE( libSymbolIt->second, SHAPE_T::POLY );
                libSymbolIt->second->AddDrawItem( line );

                line->SetUnit( std::max( 0, elem.ownerpartid ) );

                for( size_t j = i; j < elem.points.size() && j < i + 2; j++ )
                {
                    line->AddPoint( GetRelativePosition( elem.points.at( j ) + m_sheetOffset,
                                                         symbol ) );
                }

                line->SetStroke( STROKE_PARAMS( elem.lineWidth, PLOT_DASH_TYPE::SOLID ) );
            }
            else if( i + 3 == elem.points.size() )
            {
                // TODO: special case of a single line with an extra point?
                // I haven't a clue what this is all about, but the sample document we have in
                // https://gitlab.com/kicad/code/kicad/-/issues/8974 responds best by treating it
                // as another single line special case.
                LIB_SHAPE* line = new LIB_SHAPE( libSymbolIt->second, SHAPE_T::POLY );
                libSymbolIt->second->AddDrawItem( line );

                line->SetUnit( std::max( 0, elem.ownerpartid ) );

                for( size_t j = i; j < elem.points.size() && j < i + 2; j++ )
                {
                    line->AddPoint( GetRelativePosition( elem.points.at( j ) + m_sheetOffset,
                                                         symbol ) );
                }

                line->SetStroke( STROKE_PARAMS( elem.lineWidth, PLOT_DASH_TYPE::SOLID ) );
            }
            else
            {
                // Bezier always has exactly 4 control points
                LIB_SHAPE* bezier = new LIB_SHAPE( libSymbolIt->second, SHAPE_T::BEZIER );
                libSymbolIt->second->AddDrawItem( bezier );

                bezier->SetUnit( std::max( 0, elem.ownerpartid ) );

                for( size_t j = i; j < elem.points.size() && j < i + 4; j++ )
                {
                    VECTOR2I pos =
                            GetRelativePosition( elem.points.at( j ) + m_sheetOffset, symbol );

                    switch( j - i )
                    {
                    case 0: bezier->SetStart( pos );    break;
                    case 1: bezier->SetBezierC1( pos ); break;
                    case 2: bezier->SetBezierC2( pos ); break;
                    case 3: bezier->SetEnd( pos );      break;
                    default: break; // Can't get here but silence warnings
                    }
                }

                bezier->SetStroke( STROKE_PARAMS( elem.lineWidth, PLOT_DASH_TYPE::SOLID ) );
            }
        }
    }
}


void SCH_ALTIUM_PLUGIN::ParsePolyline( const std::map<wxString, wxString>& aProperties )
{
    ASCH_POLYLINE elem( aProperties );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    if( elem.OwnerPartID == ALTIUM_COMPONENT_NONE )
    {
        SCH_SHAPE* poly = new SCH_SHAPE( SHAPE_T::POLY );

        for( VECTOR2I& point : elem.Points )
            poly->AddPoint( point + m_sheetOffset );

        poly->SetStroke( STROKE_PARAMS( elem.LineWidth, GetPlotDashType( elem.LineStyle ),
                                                        GetColorFromInt( elem.Color ) ) );
        poly->SetFlags( IS_NEW );

        screen->Append( poly );
    }
    else
    {
        const auto& libSymbolIt = m_libSymbols.find( elem.OwnerIndex );

        if( libSymbolIt == m_libSymbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            m_reporter->Report( wxString::Format( wxT( "Polyline's owner (%d) not found." ),
                                                  elem.OwnerIndex ),
                                RPT_SEVERITY_DEBUG );
            return;
        }

        if( !IsComponentPartVisible( elem.OwnerIndex, elem.OwnerPartDisplayMode ) )
            return;

        SCH_SYMBOL* symbol = m_symbols.at( libSymbolIt->first );
        LIB_SHAPE*  line = new LIB_SHAPE( libSymbolIt->second, SHAPE_T::POLY );
        libSymbolIt->second->AddDrawItem( line );

        line->SetUnit( elem.OwnerPartID );

        for( VECTOR2I& point : elem.Points )
            line->AddPoint( GetRelativePosition( point + m_sheetOffset, symbol ) );

        line->SetStroke( STROKE_PARAMS( elem.LineWidth, GetPlotDashType( elem.LineStyle ),
                                                        GetColorFromInt( elem.Color ) ) );
    }
}


void SCH_ALTIUM_PLUGIN::ParsePolygon( const std::map<wxString, wxString>& aProperties )
{
    ASCH_POLYGON elem( aProperties );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    if( elem.OwnerPartID == ALTIUM_COMPONENT_NONE )
    {
        SCH_SHAPE* poly = new SCH_SHAPE( SHAPE_T::POLY );

        for( VECTOR2I& point : elem.points )
            poly->AddPoint( point + m_sheetOffset );
        poly->AddPoint( elem.points.front() + m_sheetOffset );

        SetSchShapeFillAndColor( elem, poly );
        poly->SetFlags( IS_NEW );

        screen->Append( poly );
    }
    else
    {
        const auto& libSymbolIt = m_libSymbols.find( elem.OwnerIndex );

        if( libSymbolIt == m_libSymbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            m_reporter->Report( wxString::Format( wxT( "Polygon's owner (%d) not found." ),
                                                  elem.OwnerIndex ),
                    RPT_SEVERITY_DEBUG );
            return;
        }

        if( !IsComponentPartVisible( elem.OwnerIndex, elem.OwnerPartDisplayMode ) )
            return;

        SCH_SYMBOL* symbol = m_symbols.at( libSymbolIt->first );
        LIB_SHAPE*  line = new LIB_SHAPE( libSymbolIt->second, SHAPE_T::POLY );
        libSymbolIt->second->AddDrawItem( line );

        line->SetUnit( elem.OwnerPartID );

        for( VECTOR2I& point : elem.points )
            line->AddPoint( GetRelativePosition( point + m_sheetOffset, symbol ) );

        line->AddPoint( GetRelativePosition( elem.points.front() + m_sheetOffset, symbol ) );
        SetLibShapeFillAndColor( elem, line );
    }
}


void SCH_ALTIUM_PLUGIN::ParseRoundRectangle( const std::map<wxString, wxString>& aProperties )
{
    ASCH_ROUND_RECTANGLE elem( aProperties );

    VECTOR2I sheetTopRight = elem.TopRight + m_sheetOffset;
    VECTOR2I sheetBottomLeft = elem.BottomLeft + m_sheetOffset;

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    if( elem.OwnerPartID == ALTIUM_COMPONENT_NONE )
    {
        // TODO: misses rounded edges
        SCH_SHAPE* rect = new SCH_SHAPE( SHAPE_T::RECTANGLE );

        rect->SetPosition( sheetTopRight );
        rect->SetEnd( sheetBottomLeft );
        SetSchShapeFillAndColor( elem, rect );
        rect->SetFlags( IS_NEW );

        screen->Append( rect );
    }
    else
    {
        const auto& libSymbolIt = m_libSymbols.find( elem.OwnerIndex );

        if( libSymbolIt == m_libSymbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            m_reporter->Report( wxString::Format( wxT( "Rounded rectangle's owner (%d) not "
                                                       "found." ),
                                                  elem.OwnerIndex ),
                                RPT_SEVERITY_DEBUG );
            return;
        }

        if( !IsComponentPartVisible( elem.OwnerIndex, elem.OwnerPartDisplayMode ) )
            return;

        SCH_SYMBOL* symbol = m_symbols.at( libSymbolIt->first );
        // TODO: misses rounded edges
        LIB_SHAPE*  rect = new LIB_SHAPE( libSymbolIt->second, SHAPE_T::RECTANGLE );
        libSymbolIt->second->AddDrawItem( rect );

        rect->SetUnit( elem.OwnerPartID );

        rect->SetPosition( GetRelativePosition( elem.TopRight + m_sheetOffset, symbol ) );
        rect->SetEnd( GetRelativePosition( elem.BottomLeft + m_sheetOffset, symbol ) );
        SetLibShapeFillAndColor( elem, rect );
    }
}


void SCH_ALTIUM_PLUGIN::ParseArc( const std::map<wxString, wxString>& aProperties )
{
    // The Arc can be ALTIUM_SCH_RECORD::ELLIPTICAL_ARC or ALTIUM_SCH_RECORD::ARC
    // Elliptical arcs are not handled in KiCad. So use an arc instead
    // TODO: handle elliptical arc better.

    ASCH_ARC elem( aProperties );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    int arc_radius = elem.m_Radius;

    // Try to approximate this ellipse by an arc. use the biggest of radius and secondary radius
    // One can of course use another recipe
    if( elem.m_IsElliptical )
        arc_radius = std::max( elem.m_Radius, elem.m_SecondaryRadius );

    if( elem.ownerpartid == ALTIUM_COMPONENT_NONE )
    {
        if( elem.m_StartAngle == 0 && ( elem.m_EndAngle == 0 || elem.m_EndAngle == 360 ) )
        {
            SCH_SHAPE* circle = new SCH_SHAPE( SHAPE_T::CIRCLE );

            circle->SetPosition( elem.m_Center + m_sheetOffset );
            circle->SetEnd( circle->GetPosition() + VECTOR2I( arc_radius, 0 ) );
            circle->SetStroke( STROKE_PARAMS( elem.m_LineWidth, PLOT_DASH_TYPE::SOLID ) );

            screen->Append( circle );
        }
        else
        {
            SCH_SHAPE* arc = new SCH_SHAPE( SHAPE_T::ARC );
            EDA_ANGLE  includedAngle( elem.m_EndAngle - elem.m_StartAngle, DEGREES_T );
            EDA_ANGLE  startAngle( elem.m_EndAngle, DEGREES_T );
            VECTOR2I   startOffset( KiROUND( arc_radius * startAngle.Cos() ),
                                   -KiROUND( arc_radius * startAngle.Sin() ) );

            arc->SetCenter( elem.m_Center + m_sheetOffset );
            arc->SetStart( elem.m_Center + startOffset + m_sheetOffset );
            arc->SetArcAngleAndEnd( includedAngle.Normalize(), true );

            arc->SetStroke( STROKE_PARAMS( elem.m_LineWidth, PLOT_DASH_TYPE::SOLID ) );

            screen->Append( arc );
        }
    }
    else
    {
        const auto& libSymbolIt = m_libSymbols.find( elem.ownerindex );

        if( libSymbolIt == m_libSymbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            m_reporter->Report( wxString::Format( wxT( "Arc's owner (%d) not found." ),
                                                  elem.ownerindex ),
                    RPT_SEVERITY_DEBUG );
            return;
        }

        if( !IsComponentPartVisible( elem.ownerindex, elem.ownerpartdisplaymode ) )
            return;

        SCH_SYMBOL* symbol = m_symbols.at( libSymbolIt->first );

        if( elem.m_StartAngle == 0 && ( elem.m_EndAngle == 0 || elem.m_EndAngle == 360 ) )
        {
            LIB_SHAPE* circle = new LIB_SHAPE( libSymbolIt->second, SHAPE_T::CIRCLE );
            libSymbolIt->second->AddDrawItem( circle );

            circle->SetUnit( std::max( 0, elem.ownerpartid ) );

            circle->SetPosition( GetRelativePosition( elem.m_Center + m_sheetOffset, symbol ) );
            circle->SetEnd( circle->GetPosition() + VECTOR2I( arc_radius, 0 ) );
            circle->SetStroke( STROKE_PARAMS( elem.m_LineWidth, PLOT_DASH_TYPE::SOLID ) );
        }
        else
        {
            LIB_SHAPE* arc = new LIB_SHAPE( libSymbolIt->second, SHAPE_T::ARC );
            libSymbolIt->second->AddDrawItem( arc );
            arc->SetUnit( std::max( 0, elem.ownerpartid ) );

            arc->SetCenter( GetRelativePosition( elem.m_Center + m_sheetOffset, symbol ) );

            VECTOR2I arcStart( arc_radius, 0 );
            RotatePoint( arcStart, -EDA_ANGLE( elem.m_StartAngle, DEGREES_T ) );
            arcStart += arc->GetCenter();
            arc->SetStart( arcStart );

            VECTOR2I arcEnd( arc_radius, 0 );
            RotatePoint( arcEnd, -EDA_ANGLE( elem.m_EndAngle, DEGREES_T ) );
            arcEnd += arc->GetCenter();
            arc->SetEnd( arcEnd );

            arc->SetStroke( STROKE_PARAMS( elem.m_LineWidth, PLOT_DASH_TYPE::SOLID ) );
        }
    }
}


void SCH_ALTIUM_PLUGIN::ParseEllipse( const std::map<wxString, wxString>& aProperties )
{
    ASCH_ELLIPSE elem( aProperties );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    // To do: Import true ellipses when KiCad supports them
    if( elem.Radius != elem.SecondaryRadius )
    {
        m_reporter->Report( wxString::Format( _( "Unsupported ellipse was not imported at "
                                                 "(X = %d; Y = %d)." ),
                                              ( elem.Center + m_sheetOffset ).x,
                                              ( elem.Center + m_sheetOffset ).y ),
                            RPT_SEVERITY_ERROR );
        return;
    }

    if( elem.OwnerPartID == ALTIUM_COMPONENT_NONE )
    {
        SCH_SHAPE* circle = new SCH_SHAPE( SHAPE_T::CIRCLE );

        circle->SetPosition( elem.Center + m_sheetOffset );
        circle->SetEnd( circle->GetPosition() + VECTOR2I( elem.Radius, 0 ) );
        circle->SetStroke( STROKE_PARAMS( 1, PLOT_DASH_TYPE::SOLID ) );

        circle->SetFillColor( GetColorFromInt( elem.AreaColor ) );

        if( elem.IsSolid )
            circle->SetFillMode( FILL_T::FILLED_WITH_COLOR );
        else
            circle->SetFilled( false );

        screen->Append( circle );
    }
    else
    {
        const auto& libSymbolIt = m_libSymbols.find( elem.OwnerIndex );

        if( libSymbolIt == m_libSymbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            m_reporter->Report( wxString::Format( wxT( "Ellipse's owner (%d) not found." ),
                                                  elem.OwnerIndex ),
                                RPT_SEVERITY_DEBUG );
            return;
        }

        SCH_SYMBOL* symbol = m_symbols.at( libSymbolIt->first );

        LIB_SHAPE* circle = new LIB_SHAPE( libSymbolIt->second, SHAPE_T::CIRCLE );
        libSymbolIt->second->AddDrawItem( circle );

        circle->SetUnit( elem.OwnerPartID );

        circle->SetPosition( GetRelativePosition( elem.Center + m_sheetOffset, symbol ) );
        circle->SetEnd( circle->GetPosition() + VECTOR2I( elem.Radius, 0 ) );
        circle->SetStroke( STROKE_PARAMS( 1, PLOT_DASH_TYPE::SOLID ) );

        circle->SetFillColor( GetColorFromInt( elem.AreaColor ) );

        if( elem.IsSolid )
            circle->SetFillMode( FILL_T::FILLED_WITH_COLOR );
        else
            circle->SetFilled( false );
    }
}


void SCH_ALTIUM_PLUGIN::ParseLine( const std::map<wxString, wxString>& aProperties )
{
    ASCH_LINE elem( aProperties );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    if( elem.ownerpartid == ALTIUM_COMPONENT_NONE )
    {
        // close polygon
        SCH_LINE* line = new SCH_LINE( elem.point1 + m_sheetOffset, SCH_LAYER_ID::LAYER_NOTES );
        line->SetEndPoint( elem.point2 + m_sheetOffset );
        line->SetStroke( STROKE_PARAMS( elem.lineWidth, PLOT_DASH_TYPE::SOLID ) ); // TODO?

        line->SetFlags( IS_NEW );
        screen->Append( line );
    }
    else
    {
        const auto& libSymbolIt = m_libSymbols.find( elem.ownerindex );

        if( libSymbolIt == m_libSymbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            m_reporter->Report( wxString::Format( wxT( "Line's owner (%d) not found." ),
                                                  elem.ownerindex ),
                                RPT_SEVERITY_DEBUG );
            return;
        }

        if( !IsComponentPartVisible( elem.ownerindex, elem.ownerpartdisplaymode ) )
            return;

        SCH_SYMBOL* symbol = m_symbols.at( libSymbolIt->first );
        LIB_SHAPE*  line = new LIB_SHAPE( libSymbolIt->second, SHAPE_T::POLY );
        libSymbolIt->second->AddDrawItem( line );

        line->SetUnit( std::max( 0, elem.ownerpartid ) );

        line->AddPoint( GetRelativePosition( elem.point1 + m_sheetOffset, symbol ) );
        line->AddPoint( GetRelativePosition( elem.point2 + m_sheetOffset, symbol ) );

        line->SetStroke( STROKE_PARAMS( elem.lineWidth, PLOT_DASH_TYPE::SOLID ) );
    }
}


void SCH_ALTIUM_PLUGIN::ParseSignalHarness( const std::map<wxString, wxString>& aProperties )
{
    ASCH_SIGNAL_HARNESS elem( aProperties );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    if( elem.OwnerPartID == ALTIUM_COMPONENT_NONE )
    {
        SCH_SHAPE* poly = new SCH_SHAPE( SHAPE_T::POLY );

        for( VECTOR2I& point : elem.Points )
            poly->AddPoint( point + m_sheetOffset );

        poly->SetStroke( STROKE_PARAMS( elem.LineWidth, PLOT_DASH_TYPE::SOLID,
                                        GetColorFromInt( elem.Color ) ) );
        poly->SetFlags( IS_NEW );

        screen->Append( poly );
    }
    else
    {
        // No clue if this situation can ever exist
        m_reporter->Report( wxT( "Signal harness, belonging to the part is not currently "
                                 "supported." ), RPT_SEVERITY_DEBUG );
    }
}


void SCH_ALTIUM_PLUGIN::ParseHarnessConnector( int aIndex, const std::map<wxString,
                                               wxString>& aProperties )
{
    ASCH_HARNESS_CONNECTOR elem( aProperties );

    SCH_SCREEN* currentScreen = getCurrentScreen();
    wxCHECK( currentScreen, /* void */ );

    if( elem.OwnerPartID == ALTIUM_COMPONENT_NONE )
    {
        SCH_SHEET* sheet = new SCH_SHEET( getCurrentSheet(), elem.Location + m_sheetOffset,
                                          elem.Size );

        sheet->SetScreen( new SCH_SCREEN( m_schematic ) );
        sheet->SetBackgroundColor( GetColorFromInt( elem.AreaColor ) );
        sheet->SetBorderColor( GetColorFromInt( elem.Color ) );

        currentScreen->Append( sheet );

        SCH_SHEET_PATH sheetpath = m_sheetPath;
        sheetpath.push_back( sheet );

        sheetpath.SetPageNumber( "Harness #" );

        m_harnessEntryParent = aIndex + m_harnessOwnerIndexOffset;
        m_sheets.insert( { m_harnessEntryParent, sheet } );
    }
    else
    {
        // I have no clue if this situation can ever exist
        m_reporter->Report( wxT( "Harness connector, belonging to the part is not currently "
                                 "supported." ),
                            RPT_SEVERITY_DEBUG );
    }
}


void SCH_ALTIUM_PLUGIN::ParseHarnessEntry( const std::map<wxString, wxString>& aProperties )
{
    ASCH_HARNESS_ENTRY elem( aProperties );

    const auto& sheetIt = m_sheets.find( m_harnessEntryParent );

    if( sheetIt == m_sheets.end() )
    {
        m_reporter->Report( wxString::Format( wxT( "Harness entry's parent (%d) not found." ),
                                              SCH_ALTIUM_PLUGIN::m_harnessEntryParent ),
                            RPT_SEVERITY_DEBUG );
        return;
    }

    SCH_SHEET_PIN* sheetPin = new SCH_SHEET_PIN( sheetIt->second );
    sheetIt->second->AddPin( sheetPin );

    sheetPin->SetText( elem.Name );
    sheetPin->SetShape( LABEL_FLAG_SHAPE::L_UNSPECIFIED );

    VECTOR2I pos = sheetIt->second->GetPosition();
    VECTOR2I size = sheetIt->second->GetSize();

    switch( elem.Side )
    {
    default:
    case ASCH_SHEET_ENTRY_SIDE::LEFT:
        sheetPin->SetPosition( { pos.x, pos.y + elem.DistanceFromTop } );
        sheetPin->SetTextSpinStyle( TEXT_SPIN_STYLE::LEFT );
        sheetPin->SetSide( SHEET_SIDE::LEFT );
        break;
    case ASCH_SHEET_ENTRY_SIDE::RIGHT:
        sheetPin->SetPosition( { pos.x + size.x, pos.y + elem.DistanceFromTop } );
        sheetPin->SetTextSpinStyle( TEXT_SPIN_STYLE::RIGHT );
        sheetPin->SetSide( SHEET_SIDE::RIGHT );
        break;
    case ASCH_SHEET_ENTRY_SIDE::TOP:
        sheetPin->SetPosition( { pos.x + elem.DistanceFromTop, pos.y } );
        sheetPin->SetTextSpinStyle( TEXT_SPIN_STYLE::UP );
        sheetPin->SetSide( SHEET_SIDE::TOP );
        break;
    case ASCH_SHEET_ENTRY_SIDE::BOTTOM:
        sheetPin->SetPosition( { pos.x + elem.DistanceFromTop, pos.y + size.y } );
        sheetPin->SetTextSpinStyle( TEXT_SPIN_STYLE::BOTTOM );
        sheetPin->SetSide( SHEET_SIDE::BOTTOM );
        break;
    }
}


void SCH_ALTIUM_PLUGIN::ParseHarnessType( const std::map<wxString, wxString>& aProperties )
{
    ASCH_HARNESS_TYPE elem( aProperties );

    const auto& sheetIt = m_sheets.find( m_harnessEntryParent );

    if( sheetIt == m_sheets.end() )
    {
        m_reporter->Report( wxString::Format( wxT( "Harness type's parent (%d) not found." ),
                                              m_harnessEntryParent ),
                            RPT_SEVERITY_DEBUG );
        return;
    }

    SCH_FIELD& sheetNameField = sheetIt->second->GetFields()[SHEETNAME];

    sheetNameField.SetPosition( elem.Location + m_sheetOffset );
    sheetNameField.SetText( elem.Text );

    // Always set as visible so user is aware about ( !elem.isHidden );
    sheetNameField.SetVisible( true );
    SetTextPositioning( &sheetNameField, ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT,
                        ASCH_RECORD_ORIENTATION::RIGHTWARDS );
    sheetNameField.SetTextColor( GetColorFromInt( elem.Color ) );

    SCH_FIELD& sheetFileName = sheetIt->second->GetFields()[SHEETFILENAME];
    sheetFileName.SetText( elem.Text + wxT( "." ) + KiCadSchematicFileExtension );

    wxFileName fn( m_schematic->Prj().GetProjectPath(), elem.Text, KiCadSchematicFileExtension );
    wxString fullPath = fn.GetFullPath();

    fullPath.Replace( wxT( "\\" ), wxT( "/" ) );

    SCH_SCREEN* screen = sheetIt->second->GetScreen();

    wxCHECK( screen, /* void */ );
    screen->SetFileName( fullPath );

    m_reporter->Report( wxString::Format( _( "Altium's harness connector (%s) was imported as a "
                                             "hierarchical sheet. Please review the imported "
                                             "schematic." ),
                                          elem.Text ),
                        RPT_SEVERITY_WARNING );
}


void SCH_ALTIUM_PLUGIN::ParseRectangle( const std::map<wxString, wxString>& aProperties )
{
    ASCH_RECTANGLE elem( aProperties );

    VECTOR2I sheetTopRight = elem.TopRight + m_sheetOffset;
    VECTOR2I sheetBottomLeft = elem.BottomLeft + m_sheetOffset;

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    if( elem.OwnerPartID == ALTIUM_COMPONENT_NONE )
    {
        SCH_SHAPE* rect = new SCH_SHAPE( SHAPE_T::RECTANGLE );

        rect->SetPosition( sheetTopRight );
        rect->SetEnd( sheetBottomLeft );
        SetSchShapeFillAndColor( elem, rect );
        rect->SetFlags( IS_NEW );

        screen->Append( rect );
    }
    else
    {
        const auto& libSymbolIt = m_libSymbols.find( elem.OwnerIndex );

        if( libSymbolIt == m_libSymbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            m_reporter->Report( wxString::Format( wxT( "Rectangle's owner (%d) not found." ),
                                                  elem.OwnerIndex ),
                                RPT_SEVERITY_DEBUG );
            return;
        }

        if( !IsComponentPartVisible( elem.OwnerIndex, elem.OwnerPartDisplayMode ) )
            return;

        SCH_SYMBOL* symbol = m_symbols.at( libSymbolIt->first );
        LIB_SHAPE*  rect = new LIB_SHAPE( libSymbolIt->second, SHAPE_T::RECTANGLE );
        libSymbolIt->second->AddDrawItem( rect );

        rect->SetUnit( elem.OwnerPartID );

        rect->SetPosition( GetRelativePosition( sheetTopRight, symbol ) );
        rect->SetEnd( GetRelativePosition( sheetBottomLeft, symbol ) );
        SetLibShapeFillAndColor( elem, rect );
    }
}


void SCH_ALTIUM_PLUGIN::ParseSheetSymbol( int aIndex,
                                          const std::map<wxString, wxString>& aProperties )
{
    ASCH_SHEET_SYMBOL elem( aProperties );

    SCH_SHEET*  sheet  = new SCH_SHEET( getCurrentSheet(), elem.location + m_sheetOffset,
                                        elem.size );

    sheet->SetBorderColor( GetColorFromInt( elem.color ) );

    if( elem.isSolid )
        sheet->SetBackgroundColor( GetColorFromInt( elem.areacolor ) );

    sheet->SetFlags( IS_NEW );

    SCH_SCREEN* currentScreen = getCurrentScreen();
    wxCHECK( currentScreen, /* void */ );
    currentScreen->Append( sheet );

    SCH_SHEET_PATH sheetpath = m_sheetPath;
    sheetpath.push_back( sheet );

    // We'll update later if we find a pageNumber record for it.
    sheetpath.SetPageNumber( "#" );

    SCH_SCREEN* rootScreen = m_rootSheet->GetScreen();
    wxCHECK( rootScreen, /* void */ );

    SCH_SHEET_INSTANCE sheetInstance;

    sheetInstance.m_Path = sheetpath.Path();
    sheetInstance.m_PageNumber = wxT( "#" );

    rootScreen->m_sheetInstances.emplace_back( sheetInstance );
    m_sheets.insert( { aIndex, sheet } );
}


void SCH_ALTIUM_PLUGIN::ParseSheetEntry( const std::map<wxString, wxString>& aProperties )
{
    ASCH_SHEET_ENTRY elem( aProperties );

    const auto& sheetIt = m_sheets.find( elem.ownerindex );

    if( sheetIt == m_sheets.end() )
    {
        m_reporter->Report( wxString::Format( wxT( "Sheet entry's owner (%d) not found." ),
                                              elem.ownerindex ),
                            RPT_SEVERITY_DEBUG );
        return;
    }

    SCH_SHEET_PIN* sheetPin = new SCH_SHEET_PIN( sheetIt->second );
    sheetIt->second->AddPin( sheetPin );

    sheetPin->SetText( elem.name );
    sheetPin->SetShape( LABEL_FLAG_SHAPE::L_UNSPECIFIED );
    //sheetPin->SetTextSpinStyle( getSpinStyle( term.OrientAngle, false ) );
    //sheetPin->SetPosition( getKiCadPoint( term.Position ) );

    VECTOR2I pos = sheetIt->second->GetPosition();
    VECTOR2I size = sheetIt->second->GetSize();

    switch( elem.side )
    {
    default:
    case ASCH_SHEET_ENTRY_SIDE::LEFT:
        sheetPin->SetPosition( { pos.x, pos.y + elem.distanceFromTop } );
        sheetPin->SetTextSpinStyle( TEXT_SPIN_STYLE::LEFT );
        sheetPin->SetSide( SHEET_SIDE::LEFT );
        break;

    case ASCH_SHEET_ENTRY_SIDE::RIGHT:
        sheetPin->SetPosition( { pos.x + size.x, pos.y + elem.distanceFromTop } );
        sheetPin->SetTextSpinStyle( TEXT_SPIN_STYLE::RIGHT );
        sheetPin->SetSide( SHEET_SIDE::RIGHT );
        break;

    case ASCH_SHEET_ENTRY_SIDE::TOP:
        sheetPin->SetPosition( { pos.x + elem.distanceFromTop, pos.y } );
        sheetPin->SetTextSpinStyle( TEXT_SPIN_STYLE::UP );
        sheetPin->SetSide( SHEET_SIDE::TOP );
        break;

    case ASCH_SHEET_ENTRY_SIDE::BOTTOM:
        sheetPin->SetPosition( { pos.x + elem.distanceFromTop, pos.y + size.y } );
        sheetPin->SetTextSpinStyle( TEXT_SPIN_STYLE::BOTTOM );
        sheetPin->SetSide( SHEET_SIDE::BOTTOM );
        break;
    }

    switch( elem.iotype )
    {
    default:
    case ASCH_PORT_IOTYPE::UNSPECIFIED:
        sheetPin->SetShape( LABEL_FLAG_SHAPE::L_UNSPECIFIED );
        break;

    case ASCH_PORT_IOTYPE::OUTPUT:
        sheetPin->SetShape( LABEL_FLAG_SHAPE::L_OUTPUT );
        break;

    case ASCH_PORT_IOTYPE::INPUT:
        sheetPin->SetShape( LABEL_FLAG_SHAPE::L_INPUT );
        break;

    case ASCH_PORT_IOTYPE::BIDI:
        sheetPin->SetShape( LABEL_FLAG_SHAPE::L_BIDI );
        break;
    }
}


VECTOR2I HelperGeneratePowerPortGraphics( LIB_SYMBOL* aKsymbol, ASCH_POWER_PORT_STYLE aStyle,
                                          REPORTER* aReporter )
{
    if( aStyle == ASCH_POWER_PORT_STYLE::CIRCLE || aStyle == ASCH_POWER_PORT_STYLE::ARROW )
    {
        LIB_SHAPE* line1 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
        aKsymbol->AddDrawItem( line1 );
        line1->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, schIUScale.MilsToIU( -50 ) } );

        if( aStyle == ASCH_POWER_PORT_STYLE::CIRCLE )
        {
            LIB_SHAPE* circle = new LIB_SHAPE( aKsymbol, SHAPE_T::CIRCLE );
            aKsymbol->AddDrawItem( circle );
            circle->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 5 ), PLOT_DASH_TYPE::SOLID ) );
            circle->SetPosition( { schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( -75 ) } );
            circle->SetEnd( circle->GetPosition() + VECTOR2I( schIUScale.MilsToIU( 25 ), 0 ) );
        }
        else
        {
            LIB_SHAPE* line2 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
            aKsymbol->AddDrawItem( line2 );
            line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
            line2->AddPoint( { schIUScale.MilsToIU( -25 ), schIUScale.MilsToIU( -50 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 25 ), schIUScale.MilsToIU( -50 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( -100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( -25 ), schIUScale.MilsToIU( -50 ) } );
        }

        return { 0, schIUScale.MilsToIU( 150 ) };
    }
    else if( aStyle == ASCH_POWER_PORT_STYLE::WAVE )
    {
        LIB_SHAPE* line = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
        aKsymbol->AddDrawItem( line );
        line->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
        line->AddPoint( { 0, 0 } );
        line->AddPoint( { 0, schIUScale.MilsToIU( -72 ) } );

        LIB_SHAPE* bezier = new LIB_SHAPE( aKsymbol, SHAPE_T::BEZIER );
        aKsymbol->AddDrawItem( bezier );
        bezier->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 5 ), PLOT_DASH_TYPE::SOLID ) );
        bezier->AddPoint( { schIUScale.MilsToIU( 30 ), schIUScale.MilsToIU( -50 ) } );
        bezier->AddPoint( { schIUScale.MilsToIU( 30 ), schIUScale.MilsToIU( -87 ) } );
        bezier->AddPoint( { schIUScale.MilsToIU( -30 ), schIUScale.MilsToIU( -63 ) } );
        bezier->AddPoint( { schIUScale.MilsToIU( -30 ), schIUScale.MilsToIU( -100 ) } );

        return { 0, schIUScale.MilsToIU( 150 ) };
    }
    else if( aStyle == ASCH_POWER_PORT_STYLE::POWER_GROUND
             || aStyle == ASCH_POWER_PORT_STYLE::SIGNAL_GROUND
             || aStyle == ASCH_POWER_PORT_STYLE::EARTH
             || aStyle == ASCH_POWER_PORT_STYLE::GOST_ARROW )
    {
        LIB_SHAPE* line1 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
        aKsymbol->AddDrawItem( line1 );
        line1->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, schIUScale.MilsToIU( -100 ) } );

        if( aStyle == ASCH_POWER_PORT_STYLE::POWER_GROUND )
        {
            LIB_SHAPE* line2 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
            aKsymbol->AddDrawItem( line2 );
            line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
            line2->AddPoint( { schIUScale.MilsToIU( -100 ), schIUScale.MilsToIU( -100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 100 ), schIUScale.MilsToIU( -100 ) } );

            LIB_SHAPE* line3 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
            aKsymbol->AddDrawItem( line3 );
            line3->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
            line3->AddPoint( { schIUScale.MilsToIU( -70 ), schIUScale.MilsToIU( -130 ) } );
            line3->AddPoint( { schIUScale.MilsToIU( 70 ), schIUScale.MilsToIU( -130 ) } );

            LIB_SHAPE* line4 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
            aKsymbol->AddDrawItem( line4 );
            line4->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
            line4->AddPoint( { schIUScale.MilsToIU( -40 ), schIUScale.MilsToIU( -160 ) } );
            line4->AddPoint( { schIUScale.MilsToIU( 40 ), schIUScale.MilsToIU( -160 ) } );

            LIB_SHAPE* line5 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
            aKsymbol->AddDrawItem( line5 );
            line5->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
            line5->AddPoint( { schIUScale.MilsToIU( -10 ), schIUScale.MilsToIU( -190 ) } );
            line5->AddPoint( { schIUScale.MilsToIU( 10 ), schIUScale.MilsToIU( -190 ) } );
        }
        else if( aStyle == ASCH_POWER_PORT_STYLE::SIGNAL_GROUND )
        {
            LIB_SHAPE* line2 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
            aKsymbol->AddDrawItem( line2 );
            line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
            line2->AddPoint( { schIUScale.MilsToIU( -100 ), schIUScale.MilsToIU( -100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 100 ), schIUScale.MilsToIU( -100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( -200 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( -100 ), schIUScale.MilsToIU( -100 ) } );
        }
        else if( aStyle == ASCH_POWER_PORT_STYLE::EARTH )
        {
            LIB_SHAPE* line2 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
            aKsymbol->AddDrawItem( line2 );
            line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
            line2->AddPoint( { schIUScale.MilsToIU( -150 ), schIUScale.MilsToIU( -200 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( -100 ), schIUScale.MilsToIU( -100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 100 ), schIUScale.MilsToIU( -100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 50 ), schIUScale.MilsToIU( -200 ) } );

            LIB_SHAPE* line3 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
            aKsymbol->AddDrawItem( line3 );
            line3->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
            line3->AddPoint( { schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( -100 ) } );
            line3->AddPoint( { schIUScale.MilsToIU( -50 ), schIUScale.MilsToIU( -200 ) } );
        }
        else // ASCH_POWER_PORT_STYLE::GOST_ARROW
        {
            LIB_SHAPE* line2 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
            aKsymbol->AddDrawItem( line2 );
            line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
            line2->AddPoint( { schIUScale.MilsToIU( -25 ), schIUScale.MilsToIU( -50 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( -100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 25 ), schIUScale.MilsToIU( -50 ) } );

            return { 0, schIUScale.MilsToIU( 150 ) }; // special case
        }

        return { 0, schIUScale.MilsToIU( 250 ) };
    }
    else if( aStyle == ASCH_POWER_PORT_STYLE::GOST_POWER_GROUND
             || aStyle == ASCH_POWER_PORT_STYLE::GOST_EARTH )
    {
        LIB_SHAPE* line1 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
        aKsymbol->AddDrawItem( line1 );
        line1->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, schIUScale.MilsToIU( -160 ) } );

        LIB_SHAPE* line2 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
        aKsymbol->AddDrawItem( line2 );
        line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
        line2->AddPoint( { schIUScale.MilsToIU( -100 ), schIUScale.MilsToIU( -160 ) } );
        line2->AddPoint( { schIUScale.MilsToIU( 100 ), schIUScale.MilsToIU( -160 ) } );

        LIB_SHAPE* line3 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
        aKsymbol->AddDrawItem( line3 );
        line3->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
        line3->AddPoint( { schIUScale.MilsToIU( -60 ), schIUScale.MilsToIU( -200 ) } );
        line3->AddPoint( { schIUScale.MilsToIU( 60 ), schIUScale.MilsToIU( -200 ) } );

        LIB_SHAPE* line4 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
        aKsymbol->AddDrawItem( line4 );
        line4->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
        line4->AddPoint( { schIUScale.MilsToIU( -20 ), schIUScale.MilsToIU( -240 ) } );
        line4->AddPoint( { schIUScale.MilsToIU( 20 ), schIUScale.MilsToIU( -240 ) } );

        if( aStyle == ASCH_POWER_PORT_STYLE::GOST_POWER_GROUND )
            return { 0, schIUScale.MilsToIU( 300 ) };

        LIB_SHAPE* circle = new LIB_SHAPE( aKsymbol, SHAPE_T::CIRCLE );
        aKsymbol->AddDrawItem( circle );
        circle->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
        circle->SetPosition( { schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( -160 ) } );
        circle->SetEnd( circle->GetPosition() + VECTOR2I( schIUScale.MilsToIU( 120 ), 0 ) );

        return { 0, schIUScale.MilsToIU( 350 ) };
    }
    else if( aStyle == ASCH_POWER_PORT_STYLE::GOST_BAR )
    {
        LIB_SHAPE* line1 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
        aKsymbol->AddDrawItem( line1 );
        line1->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, schIUScale.MilsToIU( -200 ) } );

        LIB_SHAPE* line2 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
        aKsymbol->AddDrawItem( line2 );
        line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
        line2->AddPoint( { schIUScale.MilsToIU( -100 ), schIUScale.MilsToIU( -200 ) } );
        line2->AddPoint( { schIUScale.MilsToIU( 100 ), schIUScale.MilsToIU( -200 ) } );

        return { 0, schIUScale.MilsToIU( 250 ) };
    }
    else
    {
        if( aStyle != ASCH_POWER_PORT_STYLE::BAR )
        {
            aReporter->Report( _( "Power Port with unknown style imported as 'Bar' type." ),
                               RPT_SEVERITY_WARNING );
        }

        LIB_SHAPE* line1 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
        aKsymbol->AddDrawItem( line1 );
        line1->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, schIUScale.MilsToIU( -100 ) } );

        LIB_SHAPE* line2 = new LIB_SHAPE( aKsymbol, SHAPE_T::POLY );
        aKsymbol->AddDrawItem( line2 );
        line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), PLOT_DASH_TYPE::SOLID ) );
        line2->AddPoint( { schIUScale.MilsToIU( -50 ), schIUScale.MilsToIU( -100 ) } );
        line2->AddPoint( { schIUScale.MilsToIU( 50 ), schIUScale.MilsToIU( -100 ) } );

        return { 0, schIUScale.MilsToIU( 150 ) };
    }
}


void SCH_ALTIUM_PLUGIN::ParsePowerPort( const std::map<wxString, wxString>& aProperties )
{
    ASCH_POWER_PORT elem( aProperties );
    LIB_ID          libId = AltiumToKiCadLibID( getLibName(), elem.text );
    LIB_SYMBOL*     libSymbol = nullptr;

    const auto& powerSymbolIt = m_powerSymbols.find( elem.text );

    if( powerSymbolIt != m_powerSymbols.end() )
    {
        libSymbol = powerSymbolIt->second; // cache hit
    }
    else
    {
        libSymbol = new LIB_SYMBOL( wxEmptyString );
        libSymbol->SetPower();
        libSymbol->SetName( elem.text );
        libSymbol->GetReferenceField().SetText( "#PWR" );
        libSymbol->GetValueField().SetText( elem.text );
        libSymbol->GetValueField().SetVisible( true );
        libSymbol->SetDescription( wxString::Format( _( "Power symbol creates a global "
                                                        "label with name '%s'" ), elem.text ) );
        libSymbol->SetKeyWords( "power-flag" );
        libSymbol->SetLibId( libId );

        // generate graphic
        LIB_PIN* pin = new LIB_PIN( libSymbol );
        libSymbol->AddDrawItem( pin );

        pin->SetName( elem.text );
        pin->SetPosition( { 0, 0 } );
        pin->SetLength( 0 );
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );

        VECTOR2I valueFieldPos =
                HelperGeneratePowerPortGraphics( libSymbol, elem.style, m_reporter );

        libSymbol->GetValueField().SetPosition( valueFieldPos );

        // this has to be done after parsing the LIB_SYMBOL!
        m_pi->SaveSymbol( getLibFileName().GetFullPath(), libSymbol, m_properties.get() );
        m_powerSymbols.insert( { elem.text, libSymbol } );
    }

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    // each symbol has its own powerSymbolIt for now
    SCH_SYMBOL* symbol = new SCH_SYMBOL();
    symbol->SetRef( &m_sheetPath, "#PWR?" );
    symbol->SetValueFieldText( elem.text );
    symbol->SetLibId( libId );
    symbol->SetLibSymbol( new LIB_SYMBOL( *libSymbol ) );

    SCH_FIELD* valueField = symbol->GetField( VALUE_FIELD );
    valueField->SetVisible( elem.showNetName );

    // TODO: Why do I need to set this a second time?
    valueField->SetPosition( libSymbol->GetValueField().GetPosition() );

    symbol->SetPosition( elem.location + m_sheetOffset );

    switch( elem.orientation )
    {
    case ASCH_RECORD_ORIENTATION::RIGHTWARDS:
        symbol->SetOrientation( SYMBOL_ORIENTATION_T::SYM_ORIENT_90 );
        valueField->SetTextAngle( ANGLE_VERTICAL );
        valueField->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;

    case ASCH_RECORD_ORIENTATION::UPWARDS:
        symbol->SetOrientation( SYMBOL_ORIENTATION_T::SYM_ORIENT_180 );
        valueField->SetTextAngle( ANGLE_HORIZONTAL );
        valueField->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        break;

    case ASCH_RECORD_ORIENTATION::LEFTWARDS:
        symbol->SetOrientation( SYMBOL_ORIENTATION_T::SYM_ORIENT_270 );
        valueField->SetTextAngle( ANGLE_VERTICAL );
        valueField->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;

    case ASCH_RECORD_ORIENTATION::DOWNWARDS:
        symbol->SetOrientation( SYMBOL_ORIENTATION_T::SYM_ORIENT_0 );
        valueField->SetTextAngle( ANGLE_HORIZONTAL );
        valueField->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        break;

    default:
        m_reporter->Report( _( "Pin has unexpected orientation." ), RPT_SEVERITY_WARNING );
        break;
    }

    screen->Append( symbol );
}


void SCH_ALTIUM_PLUGIN::ParseHarnessPort( const ASCH_PORT& aElem )
{
    SCH_TEXTBOX* textBox = new SCH_TEXTBOX();

    textBox->SetText( aElem.Name );
    textBox->SetTextColor( GetColorFromInt( aElem.TextColor ) );

    int height = aElem.Height;
    if( height <= 0 )
        height = schIUScale.MilsToIU( 100 ); //  chose default 50 grid

    textBox->SetStartX( ( aElem.Location + m_sheetOffset ).x );
    textBox->SetStartY( ( aElem.Location + m_sheetOffset ).y - ( height / 2 ) );
    textBox->SetEndX( ( aElem.Location + m_sheetOffset ).x + ( aElem.Width ) );
    textBox->SetEndY( ( aElem.Location + m_sheetOffset ).y + ( height / 2 ) );

    textBox->SetFillColor( HARNESS_PORT_COLOR_DEFAULT_BACKGROUND );
    textBox->SetFillMode( FILL_T::FILLED_WITH_COLOR );

    textBox->SetStroke( STROKE_PARAMS( 2, PLOT_DASH_TYPE::DEFAULT,
                                       HARNESS_PORT_COLOR_DEFAULT_OUTLINE ) );

    switch( aElem.Alignment )
    {
    default:
    case ASCH_TEXT_FRAME_ALIGNMENT::LEFT:
        textBox->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;

    case ASCH_TEXT_FRAME_ALIGNMENT::CENTER:
        textBox->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        break;

    case ASCH_TEXT_FRAME_ALIGNMENT::RIGHT:
        textBox->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;
    }

    size_t fontId = static_cast<int>( aElem.FontID );

    if( m_altiumSheet && fontId > 0 && fontId <= m_altiumSheet->fonts.size() )
    {
        const ASCH_SHEET_FONT& font = m_altiumSheet->fonts.at( fontId - 1 );
        textBox->SetItalic( font.Italic );
        textBox->SetBold( font.Bold );
        textBox->SetTextSize( { font.Size / 2, font.Size / 2 } );
        //textBox->SetFont(  //how to set font, we have a font mane here: ( font.fontname );
    }

    textBox->SetFlags( IS_NEW );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    screen->Append( textBox );

    m_reporter->Report( wxString::Format( _( "Altium's harness port (%s) was imported as "
                                             "a text box. Please review the imported "
                                             "schematic." ),
                                          aElem.Name ),
                        RPT_SEVERITY_WARNING );
}


void SCH_ALTIUM_PLUGIN::ParsePort( const ASCH_PORT& aElem )
{
    if( !aElem.HarnessType.IsEmpty() )
    {
        // Parse harness ports after "Additional" compound section is parsed
        m_altiumHarnessPortsCurrentSheet.emplace_back( aElem );
        return;
    }

    VECTOR2I start = aElem.Location + m_sheetOffset;
    VECTOR2I end = start;

    switch( aElem.Style )
    {
    default:
    case ASCH_PORT_STYLE::NONE_HORIZONTAL:
    case ASCH_PORT_STYLE::LEFT:
    case ASCH_PORT_STYLE::RIGHT:
    case ASCH_PORT_STYLE::LEFT_RIGHT:
        end.x += aElem.Width;
        break;

    case ASCH_PORT_STYLE::NONE_VERTICAL:
    case ASCH_PORT_STYLE::TOP:
    case ASCH_PORT_STYLE::BOTTOM:
    case ASCH_PORT_STYLE::TOP_BOTTOM:
        end.y -= aElem.Width;
        break;
    }

    // Check which connection points exists in the schematic
    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    bool startIsWireTerminal = screen->IsTerminalPoint( start, LAYER_WIRE );
    bool startIsBusTerminal  = screen->IsTerminalPoint( start, LAYER_BUS );

    bool endIsWireTerminal = screen->IsTerminalPoint( end, LAYER_WIRE );
    bool endIsBusTerminal  = screen->IsTerminalPoint( end, LAYER_BUS );

    // check if any of the points is a terminal point
    // TODO: there seems a problem to detect approximated connections towards component pins?
    bool connectionFound = startIsWireTerminal
                            || startIsBusTerminal
                            || endIsWireTerminal
                            || endIsBusTerminal;

    if( !connectionFound )
    {
        m_reporter->Report( wxString::Format( _( "Port %s has no connections." ), aElem.Name ),
                            RPT_SEVERITY_WARNING );
    }

    // Select label position. In case both match, we will add a line later.
    VECTOR2I        position = ( startIsWireTerminal || startIsBusTerminal ) ? start : end;
    SCH_LABEL_BASE* label;

    // TODO: detect correct label type depending on sheet settings, etc.
    #if 1   // Set to 1 to use SCH_HIERLABEL label, 0 to use SCH_GLOBALLABEL
    {
        label = new SCH_HIERLABEL( position, aElem.Name );
    }
    #else
    label = new SCH_GLOBALLABEL( position, aElem.Name );
    #endif

    switch( aElem.IOtype )
    {
    default:
    case ASCH_PORT_IOTYPE::UNSPECIFIED:
        label->SetShape( LABEL_FLAG_SHAPE::L_UNSPECIFIED );
        break;

    case ASCH_PORT_IOTYPE::OUTPUT:
        label->SetShape( LABEL_FLAG_SHAPE::L_OUTPUT );
        break;

    case ASCH_PORT_IOTYPE::INPUT:
        label->SetShape( LABEL_FLAG_SHAPE::L_INPUT );
        break;

    case ASCH_PORT_IOTYPE::BIDI:
        label->SetShape( LABEL_FLAG_SHAPE::L_BIDI );
        break;
    }

    switch( aElem.Style )
    {
    default:
    case ASCH_PORT_STYLE::NONE_HORIZONTAL:
    case ASCH_PORT_STYLE::LEFT:
    case ASCH_PORT_STYLE::RIGHT:
    case ASCH_PORT_STYLE::LEFT_RIGHT:
        if( ( startIsWireTerminal || startIsBusTerminal ) )
            label->SetTextSpinStyle( TEXT_SPIN_STYLE::RIGHT );
        else
            label->SetTextSpinStyle( TEXT_SPIN_STYLE::LEFT );
        break;

    case ASCH_PORT_STYLE::NONE_VERTICAL:
    case ASCH_PORT_STYLE::TOP:
    case ASCH_PORT_STYLE::BOTTOM:
    case ASCH_PORT_STYLE::TOP_BOTTOM:
        if( ( startIsWireTerminal || startIsBusTerminal ) )
            label->SetTextSpinStyle( TEXT_SPIN_STYLE::UP );
        else
            label->SetTextSpinStyle( TEXT_SPIN_STYLE::BOTTOM );
        break;
    }

    label->AutoplaceFields( screen, false );

    // Default "Sheet References" field should be hidden, at least for now
    if( label->GetFields().size() > 0 )
        label->GetFields()[0].SetVisible( false );

    label->SetFlags( IS_NEW );

    screen->Append( label );

    // This is a hack, for the case both connection points are valid: add a small wire
    if( ( startIsWireTerminal && endIsWireTerminal ) )
    {
        SCH_LINE* wire = new SCH_LINE( start, SCH_LAYER_ID::LAYER_WIRE );
        wire->SetEndPoint( end );
        wire->SetLineWidth( schIUScale.MilsToIU( 2 ) );
        wire->SetFlags( IS_NEW );
        screen->Append( wire );
    }
    else if( startIsBusTerminal && endIsBusTerminal )
    {
        SCH_LINE* wire = new SCH_LINE( start, SCH_LAYER_ID::LAYER_BUS );
        wire->SetEndPoint( end );
        wire->SetLineWidth( schIUScale.MilsToIU( 2 ) );
        wire->SetFlags( IS_NEW );
        screen->Append( wire );
    }
}


void SCH_ALTIUM_PLUGIN::ParseNoERC( const std::map<wxString, wxString>& aProperties )
{
    ASCH_NO_ERC elem( aProperties );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    if( elem.isActive )
    {
        SCH_NO_CONNECT* noConnect = new SCH_NO_CONNECT( elem.location + m_sheetOffset );

        noConnect->SetFlags( IS_NEW );
        screen->Append( noConnect );
    }
}


void SCH_ALTIUM_PLUGIN::ParseNetLabel( const std::map<wxString, wxString>& aProperties )
{
    ASCH_NET_LABEL elem( aProperties );

    SCH_LABEL* label = new SCH_LABEL( elem.location + m_sheetOffset, elem.text );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    SetTextPositioning( label, elem.justification, elem.orientation );

    label->SetFlags( IS_NEW );
    screen->Append( label );
}


void SCH_ALTIUM_PLUGIN::ParseBus( const std::map<wxString, wxString>& aProperties )
{
    ASCH_BUS elem( aProperties );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    for( size_t i = 0; i + 1 < elem.points.size(); i++ )
    {
        SCH_LINE* bus = new SCH_LINE( elem.points.at( i ) + m_sheetOffset,
                                      SCH_LAYER_ID::LAYER_BUS );
        bus->SetEndPoint( elem.points.at( i + 1 ) + m_sheetOffset );
        bus->SetLineWidth( elem.lineWidth );

        bus->SetFlags( IS_NEW );
        screen->Append( bus );
    }
}


void SCH_ALTIUM_PLUGIN::ParseWire( const std::map<wxString, wxString>& aProperties )
{
    ASCH_WIRE elem( aProperties );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    for( size_t i = 0; i + 1 < elem.points.size(); i++ )
    {
        SCH_LINE* wire =
                new SCH_LINE( elem.points.at( i ) + m_sheetOffset, SCH_LAYER_ID::LAYER_WIRE );
        wire->SetEndPoint( elem.points.at( i + 1 ) + m_sheetOffset );
        wire->SetLineWidth( elem.lineWidth );

        wire->SetFlags( IS_NEW );
        screen->Append( wire );
    }
}


void SCH_ALTIUM_PLUGIN::ParseJunction( const std::map<wxString, wxString>& aProperties )
{
    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    ASCH_JUNCTION elem( aProperties );

    SCH_JUNCTION* junction = new SCH_JUNCTION( elem.location + m_sheetOffset );

    junction->SetFlags( IS_NEW );
    screen->Append( junction );
}


void SCH_ALTIUM_PLUGIN::ParseImage( const std::map<wxString, wxString>& aProperties )
{
    ASCH_IMAGE elem( aProperties );

    const auto& component = m_altiumComponents.find( elem.ownerindex );

    //Hide the image if it is owned by a component but the part id do not match
    if( component != m_altiumComponents.end()
        && component->second.currentpartid != elem.ownerpartid )
        return;

    VECTOR2I                    center = ( elem.location + elem.corner ) / 2 + m_sheetOffset;
    std::unique_ptr<SCH_BITMAP> bitmap = std::make_unique<SCH_BITMAP>( center );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    if( elem.embedimage )
    {
        const ASCH_STORAGE_FILE* storageFile = GetFileFromStorage( elem.filename );

        if( !storageFile )
        {
            wxString msg = wxString::Format( _( "Embedded file %s not found in storage." ),
                                             elem.filename );
            m_reporter->Report( msg, RPT_SEVERITY_ERROR );
            return;
        }

        wxString storagePath = wxFileName::CreateTempFileName( "kicad_import_" );

        // As wxZlibInputStream is not seekable, we need to write a temporary file
        wxMemoryInputStream fileStream( storageFile->data.data(), storageFile->data.size() );
        wxZlibInputStream   zlibInputStream( fileStream );
        wxFFileOutputStream outputStream( storagePath );
        outputStream.Write( zlibInputStream );
        outputStream.Close();

        if( !bitmap->ReadImageFile( storagePath ) )
        {
            m_reporter->Report( wxString::Format( _( "Error reading image %s." ), storagePath ),
                                RPT_SEVERITY_ERROR );
            return;
        }

        // Remove temporary file
        wxRemoveFile( storagePath );
    }
    else
    {
        if( !wxFileExists( elem.filename ) )
        {
            m_reporter->Report( wxString::Format( _( "File not found %s." ), elem.filename ),
                                RPT_SEVERITY_ERROR );
            return;
        }

        if( !bitmap->ReadImageFile( elem.filename ) )
        {
            m_reporter->Report( wxString::Format( _( "Error reading image %s." ), elem.filename ),
                                RPT_SEVERITY_ERROR );
            return;
        }
    }

    // we only support one scale, thus we need to select one in case it does not keep aspect ratio
    VECTOR2I currentImageSize = bitmap->GetSize();
    VECTOR2I expectedImageSize = elem.location - elem.corner;
    double   scaleX = std::abs( static_cast<double>( expectedImageSize.x ) / currentImageSize.x );
    double   scaleY = std::abs( static_cast<double>( expectedImageSize.y ) / currentImageSize.y );
    bitmap->SetImageScale( std::min( scaleX, scaleY ) );

    bitmap->SetFlags( IS_NEW );
    screen->Append( bitmap.release() );
}


void SCH_ALTIUM_PLUGIN::ParseSheet( const std::map<wxString, wxString>& aProperties )
{
    m_altiumSheet = std::make_unique<ASCH_SHEET>( aProperties );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    PAGE_INFO pageInfo;

    bool isPortrait = m_altiumSheet->sheetOrientation == ASCH_SHEET_WORKSPACEORIENTATION::PORTRAIT;

    switch( m_altiumSheet->sheetSize )
    {
    default:
    case ASCH_SHEET_SIZE::A4:      pageInfo.SetType( "A4", isPortrait );       break;
    case ASCH_SHEET_SIZE::A3:      pageInfo.SetType( "A3", isPortrait );       break;
    case ASCH_SHEET_SIZE::A2:      pageInfo.SetType( "A2", isPortrait );       break;
    case ASCH_SHEET_SIZE::A1:      pageInfo.SetType( "A1", isPortrait );       break;
    case ASCH_SHEET_SIZE::A0:      pageInfo.SetType( "A0", isPortrait );       break;
    case ASCH_SHEET_SIZE::A:       pageInfo.SetType( "A", isPortrait );        break;
    case ASCH_SHEET_SIZE::B:       pageInfo.SetType( "B", isPortrait );        break;
    case ASCH_SHEET_SIZE::C:       pageInfo.SetType( "C", isPortrait );        break;
    case ASCH_SHEET_SIZE::D:       pageInfo.SetType( "D", isPortrait );        break;
    case ASCH_SHEET_SIZE::E:       pageInfo.SetType( "E", isPortrait );        break;
    case ASCH_SHEET_SIZE::LETTER:  pageInfo.SetType( "USLetter", isPortrait ); break;
    case ASCH_SHEET_SIZE::LEGAL:   pageInfo.SetType( "USLegal", isPortrait );  break;
    case ASCH_SHEET_SIZE::TABLOID: pageInfo.SetType( "A3", isPortrait );       break;
    case ASCH_SHEET_SIZE::ORCAD_A: pageInfo.SetType( "A", isPortrait );        break;
    case ASCH_SHEET_SIZE::ORCAD_B: pageInfo.SetType( "B", isPortrait );        break;
    case ASCH_SHEET_SIZE::ORCAD_C: pageInfo.SetType( "C", isPortrait );        break;
    case ASCH_SHEET_SIZE::ORCAD_D: pageInfo.SetType( "D", isPortrait );        break;
    case ASCH_SHEET_SIZE::ORCAD_E: pageInfo.SetType( "E", isPortrait );        break;
    }

    screen->SetPageSettings( pageInfo );

    m_sheetOffset = { 0, pageInfo.GetHeightIU( schIUScale.IU_PER_MILS ) };
}


void SCH_ALTIUM_PLUGIN::ParseSheetName( const std::map<wxString, wxString>& aProperties )
{
    ASCH_SHEET_NAME elem( aProperties );

    const auto& sheetIt = m_sheets.find( elem.ownerindex );

    if( sheetIt == m_sheets.end() )
    {
        m_reporter->Report( wxString::Format( wxT( "Sheetname's owner (%d) not found." ),
                                              elem.ownerindex ),
                            RPT_SEVERITY_DEBUG );
        return;
    }

    SCH_FIELD& sheetNameField = sheetIt->second->GetFields()[SHEETNAME];

    sheetNameField.SetPosition( elem.location + m_sheetOffset );
    sheetNameField.SetText( elem.text );
    sheetNameField.SetVisible( !elem.isHidden );
    SetTextPositioning( &sheetNameField, ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT, elem.orientation );
}


void SCH_ALTIUM_PLUGIN::ParseFileName( const std::map<wxString, wxString>& aProperties )
{
    ASCH_FILE_NAME elem( aProperties );

    const auto& sheetIt = m_sheets.find( elem.ownerindex );

    if( sheetIt == m_sheets.end() )
    {
        m_reporter->Report( wxString::Format( wxT( "Filename's owner (%d) not found." ),
                                              elem.ownerindex ),
                            RPT_SEVERITY_DEBUG );
        return;
    }

    SCH_FIELD& filenameField = sheetIt->second->GetFields()[SHEETFILENAME];

    filenameField.SetPosition( elem.location + m_sheetOffset );

    // Keep the filename of the Altium file until after the file is actually loaded.
    filenameField.SetText( elem.text );
    filenameField.SetVisible( !elem.isHidden );
    SetTextPositioning( &filenameField, ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT, elem.orientation );
}


void SCH_ALTIUM_PLUGIN::ParseDesignator( const std::map<wxString, wxString>& aProperties )
{
    ASCH_DESIGNATOR elem( aProperties );

    const auto& libSymbolIt = m_libSymbols.find( elem.ownerindex );

    if( libSymbolIt == m_libSymbols.end() )
    {
        // TODO: e.g. can depend on Template (RECORD=39
        m_reporter->Report( wxString::Format( wxT( "Designator's owner (%d) not found." ),
                                              elem.ownerindex ),
                            RPT_SEVERITY_DEBUG );
        return;
    }

    SCH_SYMBOL*    symbol = m_symbols.at( libSymbolIt->first );
    SCH_SHEET_PATH sheetpath;

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    // Graphics symbols have no reference. '#GRAPHIC' allows them to not have footprint associated.
    // Note: not all unnamed imported symbols are necessarily graphics.
    bool emptyRef = elem.text.IsEmpty();
    symbol->SetRef( &m_sheetPath, emptyRef ? wxString( wxS( "#GRAPHIC" ) ) : elem.text );

    // I am not sure value and ref should be invisible just because emptyRef is true
    // I have examples with this criteria fully incorrect.
    bool visible = !emptyRef;

    symbol->GetField( VALUE_FIELD )->SetVisible( visible );
    symbol->GetField( REFERENCE_FIELD )->SetVisible( visible );

    SCH_FIELD* field = symbol->GetField( REFERENCE_FIELD );
    field->SetPosition( elem.location + m_sheetOffset );
    SetTextPositioning( field, elem.justification, elem.orientation );
}


void SCH_ALTIUM_PLUGIN::ParseBusEntry( const std::map<wxString, wxString>& aProperties )
{
    ASCH_BUS_ENTRY elem( aProperties );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    SCH_BUS_WIRE_ENTRY* busWireEntry = new SCH_BUS_WIRE_ENTRY( elem.location + m_sheetOffset );

    VECTOR2I vector = elem.corner - elem.location;
    busWireEntry->SetSize( { vector.x, vector.y } );

    busWireEntry->SetFlags( IS_NEW );
    screen->Append( busWireEntry );
}


void SCH_ALTIUM_PLUGIN::ParseParameter( const std::map<wxString, wxString>& aProperties )
{
    ASCH_PARAMETER elem( aProperties );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    // TODO: fill in replacements from variant, sheet and project
    std::map<wxString, wxString> variableMap = {
        { "COMMENT", "VALUE"        },
        { "VALUE",   "ALTIUM_VALUE" },
    };

    if( elem.ownerindex <= 0 && elem.ownerpartid == ALTIUM_COMPONENT_NONE )
    {
        // This is some sheet parameter
        if( elem.text == "*" )
            return; // indicates parameter not set?

        wxString paramName = elem.name.Upper();

        if( paramName == "SHEETNUMBER" )
        {
            m_sheetPath.SetPageNumber( elem.text );
        }
        else if( paramName == "TITLE" )
        {
            m_currentTitleBlock->SetTitle( elem.text );
        }
        else if( paramName == "REVISION" )
        {
            m_currentTitleBlock->SetRevision( elem.text );
        }
        else if( paramName == "DATE" )
        {
            m_currentTitleBlock->SetDate( elem.text );
        }
        else if( paramName == "COMPANYNAME" )
        {
            m_currentTitleBlock->SetCompany( elem.text );
        }
        else
        {
            m_schematic->Prj().GetTextVars()[ paramName ] = elem.text;
        }
    }
    else
    {
        const auto& libSymbolIt = m_libSymbols.find( elem.ownerindex );

        if( libSymbolIt == m_libSymbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            return;
        }

        SCH_SYMBOL* symbol = m_symbols.at( libSymbolIt->first );
        SCH_FIELD*  field = nullptr;
        wxString    upperName = elem.name.Upper();

        if( upperName == "COMMENT" )
        {
            field = symbol->GetField( VALUE_FIELD );
        }
        else
        {
            int      fieldIdx = symbol->GetFieldCount();
            wxString fieldName = elem.name.Upper();

            if( fieldName.IsEmpty() )
            {
                int disambiguate = 1;

                do
                {
                    fieldName = wxString::Format( "ALTIUM_UNNAMED_%d", disambiguate++ );
                } while( !symbol->GetFieldText( fieldName ).IsEmpty() );
            }
            else if( fieldName == "VALUE" )
            {
                fieldName = "ALTIUM_VALUE";
            }

            field = symbol->AddField( SCH_FIELD( VECTOR2I(), fieldIdx, symbol, fieldName ) );
        }

        wxString kicadText = AltiumSpecialStringsToKiCadVariables( elem.text, variableMap );
        field->SetText( kicadText );
        field->SetPosition( elem.location + m_sheetOffset );
        field->SetVisible( !elem.isHidden );
        SetTextPositioning( field, elem.justification, elem.orientation );
    }
}


void SCH_ALTIUM_PLUGIN::ParseImplementationList( int aIndex,
                                                 const std::map<wxString, wxString>& aProperties )
{
    ASCH_IMPLEMENTATION_LIST elem( aProperties );

    m_altiumImplementationList.emplace( aIndex, elem.ownerindex );
}


void SCH_ALTIUM_PLUGIN::ParseImplementation( const std::map<wxString, wxString>& aProperties )
{
    ASCH_IMPLEMENTATION elem( aProperties );

    // Only get footprint, currently assigned only
    if( ( elem.type == "PCBLIB" ) && ( elem.isCurrent ) )
    {
        const auto& implementationOwnerIt = m_altiumImplementationList.find( elem.ownerindex );

        if( implementationOwnerIt == m_altiumImplementationList.end() )
        {
            m_reporter->Report( wxString::Format( wxT( "Implementation's owner (%d) not found." ),
                                                  elem.ownerindex ),
                                RPT_SEVERITY_DEBUG );
            return;
        }

        const auto& libSymbolIt = m_libSymbols.find( implementationOwnerIt->second );

        if( libSymbolIt == m_libSymbols.end() )
        {
            m_reporter->Report( wxString::Format( wxT( "Footprint's owner (%d) not found." ),
                                                  implementationOwnerIt->second ),
                                RPT_SEVERITY_DEBUG );
            return;
        }

        LIB_ID        fpLibId = AltiumToKiCadLibID( elem.libname, elem.name );
        wxArrayString fpFilters;
        fpFilters.Add( fpLibId.Format() );

        libSymbolIt->second->SetFPFilters( fpFilters ); // TODO: not ideal as we overwrite it

        SCH_SYMBOL* symbol = m_symbols.at( libSymbolIt->first );

        symbol->SetFootprintFieldText( fpLibId.Format() );
    }
}
