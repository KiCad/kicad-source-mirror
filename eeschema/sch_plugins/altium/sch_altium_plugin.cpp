/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <plugins/altium/altium_parser.h>
#include <plugins/altium/altium_parser_utils.h>
#include <sch_plugins/altium/sch_altium_plugin.h>

#include <schematic.h>

#include <lib_arc.h>
#include <lib_bezier.h>
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
#include <sch_symbol.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_text.h>

#include <bezier_curves.h>
#include <compoundfilereader.h>
#include <kicad_string.h>
#include <sch_edit_frame.h>
#include <wildcards_and_files_ext.h>


const wxPoint GetRelativePosition( const wxPoint& aPosition, const SCH_COMPONENT* aComponent )
{
    TRANSFORM t = aComponent->GetTransform().InverseTransform();
    return t.TransformCoordinate( aPosition - aComponent->GetPosition() );
}


COLOR4D GetColorFromInt( int color )
{
    int red   = color & 0x0000FF;
    int green = ( color & 0x00FF00 ) >> 8;
    int blue  = ( color & 0xFF0000 ) >> 16;

    return COLOR4D().FromCSSRGBA( red, green, blue, 1.0 );
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
                                    SCH_SHEET* aAppendToMe, const PROPERTIES* aProperties )
{
    wxASSERT( !aFileName || aSchematic != NULL );

    wxFileName fileName( aFileName );
    fileName.SetExt( ".kicad_sch" );
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
        THROW_IO_ERROR( "Reading error" );

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
        THROW_IO_ERROR( "FileHeader not found" );

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
            THROW_IO_ERROR( "Header expected" );
    }

    // Prepare some local variables
    wxASSERT( m_altiumPortsCurrentSheet.empty() );
    wxASSERT( !m_currentTitleBlock );

    m_currentTitleBlock = std::make_unique<TITLE_BLOCK>();

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
            break;
        case ALTIUM_SCH_RECORD::PIECHART:
            break;
        case ALTIUM_SCH_RECORD::ROUND_RECTANGLE:
            ParseRoundRectangle( properties );
            break;
        case ALTIUM_SCH_RECORD::ELLIPTICAL_ARC:
            break;
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
            break;
        case ALTIUM_SCH_RECORD::JUNCTION:
            ParseJunction( properties );
            break;
        case ALTIUM_SCH_RECORD::IMAGE:
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
        case ALTIUM_SCH_RECORD::RECORD_209:
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
        THROW_IO_ERROR( "stream was not parsed correctly!" );

    if( reader.GetRemainingBytes() != 0 )
        THROW_IO_ERROR( "stream is not fully parsed" );

    // assign LIB_PART -> COMPONENT
    for( auto component : m_components )
    {
        auto kpart = m_symbols.find( component.first );

        if( kpart == m_symbols.end() )
            THROW_IO_ERROR( "every component should have a symbol attached" );

        m_pi->SaveSymbol( getLibFileName().GetFullPath(), new LIB_PART( *( kpart->second ) ),
                          m_properties.get() );

        component.second->SetLibSymbol( kpart->second );
    }

    // Handle title blocks
    m_currentSheet->GetScreen()->SetTitleBlock( *m_currentTitleBlock );
    m_currentTitleBlock.reset();

    // Handle Ports
    for( const ASCH_PORT& port : m_altiumPortsCurrentSheet )
        ParsePort( port );

    m_altiumPortsCurrentSheet.clear();

    m_components.clear();
    m_symbols.clear();

    // Otherwise we cannot save the imported sheet?
    m_currentSheet->SetModified();
}


bool SCH_ALTIUM_PLUGIN::IsComponentPartVisible( int aOwnerindex, int aOwnerpartdisplaymode ) const
{
    const auto& component = m_altiumComponents.find( aOwnerindex );

    if( component == m_altiumComponents.end() )
        return false;

    return component->second.displaymode == aOwnerpartdisplaymode;
}


void SCH_ALTIUM_PLUGIN::ParseComponent( int aIndex,
                                        const std::map<wxString, wxString>& aProperties )
{
    auto pair = m_altiumComponents.insert( { aIndex, ASCH_COMPONENT( aProperties ) } );
    const ASCH_COMPONENT& elem = pair.first->second;

    // TODO: this is a hack until we correctly apply all transformations to every element
    wxString name = wxString::Format( "%d%s_%s",
                                      elem.orientation,
                                      elem.isMirrored ? "_mirrored" : "",
                                      elem.libreference );
    LIB_ID libId = AltiumToKiCadLibID( getLibName(), name );

    LIB_PART* kpart = new LIB_PART( wxEmptyString );
    kpart->SetName( name );
    kpart->SetDescription( elem.componentdescription );
    kpart->SetLibId( libId );
    m_symbols.insert( { aIndex, kpart } );

    // each component has its own symbol for now
    SCH_COMPONENT* component = new SCH_COMPONENT();

    component->SetPosition( elem.location + m_sheetOffset );
    //component->SetOrientation( elem.orientation ); // TODO: keep it simple for now, and only set position
    component->SetLibId( libId );
    //component->SetLibSymbol( kpart ); // this has to be done after parsing the LIB_PART!

    component->SetUnit( elem.currentpartid );

    m_currentSheet->GetScreen()->Append( component );

    m_components.insert( { aIndex, component } );
}


void SCH_ALTIUM_PLUGIN::ParsePin( const std::map<wxString, wxString>& aProperties )
{
    ASCH_PIN elem( aProperties );

    const auto& symbol = m_symbols.find( elem.ownerindex );

    if( symbol == m_symbols.end() )
    {
        // TODO: e.g. can depend on Template (RECORD=39
        wxLogWarning( wxString::Format( "Pin has non-existent ownerindex %d",
                                        elem.ownerindex ) );
        return;
    }

    if( !IsComponentPartVisible( elem.ownerindex, elem.ownerpartdisplaymode ) )
        return;

    const auto& component = m_components.at( symbol->first );

    LIB_PIN* pin = new LIB_PIN( symbol->second );
    symbol->second->AddDrawItem( pin );

    pin->SetUnit( elem.ownerpartid );

    pin->SetName( elem.name );
    pin->SetNumber( elem.designator );
    pin->SetLength( elem.pinlength );

    wxPoint pinLocation = elem.location; // the location given is not the connection point!

    switch( elem.orientation )
    {
    case ASCH_RECORD_ORIENTATION::RIGHTWARDS:
        pin->SetOrientation( DrawPinOrient::PIN_LEFT );
        pinLocation.x += elem.pinlength;
        break;
    case ASCH_RECORD_ORIENTATION::UPWARDS:
        pin->SetOrientation( DrawPinOrient::PIN_DOWN );
        pinLocation.y -= elem.pinlength;
        break;
    case ASCH_RECORD_ORIENTATION::LEFTWARDS:
        pin->SetOrientation( DrawPinOrient::PIN_RIGHT );
        pinLocation.x -= elem.pinlength;
        break;
    case ASCH_RECORD_ORIENTATION::DOWNWARDS:
        pin->SetOrientation( DrawPinOrient::PIN_UP );
        pinLocation.y += elem.pinlength;
        break;
    default:
        wxLogWarning( "Pin has unexpected orientation" );
        break;
    }

    // TODO: position can be sometimes off a little bit!
    pin->SetPosition( GetRelativePosition( pinLocation + m_sheetOffset, component ) );
    // TODO: the following fix is even worse for now?
    // pin->SetPosition( GetRelativePosition( elem.kicadLocation, component ) );

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
        wxLogWarning( "Pin has unexpected electrical type" );
        break;
    }

    if( elem.symbolOuterEdge == ASCH_PIN_SYMBOL_OUTEREDGE::UNKNOWN )
        wxLogWarning( "Pin has unexpected outer edge type" );

    if( elem.symbolInnerEdge == ASCH_PIN_SYMBOL_INNEREDGE::UNKNOWN )
        wxLogWarning( "Pin has unexpected inner edge type" );

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


void SetEdaTextJustification( EDA_TEXT* text, ASCH_LABEL_JUSTIFICATION justification )
{
    switch( justification )
    {
    default:
    case ASCH_LABEL_JUSTIFICATION::UNKNOWN:
    case ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT:
    case ASCH_LABEL_JUSTIFICATION::BOTTOM_CENTER:
    case ASCH_LABEL_JUSTIFICATION::BOTTOM_RIGHT:
        text->SetVertJustify( EDA_TEXT_VJUSTIFY_T::GR_TEXT_VJUSTIFY_BOTTOM );
        break;
    case ASCH_LABEL_JUSTIFICATION::CENTER_LEFT:
    case ASCH_LABEL_JUSTIFICATION::CENTER_CENTER:
    case ASCH_LABEL_JUSTIFICATION::CENTER_RIGHT:
        text->SetVertJustify( EDA_TEXT_VJUSTIFY_T::GR_TEXT_VJUSTIFY_CENTER );
        break;
    case ASCH_LABEL_JUSTIFICATION::TOP_LEFT:
    case ASCH_LABEL_JUSTIFICATION::TOP_CENTER:
    case ASCH_LABEL_JUSTIFICATION::TOP_RIGHT:
        text->SetVertJustify( EDA_TEXT_VJUSTIFY_T::GR_TEXT_VJUSTIFY_TOP );
        break;
    }

    switch( justification )
    {
    default:
    case ASCH_LABEL_JUSTIFICATION::UNKNOWN:
    case ASCH_LABEL_JUSTIFICATION::BOTTOM_LEFT:
    case ASCH_LABEL_JUSTIFICATION::CENTER_LEFT:
    case ASCH_LABEL_JUSTIFICATION::TOP_LEFT:
        text->SetHorizJustify( EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_LEFT );
        break;
    case ASCH_LABEL_JUSTIFICATION::BOTTOM_CENTER:
    case ASCH_LABEL_JUSTIFICATION::CENTER_CENTER:
    case ASCH_LABEL_JUSTIFICATION::TOP_CENTER:
        text->SetHorizJustify( EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_CENTER );
        break;
    case ASCH_LABEL_JUSTIFICATION::BOTTOM_RIGHT:
    case ASCH_LABEL_JUSTIFICATION::CENTER_RIGHT:
    case ASCH_LABEL_JUSTIFICATION::TOP_RIGHT:
        text->SetHorizJustify( EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_RIGHT );
        break;
    }
}


void SCH_ALTIUM_PLUGIN::ParseLabel( const std::map<wxString, wxString>& aProperties )
{
    ASCH_LABEL elem( aProperties );

    // TODO: text variable support
    if( elem.ownerpartid == ALTIUM_COMPONENT_NONE )
    {
        SCH_TEXT* text = new SCH_TEXT( elem.location + m_sheetOffset, elem.text );

        SetEdaTextJustification( text, elem.justification );

        size_t fontId = static_cast<int>( elem.fontId );

        if( m_altiumSheet && fontId > 0 && fontId <= m_altiumSheet->fonts.size() )
        {
            const ASCH_SHEET_FONT& font = m_altiumSheet->fonts.at( fontId - 1 );
            text->SetItalic( font.italic );
            text->SetBold( font.bold );
            text->SetTextSize( { font.size / 2, font.size / 2 } );
        }

        text->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( text );
    }
    else
    {
        const auto& symbol = m_symbols.find( elem.ownerindex );

        if( symbol == m_symbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            wxLogWarning( wxString::Format( "Label has non-existent ownerindex %d",
                                            elem.ownerindex ) );
            return;
        }

        const auto& component = m_components.at( symbol->first );

        LIB_TEXT* text = new LIB_TEXT( symbol->second );
        symbol->second->AddDrawItem( text );

        text->SetUnit( elem.ownerpartid );

        text->SetPosition( GetRelativePosition( elem.location + m_sheetOffset, component ) );
        text->SetText( elem.text );
        SetEdaTextJustification( text, elem.justification );

        size_t fontId = static_cast<int>( elem.fontId );

        if( m_altiumSheet && fontId > 0 && fontId <= m_altiumSheet->fonts.size() )
        {
            const ASCH_SHEET_FONT& font = m_altiumSheet->fonts.at( fontId - 1 );
            text->SetItalic( font.italic );
            text->SetBold( font.bold );
            text->SetTextSize( { font.size / 2, font.size / 2 } );
        }
    }
}


void SCH_ALTIUM_PLUGIN::ParseBezier( const std::map<wxString, wxString>& aProperties )
{
    ASCH_BEZIER elem( aProperties );

    if( elem.points.size() < 2 )
    {
        wxLogWarning( wxString::Format( "Bezier has %d control points. At least 2 are expected.",
                                        static_cast<int>( elem.points.size() ) ) );
        return;
    }

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
                line->SetLineWidth( elem.lineWidth );
                line->SetLineStyle( PLOT_DASH_TYPE::SOLID );

                line->SetFlags( IS_NEW );
                m_currentSheet->GetScreen()->Append( line );
            }
            else
            {
                // simulate bezier using line segments
                std::vector<wxPoint> bezierPoints;
                std::vector<wxPoint> polyPoints;
                for( size_t j = i; j < elem.points.size() && j < i + 4; j++ )
                {
                    bezierPoints.push_back( elem.points.at( j ) + m_sheetOffset );
                }

                BEZIER_POLY converter( bezierPoints );
                converter.GetPoly( polyPoints );

                for( size_t k = 0; k + 1 < polyPoints.size(); k++ )
                {
                    SCH_LINE* line = new SCH_LINE( polyPoints.at( k ) + m_sheetOffset,
                                                   SCH_LAYER_ID::LAYER_NOTES );

                    line->SetEndPoint( polyPoints.at( k + 1 ) + m_sheetOffset );
                    line->SetLineWidth( elem.lineWidth );

                    line->SetFlags( IS_NEW );
                    m_currentSheet->GetScreen()->Append( line );
                }
            }
        }
    }
    else
    {
        const auto& symbol = m_symbols.find( elem.ownerindex );

        if( symbol == m_symbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            wxLogWarning( wxString::Format( "Bezier has non-existent ownerindex %d",
                                            elem.ownerindex ) );
            return;
        }

        if( !IsComponentPartVisible( elem.ownerindex, elem.ownerpartdisplaymode ) )
            return;

        const auto& component = m_components.at( symbol->first );

        for( size_t i = 0; i + 1 < elem.points.size(); i += 3 )
        {
            if( i + 2 == elem.points.size() )
            {
                // special case: single line
                LIB_POLYLINE* line = new LIB_POLYLINE( symbol->second );
                symbol->second->AddDrawItem( line );

                line->SetUnit( elem.ownerpartid );

                for( size_t j = i; j < elem.points.size() && j < i + 2; j++ )
                {
                    line->AddPoint( GetRelativePosition( elem.points.at( j ) + m_sheetOffset,
                                                         component ) );
                }

                line->SetWidth( elem.lineWidth );
            }
            else
            {
                // bezier always has maximum of 4 control points
                LIB_BEZIER* bezier = new LIB_BEZIER( symbol->second );
                symbol->second->AddDrawItem( bezier );

                bezier->SetUnit( elem.ownerpartid );

                for( size_t j = i; j < elem.points.size() && j < i + 4; j++ )
                {
                    bezier->AddPoint( GetRelativePosition( elem.points.at( j ) + m_sheetOffset,
                                      component ) );
                }

                bezier->SetWidth( elem.lineWidth );
            }
        }
    }
}


void SCH_ALTIUM_PLUGIN::ParsePolyline( const std::map<wxString, wxString>& aProperties )
{
    ASCH_POLYLINE elem( aProperties );

    if( elem.ownerpartid == ALTIUM_COMPONENT_NONE )
    {
        PLOT_DASH_TYPE dashType = PLOT_DASH_TYPE::DEFAULT;
        switch( elem.linestyle )
        {
        default:
        case ASCH_POLYLINE_LINESTYLE::SOLID:       dashType = PLOT_DASH_TYPE::SOLID;   break;
        case ASCH_POLYLINE_LINESTYLE::DASHED:      dashType = PLOT_DASH_TYPE::DASH;    break;
        case ASCH_POLYLINE_LINESTYLE::DOTTED:      dashType = PLOT_DASH_TYPE::DOT;     break;
        case ASCH_POLYLINE_LINESTYLE::DASH_DOTTED: dashType = PLOT_DASH_TYPE::DASHDOT; break;
        }

        for( size_t i = 0; i + 1 < elem.points.size(); i++ )
        {
            SCH_LINE* line = new SCH_LINE( elem.points.at( i ) + m_sheetOffset,
                                           SCH_LAYER_ID::LAYER_NOTES );

            line->SetEndPoint( elem.points.at( i + 1 ) + m_sheetOffset );
            line->SetLineWidth( elem.lineWidth );
            line->SetLineStyle( dashType );

            line->SetFlags( IS_NEW );
            m_currentSheet->GetScreen()->Append( line );
        }
    }
    else
    {
        const auto& symbol = m_symbols.find( elem.ownerindex );
        if( symbol == m_symbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            wxLogWarning( wxString::Format( "Polyline has non-existent ownerindex %d",
                                            elem.ownerindex ) );
            return;
        }

        if( !IsComponentPartVisible( elem.ownerindex, elem.ownerpartdisplaymode ) )
            return;

        const auto& component = m_components.at( symbol->first );

        LIB_POLYLINE* line = new LIB_POLYLINE( symbol->second );
        symbol->second->AddDrawItem( line );

        line->SetUnit( elem.ownerpartid );

        for( wxPoint& point : elem.points )
        {
            line->AddPoint( GetRelativePosition( point + m_sheetOffset, component ) );
        }

        line->SetWidth( elem.lineWidth );
    }
}


void SCH_ALTIUM_PLUGIN::ParsePolygon( const std::map<wxString, wxString>& aProperties )
{
    ASCH_POLYGON elem( aProperties );

    if( elem.ownerpartid == ALTIUM_COMPONENT_NONE )
    {
        // TODO: we cannot fill this polygon, only draw it for now
        for( size_t i = 0; i + 1 < elem.points.size(); i++ )
        {
            SCH_LINE* line =
                    new SCH_LINE( elem.points.at( i ) + m_sheetOffset, SCH_LAYER_ID::LAYER_NOTES );
            line->SetEndPoint( elem.points.at( i + 1 ) + m_sheetOffset );
            line->SetLineWidth( elem.lineWidth );
            line->SetLineStyle( PLOT_DASH_TYPE::SOLID );

            line->SetFlags( IS_NEW );
            m_currentSheet->GetScreen()->Append( line );
        }

        // close polygon
        SCH_LINE* line =
                new SCH_LINE( elem.points.front() + m_sheetOffset, SCH_LAYER_ID::LAYER_NOTES );
        line->SetEndPoint( elem.points.back() + m_sheetOffset );
        line->SetLineWidth( elem.lineWidth );
        line->SetLineStyle( PLOT_DASH_TYPE::SOLID );

        line->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( line );
    }
    else
    {
        const auto& symbol = m_symbols.find( elem.ownerindex );

        if( symbol == m_symbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            wxLogWarning( wxString::Format( "Polygon has non-existent ownerindex %d",
                                            elem.ownerindex ) );
            return;
        }

        if( !IsComponentPartVisible( elem.ownerindex, elem.ownerpartdisplaymode ) )
            return;

        const auto& component = m_components.at( symbol->first );

        LIB_POLYLINE* line = new LIB_POLYLINE( symbol->second );
        symbol->second->AddDrawItem( line );

        line->SetUnit( elem.ownerpartid );

        for( wxPoint& point : elem.points )
            line->AddPoint( GetRelativePosition( point + m_sheetOffset, component ) );

        line->AddPoint( GetRelativePosition( elem.points.front() + m_sheetOffset, component ) );

        line->SetWidth( elem.lineWidth );

        if( !elem.isSolid )
            line->SetFillMode( FILL_TYPE::NO_FILL );
        else if( elem.color == elem.areacolor )
            line->SetFillMode( FILL_TYPE::FILLED_SHAPE );
        else
            line->SetFillMode( FILL_TYPE::FILLED_WITH_BG_BODYCOLOR );
    }
}


void SCH_ALTIUM_PLUGIN::ParseRoundRectangle( const std::map<wxString, wxString>& aProperties )
{
    ASCH_ROUND_RECTANGLE elem( aProperties );

    wxPoint sheetTopRight   = elem.topRight + m_sheetOffset;
    wxPoint sheetBottomLeft = elem.bottomLeft + m_sheetOffset;

    if( elem.ownerpartid == ALTIUM_COMPONENT_NONE )
    {
        const wxPoint topLeft     = { sheetBottomLeft.x, sheetTopRight.y };
        const wxPoint bottomRight = { sheetTopRight.x, sheetBottomLeft.y };

        // TODO: we cannot fill this rectangle, only draw it for now
        // TODO: misses rounded edges
        SCH_LINE* lineTop = new SCH_LINE( sheetTopRight, SCH_LAYER_ID::LAYER_NOTES );
        lineTop->SetEndPoint( topLeft );
        lineTop->SetLineWidth( elem.lineWidth );
        lineTop->SetLineStyle( PLOT_DASH_TYPE::SOLID );
        lineTop->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( lineTop );

        SCH_LINE* lineBottom = new SCH_LINE( sheetBottomLeft, SCH_LAYER_ID::LAYER_NOTES );
        lineBottom->SetEndPoint( bottomRight );
        lineBottom->SetLineWidth( elem.lineWidth );
        lineBottom->SetLineStyle( PLOT_DASH_TYPE::SOLID );
        lineBottom->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( lineBottom );

        SCH_LINE* lineRight = new SCH_LINE( sheetTopRight, SCH_LAYER_ID::LAYER_NOTES );
        lineRight->SetEndPoint( bottomRight );
        lineRight->SetLineWidth( elem.lineWidth );
        lineRight->SetLineStyle( PLOT_DASH_TYPE::SOLID );
        lineRight->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( lineRight );

        SCH_LINE* lineLeft = new SCH_LINE( sheetBottomLeft, SCH_LAYER_ID::LAYER_NOTES );
        lineLeft->SetEndPoint( topLeft );
        lineLeft->SetLineWidth( elem.lineWidth );
        lineLeft->SetLineStyle( PLOT_DASH_TYPE::SOLID );
        lineLeft->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( lineLeft );
    }
    else
    {
        const auto& symbol = m_symbols.find( elem.ownerindex );

        if( symbol == m_symbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            wxLogWarning( wxString::Format( "Rounded Rectangle has non-existent ownerindex %d",
                                            elem.ownerindex ) );
            return;
        }

        if( !IsComponentPartVisible( elem.ownerindex, elem.ownerpartdisplaymode ) )
            return;

        const auto& component = m_components.at( symbol->first );

        // TODO: misses rounded edges
        LIB_RECTANGLE* rect = new LIB_RECTANGLE( symbol->second );
        symbol->second->AddDrawItem( rect );

        rect->SetUnit( elem.ownerpartid );

        rect->SetPosition( GetRelativePosition( elem.topRight + m_sheetOffset, component ) );
        rect->SetEnd( GetRelativePosition( elem.bottomLeft + m_sheetOffset, component ) );
        rect->SetWidth( elem.lineWidth );

        if( !elem.isSolid )
            rect->SetFillMode( FILL_TYPE::NO_FILL );
        else if( elem.color == elem.areacolor )
            rect->SetFillMode( FILL_TYPE::FILLED_SHAPE );
        else
            rect->SetFillMode( FILL_TYPE::FILLED_WITH_BG_BODYCOLOR );
    }
}


void SCH_ALTIUM_PLUGIN::ParseArc( const std::map<wxString, wxString>& aProperties )
{
    ASCH_ARC elem( aProperties );

    if( elem.ownerpartid == ALTIUM_COMPONENT_NONE )
    {
        wxLogError( "Arc drawing is not possible for now on schematic." );
    }
    else
    {
        const auto& symbol = m_symbols.find( elem.ownerindex );
        if( symbol == m_symbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            wxLogWarning( wxString::Format( "Arc has non-existent ownerindex %d",
                                            elem.ownerindex ) );
            return;
        }

        if( !IsComponentPartVisible( elem.ownerindex, elem.ownerpartdisplaymode ) )
            return;

        const auto& component = m_components.at( symbol->first );

        if( elem.startAngle == 0 && ( elem.endAngle == 0 || elem.endAngle == 360 ) )
        {
            LIB_CIRCLE* circle = new LIB_CIRCLE( symbol->second );
            symbol->second->AddDrawItem( circle );

            circle->SetUnit( elem.ownerpartid );

            circle->SetPosition( GetRelativePosition( elem.center + m_sheetOffset, component ) );
            circle->SetRadius( elem.radius );
            circle->SetWidth( elem.lineWidth );
        }
        else
        {
            LIB_ARC* arc = new LIB_ARC( symbol->second );
            symbol->second->AddDrawItem( arc );

            arc->SetUnit( elem.ownerpartid );

            // TODO: correct?
            arc->SetPosition( GetRelativePosition( elem.center + m_sheetOffset, component ) );
            arc->SetRadius( elem.radius );
            arc->SetFirstRadiusAngle( elem.startAngle * 10. );
            arc->SetSecondRadiusAngle( elem.endAngle * 10. );
        }
    }
}


void SCH_ALTIUM_PLUGIN::ParseLine( const std::map<wxString, wxString>& aProperties )
{
    ASCH_LINE elem( aProperties );

    if( elem.ownerpartid == ALTIUM_COMPONENT_NONE )
    {
        // close polygon
        SCH_LINE* line = new SCH_LINE( elem.point1 + m_sheetOffset, SCH_LAYER_ID::LAYER_NOTES );
        line->SetEndPoint( elem.point2 + m_sheetOffset );
        line->SetLineWidth( elem.lineWidth );
        line->SetLineStyle( PLOT_DASH_TYPE::SOLID ); // TODO?

        line->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( line );
    }
    else
    {
        const auto& symbol = m_symbols.find( elem.ownerindex );

        if( symbol == m_symbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            wxLogWarning( wxString::Format( "Line has non-existent ownerindex %d",
                                            elem.ownerindex ) );
            return;
        }

        if( !IsComponentPartVisible( elem.ownerindex, elem.ownerpartdisplaymode ) )
            return;

        const auto& component = m_components.at( symbol->first );

        LIB_POLYLINE* line = new LIB_POLYLINE( symbol->second );
        symbol->second->AddDrawItem( line );

        line->SetUnit( elem.ownerpartid );

        line->AddPoint( GetRelativePosition( elem.point1 + m_sheetOffset, component ) );
        line->AddPoint( GetRelativePosition( elem.point2 + m_sheetOffset, component ) );

        line->SetWidth( elem.lineWidth );
    }
}


void SCH_ALTIUM_PLUGIN::ParseRectangle( const std::map<wxString, wxString>& aProperties )
{
    ASCH_RECTANGLE elem( aProperties );

    wxPoint sheetTopRight   = elem.topRight + m_sheetOffset;
    wxPoint sheetBottomLeft = elem.bottomLeft + m_sheetOffset;

    if( elem.ownerpartid == ALTIUM_COMPONENT_NONE )
    {
        const wxPoint topLeft     = { sheetBottomLeft.x, sheetTopRight.y };
        const wxPoint bottomRight = { sheetTopRight.x, sheetBottomLeft.y };

        // TODO: we cannot fill this rectangle, only draw it for now
        SCH_LINE* lineTop = new SCH_LINE( sheetTopRight, SCH_LAYER_ID::LAYER_NOTES );
        lineTop->SetEndPoint( topLeft );
        lineTop->SetLineWidth( elem.lineWidth );
        lineTop->SetLineStyle( PLOT_DASH_TYPE::SOLID );
        lineTop->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( lineTop );

        SCH_LINE* lineBottom = new SCH_LINE( sheetBottomLeft, SCH_LAYER_ID::LAYER_NOTES );
        lineBottom->SetEndPoint( bottomRight );
        lineBottom->SetLineWidth( elem.lineWidth );
        lineBottom->SetLineStyle( PLOT_DASH_TYPE::SOLID );
        lineBottom->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( lineBottom );

        SCH_LINE* lineRight = new SCH_LINE( sheetTopRight, SCH_LAYER_ID::LAYER_NOTES );
        lineRight->SetEndPoint( bottomRight );
        lineRight->SetLineWidth( elem.lineWidth );
        lineRight->SetLineStyle( PLOT_DASH_TYPE::SOLID );
        lineRight->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( lineRight );

        SCH_LINE* lineLeft = new SCH_LINE( sheetBottomLeft, SCH_LAYER_ID::LAYER_NOTES );
        lineLeft->SetEndPoint( topLeft );
        lineLeft->SetLineWidth( elem.lineWidth );
        lineLeft->SetLineStyle( PLOT_DASH_TYPE::SOLID );
        lineLeft->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( lineLeft );
    }
    else
    {
        const auto& symbol = m_symbols.find( elem.ownerindex );

        if( symbol == m_symbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            wxLogWarning( wxString::Format( "Rectangle has non-existent ownerindex %d",
                                            elem.ownerindex ) );
            return;
        }

        if( !IsComponentPartVisible( elem.ownerindex, elem.ownerpartdisplaymode ) )
            return;

        const auto& component = m_components.at( symbol->first );

        LIB_RECTANGLE* rect = new LIB_RECTANGLE( symbol->second );
        symbol->second->AddDrawItem( rect );

        rect->SetUnit( elem.ownerpartid );

        rect->SetPosition( GetRelativePosition( sheetTopRight, component ) );
        rect->SetEnd( GetRelativePosition( sheetBottomLeft, component ) );
        rect->SetWidth( elem.lineWidth );

        if( !elem.isSolid )
            rect->SetFillMode( FILL_TYPE::NO_FILL );
        else if( elem.color == elem.areacolor )
            rect->SetFillMode( FILL_TYPE::FILLED_SHAPE );
        else
            rect->SetFillMode( FILL_TYPE::FILLED_WITH_BG_BODYCOLOR );
    }
}


void SCH_ALTIUM_PLUGIN::ParseSheetSymbol(
        int aIndex, const std::map<wxString, wxString>& aProperties )
{
    ASCH_SHEET_SYMBOL elem( aProperties );

    SCH_SHEET*  sheet  = new SCH_SHEET( m_currentSheet, elem.location + m_sheetOffset );
    SCH_SCREEN* screen = new SCH_SCREEN( m_schematic );

    sheet->SetSize( elem.size );

    sheet->SetBorderColor( GetColorFromInt( elem.color ) );
    if( elem.isSolid )
        sheet->SetBackgroundColor( GetColorFromInt( elem.areacolor ) );

    sheet->SetScreen( screen );

    sheet->SetFlags( IS_NEW );
    m_currentSheet->GetScreen()->Append( sheet );

    m_sheets.insert( { aIndex, sheet } );
}


void SCH_ALTIUM_PLUGIN::ParseSheetEntry( const std::map<wxString, wxString>& aProperties )
{
    ASCH_SHEET_ENTRY elem( aProperties );

    const auto& sheet = m_sheets.find( elem.ownerindex );
    if( sheet == m_sheets.end() )
    {
        wxLogError( wxString::Format( "Sheet Entry has non-existent ownerindex %d",
                                      elem.ownerindex ) );
        return;
    }

    SCH_SHEET_PIN* sheetPin = new SCH_SHEET_PIN( sheet->second );
    sheet->second->AddPin( sheetPin );

    sheetPin->SetText( elem.name );
    sheetPin->SetShape( PINSHEETLABEL_SHAPE::PS_UNSPECIFIED );
    //sheetPin->SetLabelSpinStyle( getSpinStyle( term.OrientAngle, false ) );
    //sheetPin->SetPosition( getKiCadPoint( term.Position ) );

    wxPoint pos  = sheet->second->GetPosition();
    wxSize  size = sheet->second->GetSize();

    switch( elem.side )
    {
    default:
    case ASCH_SHEET_ENTRY_SIDE::LEFT:
        sheetPin->SetPosition( { pos.x, pos.y + elem.distanceFromTop } );
        sheetPin->SetLabelSpinStyle( LABEL_SPIN_STYLE::LEFT );
        sheetPin->SetEdge( SHEET_SIDE::SHEET_LEFT_SIDE );
        break;
    case ASCH_SHEET_ENTRY_SIDE::RIGHT:
        sheetPin->SetPosition( { pos.x + size.x, pos.y + elem.distanceFromTop } );
        sheetPin->SetLabelSpinStyle( LABEL_SPIN_STYLE::RIGHT );
        sheetPin->SetEdge( SHEET_SIDE::SHEET_RIGHT_SIDE );
        break;
    case ASCH_SHEET_ENTRY_SIDE::TOP:
        sheetPin->SetPosition( { pos.x + elem.distanceFromTop, pos.y } );
        sheetPin->SetLabelSpinStyle( LABEL_SPIN_STYLE::UP );
        sheetPin->SetEdge( SHEET_SIDE::SHEET_TOP_SIDE );
        break;
    case ASCH_SHEET_ENTRY_SIDE::BOTTOM:
        sheetPin->SetPosition( { pos.x + elem.distanceFromTop, pos.y + size.y } );
        sheetPin->SetLabelSpinStyle( LABEL_SPIN_STYLE::BOTTOM );
        sheetPin->SetEdge( SHEET_SIDE::SHEET_BOTTOM_SIDE );
        break;
    }

    switch( elem.iotype )
    {
    default:
    case ASCH_PORT_IOTYPE::UNSPECIFIED:
        sheetPin->SetShape( PINSHEETLABEL_SHAPE::PS_UNSPECIFIED );
        break;
    case ASCH_PORT_IOTYPE::OUTPUT:
        sheetPin->SetShape( PINSHEETLABEL_SHAPE::PS_OUTPUT );
        break;
    case ASCH_PORT_IOTYPE::INPUT:
        sheetPin->SetShape( PINSHEETLABEL_SHAPE::PS_INPUT );
        break;
    case ASCH_PORT_IOTYPE::BIDI:
        sheetPin->SetShape( PINSHEETLABEL_SHAPE::PS_BIDI );
        break;
    }
}


wxPoint HelperGeneratePowerPortGraphics( LIB_PART* aKPart, ASCH_POWER_PORT_STYLE aStyle )
{
    if( aStyle == ASCH_POWER_PORT_STYLE::CIRCLE || aStyle == ASCH_POWER_PORT_STYLE::ARROW )
    {
        LIB_POLYLINE* line1 = new LIB_POLYLINE( aKPart );
        aKPart->AddDrawItem( line1 );
        line1->SetWidth( Mils2iu( 10 ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, Mils2iu( -50 ) } );

        if( aStyle == ASCH_POWER_PORT_STYLE::CIRCLE )
        {
            LIB_CIRCLE* circle = new LIB_CIRCLE( aKPart );
            aKPart->AddDrawItem( circle );
            circle->SetWidth( Mils2iu( 5 ) );
            circle->SetRadius( Mils2iu( 25 ) );
            circle->SetPosition( { Mils2iu( 0 ), Mils2iu( -75 ) } );
        }
        else
        {
            LIB_POLYLINE* line2 = new LIB_POLYLINE( aKPart );
            aKPart->AddDrawItem( line2 );
            line2->SetWidth( Mils2iu( 10 ) );
            line2->AddPoint( { Mils2iu( -25 ), Mils2iu( -50 ) } );
            line2->AddPoint( { Mils2iu( 25 ), Mils2iu( -50 ) } );
            line2->AddPoint( { Mils2iu( 0 ), Mils2iu( -100 ) } );
            line2->AddPoint( { Mils2iu( -25 ), Mils2iu( -50 ) } );
        }

        return { 0, Mils2iu( 150 ) };
    }
    else if( aStyle == ASCH_POWER_PORT_STYLE::WAVE )
    {
        LIB_POLYLINE* line = new LIB_POLYLINE( aKPart );
        aKPart->AddDrawItem( line );
        line->SetWidth( Mils2iu( 10 ) );
        line->AddPoint( { 0, 0 } );
        line->AddPoint( { 0, Mils2iu( -72 ) } );

        LIB_BEZIER* bezier = new LIB_BEZIER( aKPart );
        aKPart->AddDrawItem( bezier );
        bezier->SetWidth( Mils2iu( 5 ) );
        bezier->AddPoint( { Mils2iu( 30 ), Mils2iu( -50 ) } );
        bezier->AddPoint( { Mils2iu( 30 ), Mils2iu( -87 ) } );
        bezier->AddPoint( { Mils2iu( -30 ), Mils2iu( -63 ) } );
        bezier->AddPoint( { Mils2iu( -30 ), Mils2iu( -100 ) } );

        return { 0, Mils2iu( 150 ) };
    }
    else if( aStyle == ASCH_POWER_PORT_STYLE::POWER_GROUND
             || aStyle == ASCH_POWER_PORT_STYLE::SIGNAL_GROUND
             || aStyle == ASCH_POWER_PORT_STYLE::EARTH
             || aStyle == ASCH_POWER_PORT_STYLE::GOST_ARROW )
    {
        LIB_POLYLINE* line1 = new LIB_POLYLINE( aKPart );
        aKPart->AddDrawItem( line1 );
        line1->SetWidth( Mils2iu( 10 ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, Mils2iu( -100 ) } );

        if( aStyle == ASCH_POWER_PORT_STYLE::POWER_GROUND )
        {
            LIB_POLYLINE* line2 = new LIB_POLYLINE( aKPart );
            aKPart->AddDrawItem( line2 );
            line2->SetWidth( Mils2iu( 10 ) );
            line2->AddPoint( { Mils2iu( -100 ), Mils2iu( -100 ) } );
            line2->AddPoint( { Mils2iu( 100 ), Mils2iu( -100 ) } );

            LIB_POLYLINE* line3 = new LIB_POLYLINE( aKPart );
            aKPart->AddDrawItem( line3 );
            line3->SetWidth( Mils2iu( 10 ) );
            line3->AddPoint( { Mils2iu( -70 ), Mils2iu( -130 ) } );
            line3->AddPoint( { Mils2iu( 70 ), Mils2iu( -130 ) } );

            LIB_POLYLINE* line4 = new LIB_POLYLINE( aKPart );
            aKPart->AddDrawItem( line4 );
            line4->SetWidth( Mils2iu( 10 ) );
            line4->AddPoint( { Mils2iu( -40 ), Mils2iu( -160 ) } );
            line4->AddPoint( { Mils2iu( 40 ), Mils2iu( -160 ) } );

            LIB_POLYLINE* line5 = new LIB_POLYLINE( aKPart );
            aKPart->AddDrawItem( line5 );
            line5->SetWidth( Mils2iu( 10 ) );
            line5->AddPoint( { Mils2iu( -10 ), Mils2iu( -190 ) } );
            line5->AddPoint( { Mils2iu( 10 ), Mils2iu( -190 ) } );
        }
        else if( aStyle == ASCH_POWER_PORT_STYLE::SIGNAL_GROUND )
        {
            LIB_POLYLINE* line2 = new LIB_POLYLINE( aKPart );
            aKPart->AddDrawItem( line2 );
            line2->SetWidth( Mils2iu( 10 ) );
            line2->AddPoint( { Mils2iu( -100 ), Mils2iu( -100 ) } );
            line2->AddPoint( { Mils2iu( 100 ), Mils2iu( -100 ) } );
            line2->AddPoint( { Mils2iu( 0 ), Mils2iu( -200 ) } );
            line2->AddPoint( { Mils2iu( -100 ), Mils2iu( -100 ) } );
        }
        else if( aStyle == ASCH_POWER_PORT_STYLE::EARTH )
        {
            LIB_POLYLINE* line2 = new LIB_POLYLINE( aKPart );
            aKPart->AddDrawItem( line2 );
            line2->SetWidth( Mils2iu( 10 ) );
            line2->AddPoint( { Mils2iu( -150 ), Mils2iu( -200 ) } );
            line2->AddPoint( { Mils2iu( -100 ), Mils2iu( -100 ) } );
            line2->AddPoint( { Mils2iu( 100 ), Mils2iu( -100 ) } );
            line2->AddPoint( { Mils2iu( 50 ), Mils2iu( -200 ) } );

            LIB_POLYLINE* line3 = new LIB_POLYLINE( aKPart );
            aKPart->AddDrawItem( line3 );
            line3->SetWidth( Mils2iu( 10 ) );
            line3->AddPoint( { Mils2iu( 0 ), Mils2iu( -100 ) } );
            line3->AddPoint( { Mils2iu( -50 ), Mils2iu( -200 ) } );
        }
        else // ASCH_POWER_PORT_STYLE::GOST_ARROW
        {
            LIB_POLYLINE* line2 = new LIB_POLYLINE( aKPart );
            aKPart->AddDrawItem( line2 );
            line2->SetWidth( Mils2iu( 10 ) );
            line2->AddPoint( { Mils2iu( -25 ), Mils2iu( -50 ) } );
            line2->AddPoint( { Mils2iu( 0 ), Mils2iu( -100 ) } );
            line2->AddPoint( { Mils2iu( 25 ), Mils2iu( -50 ) } );

            return { 0, Mils2iu( 150 ) }; // special case
        }

        return { 0, Mils2iu( 250 ) };
    }
    else if( aStyle == ASCH_POWER_PORT_STYLE::GOST_POWER_GROUND
             || aStyle == ASCH_POWER_PORT_STYLE::GOST_EARTH )
    {
        LIB_POLYLINE* line1 = new LIB_POLYLINE( aKPart );
        aKPart->AddDrawItem( line1 );
        line1->SetWidth( Mils2iu( 10 ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, Mils2iu( -160 ) } );

        LIB_POLYLINE* line2 = new LIB_POLYLINE( aKPart );
        aKPart->AddDrawItem( line2 );
        line2->SetWidth( Mils2iu( 10 ) );
        line2->AddPoint( { Mils2iu( -100 ), Mils2iu( -160 ) } );
        line2->AddPoint( { Mils2iu( 100 ), Mils2iu( -160 ) } );

        LIB_POLYLINE* line3 = new LIB_POLYLINE( aKPart );
        aKPart->AddDrawItem( line3 );
        line3->SetWidth( Mils2iu( 10 ) );
        line3->AddPoint( { Mils2iu( -60 ), Mils2iu( -200 ) } );
        line3->AddPoint( { Mils2iu( 60 ), Mils2iu( -200 ) } );

        LIB_POLYLINE* line4 = new LIB_POLYLINE( aKPart );
        aKPart->AddDrawItem( line4 );
        line4->SetWidth( Mils2iu( 10 ) );
        line4->AddPoint( { Mils2iu( -20 ), Mils2iu( -240 ) } );
        line4->AddPoint( { Mils2iu( 20 ), Mils2iu( -240 ) } );

        if( aStyle == ASCH_POWER_PORT_STYLE::GOST_POWER_GROUND )
            return { 0, Mils2iu( 300 ) };

        LIB_CIRCLE* circle = new LIB_CIRCLE( aKPart );
        aKPart->AddDrawItem( circle );
        circle->SetWidth( Mils2iu( 10 ) );
        circle->SetRadius( Mils2iu( 120 ) );
        circle->SetPosition( { Mils2iu( 0 ), Mils2iu( -160 ) } );

        return { 0, Mils2iu( 350 ) };
    }
    else if( aStyle == ASCH_POWER_PORT_STYLE::GOST_BAR )
    {
        LIB_POLYLINE* line1 = new LIB_POLYLINE( aKPart );
        aKPart->AddDrawItem( line1 );
        line1->SetWidth( Mils2iu( 10 ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, Mils2iu( -200 ) } );

        LIB_POLYLINE* line2 = new LIB_POLYLINE( aKPart );
        aKPart->AddDrawItem( line2 );
        line2->SetWidth( Mils2iu( 10 ) );
        line2->AddPoint( { Mils2iu( -100 ), Mils2iu( -200 ) } );
        line2->AddPoint( { Mils2iu( 100 ), Mils2iu( -200 ) } );

        return { 0, Mils2iu( 250 ) };
    }
    else
    {
        if( aStyle != ASCH_POWER_PORT_STYLE::BAR )
            wxLogWarning( "Power Port has unknown style, use bar instead. " );

        LIB_POLYLINE* line1 = new LIB_POLYLINE( aKPart );
        aKPart->AddDrawItem( line1 );
        line1->SetWidth( Mils2iu( 10 ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, Mils2iu( -100 ) } );

        LIB_POLYLINE* line2 = new LIB_POLYLINE( aKPart );
        aKPart->AddDrawItem( line2 );
        line2->SetWidth( Mils2iu( 10 ) );
        line2->AddPoint( { Mils2iu( -50 ), Mils2iu( -100 ) } );
        line2->AddPoint( { Mils2iu( 50 ), Mils2iu( -100 ) } );

        return { 0, Mils2iu( 150 ) };
    }
}


void SCH_ALTIUM_PLUGIN::ParsePowerPort( const std::map<wxString, wxString>& aProperties )
{
    ASCH_POWER_PORT elem( aProperties );

    LIB_ID libId = AltiumToKiCadLibID( getLibName(), elem.text );

    LIB_PART* kpart = nullptr;

    const auto& symbol = m_powerSymbols.find( elem.text );
    if( symbol != m_powerSymbols.end() )
    {
        kpart = symbol->second; // cache hit
    }
    else
    {
        kpart = new LIB_PART( wxEmptyString );
        kpart->SetPower();
        kpart->SetName( elem.text );
        kpart->GetReferenceField().SetText( "#PWR" );
        kpart->GetValueField().SetText( elem.text );
        kpart->GetValueField().SetVisible( true ); // TODO: why does this not work?
        kpart->SetDescription( wxString::Format( _( "Power symbol creates a global label with name '%s'" ),
                                                 elem.text ) );
        kpart->SetKeyWords( "power-flag" );
        kpart->SetLibId( libId );

        // generate graphic
        LIB_PIN* pin = new LIB_PIN( kpart );
        kpart->AddDrawItem( pin );

        pin->SetName( elem.text );
        pin->SetPosition( { 0, 0 } );
        pin->SetLength( 0 );

        // marks the pin as a global label
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
        pin->SetVisible( false );

        wxPoint valueFieldPos = HelperGeneratePowerPortGraphics( kpart, elem.style );

        kpart->GetValueField().SetPosition( valueFieldPos );

        // this has to be done after parsing the LIB_PART!
        m_pi->SaveSymbol( getLibFileName().GetFullPath(), kpart, m_properties.get() );
        m_powerSymbols.insert( { elem.text, kpart } );
    }

    SCH_SHEET_PATH sheetpath;
    m_rootSheet->LocatePathOfScreen( m_currentSheet->GetScreen(), &sheetpath );

    // each component has its own symbol for now
    SCH_COMPONENT* component = new SCH_COMPONENT();
    component->SetRef( &sheetpath, "#PWR?" );
    component->SetValue( elem.text );
    component->SetLibId( libId );
    component->SetLibSymbol( new LIB_PART( *kpart ) );

    SCH_FIELD* valueField = component->GetField( VALUE_FIELD );

    // TODO: Why do I need to set those a second time?
    valueField->SetVisible( true );
    valueField->SetPosition( kpart->GetValueField().GetPosition() );

    component->SetPosition( elem.location + m_sheetOffset );

    switch( elem.orientation )
    {
    case ASCH_RECORD_ORIENTATION::RIGHTWARDS:
        component->SetOrientation( COMPONENT_ORIENTATION_T::CMP_ORIENT_90 );
        valueField->SetTextAngle( -900. );
        valueField->SetHorizJustify( EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_LEFT );
        break;
    case ASCH_RECORD_ORIENTATION::UPWARDS:
        component->SetOrientation( COMPONENT_ORIENTATION_T::CMP_ORIENT_180 );
        valueField->SetTextAngle( -1800. );
        valueField->SetHorizJustify( EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_CENTER );
        break;
    case ASCH_RECORD_ORIENTATION::LEFTWARDS:
        component->SetOrientation( COMPONENT_ORIENTATION_T::CMP_ORIENT_270 );
        valueField->SetTextAngle( -2700. );
        valueField->SetHorizJustify( EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_RIGHT );
        break;
    case ASCH_RECORD_ORIENTATION::DOWNWARDS:
        component->SetOrientation( COMPONENT_ORIENTATION_T::CMP_ORIENT_0 );
        valueField->SetTextAngle( 0. );
        valueField->SetHorizJustify( EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_CENTER );
        break;
    default:
        wxLogWarning( "Pin has unexpected orientation" );
        break;
    }

    m_currentSheet->GetScreen()->Append( component );
}


void SCH_ALTIUM_PLUGIN::ParsePort( const ASCH_PORT& aElem )
{
    // Get both connection points where we could connect to
    wxPoint start = aElem.location + m_sheetOffset;
    wxPoint end   = start;

    switch( aElem.style )
    {
    default:
    case ASCH_PORT_STYLE::NONE_HORIZONTAL:
    case ASCH_PORT_STYLE::LEFT:
    case ASCH_PORT_STYLE::RIGHT:
    case ASCH_PORT_STYLE::LEFT_RIGHT:
        end.x += aElem.width;
        break;
    case ASCH_PORT_STYLE::NONE_VERTICAL:
    case ASCH_PORT_STYLE::TOP:
    case ASCH_PORT_STYLE::BOTTOM:
    case ASCH_PORT_STYLE::TOP_BOTTOM:
        end.y -= aElem.width;
        break;
    }

    // Check which connection points exists in the schematic
    SCH_SCREEN* screen = m_currentSheet->GetScreen();

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
        wxLogError( wxString::Format( "There is a Port for \"%s\", but no connections towards it?",
                                      aElem.name ) );
    }

    // Select label position. In case both match, we will add a line later.
    wxPoint position = ( startIsWireTerminal || startIsBusTerminal ) ? start : end;

    SCH_TEXT* const label = new SCH_GLOBALLABEL( position, aElem.name );
    // TODO: detect correct label type depending on sheet settings, etc.
    // label = new SCH_HIERLABEL( elem.location + m_sheetOffset, elem.name );

    switch( aElem.iotype )
    {
    default:
    case ASCH_PORT_IOTYPE::UNSPECIFIED:
        label->SetShape( PINSHEETLABEL_SHAPE::PS_UNSPECIFIED );
        break;
    case ASCH_PORT_IOTYPE::OUTPUT:
        label->SetShape( PINSHEETLABEL_SHAPE::PS_OUTPUT );
        break;
    case ASCH_PORT_IOTYPE::INPUT:
        label->SetShape( PINSHEETLABEL_SHAPE::PS_INPUT );
        break;
    case ASCH_PORT_IOTYPE::BIDI:
        label->SetShape( PINSHEETLABEL_SHAPE::PS_BIDI );
        break;
    }

    switch( aElem.style )
    {
    default:
    case ASCH_PORT_STYLE::NONE_HORIZONTAL:
    case ASCH_PORT_STYLE::LEFT:
    case ASCH_PORT_STYLE::RIGHT:
    case ASCH_PORT_STYLE::LEFT_RIGHT:
        if( ( startIsWireTerminal || startIsBusTerminal ) )
            label->SetLabelSpinStyle( LABEL_SPIN_STYLE::RIGHT );
        else
            label->SetLabelSpinStyle( LABEL_SPIN_STYLE::LEFT );
        break;
    case ASCH_PORT_STYLE::NONE_VERTICAL:
    case ASCH_PORT_STYLE::TOP:
    case ASCH_PORT_STYLE::BOTTOM:
    case ASCH_PORT_STYLE::TOP_BOTTOM:
        if( ( startIsWireTerminal || startIsBusTerminal ) )
            label->SetLabelSpinStyle( LABEL_SPIN_STYLE::UP );
        else
            label->SetLabelSpinStyle( LABEL_SPIN_STYLE::BOTTOM );
        break;
    }

    label->SetFlags( IS_NEW );
    m_currentSheet->GetScreen()->Append( label );

    // This is a hack, for the case both connection points are valid: add a small wire
    if( ( startIsWireTerminal && endIsWireTerminal ) || !connectionFound )
    {
        SCH_LINE* wire = new SCH_LINE( start, SCH_LAYER_ID::LAYER_WIRE );
        wire->SetEndPoint( end );
        wire->SetLineWidth( Mils2iu( 2 ) );
        wire->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( wire );
    }
    else if( startIsBusTerminal && endIsBusTerminal )
    {
        SCH_LINE* wire = new SCH_LINE( start, SCH_LAYER_ID::LAYER_BUS );
        wire->SetEndPoint( end );
        wire->SetLineWidth( Mils2iu( 2 ) );
        wire->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( wire );
    }
}


void SCH_ALTIUM_PLUGIN::ParseNoERC( const std::map<wxString, wxString>& aProperties )
{
    ASCH_NO_ERC elem( aProperties );

    if( elem.isActive )
    {
        SCH_NO_CONNECT* noConnect = new SCH_NO_CONNECT( elem.location + m_sheetOffset );

        noConnect->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( noConnect );
    }
}


void SCH_ALTIUM_PLUGIN::ParseNetLabel( const std::map<wxString, wxString>& aProperties )
{
    ASCH_NET_LABEL elem( aProperties );

    SCH_LABEL* label = new SCH_LABEL( elem.location + m_sheetOffset, elem.text );

    switch( elem.orientation )
    {
    case ASCH_RECORD_ORIENTATION::RIGHTWARDS:
        label->SetLabelSpinStyle( LABEL_SPIN_STYLE::RIGHT );
        break;
    case ASCH_RECORD_ORIENTATION::UPWARDS:
        label->SetLabelSpinStyle( LABEL_SPIN_STYLE::UP );
        break;
    case ASCH_RECORD_ORIENTATION::LEFTWARDS:
        label->SetLabelSpinStyle( LABEL_SPIN_STYLE::LEFT );
        break;
    case ASCH_RECORD_ORIENTATION::DOWNWARDS:
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

    for( size_t i = 0; i + 1 < elem.points.size(); i++ )
    {
        SCH_LINE* bus =
                new SCH_LINE( elem.points.at( i ) + m_sheetOffset, SCH_LAYER_ID::LAYER_BUS );
        bus->SetEndPoint( elem.points.at( i + 1 ) + m_sheetOffset );
        bus->SetLineWidth( elem.lineWidth );

        bus->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( bus );
    }
}


void SCH_ALTIUM_PLUGIN::ParseWire( const std::map<wxString, wxString>& aProperties )
{
    ASCH_WIRE elem( aProperties );

    for( size_t i = 0; i + 1 < elem.points.size(); i++ )
    {
        SCH_LINE* wire =
                new SCH_LINE( elem.points.at( i ) + m_sheetOffset, SCH_LAYER_ID::LAYER_WIRE );
        wire->SetEndPoint( elem.points.at( i + 1 ) + m_sheetOffset );
        wire->SetLineWidth( elem.lineWidth );

        wire->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( wire );
    }
}


void SCH_ALTIUM_PLUGIN::ParseJunction( const std::map<wxString, wxString>& aProperties )
{
    ASCH_JUNCTION elem( aProperties );

    SCH_JUNCTION* junction = new SCH_JUNCTION( elem.location + m_sheetOffset );

    junction->SetFlags( IS_NEW );
    m_currentSheet->GetScreen()->Append( junction );
}


void SCH_ALTIUM_PLUGIN::ParseSheet( const std::map<wxString, wxString>& aProperties )
{
    m_altiumSheet = std::make_unique<ASCH_SHEET>( aProperties );

    PAGE_INFO pageInfo;

    bool isPortrait = m_altiumSheet->sheetOrientation == ASCH_SHEET_WORKSPACEORIENTATION::PORTRAIT;
    switch( m_altiumSheet->sheetSize )
    {
    default:
    case ASCH_SHEET_SIZE::A4:
        pageInfo.SetType( "A4", isPortrait );
        break;
    case ASCH_SHEET_SIZE::A3:
        pageInfo.SetType( "A3", isPortrait );
        break;
    case ASCH_SHEET_SIZE::A2:
        pageInfo.SetType( "A2", isPortrait );
        break;
    case ASCH_SHEET_SIZE::A1:
        pageInfo.SetType( "A1", isPortrait );
        break;
    case ASCH_SHEET_SIZE::A0:
        pageInfo.SetType( "A0", isPortrait );
        break;
    case ASCH_SHEET_SIZE::A:
        pageInfo.SetType( "A", isPortrait );
        break;
    case ASCH_SHEET_SIZE::B:
        pageInfo.SetType( "B", isPortrait );
        break;
    case ASCH_SHEET_SIZE::C:
        pageInfo.SetType( "C", isPortrait );
        break;
    case ASCH_SHEET_SIZE::D:
        pageInfo.SetType( "D", isPortrait );
        break;
    case ASCH_SHEET_SIZE::E:
        pageInfo.SetType( "E", isPortrait );
        break;
    case ASCH_SHEET_SIZE::LETTER:
        pageInfo.SetType( "USLetter", isPortrait );
        break;
    case ASCH_SHEET_SIZE::LEGAL:
        pageInfo.SetType( "USLegal", isPortrait );
        break;
    case ASCH_SHEET_SIZE::TABLOID:
        pageInfo.SetType( "A3", isPortrait ); // TODO: use User
        break;
    case ASCH_SHEET_SIZE::ORCAD_A:
        pageInfo.SetType( "A", isPortrait );
        break;
    case ASCH_SHEET_SIZE::ORCAD_B:
        pageInfo.SetType( "B", isPortrait );
        break;
    case ASCH_SHEET_SIZE::ORCAD_C:
        pageInfo.SetType( "C", isPortrait );
        break;
    case ASCH_SHEET_SIZE::ORCAD_D:
        pageInfo.SetType( "D", isPortrait );
        break;
    case ASCH_SHEET_SIZE::ORCAD_E:
        pageInfo.SetType( "E", isPortrait );
        break;
    }

    m_currentSheet->GetScreen()->SetPageSettings( pageInfo );

    m_sheetOffset = { 0, pageInfo.GetHeightIU() };
}


void SetFieldOrientation( SCH_FIELD& aField, ASCH_RECORD_ORIENTATION aOrientation )
{
    switch( aOrientation )
    {
    default:
    case ASCH_RECORD_ORIENTATION::RIGHTWARDS:
        aField.SetTextAngle( 0 );
        break;
    case ASCH_RECORD_ORIENTATION::UPWARDS:
        aField.SetTextAngle( 900 );
        break;
    case ASCH_RECORD_ORIENTATION::LEFTWARDS:
        aField.SetTextAngle( 1800 );
        break;
    case ASCH_RECORD_ORIENTATION::DOWNWARDS:
        aField.SetTextAngle( 2700 );
        break;
    }
}


void SCH_ALTIUM_PLUGIN::ParseSheetName( const std::map<wxString, wxString>& aProperties )
{
    ASCH_SHEET_NAME elem( aProperties );

    const auto& sheet = m_sheets.find( elem.ownerindex );
    if( sheet == m_sheets.end() )
    {
        wxLogError( wxString::Format( "Sheet Name has non-existent ownerindex %d",
                                      elem.ownerindex ) );
        return;
    }

    SCH_FIELD& sheetNameField = sheet->second->GetFields()[SHEETNAME];

    sheetNameField.SetPosition( elem.location + m_sheetOffset );
    sheetNameField.SetText( elem.text );
    sheetNameField.SetVisible( !elem.isHidden );

    sheetNameField.SetHorizJustify( EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_LEFT );
    sheetNameField.SetVertJustify( EDA_TEXT_VJUSTIFY_T::GR_TEXT_VJUSTIFY_BOTTOM );

    SetFieldOrientation( sheetNameField, elem.orientation );
}


void SCH_ALTIUM_PLUGIN::ParseFileName( const std::map<wxString, wxString>& aProperties )
{
    ASCH_FILE_NAME elem( aProperties );

    const auto& sheet = m_sheets.find( elem.ownerindex );
    if( sheet == m_sheets.end() )
    {
        wxLogError( wxString::Format( "File Name has non-existent ownerindex %d",
                                      elem.ownerindex ) );
        return;
    }

    SCH_FIELD& filenameField = sheet->second->GetFields()[SHEETFILENAME];

    filenameField.SetPosition( elem.location + m_sheetOffset );
    elem.text.RemoveLast( 6 );
    elem.text += "kicad_sch";
    filenameField.SetText( elem.text );
    filenameField.SetVisible( !elem.isHidden );

    filenameField.SetHorizJustify( EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_LEFT );
    filenameField.SetVertJustify( EDA_TEXT_VJUSTIFY_T::GR_TEXT_VJUSTIFY_BOTTOM );

    SetFieldOrientation( filenameField, elem.orientation );
}


void SCH_ALTIUM_PLUGIN::ParseDesignator( const std::map<wxString, wxString>& aProperties )
{
    ASCH_DESIGNATOR elem( aProperties );

    const auto& symbol = m_symbols.find( elem.ownerindex );
    if( symbol == m_symbols.end() )
    {
        // TODO: e.g. can depend on Template (RECORD=39
        wxLogWarning( wxString::Format( "Designator has non-existent ownerindex %d",
                                        elem.ownerindex ) );
        return;
    }

    const auto& component = m_components.at( symbol->first );

    SCH_SHEET_PATH sheetpath;
    m_rootSheet->LocatePathOfScreen( m_currentSheet->GetScreen(), &sheetpath );

    component->SetRef( &sheetpath, elem.text );

    SCH_FIELD* refField = component->GetField( REFERENCE_FIELD );

    refField->SetPosition( elem.location + m_sheetOffset );
    refField->SetVisible( true );

    refField->SetHorizJustify( EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_LEFT );
    refField->SetVertJustify( EDA_TEXT_VJUSTIFY_T::GR_TEXT_VJUSTIFY_BOTTOM );

    SetFieldOrientation( *refField, elem.orientation );
}


void SCH_ALTIUM_PLUGIN::ParseBusEntry( const std::map<wxString, wxString>& aProperties )
{
    ASCH_BUS_ENTRY elem( aProperties );

    SCH_BUS_WIRE_ENTRY* busWireEntry = new SCH_BUS_WIRE_ENTRY( elem.location + m_sheetOffset );

    wxPoint vector = elem.corner - elem.location;
    busWireEntry->SetSize( { vector.x, vector.y } );

    busWireEntry->SetFlags( IS_NEW );
    m_currentSheet->GetScreen()->Append( busWireEntry );
}


void SCH_ALTIUM_PLUGIN::ParseParameter( const std::map<wxString, wxString>& aProperties )
{
    ASCH_PARAMETER elem( aProperties );

    if( elem.ownerindex <= 0 && elem.ownerpartid == ALTIUM_COMPONENT_NONE )
    {
        // This is some sheet parameter
        if( elem.text == "*" )
            return; // indicates parameter not set?

        SCH_SHEET_PATH sheetpath;
        m_rootSheet->LocatePathOfScreen( m_currentSheet->GetScreen(), &sheetpath );

        if( elem.name == "SheetNumber" )
            m_rootSheet->SetPageNumber( sheetpath, elem.text );
        else if( elem.name == "Title" )
            m_currentTitleBlock->SetTitle( elem.text );
        else if( elem.name == "Revision" )
            m_currentTitleBlock->SetRevision( elem.text );
        else if( elem.name == "Date" )
            m_currentTitleBlock->SetDate( elem.text );
        else if( elem.name == "CompanyName" )
            m_currentTitleBlock->SetCompany( elem.text );
        // TODO: parse other parameters
        // TODO: handle parameters in labels
    }
    else
    {
        const auto& symbol = m_symbols.find( elem.ownerindex );
        if( symbol == m_symbols.end() )
        {
            // TODO: e.g. can depend on Template (RECORD=39
            return;
        }

        const auto& component = m_components.at( symbol->first );

        // TODO: location not correct?
        const wxPoint position = elem.location + m_sheetOffset;

        SCH_FIELD* field = nullptr;
        if( elem.name == "Value" )
        {
            field = component->GetField( VALUE_FIELD );
            field->SetPosition( position );
        }
        else
        {
            int fieldIdx = component->GetFieldCount();
            field = component->AddField( { position, fieldIdx, component, elem.name } );
        }

        // TODO: improve text replacement (https://gitlab.com/kicad/code/kicad/-/issues/6256)
        if( elem.text == "=Value" && field->GetId() != VALUE_FIELD )
            field->SetText( "${VALUE}" );
        else
            field->SetText( elem.text );

        field->SetVisible( !elem.isHidden );
        field->SetHorizJustify( EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_LEFT );

        switch( elem.orientation )
        {
        case ASCH_RECORD_ORIENTATION::RIGHTWARDS: field->SetTextAngle( 0 ); break;
        case ASCH_RECORD_ORIENTATION::UPWARDS: field->SetTextAngle( 90 ); break;
        case ASCH_RECORD_ORIENTATION::LEFTWARDS: field->SetTextAngle( 180 ); break;
        case ASCH_RECORD_ORIENTATION::DOWNWARDS: field->SetTextAngle( 270 ); break;
        default:
            break;
        }
    }
}
