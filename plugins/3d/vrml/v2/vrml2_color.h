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
 * @file vrml2_color.h
 */


#ifndef VRML2_COLOR_H
#define VRML2_COLOR_H

#include <vector>

#include "vrml2_node.h"

class WRL2BASE;
class SGNODE;

class WRL2COLOR : public WRL2NODE
{
public:
    WRL2COLOR();
    WRL2COLOR( WRL2NODE* aParent );
    virtual ~WRL2COLOR();

    bool Read( WRLPROC& proc, WRL2BASE* aTopNode ) override;
    bool AddRefNode( WRL2NODE* aNode ) override;
    bool AddChildNode( WRL2NODE* aNode ) override;
    SGNODE* TranslateToSG( SGNODE* aParent ) override;

    /**
     * @return true if the color set is non-empty.
     */
    bool HasColors( void );

    /**
     * Retrieve the given color (or default 0.8, 0.8, 0.8 if index is invalid).
     */
    void GetColor( int aIndex, float& red, float& green, float& blue );

    /**
     * Retrieve the current list of colors.
     */
    void GetColors( WRLVEC3F*& aColorList, size_t& aListSize);

    bool isDangling( void ) override;

private:
    std::vector< WRLVEC3F > colors;
};

#endif  // VRML2_COLOR_H
