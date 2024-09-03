/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pcb_board_outline.h"
#include <eda_item_flags.h>
#include <eda_item.h>
#include <layer_ids.h>
#include <memory>
#include <board_item_container.h>

void PCB_BOARD_OUTLINE::CopyPros( const PCB_BOARD_OUTLINE& aCopyFrom, PCB_BOARD_OUTLINE& aCopyTo )
{
    aCopyTo.m_outlines = aCopyFrom.m_outlines;
    aCopyTo.m_parent = aCopyFrom.m_parent;
}


PCB_BOARD_OUTLINE::PCB_BOARD_OUTLINE( BOARD_ITEM* aParent ) :
        BOARD_ITEM( aParent, KICAD_T::PCB_BOARD_OUTLINE_T, Edge_Cuts )
{
    SetFlags( SKIP_STRUCT );
}

PCB_BOARD_OUTLINE::PCB_BOARD_OUTLINE( const PCB_BOARD_OUTLINE& aOther ) :
        PCB_BOARD_OUTLINE( aOther.GetParent() )
{
    CopyPros( aOther, *this );
}

PCB_BOARD_OUTLINE& PCB_BOARD_OUTLINE::operator=( const PCB_BOARD_OUTLINE& aOther )
{
    CopyPros( aOther, *this );
    return *this;
}

PCB_BOARD_OUTLINE::~PCB_BOARD_OUTLINE()
{
}


std::vector<int> PCB_BOARD_OUTLINE::ViewGetLayers() const
{
    std::vector<int> layers { LAYER_BOARD_OUTLINE_AREA };
    return layers;
}


const BOX2I PCB_BOARD_OUTLINE::GetBoundingBox() const
{
    return m_outlines.BBox();
}


PCB_LAYER_ID PCB_BOARD_OUTLINE::GetLayer() const
{
    return UNDEFINED_LAYER;
}


LSET PCB_BOARD_OUTLINE::GetLayerSet() const
{
    return {};
}

bool PCB_BOARD_OUTLINE::IsOnLayer( PCB_LAYER_ID aLayer ) const
{
    WXUNUSED( aLayer )
    return {};
}

double PCB_BOARD_OUTLINE::Similarity( const BOARD_ITEM& aItem ) const
{
    if( aItem.Type() == KICAD_T::PCB_BOARD_OUTLINE_T )
    {
        if( m_parent == aItem.GetParent() )
            return 1.0;

        return 0.5;
    }

    return 0.0;
}

bool PCB_BOARD_OUTLINE::operator==( const BOARD_ITEM& aItem ) const
{
    if( const PCB_BOARD_OUTLINE* outline = dynamic_cast<const PCB_BOARD_OUTLINE*>( &aItem ) )
    {
        return outline->m_parent == m_parent;
    }

    return {};
}

EDA_ITEM* PCB_BOARD_OUTLINE::Clone() const
{
    return new PCB_BOARD_OUTLINE( *this );
}

wxString PCB_BOARD_OUTLINE::GetClass() const
{
    return "PCB_BOARD_OUTLINE";
}


#if defined( DEBUG )

void PCB_BOARD_OUTLINE::Show( int nestLevel, std::ostream& os ) const
{
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() << ">\n";
}

#endif
