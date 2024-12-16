/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright (C) 2020-2023 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <hash_eda.h>
#include <hash.h>
#include <footprint.h>
#include <pcb_text.h>
#include <pcb_table.h>
#include <pcb_textbox.h>
#include <pcb_shape.h>
#include <pad.h>
#include <pcb_track.h>

#include <macros.h>
#include <functional>

#include <wx/log.h>

using namespace std;

// Common calculation part for all BOARD_ITEMs
static inline size_t hash_board_item( const BOARD_ITEM* aItem, int aFlags )
{
    size_t ret = 0;

    if( aFlags & HASH_LAYER )
        ret = hash<BASE_SET>{}( aItem->GetLayerSet() );

    return ret;
}


size_t hash_fp_item( const EDA_ITEM* aItem, int aFlags )
{
    size_t ret = 0;

    switch( aItem->Type() )
    {
    case PCB_FOOTPRINT_T:
    {
        const FOOTPRINT* footprint = static_cast<const FOOTPRINT*>( aItem );

        ret = hash_board_item( footprint, aFlags );

        if( aFlags & HASH_POS )
            hash_combine( ret, footprint->GetPosition().x, footprint->GetPosition().y );

        if( aFlags & HASH_ROT )
            hash_combine( ret, footprint->GetOrientation().AsDegrees() );

        for( BOARD_ITEM* item : footprint->GraphicalItems() )
            hash_combine( ret, hash_fp_item( item, aFlags ) );

        for( PAD* pad : footprint->Pads() )
            hash_combine( ret, hash_fp_item( static_cast<EDA_ITEM*>( pad ), aFlags ) );
    }
        break;

    case PCB_VIA_T:
    {
        const PCB_VIA* via = static_cast<const PCB_VIA*>( aItem );

        ret = hash<int>{}( via->GetDrillValue() );
        hash_combine( ret, via->TopLayer() );
        hash_combine( ret, via->BottomLayer() );

        via->GetLayerSet().RunOnLayers(
                [&]( PCB_LAYER_ID layer )
                {
                    hash_combine( ret, via->GetWidth( layer ) );
                    hash_combine( ret, via->FlashLayer( layer ) );
                } );

        break;
    }

    case PCB_PAD_T:
    {
        const PAD* pad = static_cast<const PAD*>( aItem );

        ret = hash<int>{}( static_cast<int>( pad->GetAttribute() ) );

        auto hashPadLayer =
            [&]( PCB_LAYER_ID aLayer )
            {
                hash_combine( ret, pad->GetShape( aLayer ) );
                hash_combine( ret, pad->GetSize( aLayer ).x, pad->GetSize( aLayer ).y );
                hash_combine( ret, pad->GetOffset( aLayer ).x, pad->GetOffset( aLayer ).y );

                switch( pad->GetShape( PADSTACK::ALL_LAYERS ) )
                {
                case PAD_SHAPE::CHAMFERED_RECT:
                    hash_combine( ret, pad->GetChamferPositions( aLayer ) );
                    hash_combine( ret, pad->GetChamferRectRatio( aLayer ) );
                    break;

                case PAD_SHAPE::ROUNDRECT:
                    hash_combine( ret, pad->GetRoundRectCornerRadius( aLayer ) );
                    break;

                case PAD_SHAPE::TRAPEZOID:
                    hash_combine( ret, pad->GetDelta( aLayer ).x, pad->GetDelta( aLayer ).y );
                    break;

                case PAD_SHAPE::CUSTOM:
                {
                    auto poly = pad->GetEffectivePolygon( aLayer, ERROR_INSIDE );

                    for( int ii = 0; ii < poly->VertexCount(); ++ii )
                    {
                        VECTOR2I point = poly->CVertex( ii ) - pad->GetPosition();
                        hash_combine( ret, point.x, point.y );
                    }

                    break;
                }
                default:
                    break;
                }
            };

        pad->Padstack().ForEachUniqueLayer( hashPadLayer );

        if( pad->GetAttribute() == PAD_ATTRIB::PTH || pad->GetAttribute() == PAD_ATTRIB::NPTH )
        {
            hash_combine( ret, pad->GetDrillSizeX(), pad->GetDrillSizeY() );
            hash_combine( ret, pad->GetDrillShape() );

            pad->GetLayerSet().RunOnLayers(
                    [&]( PCB_LAYER_ID layer )
                    {
                        hash_combine( ret, pad->FlashLayer( layer ) );
                    } );
        }

        hash_combine( ret, hash_board_item( pad, aFlags ) );

        if( aFlags & HASH_POS )
        {
            if( aFlags & REL_COORD )
                hash_combine( ret, pad->GetFPRelativePosition().x, pad->GetFPRelativePosition().y );
            else
                hash_combine( ret, pad->GetPosition().x, pad->GetPosition().y );
        }

        if( aFlags & HASH_ROT )
            hash_combine( ret, pad->GetOrientation().AsDegrees() );

        if( aFlags & HASH_NET )
            hash_combine( ret, pad->GetNetCode() );
    }
    break;

    case PCB_FIELD_T:
        if( !( aFlags & HASH_REF ) && static_cast<const PCB_FIELD*>( aItem )->IsReference() )
            break;

        if( !( aFlags & HASH_VALUE ) && static_cast<const PCB_FIELD*>( aItem )->IsValue() )
            break;

        KI_FALLTHROUGH;
    case PCB_TEXT_T:
    {
        const PCB_TEXT* text = static_cast<const PCB_TEXT*>( aItem );

        ret = hash_board_item( text, aFlags );
        hash_combine( ret, text->GetText().ToStdString() );
        hash_combine( ret, text->IsItalic() );
        hash_combine( ret, text->IsBold() );
        hash_combine( ret, text->IsMirrored() );
        hash_combine( ret, text->GetTextWidth() );
        hash_combine( ret, text->GetTextHeight() );
        hash_combine( ret, text->GetHorizJustify() );
        hash_combine( ret, text->GetVertJustify() );

        if( aFlags & HASH_POS )
        {
            VECTOR2I pos = ( aFlags & REL_COORD ) ? text->GetFPRelativePosition()
                                                  : text->GetPosition();

            hash_combine( ret, pos.x, pos.y );
        }

        if( aFlags & HASH_ROT )
            hash_combine( ret, text->GetTextAngle().AsDegrees() );
    }
        break;

    case PCB_SHAPE_T:
    {
        const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( aItem );
        ret = hash_board_item( shape, aFlags );
        hash_combine( ret, shape->GetShape() );
        hash_combine( ret, shape->GetWidth() );
        hash_combine( ret, shape->IsFilled() );
        hash_combine( ret, shape->GetLineStyle() );

        if( shape->GetShape() == SHAPE_T::ARC || shape->GetShape() == SHAPE_T::CIRCLE )
            hash_combine( ret, shape->GetRadius() );

        if( aFlags & HASH_POS )
        {
            std::vector<VECTOR2I> points;

            points.push_back( shape->GetStart() );
            points.push_back( shape->GetEnd() );

            if( shape->GetShape() == SHAPE_T::CIRCLE )
                points.push_back( shape->GetCenter() );

            if( shape->GetShape() == SHAPE_T::ARC )
                points.push_back( shape->GetArcMid() );

            FOOTPRINT* parentFP = shape->GetParentFootprint();

            if( shape->GetShape() == SHAPE_T::POLY )
            {
                const SHAPE_POLY_SET& poly = shape->GetPolyShape();

                for( auto it = poly.CIterateWithHoles(); it; it++ )
                    points.push_back( *it );
            }

            if( shape->GetShape() == SHAPE_T::BEZIER )
            {
                points.push_back( shape->GetBezierC1() );
                points.push_back( shape->GetBezierC2() );
            }

            if( parentFP && ( aFlags & REL_COORD ) )
            {
                for( VECTOR2I& point : points )
                {
                    point -= parentFP->GetPosition();
                    RotatePoint( point, -parentFP->GetOrientation() );
                }
            }

            if( aFlags & REL_POS )
            {
                for( VECTOR2I& point : points )
                    point -= shape->GetPosition();
            }

            for( VECTOR2I& point : points )
                hash_combine( ret, point.x, point.y );
        }
    }
        break;

    case PCB_TABLECELL_T:
    case PCB_TEXTBOX_T:
    {
        const PCB_TEXTBOX* textbox = static_cast<const PCB_TEXTBOX*>( aItem );

        ret = hash_board_item( textbox, aFlags );
        hash_combine( ret, textbox->GetText().ToStdString() );
        hash_combine( ret, textbox->IsItalic() );
        hash_combine( ret, textbox->IsBold() );
        hash_combine( ret, textbox->IsMirrored() );
        hash_combine( ret, textbox->GetTextWidth() );
        hash_combine( ret, textbox->GetTextHeight() );
        hash_combine( ret, textbox->GetHorizJustify() );
        hash_combine( ret, textbox->GetVertJustify() );

        if( aFlags & HASH_ROT )
            hash_combine( ret, textbox->GetTextAngle().AsDegrees() );

        hash_combine( ret, textbox->GetShape() );
        hash_combine( ret, textbox->GetWidth() );
        hash_combine( ret, textbox->GetLineStyle() );

        if( aFlags & HASH_POS )
        {
            VECTOR2I start = textbox->GetStart();
            VECTOR2I end = textbox->GetEnd();

            FOOTPRINT* parentFP = textbox->GetParentFootprint();

            if( parentFP && ( aFlags & REL_COORD ) )
            {
                start -= parentFP->GetPosition();
                end -= parentFP->GetPosition();

                RotatePoint( start, -parentFP->GetOrientation() );
                RotatePoint( end, -parentFP->GetOrientation() );
            }

            hash_combine( ret, start.x );
            hash_combine( ret, start.y );
            hash_combine( ret, end.x );
            hash_combine( ret, end.y );
        }
    }
        break;

    case PCB_TABLE_T:
    {
        const PCB_TABLE* table = static_cast<const PCB_TABLE*>( aItem );

        ret = hash_board_item( table, aFlags );

        hash_combine( ret, table->StrokeExternal() );
        hash_combine( ret, table->StrokeHeader() );
        hash_combine( ret, table->StrokeColumns() );
        hash_combine( ret, table->StrokeRows() );

        auto hash_stroke =
                [&]( const STROKE_PARAMS& stroke )
                {
                    hash_combine( ret, stroke.GetColor() );
                    hash_combine( ret, stroke.GetWidth() );
                    hash_combine( ret, stroke.GetLineStyle() );
                };

        hash_stroke( table->GetSeparatorsStroke() );
        hash_stroke( table->GetBorderStroke() );
    }
        break;

    default:
        wxASSERT_MSG( false, "Unhandled type in function hash_fp_item() (exporter_gencad.cpp)" );
    }

    return ret;
}
