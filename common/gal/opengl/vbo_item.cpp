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
 * @file vbo_item.cpp
 * @brief Class to handle an item held in a Vertex Buffer Object.
 */

#include <gal/opengl/vbo_container.h>
#include <gal/opengl/vbo_item.h>
#include <cstring>

using namespace KiGfx;

VBO_ITEM::VBO_ITEM( VBO_CONTAINER* aContainer ) :
        m_offset( 0 ),
        m_size( 0 ),
        m_container( aContainer ),
        m_isDirty( true )
{
    // The item's size is not known yet, so we just start an item in the container
    aContainer->StartItem( this );
}


VBO_ITEM::~VBO_ITEM()
{
    m_container->Free( this );
}


void VBO_ITEM::PushVertex( const VBO_VERTEX* aVertex )
{
    m_container->Add( this, aVertex );

    m_size++;
    m_isDirty = true;
}


void VBO_ITEM::PushVertices( const VBO_VERTEX* aVertices, GLuint aSize )
{
    for( unsigned int i = 0; i < aSize; ++i )
    {
        PushVertex( &aVertices[i] );
    }
}


VBO_VERTEX* VBO_ITEM::GetVertices()
{
    if( m_isDirty )
        Finish();

    return m_container->GetVertices( m_offset );
}


void VBO_ITEM::ChangeColor( const COLOR4D& aColor )
{
    VBO_VERTEX* vertexPtr = GetVertices();

    for( unsigned int i = 0; i < m_size; ++i )
    {
        vertexPtr->r = aColor.r * 255;
        vertexPtr->g = aColor.g * 255;
        vertexPtr->b = aColor.b * 255;
        vertexPtr->a = aColor.a * 255;

        // Move on to the next vertex
        vertexPtr++;
    }
}


void VBO_ITEM::ChangeDepth( int aDepth )
{
    VBO_VERTEX* vertexPtr = GetVertices();

    for( unsigned int i = 0; i < m_size; ++i )
    {
        vertexPtr->z = aDepth;

        // Move on to the next vertex
        vertexPtr++;
    }
}


void VBO_ITEM::Finish()
{
    // The unknown-sized item has just ended, so we need to inform the container about it
    m_container->EndItem();

    m_isDirty = false;
}
