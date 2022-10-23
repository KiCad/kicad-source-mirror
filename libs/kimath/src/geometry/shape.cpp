/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
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


#include <geometry/shape.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_compound.h>

bool SHAPE::Parse( std::stringstream& aStream )
{
    assert( false );
    return false;
}


const std::string SHAPE::Format( bool aCplusPlus ) const
{
    std::stringstream ss;
    ss << "shape " << m_type;
    return ss.str();
}


int SHAPE::GetClearance( const SHAPE* aOther ) const
{
    int actual_clearance = std::numeric_limits<int>::max();
    std::vector<const SHAPE*> a_shapes;
    std::vector<const SHAPE*> b_shapes;

    GetIndexableSubshapes( a_shapes );
    aOther->GetIndexableSubshapes( b_shapes );

    if( GetIndexableSubshapeCount() == 0 )
        a_shapes.push_back( this );

    if( aOther->GetIndexableSubshapeCount() == 0 )
        b_shapes.push_back( aOther );

    for( const SHAPE* a : a_shapes )
    {
        for( const SHAPE* b : b_shapes )
        {
            int temp_dist = 0;
            a->Collide( b, std::numeric_limits<int>::max() / 2, &temp_dist );

            if( temp_dist < actual_clearance )
                actual_clearance = temp_dist;
        }
    }

    return actual_clearance;
}
