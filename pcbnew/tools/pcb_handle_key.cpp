/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pcb_handle_key.h"

#include <tool/edit_points.h>


PCB_HANDLE_KEY::PCB_HANDLE_KEY( const KIID& aParent, bool aIsLine, int aIndex, unsigned aPointCount,
                                unsigned aLineCount ) :
        m_parent( aParent ),
        m_isLine( aIsLine ),
        m_index( aIndex ),
        m_pointCount( aPointCount ),
        m_lineCount( aLineCount )
{
}


std::optional<PCB_HANDLE_KEY> PCB_HANDLE_KEY::Make( const EDIT_POINTS& aPoints, const EDIT_POINT* aPoint )
{
    const EDA_ITEM* parent = aPoints.GetParent();

    if( !aPoint || !parent )
        return std::nullopt;

    for( unsigned ii = 0; ii < aPoints.PointsSize(); ++ii )
    {
        if( &aPoints.Point( ii ) == aPoint )
        {
            return PCB_HANDLE_KEY( parent->m_Uuid, false, ii, aPoints.PointsSize(), aPoints.LinesSize() );
        }
    }

    for( unsigned ii = 0; ii < aPoints.LinesSize(); ++ii )
    {
        if( &aPoints.Line( ii ) == aPoint )
        {
            return PCB_HANDLE_KEY( parent->m_Uuid, true, ii, aPoints.PointsSize(), aPoints.LinesSize() );
        }
    }

    return std::nullopt;
}


EDIT_POINT* PCB_HANDLE_KEY::Resolve( EDIT_POINTS& aPoints ) const
{
    const EDA_ITEM* parent = aPoints.GetParent();

    if( !parent || parent->m_Uuid != m_parent )
        return nullptr;

    if( aPoints.PointsSize() != m_pointCount || aPoints.LinesSize() != m_lineCount )
        return nullptr;

    if( m_isLine )
    {
        if( (unsigned) m_index >= aPoints.LinesSize() )
            return nullptr;

        return &aPoints.Line( m_index );
    }

    if( (unsigned) m_index >= aPoints.PointsSize() )
        return nullptr;

    return &aPoints.Point( m_index );
}


bool PCB_HANDLE_KEY::operator==( const PCB_HANDLE_KEY& aOther ) const
{
    return m_parent == aOther.m_parent && m_isLine == aOther.m_isLine && m_index == aOther.m_index;
}


bool PCB_HANDLE_KEY::operator<( const PCB_HANDLE_KEY& aOther ) const
{
    if( m_parent != aOther.m_parent )
        return m_parent < aOther.m_parent;

    if( m_isLine != aOther.m_isLine )
        return m_isLine < aOther.m_isLine;

    return m_index < aOther.m_index;
}
