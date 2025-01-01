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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file sg_coordindex.h
 */

#ifndef SG_COORDINDEX_H
#define SG_COORDINDEX_H

#include "3d_cache/sg/sg_index.h"

/**
 * An object to maintain a coordinate index list.
 *
 * Users must ensure that coordinate indices are specified as triplets (triangular faces)
 * since no checking is performed.  In instances where it is not possible to determine which
 * side of the triangle is to be rendered (for example IGES entities) then the user must
 * supply each triplet in both point orders.
 */
class SGCOORDINDEX : public SGINDEX
{
public:
    SGCOORDINDEX( SGNODE* aParent );
    virtual ~SGCOORDINDEX();

    /**
     * Add all coordinate indices to the given list in preparation for a normals calculation.
     */
    void GatherCoordIndices( std::vector< int >& aIndexList );
};

#endif  // SG_COORDINDEX_H
