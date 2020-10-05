/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#include <sch_plugins/altium/sch_altium_plugin.h>

#include <schematic.h>

#include <lib_arc.h>
#include <lib_circle.h>
#include <lib_id.h>
#include <lib_item.h>
#include <lib_pin.h>
#include <lib_polyline.h>
#include <lib_rectangle.h>
#include <lib_text.h>

#include <bus_alias.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_component.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_no_connect.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_text.h>

#include "altium_parser_sch.h"
#include <memory>
#include <compoundfilereader.h>
#include <plugins/altium/altium_parser.h>
#include <sch_plugins/legacy/sch_legacy_plugin.h>
#include <wildcards_and_files_ext.h>
#include <wx/textfile.h>


const wxPoint calculateComponentPoint( const wxPoint& aPosition, const SCH_COMPONENT* aComponent )
{
    const wxPoint newPos = aPosition - aComponent->GetPosition();
    return { newPos.x, -newPos.y };
}


SCH_ALTIUM_PLUGIN::SCH_ALTIUM_PLUGIN()
{
    m_rootSheet    = nullptr;
    m_currentSheet = nullptr;
    m_schematic    = nullptr;
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
        m_libName = LIB_ID::FixIllegalChars( m_libName, LIB_ID::ID_SCH, true );
    }

    return m_libName;
}


wxFileName SCH_ALTIUM_PLUGIN::getLibFileName()
{
    wxFileName fn( m_schematic->Prj().GetProjectPath(), getLibName(), KiCadSymbolLibFileExtension );

    return fn;
}

SCH_SHEET* SCH_ALTIUM_PLUGIN::Load( const wxString& aFileName, SCHEMATIC* aSchematic,
        SCH_SHEET* aAppendToMe, const PROPERTIES* aProperties )
{
    wxASSERT( !aFileName || aSchematic != NULL );

    m_filename  = aFileName;
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
        m_rootSheet->SetFileName( aFileName );
    }

    if( !m_rootSheet->GetScreen() )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( m_schematic );
        screen->SetFileName( aFileName );
        m_rootSheet->SetScreen( screen );
    }

    SYMBOL_LIB_TABLE* libTable = m_schematic->Prj().SchSymbolLibTable();

    wxCHECK_MSG( libTable, NULL, "Could not load symbol lib table." );

    m_pi.set( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
    m_properties                                        = std::make_unique<PROPERTIES>();
    ( *m_properties )[SCH_LEGACY_PLUGIN::PropBuffering] = "";

    /// @note No check is being done here to see if the existing symbol library exists so this
    ///       will overwrite the existing one.
    if( !libTable->HasLibrary( getLibName() ) )
    {
        // Create a new empty symbol library.
        m_pi->CreateSymbolLib( getLibFileName().GetFullPath() );
        wxString libTableUri = "${KIPRJMOD}/" + getLibFileName().GetFullName();

        // Add the new library to the project symbol library table.
        libTable->InsertRow(
                new SYMBOL_LIB_TABLE_ROW( getLibName(), libTableUri, wxString( "KiCad" ) ) );

        // Save project symbol library table.
        wxFileName fn( m_schematic->Prj().GetProjectPath(),
                SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

        // So output formatter goes out of scope and closes the file before reloading.
        {
            FILE_OUTPUTFORMATTER formatter( fn.GetFullPath() );
            libTable->Format( &formatter, 0 );
        }

        // Relaod the symbol library table.
        m_schematic->Prj().SetElem( PROJECT::ELEM_SYMBOL_LIB_TABLE, NULL );
        m_schematic->Prj().SchSymbolLibTable();
    }

    m_currentSheet = m_rootSheet;
    ParseAltiumSch( aFileName );

    m_pi->SaveLibrary( getLibFileName().GetFullPath() );

    return m_rootSheet;
}


/*wxString SCH_EAGLE_PLUGIN::fixSymbolName( const wxString& aName )
{
    wxString ret = LIB_ID::FixIllegalChars( aName, LIB_ID::ID_SCH );

    return ret;
}*/

void SCH_ALTIUM_PLUGIN::ParseAltiumSch( const wxString& aFileName )
{
    // Open file
    FILE* fp = wxFopen( aFileName, "rb" );
    if( fp == nullptr )
    {
        wxLogError( wxString::Format( _( "Cannot open file '%s'" ), aFileName ) );
        return;
    }

    fseek( fp, 0, SEEK_END );
    long len = ftell( fp );
    if( len < 0 )
    {
        fclose( fp );
        THROW_IO_ERROR( "Reading error, cannot determine length of file" );
    }

    std::unique_ptr<unsigned char[]> buffer( new unsigned char[len] );
    fseek( fp, 0, SEEK_SET );

    size_t bytesRead = fread( buffer.get(), sizeof( unsigned char ), len, fp );
    fclose( fp );
    if( static_cast<size_t>( len ) != bytesRead )
    {
        THROW_IO_ERROR( "Reading error" );
    }

    try
    {
        CFB::CompoundFileReader reader( buffer.get(), bytesRead );
        Parse( reader );
    }
    catch( CFB::CFBException& exception )
    {
        THROW_IO_ERROR( exception.what() );
    }
}


void SCH_ALTIUM_PLUGIN::Parse( const CFB::CompoundFileReader& aReader )
{
    const CFB::COMPOUND_FILE_ENTRY* file = FindStream( aReader, "FileHeader" );
    if( file == nullptr )
    {
        THROW_IO_ERROR( "FileHeader not found" );
    }

    ALTIUM_PARSER reader( aReader, file );

    if( reader.GetRemainingBytes() <= 0 )
    {
        THROW_IO_ERROR( "FileHeader does not contain any data" );
    }
    else
    {
        std::map<wxString, wxString> properties = reader.ReadProperties();

        int               recordId = ALTIUM_PARSER::PropertiesReadInt( properties, "RECORD", 0 );
        ALTIUM_SCH_RECORD record   = static_cast<ALTIUM_SCH_RECORD>( recordId );

        if( record != ALTIUM_SCH_RECORD::HEADER )
        {
            THROW_IO_ERROR( "Header expected" );
        }
    }

    // index is required required to resolve OWNERINDEX
    for( int index = 0; reader.GetRemainingBytes() > 0; index++ )
    {
        std::map<wxString, wxString> properties = reader.ReadProperties();

        int               recordId = ALTIUM_PARSER::PropertiesReadInt( properties, "RECORD", 0 );
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
            break;
        case ALTIUM_SCH_RECORD::LABEL:
            break;
        case ALTIUM_SCH_RECORD::BEZIER:
            break;
        case ALTIUM_SCH_RECORD::POLYLINE:
            break;
        case ALTIUM_SCH_RECORD::POLYGON:
            break;
        case ALTIUM_SCH_RECORD::ELLIPSE:
            break;
        case ALTIUM_SCH_RECORD::PIECHART:
            break;
        case ALTIUM_SCH_RECORD::ROUND_RECTANGLE:
            break;
        case ALTIUM_SCH_RECORD::ELLIPTICAL_ARC:
            break;
        case ALTIUM_SCH_RECORD::ARC:
            break;
        case ALTIUM_SCH_RECORD::LINE:
            break;
        case ALTIUM_SCH_RECORD::RECTANGLE:
            ParseRectangle( properties );
            break;
        case ALTIUM_SCH_RECORD::SHEET_SYMBOL:
            break;
        case ALTIUM_SCH_RECORD::SHEET_ENTRY:
            break;
        case ALTIUM_SCH_RECORD::POWER_PORT:
            break;
        case ALTIUM_SCH_RECORD::PORT:
            break;
        case ALTIUM_SCH_RECORD::NO_ERC:
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
            break;
        case ALTIUM_SCH_RECORD::JUNCTION:
            break;
        case ALTIUM_SCH_RECORD::IMAGE:
            break;
        case ALTIUM_SCH_RECORD::SHEET:
            break;
        case ALTIUM_SCH_RECORD::SHEET_NAME:
            break;
        case ALTIUM_SCH_RECORD::FILE_NAME:
            break;
        case ALTIUM_SCH_RECORD::DESIGNATOR:
            ParseDesignator( properties );
            break;
        case ALTIUM_SCH_RECORD::BUS_ENTRY:
            break;
        case ALTIUM_SCH_RECORD::TEMPLATE:
            break;
        case ALTIUM_SCH_RECORD::PARAMETER:
            break;
        case ALTIUM_SCH_RECORD::WARNING_SIGN:
            break;
        case ALTIUM_SCH_RECORD::IMPLEMENTATION_LIST:
            break;
        case ALTIUM_SCH_RECORD::IMPLEMENTATION:
            break;
        case ALTIUM_SCH_RECORD::RECORD_46:
            break;
        case ALTIUM_SCH_RECORD::RECORD_47:
            break;
        case ALTIUM_SCH_RECORD::RECORD_48:
            break;
        case ALTIUM_SCH_RECORD::RECORD_215:
            break;
        case ALTIUM_SCH_RECORD::RECORD_216:
            break;
        case ALTIUM_SCH_RECORD::RECORD_217:
            break;
        case ALTIUM_SCH_RECORD::RECORD_218:
            break;
        case ALTIUM_SCH_RECORD::RECORD_226:
            break;
        default:
            wxLogError( wxString::Format( "Unknown Record id: %d", recordId ) );
            break;
        }
    }

    if( reader.HasParsingError() )
    {
        THROW_IO_ERROR( "stream was not parsed correctly!" );
    }

    if( reader.GetRemainingBytes() != 0 )
    {
        THROW_IO_ERROR( "stream is not fully parsed" );
    }

    // assign LIB_PART -> COMPONENT
    for( auto component : m_components )
    {
        auto kpart = m_symbols.find( component.first );
        if( kpart == m_symbols.end() )
        {
            THROW_IO_ERROR( "every component should have a symbol attached" );
        }

        m_pi->SaveSymbol( getLibFileName().GetFullPath(), new LIB_PART( *( kpart->second ) ),
                m_properties.get() );

        component.second->SetLibSymbol( kpart->second );
    }

    m_components.clear();
    m_symbols.clear();

    // Otherwise we cannot save the imported sheet?
    m_currentSheet->SetModified();
}


void SCH_ALTIUM_PLUGIN::ParseComponent( int index, const std::map<wxString, wxString>& aProperties )
{
    ASCH_COMPONENT elem( aProperties );

    LIB_ID libId( getLibName(), elem.libreference );

    LIB_PART* kpart = new LIB_PART( wxEmptyString );
    kpart->SetName( elem.libreference );
    kpart->SetLibId( libId );
    m_symbols.insert( { index, kpart } );

    // each component has its own symbol for now
    SCH_COMPONENT* component = new SCH_COMPONENT();

    component->SetPosition( elem.location );
    component->SetOrientation( elem.orientation );
    component->SetLibId( libId );
    //component->SetLibSymbol( kpart ); // this has to be done after parsing the LIB_PART!

    m_currentSheet->GetScreen()->Append( component );

    m_components.insert( { index, component } );
    std::cout << "component index: " << index << " partid: " << elem.currentpartid << std::endl;
}


void SCH_ALTIUM_PLUGIN::ParsePin( const std::map<wxString, wxString>& aProperties )
{
    ASCH_PIN elem( aProperties );

    const auto& symbol = m_symbols.find( elem.ownerindex );
    if( symbol == m_symbols.end() )
    {
        // TODO: e.g. can depend on Template (RECORD=39
        wxLogWarning( wxString::Format(
                "Pin tries to access symbol with ownerindex %d which does not exist",
                elem.ownerindex ) );
        return;
    }

    const auto& component = m_components.at( symbol->first );

    LIB_PIN* pin = new LIB_PIN( symbol->second );
    symbol->second->AddDrawItem( pin );

    pin->SetPosition( calculateComponentPoint( elem.location, component ) );
    pin->SetOrientation( elem.orientation );
    pin->SetName( elem.name );
    pin->SetNumber( elem.designator );
}


void SCH_ALTIUM_PLUGIN::ParseRectangle( const std::map<wxString, wxString>& aProperties )
{
    ASCH_RECTANGLE elem( aProperties );

    const auto& symbol = m_symbols.find( elem.ownerindex );
    if( symbol == m_symbols.end() )
    {
        // TODO: e.g. can depend on Template (RECORD=39
        wxLogWarning( wxString::Format(
                "Pin tries to access symbol with ownerindex %d which does not exist",
                elem.ownerindex ) );
        return;
    }

    const auto& component = m_components.at( symbol->first );

    LIB_RECTANGLE* rect = new LIB_RECTANGLE( symbol->second );
    symbol->second->AddDrawItem( rect );
    rect->SetPosition( calculateComponentPoint( elem.topRight, component ) );
    rect->SetEnd( calculateComponentPoint( elem.bottomLeft, component ) );
    rect->SetWidth( elem.lineWidth );
    if( elem.isTransparent )
    {
        rect->SetFillMode( NO_FILL );
    }
    else if( elem.isSolid )
    {
        rect->SetFillMode( FILLED_SHAPE );
    }
    else
    {
        rect->SetFillMode( FILLED_WITH_BG_BODYCOLOR );
    }
}


void SCH_ALTIUM_PLUGIN::ParseNetLabel( const std::map<wxString, wxString>& aProperties )
{
    ASCH_NET_LABEL elem( aProperties );

    SCH_LABEL* label = new SCH_LABEL( elem.location, elem.text );

    switch( elem.orientation )
    {
    case 0:
        label->SetLabelSpinStyle( LABEL_SPIN_STYLE::LEFT );
        break;
    case 1:
        label->SetLabelSpinStyle( LABEL_SPIN_STYLE::UP );
        break;
    case 2:
        label->SetLabelSpinStyle( LABEL_SPIN_STYLE::RIGHT );
        break;
    case 3:
        label->SetLabelSpinStyle( LABEL_SPIN_STYLE::BOTTOM );
        break;
    default:
        break;
    }

    label->SetFlags( IS_NEW );
    m_currentSheet->GetScreen()->Append( label );
}


void SCH_ALTIUM_PLUGIN::ParseBus( const std::map<wxString, wxString>& aProperties )
{
    ASCH_BUS elem( aProperties );

    for( int i = 0; i < (int) elem.points.size() - 1; i++ )
    {
        SCH_LINE* bus = new SCH_LINE( elem.points.at( i ), SCH_LAYER_ID::LAYER_BUS );

        bus->SetEndPoint( elem.points.at( i + 1 ) );

        bus->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( bus );
    }
}


void SCH_ALTIUM_PLUGIN::ParseWire( const std::map<wxString, wxString>& aProperties )
{
    ASCH_WIRE elem( aProperties );

    for( int i = 0; i < (int) elem.points.size() - 1; i++ )
    {
        SCH_LINE* wire = new SCH_LINE( elem.points.at( i ), SCH_LAYER_ID::LAYER_WIRE );

        wire->SetEndPoint( elem.points.at( i + 1 ) );

        wire->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( wire );
    }
}


void SCH_ALTIUM_PLUGIN::ParseDesignator( const std::map<wxString, wxString>& aProperties )
{
    ASCH_DESIGNATOR elem( aProperties );

    if( elem.ownerpartid == ALTIUM_COMPONENT_NONE )
    {
        return; // TODO: what to do?
    }

    const auto& component = m_components.find( elem.ownerpartid );
    if( component == m_components.end() )
    {
        // TODO: e.g. can depend on Template (RECORD=39
        THROW_IO_ERROR( wxString::Format(
                "Designator tries to access component with ownerpartid %d which does not exist",
                elem.ownerpartid ) );
    }

    LIB_PART* symbol = m_symbols.at( elem.ownerpartid );
    // TODO: component->second->SetRef(m_sheet, elem.name);

    LIB_TEXT* text = new LIB_TEXT( symbol );
    symbol->AddDrawItem( text );

    text->SetPosition( elem.location );
    text->SetTextAngle( elem.orientation * 90. );
    text->SetText( elem.name ); // TODO: use variable
}
