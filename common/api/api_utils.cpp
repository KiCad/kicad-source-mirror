/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <magic_enum.hpp>
#include <api/api_utils.h>
#include <geometry/shape_poly_set.h>

namespace kiapi::common
{

std::optional<KICAD_T> TypeNameFromAny( const google::protobuf::Any& aMessage )
{
    static const std::map<std::string, KICAD_T> s_types = {
        { "type.googleapis.com/kiapi.board.types.Track", PCB_TRACE_T },
        { "type.googleapis.com/kiapi.board.types.Arc", PCB_ARC_T },
        { "type.googleapis.com/kiapi.board.types.Via", PCB_VIA_T },
        { "type.googleapis.com/kiapi.board.types.Text", PCB_TEXT_T },
        { "type.googleapis.com/kiapi.board.types.TextBox", PCB_TEXTBOX_T },
        { "type.googleapis.com/kiapi.board.types.GraphicShape", PCB_SHAPE_T },
        { "type.googleapis.com/kiapi.board.types.Pad", PCB_PAD_T },
        { "type.googleapis.com/kiapi.board.types.Zone", PCB_ZONE_T },
        { "type.googleapis.com/kiapi.board.types.Dimension", PCB_DIMENSION_T },
        { "type.googleapis.com/kiapi.board.types.ReferenceImage", PCB_REFERENCE_IMAGE_T },
        { "type.googleapis.com/kiapi.board.types.Group", PCB_GROUP_T },
        { "type.googleapis.com/kiapi.board.types.Field", PCB_FIELD_T },
        { "type.googleapis.com/kiapi.board.types.FootprintInstance", PCB_FOOTPRINT_T },
    };

    auto it = s_types.find( aMessage.type_url() );

    if( it != s_types.end() )
        return it->second;

    return std::nullopt;
}


LIB_ID LibIdFromProto( const types::LibraryIdentifier& aId )
{
    return LIB_ID( aId.library_nickname(), aId.entry_name() );
}


types::LibraryIdentifier LibIdToProto( const LIB_ID& aId )
{
    types::LibraryIdentifier msg;
    msg.set_library_nickname( aId.GetLibNickname() );
    msg.set_entry_name( aId.GetLibItemName() );
    return msg;
}


void PackVector2( types::Vector2& aOutput, const VECTOR2I& aInput )
{
    aOutput.set_x_nm( aInput.x );
    aOutput.set_y_nm( aInput.y );
}


VECTOR2I UnpackVector2( const types::Vector2& aInput )
{
    return VECTOR2I( aInput.x_nm(), aInput.y_nm() );
}


void PackBox2( types::Box2& aOutput, const BOX2I& aInput )
{
    PackVector2( *aOutput.mutable_position(), aInput.GetOrigin() );
    PackVector2( *aOutput.mutable_size(), aInput.GetSize() );
}


BOX2I UnpackBox2( const types::Box2& aInput )
{
    return BOX2I( UnpackVector2( aInput.position() ), UnpackVector2( aInput.size() ) );
}


void PackPolyLine( kiapi::common::types::PolyLine& aOutput, const SHAPE_LINE_CHAIN& aSlc )
{
    for( int vertex = 0; vertex < aSlc.PointCount(); vertex = aSlc.NextShape( vertex ) )
    {
        if( vertex < 0 )
            break;

        kiapi::common::types::PolyLineNode* node = aOutput.mutable_nodes()->Add();

        if( aSlc.IsPtOnArc( vertex ) )
        {
            const SHAPE_ARC& arc = aSlc.Arc( aSlc.ArcIndex( vertex ) );
            node->mutable_arc()->mutable_start()->set_x_nm( arc.GetP0().x );
            node->mutable_arc()->mutable_start()->set_y_nm( arc.GetP0().y );
            node->mutable_arc()->mutable_mid()->set_x_nm( arc.GetArcMid().x );
            node->mutable_arc()->mutable_mid()->set_y_nm( arc.GetArcMid().y );
            node->mutable_arc()->mutable_end()->set_x_nm( arc.GetP1().x );
            node->mutable_arc()->mutable_end()->set_y_nm( arc.GetP1().y );
        }
        else
        {
            node->mutable_point()->set_x_nm( aSlc.CPoint( vertex ).x );
            node->mutable_point()->set_y_nm( aSlc.CPoint( vertex ).y );
        }
    }

    aOutput.set_closed( aSlc.IsClosed() );
}


SHAPE_LINE_CHAIN UnpackPolyLine( const kiapi::common::types::PolyLine& aInput )
{
    SHAPE_LINE_CHAIN slc;

    for( const kiapi::common::types::PolyLineNode& node : aInput.nodes() )
    {
        if( node.has_point() )
        {
            slc.Append( VECTOR2I( node.point().x_nm(), node.point().y_nm() ) );
        }
        else if( node.has_arc() )
        {
            slc.Append( SHAPE_ARC( VECTOR2I( node.arc().start().x_nm(), node.arc().start().y_nm() ),
                                   VECTOR2I( node.arc().mid().x_nm(), node.arc().mid().y_nm() ),
                                   VECTOR2I( node.arc().end().x_nm(), node.arc().end().y_nm() ),
                                   0 /* don't care about width here */ ) );
        }
    }

    slc.SetClosed( aInput.closed() );

    return slc;
}


void PackPolySet( types::PolySet& aOutput, const SHAPE_POLY_SET& aInput )
{
    for( int idx = 0; idx < aInput.OutlineCount(); ++idx )
    {
        const SHAPE_POLY_SET::POLYGON& poly = aInput.Polygon( idx );

        if( poly.empty() )
            continue;

        types::PolygonWithHoles* polyMsg = aOutput.mutable_polygons()->Add();
        PackPolyLine( *polyMsg->mutable_outline(), poly.front() );

        if( poly.size() > 1 )
        {
            for( size_t hole = 1; hole < poly.size(); ++hole )
            {
                types::PolyLine* pl = polyMsg->mutable_holes()->Add();
                PackPolyLine( *pl, poly[hole] );
            }
        }
    }
}


SHAPE_POLY_SET UnpackPolySet( const types::PolySet& aInput )
{
    SHAPE_POLY_SET sps;

    for( const types::PolygonWithHoles& polygonWithHoles : aInput.polygons() )
    {
        SHAPE_POLY_SET::POLYGON polygon;

        polygon.emplace_back( UnpackPolyLine( polygonWithHoles.outline() ) );

        for( const types::PolyLine& holeMsg : polygonWithHoles.holes() )
            polygon.emplace_back( UnpackPolyLine( holeMsg ) );

        sps.AddPolygon( polygon );
    }

    return sps;
}

} // namespace kiapi::common
