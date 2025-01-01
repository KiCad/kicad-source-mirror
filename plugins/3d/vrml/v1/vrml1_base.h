/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file vrmlv2_node.h
 * defines the base class for VRML1.0 nodes
 */

#ifndef VRML1_BASE_H
#define VRML1_BASE_H

#include <list>
#include <string>
#include <map>

#include "vrml1_node.h"

class SGNODE;
class WRL1INLINE;

/**
 * Represent the top node of a VRML1 model.
 */
class WRL1BASE : public WRL1NODE
{
public:
    WRL1BASE();
    virtual ~WRL1BASE();

    // function to read entire VRML file
    bool Read( WRLPROC& proc );

    // read in a VRML node
    bool ReadNode( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode );

    virtual std::string GetName( void ) override;
    virtual bool SetName( const std::string& aName ) override;

    bool Read( WRLPROC& proc, WRL1BASE* aTopNode ) override;
    bool SetParent( WRL1NODE* aParent, bool doUnlink = true ) override;
    SGNODE* TranslateToSG( SGNODE* aParent, WRL1STATUS* sp ) override;

private:
    // handle cases of USE / DEF
    bool implementUse( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode );
    bool implementDef( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode );

    bool readGroup( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode );
    bool readSeparator( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode );
    bool readSwitch( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode );
    bool readMaterial( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode );
    bool readMatBinding( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode );
    bool readCoords( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode );
    bool readFaceSet( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode );
    bool readTransform( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode );
    bool readShapeHints( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode );
};

#endif  // VRML1_BASE_H
