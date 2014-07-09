/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

/**
 * @file vertex_container.cpp
 * @brief Class to store vertices and handle transfers between system memory and GPU memory.
 */

#include <gal/opengl/vertex_container.h>
#include <gal/opengl/cached_container.h>
#include <gal/opengl/noncached_container.h>
#include <gal/opengl/shader.h>
#include <cstdlib>
#include <cstring>

using namespace KIGFX;

VERTEX_CONTAINER* VERTEX_CONTAINER::MakeContainer( bool aCached )
{
    if( aCached )
        return new CACHED_CONTAINER;
    else
        return new NONCACHED_CONTAINER;
}


VERTEX_CONTAINER::VERTEX_CONTAINER( unsigned int aSize ) :
    m_freeSpace( aSize ), m_currentSize( aSize ), m_initialSize( aSize ),
    m_failed( false ), m_dirty( true )
{
    m_vertices = static_cast<VERTEX*>( malloc( aSize * sizeof( VERTEX ) ) );
    memset( m_vertices, 0x00, aSize * sizeof( VERTEX ) );
}


VERTEX_CONTAINER::~VERTEX_CONTAINER()
{
    free( m_vertices );
}
