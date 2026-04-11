/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <magic_enum.hpp>
#include <api/api_utils.h>
#include <api/schematic/schematic_types.pb.h>
#include <geometry/shape_poly_set.h>
#include <kiid.h>
#include <project.h>
#include <wx/log.h>

const wxChar* const traceApi = wxT( "KICAD_API" );


namespace kiapi::common
{

KICOMMON_API std::optional<KICAD_T> TypeNameFromAny( const google::protobuf::Any& aMessage )
{
    if( aMessage.type_url() == "type.googleapis.com/kiapi.schematic.types.BusEntry" )
    {
        kiapi::schematic::types::BusEntry entry;

        if( !aMessage.UnpackTo( &entry ) )
            return std::nullopt;

        switch( entry.type() )
        {
        case kiapi::schematic::types::BET_WIRE_TO_BUS:
            return SCH_BUS_WIRE_ENTRY_T;

        case kiapi::schematic::types::BET_BUS_TO_BUS:
            return SCH_BUS_BUS_ENTRY_T;

        default:
            return std::nullopt;
        }
    }

    static const std::map<std::string, KICAD_T> s_types = {
        { "type.googleapis.com/kiapi.board.types.Track", PCB_TRACE_T },
        { "type.googleapis.com/kiapi.board.types.Arc", PCB_ARC_T },
        { "type.googleapis.com/kiapi.board.types.Via", PCB_VIA_T },
        { "type.googleapis.com/kiapi.board.types.BoardText", PCB_TEXT_T },
        { "type.googleapis.com/kiapi.board.types.BoardTextBox", PCB_TEXTBOX_T },
        { "type.googleapis.com/kiapi.board.types.BoardGraphicShape", PCB_SHAPE_T },
        { "type.googleapis.com/kiapi.board.types.Barcode", PCB_BARCODE_T },
        { "type.googleapis.com/kiapi.board.types.Pad", PCB_PAD_T },
        { "type.googleapis.com/kiapi.board.types.Zone", PCB_ZONE_T },
        { "type.googleapis.com/kiapi.board.types.Dimension", PCB_DIMENSION_T },
        { "type.googleapis.com/kiapi.board.types.ReferenceImage", PCB_REFERENCE_IMAGE_T },
        { "type.googleapis.com/kiapi.board.types.Group", PCB_GROUP_T },
        { "type.googleapis.com/kiapi.board.types.Field", PCB_FIELD_T },
        { "type.googleapis.com/kiapi.board.types.FootprintInstance", PCB_FOOTPRINT_T },
        { "type.googleapis.com/kiapi.schematic.types.Junction", SCH_JUNCTION_T },
        { "type.googleapis.com/kiapi.schematic.types.NoConnectMarker", SCH_NO_CONNECT_T },
        { "type.googleapis.com/kiapi.schematic.types.BusEntry", SCH_BUS_WIRE_ENTRY_T },
        { "type.googleapis.com/kiapi.schematic.types.SchematicLine", SCH_LINE_T },
        { "type.googleapis.com/kiapi.schematic.types.SchematicGraphicShape", SCH_SHAPE_T },
        { "type.googleapis.com/kiapi.schematic.types.SchematicImage", SCH_BITMAP_T },
        { "type.googleapis.com/kiapi.schematic.types.SchematicTextBox", SCH_TEXTBOX_T },
        { "type.googleapis.com/kiapi.schematic.types.SchematicText", SCH_TEXT_T },
        { "type.googleapis.com/kiapi.schematic.types.Table", SCH_TABLE_T },
        { "type.googleapis.com/kiapi.schematic.types.LocalLabel", SCH_LABEL_T },
        { "type.googleapis.com/kiapi.schematic.types.GlobalLabel", SCH_GLOBAL_LABEL_T },
        { "type.googleapis.com/kiapi.schematic.types.HierarchicalLabel", SCH_HIER_LABEL_T },
        { "type.googleapis.com/kiapi.schematic.types.DirectiveLabel", SCH_DIRECTIVE_LABEL_T },
        { "type.googleapis.com/kiapi.schematic.types.Group", SCH_GROUP_T },
        { "type.googleapis.com/kiapi.schematic.types.SheetSymbol", SCH_SHEET_T },
        { "type.googleapis.com/kiapi.schematic.types.SchematicSymbolInstance", SCH_SYMBOL_T },
        { "type.googleapis.com/kiapi.schematic.types.SchematicPin", SCH_PIN_T },
    };

    auto it = s_types.find( aMessage.type_url() );

    if( it != s_types.end() )
        return it->second;

    wxLogTrace( traceApi, wxString::Format( wxS( "Any message type %s is not known" ),
                                            aMessage.type_url() ) );

    return std::nullopt;
}


KICOMMON_API LIB_ID UnpackLibId( const types::LibraryIdentifier& aId )
{
    return LIB_ID( aId.library_nickname(), aId.entry_name() );
}


KICOMMON_API void PackLibId( types::LibraryIdentifier* aOutput, const LIB_ID& aId )
{
    aOutput->set_library_nickname( aId.GetLibNickname() );
    aOutput->set_entry_name( aId.GetLibItemName() );
}


KICOMMON_API void PackVector2( types::Vector2& aOutput, const VECTOR2I& aInput, const EDA_IU_SCALE& aScale )
{
    aOutput.set_x_nm( aScale.IUToNm( aInput.x ) );
    aOutput.set_y_nm( aScale.IUToNm( aInput.y ) );
}


KICOMMON_API VECTOR2I UnpackVector2( const types::Vector2& aInput, const EDA_IU_SCALE& aScale )
{
    return VECTOR2I( aScale.NmToIU( aInput.x_nm() ), aScale.NmToIU( aInput.y_nm() ) );
}


KICOMMON_API void PackVector3D( types::Vector3D& aOutput, const VECTOR3D& aInput )
{
    aOutput.set_x_nm( aInput.x );
    aOutput.set_y_nm( aInput.y );
    aOutput.set_z_nm( aInput.z );
}


KICOMMON_API VECTOR3D UnpackVector3D( const types::Vector3D& aInput )
{
    return VECTOR3D( aInput.x_nm(), aInput.y_nm(), aInput.z_nm() );
}


KICOMMON_API void PackBox2( types::Box2& aOutput, const BOX2I& aInput, const EDA_IU_SCALE& aScale )
{
    PackVector2( *aOutput.mutable_position(), aInput.GetOrigin(), aScale );
    PackVector2( *aOutput.mutable_size(), aInput.GetSize(), aScale );
}


KICOMMON_API BOX2I UnpackBox2( const types::Box2& aInput, const EDA_IU_SCALE& aScale )
{
    return BOX2I( UnpackVector2( aInput.position(), aScale ), UnpackVector2( aInput.size(), aScale ) );
}


KICOMMON_API void PackDistance( types::Distance& aOutput, int aInput, const EDA_IU_SCALE& aScale )
{
    aOutput.set_value_nm( aScale.IUToNm( aInput ) );
}


KICOMMON_API int UnpackDistance( const types::Distance& aInput, const EDA_IU_SCALE& aScale )
{
    return aScale.NmToIU( aInput.value_nm() );
}


KICOMMON_API void PackPolyLine( types::PolyLine& aOutput, const SHAPE_LINE_CHAIN& aSlc, const EDA_IU_SCALE& aScale )
{
    for( int vertex = 0; vertex < aSlc.PointCount(); vertex = aSlc.NextShape( vertex ) )
    {
        if( vertex < 0 )
            break;

        if( aSlc.IsArcStart( vertex ) )
        {
            types::PolyLineNode* node = aOutput.mutable_nodes()->Add();
            const SHAPE_ARC& arc = aSlc.Arc( aSlc.ArcIndex( vertex ) );
            PackVector2( *node->mutable_arc()->mutable_start(), arc.GetP0(), aScale );
            PackVector2( *node->mutable_arc()->mutable_mid(), arc.GetArcMid(), aScale );
            PackVector2( *node->mutable_arc()->mutable_end(), arc.GetP1(), aScale );
        }
        else if( !aSlc.IsPtOnArc( vertex ) )
        {
            types::PolyLineNode* node = aOutput.mutable_nodes()->Add();
            PackVector2( *node->mutable_point(), aSlc.CPoint( vertex ), aScale );
        }
    }

    aOutput.set_closed( aSlc.IsClosed() );
}


KICOMMON_API SHAPE_LINE_CHAIN UnpackPolyLine( const types::PolyLine& aInput, const EDA_IU_SCALE& aScale )
{
    SHAPE_LINE_CHAIN slc;

    for( const types::PolyLineNode& node : aInput.nodes() )
    {
        if( node.has_point() )
        {
            slc.Append( UnpackVector2( node.point(), aScale ) );
        }
        else if( node.has_arc() )
        {
            slc.Append( SHAPE_ARC( UnpackVector2( node.arc().start(), aScale ),
                                   UnpackVector2( node.arc().mid(), aScale ),
                                   UnpackVector2( node.arc().end(), aScale ),
                                   0 ) );
        }
    }

    slc.SetClosed( aInput.closed() );

    return slc;
}


KICOMMON_API void PackPolySet( types::PolySet& aOutput, const SHAPE_POLY_SET& aInput, const EDA_IU_SCALE& aScale )
{
    for( int idx = 0; idx < aInput.OutlineCount(); ++idx )
    {
        const SHAPE_POLY_SET::POLYGON& poly = aInput.Polygon( idx );

        if( poly.empty() )
            continue;

        types::PolygonWithHoles* polyMsg = aOutput.mutable_polygons()->Add();
        PackPolyLine( *polyMsg->mutable_outline(), poly.front(), aScale );

        if( poly.size() > 1 )
        {
            for( size_t hole = 1; hole < poly.size(); ++hole )
            {
                types::PolyLine* pl = polyMsg->mutable_holes()->Add();
                PackPolyLine( *pl, poly[hole], aScale );
            }
        }
    }
}


KICOMMON_API SHAPE_POLY_SET UnpackPolySet( const types::PolySet& aInput, const EDA_IU_SCALE& aScale )
{
    SHAPE_POLY_SET sps;

    for( const types::PolygonWithHoles& polygonWithHoles : aInput.polygons() )
    {
        SHAPE_POLY_SET::POLYGON polygon;

        polygon.emplace_back( UnpackPolyLine( polygonWithHoles.outline(), aScale ) );

        for( const types::PolyLine& holeMsg : polygonWithHoles.holes() )
            polygon.emplace_back( UnpackPolyLine( holeMsg, aScale ) );

        sps.AddPolygon( polygon );
    }

    return sps;
}


KICOMMON_API void PackColor( types::Color& aOutput, const KIGFX::COLOR4D& aInput )
{
    aOutput.set_r( aInput.r );
    aOutput.set_g( aInput.g );
    aOutput.set_b( aInput.b );
    aOutput.set_a( aInput.a );
}


KICOMMON_API KIGFX::COLOR4D UnpackColor( const types::Color& aInput )
{
    double r = std::clamp( aInput.r(), 0.0, 1.0 );
    double g = std::clamp( aInput.g(), 0.0, 1.0 );
    double b = std::clamp( aInput.b(), 0.0, 1.0 );
    double a = std::clamp( aInput.a(), 0.0, 1.0 );

    return KIGFX::COLOR4D( r, g, b, a );
}

KICOMMON_API void PackSheetPath( types::SheetPath& aOutput, const KIID_PATH& aInput )
{
    aOutput.clear_path();

    for( const KIID& entry : aInput )
        aOutput.add_path()->set_value( entry.AsStdString() );
}

KICOMMON_API KIID_PATH UnpackSheetPath( const types::SheetPath& aInput )
{
    KIID_PATH output;

    for( const types::KIID& sheet : aInput.path() )
        output.push_back( KIID( sheet.value() ) );

    return output;
}

KICOMMON_API void PackProject( types::ProjectSpecifier& aOutput, const PROJECT& aInput )
{
    aOutput.set_name( aInput.GetProjectName().ToUTF8() );
    aOutput.set_path( aInput.GetProjectPath().ToUTF8() );
}

} // namespace kiapi::common
