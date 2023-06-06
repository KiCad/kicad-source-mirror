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
#include <pcb_textbox.h>
#include <pcb_shape.h>
#include <pad.h>

#include <macros.h>
#include <functional>

using namespace std;

// Common calculation part for all BOARD_ITEMs
static inline size_t hash_board_item( const BOARD_ITEM* aItem, int aFlags )
{
    size_t ret = 0;

    if( aFlags & HASH_LAYER )
        ret = hash<unsigned long long>{}( aItem->GetLayerSet().to_ullong() );

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

    case PCB_PAD_T:
    {
        const PAD* pad = static_cast<const PAD*>( aItem );

        ret = hash<int>{}( static_cast<int>( pad->GetShape() ) );
        hash_combine( ret, pad->GetDrillShape() );
        hash_combine( ret, pad->GetSize().x, pad->GetSize().y );
        hash_combine( ret, pad->GetOffset().x, pad->GetOffset().y );
        hash_combine( ret, pad->GetDelta().x, pad->GetDelta().y );

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

        if( shape->GetShape() == SHAPE_T::ARC || shape->GetShape() == SHAPE_T::CIRCLE )
            hash_combine( ret, shape->GetRadius() );

        if( aFlags & HASH_POS )
        {
            VECTOR2I start = shape->GetStart();
            VECTOR2I end = shape->GetEnd();
            VECTOR2I center = shape->GetCenter();

            FOOTPRINT* parentFP = shape->GetParentFootprint();

            if( parentFP && ( aFlags & REL_COORD ) )
            {
                start -= parentFP->GetPosition();
                end -= parentFP->GetPosition();
                center -= parentFP->GetPosition();

                RotatePoint( start, -parentFP->GetOrientation() );
                RotatePoint( end, -parentFP->GetOrientation() );
                RotatePoint( center, -parentFP->GetOrientation() );
            }

            hash_combine( ret, start.x );
            hash_combine( ret, start.y );
            hash_combine( ret, end.x );
            hash_combine( ret, end.y );

            if( shape->GetShape() == SHAPE_T::ARC )
            {
                hash_combine( ret, center.x );
                hash_combine( ret, center.y );
            }
        }
    }
        break;

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

    default:
        wxASSERT_MSG( false, "Unhandled type in function hash_fp_item() (exporter_gencad.cpp)" );
    }

    return ret;
}
