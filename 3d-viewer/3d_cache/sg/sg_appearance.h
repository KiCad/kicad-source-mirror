/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * defines the generic material appearance of a scenegraph object
 */

#ifndef SG_APPEARANCE_H
#define SG_APPEARANCE_H

#include "3d_cache/sg/sg_node.h"

class SGAPPEARANCE : public SGNODE
{
public:
    float shininess;    // default 0.2
    float transparency; // default 0.0
    SGCOLOR ambient;    // default 0.05317 0.17879 0.01804
    SGCOLOR diffuse;    // default 0.8 0.8 0.8
    SGCOLOR emissive;   // default 0.0 0.0 0.0
    SGCOLOR specular;   // default 0.0 0.0 0.0

    void unlinkChildNode( const SGNODE* aNode ) override;
    void unlinkRefNode( const SGNODE* aNode ) override;

public:
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

    SGNODE* FindNode(const char *aNodeName, const SGNODE *aCaller) override;
    bool AddRefNode( SGNODE* aNode ) override;
    bool AddChildNode( SGNODE* aNode ) override;

    void ReNameNodes( void ) override;
    bool WriteVRML( std::ofstream& aFile, bool aReuseFlag ) override;

    bool WriteCache( std::ofstream& aFile, SGNODE* parentNode ) override;
    bool ReadCache( std::ifstream& aFile, SGNODE* parentNode ) override;
};

#endif  // SG_APPEARANCE_H
