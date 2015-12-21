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

#include "vrml2_node.h"

// BUG: there are no referenced nodes; however it is indeed
// possible for the BASE node to have reference nodes, for example:
// DEF BLAH Transform{}
// USE BLAH
// The code must be adjusted to respond to unlink requests and addNodeRef requests

/**
 * Class WRL2BASE
 * represents the top node of a VRML2 model
 */
class WRL2BASE : public WRL2NODE
{
protected:
    std::list< WRL2NODE* > m_Children;      // nodes owned by this node

public:

    // functions inherited from WRL2NODE
    void unlinkChildNode( const WRL2NODE* aNode );
    void unlinkRefNode( const WRL2NODE* aNode );
    bool isDangling( void );

public:
    WRL2BASE();
    virtual ~WRL2BASE();

    // overrides
    virtual const char* GetName( void );
    virtual bool SetName(const char *aName);

    // functions inherited from WRL2NODE
    bool Read( WRLPROC& proc );
    bool SetParent( WRL2NODE* aParent );
    WRL2NODE* FindNode( const char *aNodeName, const WRL2NODE *aCaller );
    bool AddRefNode( WRL2NODE* aNode );
    bool AddChildNode( WRL2NODE* aNode );
};

#endif  // VRML2_BASE_H
