/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * Class WRL1BASE
 * represents the top node of a VRML1 model
 */
class WRL1BASE : public WRL1NODE
{
private:
    bool m_useInline;

    // handle cases of USE / DEF
    bool implementUse( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode );
    bool implementDef( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode );

    bool readSeparator( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode );

    std::map< std::string, WRL1INLINE* > m_inlineModels;

public:
    WRL1BASE();
    virtual ~WRL1BASE();

    // function to enable/disable Inline{} expansion
    void SetEnableInline( bool enable );
    bool GetEnableInline( void );

    // functions to manipulate Inline{} objects
    SGNODE* AddInlineData( const std::string& aName, WRL1INLINE* aObject );
    SGNODE* GetInlineData( const std::string& aName, WRL1INLINE* aObject );
    void DelInlineData( const std::string& aName, WRL1INLINE* aObject );

    // function to read entire VRML file
    bool Read( WRLPROC& proc );

    // read in a VRML node
    bool ReadNode( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode );

    // overrides
    virtual std::string GetName( void );
    virtual bool SetName( const std::string& aName );

    // functions inherited from WRL1NODE
    bool Read( WRLPROC& proc, WRL1BASE* aTopNode );
    bool SetParent( WRL1NODE* aParent );
    SGNODE* TranslateToSG( SGNODE* aParent, bool calcNormals );
};

#endif  // VRML1_BASE_H
