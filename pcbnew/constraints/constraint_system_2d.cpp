/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include <constraints/constraint_system_2d.h>

#include <algorithm>
#include <cassert>

#include <GCS.h>


CONSTRAINT_SYSTEM_2D::CONSTRAINT_SYSTEM_2D() :
        m_solver( std::make_unique<GCS::System>() )
{
}


CONSTRAINT_SYSTEM_2D::~CONSTRAINT_SYSTEM_2D()
{
    m_solver->clear();
}


void CONSTRAINT_SYSTEM_2D::SetCoordinateFrame( const VECTOR2I& aOrigin, double aScale )
{
    assert( aScale > 0.0 );

    m_originX = aOrigin.x;
    m_originY = aOrigin.y;
    m_scale = aScale;
    m_inverseScale = 1.0 / aScale;
}


double CONSTRAINT_SYSTEM_2D::NormalizeX( int aIU ) const
{
    return ( aIU - m_originX ) * m_inverseScale;
}


double CONSTRAINT_SYSTEM_2D::NormalizeY( int aIU ) const
{
    return ( aIU - m_originY ) * m_inverseScale;
}


double CONSTRAINT_SYSTEM_2D::DenormalizeX( double aNormalized ) const
{
    return aNormalized * m_scale + m_originX;
}


double CONSTRAINT_SYSTEM_2D::DenormalizeY( double aNormalized ) const
{
    return aNormalized * m_scale + m_originY;
}


int CONSTRAINT_SYSTEM_2D::AddParameter( double aValue )
{
    m_parameters.push_back( aValue );
    return static_cast<int>( m_parameters.size() ) - 1;
}


double& CONSTRAINT_SYSTEM_2D::Parameter( int aIndex )
{
    return m_parameters.at( aIndex );
}


const double& CONSTRAINT_SYSTEM_2D::Parameter( int aIndex ) const
{
    return m_parameters.at( aIndex );
}


double* CONSTRAINT_SYSTEM_2D::ParameterPointer( int aIndex )
{
    return &m_parameters.at( aIndex );
}


CONSTRAINT_SYSTEM_2D::SNAPSHOT CONSTRAINT_SYSTEM_2D::Snapshot() const
{
    return { m_parameters.begin(), m_parameters.end() };
}


bool CONSTRAINT_SYSTEM_2D::Restore( const SNAPSHOT& aSnapshot )
{
    if( aSnapshot.size() != m_parameters.size() )
        return false;

    std::copy( aSnapshot.begin(), aSnapshot.end(), m_parameters.begin() );
    return true;
}


bool CONSTRAINT_SYSTEM_2D::RestorePrefix( const SNAPSHOT& aSnapshot )
{
    if( aSnapshot.size() > m_parameters.size() )
        return false;

    std::copy( aSnapshot.begin(), aSnapshot.end(), m_parameters.begin() );
    return true;
}


void CONSTRAINT_SYSTEM_2D::Clear()
{
    m_solver->clear();
    m_parameters.clear();
}


GCS::System& CONSTRAINT_SYSTEM_2D::Solver()
{
    return *m_solver;
}


const GCS::System& CONSTRAINT_SYSTEM_2D::Solver() const
{
    return *m_solver;
}
