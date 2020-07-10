/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

    if( aFlags & HASH_LAYER )
        ret = hash<unsigned long long>{}( aItem->GetLayerSet().to_ullong() );

    return ret;
}


size_t hash_eda( const EDA_ITEM* aItem, int aFlags )
{
    size_t ret;

    switch( aItem->Type() )
    {
    case PCB_MODULE_T:
        {
            const MODULE* module = static_cast<const MODULE*>( aItem );

            ret    = hash_board_item( module, aFlags );

            if( aFlags & HASH_POS )
                hash_combine( ret, module->GetPosition().x, module->GetPosition().y );

            if( aFlags & HASH_ROT )
                hash_combine( ret, module->GetOrientation() );

            for( auto i : module->GraphicalItems() )
                hash_combine( ret, hash_eda( i, aFlags ) );

            for( auto i : module->Pads() )
                hash_combine( ret, hash_eda( static_cast<EDA_ITEM*>( i ), aFlags ) );
        }
        break;

    case PCB_PAD_T:
        {
            const D_PAD* pad = static_cast<const D_PAD*>( aItem );

            ret = hash<int>{}( pad->GetShape() << 16 );
            hash_combine( ret, pad->GetDrillShape() << 18 );
            hash_combine( ret, pad->GetSize().x << 8 );
            hash_combine( ret, pad->GetSize().y << 9 );
            hash_combine( ret, pad->GetOffset().x << 6 );
            hash_combine( ret, pad->GetOffset().y << 7 );
            hash_combine( ret, pad->GetDelta().x << 4 );
            hash_combine( ret, pad->GetDelta().y << 5 );

            hash_combine( ret, hash_board_item( pad, aFlags ) );

            if( aFlags & HASH_POS )
            {
                if( aFlags & REL_COORD )
                    hash_combine( ret, pad->GetPos0().x, pad->GetPos0().y );
                else
                    hash_combine( ret, pad->GetPosition().x, pad->GetPosition().y );
            }

            if( aFlags & HASH_ROT )
                hash_combine( ret, pad->GetOrientation() );

            if( aFlags & HASH_NET )
                hash_combine( ret, pad->GetNetCode() );
        }
        break;

    case PCB_MODULE_TEXT_T:
        {
            const TEXTE_MODULE* text = static_cast<const TEXTE_MODULE*>( aItem );

            if( !( aFlags & HASH_REF ) && text->GetType() == TEXTE_MODULE::TEXT_is_REFERENCE )
                break;

            if( !( aFlags & HASH_VALUE ) && text->GetType() == TEXTE_MODULE::TEXT_is_VALUE )
                break;

            ret = hash_board_item( text, aFlags );
            hash_combine( ret, text->GetText().ToStdString() );
            hash_combine( ret,  text->IsItalic() );
            hash_combine( ret, text->IsBold() );
            hash_combine( ret, text->IsMirrored() );
            hash_combine( ret, text->GetTextWidth() );
            hash_combine( ret, text->GetTextHeight() );
            hash_combine( ret, text->GetHorizJustify() );
            hash_combine( ret, text->GetVertJustify() );

            if( aFlags & HASH_POS )
            {
                if( aFlags & REL_COORD )
                    hash_combine( ret, text->GetPos0().x, text->GetPos0().y );
                else
                    hash_combine( ret, text->GetPosition().x, text->GetPosition().y );
            }

            if( aFlags & HASH_ROT )
                hash_combine( ret, text->GetTextAngle() );
        }
        break;

    case PCB_MODULE_EDGE_T:
        {
            const EDGE_MODULE* segment = static_cast<const EDGE_MODULE*>( aItem );
            ret = hash_board_item( segment, aFlags );
            hash_combine( ret, segment->GetType() );
            hash_combine( ret, segment->GetShape() );
            hash_combine( ret, segment->GetWidth() );
            hash_combine( ret, segment->GetRadius() );

            if( aFlags & HASH_POS )
            {
                if( aFlags & REL_COORD )
                {
                    hash_combine( ret, segment->GetStart0().x );
                    hash_combine( ret, segment->GetStart0().y );
                    hash_combine( ret, segment->GetEnd0().x );
                    hash_combine( ret, segment->GetEnd0().y );
                }
                else
                {
                    hash_combine( ret, segment->GetStart().x );
                    hash_combine( ret, segment->GetStart().y );
                    hash_combine( ret, segment->GetEnd().x );
                    hash_combine( ret, segment->GetEnd().y );
                }
            }

            if( aFlags & HASH_ROT )
                hash_combine( ret, segment->GetAngle() );
        }
        break;

    default:
        wxASSERT_MSG( false, "Unhandled type in function hashModItem() (exporter_gencad.cpp)" );
    }

    return ret;
}
