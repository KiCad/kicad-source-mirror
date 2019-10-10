/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
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

#include <class_module.h>
#include <class_text_mod.h>
#include <class_edge_mod.h>
#include <class_pad.h>

#include <functional>

using namespace std;

// Common calculation part for all BOARD_ITEMs
static inline size_t hash_board_item( const BOARD_ITEM* aItem, int aFlags )
{
    size_t ret = 0;

    if( aFlags & LAYER )
        ret = hash<unsigned long long>{}( aItem->GetLayerSet().to_ullong() );

    return ret;
}


size_t hash_eda( const EDA_ITEM* aItem, int aFlags )
{
    size_t ret = 0xa82de1c0;

    switch( aItem->Type() )
    {
    case PCB_MODULE_T:
        {
            const MODULE* module = static_cast<const MODULE*>( aItem );

            ret += hash_board_item( module, aFlags );

            if( aFlags & POSITION )
            {
                ret += hash<int>{}( module->GetPosition().x );
                ret += hash<int>{}( module->GetPosition().y );
            }

            if( aFlags & ROTATION )
                ret += hash<double>{}( module->GetOrientation() );

            for( auto i : module->GraphicalItems() )
                ret += hash_eda( i, aFlags );

            for( auto i : module->Pads() )
                ret += hash_eda( static_cast<EDA_ITEM*>( i ), aFlags );
        }
        break;

    case PCB_PAD_T:
        {
            const D_PAD* pad = static_cast<const D_PAD*>( aItem );
            ret += hash_board_item( pad, aFlags );
            ret += hash<int>{}( pad->GetShape() << 16 );
            ret += hash<int>{}( pad->GetDrillShape() << 18 );
            ret += hash<int>{}( pad->GetSize().x << 8 );
            ret += hash<int>{}( pad->GetSize().y << 9 );
            ret += hash<int>{}( pad->GetOffset().x << 6 );
            ret += hash<int>{}( pad->GetOffset().y << 7 );
            ret += hash<int>{}( pad->GetDelta().x << 4 );
            ret += hash<int>{}( pad->GetDelta().y << 5 );

            if( aFlags & POSITION )
            {
                if( aFlags & REL_COORD )
                {
                    ret += hash<int>{}( pad->GetPos0().x );
                    ret += hash<int>{}( pad->GetPos0().y );
                }
                else
                {
                    ret += hash<int>{}( pad->GetPosition().x );
                    ret += hash<int>{}( pad->GetPosition().y );
                }
            }

            if( aFlags & ROTATION )
                ret += hash<double>{}( pad->GetOrientation() );

            if( aFlags & NET )
                ret += hash<int>{}( pad->GetNetCode() << 6 );
        }
        break;

    case PCB_MODULE_TEXT_T:
        {
            const TEXTE_MODULE* text = static_cast<const TEXTE_MODULE*>( aItem );

            if( !( aFlags & REFERENCE ) && text->GetType() == TEXTE_MODULE::TEXT_is_REFERENCE )
                break;

            if( !( aFlags & VALUE ) && text->GetType() == TEXTE_MODULE::TEXT_is_VALUE )
                break;

            ret += hash_board_item( text, aFlags );
            ret += hash<string>{}( text->GetText().ToStdString() );
            ret += hash<bool>{}( text->IsItalic() );
            ret += hash<bool>{}( text->IsBold() );
            ret += hash<bool>{}( text->IsMirrored() );
            ret += hash<int>{}( text->GetTextWidth() );
            ret += hash<int>{}( text->GetTextHeight() );
            ret += hash<int>{}( text->GetHorizJustify() );
            ret += hash<int>{}( text->GetVertJustify() );

            if( aFlags & POSITION )
            {
                if( aFlags & REL_COORD )
                {
                    ret += hash<int>{}( text->GetPos0().x );
                    ret += hash<int>{}( text->GetPos0().y );
                }
                else
                {
                    ret += hash<int>{}( text->GetPosition().x );
                    ret += hash<int>{}( text->GetPosition().y );
                }
            }

            if( aFlags & ROTATION )
                ret += hash<double>{}( text->GetTextAngle() );
        }
        break;

    case PCB_MODULE_EDGE_T:
        {
            const EDGE_MODULE* segment = static_cast<const EDGE_MODULE*>( aItem );
            ret += hash_board_item( segment, aFlags );
            ret += hash<int>{}( segment->GetType() );
            ret += hash<int>{}( segment->GetShape() );
            ret += hash<int>{}( segment->GetWidth() );
            ret += hash<int>{}( segment->GetRadius() );

            if( aFlags & POSITION )
            {
                if( aFlags & REL_COORD )
                {
                    ret += hash<int>{}( segment->GetStart0().x );
                    ret += hash<int>{}( segment->GetStart0().y );
                    ret += hash<int>{}( segment->GetEnd0().x );
                    ret += hash<int>{}( segment->GetEnd0().y );
                }
                else
                {
                    ret += hash<int>{}( segment->GetStart().x );
                    ret += hash<int>{}( segment->GetStart().y );
                    ret += hash<int>{}( segment->GetEnd().x );
                    ret += hash<int>{}( segment->GetEnd().y );
                }
            }

            if( aFlags & ROTATION )
                ret += hash<double>{}( segment->GetAngle() );
        }
        break;

    default:
        wxASSERT_MSG( false, "Unhandled type in function hashModItem() (exporter_gencad.cpp)" );
    }

    return ret;
}
