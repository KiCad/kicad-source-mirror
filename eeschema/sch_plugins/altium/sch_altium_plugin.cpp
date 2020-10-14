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
#include <sch_component.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_no_connect.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_text.h>

#include "altium_parser_sch.h"
#include <bezier_curves.h>
#include <compoundfilereader.h>
#include <memory>
#include <plugins/altium/altium_parser.h>
#include <sch_plugins/legacy/sch_legacy_plugin.h>
#include <wildcards_and_files_ext.h>
#include <wx/textfile.h>


const wxPoint GetRelativePosition( const wxPoint& aPosition, const SCH_COMPONENT* aComponent )
{
    TRANSFORM t = aComponent->GetTransform().InverseTransform();
    return t.TransformCoordinate( aPosition - aComponent->GetPosition() );
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
            ParseJunction( properties );
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
    //component->SetOrientation( elem.orientation ); // TODO: keep it simple for now, and only set position
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

    pin->SetName( elem.name );
    pin->SetNumber( elem.designator );
    pin->SetLength( elem.pinlength );

    wxPoint pinLocation = elem.location; // the location given is not the connection point!
    switch( elem.orientation )
    {
    case ASCH_PIN_ORIENTATION::RIGHTWARDS:
        pin->SetOrientation( DrawPinOrient::PIN_LEFT );
        pinLocation.x += elem.pinlength;
        break;
    case ASCH_PIN_ORIENTATION::UPWARDS:
        pin->SetOrientation( DrawPinOrient::PIN_DOWN );
        pinLocation.y -= elem.pinlength;
        break;
    case ASCH_PIN_ORIENTATION::LEFTWARDS:
        pin->SetOrientation( DrawPinOrient::PIN_RIGHT );
        pinLocation.x -= elem.pinlength;
        break;
    case ASCH_PIN_ORIENTATION::DOWNWARDS:
        pin->SetOrientation( DrawPinOrient::PIN_UP );
        pinLocation.y += elem.pinlength;
        break;
    default:
        wxLogWarning( "Pin has unexpected orientation" );
        break;
    }

    // TODO: position can be sometimes off a little bit!
    pin->SetPosition( GetRelativePosition( pinLocation, component ) );
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
                SCH_LINE* line = new SCH_LINE( elem.points.at( i ), SCH_LAYER_ID::LAYER_NOTES );
                line->SetEndPoint( elem.points.at( i + 1 ) );
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
                    bezierPoints.push_back( elem.points.at( j ) );
                }

                BEZIER_POLY converter( bezierPoints );
                converter.GetPoly( polyPoints );

                for( size_t k = 0; k < polyPoints.size() - 1; k++ )
                {
                    SCH_LINE* line = new SCH_LINE( polyPoints.at( k ), SCH_LAYER_ID::LAYER_NOTES );
                    line->SetEndPoint( polyPoints.at( k + 1 ) );
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
            wxLogWarning( wxString::Format(
                    "Bezier tries to access symbol with ownerindex %d which does not exist",
                    elem.ownerindex ) );
            return;
        }

        const auto& component = m_components.at( symbol->first );

        for( size_t i = 0; i + 1 < elem.points.size(); i += 3 )
        {
            if( i + 2 == elem.points.size() )
            {
                // special case: single line
                LIB_POLYLINE* line = new LIB_POLYLINE( symbol->second );
                symbol->second->AddDrawItem( line );

                for( size_t j = i; j < elem.points.size() && j < i + 2; j++ )
                {
                    line->AddPoint( GetRelativePosition( elem.points.at( j ), component ) );
                }

                line->SetWidth( elem.lineWidth );
            }
            else
            {
                // bezier always has maximum of 4 control points
                LIB_BEZIER* bezier = new LIB_BEZIER( symbol->second );
                symbol->second->AddDrawItem( bezier );

                for( size_t j = i; j < elem.points.size() && j < i + 4; j++ )
                {
                    bezier->AddPoint( GetRelativePosition( elem.points.at( j ), component ) );
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
        case ASCH_POLYLINE_LINESTYLE::SOLID:
            dashType = PLOT_DASH_TYPE::SOLID;
            break;
        case ASCH_POLYLINE_LINESTYLE::DASHED:
            dashType = PLOT_DASH_TYPE::DASH;
            break;
        case ASCH_POLYLINE_LINESTYLE::DOTTED:
            dashType = PLOT_DASH_TYPE::DOT;
            break;
        case ASCH_POLYLINE_LINESTYLE::DASH_DOTTED:
            dashType = PLOT_DASH_TYPE::DASHDOT;
            break;
        }

        for( size_t i = 0; i < elem.points.size() - 1; i++ )
        {
            SCH_LINE* line = new SCH_LINE( elem.points.at( i ), SCH_LAYER_ID::LAYER_NOTES );
            line->SetEndPoint( elem.points.at( i + 1 ) );
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
            wxLogWarning( wxString::Format(
                    "Polyline tries to access symbol with ownerindex %d which does not exist",
                    elem.ownerindex ) );
            return;
        }

        const auto& component = m_components.at( symbol->first );

        LIB_POLYLINE* line = new LIB_POLYLINE( symbol->second );
        symbol->second->AddDrawItem( line );

        for( wxPoint& point : elem.points )
        {
            line->AddPoint( GetRelativePosition( point, component ) );
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
        for( size_t i = 0; i < elem.points.size() - 1; i++ )
        {
            SCH_LINE* line = new SCH_LINE( elem.points.at( i ), SCH_LAYER_ID::LAYER_NOTES );
            line->SetEndPoint( elem.points.at( i + 1 ) );
            line->SetLineWidth( elem.lineWidth );
            line->SetLineStyle( PLOT_DASH_TYPE::SOLID );

            line->SetFlags( IS_NEW );
            m_currentSheet->GetScreen()->Append( line );
        }

        // close polygon
        SCH_LINE* line = new SCH_LINE( elem.points.front(), SCH_LAYER_ID::LAYER_NOTES );
        line->SetEndPoint( elem.points.back() );
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
            wxLogWarning( wxString::Format(
                    "Polygon tries to access symbol with ownerindex %d which does not exist",
                    elem.ownerindex ) );
            return;
        }

        const auto& component = m_components.at( symbol->first );

        LIB_POLYLINE* line = new LIB_POLYLINE( symbol->second );
        symbol->second->AddDrawItem( line );

        for( wxPoint& point : elem.points )
        {
            line->AddPoint( GetRelativePosition( point, component ) );
        }
        line->AddPoint( GetRelativePosition( elem.points.front(), component ) );

        line->SetWidth( elem.lineWidth );

        if( !elem.isSolid )
            line->SetFillMode( FILL_TYPE::NO_FILL );
        else if( elem.color == elem.areacolor )
            line->SetFillMode( FILL_TYPE::FILLED_SHAPE );
        else
            line->SetFillMode( FILL_TYPE::FILLED_WITH_BG_BODYCOLOR );
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
            wxLogWarning( wxString::Format(
                    "Arc tries to access symbol with ownerindex %d which does not exist",
                    elem.ownerindex ) );
            return;
        }

        const auto& component = m_components.at( symbol->first );

        if( elem.startAngle == 0 && ( elem.endAngle == 0 || elem.endAngle == 360 ) )
        {
            LIB_CIRCLE* circle = new LIB_CIRCLE( symbol->second );
            symbol->second->AddDrawItem( circle );

            circle->SetPosition( GetRelativePosition( elem.center, component ) );
            circle->SetRadius( elem.radius );
            circle->SetWidth( elem.lineWidth );
        }
        else
        {
            LIB_ARC* arc = new LIB_ARC( symbol->second );
            symbol->second->AddDrawItem( arc );

            // TODO: correct?
            arc->SetPosition( GetRelativePosition( elem.center, component ) );
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
        SCH_LINE* line = new SCH_LINE( elem.point1, SCH_LAYER_ID::LAYER_NOTES );
        line->SetEndPoint( elem.point2 );
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
            wxLogWarning( wxString::Format(
                    "Line tries to access symbol with ownerindex %d which does not exist",
                    elem.ownerindex ) );
            return;
        }

        const auto& component = m_components.at( symbol->first );

        LIB_POLYLINE* line = new LIB_POLYLINE( symbol->second );
        symbol->second->AddDrawItem( line );

        line->AddPoint( GetRelativePosition( elem.point1, component ) );
        line->AddPoint( GetRelativePosition( elem.point2, component ) );

        line->SetWidth( elem.lineWidth );
    }
}


void SCH_ALTIUM_PLUGIN::ParseRectangle( const std::map<wxString, wxString>& aProperties )
{
    ASCH_RECTANGLE elem( aProperties );

    if( elem.ownerpartid == ALTIUM_COMPONENT_NONE )
    {
        const wxPoint topLeft     = { elem.bottomLeft.x, elem.topRight.y };
        const wxPoint bottomRight = { elem.topRight.x, elem.bottomLeft.y };

        // TODO: we cannot fill this rectangle, only draw it for now
        SCH_LINE* lineTop = new SCH_LINE( elem.topRight, SCH_LAYER_ID::LAYER_NOTES );
        lineTop->SetEndPoint( topLeft );
        lineTop->SetLineWidth( elem.lineWidth );
        lineTop->SetLineStyle( PLOT_DASH_TYPE::SOLID );
        lineTop->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( lineTop );

        SCH_LINE* lineBottom = new SCH_LINE( elem.bottomLeft, SCH_LAYER_ID::LAYER_NOTES );
        lineBottom->SetEndPoint( bottomRight );
        lineBottom->SetLineWidth( elem.lineWidth );
        lineBottom->SetLineStyle( PLOT_DASH_TYPE::SOLID );
        lineBottom->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( lineBottom );

        SCH_LINE* lineRight = new SCH_LINE( elem.topRight, SCH_LAYER_ID::LAYER_NOTES );
        lineRight->SetEndPoint( bottomRight );
        lineRight->SetLineWidth( elem.lineWidth );
        lineRight->SetLineStyle( PLOT_DASH_TYPE::SOLID );
        lineRight->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( lineRight );

        SCH_LINE* lineLeft = new SCH_LINE( elem.bottomLeft, SCH_LAYER_ID::LAYER_NOTES );
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
            wxLogWarning( wxString::Format(
                    "Rectangle tries to access symbol with ownerindex %d which does not exist",
                    elem.ownerindex ) );
            return;
        }

        const auto& component = m_components.at( symbol->first );

        LIB_RECTANGLE* rect = new LIB_RECTANGLE( symbol->second );
        symbol->second->AddDrawItem( rect );
        rect->SetPosition( GetRelativePosition( elem.topRight, component ) );
        rect->SetEnd( GetRelativePosition( elem.bottomLeft, component ) );
        rect->SetWidth( elem.lineWidth );

        if( !elem.isSolid )
            rect->SetFillMode( FILL_TYPE::NO_FILL );
        else if( elem.color == elem.areacolor )
            rect->SetFillMode( FILL_TYPE::FILLED_SHAPE );
        else
            rect->SetFillMode( FILL_TYPE::FILLED_WITH_BG_BODYCOLOR );
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

    for( size_t i = 0; i < elem.points.size() - 1; i++ )
    {
        SCH_LINE* bus = new SCH_LINE( elem.points.at( i ), SCH_LAYER_ID::LAYER_BUS );
        bus->SetEndPoint( elem.points.at( i + 1 ) );
        bus->SetLineWidth( elem.lineWidth );

        bus->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( bus );
    }
}


void SCH_ALTIUM_PLUGIN::ParseWire( const std::map<wxString, wxString>& aProperties )
{
    ASCH_WIRE elem( aProperties );

    for( size_t i = 0; i < elem.points.size() - 1; i++ )
    {
        SCH_LINE* wire = new SCH_LINE( elem.points.at( i ), SCH_LAYER_ID::LAYER_WIRE );
        wire->SetEndPoint( elem.points.at( i + 1 ) );
        wire->SetLineWidth( elem.lineWidth );

        wire->SetFlags( IS_NEW );
        m_currentSheet->GetScreen()->Append( wire );
    }
}


void SCH_ALTIUM_PLUGIN::ParseJunction( const std::map<wxString, wxString>& aProperties )
{
    ASCH_JUNCTION elem( aProperties );

    SCH_JUNCTION* junction = new SCH_JUNCTION( elem.location );

    junction->SetFlags( IS_NEW );
    m_currentSheet->GetScreen()->Append( junction );
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
