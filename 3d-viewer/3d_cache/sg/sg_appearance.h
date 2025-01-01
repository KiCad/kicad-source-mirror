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
 * @file sg_appearance.h
 */

#ifndef SG_APPEARANCE_H
#define SG_APPEARANCE_H

#include "3d_cache/sg/sg_node.h"

/**
 * Defines the generic material appearance of a scenegraph object.
 */
class SGAPPEARANCE : public SGNODE
{
public:
    void unlinkChildNode( const SGNODE* aNode ) noexcept override;
    void unlinkRefNode( const SGNODE* aNode ) noexcept override;

    SGAPPEARANCE( SGNODE* aParent );
    virtual ~SGAPPEARANCE();

    virtual bool SetParent( SGNODE* aParent, bool notify = true ) override;

    bool SetEmissive( float aRVal, float aGVal, float aBVal );
    bool SetEmissive( const SGCOLOR* aRGBColor );
    bool SetEmissive( const SGCOLOR& aRGBColor );

    bool SetDiffuse( float aRVal, float aGVal, float aBVal );
    bool SetDiffuse( const SGCOLOR* aRGBColor );
    bool SetDiffuse( const SGCOLOR& aRGBColor );

    bool SetSpecular( float aRVal, float aGVal, float aBVal );
    bool SetSpecular( const SGCOLOR* aRGBColor );
    bool SetSpecular( const SGCOLOR& aRGBColor );

    bool SetAmbient( float aRVal, float aGVal, float aBVal );
    bool SetAmbient( const SGCOLOR* aRGBColor );
    bool SetAmbient( const SGCOLOR& aRGBColor );

    SGNODE* FindNode(const char *aNodeName, const SGNODE *aCaller) noexcept override;
    bool AddRefNode( SGNODE* aNode ) noexcept override;
    bool AddChildNode( SGNODE* aNode ) noexcept override;

    void ReNameNodes( void ) override;
    bool WriteVRML( std::ostream& aFile, bool aReuseFlag ) override;

    bool WriteCache( std::ostream& aFile, SGNODE* parentNode ) override;
    bool ReadCache( std::istream& aFile, SGNODE* parentNode ) override;

    float shininess;    // default 0.2
    float transparency; // default 0.0
    SGCOLOR ambient;    // default 0.05317 0.17879 0.01804
    SGCOLOR diffuse;    // default 0.8 0.8 0.8
    SGCOLOR emissive;   // default 0.0 0.0 0.0
    SGCOLOR specular;   // default 0.0 0.0 0.0

};

#endif  // SG_APPEARANCE_H
