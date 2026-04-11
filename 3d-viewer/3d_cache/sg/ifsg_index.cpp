/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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


#include <iostream>
#include <sstream>
#include <wx/log.h>

#include "plugins/3dapi/ifsg_index.h"
#include "3d_cache/sg/sg_coordindex.h"


IFSG_INDEX::IFSG_INDEX() : IFSG_NODE()
{
}


bool IFSG_INDEX::GetIndices( size_t& nIndices, int*& aIndexList )
{
    wxCHECK( m_node, false );

    return ( (SGINDEX*) m_node )->GetIndices( nIndices, aIndexList );
}


bool IFSG_INDEX::SetIndices( size_t nIndices, int* aIndexList )
{
    wxCHECK( m_node, false );

    ( (SGINDEX*) m_node )->SetIndices( nIndices, aIndexList );

    return true;
}


bool IFSG_INDEX::AddIndex( int aIndex )
{
    wxCHECK( m_node, false );

    ( (SGINDEX*) m_node )->AddIndex( aIndex );

    return true;
}
