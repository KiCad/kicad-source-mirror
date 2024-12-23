/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
 * Copyright (C) 2021-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <io/io_utils.h>
#include <io/altium/altium_binary_parser.h>
#include <io/altium/altium_ascii_parser.h>
#include <io/altium/altium_parser_utils.h>
#include <sch_io/altium/sch_io_altium.h>

#include <schematic.h>
#include <project_sch.h>
#include <project/project_file.h>
#include <project/net_settings.h>

#include <lib_id.h>
#include <sch_pin.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_symbol.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_shape.h>
#include <sch_no_connect.h>
#include <sch_screen.h>
#include <sch_label.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_textbox.h>
#include <symbol_lib_table.h>

#include <bezier_curves.h>
#include <compoundfilereader.h>
#include <font/fontconfig.h>
#include <geometry/ellipse.h>
#include <string_utils.h>
#include <sch_edit_frame.h>
#include <wildcards_and_files_ext.h>
#include <wx/log.h>
#include <wx/dir.h>
#include <wx/mstream.h>
#include <wx/zstream.h>
#include <wx/wfstream.h>
#include <magic_enum.hpp>
#include "sch_io_altium.h"


/**
 * Flag to enable Altium schematic debugging output.
 *
 * @ingroup trace_env_vars
 */
static const wxChar traceAltiumSch[] = wxT( "KICAD_ALTIUM_SCH" );


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


static LINE_STYLE GetPlotDashType( const ASCH_POLYLINE_LINESTYLE linestyle )
{
    switch( linestyle )
    {
    case ASCH_POLYLINE_LINESTYLE::SOLID:       return LINE_STYLE::SOLID;
    case ASCH_POLYLINE_LINESTYLE::DASHED:      return LINE_STYLE::DASH;
    case ASCH_POLYLINE_LINESTYLE::DOTTED:      return LINE_STYLE::DOT;
    case ASCH_POLYLINE_LINESTYLE::DASH_DOTTED: return LINE_STYLE::DASHDOT;
    default:                                   return LINE_STYLE::DEFAULT;
    }
}


static void SetSchShapeLine( const ASCH_BORDER_INTERFACE& elem, SCH_SHAPE* shape )
{
    shape->SetStroke( STROKE_PARAMS( elem.LineWidth, LINE_STYLE::SOLID,
                                     GetColorFromInt( elem.Color ) ) );
}

static void SetSchShapeFillAndColor( const ASCH_FILL_INTERFACE& elem, SCH_SHAPE* shape )
{

    if( !elem.IsSolid )
    {
        shape->SetFillMode( FILL_T::NO_FILL );
    }
    else
    {
        shape->SetFillMode( FILL_T::FILLED_WITH_COLOR );
        shape->SetFillColor( GetColorFromInt( elem.AreaColor ) );
    }

    // Fixup small circles that had their widths set to 0
    if( shape->GetShape() == SHAPE_T::CIRCLE && shape->GetStroke().GetWidth() == 0
        && shape->GetRadius() <= schIUScale.MilsToIU( 10 ) )
    {
        shape->SetFillMode( FILL_T::FILLED_SHAPE );
    }
}


static void SetLibShapeLine( const ASCH_BORDER_INTERFACE& elem, SCH_SHAPE* shape,
                             ALTIUM_SCH_RECORD aType )
{
    COLOR4D default_color;
    COLOR4D alt_default_color = COLOR4D( PUREBLUE ); // PUREBLUE is used for many objects, so if
                                                     // it is used, we will assume that it should
                                                     // blend with the others
    STROKE_PARAMS stroke;
    stroke.SetColor( GetColorFromInt( elem.Color ) );
    stroke.SetLineStyle( LINE_STYLE::SOLID );

    switch( aType )
    {
    case ALTIUM_SCH_RECORD::ARC:             default_color = COLOR4D( PUREBLUE );       break;
    case ALTIUM_SCH_RECORD::BEZIER:          default_color = COLOR4D( PURERED );        break;
    case ALTIUM_SCH_RECORD::ELLIPSE:         default_color = COLOR4D( PUREBLUE );       break;
    case ALTIUM_SCH_RECORD::ELLIPTICAL_ARC:  default_color = COLOR4D( PUREBLUE );       break;
    case ALTIUM_SCH_RECORD::LINE:            default_color = COLOR4D( PUREBLUE );       break;
    case ALTIUM_SCH_RECORD::POLYGON:         default_color = COLOR4D( PUREBLUE );       break;
    case ALTIUM_SCH_RECORD::POLYLINE:        default_color = COLOR4D( BLACK );          break;
    case ALTIUM_SCH_RECORD::RECTANGLE:       default_color = COLOR4D( 0.5, 0, 0, 1.0 ); break;
    case ALTIUM_SCH_RECORD::ROUND_RECTANGLE: default_color = COLOR4D( PUREBLUE );       break;
    default:                                 default_color = COLOR4D( PUREBLUE );       break;
    }

    if( stroke.GetColor() == default_color || stroke.GetColor() == alt_default_color )
        stroke.SetColor( COLOR4D::UNSPECIFIED );

    // In Altium libraries, you cannot change the width of the pins.  So, to match pin width,
    // if the line width of other elements is the default pin width (10 mil), we set the width
    // to the KiCad default pin width ( represented by 0 )
    if( elem.LineWidth == 2540 )
        stroke.SetWidth( 0 );
    else
        stroke.SetWidth( elem.LineWidth );

    shape->SetStroke( stroke );
}

static void SetLibShapeFillAndColor( const ASCH_FILL_INTERFACE& elem, SCH_SHAPE* shape,
                                     ALTIUM_SCH_RECORD aType, int aStrokeColor )
{
    COLOR4D bgcolor = GetColorFromInt( elem.AreaColor );
    COLOR4D default_bgcolor;

    switch (aType)
    {
    case ALTIUM_SCH_RECORD::RECTANGLE:
        default_bgcolor = GetColorFromInt( 11599871 ); // Light Yellow
        break;
    default:
        default_bgcolor = GetColorFromInt( 12632256 ); // Grey
        break;
    }

    if( elem.IsTransparent )
        bgcolor = bgcolor.WithAlpha( 0.5 );

    if( !elem.IsSolid )
    {
        shape->SetFillMode( FILL_T::NO_FILL );
    }
    else if( elem.AreaColor == aStrokeColor )
    {
        bgcolor = shape->GetStroke().GetColor();

        shape->SetFillMode( FILL_T::FILLED_SHAPE );
    }
    else if( bgcolor.WithAlpha( 1.0 ) == default_bgcolor )
    {
        shape->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );
    }
    else
    {
        shape->SetFillMode( FILL_T::FILLED_WITH_COLOR );
    }

    shape->SetFillColor( bgcolor );

    if( elem.AreaColor == aStrokeColor
            && shape->GetStroke().GetWidth() == schIUScale.MilsToIU( 1 ) )
    {
        STROKE_PARAMS stroke = shape->GetStroke();
        stroke.SetWidth( -1 );
        shape->SetStroke( stroke );
    }

    // Fixup small circles that had their widths set to 0
    if( shape->GetShape() == SHAPE_T::CIRCLE && shape->GetStroke().GetWidth() == 0
        && shape->GetRadius() <= schIUScale.MilsToIU( 10 ) )
    {
        shape->SetFillMode( FILL_T::FILLED_SHAPE );
    }
}


SCH_IO_ALTIUM::SCH_IO_ALTIUM() :
        SCH_IO( wxS( "Altium" ) )
{
    m_isIntLib     = false;
    m_rootSheet    = nullptr;
    m_schematic    = nullptr;
    m_harnessOwnerIndexOffset = 0;
    m_harnessEntryParent      = 0;

    m_reporter     = &WXLOG_REPORTER::GetInstance();
}


SCH_IO_ALTIUM::~SCH_IO_ALTIUM()
{
    for( auto& [libName, lib] : m_libCache )
    {
        for( auto& [name, symbol] : lib )
            delete symbol;
    }
}


int SCH_IO_ALTIUM::GetModifyHash() const
{
    return 0;
}


bool SCH_IO_ALTIUM::isBinaryFile( const wxString& aFileName )
{
    // Compound File Binary Format header
    return IO_UTILS::fileStartsWithBinaryHeader( aFileName, IO_UTILS::COMPOUND_FILE_HEADER );
}


bool SCH_IO_ALTIUM::isASCIIFile( const wxString& aFileName )
{
    // ASCII file format
    return IO_UTILS::fileStartsWithPrefix( aFileName, wxS( "|HEADER=" ), false );
}


bool SCH_IO_ALTIUM::checkFileHeader( const wxString& aFileName )
{
    return isBinaryFile( aFileName ) || isASCIIFile( aFileName );
}


bool SCH_IO_ALTIUM::CanReadSchematicFile( const wxString& aFileName ) const
{
    if( !SCH_IO::CanReadSchematicFile( aFileName ) )
        return false;

    return checkFileHeader( aFileName );
}


bool SCH_IO_ALTIUM::CanReadLibrary( const wxString& aFileName ) const
{
    if( !SCH_IO::CanReadLibrary( aFileName ) )
        return false;

    return checkFileHeader( aFileName );
}


void SCH_IO_ALTIUM::fixupSymbolPinNameNumbers( SYMBOL* aSymbol )
{
    std::vector<SCH_PIN*> pins = aSymbol->GetPins();

    bool names_visible = false;
    bool numbers_visible = false;

    for( SCH_PIN* pin : pins )
    {
        if( pin->GetNameTextSize() > 0 && !pin->GetName().empty() )
            names_visible = true;

        if( pin->GetNumberTextSize() > 0 && !pin->GetNumber().empty() )
            numbers_visible = true;
    }

    if( !names_visible )
    {
        for( SCH_PIN* pin : pins )
            pin->SetNameTextSize( schIUScale.MilsToIU( DEFAULT_PINNAME_SIZE ) );

        aSymbol->SetShowPinNames( false );
    }

    if( !numbers_visible )
    {
        for( SCH_PIN* pin : pins )
            pin->SetNumberTextSize( schIUScale.MilsToIU( DEFAULT_PINNUM_SIZE ) );

        aSymbol->SetShowPinNumbers( false );
    }
}


wxString SCH_IO_ALTIUM::getLibName()
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
        m_libName = LIB_ID::FixIllegalChars( m_libName, true ).wx_str();
    }

    return m_libName;
}


wxFileName SCH_IO_ALTIUM::getLibFileName()
{
    wxFileName fn( m_schematic->Prj().GetProjectPath(), getLibName(),
                   FILEEXT::KiCadSymbolLibFileExtension );

    return fn;
}


SCH_SHEET* SCH_IO_ALTIUM::LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                             SCH_SHEET* aAppendToMe,
                                             const std::map<std::string, UTF8>* aProperties )
{
    wxCHECK( !aFileName.IsEmpty() && aSchematic, nullptr );

    wxFileName fileName( aFileName );
    fileName.SetExt( FILEEXT::KiCadSchematicFileExtension );
    m_schematic = aSchematic;

    // Show the font substitution warnings
    fontconfig::FONTCONFIG::SetReporter( &WXLOG_REPORTER::GetInstance() );

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

    SYMBOL_LIB_TABLE* libTable = PROJECT_SCH::SchSymbolLibTable( &m_schematic->Prj() );

    wxCHECK_MSG( libTable, nullptr, "Could not load symbol lib table." );

    m_pi.reset( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

    /// @note No check is being done here to see if the existing symbol library exists so this
    ///       will overwrite the existing one.
    if( !libTable->HasLibrary( getLibName() ) )
    {
        // Create a new empty symbol library.
        m_pi->CreateLibrary( getLibFileName().GetFullPath() );
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
        m_schematic->Prj().SetElem( PROJECT::ELEM::SYMBOL_LIB_TABLE, nullptr );
        PROJECT_SCH::SchSymbolLibTable( &m_schematic->Prj() );
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

    // Set up the default netclass wire & bus width based on imported wires & buses.
    //

    int minWireWidth = std::numeric_limits<int>::max();
    int minBusWidth = std::numeric_limits<int>::max();

    for( SCH_SCREEN* screen = allSheets.GetFirst(); screen != nullptr; screen = allSheets.GetNext() )
    {
        std::vector<SCH_MARKER*> markers;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_LINE_T ) )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( line->IsWire() && line->GetLineWidth() > 0 )
                minWireWidth = std::min( minWireWidth, line->GetLineWidth() );

            if( line->IsBus() && line->GetLineWidth() > 0 )
                minBusWidth = std::min( minBusWidth, line->GetLineWidth() );
        }
    }

    std::shared_ptr<NET_SETTINGS>& netSettings = m_schematic->Prj().GetProjectFile().NetSettings();

    if( minWireWidth < std::numeric_limits<int>::max() )
        netSettings->GetDefaultNetclass()->SetWireWidth( minWireWidth );

    if( minBusWidth < std::numeric_limits<int>::max() )
        netSettings->GetDefaultNetclass()->SetBusWidth( minBusWidth );

    return m_rootSheet;
}


SCH_SCREEN* SCH_IO_ALTIUM::getCurrentScreen()
{
    return m_sheetPath.LastScreen();
}


SCH_SHEET* SCH_IO_ALTIUM::getCurrentSheet()
{
    return m_sheetPath.Last();
}


void SCH_IO_ALTIUM::ParseAltiumSch( const wxString& aFileName )
{
    // Load path may be different from the project path.
    wxFileName parentFileName = aFileName;

    if( isBinaryFile( aFileName ) )
    {
        ALTIUM_COMPOUND_FILE altiumSchFile( aFileName );

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
            wxLogTrace( traceAltiumSch, wxS( "Unhandled exception in Altium schematic "
                                             "parsers: %s." ), exc.what() );
            throw;
        }
    }
    else // ASCII
    {
        ParseASCIISchematic( aFileName );
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

        if( !loadAltiumFileName.IsFileReadable() )
        {
            // Try case-insensitive search
            wxArrayString files;
            wxDir::GetAllFiles( parentFileName.GetPath(), &files, wxEmptyString,
                                wxDIR_FILES | wxDIR_HIDDEN );

            for( const wxString& candidate : files )
            {
                wxFileName candidateFname( candidate );

                if( candidateFname.GetFullName().IsSameAs( sheet->GetFileName(), false ) )
                {
                    loadAltiumFileName = candidateFname;
                    break;
                }
            }
        }

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

            if( sheet->GetName().Trim().empty() )
                sheet->SetName( loadAltiumFileName.GetName() );

            wxCHECK2( screen, continue );

            m_sheetPath.push_back( sheet );
            ParseAltiumSch( loadAltiumFileName.GetFullPath() );

            // Map the loaded Altium file to the project file.
            wxFileName projectFileName = loadAltiumFileName;
            projectFileName.SetPath( m_schematic->Prj().GetProjectPath() );
            projectFileName.SetExt( FILEEXT::KiCadSchematicFileExtension );
            sheet->SetFileName( projectFileName.GetFullName() );
            screen->SetFileName( projectFileName.GetFullPath() );

            m_sheetPath.pop_back();
        }
    }
}


void SCH_IO_ALTIUM::ParseStorage( const ALTIUM_COMPOUND_FILE& aAltiumSchFile )
{
    const CFB::COMPOUND_FILE_ENTRY* file = aAltiumSchFile.FindStream( { "Storage" } );

    if( file == nullptr )
        return;

    ALTIUM_BINARY_PARSER reader( aAltiumSchFile, file );

    std::map<wxString, wxString> properties = reader.ReadProperties();
    wxString header = ALTIUM_PROPS_UTILS::ReadString( properties, "HEADER", "" );
    int      weight = ALTIUM_PROPS_UTILS::ReadInt( properties, "WEIGHT", 0 );

    if( weight < 0 )
        THROW_IO_ERROR( "Storage weight is negative!" );

    for( int i = 0; i < weight; i++ )
        m_altiumStorage.emplace_back( reader );

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


void SCH_IO_ALTIUM::ParseAdditional( const ALTIUM_COMPOUND_FILE& aAltiumSchFile )
{
    wxString streamName = wxS( "Additional" );

    const CFB::COMPOUND_FILE_ENTRY* file =
            aAltiumSchFile.FindStream( { streamName.ToStdString() } );

    if( file == nullptr )
        return;

    ALTIUM_BINARY_PARSER reader( aAltiumSchFile, file );

    if( reader.GetRemainingBytes() <= 0 )
    {
        THROW_IO_ERROR( "Additional section does not contain any data" );
    }
    else
    {
        std::map<wxString, wxString> properties = reader.ReadProperties();

        int               recordId = ALTIUM_PROPS_UTILS::ReadInt( properties, "RECORD", 0 );
        ALTIUM_SCH_RECORD record = static_cast<ALTIUM_SCH_RECORD>( recordId );

        if( record != ALTIUM_SCH_RECORD::HEADER )
            THROW_IO_ERROR( "Header expected" );
    }

    for( int index = 0; reader.GetRemainingBytes() > 0; index++ )
    {
        std::map<wxString, wxString> properties = reader.ReadProperties();

        ParseRecord( index, properties, streamName );
    }

    // Handle harness Ports
    for( const ASCH_PORT& port : m_altiumHarnessPortsCurrentSheet )
        ParseHarnessPort( port );

    if( reader.HasParsingError() )
        THROW_IO_ERROR( "stream was not parsed correctly!" );

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( "stream is not fully parsed" );

    m_altiumHarnessPortsCurrentSheet.clear();
}


void SCH_IO_ALTIUM::ParseFileHeader( const ALTIUM_COMPOUND_FILE& aAltiumSchFile )
{
    wxString streamName = wxS( "FileHeader" );

    const CFB::COMPOUND_FILE_ENTRY* file =
            aAltiumSchFile.FindStream( { streamName.ToStdString() } );

    if( file == nullptr )
        THROW_IO_ERROR( "FileHeader not found" );

    ALTIUM_BINARY_PARSER reader( aAltiumSchFile, file );

    if( reader.GetRemainingBytes() <= 0 )
    {
        THROW_IO_ERROR( "FileHeader does not contain any data" );
    }
    else
    {
        std::map<wxString, wxString> properties = reader.ReadProperties();

        wxString libtype = ALTIUM_PROPS_UTILS::ReadString( properties, "HEADER", "" );

        if( libtype.CmpNoCase( "Protel for Windows - Schematic Capture Binary File Version 5.0" ) )
            THROW_IO_ERROR( _( "Expected Altium Schematic file version 5.0" ) );
    }

    // Prepare some local variables
    wxCHECK( m_altiumPortsCurrentSheet.empty(), /* void */ );
    wxCHECK( !m_currentTitleBlock, /* void */ );

    m_currentTitleBlock = std::make_unique<TITLE_BLOCK>();

    // index is required to resolve OWNERINDEX
    for( int index = 0; reader.GetRemainingBytes() > 0; index++ )
    {
        std::map<wxString, wxString> properties = reader.ReadProperties();

        ParseRecord( index, properties, streamName );
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

        fixupSymbolPinNameNumbers( symbol.second );
        fixupSymbolPinNameNumbers( libSymbolIt->second );

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
    m_altiumTemplates.clear();
    m_altiumImplementationList.clear();

    m_symbols.clear();
    m_libSymbols.clear();

    // Otherwise we cannot save the imported sheet?
    SCH_SHEET* sheet = getCurrentSheet();

    wxCHECK( sheet, /* void */ );

    sheet->SetModified();
}


void SCH_IO_ALTIUM::ParseASCIISchematic( const wxString& aFileName )
{
    // Read storage content first
    {
        ALTIUM_ASCII_PARSER storageReader( aFileName );

        while( storageReader.CanRead() )
        {
            std::map<wxString, wxString> properties = storageReader.ReadProperties();

            // Binary data
            if( properties.find( wxS( "BINARY" ) ) != properties.end() )
                m_altiumStorage.emplace_back( properties );
        }
    }

    // Read other data
    ALTIUM_ASCII_PARSER reader( aFileName );

    if( !reader.CanRead() )
    {
        THROW_IO_ERROR( "FileHeader does not contain any data" );
    }
    else
    {
        std::map<wxString, wxString> properties = reader.ReadProperties();

        wxString libtype = ALTIUM_PROPS_UTILS::ReadString( properties, "HEADER", "" );

        if( libtype.CmpNoCase( "Protel for Windows - Schematic Capture Ascii File Version 5.0" ) )
            THROW_IO_ERROR( _( "Expected Altium Schematic file version 5.0" ) );
    }

    // Prepare some local variables
    wxCHECK( m_altiumPortsCurrentSheet.empty(), /* void */ );
    wxCHECK( !m_currentTitleBlock, /* void */ );

    m_currentTitleBlock = std::make_unique<TITLE_BLOCK>();

    // index is required to resolve OWNERINDEX
    int index = 0;

    while( reader.CanRead() )
    {
        std::map<wxString, wxString> properties = reader.ReadProperties();

        // Reset index at headers
        if( properties.find( wxS( "HEADER" ) ) != properties.end() )
        {
            index = 0;
            continue;
        }

        if( properties.find( wxS( "RECORD" ) ) != properties.end() )
            ParseRecord( index, properties, aFileName );

        index++;
    }

    if( reader.HasParsingError() )
        THROW_IO_ERROR( "stream was not parsed correctly!" );

    if( reader.CanRead() )
        THROW_IO_ERROR( "stream is not fully parsed" );

    // assign LIB_SYMBOL -> COMPONENT
    for( std::pair<const int, SCH_SYMBOL*>& symbol : m_symbols )
    {
        auto libSymbolIt = m_libSymbols.find( symbol.first );

        if( libSymbolIt == m_libSymbols.end() )
            THROW_IO_ERROR( "every symbol should have a symbol attached" );

        fixupSymbolPinNameNumbers( symbol.second );
        fixupSymbolPinNameNumbers( libSymbolIt->second );

        m_pi->SaveSymbol( getLibFileName().GetFullPath(),
                          new LIB_SYMBOL( *( libSymbolIt->second ) ), m_properties.get() );

        symbol.second->SetLibSymbol( libSymbolIt->second );
    }

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    // Handle title blocks
    screen->SetTitleBlock( *m_currentTitleBlock );
    m_currentTitleBlock.reset();

    // Handle harness Ports
    for( const ASCH_PORT& port : m_altiumHarnessPortsCurrentSheet )
        ParseHarnessPort( port );

    // Handle Ports
    for( const ASCH_PORT& port : m_altiumPortsCurrentSheet )
        ParsePort( port );

    m_altiumPortsCurrentSheet.clear();
    m_altiumComponents.clear();
    m_altiumTemplates.clear();
    m_altiumImplementationList.clear();

    m_symbols.clear();
    m_libSymbols.clear();

    // Otherwise we cannot save the imported sheet?
    SCH_SHEET* sheet = getCurrentSheet();

    wxCHECK( sheet, /* void */ );

    sheet->SetModified();
}


void SCH_IO_ALTIUM::ParseRecord( int index, std::map<wxString, wxString>& properties,
                                 const wxString& aSectionName )
{
    int               recordId = ALTIUM_PROPS_UTILS::ReadInt( properties, "RECORD", -1 );
    ALTIUM_SCH_RECORD record   = static_cast<ALTIUM_SCH_RECORD>( recordId );

    // see: https://github.com/vadmium/python-altium/blob/master/format.md
    switch( record )
    {
        // FileHeader section

    case ALTIUM_SCH_RECORD::HEADER:
        THROW_IO_ERROR( "Header already parsed" );

    case ALTIUM_SCH_RECORD::COMPONENT:
        ParseComponent( index, properties );
        break;

    case ALTIUM_SCH_RECORD::PIN:
        ParsePin( properties );
        break;

        case ALTIUM_SCH_RECORD::IEEE_SYMBOL:
            m_reporter->Report( _( "Record 'IEEE_SYMBOL' not handled." ), RPT_SEVERITY_INFO );
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
        ParsePieChart( properties );
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
        ParseTemplate( index, properties );
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

    case ALTIUM_SCH_RECORD::MAP_DEFINER_LIST:
        break;

    case ALTIUM_SCH_RECORD::MAP_DEFINER:
        break;

    case ALTIUM_SCH_RECORD::IMPL_PARAMS:
        break;

    case ALTIUM_SCH_RECORD::NOTE:
        ParseNote( properties );
        break;

    case ALTIUM_SCH_RECORD::COMPILE_MASK:
        m_reporter->Report( _( "Compile mask not currently supported." ), RPT_SEVERITY_ERROR );
        break;

    case ALTIUM_SCH_RECORD::HYPERLINK:
        break;

    // Additional section

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
        m_reporter->Report(
                wxString::Format( _( "Unknown or unexpected record id %d found in %s." ), recordId,
                                  aSectionName ),
                RPT_SEVERITY_ERROR );
        break;
    }

    SCH_IO_ALTIUM::m_harnessOwnerIndexOffset = index;
}


bool SCH_IO_ALTIUM::IsComponentPartVisible( const ASCH_OWNER_INTERFACE& aElem ) const
{
    const auto& component = m_altiumComponents.find( aElem.ownerindex );
    const auto& templ = m_altiumTemplates.find( aElem.ownerindex );

    if( component != m_altiumComponents.end() )
        return component->second.displaymode == aElem.ownerpartdisplaymode;

    if( templ != m_altiumTemplates.end() )
        return true;

    return false;
}


const ASCH_STORAGE_FILE* SCH_IO_ALTIUM::GetFileFromStorage( const wxString& aFilename ) const
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


void SCH_IO_ALTIUM::ParseComponent( int aIndex, const std::map<wxString, wxString>& aProperties )
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

    if( elem.displaymodecount > 1 )
        name << '_' << elem.displaymode;

    LIB_ID libId = AltiumToKiCadLibID( getLibName(), name );

    LIB_SYMBOL* ksymbol = new LIB_SYMBOL( wxEmptyString );
    ksymbol->SetName( name );
    ksymbol->SetDescription( elem.componentdescription );
    ksymbol->SetLibId( libId );
    m_libSymbols.insert( { aIndex, ksymbol } );

    // each component has its own symbol for now
    SCH_SYMBOL* symbol = new SCH_SYMBOL();

    symbol->SetPosition( elem.location + m_sheetOffset );

    for( SCH_FIELD& field : symbol->GetFields() )
        field.SetVisible( false );

    // TODO: keep it simple for now, and only set position.
    // component->SetOrientation( elem.orientation );
    symbol->SetLibId( libId );
    symbol->SetUnit( std::max( 0, elem.currentpartid ) );
    symbol->GetField( DESCRIPTION_FIELD )->SetText( elem.componentdescription );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    screen->Append( symbol );

    m_symbols.insert( { aIndex, symbol } );
}


void SCH_IO_ALTIUM::ParseTemplate( int aIndex, const std::map<wxString, wxString>& aProperties )
{
    SCH_SHEET* currentSheet = m_sheetPath.Last();
    wxCHECK( currentSheet, /* void */ );

    wxString sheetName = currentSheet->GetName();

    if( sheetName.IsEmpty() )
        sheetName = wxT( "root" );

    ASCH_TEMPLATE altiumTemplate( aProperties );

    // Extract base name from path
    wxString baseName = altiumTemplate.filename.AfterLast( '\\' ).BeforeLast( '.' );

    if( baseName.IsEmpty() )
        baseName = wxS( "Template" );

    m_altiumTemplates.insert( { aIndex, altiumTemplate } );
    // No need to create a symbol - graphics is put on the sheet
}


void SCH_IO_ALTIUM::ParsePin( const std::map<wxString, wxString>& aProperties,
                              std::vector<LIB_SYMBOL*>& aSymbol )
{
    ASCH_PIN elem( aProperties );

    LIB_SYMBOL* symbol = (int) aSymbol.size() <= elem.ownerpartdisplaymode
                                                         ? nullptr
                                                         : aSymbol[elem.ownerpartdisplaymode];
    SCH_SYMBOL* schSymbol = nullptr;

    if( !symbol )
    {
        const auto& libSymbolIt = m_libSymbols.find( elem.ownerindex );

        if( libSymbolIt == m_libSymbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            m_reporter->Report( wxString::Format( wxT( "Pin's owner (%d) not found." ),
                                                  elem.ownerindex ),
                                RPT_SEVERITY_DEBUG );
            return;
        }

        if( !IsComponentPartVisible( elem ) )
            return;

        schSymbol = m_symbols.at( libSymbolIt->first );
        symbol = libSymbolIt->second;
    }

    SCH_PIN* pin = new SCH_PIN( symbol );

    // Make sure that these are visible when initializing the symbol
    // This may be overriden by the file data but not by the pin defaults
    pin->SetNameTextSize( schIUScale.MilsToIU( DEFAULT_PINNAME_SIZE ) );
    pin->SetNumberTextSize( schIUScale.MilsToIU( DEFAULT_PINNUM_SIZE ) );

    symbol->AddDrawItem( pin, false );

    pin->SetUnit( std::max( 0, elem.ownerpartid ) );

    pin->SetName( AltiumPinNamesToKiCad( elem.name ) );
    pin->SetNumber( elem.designator );
    pin->SetLength( elem.pinlength );

    if( elem.hidden )
        pin->SetVisible( false );

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

    if( schSymbol )
        pinLocation = GetRelativePosition( pinLocation + m_sheetOffset, schSymbol );

    pin->SetPosition( pinLocation );

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

    if( elem.symbolOuterEdge == ASCH_PIN_SYMBOL::UNKNOWN )
        m_reporter->Report( _( "Pin has unexpected outer edge type." ), RPT_SEVERITY_WARNING );

    if( elem.symbolInnerEdge == ASCH_PIN_SYMBOL::UNKNOWN )
        m_reporter->Report( _( "Pin has unexpected inner edge type." ), RPT_SEVERITY_WARNING );

    if( elem.symbolOuterEdge == ASCH_PIN_SYMBOL::NEGATED )
    {
        switch( elem.symbolInnerEdge )
        {
        case ASCH_PIN_SYMBOL::CLOCK:
            pin->SetShape( GRAPHIC_PINSHAPE::INVERTED_CLOCK );
            break;

        default:
            pin->SetShape( GRAPHIC_PINSHAPE::INVERTED );
            break;
        }
    }
    else if( elem.symbolOuterEdge == ASCH_PIN_SYMBOL::LOW_INPUT )
    {
        switch( elem.symbolInnerEdge )
        {
        case ASCH_PIN_SYMBOL::CLOCK:
            pin->SetShape( GRAPHIC_PINSHAPE::CLOCK_LOW );
            break;

        default:
            pin->SetShape( GRAPHIC_PINSHAPE::INPUT_LOW );
            break;
        }
    }
    else if( elem.symbolOuterEdge == ASCH_PIN_SYMBOL::LOW_OUTPUT )
    {
        pin->SetShape( GRAPHIC_PINSHAPE::OUTPUT_LOW );
    }
    else
    {
        switch( elem.symbolInnerEdge )
        {
        case ASCH_PIN_SYMBOL::CLOCK:
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
        hjustify *= -1;
        angle = ANGLE_HORIZONTAL;
        break;

    case ASCH_RECORD_ORIENTATION::UPWARDS:
        angle = ANGLE_VERTICAL;
        break;

    case ASCH_RECORD_ORIENTATION::DOWNWARDS:
        hjustify *= -1;
        angle = ANGLE_VERTICAL;
        break;
    }

    text->SetVertJustify( static_cast<GR_TEXT_V_ALIGN_T>( vjustify ) );
    text->SetHorizJustify( static_cast<GR_TEXT_H_ALIGN_T>( hjustify ) );
    text->SetTextAngle( angle );
}


bool SCH_IO_ALTIUM::ShouldPutItemOnSheet( int aOwnerindex )
{
    // No component assigned -> Put on sheet
    if( aOwnerindex == ALTIUM_COMPONENT_NONE )
        return true;

    // For a template -> Put on sheet so we can resolve variables
    if( m_altiumTemplates.find( aOwnerindex ) != m_altiumTemplates.end() )
        return true;

    return false;
}


void SCH_IO_ALTIUM::ParseLabel( const std::map<wxString, wxString>& aProperties,
                                std::vector<LIB_SYMBOL*>& aSymbol, std::vector<int>& aFontSizes )
{
    ASCH_LABEL elem( aProperties );

    if( aSymbol.empty() && ShouldPutItemOnSheet( elem.ownerindex ) )
    {
        static const std::map<wxString, wxString> variableMap = {
            { "APPLICATION_BUILDNUMBER", "KICAD_VERSION" },
            { "SHEETNUMBER",             "#"             },
            { "SHEETTOTAL",              "##"            },
            { "TITLE",                   "TITLE"         },  // including 1:1 maps makes it easier
            { "REVISION",                "REVISION"      },  //   to see that the list is complete
            { "DATE",                    "ISSUE_DATE"    },
            { "CURRENTDATE",             "CURRENT_DATE"  },
            { "COMPANYNAME",             "COMPANY"       },
            { "DOCUMENTNAME",            "FILENAME"      },
            { "DOCUMENTFULLPATHANDNAME", "FILEPATH"      },
            { "PROJECTNAME",             "PROJECTNAME"   },
        };

        wxString  kicadText = AltiumSchSpecialStringsToKiCadVariables( elem.text, variableMap );
        SCH_TEXT* textItem = new SCH_TEXT( elem.location + m_sheetOffset, kicadText );

        SetTextPositioning( textItem, elem.justification, elem.orientation );

        size_t fontId = static_cast<int>( elem.fontId );

        if( m_altiumSheet && fontId > 0 && fontId <= m_altiumSheet->fonts.size() )
        {
            const ASCH_SHEET_FONT& font = m_altiumSheet->fonts.at( fontId - 1 );
            textItem->SetTextSize( { font.Size / 2, font.Size / 2 } );

            // Must come after SetTextSize()
            textItem->SetBold( font.Bold );
            textItem->SetItalic( font.Italic );
        }

        textItem->SetFlags(IS_NEW );

        SCH_SCREEN* screen = getCurrentScreen();
        wxCHECK( screen, /* void */ );

        screen->Append( textItem );
    }
    else
    {
        LIB_SYMBOL* symbol = (int) aSymbol.size() <= elem.ownerpartdisplaymode
                                                            ? nullptr
                                                            : aSymbol[elem.ownerpartdisplaymode];
        SCH_SYMBOL* schsym = nullptr;

        if( !symbol )
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

            symbol = libSymbolIt->second;
            schsym = m_symbols.at( libSymbolIt->first );
        }

        VECTOR2I  pos = elem.location;
        SCH_TEXT* textItem = new SCH_TEXT( { 0, 0 }, elem.text, LAYER_DEVICE );
        symbol->AddDrawItem( textItem, false );

        /// Handle labels that are in a library symbol, not on schematic
        if( schsym )
            pos = GetRelativePosition( elem.location + m_sheetOffset, schsym );

        textItem->SetPosition( pos );
        textItem->SetUnit( std::max( 0, elem.ownerpartid ) );
        SetTextPositioning( textItem, elem.justification, elem.orientation );

        size_t fontId = elem.fontId;

        if( m_altiumSheet && fontId > 0 && fontId <= m_altiumSheet->fonts.size() )
        {
            const ASCH_SHEET_FONT& font = m_altiumSheet->fonts.at( fontId - 1 );
            textItem->SetTextSize( { font.Size / 2, font.Size / 2 } );

            // Must come after SetTextSize()
            textItem->SetBold( font.Bold );
            textItem->SetItalic( font.Italic );
        }
        else if( fontId > 0 && fontId <= aFontSizes.size() )
        {
            int size = aFontSizes[fontId - 1];
            textItem->SetTextSize( { size, size } );
        }
    }
}


void SCH_IO_ALTIUM::ParseTextFrame( const std::map<wxString, wxString>& aProperties,
                                    std::vector<LIB_SYMBOL*>& aSymbol,
                                    std::vector<int>& aFontSizes )
{
    ASCH_TEXT_FRAME elem( aProperties );

    if( aSymbol.empty() && ShouldPutItemOnSheet( elem.ownerindex ) )
        AddTextBox( &elem );
    else
        AddLibTextBox( &elem, aSymbol, aFontSizes );
}


void SCH_IO_ALTIUM::ParseNote( const std::map<wxString, wxString>& aProperties )
{
    ASCH_NOTE elem( aProperties );
    AddTextBox( static_cast<ASCH_TEXT_FRAME*>( &elem ) );

    // TODO: need some sort of property system for storing author....
}


void SCH_IO_ALTIUM::AddTextBox( const ASCH_TEXT_FRAME *aElem )
{
    SCH_TEXTBOX* textBox = new SCH_TEXTBOX();

    VECTOR2I sheetTopRight = aElem->TopRight + m_sheetOffset;
    VECTOR2I sheetBottomLeft = aElem->BottomLeft +m_sheetOffset;

    textBox->SetStart( sheetTopRight );
    textBox->SetEnd( sheetBottomLeft );

    textBox->SetText( aElem->Text );

    textBox->SetFillColor( GetColorFromInt( aElem->AreaColor ) );

    if( aElem->isSolid)
        textBox->SetFillMode( FILL_T::FILLED_WITH_COLOR );
    else
        textBox->SetFilled( false );

    if( aElem->ShowBorder )
        textBox->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::DEFAULT,
                                           GetColorFromInt( aElem->BorderColor ) ) );
    else
        textBox->SetStroke( STROKE_PARAMS( -1, LINE_STYLE::DEFAULT,
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

    size_t fontId = static_cast<int>( aElem->FontID );

    if( m_altiumSheet && fontId > 0 && fontId <= m_altiumSheet->fonts.size() )
    {
        const ASCH_SHEET_FONT& font = m_altiumSheet->fonts.at( fontId - 1 );
        textBox->SetTextSize( { font.Size / 2, font.Size / 2 } );

        // Must come after SetTextSize()
        textBox->SetBold( font.Bold );
        textBox->SetItalic( font.Italic );
        //textBox->SetFont(  //how to set font, we have a font name here: ( font.fontname );
    }

    textBox->SetFlags( IS_NEW );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    screen->Append( textBox );
}


void SCH_IO_ALTIUM::AddLibTextBox( const ASCH_TEXT_FRAME *aElem, std::vector<LIB_SYMBOL*>& aSymbol,
                                   std::vector<int>& aFontSizes )
{
    LIB_SYMBOL* symbol = static_cast<int>( aSymbol.size() ) <= aElem->ownerpartdisplaymode
                                 ? nullptr
                                 : aSymbol[aElem->ownerpartdisplaymode];
    SCH_SYMBOL* schsym = nullptr;

    if( !symbol )
    {
        const auto& libSymbolIt = m_libSymbols.find( aElem->ownerindex );

        if( libSymbolIt == m_libSymbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            m_reporter->Report(
                    wxString::Format( wxT( "Label's owner (%d) not found." ), aElem->ownerindex ),
                    RPT_SEVERITY_DEBUG );
            return;
        }

        symbol = libSymbolIt->second;
        schsym = m_symbols.at( libSymbolIt->first );
    }

    SCH_TEXTBOX* textBox = new SCH_TEXTBOX( LAYER_DEVICE );

    textBox->SetUnit( std::max( 0, aElem->ownerpartid ) );
    symbol->AddDrawItem( textBox, false );

    /// Handle text frames that are in a library symbol, not on schematic
    if( !schsym )
    {
        textBox->SetStart( aElem->TopRight );
        textBox->SetEnd( aElem->BottomLeft );
    }
    else
    {
        textBox->SetStart( GetRelativePosition( aElem->TopRight + m_sheetOffset, schsym ) );
        textBox->SetEnd( GetRelativePosition( aElem->BottomLeft + m_sheetOffset, schsym ) );
    }

    textBox->SetText( aElem->Text );

    textBox->SetFillColor( GetColorFromInt( aElem->AreaColor ) );

    if( aElem->isSolid)
        textBox->SetFillMode( FILL_T::FILLED_WITH_COLOR );
    else
        textBox->SetFilled( false );

    if( aElem->ShowBorder )
        textBox->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::DEFAULT,
                                           GetColorFromInt( aElem->BorderColor ) ) );
    else
        textBox->SetStroke( STROKE_PARAMS( -1 ) );

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

    if( aElem->FontID > 0 && aElem->FontID <= static_cast<int>( aFontSizes.size() ) )
    {
        int size = aFontSizes[aElem->FontID - 1];
        textBox->SetTextSize( { size, size } );
    }
}


void SCH_IO_ALTIUM::ParseBezier( const std::map<wxString, wxString>& aProperties,
                                 std::vector<LIB_SYMBOL*>&           aSymbol )
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

    if( aSymbol.empty() && ShouldPutItemOnSheet( elem.ownerindex ) )
    {
        SCH_SCREEN* currentScreen = getCurrentScreen();
        wxCHECK( currentScreen, /* void */ );

        for( size_t i = 0; i + 1 < elem.points.size(); i += 3 )
        {
            if( i + 2 == elem.points.size() )
            {
                // special case: single line
                SCH_LINE* line = new SCH_LINE( elem.points.at( i ) + m_sheetOffset,
                                               SCH_LAYER_ID::LAYER_NOTES );

                line->SetEndPoint( elem.points.at( i + 1 ) + m_sheetOffset );
                line->SetStroke( STROKE_PARAMS( elem.LineWidth, LINE_STYLE::SOLID ) );

                line->SetFlags( IS_NEW );

                currentScreen->Append( line );
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
                    line->SetStroke( STROKE_PARAMS( elem.LineWidth, LINE_STYLE::SOLID ) );

                    line->SetFlags( IS_NEW );
                    currentScreen->Append( line );
                }
            }
        }
    }
    else
    {
        LIB_SYMBOL* symbol = (int) aSymbol.size() <= elem.ownerpartdisplaymode
                                                            ? nullptr
                                                            : aSymbol[elem.ownerpartdisplaymode];
        SCH_SYMBOL* schsym = nullptr;

        if( !symbol )
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

            symbol = libSymbolIt->second;
            schsym = m_symbols.at( libSymbolIt->first );
        }

        if( aSymbol.empty() && !IsComponentPartVisible( elem ) )
            return;

        for( size_t i = 0; i + 1 < elem.points.size(); i += 3 )
        {
            if( i + 2 == elem.points.size() )
            {
                // special case: single line
                SCH_SHAPE* line = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
                symbol->AddDrawItem( line, false );

                line->SetUnit( std::max( 0, elem.ownerpartid ) );

                for( size_t j = i; j < elem.points.size() && j < i + 2; j++ )
                {
                    VECTOR2I pos = elem.points.at( j );

                    if( schsym )
                        pos = GetRelativePosition( pos + m_sheetOffset, schsym );

                    line->AddPoint( pos );
                }

                line->SetStroke( STROKE_PARAMS( elem.LineWidth, LINE_STYLE::SOLID ) );
            }
            else if( i + 3 == elem.points.size() )
            {
                // TODO: special case of a single line with an extra point?
                // I haven't a clue what this is all about, but the sample document we have in
                // https://gitlab.com/kicad/code/kicad/-/issues/8974 responds best by treating it
                // as another single line special case.
                SCH_SHAPE* line = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
                symbol->AddDrawItem( line, false );

                line->SetUnit( std::max( 0, elem.ownerpartid ) );

                for( size_t j = i; j < elem.points.size() && j < i + 2; j++ )
                {
                    VECTOR2I pos = elem.points.at( j );

                    if( schsym )
                        pos = GetRelativePosition( pos + m_sheetOffset, schsym );

                    line->AddPoint( pos );
                }

                line->SetStroke( STROKE_PARAMS( elem.LineWidth, LINE_STYLE::SOLID ) );
            }
            else
            {
                // Bezier always has exactly 4 control points
                SCH_SHAPE* bezier = new SCH_SHAPE( SHAPE_T::BEZIER, LAYER_DEVICE );
                symbol->AddDrawItem( bezier, false );

                bezier->SetUnit( std::max( 0, elem.ownerpartid ) );

                for( size_t j = i; j < elem.points.size() && j < i + 4; j++ )
                {
                    VECTOR2I pos = elem.points.at( j );

                    if( schsym )
                        pos = GetRelativePosition( pos + m_sheetOffset, schsym );

                    switch( j - i )
                    {
                    case 0: bezier->SetStart( pos );    break;
                    case 1: bezier->SetBezierC1( pos ); break;
                    case 2: bezier->SetBezierC2( pos ); break;
                    case 3: bezier->SetEnd( pos );      break;
                    default: break; // Can't get here but silence warnings
                    }
                }

                bezier->SetStroke( STROKE_PARAMS( elem.LineWidth, LINE_STYLE::SOLID ) );
                bezier->RebuildBezierToSegmentsPointsList( bezier->GetWidth() / 2 );
            }
        }
    }
}


void SCH_IO_ALTIUM::ParsePolyline( const std::map<wxString, wxString>& aProperties,
                                   std::vector<LIB_SYMBOL*>&           aSymbol )
{
    ASCH_POLYLINE elem( aProperties );

    if( elem.Points.size() < 2 )
        return;

    if( aSymbol.empty() && ShouldPutItemOnSheet( elem.ownerindex ) )
    {
        SCH_SCREEN* screen = getCurrentScreen();
        wxCHECK( screen, /* void */ );

        for( size_t i = 1; i < elem.Points.size(); i++ )
        {
            SCH_LINE* line = new SCH_LINE;

            line->SetStartPoint( elem.Points[i - 1] + m_sheetOffset );
            line->SetEndPoint( elem.Points[i] + m_sheetOffset );

            line->SetStroke( STROKE_PARAMS( elem.LineWidth, GetPlotDashType( elem.LineStyle ),
                                            GetColorFromInt( elem.Color ) ) );

            line->SetFlags( IS_NEW );

            screen->Append( line );
        }
    }
    else
    {
        LIB_SYMBOL* symbol = (int) aSymbol.size() <= elem.ownerpartdisplaymode
                                                             ? nullptr
                                                             : aSymbol[elem.ownerpartdisplaymode];
        SCH_SYMBOL* schsym = nullptr;

        if( !symbol )
        {
            const auto& libSymbolIt = m_libSymbols.find( elem.ownerindex );

            if( libSymbolIt == m_libSymbols.end() )
            {
                // TODO: e.g. can depend on Template (RECORD=39
                m_reporter->Report( wxString::Format( wxT( "Polyline's owner (%d) not found." ),
                                                      elem.ownerindex ),
                                    RPT_SEVERITY_DEBUG );
                return;
            }

            symbol = libSymbolIt->second;
            schsym = m_symbols.at( libSymbolIt->first );
        }

        if( aSymbol.empty() && !IsComponentPartVisible( elem ) )
            return;

        SCH_SHAPE*  line = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        symbol->AddDrawItem( line, false );

        line->SetUnit( std::max( 0, elem.ownerpartid ) );

        for( VECTOR2I point : elem.Points )
        {
            if( schsym )
                point = GetRelativePosition( point + m_sheetOffset, schsym );

            line->AddPoint( point );
        }

        SetLibShapeLine( elem, line, ALTIUM_SCH_RECORD::POLYLINE );
        STROKE_PARAMS stroke = line->GetStroke();
        stroke.SetLineStyle( GetPlotDashType( elem.LineStyle ) );

        line->SetStroke( stroke );
    }
}


void SCH_IO_ALTIUM::ParsePolygon( const std::map<wxString, wxString>& aProperties,
                                  std::vector<LIB_SYMBOL*>& aSymbol )
{
    ASCH_POLYGON elem( aProperties );

    if( aSymbol.empty() && ShouldPutItemOnSheet( elem.ownerindex ) )
    {
        SCH_SCREEN* screen = getCurrentScreen();
        wxCHECK( screen, /* void */ );

        SCH_SHAPE* poly = new SCH_SHAPE( SHAPE_T::POLY );

        for( VECTOR2I& point : elem.points )
            poly->AddPoint( point + m_sheetOffset );
        poly->AddPoint( elem.points.front() + m_sheetOffset );

        SetSchShapeLine( elem, poly );
        SetSchShapeFillAndColor( elem, poly );
        poly->SetFlags( IS_NEW );

        screen->Append( poly );
    }
    else
    {
        LIB_SYMBOL* symbol = (int) aSymbol.size() <= elem.ownerpartdisplaymode
                                                            ? nullptr
                                                            : aSymbol[elem.ownerpartdisplaymode];
        SCH_SYMBOL* schsym = nullptr;

        if( !symbol )
        {
            const auto& libSymbolIt = m_libSymbols.find( elem.ownerindex );

            if( libSymbolIt == m_libSymbols.end() )
            {
                // TODO: e.g. can depend on Template (RECORD=39
                m_reporter->Report( wxString::Format( wxT( "Polygon's owner (%d) not found." ),
                                                      elem.ownerindex ),
                                    RPT_SEVERITY_DEBUG );
                return;
            }

            symbol = libSymbolIt->second;
            schsym = m_symbols.at( libSymbolIt->first );
        }

        if( aSymbol.empty() && !IsComponentPartVisible( elem ) )
            return;

        SCH_SHAPE* line = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

        symbol->AddDrawItem( line, false );
        line->SetUnit( std::max( 0, elem.ownerpartid ) );

        for( VECTOR2I point : elem.points )
        {
            if( schsym )
                point = GetRelativePosition( point + m_sheetOffset, schsym );

            line->AddPoint( point );
        }

        VECTOR2I point = elem.points.front();

        if( schsym )
            point = GetRelativePosition( elem.points.front() + m_sheetOffset, schsym );

        line->AddPoint( point );

        SetLibShapeLine( elem, line, ALTIUM_SCH_RECORD::POLYGON );
        SetLibShapeFillAndColor( elem, line, ALTIUM_SCH_RECORD::POLYGON, elem.Color );

        if( line->GetFillColor() == line->GetStroke().GetColor()
          && line->GetFillMode() != FILL_T::NO_FILL )
        {
            STROKE_PARAMS stroke = line->GetStroke();
            stroke.SetWidth( -1 );
            line->SetStroke( stroke );
        }
    }
}


void SCH_IO_ALTIUM::ParseRoundRectangle( const std::map<wxString, wxString>& aProperties,
                                         std::vector<LIB_SYMBOL*>& aSymbol )
{
    ASCH_ROUND_RECTANGLE elem( aProperties );

    if( aSymbol.empty() && ShouldPutItemOnSheet( elem.ownerindex ) )
    {
        SCH_SCREEN* screen = getCurrentScreen();
        wxCHECK( screen, /* void */ );

        // TODO: misses rounded edges
        SCH_SHAPE* rect = new SCH_SHAPE( SHAPE_T::RECTANGLE );

        rect->SetPosition( elem.TopRight + m_sheetOffset );
        rect->SetEnd( elem.BottomLeft + m_sheetOffset );
        SetSchShapeLine( elem, rect );
        SetSchShapeFillAndColor( elem, rect );
        rect->SetFlags( IS_NEW );

        screen->Append( rect );
    }
    else
    {
        LIB_SYMBOL* symbol = (int) aSymbol.size() <= elem.ownerpartdisplaymode
                                                            ? nullptr
                                                            : aSymbol[elem.ownerpartdisplaymode];
        SCH_SYMBOL* schsym = nullptr;

        if( !symbol )
        {
            const auto& libSymbolIt = m_libSymbols.find( elem.ownerindex );

            if( libSymbolIt == m_libSymbols.end() )
            {
                // TODO: e.g. can depend on Template (RECORD=39
                m_reporter->Report( wxString::Format( wxT( "Rounded rectangle's owner (%d) not "
                                                           "found." ),
                                                      elem.ownerindex ),
                                    RPT_SEVERITY_DEBUG );
                return;
            }

            symbol = libSymbolIt->second;
            schsym = m_symbols.at( libSymbolIt->first );
        }

        if( aSymbol.empty() && !IsComponentPartVisible( elem ) )
            return;

        SCH_SHAPE* rect = nullptr;

        int width = std::abs( elem.TopRight.x - elem.BottomLeft.x );
        int height = std::abs( elem.TopRight.y - elem.BottomLeft.y );

        // If it is a circle, make it a circle
        if( std::abs( elem.CornerRadius.x ) >= width / 2
            && std::abs( elem.CornerRadius.y ) >= height / 2 )
        {
            rect = new SCH_SHAPE( SHAPE_T::CIRCLE, LAYER_DEVICE );

            VECTOR2I center = ( elem.TopRight + elem.BottomLeft ) / 2;
            int      radius = std::min( width / 2, height / 2 );

            if( schsym )
                center = GetRelativePosition( center + m_sheetOffset, schsym );

            rect->SetPosition( center );
            rect->SetEnd( VECTOR2I( rect->GetPosition().x + radius, rect->GetPosition().y ) );
        }
        else
        {
            rect = new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE );

            if( !schsym )
            {
                rect->SetPosition( elem.TopRight );
                rect->SetEnd( elem.BottomLeft );
            }
            else
            {
                rect->SetPosition( GetRelativePosition( elem.TopRight + m_sheetOffset, schsym ) );
                rect->SetEnd( GetRelativePosition( elem.BottomLeft + m_sheetOffset, schsym ) );
            }

            rect->Normalize();
        }

        SetLibShapeLine( elem, rect, ALTIUM_SCH_RECORD::ROUND_RECTANGLE );
        SetLibShapeFillAndColor( elem, rect, ALTIUM_SCH_RECORD::ROUND_RECTANGLE, elem.Color );

        symbol->AddDrawItem( rect, false );
        rect->SetUnit( std::max( 0, elem.ownerpartid ) );
    }
}


void SCH_IO_ALTIUM::ParseArc( const std::map<wxString, wxString>& aProperties,
                              std::vector<LIB_SYMBOL*>& aSymbol )
{
    ASCH_ARC elem( aProperties );

    int       arc_radius = elem.m_Radius;
    VECTOR2I  center = elem.m_Center;
    EDA_ANGLE startAngle( elem.m_EndAngle, DEGREES_T );
    EDA_ANGLE endAngle( elem.m_StartAngle, DEGREES_T );
    VECTOR2I  startOffset( KiROUND( arc_radius * startAngle.Cos() ),
                           -KiROUND( arc_radius * startAngle.Sin() ) );
    VECTOR2I  endOffset( KiROUND( arc_radius * endAngle.Cos() ),
                         -KiROUND( arc_radius * endAngle.Sin() ) );

    if( aSymbol.empty() && ShouldPutItemOnSheet( elem.ownerindex ) )
    {
        SCH_SCREEN* currentScreen = getCurrentScreen();
        wxCHECK( currentScreen, /* void */ );

        if( elem.m_StartAngle == 0 && ( elem.m_EndAngle == 0 || elem.m_EndAngle == 360 ) )
        {
            SCH_SHAPE* circle = new SCH_SHAPE( SHAPE_T::CIRCLE );

            circle->SetPosition( elem.m_Center + m_sheetOffset );
            circle->SetEnd( circle->GetPosition() + VECTOR2I( arc_radius, 0 ) );

            SetSchShapeLine( elem, circle );
            SetSchShapeFillAndColor( elem, circle );

            currentScreen->Append( circle );
        }
        else
        {
            SCH_SHAPE* arc = new SCH_SHAPE( SHAPE_T::ARC );

            arc->SetCenter( elem.m_Center + m_sheetOffset );
            arc->SetStart( elem.m_Center + startOffset + m_sheetOffset );
            arc->SetEnd( elem.m_Center + endOffset + m_sheetOffset );

            SetSchShapeLine( elem, arc );
            SetSchShapeFillAndColor( elem, arc );

            currentScreen->Append( arc );
        }
    }
    else
    {
        LIB_SYMBOL* symbol = (int) aSymbol.size() <= elem.ownerpartdisplaymode
                                                             ? nullptr
                                                             : aSymbol[elem.ownerpartdisplaymode];
        SCH_SYMBOL* schsym = nullptr;

        if( !symbol )
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

            symbol = libSymbolIt->second;
            schsym = m_symbols.at( libSymbolIt->first );
        }

        if( aSymbol.empty() && !IsComponentPartVisible( elem ) )
            return;

        if( elem.m_StartAngle == 0 && ( elem.m_EndAngle == 0 || elem.m_EndAngle == 360 ) )
        {
            SCH_SHAPE* circle = new SCH_SHAPE( SHAPE_T::CIRCLE, LAYER_DEVICE );
            symbol->AddDrawItem( circle, false );

            circle->SetUnit( std::max( 0, elem.ownerpartid ) );

            if( schsym )
                center = GetRelativePosition( center + m_sheetOffset, schsym );

            circle->SetPosition( center );

            circle->SetEnd( circle->GetPosition() + VECTOR2I( arc_radius, 0 ) );
            SetLibShapeLine( elem, circle, ALTIUM_SCH_RECORD::ARC );
            SetLibShapeFillAndColor( elem, circle, ALTIUM_SCH_RECORD::ARC, elem.Color );
        }
        else
        {
            SCH_SHAPE* arc = new SCH_SHAPE( SHAPE_T::ARC, LAYER_DEVICE );
            symbol->AddDrawItem( arc, false );
            arc->SetUnit( std::max( 0, elem.ownerpartid ) );

            if( schsym )
                center = GetRelativePosition( elem.m_Center + m_sheetOffset, schsym );

            arc->SetCenter( center );
            arc->SetStart( center + startOffset );
            arc->SetEnd( center + endOffset );

            SetLibShapeLine( elem, arc, ALTIUM_SCH_RECORD::ARC );
            SetLibShapeFillAndColor( elem, arc, ALTIUM_SCH_RECORD::ARC, elem.Color );
        }
    }
}


void SCH_IO_ALTIUM::ParseEllipticalArc( const std::map<wxString, wxString>& aProperties,
                                        std::vector<LIB_SYMBOL*>& aSymbol )
{
    ASCH_ARC elem( aProperties );

    if( elem.m_Radius == elem.m_SecondaryRadius && elem.m_StartAngle == 0
        && ( elem.m_EndAngle == 0 || elem.m_EndAngle == 360 ) )
    {
        ParseCircle( aProperties, aSymbol );
        return;
    }

    if( aSymbol.empty() && ShouldPutItemOnSheet( elem.ownerindex ) )
    {
        SCH_SCREEN* currentScreen = getCurrentScreen();
        wxCHECK( currentScreen, /* void */ );

        ELLIPSE<int>             ellipse( elem.m_Center + m_sheetOffset, elem.m_Radius,
                                          KiROUND( elem.m_SecondaryRadius ), ANGLE_0,
                                          EDA_ANGLE( elem.m_StartAngle, DEGREES_T ),
                                          EDA_ANGLE( elem.m_EndAngle, DEGREES_T ) );
        std::vector<BEZIER<int>> beziers;

        TransformEllipseToBeziers( ellipse, beziers );

        for( const BEZIER<int>& bezier : beziers )
        {
            SCH_SHAPE* schbezier = new SCH_SHAPE( SHAPE_T::BEZIER );
            schbezier->SetStart( bezier.Start );
            schbezier->SetBezierC1( bezier.C1 );
            schbezier->SetBezierC2( bezier.C2 );
            schbezier->SetEnd( bezier.End );
            schbezier->SetStroke( STROKE_PARAMS( elem.LineWidth, LINE_STYLE::SOLID ) );
            schbezier->RebuildBezierToSegmentsPointsList( elem.LineWidth / 2 );

            currentScreen->Append( schbezier );
        }
    }
    else
    {
        LIB_SYMBOL* symbol = (int) aSymbol.size() <= elem.ownerpartdisplaymode
                                                             ? nullptr
                                                             : aSymbol[elem.ownerpartdisplaymode];
        SCH_SYMBOL* schsym = nullptr;

        if( !symbol )
        {
            const auto& libSymbolIt = m_libSymbols.find( elem.ownerindex );

            if( libSymbolIt == m_libSymbols.end() )
            {
                // TODO: e.g. can depend on Template (RECORD=39
                m_reporter->Report( wxString::Format( wxT( "Elliptical Arc's owner (%d) not found." ),
                                                      elem.ownerindex ),
                                    RPT_SEVERITY_DEBUG );
                return;
            }

            symbol = libSymbolIt->second;
            schsym = m_symbols.at( libSymbolIt->first );
        }

        if( aSymbol.empty() && !IsComponentPartVisible( elem ) )
            return;

        ELLIPSE<int>             ellipse( elem.m_Center, elem.m_Radius,
                                          KiROUND( elem.m_SecondaryRadius ), ANGLE_0,
                                          EDA_ANGLE( elem.m_StartAngle, DEGREES_T ),
                                          EDA_ANGLE( elem.m_EndAngle, DEGREES_T ) );
        std::vector<BEZIER<int>> beziers;

        TransformEllipseToBeziers( ellipse, beziers );

        for( const BEZIER<int>& bezier : beziers )
        {
            SCH_SHAPE* schbezier = new SCH_SHAPE( SHAPE_T::BEZIER, LAYER_DEVICE );
            symbol->AddDrawItem( schbezier, false );

            schbezier->SetUnit( std::max( 0, elem.ownerpartid ) );

            if( schsym )
            {
                schbezier->SetStart( GetRelativePosition( bezier.Start + m_sheetOffset, schsym ) );
                schbezier->SetBezierC1( GetRelativePosition( bezier.C1 + m_sheetOffset, schsym ) );
                schbezier->SetBezierC2( GetRelativePosition( bezier.C2 + m_sheetOffset, schsym ) );
                schbezier->SetEnd( GetRelativePosition( bezier.End + m_sheetOffset, schsym ) );
            }
            else
            {
                schbezier->SetStart( bezier.Start );
                schbezier->SetBezierC1( bezier.C1 );
                schbezier->SetBezierC2( bezier.C2 );
                schbezier->SetEnd( bezier.End );
            }

            SetLibShapeLine( elem, schbezier, ALTIUM_SCH_RECORD::ELLIPTICAL_ARC );
            schbezier->RebuildBezierToSegmentsPointsList( elem.LineWidth / 2 );
        }
    }
}


void SCH_IO_ALTIUM::ParsePieChart( const std::map<wxString, wxString>& aProperties,
                                  std::vector<LIB_SYMBOL*>& aSymbol )
{
    ParseArc( aProperties, aSymbol );

    ASCH_PIECHART elem( aProperties );

    int       arc_radius = elem.m_Radius;
    VECTOR2I  center = elem.m_Center;
    EDA_ANGLE startAngle( elem.m_EndAngle, DEGREES_T );
    EDA_ANGLE endAngle( elem.m_StartAngle, DEGREES_T );
    VECTOR2I  startOffset( KiROUND( arc_radius * startAngle.Cos() ),
                           -KiROUND( arc_radius * startAngle.Sin() ) );
    VECTOR2I  endOffset( KiROUND( arc_radius * endAngle.Cos() ),
                         -KiROUND( arc_radius * endAngle.Sin() ) );

    if( aSymbol.empty() && ShouldPutItemOnSheet( elem.ownerindex ) )
    {
        SCH_SCREEN* screen = getCurrentScreen();
        wxCHECK( screen, /* void */ );

        // close polygon
        SCH_LINE* line = new SCH_LINE( center + m_sheetOffset, SCH_LAYER_ID::LAYER_NOTES );
        line->SetEndPoint( center + startOffset + m_sheetOffset );
        line->SetStroke( STROKE_PARAMS( elem.LineWidth, LINE_STYLE::SOLID ) );

        line->SetFlags( IS_NEW );
        screen->Append( line );

        line = new SCH_LINE( center + m_sheetOffset, SCH_LAYER_ID::LAYER_NOTES );
        line->SetEndPoint( center + endOffset + m_sheetOffset );
        line->SetStroke( STROKE_PARAMS( elem.LineWidth, LINE_STYLE::SOLID ) );

        line->SetFlags( IS_NEW );
        screen->Append( line );
    }
    else
    {
        LIB_SYMBOL* symbol = (int) aSymbol.size() <= elem.ownerpartdisplaymode
                                                             ? nullptr
                                                             : aSymbol[elem.ownerpartdisplaymode];
        SCH_SYMBOL* schsym = nullptr;

        if( !symbol )
        {
            const auto& libSymbolIt = m_libSymbols.find( elem.ownerindex );

            if( libSymbolIt == m_libSymbols.end() )
            {
                // TODO: e.g. can depend on Template (RECORD=39
                m_reporter->Report( wxString::Format( wxT( "Piechart's owner (%d) not found." ),
                                                      elem.ownerindex ),
                                    RPT_SEVERITY_DEBUG );
                return;
            }

            symbol = libSymbolIt->second;
            schsym = m_symbols.at( libSymbolIt->first );
        }

        if( aSymbol.empty() && !IsComponentPartVisible( elem ) )
            return;

        SCH_SHAPE*  line = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        symbol->AddDrawItem( line, false );

        line->SetUnit( std::max( 0, elem.ownerpartid ) );

        if( !schsym )
        {
            line->AddPoint( center + startOffset );
            line->AddPoint( center );
            line->AddPoint( center + endOffset );
        }
        else
        {
            line->AddPoint( GetRelativePosition( center + startOffset + m_sheetOffset, schsym ) );
            line->AddPoint( GetRelativePosition( center + m_sheetOffset, schsym ) );
            line->AddPoint( GetRelativePosition( center + endOffset + m_sheetOffset, schsym ) );
        }

        SetLibShapeLine( elem, line, ALTIUM_SCH_RECORD::LINE );
    }
}


void SCH_IO_ALTIUM::ParseEllipse( const std::map<wxString, wxString>& aProperties,
                                  std::vector<LIB_SYMBOL*>& aSymbol )
{
    ASCH_ELLIPSE elem( aProperties );

    if( elem.Radius == elem.SecondaryRadius )
    {
        ParseCircle( aProperties, aSymbol );
        return;
    }

    if( aSymbol.empty() && ShouldPutItemOnSheet( elem.ownerindex ) )
    {
        SCH_SCREEN* screen = getCurrentScreen();
        wxCHECK( screen, /* void */ );

        COLOR4D fillColor = GetColorFromInt( elem.AreaColor );

        if( elem.IsTransparent )
            fillColor = fillColor.WithAlpha( 0.5 );

        FILL_T fillMode = elem.IsSolid ? FILL_T::FILLED_WITH_COLOR : FILL_T::NO_FILL;

        ELLIPSE<int> ellipse( elem.Center + m_sheetOffset, elem.Radius,
                              KiROUND( elem.SecondaryRadius ), ANGLE_0 );

        std::vector<BEZIER<int>> beziers;
        std::vector<VECTOR2I>    polyPoints;

        TransformEllipseToBeziers( ellipse, beziers );

        for( const BEZIER<int>& bezier : beziers )
        {
            SCH_SHAPE* schbezier = new SCH_SHAPE( SHAPE_T::BEZIER );
            schbezier->SetStart( bezier.Start );
            schbezier->SetBezierC1( bezier.C1 );
            schbezier->SetBezierC2( bezier.C2 );
            schbezier->SetEnd( bezier.End );
            schbezier->SetStroke( STROKE_PARAMS( elem.LineWidth, LINE_STYLE::SOLID ) );
            schbezier->SetFillColor( fillColor );
            schbezier->SetFillMode( fillMode );

            schbezier->RebuildBezierToSegmentsPointsList( schbezier->GetWidth() / 2 );
            screen->Append( schbezier );

            polyPoints.push_back( bezier.Start );
        }

        if( fillMode != FILL_T::NO_FILL )
        {
            SCH_SHAPE* schpoly = new SCH_SHAPE( SHAPE_T::POLY );
            schpoly->SetFillColor( fillColor );
            schpoly->SetFillMode( fillMode );
            schpoly->SetWidth( -1 );

            for( const VECTOR2I& point : polyPoints )
                schpoly->AddPoint( point );

            schpoly->AddPoint( polyPoints[0] );

            screen->Append( schpoly );
        }
    }
    else
    {
        LIB_SYMBOL* symbol = (int) aSymbol.size() <= elem.ownerpartdisplaymode
                                                             ? nullptr
                                                             : aSymbol[elem.ownerpartdisplaymode];
        SCH_SYMBOL* schsym = nullptr;

        if( !symbol )
        {
            const auto& libSymbolIt = m_libSymbols.find( elem.ownerindex );

            if( libSymbolIt == m_libSymbols.end() )
            {
                // TODO: e.g. can depend on Template (RECORD=39
                m_reporter->Report( wxString::Format( wxT( "Ellipse's owner (%d) not found." ),
                                                      elem.ownerindex ),
                                    RPT_SEVERITY_DEBUG );
                return;
            }

            symbol = libSymbolIt->second;
            schsym = m_symbols.at( libSymbolIt->first );
        }

        ELLIPSE<int> ellipse( elem.Center, elem.Radius, KiROUND( elem.SecondaryRadius ),
                              ANGLE_0 );

        std::vector<BEZIER<int>> beziers;
        std::vector<VECTOR2I>    polyPoints;

        TransformEllipseToBeziers( ellipse, beziers );

        for( const BEZIER<int>& bezier : beziers )
        {
            SCH_SHAPE* libbezier = new SCH_SHAPE( SHAPE_T::BEZIER, LAYER_DEVICE );
            symbol->AddDrawItem( libbezier, false );
            libbezier->SetUnit( std::max( 0, elem.ownerpartid ) );

            if( !schsym )
            {
                libbezier->SetStart( bezier.Start );
                libbezier->SetBezierC1( bezier.C1 );
                libbezier->SetBezierC2( bezier.C2 );
                libbezier->SetEnd( bezier.End );
            }
            else
            {
                libbezier->SetStart( GetRelativePosition( bezier.Start + m_sheetOffset, schsym ) );
                libbezier->SetBezierC1( GetRelativePosition( bezier.C1 + m_sheetOffset, schsym ) );
                libbezier->SetBezierC2( GetRelativePosition( bezier.C2 + m_sheetOffset, schsym ) );
                libbezier->SetEnd( GetRelativePosition( bezier.End + m_sheetOffset, schsym ) );
            }

            SetLibShapeLine( elem, libbezier, ALTIUM_SCH_RECORD::ELLIPSE );
            SetLibShapeFillAndColor( elem, libbezier, ALTIUM_SCH_RECORD::ELLIPSE, elem.Color );
            libbezier->RebuildBezierToSegmentsPointsList( libbezier->GetWidth() / 2 );

            polyPoints.push_back( libbezier->GetStart() );
        }

        // A series of beziers won't fill the center, so if this is meant to be fully filled,
        // Add a polygon to fill the center
        if( elem.IsSolid )
        {
            SCH_SHAPE* libline = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            symbol->AddDrawItem( libline, false );
            libline->SetUnit( std::max( 0, elem.ownerpartid ) );

            for( const VECTOR2I& point : polyPoints )
                libline->AddPoint( point );

            libline->AddPoint( polyPoints[0] );

            libline->SetWidth( -1 );
            SetLibShapeFillAndColor( elem, libline, ALTIUM_SCH_RECORD::ELLIPSE, elem.Color );
        }
    }
}


void SCH_IO_ALTIUM::ParseCircle( const std::map<wxString, wxString>& aProperties,
                                 std::vector<LIB_SYMBOL*>& aSymbol )
{
    ASCH_ELLIPSE elem( aProperties );

    if( aSymbol.empty() && ShouldPutItemOnSheet( elem.ownerindex ) )
    {
        SCH_SCREEN* screen = getCurrentScreen();
        wxCHECK( screen, /* void */ );

        SCH_SHAPE* circle = new SCH_SHAPE( SHAPE_T::CIRCLE );

        circle->SetPosition( elem.Center + m_sheetOffset );
        circle->SetEnd( circle->GetPosition() + VECTOR2I( elem.Radius, 0 ) );
        circle->SetStroke( STROKE_PARAMS( 1, LINE_STYLE::SOLID ) );

        circle->SetFillColor( GetColorFromInt( elem.AreaColor ) );

        if( elem.IsSolid )
            circle->SetFillMode( FILL_T::FILLED_WITH_COLOR );
        else
            circle->SetFilled( false );

        screen->Append( circle );
    }
    else
    {
        LIB_SYMBOL* symbol = (int) aSymbol.size() <= elem.ownerpartdisplaymode
                                                             ? nullptr
                                                             : aSymbol[elem.ownerpartdisplaymode];
        SCH_SYMBOL* schsym = nullptr;

        if( !symbol )
        {
            const auto& libSymbolIt = m_libSymbols.find( elem.ownerindex );

            if( libSymbolIt == m_libSymbols.end() )
            {
                // TODO: e.g. can depend on Template (RECORD=39
                m_reporter->Report( wxString::Format( wxT( "Ellipse's owner (%d) not found." ),
                                                      elem.ownerindex ),
                                    RPT_SEVERITY_DEBUG );
                return;
            }

            symbol = libSymbolIt->second;
            schsym = m_symbols.at( libSymbolIt->first );
        }

        VECTOR2I   center = elem.Center;
        SCH_SHAPE* circle = new SCH_SHAPE( SHAPE_T::CIRCLE, LAYER_DEVICE );
        symbol->AddDrawItem( circle, false );

        circle->SetUnit( std::max( 0, elem.ownerpartid ) );

        if( schsym )
            center = GetRelativePosition( center + m_sheetOffset, schsym );

        circle->SetPosition( center );
        circle->SetEnd( circle->GetPosition() + VECTOR2I( elem.Radius, 0 ) );

        SetLibShapeLine( elem, circle, ALTIUM_SCH_RECORD::ELLIPSE );
        SetLibShapeFillAndColor( elem, circle, ALTIUM_SCH_RECORD::ELLIPSE, elem.Color );
    }
}


void SCH_IO_ALTIUM::ParseLine( const std::map<wxString, wxString>& aProperties,
                               std::vector<LIB_SYMBOL*>& aSymbol )
{
    ASCH_LINE elem( aProperties );

    if( aSymbol.empty() && ShouldPutItemOnSheet( elem.ownerindex ) )
    {

        SCH_SCREEN* screen = getCurrentScreen();
        wxCHECK( screen, /* void */ );

        // close polygon
        SCH_LINE* line = new SCH_LINE( elem.point1 + m_sheetOffset, SCH_LAYER_ID::LAYER_NOTES );
        line->SetEndPoint( elem.point2 + m_sheetOffset );
        line->SetStroke( STROKE_PARAMS( elem.LineWidth, GetPlotDashType( elem.LineStyle ),
                                        GetColorFromInt( elem.Color ) ) );

        line->SetFlags( IS_NEW );
        screen->Append( line );
    }
    else
    {
        LIB_SYMBOL* symbol = (int) aSymbol.size() <= elem.ownerpartdisplaymode
                                                             ? nullptr
                                                             : aSymbol[elem.ownerpartdisplaymode];
        SCH_SYMBOL* schsym = nullptr;

        if( !symbol )
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

            symbol = libSymbolIt->second;
            schsym = m_symbols.at( libSymbolIt->first );
        }

        if( aSymbol.empty() && !IsComponentPartVisible( elem ) )
            return;

        SCH_SHAPE*  line = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        symbol->AddDrawItem( line, false );

        line->SetUnit( std::max( 0, elem.ownerpartid ) );

        if( !schsym )
        {
            line->AddPoint( elem.point1 );
            line->AddPoint( elem.point2 );
        }
        else
        {
            line->AddPoint( GetRelativePosition( elem.point1 + m_sheetOffset, schsym ) );
            line->AddPoint( GetRelativePosition( elem.point2 + m_sheetOffset, schsym ) );
        }

        SetLibShapeLine( elem, line, ALTIUM_SCH_RECORD::LINE );
        line->SetLineStyle( GetPlotDashType( elem.LineStyle ) );
    }
}


void SCH_IO_ALTIUM::ParseSignalHarness( const std::map<wxString, wxString>& aProperties )
{
    ASCH_SIGNAL_HARNESS elem( aProperties );

    if( ShouldPutItemOnSheet( elem.ownerindex ) )
    {
        SCH_SCREEN* screen = getCurrentScreen();
        wxCHECK( screen, /* void */ );

        SCH_SHAPE* poly = new SCH_SHAPE( SHAPE_T::POLY );

        for( VECTOR2I& point : elem.Points )
            poly->AddPoint( point + m_sheetOffset );

        poly->SetStroke( STROKE_PARAMS( elem.LineWidth, LINE_STYLE::SOLID,
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


void SCH_IO_ALTIUM::ParseHarnessConnector( int aIndex, const std::map<wxString,
                                           wxString>& aProperties )
{
    ASCH_HARNESS_CONNECTOR elem( aProperties );

    if( ShouldPutItemOnSheet( elem.ownerindex ) )
    {
        SCH_SCREEN* currentScreen = getCurrentScreen();
        wxCHECK( currentScreen, /* void */ );

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


void SCH_IO_ALTIUM::ParseHarnessEntry( const std::map<wxString, wxString>& aProperties )
{
    ASCH_HARNESS_ENTRY elem( aProperties );

    const auto& sheetIt = m_sheets.find( m_harnessEntryParent );

    if( sheetIt == m_sheets.end() )
    {
        m_reporter->Report( wxString::Format( wxT( "Harness entry's parent (%d) not found." ),
                                              SCH_IO_ALTIUM::m_harnessEntryParent ),
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
        sheetPin->SetSpinStyle( SPIN_STYLE::LEFT );
        sheetPin->SetSide( SHEET_SIDE::LEFT );
        break;
    case ASCH_SHEET_ENTRY_SIDE::RIGHT:
        sheetPin->SetPosition( { pos.x + size.x, pos.y + elem.DistanceFromTop } );
        sheetPin->SetSpinStyle( SPIN_STYLE::RIGHT );
        sheetPin->SetSide( SHEET_SIDE::RIGHT );
        break;
    case ASCH_SHEET_ENTRY_SIDE::TOP:
        sheetPin->SetPosition( { pos.x + elem.DistanceFromTop, pos.y } );
        sheetPin->SetSpinStyle( SPIN_STYLE::UP );
        sheetPin->SetSide( SHEET_SIDE::TOP );
        break;
    case ASCH_SHEET_ENTRY_SIDE::BOTTOM:
        sheetPin->SetPosition( { pos.x + elem.DistanceFromTop, pos.y + size.y } );
        sheetPin->SetSpinStyle( SPIN_STYLE::BOTTOM );
        sheetPin->SetSide( SHEET_SIDE::BOTTOM );
        break;
    }
}


void SCH_IO_ALTIUM::ParseHarnessType( const std::map<wxString, wxString>& aProperties )
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
    sheetFileName.SetText( elem.Text + wxT( "." ) + FILEEXT::KiCadSchematicFileExtension );

    wxFileName fn( m_schematic->Prj().GetProjectPath(), elem.Text,
                   FILEEXT::KiCadSchematicFileExtension );
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


void SCH_IO_ALTIUM::ParseRectangle( const std::map<wxString, wxString>& aProperties,
                                    std::vector<LIB_SYMBOL*>& aSymbol )
{
    ASCH_RECTANGLE elem( aProperties );

    VECTOR2I sheetTopRight = elem.TopRight + m_sheetOffset;
    VECTOR2I sheetBottomLeft = elem.BottomLeft + m_sheetOffset;

    if( aSymbol.empty() && ShouldPutItemOnSheet( elem.ownerindex ) )
    {
        SCH_SCREEN* screen = getCurrentScreen();
        wxCHECK( screen, /* void */ );

        SCH_SHAPE* rect = new SCH_SHAPE( SHAPE_T::RECTANGLE );

        rect->SetPosition( sheetTopRight );
        rect->SetEnd( sheetBottomLeft );
        SetSchShapeLine( elem, rect );
        SetSchShapeFillAndColor( elem, rect );
        rect->SetFlags( IS_NEW );

        screen->Append( rect );
    }
    else
    {
        LIB_SYMBOL* symbol = (int) aSymbol.size() <= elem.ownerpartdisplaymode
                                                             ? nullptr
                                                             : aSymbol[elem.ownerpartdisplaymode];
        SCH_SYMBOL* schsym = nullptr;

        if( !symbol )
        {
            const auto& libSymbolIt = m_libSymbols.find( elem.ownerindex );

            if( libSymbolIt == m_libSymbols.end() )
            {
                // TODO: e.g. can depend on Template (RECORD=39
                m_reporter->Report( wxString::Format( wxT( "Rectangle's owner (%d) not found." ),
                                                    elem.ownerindex ),
                                    RPT_SEVERITY_DEBUG );
                return;
            }

            symbol = libSymbolIt->second;
            schsym = m_symbols.at( libSymbolIt->first );
        }

        if( aSymbol.empty() && !IsComponentPartVisible( elem ) )
            return;

        SCH_SHAPE*  rect = new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE );
        symbol->AddDrawItem( rect, false );

        rect->SetUnit( std::max( 0, elem.ownerpartid ) );

        if( !schsym )
        {
            rect->SetPosition( sheetTopRight );
            rect->SetEnd( sheetBottomLeft );
        }
        else
        {
            rect->SetPosition( GetRelativePosition( sheetTopRight, schsym ) );
            rect->SetEnd( GetRelativePosition( sheetBottomLeft, schsym ) );
        }

        SetLibShapeLine( elem, rect, ALTIUM_SCH_RECORD::RECTANGLE );
        SetLibShapeFillAndColor( elem, rect, ALTIUM_SCH_RECORD::RECTANGLE, elem.Color );
    }
}


void SCH_IO_ALTIUM::ParseSheetSymbol( int aIndex, const std::map<wxString, wxString>& aProperties )
{
    ASCH_SHEET_SYMBOL elem( aProperties );

    SCH_SHEET* sheet = new SCH_SHEET( getCurrentSheet(), elem.location + m_sheetOffset, elem.size );

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


void SCH_IO_ALTIUM::ParseSheetEntry( const std::map<wxString, wxString>& aProperties )
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
    //sheetPin->SetSpinStyle( getSpinStyle( term.OrientAngle, false ) );
    //sheetPin->SetPosition( getKiCadPoint( term.Position ) );

    VECTOR2I pos = sheetIt->second->GetPosition();
    VECTOR2I size = sheetIt->second->GetSize();

    switch( elem.side )
    {
    default:
    case ASCH_SHEET_ENTRY_SIDE::LEFT:
        sheetPin->SetPosition( { pos.x, pos.y + elem.distanceFromTop } );
        sheetPin->SetSpinStyle( SPIN_STYLE::LEFT );
        sheetPin->SetSide( SHEET_SIDE::LEFT );
        break;

    case ASCH_SHEET_ENTRY_SIDE::RIGHT:
        sheetPin->SetPosition( { pos.x + size.x, pos.y + elem.distanceFromTop } );
        sheetPin->SetSpinStyle( SPIN_STYLE::RIGHT );
        sheetPin->SetSide( SHEET_SIDE::RIGHT );
        break;

    case ASCH_SHEET_ENTRY_SIDE::TOP:
        sheetPin->SetPosition( { pos.x + elem.distanceFromTop, pos.y } );
        sheetPin->SetSpinStyle( SPIN_STYLE::UP );
        sheetPin->SetSide( SHEET_SIDE::TOP );
        break;

    case ASCH_SHEET_ENTRY_SIDE::BOTTOM:
        sheetPin->SetPosition( { pos.x + elem.distanceFromTop, pos.y + size.y } );
        sheetPin->SetSpinStyle( SPIN_STYLE::BOTTOM );
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
        SCH_SHAPE* line1 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line1->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, schIUScale.MilsToIU( 50 ) } );
        aKsymbol->AddDrawItem( line1, false );

        if( aStyle == ASCH_POWER_PORT_STYLE::CIRCLE )
        {
            SCH_SHAPE* circle = new SCH_SHAPE( SHAPE_T::CIRCLE, LAYER_DEVICE );
            circle->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 5 ), LINE_STYLE::SOLID ) );
            circle->SetPosition( { schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( 75 ) } );
            circle->SetEnd( circle->GetPosition() + VECTOR2I( schIUScale.MilsToIU( 25 ), 0 ) );
            aKsymbol->AddDrawItem( circle, false );
        }
        else
        {
            SCH_SHAPE* line2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
            line2->AddPoint( { schIUScale.MilsToIU( -25 ), schIUScale.MilsToIU( 50 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 25 ), schIUScale.MilsToIU( 50 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( 100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( -25 ), schIUScale.MilsToIU( 50 ) } );
            aKsymbol->AddDrawItem( line2, false );
        }

        return { 0, schIUScale.MilsToIU( 150 ) };
    }
    else if( aStyle == ASCH_POWER_PORT_STYLE::WAVE )
    {
        SCH_SHAPE* line = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line->AddPoint( { 0, 0 } );
        line->AddPoint( { 0, schIUScale.MilsToIU( 72 ) } );
        aKsymbol->AddDrawItem( line, false );

        SCH_SHAPE* bezier = new SCH_SHAPE( SHAPE_T::BEZIER, LAYER_DEVICE );
        bezier->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 5 ), LINE_STYLE::SOLID ) );
        bezier->SetStart( { schIUScale.MilsToIU( 30 ), schIUScale.MilsToIU( 50 ) } );
        bezier->SetBezierC1( { schIUScale.MilsToIU( 30 ), schIUScale.MilsToIU( 87 ) } );
        bezier->SetBezierC2( { schIUScale.MilsToIU( -30 ), schIUScale.MilsToIU( 63 ) } );
        bezier->SetEnd( { schIUScale.MilsToIU( -30 ), schIUScale.MilsToIU( 100 ) } );
        aKsymbol->AddDrawItem( bezier, false );

        return { 0, schIUScale.MilsToIU( 150 ) };
    }
    else if( aStyle == ASCH_POWER_PORT_STYLE::POWER_GROUND
             || aStyle == ASCH_POWER_PORT_STYLE::SIGNAL_GROUND
             || aStyle == ASCH_POWER_PORT_STYLE::EARTH
             || aStyle == ASCH_POWER_PORT_STYLE::GOST_ARROW )
    {
        SCH_SHAPE* line1 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line1->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, schIUScale.MilsToIU( 100 ) } );
        aKsymbol->AddDrawItem( line1, false );

        if( aStyle == ASCH_POWER_PORT_STYLE::POWER_GROUND )
        {
            SCH_SHAPE* line2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
            line2->AddPoint( { schIUScale.MilsToIU( -100 ), schIUScale.MilsToIU( 100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 100 ), schIUScale.MilsToIU( 100 ) } );
            aKsymbol->AddDrawItem( line2, false );

            SCH_SHAPE* line3 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line3->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
            line3->AddPoint( { schIUScale.MilsToIU( -70 ), schIUScale.MilsToIU( 130 ) } );
            line3->AddPoint( { schIUScale.MilsToIU( 70 ), schIUScale.MilsToIU( 130 ) } );
            aKsymbol->AddDrawItem( line3, false );

            SCH_SHAPE* line4 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line4->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
            line4->AddPoint( { schIUScale.MilsToIU( -40 ), schIUScale.MilsToIU( 160 ) } );
            line4->AddPoint( { schIUScale.MilsToIU( 40 ), schIUScale.MilsToIU( 160 ) } );
            aKsymbol->AddDrawItem( line4, false );

            SCH_SHAPE* line5 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line5->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
            line5->AddPoint( { schIUScale.MilsToIU( -10 ), schIUScale.MilsToIU( 190 ) } );
            line5->AddPoint( { schIUScale.MilsToIU( 10 ), schIUScale.MilsToIU( 190 ) } );
            aKsymbol->AddDrawItem( line5, false );
        }
        else if( aStyle == ASCH_POWER_PORT_STYLE::SIGNAL_GROUND )
        {
            SCH_SHAPE* line2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
            line2->AddPoint( { schIUScale.MilsToIU( -100 ), schIUScale.MilsToIU( 100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 100 ), schIUScale.MilsToIU( 100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( 200 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( -100 ), schIUScale.MilsToIU( 100 ) } );
            aKsymbol->AddDrawItem( line2, false );
        }
        else if( aStyle == ASCH_POWER_PORT_STYLE::EARTH )
        {
            SCH_SHAPE* line2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
            line2->AddPoint( { schIUScale.MilsToIU( -150 ), schIUScale.MilsToIU( 200 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( -100 ), schIUScale.MilsToIU( 100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 100 ), schIUScale.MilsToIU( 100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 50 ), schIUScale.MilsToIU( 200 ) } );
            aKsymbol->AddDrawItem( line2, false );

            SCH_SHAPE* line3 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line3->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
            line3->AddPoint( { schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( 100 ) } );
            line3->AddPoint( { schIUScale.MilsToIU( -50 ), schIUScale.MilsToIU( 200 ) } );
            aKsymbol->AddDrawItem( line3, false );
        }
        else // ASCH_POWER_PORT_STYLE::GOST_ARROW
        {
            SCH_SHAPE* line2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
            line2->AddPoint( { schIUScale.MilsToIU( -25 ), schIUScale.MilsToIU( 50 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( 100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 25 ), schIUScale.MilsToIU( 50 ) } );
            aKsymbol->AddDrawItem( line2, false );

            return { 0, schIUScale.MilsToIU( 150 ) }; // special case
        }

        return { 0, schIUScale.MilsToIU( 250 ) };
    }
    else if( aStyle == ASCH_POWER_PORT_STYLE::GOST_POWER_GROUND
             || aStyle == ASCH_POWER_PORT_STYLE::GOST_EARTH )
    {
        SCH_SHAPE* line1 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line1->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, schIUScale.MilsToIU( 160 ) } );
        aKsymbol->AddDrawItem( line1, false );

        SCH_SHAPE* line2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line2->AddPoint( { schIUScale.MilsToIU( -100 ), schIUScale.MilsToIU( 160 ) } );
        line2->AddPoint( { schIUScale.MilsToIU( 100 ), schIUScale.MilsToIU( 160 ) } );
        aKsymbol->AddDrawItem( line2, false );

        SCH_SHAPE* line3 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line3->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line3->AddPoint( { schIUScale.MilsToIU( -60 ), schIUScale.MilsToIU( 200 ) } );
        line3->AddPoint( { schIUScale.MilsToIU( 60 ), schIUScale.MilsToIU( 200 ) } );
        aKsymbol->AddDrawItem( line3, false );

        SCH_SHAPE* line4 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line4->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line4->AddPoint( { schIUScale.MilsToIU( -20 ), schIUScale.MilsToIU( 240 ) } );
        line4->AddPoint( { schIUScale.MilsToIU( 20 ), schIUScale.MilsToIU( 240 ) } );
        aKsymbol->AddDrawItem( line4, false );

        if( aStyle == ASCH_POWER_PORT_STYLE::GOST_POWER_GROUND )
            return { 0, schIUScale.MilsToIU( -300 ) };

        SCH_SHAPE* circle = new SCH_SHAPE( SHAPE_T::CIRCLE, LAYER_DEVICE );
        circle->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        circle->SetPosition( { schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( 160 ) } );
        circle->SetEnd( circle->GetPosition() + VECTOR2I( schIUScale.MilsToIU( 120 ), 0 ) );
        aKsymbol->AddDrawItem( circle, false );

        return { 0, schIUScale.MilsToIU( 350 ) };
    }
    else if( aStyle == ASCH_POWER_PORT_STYLE::GOST_BAR )
    {
        SCH_SHAPE* line1 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line1->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, schIUScale.MilsToIU( 200 ) } );
        aKsymbol->AddDrawItem( line1, false );

        SCH_SHAPE* line2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line2->AddPoint( { schIUScale.MilsToIU( -100 ), schIUScale.MilsToIU( 200 ) } );
        line2->AddPoint( { schIUScale.MilsToIU( 100 ), schIUScale.MilsToIU( 200 ) } );
        aKsymbol->AddDrawItem( line2, false );

        return { 0, schIUScale.MilsToIU( 250 ) };
    }
    else
    {
        if( aStyle != ASCH_POWER_PORT_STYLE::BAR )
        {
            aReporter->Report( _( "Power Port with unknown style imported as 'Bar' type." ),
                               RPT_SEVERITY_WARNING );
        }

        SCH_SHAPE* line1 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line1->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, schIUScale.MilsToIU( 100 ) } );
        aKsymbol->AddDrawItem( line1, false );

        SCH_SHAPE* line2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line2->AddPoint( { schIUScale.MilsToIU( -50 ), schIUScale.MilsToIU( 100 ) } );
        line2->AddPoint( { schIUScale.MilsToIU( 50 ), schIUScale.MilsToIU( 100 ) } );
        aKsymbol->AddDrawItem( line2, false );

        return { 0, schIUScale.MilsToIU( 150 ) };
    }
}


void SCH_IO_ALTIUM::ParsePowerPort( const std::map<wxString, wxString>& aProperties )
{
    ASCH_POWER_PORT elem( aProperties );

    wxString    symName( elem.text );
    std::string styleName( magic_enum::enum_name<ASCH_POWER_PORT_STYLE>( elem.style ) );

    if( !styleName.empty() )
        symName << '_' << styleName;

    LIB_ID      libId = AltiumToKiCadLibID( getLibName(), symName );
    LIB_SYMBOL* libSymbol = nullptr;

    const auto& powerSymbolIt = m_powerSymbols.find( symName );

    if( powerSymbolIt != m_powerSymbols.end() )
    {
        libSymbol = powerSymbolIt->second; // cache hit
    }
    else if( LIB_SYMBOL* alreadyLoaded = m_pi->LoadSymbol( getLibFileName().GetFullPath(), symName,
                                                           m_properties.get() ) )
    {
        libSymbol = alreadyLoaded;
    }
    else
    {
        libSymbol = new LIB_SYMBOL( wxEmptyString );
        libSymbol->SetPower();
        libSymbol->SetName( elem.text );
        libSymbol->GetReferenceField().SetText( "#PWR" );
        libSymbol->GetReferenceField().SetVisible( false );
        libSymbol->GetValueField().SetText( elem.text );
        libSymbol->GetValueField().SetVisible( true );
        libSymbol->SetDescription( wxString::Format( _( "Power symbol creates a global "
                                                        "label with name '%s'" ), elem.text ) );
        libSymbol->SetKeyWords( "power-flag" );
        libSymbol->SetLibId( libId );

        // generate graphic
        SCH_PIN* pin = new SCH_PIN( libSymbol );
        libSymbol->AddDrawItem( pin, false );

        pin->SetName( elem.text );
        pin->SetPosition( { 0, 0 } );
        pin->SetLength( 0 );
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
        pin->SetVisible( false );

        VECTOR2I valueFieldPos = HelperGeneratePowerPortGraphics( libSymbol, elem.style,
                                                                  m_reporter );

        libSymbol->GetValueField().SetPosition( valueFieldPos );

        // this has to be done after parsing the LIB_SYMBOL!
        m_pi->SaveSymbol( getLibFileName().GetFullPath(), libSymbol, m_properties.get() );
        m_powerSymbols.insert( { symName, libSymbol } );
    }

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    // each symbol has its own powerSymbolIt for now
    SCH_SYMBOL* symbol = new SCH_SYMBOL();
    symbol->SetRef( &m_sheetPath, "#PWR?" );
    symbol->GetField( REFERENCE_FIELD )->SetVisible( false );
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


void SCH_IO_ALTIUM::ParseHarnessPort( const ASCH_PORT& aElem )
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

    textBox->SetStroke( STROKE_PARAMS( 2, LINE_STYLE::DEFAULT,
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
        textBox->SetTextSize( { font.Size / 2, font.Size / 2 } );

        // Must come after SetTextSize()
        textBox->SetBold( font.Bold );
        textBox->SetItalic( font.Italic );
        //textBox->SetFont(  //how to set font, we have a font name here: ( font.fontname );
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


void SCH_IO_ALTIUM::ParsePort( const ASCH_PORT& aElem )
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
    case ASCH_PORT_IOTYPE::UNSPECIFIED: label->SetShape( LABEL_FLAG_SHAPE::L_UNSPECIFIED ); break;
    case ASCH_PORT_IOTYPE::OUTPUT:      label->SetShape( LABEL_FLAG_SHAPE::L_OUTPUT );      break;
    case ASCH_PORT_IOTYPE::INPUT:       label->SetShape( LABEL_FLAG_SHAPE::L_INPUT );       break;
    case ASCH_PORT_IOTYPE::BIDI:        label->SetShape( LABEL_FLAG_SHAPE::L_BIDI );        break;
    }

    switch( aElem.Style )
    {
    default:
    case ASCH_PORT_STYLE::NONE_HORIZONTAL:
    case ASCH_PORT_STYLE::LEFT:
    case ASCH_PORT_STYLE::RIGHT:
    case ASCH_PORT_STYLE::LEFT_RIGHT:
        if( ( startIsWireTerminal || startIsBusTerminal ) )
            label->SetSpinStyle( SPIN_STYLE::RIGHT );
        else
            label->SetSpinStyle( SPIN_STYLE::LEFT );

        break;

    case ASCH_PORT_STYLE::NONE_VERTICAL:
    case ASCH_PORT_STYLE::TOP:
    case ASCH_PORT_STYLE::BOTTOM:
    case ASCH_PORT_STYLE::TOP_BOTTOM:
        if( ( startIsWireTerminal || startIsBusTerminal ) )
            label->SetSpinStyle( SPIN_STYLE::UP );
        else
            label->SetSpinStyle( SPIN_STYLE::BOTTOM );

        break;
    }

    label->AutoplaceFields( screen, AUTOPLACE_AUTO );

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


void SCH_IO_ALTIUM::ParseNoERC( const std::map<wxString, wxString>& aProperties )
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


void SCH_IO_ALTIUM::ParseNetLabel( const std::map<wxString, wxString>& aProperties )
{
    ASCH_NET_LABEL elem( aProperties );

    SCH_LABEL* label = new SCH_LABEL( elem.location + m_sheetOffset, elem.text );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    SetTextPositioning( label, elem.justification, elem.orientation );

    label->SetFlags( IS_NEW );
    screen->Append( label );
}


void SCH_IO_ALTIUM::ParseBus( const std::map<wxString, wxString>& aProperties )
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


void SCH_IO_ALTIUM::ParseWire( const std::map<wxString, wxString>& aProperties )
{
    ASCH_WIRE elem( aProperties );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    for( size_t i = 0; i + 1 < elem.points.size(); i++ )
    {
        SCH_LINE* wire = new SCH_LINE( elem.points.at( i ) + m_sheetOffset,
                                       SCH_LAYER_ID::LAYER_WIRE );
        wire->SetEndPoint( elem.points.at( i + 1 ) + m_sheetOffset );
        // wire->SetLineWidth( elem.lineWidth );

        wire->SetFlags( IS_NEW );
        screen->Append( wire );
    }
}


void SCH_IO_ALTIUM::ParseJunction( const std::map<wxString, wxString>& aProperties )
{
    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    ASCH_JUNCTION elem( aProperties );

    SCH_JUNCTION* junction = new SCH_JUNCTION( elem.location + m_sheetOffset );

    junction->SetFlags( IS_NEW );
    screen->Append( junction );
}


void SCH_IO_ALTIUM::ParseImage( const std::map<wxString, wxString>& aProperties )
{
    ASCH_IMAGE elem( aProperties );

    const auto& component = m_altiumComponents.find( elem.ownerindex );

    //Hide the image if it is owned by a component but the part id do not match
    if( component != m_altiumComponents.end()
        && component->second.currentpartid != elem.ownerpartid )
        return;

    VECTOR2I                    center = ( elem.location + elem.corner ) / 2 + m_sheetOffset;
    std::unique_ptr<SCH_BITMAP> bitmap = std::make_unique<SCH_BITMAP>( center );
    REFERENCE_IMAGE&            refImage = bitmap->GetReferenceImage();

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

        if( !refImage.ReadImageFile( storagePath ) )
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

        if( !refImage.ReadImageFile( elem.filename ) )
        {
            m_reporter->Report( wxString::Format( _( "Error reading image %s." ), elem.filename ),
                                RPT_SEVERITY_ERROR );
            return;
        }
    }

    // we only support one scale, thus we need to select one in case it does not keep aspect ratio
    const VECTOR2I currentImageSize = refImage.GetSize();
    const VECTOR2I expectedImageSize = elem.location - elem.corner;
    const double   scaleX =
            std::abs( static_cast<double>( expectedImageSize.x ) / currentImageSize.x );
    const double scaleY =
            std::abs( static_cast<double>( expectedImageSize.y ) / currentImageSize.y );
    refImage.SetImageScale( std::min( scaleX, scaleY ) );

    bitmap->SetFlags( IS_NEW );
    screen->Append( bitmap.release() );
}


void SCH_IO_ALTIUM::ParseSheet( const std::map<wxString, wxString>& aProperties )
{
    m_altiumSheet = std::make_unique<ASCH_SHEET>( aProperties );

    SCH_SCREEN* screen = getCurrentScreen();
    wxCHECK( screen, /* void */ );

    PAGE_INFO pageInfo;

    bool isPortrait = m_altiumSheet->sheetOrientation == ASCH_SHEET_WORKSPACEORIENTATION::PORTRAIT;

    if( m_altiumSheet->useCustomSheet )
    {
        PAGE_INFO::SetCustomWidthMils( schIUScale.IUToMils( m_altiumSheet->customSize.x ) );
        PAGE_INFO::SetCustomHeightMils( schIUScale.IUToMils( m_altiumSheet->customSize.y ) );
        pageInfo.SetType( PAGE_INFO::Custom, isPortrait );
    }
    else
    {
        switch( m_altiumSheet->sheetSize )
        {
        default:
        case ASCH_SHEET_SIZE::A4: pageInfo.SetType( "A4", isPortrait ); break;
        case ASCH_SHEET_SIZE::A3: pageInfo.SetType( "A3", isPortrait ); break;
        case ASCH_SHEET_SIZE::A2: pageInfo.SetType( "A2", isPortrait ); break;
        case ASCH_SHEET_SIZE::A1: pageInfo.SetType( "A1", isPortrait ); break;
        case ASCH_SHEET_SIZE::A0: pageInfo.SetType( "A0", isPortrait ); break;
        case ASCH_SHEET_SIZE::A: pageInfo.SetType( "A", isPortrait ); break;
        case ASCH_SHEET_SIZE::B: pageInfo.SetType( "B", isPortrait ); break;
        case ASCH_SHEET_SIZE::C: pageInfo.SetType( "C", isPortrait ); break;
        case ASCH_SHEET_SIZE::D: pageInfo.SetType( "D", isPortrait ); break;
        case ASCH_SHEET_SIZE::E: pageInfo.SetType( "E", isPortrait ); break;
        case ASCH_SHEET_SIZE::LETTER: pageInfo.SetType( "USLetter", isPortrait ); break;
        case ASCH_SHEET_SIZE::LEGAL: pageInfo.SetType( "USLegal", isPortrait ); break;
        case ASCH_SHEET_SIZE::TABLOID: pageInfo.SetType( "A3", isPortrait ); break;
        case ASCH_SHEET_SIZE::ORCAD_A: pageInfo.SetType( "A", isPortrait ); break;
        case ASCH_SHEET_SIZE::ORCAD_B: pageInfo.SetType( "B", isPortrait ); break;
        case ASCH_SHEET_SIZE::ORCAD_C: pageInfo.SetType( "C", isPortrait ); break;
        case ASCH_SHEET_SIZE::ORCAD_D: pageInfo.SetType( "D", isPortrait ); break;
        case ASCH_SHEET_SIZE::ORCAD_E: pageInfo.SetType( "E", isPortrait ); break;
        }
    }

    screen->SetPageSettings( pageInfo );

    m_sheetOffset = { 0, pageInfo.GetHeightIU( schIUScale.IU_PER_MILS ) };
}


void SCH_IO_ALTIUM::ParseSheetName( const std::map<wxString, wxString>& aProperties )
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


void SCH_IO_ALTIUM::ParseFileName( const std::map<wxString, wxString>& aProperties )
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


void SCH_IO_ALTIUM::ParseDesignator( const std::map<wxString, wxString>& aProperties )
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


void SCH_IO_ALTIUM::ParseLibDesignator( const std::map<wxString, wxString>& aProperties,
                                        std::vector<LIB_SYMBOL*>&           aSymbol,
                                        std::vector<int>&                   aFontSizes )
{
    ASCH_DESIGNATOR elem( aProperties );

    // Designators are shared by everyone
    for( LIB_SYMBOL* symbol : aSymbol )
    {
        bool emptyRef = elem.text.IsEmpty();
        SCH_FIELD& refField = symbol->GetReferenceField();

        if( emptyRef )
            refField.SetText( wxT( "X" ) );
        else
            refField.SetText( elem.text.BeforeLast( '?' ) ); // remove the '?' at the end for KiCad-style

        refField.SetPosition( elem.location );

        if( elem.fontId > 0 && elem.fontId <= static_cast<int>( aFontSizes.size() ) )
        {
            int size = aFontSizes[elem.fontId - 1];
            refField.SetTextSize( { size, size } );
        }
    }
}


void SCH_IO_ALTIUM::ParseBusEntry( const std::map<wxString, wxString>& aProperties )
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


void SCH_IO_ALTIUM::ParseParameter( const std::map<wxString, wxString>& aProperties )
{
    ASCH_PARAMETER elem( aProperties );

    // TODO: fill in replacements from variant, sheet and project
    static const std::map<wxString, wxString> variableMap = {
        { "COMMENT", "VALUE"        },
        { "VALUE",   "ALTIUM_VALUE" },
    };

    if( elem.ownerindex <= 0 )
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
            int      fieldIdx = 0;
            wxString fieldName = elem.name.Upper();

            fieldIdx = symbol->GetFieldCount();

            if( fieldName.IsEmpty() )
            {
                int disambiguate = 1;

                while( 1 )
                {
                    fieldName = wxString::Format( "ALTIUM_UNNAMED_%d", disambiguate++ );

                    if( !symbol->FindField( fieldName ) )
                        break;

                }
            }
            else if( fieldName == "VALUE" )
            {
                fieldName = "ALTIUM_VALUE";
            }

            field = symbol->AddField( SCH_FIELD( VECTOR2I(), fieldIdx, symbol, fieldName ) );
        }

        wxString kicadText = AltiumSchSpecialStringsToKiCadVariables( elem.text, variableMap );
        field->SetText( kicadText );
        field->SetPosition( elem.location + m_sheetOffset );
        field->SetVisible( !elem.isHidden );
        SetTextPositioning( field, elem.justification, elem.orientation );
    }
}


void SCH_IO_ALTIUM::ParseLibParameter( const std::map<wxString, wxString>& aProperties,
                                       std::vector<LIB_SYMBOL*>&           aSymbol,
                                       std::vector<int>&                   aFontSizes )
{
    ASCH_PARAMETER elem( aProperties );

    // Part ID 1 is the current library part.
    // Part ID ALTIUM_COMPONENT_NONE(-1) means all parts
    // If a parameter is assigned to a specific element such as a pin,
    // we will need to handle it here.
    // TODO: Handle HIDDENNETNAME property (others?)
    if( elem.ownerpartid != 1 && elem.ownerpartid != ALTIUM_COMPONENT_NONE )
        return;

    // If ownerindex is populated, this is parameter belongs to a subelement (e.g. pin).
    // Ignore for now.
    // TODO: Update this when KiCad supports parameters for any object
    if( elem.ownerindex != ALTIUM_COMPONENT_NONE )
        return;

    // TODO: fill in replacements from variant, sheet and project
    // N.B. We do not keep the Altium "VALUE" variable here because
    // we don't have a way to assign variables to specific symbols
    std::map<wxString, wxString> variableMap = {
        { "COMMENT", "VALUE" },
    };

    for( LIB_SYMBOL* libSymbol : aSymbol )
    {
        SCH_FIELD*  field = nullptr;
        wxString    upperName = elem.name.Upper();

        if( upperName == "COMMENT" )
        {
            field = &libSymbol->GetValueField();
        }
        else
        {
            int      fieldIdx = libSymbol->GetFieldCount();
            wxString fieldNameStem = elem.name;
            wxString fieldName = fieldNameStem;
            int disambiguate = 1;

            if( fieldName.IsEmpty() )
            {
                fieldNameStem = "ALTIUM_UNNAMED";
                fieldName = "ALTIUM_UNNAMED_1";
                disambiguate = 2;
            }
            else if( upperName == "VALUE" )
            {
                fieldNameStem = "ALTIUM_VALUE";
                fieldName = "ALTIUM_VALUE";
            }

            // Avoid adding duplicate fields
            while( libSymbol->FindField( fieldName ) )
                fieldName = wxString::Format( "%s_%d", fieldNameStem, disambiguate++ );

            SCH_FIELD* new_field = new SCH_FIELD( libSymbol, fieldIdx, fieldName );
            libSymbol->AddField( new_field );
            field = new_field;
        }

        wxString kicadText = AltiumSchSpecialStringsToKiCadVariables( elem.text, variableMap );
        field->SetText( kicadText );

        field->SetTextPos( elem.location );
        SetTextPositioning( field, elem.justification, elem.orientation );
        field->SetVisible( !elem.isHidden );

        if( elem.fontId > 0 && elem.fontId <= static_cast<int>( aFontSizes.size() ) )
        {
            int size = aFontSizes[elem.fontId - 1];
            field->SetTextSize( { size, size } );
        }
        else
        {
            int size = schIUScale.MilsToIU( DEFAULT_TEXT_SIZE );
            field->SetTextSize( { size, size } );
        }

    }
}


void SCH_IO_ALTIUM::ParseImplementationList( int aIndex,
                                             const std::map<wxString, wxString>& aProperties )
{
    ASCH_IMPLEMENTATION_LIST elem( aProperties );

    m_altiumImplementationList.emplace( aIndex, elem.ownerindex );
}


void SCH_IO_ALTIUM::ParseImplementation( const std::map<wxString, wxString>& aProperties,
                                         std::vector<LIB_SYMBOL*>&           aSymbol )
{
    ASCH_IMPLEMENTATION elem( aProperties );

    if( elem.type != wxS( "PCBLIB" ) )
        return;

    // For schematic files, we need to check if the model is current.
    if( aSymbol.size() == 0 && !elem.isCurrent )
        return;

    // For IntLibs we want to use the same lib name for footprints
    wxString libName = m_isIntLib ? m_libName : elem.libname;

    wxArrayString fpFilters;
    fpFilters.Add( wxString::Format( wxS( "*%s*" ), elem.name ) );

    // Parse the footprint fields for the library symbol
    if( !aSymbol.empty() )
    {
        for( LIB_SYMBOL* symbol : aSymbol )
        {
            LIB_ID fpLibId = AltiumToKiCadLibID( libName, elem.name );

            symbol->SetFPFilters( fpFilters );
            SCH_FIELD& footprintField = symbol->GetFootprintField();
            footprintField.SetText( fpLibId.Format() );
        }

        return;
    }

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

    LIB_ID fpLibId = AltiumToKiCadLibID( libName, elem.name );

    libSymbolIt->second->SetFPFilters( fpFilters ); // TODO: not ideal as we overwrite it

    SCH_SYMBOL* symbol = m_symbols.at( libSymbolIt->first );

    symbol->SetFootprintFieldText( fpLibId.Format() );
}



std::vector<LIB_SYMBOL*> SCH_IO_ALTIUM::ParseLibComponent( const std::map<wxString,
                                                           wxString>& aProperties )
{
    ASCH_SYMBOL elem( aProperties );

    std::vector<LIB_SYMBOL*> symbols;

    symbols.reserve( elem.displaymodecount );

    for( int i = 0; i < elem.displaymodecount; i++ )
    {
        LIB_SYMBOL* symbol = new LIB_SYMBOL( wxEmptyString );

        if( elem.displaymodecount > 1 )
            symbol->SetName( wxString::Format( "%s (Altium Display %d)", elem.libreference,
                                               i + 1 ) );
        else
            symbol->SetName( elem.libreference );

        LIB_ID libId = AltiumToKiCadLibID( getLibName(), symbol->GetName() );
        symbol->SetDescription( elem.componentdescription );
        symbol->SetLibId( libId );
        symbol->SetUnitCount( elem.partcount - 1 );
        symbols.push_back( symbol );
    }

    return symbols;
}


std::map<wxString,LIB_SYMBOL*> SCH_IO_ALTIUM::ParseLibFile( const ALTIUM_COMPOUND_FILE& aAltiumLibFile )
{
    std::map<wxString,LIB_SYMBOL*> ret;
    std::vector<int> fontSizes;
    struct SYMBOL_PIN_FRAC
    {
        int x_frac;
        int y_frac;
        int len_frac;
    };

    ParseLibHeader( aAltiumLibFile, fontSizes );

    std::map<wxString, ALTIUM_SYMBOL_DATA> syms = aAltiumLibFile.GetLibSymbols( nullptr );

    for( auto& [name, entry] : syms )
    {

        std::map<int, SYMBOL_PIN_FRAC> pinFracs;

        if( entry.m_pinsFrac )
        {
            auto parse_binary_pin_frac =
                    [&]( const std::string& binaryData ) -> std::map<wxString, wxString>
            {
                std::map<wxString, wxString> result;
                ALTIUM_COMPRESSED_READER     cmpreader( binaryData );

                std::pair<int, std::string*> pinFracData = cmpreader.ReadCompressedString();

                ALTIUM_BINARY_READER binreader( *pinFracData.second );
                SYMBOL_PIN_FRAC      pinFrac;

                pinFrac.x_frac = binreader.ReadInt32();
                pinFrac.y_frac = binreader.ReadInt32();
                pinFrac.len_frac = binreader.ReadInt32();
                pinFracs.insert( { pinFracData.first, pinFrac } );

                return result;
            };

            ALTIUM_BINARY_PARSER       reader( aAltiumLibFile, entry.m_pinsFrac );

            while( reader.GetRemainingBytes() > 0 )
            {
                reader.ReadProperties( parse_binary_pin_frac );
            }
        }

        ALTIUM_BINARY_PARSER reader( aAltiumLibFile, entry.m_symbol );
        std::vector<LIB_SYMBOL*> symbols;
        int pin_index = 0;

        if( reader.GetRemainingBytes() <= 0 )
        {
            THROW_IO_ERROR( "LibSymbol does not contain any data" );
        }

        {
            std::map<wxString, wxString> properties = reader.ReadProperties();
            int               recordId = ALTIUM_PROPS_UTILS::ReadInt( properties, "RECORD", 0 );
            ALTIUM_SCH_RECORD record   = static_cast<ALTIUM_SCH_RECORD>( recordId );

            if( record != ALTIUM_SCH_RECORD::COMPONENT )
                THROW_IO_ERROR( "LibSymbol does not start with COMPONENT record" );

            symbols = ParseLibComponent( properties );
        }

        auto handleBinaryPinLambda =
                [&]( const std::string& binaryData ) -> std::map<wxString, wxString>
                {
                    std::map<wxString, wxString> result;

                    ALTIUM_BINARY_READER binreader( binaryData );

                    int32_t recordId = binreader.ReadInt32();

                    if( recordId != static_cast<int32_t>( ALTIUM_SCH_RECORD::PIN ) )
                        THROW_IO_ERROR( "Binary record missing PIN record" );

                    result["RECORD"] = wxString::Format( "%d", recordId );
                    binreader.ReadByte(); // unknown
                    result["OWNERPARTID"] = wxString::Format( "%d",  binreader.ReadInt16() );
                    result["OWNERPARTDISPLAYMODE"] = wxString::Format( "%d",  binreader.ReadByte() );
                    result["SYMBOL_INNEREDGE"] = wxString::Format( "%d",  binreader.ReadByte() );
                    result["SYMBOL_OUTEREDGE"] = wxString::Format( "%d",  binreader.ReadByte() );
                    result["SYMBOL_INNER"] = wxString::Format( "%d",  binreader.ReadByte() );
                    result["SYMBOL_OUTER"] = wxString::Format( "%d",  binreader.ReadByte() );
                    result["TEXT"] = binreader.ReadShortPascalString();
                    binreader.ReadByte(); // unknown
                    result["ELECTRICAL"] = wxString::Format( "%d",  binreader.ReadByte() );
                    result["PINCONGLOMERATE"] = wxString::Format( "%d",  binreader.ReadByte() );
                    result["PINLENGTH"] = wxString::Format( "%d",  binreader.ReadInt16() );
                    result["LOCATION.X"] = wxString::Format( "%d",  binreader.ReadInt16() );
                    result["LOCATION.Y"] = wxString::Format( "%d",  binreader.ReadInt16() );
                    result["COLOR"] = wxString::Format( "%d",  binreader.ReadInt32() );
                    result["NAME"] = binreader.ReadShortPascalString();
                    result["DESIGNATOR"] = binreader.ReadShortPascalString();
                    result["SWAPIDGROUP"] = binreader.ReadShortPascalString();


                    if( auto it = pinFracs.find( pin_index ); it != pinFracs.end() )
                    {
                        result["LOCATION.X_FRAC"] = wxString::Format( "%d", it->second.x_frac );
                        result["LOCATION.Y_FRAC"] = wxString::Format( "%d", it->second.y_frac );
                        result["PINLENGTH_FRAC"] = wxString::Format( "%d", it->second.len_frac );
                    }

                    std::string partSeq = binreader.ReadShortPascalString(); // This is 'part|&|seq'
                    std::vector<std::string> partSeqSplit = split( partSeq, "|" );

                    if( partSeqSplit.size() == 3 )
                    {
                        result["PART"] = partSeqSplit[0];
                        result["SEQ"] = partSeqSplit[2];
                    }

                    return result;
                };

        while( reader.GetRemainingBytes() > 0 )
        {
            std::map<wxString, wxString> properties = reader.ReadProperties( handleBinaryPinLambda );

            if( properties.empty() )
                continue;

            int               recordId = ALTIUM_PROPS_UTILS::ReadInt( properties, "RECORD", 0 );
            ALTIUM_SCH_RECORD record   = static_cast<ALTIUM_SCH_RECORD>( recordId );

            switch( record )
            {
            case ALTIUM_SCH_RECORD::PIN:
            {
                ParsePin( properties, symbols );
                pin_index++;
                break;
            }

            case ALTIUM_SCH_RECORD::LABEL: ParseLabel( properties, symbols, fontSizes ); break;

            case ALTIUM_SCH_RECORD::BEZIER: ParseBezier( properties, symbols ); break;

            case ALTIUM_SCH_RECORD::POLYLINE: ParsePolyline( properties, symbols ); break;

            case ALTIUM_SCH_RECORD::POLYGON: ParsePolygon( properties, symbols ); break;

            case ALTIUM_SCH_RECORD::ELLIPSE: ParseEllipse( properties, symbols ); break;

            case ALTIUM_SCH_RECORD::PIECHART: ParsePieChart( properties, symbols ); break;

            case ALTIUM_SCH_RECORD::ROUND_RECTANGLE: ParseRoundRectangle( properties, symbols ); break;

            case ALTIUM_SCH_RECORD::ELLIPTICAL_ARC: ParseEllipticalArc( properties, symbols ); break;

            case ALTIUM_SCH_RECORD::ARC: ParseArc( properties, symbols ); break;

            case ALTIUM_SCH_RECORD::LINE: ParseLine( properties, symbols ); break;

            case ALTIUM_SCH_RECORD::RECTANGLE: ParseRectangle( properties, symbols ); break;

            case ALTIUM_SCH_RECORD::DESIGNATOR: ParseLibDesignator( properties, symbols, fontSizes ); break;

            case ALTIUM_SCH_RECORD::PARAMETER: ParseLibParameter( properties, symbols, fontSizes ); break;

            case ALTIUM_SCH_RECORD::TEXT_FRAME: ParseTextFrame( properties, symbols, fontSizes ); break;

            // Nothing for now.  TODO: Figure out how implementation lists are generted in libs
            case ALTIUM_SCH_RECORD::IMPLEMENTATION_LIST: break;

            case ALTIUM_SCH_RECORD::IMPLEMENTATION: ParseImplementation( properties, symbols ); break;

            case ALTIUM_SCH_RECORD::IMPL_PARAMS: break;

            case ALTIUM_SCH_RECORD::MAP_DEFINER_LIST: break;
            case ALTIUM_SCH_RECORD::MAP_DEFINER: break;

            // TODO: add support for these.  They are just drawn symbols, so we can probably hardcode
            case ALTIUM_SCH_RECORD::IEEE_SYMBOL: break;

            // TODO: Hanlde images once libedit supports them
            case ALTIUM_SCH_RECORD::IMAGE: break;

            default:
                m_reporter->Report( wxString::Format( _( "Unknown or unexpected record id %d found "
                                                         "in %s." ),
                                                      recordId, symbols[0]->GetName() ),
                                    RPT_SEVERITY_ERROR );
                break;
            }
        }

        if( reader.HasParsingError() )
            THROW_IO_ERROR( "stream was not parsed correctly!" );

        if( reader.GetRemainingBytes() != 0 )
            THROW_IO_ERROR( "stream is not fully parsed" );

        for( size_t ii = 0; ii < symbols.size(); ii++ )
        {
            LIB_SYMBOL* symbol = symbols[ii];
            symbol->FixupDrawItems();
            fixupSymbolPinNameNumbers( symbol );

            SCH_FIELD& valField = symbol->GetValueField();

            if( valField.GetText().IsEmpty() )
                valField.SetText( name );

            if( symbols.size() == 1 )
                ret[name] = symbol;
            else
                ret[wxString::Format( "%s (Altium Display %zd)", name, ii + 1 )] = symbol;
        }
    }

    return ret;
}


long long SCH_IO_ALTIUM::getLibraryTimestamp( const wxString& aLibraryPath ) const
{
    wxFileName fn( aLibraryPath );

    if( fn.IsFileReadable() && fn.GetModificationTime().IsValid() )
        return fn.GetModificationTime().GetValue().GetValue();
    else
        return wxDateTime( 0.0 ).GetValue().GetValue();
}


void SCH_IO_ALTIUM::ensureLoadedLibrary( const wxString&        aLibraryPath,
                                         const std::map<std::string, UTF8>* aProperties )
{
    // Suppress font substitution warnings
    fontconfig::FONTCONFIG::SetReporter( nullptr );

    if( m_libCache.count( aLibraryPath ) )
    {
        wxCHECK( m_timestamps.count( aLibraryPath ), /*void*/ );

        if( m_timestamps.at( aLibraryPath ) == getLibraryTimestamp( aLibraryPath ) )
            return;
    }

    std::vector<std::unique_ptr<ALTIUM_COMPOUND_FILE>> compoundFiles;

    wxFileName fileName( aLibraryPath );
    m_libName = fileName.GetName();

    try
    {
        if( aLibraryPath.Lower().EndsWith( wxS( ".schlib" ) ) )
        {
            m_isIntLib = false;

            compoundFiles.push_back( std::make_unique<ALTIUM_COMPOUND_FILE>( aLibraryPath ) );
        }
        else if( aLibraryPath.Lower().EndsWith( wxS( ".intlib" ) ) )
        {
            m_isIntLib = true;

            std::unique_ptr<ALTIUM_COMPOUND_FILE> intCom =
                    std::make_unique<ALTIUM_COMPOUND_FILE>( aLibraryPath );

            std::map<wxString, const CFB::COMPOUND_FILE_ENTRY*> schLibFiles =
                    intCom->EnumDir( L"SchLib" );

            for( const auto& [schLibName, cfe] : schLibFiles )
                compoundFiles.push_back( intCom->DecodeIntLibStream( *cfe ) );
        }

        std::map<wxString, LIB_SYMBOL*>& cacheMapRef = m_libCache[aLibraryPath];

        for( auto& altiumSchFilePtr : compoundFiles )
        {
            std::map<wxString, LIB_SYMBOL*> parsed = ParseLibFile( *altiumSchFilePtr );
            cacheMapRef.insert( parsed.begin(), parsed.end() );
        }

        m_timestamps[aLibraryPath] = getLibraryTimestamp( aLibraryPath );
    }
    catch( const CFB::CFBException& exception )
    {
        THROW_IO_ERROR( exception.what() );
    }
    catch( const std::exception& exc )
    {
        wxFAIL_MSG( wxString::Format( wxT( "Unhandled exception in Altium schematic parsers: %s." ),
                                      exc.what() ) );
        throw;
    }
}


void SCH_IO_ALTIUM::ParseLibHeader( const ALTIUM_COMPOUND_FILE& aAltiumSchFile,
                                    std::vector<int>& aFontSizes )
{
    const CFB::COMPOUND_FILE_ENTRY* file = aAltiumSchFile.FindStream( { "FileHeader" } );

    if( file == nullptr )
        THROW_IO_ERROR( "FileHeader not found" );

    ALTIUM_BINARY_PARSER reader( aAltiumSchFile, file );

    if( reader.GetRemainingBytes() <= 0 )
    {
        THROW_IO_ERROR( "FileHeader does not contain any data" );
    }

    std::map<wxString, wxString> properties = reader.ReadProperties();

    wxString libtype = ALTIUM_PROPS_UTILS::ReadString( properties, "HEADER", "" );

    if( libtype.CmpNoCase( "Protel for Windows - Schematic Library Editor Binary File Version 5.0" ) )
        THROW_IO_ERROR( _( "Expected Altium Schematic Library file version 5.0" ) );

    for( auto& [key, value] : properties )
    {
        wxString upperKey = key.Upper();
        wxString remaining;

        if( upperKey.StartsWith( "SIZE", &remaining ) )
        {
            if( !remaining.empty() )
            {
                int ind = wxAtoi( remaining );

                if( static_cast<int>( aFontSizes.size() ) < ind )
                    aFontSizes.resize( ind );

                // Altium stores in pt.  1 pt = 1/72 inch.  1 mil = 1/1000 inch.
                int scaled = schIUScale.MilsToIU( wxAtoi( value ) * 72.0 / 10.0 );
                aFontSizes[ind - 1] = scaled;
            }
        }
    }

}


void SCH_IO_ALTIUM::doEnumerateSymbolLib( const wxString& aLibraryPath,
        const std::map<std::string, UTF8>* aProperties,
        std::function<void(const wxString&, LIB_SYMBOL*)> aInserter )
{
    ensureLoadedLibrary( aLibraryPath, aProperties );

    bool powerSymbolsOnly = ( aProperties &&
                              aProperties->find( SYMBOL_LIB_TABLE::PropPowerSymsOnly )
                                      != aProperties->end() );

    auto it = m_libCache.find( aLibraryPath );

    if( it != m_libCache.end() )
    {
        for( auto& [libnameStr, libSymbol] : it->second )
        {
            if( powerSymbolsOnly && !libSymbol->IsPower() )
                continue;

            aInserter( libnameStr, libSymbol );
        }
    }
}


void SCH_IO_ALTIUM::EnumerateSymbolLib( wxArrayString&         aSymbolNameList,
                                        const wxString&        aLibraryPath,
                                        const std::map<std::string, UTF8>* aProperties )
{
    doEnumerateSymbolLib( aLibraryPath, aProperties,
                          [&]( const wxString& aStr, LIB_SYMBOL* )
                          {
                              aSymbolNameList.Add( aStr );
                          } );
}


void SCH_IO_ALTIUM::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                        const wxString&           aLibraryPath,
                                        const std::map<std::string, UTF8>*    aProperties )
{
    doEnumerateSymbolLib( aLibraryPath, aProperties,
                          [&]( const wxString&, LIB_SYMBOL* aSymbol )
                          {
                              aSymbolList.emplace_back( aSymbol );
                          } );
}


LIB_SYMBOL* SCH_IO_ALTIUM::LoadSymbol( const wxString&        aLibraryPath,
                                       const wxString&        aAliasName,
                                       const std::map<std::string, UTF8>* aProperties )
{
    ensureLoadedLibrary( aLibraryPath, aProperties );

    auto it = m_libCache.find( aLibraryPath );

    if( it != m_libCache.end() )
    {
        auto it2 = it->second.find( aAliasName );

        if( it2 != it->second.end() )
            return it2->second;
    }

    return nullptr;
}
