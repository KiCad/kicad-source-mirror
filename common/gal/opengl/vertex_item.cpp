/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file vertex_item.cpp
 * @brief Class to handle an item held in a container.
 */

#include <gal/opengl/vertex_item.h>
#include <gal/opengl/vertex_manager.h>
#include <cstring>

using namespace KIGFX;

VERTEX_ITEM::VERTEX_ITEM( const VERTEX_MANAGER& aManager ) :
        m_manager( aManager ),
        m_offset( 0 ),
        m_size( 0 )
{
    // As the item is created, we are going to modify it, so call to SetItem() is needed
    m_manager.SetItem( *this );
}


VERTEX_ITEM::~VERTEX_ITEM()
{
    m_manager.FreeItem( *this );
}


VERTEX* VERTEX_ITEM::GetVertices() const
{
    return m_manager.GetVertices( *this );
}
