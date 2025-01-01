/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2017 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file sg_colors.h
 */

#ifndef SG_COLORS_H
#define SG_COLORS_H

#include <vector>
#include "3d_cache/sg/sg_node.h"

/**
 * Define an RGB color set for a scenegraph object.
 */
class SGCOLORS : public SGNODE
{
public:
    SGCOLORS( SGNODE* aParent );
    virtual ~SGCOLORS();

    virtual bool SetParent( SGNODE* aParent, bool notify = true ) override;

    void unlinkChildNode( const SGNODE* aNode ) noexcept override;
    void unlinkRefNode( const SGNODE* aNode ) noexcept override;

    SGNODE* FindNode(const char *aNodeName, const SGNODE *aCaller) noexcept override;
    bool AddRefNode( SGNODE* aNode ) noexcept override;
    bool AddChildNode( SGNODE* aNode ) noexcept override;

    bool GetColorList( size_t& aListSize, SGCOLOR*& aColorList );
    void SetColorList( size_t aListSize, const SGCOLOR* aColorList );
    void AddColor( double aRedValue, double aGreenValue, double aBlueValue );
    void AddColor( const SGCOLOR& aColor );

    void ReNameNodes( void ) override;
    bool WriteVRML( std::ostream& aFile, bool aReuseFlag ) override;

    bool WriteCache( std::ostream& aFile, SGNODE* parentNode ) override;
    bool ReadCache( std::istream& aFile, SGNODE* parentNode ) override;

    std::vector< SGCOLOR > colors;
};

#endif  // SG_COLORS_H
