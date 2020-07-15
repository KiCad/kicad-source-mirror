/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <sstream>
#include <string>

#include <geometry/shape_compound.h>

const std::string SHAPE_COMPOUND::Format() const
{
    std::stringstream ss;

    ss << "compound";

    // fixme: implement

    return ss.str();
}

SHAPE_COMPOUND::SHAPE_COMPOUND( const std::vector<SHAPE*>& aShapes ) :
         SHAPE( SH_COMPOUND ),
         m_dirty( true ),
         m_shapes( aShapes )
{

}


SHAPE_COMPOUND::SHAPE_COMPOUND( const SHAPE_COMPOUND& aOther )
    : SHAPE( SH_COMPOUND )
{
    for ( auto shape : aOther.Shapes() )
        m_shapes.push_back( shape->Clone() );
}




SHAPE_COMPOUND::~SHAPE_COMPOUND()
{
    for( auto shape : m_shapes )
        delete shape;
}


SHAPE_COMPOUND* SHAPE_COMPOUND::Clone() const
{
    return new SHAPE_COMPOUND( *this );
}


const BOX2I SHAPE_COMPOUND::BBox( int aClearance ) const
{

}


int SHAPE_COMPOUND::Distance( const SEG& aSeg ) const
{

}


void SHAPE_COMPOUND::Rotate( double aAngle, const VECTOR2I& aCenter )
{
}


bool SHAPE_COMPOUND::IsSolid() const
{
    return true;
}


bool SHAPE_COMPOUND::Collide( const SEG& aSeg, int aClearance, int* aActual ) const
{
    for( auto& item : m_shapes )
    {
        if( item->Collide( aSeg, aClearance ) )
            return true;
    }

    return false;
}
