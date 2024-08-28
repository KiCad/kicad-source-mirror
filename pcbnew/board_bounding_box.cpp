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

#include "board_bounding_box.h"

#include <memory>
#include <board_item_container.h>

BOARD_BOUNDING_BOX::BOARD_BOUNDING_BOX( BOX2I const& aBoundingBox ) :
        EDA_ITEM( KICAD_T::PCB_BOUNDING_BOX_T ),
        m_boundingBox( std::make_shared<BOX2I>( aBoundingBox ) )
{
    SetFlags( SKIP_STRUCT );
}

BOARD_BOUNDING_BOX::BOARD_BOUNDING_BOX( const BOARD_BOUNDING_BOX& aOther ) :
        EDA_ITEM( KICAD_T::PCB_BOUNDING_BOX_T ), m_boundingBox( aOther.m_boundingBox )
{
    SetFlags( SKIP_STRUCT );
}

BOARD_BOUNDING_BOX& BOARD_BOUNDING_BOX::operator=( const BOARD_BOUNDING_BOX& aOther )
{
    m_parent = aOther.m_parent;
    m_boundingBox = aOther.m_boundingBox;
    return *this;
}

EDA_ITEM* BOARD_BOUNDING_BOX::Clone() const
{
    return new BOARD_BOUNDING_BOX( *this );
}

BOARD_BOUNDING_BOX::~BOARD_BOUNDING_BOX()
{
}

const BOX2I BOARD_BOUNDING_BOX::GetBoundingBox() const
{
    return *m_boundingBox;
}

void BOARD_BOUNDING_BOX::SetBoundingBox( BOX2I const& aBoundingBox )
{
    m_boundingBox = std::make_shared<BOX2I>( aBoundingBox );
}

wxString BOARD_BOUNDING_BOX::GetClass() const
{
    return "BOARD_BOUNDING_BOX";
}

void BOARD_BOUNDING_BOX::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 1;
    aLayers[0] = LAYER_BOARD_BOUNDING_BOX;
}

#if defined( DEBUG )


void BOARD_BOUNDING_BOX::Show( int nestLevel, std::ostream& os ) const
{
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() << m_boundingBox->Format()
                                 << "/>\n";
}

#endif
