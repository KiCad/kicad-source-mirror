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
#include <gal/opengl/cached_container_ram.h>
#include <gal/opengl/cached_container_gpu.h>
#include <gal/opengl/noncached_container.h>
#include <gal/opengl/shader.h>

#include <cstring>

using namespace KIGFX;

VERTEX_CONTAINER* VERTEX_CONTAINER::MakeContainer( bool aCached )
{
    if( aCached )
    {
        const char* vendor = (const char*) glGetString( GL_VENDOR );

        // Open source drivers do not cope well with GPU memory mapping,
        // so the vertex data has to be kept in RAM
        if( strstr( vendor, "X.Org" ) || strstr( vendor, "nouveau" ) )
            return new CACHED_CONTAINER_RAM;
        else
            return new CACHED_CONTAINER_GPU;
    }

    return new NONCACHED_CONTAINER;
}


VERTEX_CONTAINER::VERTEX_CONTAINER( unsigned int aSize ) :
        m_freeSpace( aSize ),
        m_currentSize( aSize ),
        m_initialSize( aSize ),
        m_vertices( nullptr ),
        m_failed( false ),
        m_dirty( true )
{
}


VERTEX_CONTAINER::~VERTEX_CONTAINER()
{
}
