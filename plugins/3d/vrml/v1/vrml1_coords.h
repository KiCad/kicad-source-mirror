/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file vrml1_coords.h
 */


#ifndef VRML1_COORDS_H
#define VRML1_COORDS_H

#include <vector>

#include "vrml1_node.h"

class WRL1BASE;
class SGNODE;


class WRL1COORDS : public WRL1NODE
{
public:
    WRL1COORDS( NAMEREGISTER* aDictionary );
    WRL1COORDS( NAMEREGISTER* aDictionary, WRL1NODE* aParent );
    virtual ~WRL1COORDS();

    bool Read( WRLPROC& proc, WRL1BASE* aTopNode ) override;
    bool AddRefNode( WRL1NODE* aNode ) override;
    bool AddChildNode( WRL1NODE* aNode ) override;
    SGNODE* TranslateToSG( SGNODE* aParent, WRL1STATUS* sp ) override;

    void GetCoords( WRLVEC3F*& aCoordList, size_t& aListSize );

private:
    std::vector< WRLVEC3F > points;
};

#endif  // VRML1_COORDS_H
