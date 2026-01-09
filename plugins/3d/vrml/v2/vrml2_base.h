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
 * defines the base class for VRML2.0 nodes
 */

/*
 * Notes on deleting unsupported entities:
 * 1. PROTO: PROTO ProtoName [parameter list] {body}
 *      the parameter list will always have '[]'. So the items
 *      to delete are: String, List, Body
 * 2. EXTERNPROTO: EXTERNPROTO extern protoname [] MFstring
 *      delete: string, string, string, list, list
 * 3. Unsupported node types:  NodeName (Optional DEF RefName) {body}
 *      This scheme should also apply to PROTO'd node types.
 * 4. ROUTE:  ROUTE nodename1.event to nodename2.event
 *      Delete a String 3 times
 * 5. Script: Script { ... }
 */

#ifndef VRML2_BASE_H
#define VRML2_BASE_H

#include <list>
#include <string>
#include <map>

#include "vrml2_node.h"

class SGNODE;
class WRL2INLINE;

/**
 * The top node of a VRML2 model.
 */
class WRL2BASE : public WRL2NODE
{
public:
    WRL2BASE();
    virtual ~WRL2BASE();

    // function to enable/disable Inline{} expansion
    void SetEnableInline( bool enable );
    bool GetEnableInline( void );

    // function to check if unit conversion should be applied
    // when false, coordinates are used as-is (PCBnew export style with top-level scale)
    // when true, coordinates are multiplied by 2.54 (legacy KiCad 0.1 inch units)
    bool GetApplyUnitConversion( void ) const;
    void SetApplyUnitConversion( bool apply );

    // function to manipulate Inline{} objects
    SGNODE* GetInlineData( const std::string& aName );

    // function to read entire VRML file
    bool Read( WRLPROC& proc );

    // read in a VRML node
    bool ReadNode( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode );

    virtual std::string GetName( void ) override;
    virtual bool SetName( const std::string& aName ) override;

    bool Read( WRLPROC& proc, WRL2BASE* aTopNode ) override;
    bool SetParent( WRL2NODE* aParent, bool doUnlink = true ) override;
    SGNODE* TranslateToSG( SGNODE* aParent ) override;

    bool isDangling( void ) override;

private:
    // handle cases of USE / DEF
    bool implementUse( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode );
    bool implementDef( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode );

    bool readTransform( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode );
    bool readShape( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode );
    bool readAppearance( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode );
    bool readMaterial( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode );
    bool readFaceSet( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode );
    bool readLineSet( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode );
    bool readPointSet( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode );
    bool readCoords( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode );
    bool readNorms( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode );
    bool readColor( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode );
    bool readBox( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode );
    bool readSwitch( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode );
    bool readInline( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode );

    bool m_useInline;
    bool m_applyUnitConversion;  // if true, multiply coords by 2.54 (legacy mode)
    std::string m_dir;  // parent directory of the file
    std::map< std::string, SGNODE* > m_inlineModels;
};

#endif  // VRML2_BASE_H
