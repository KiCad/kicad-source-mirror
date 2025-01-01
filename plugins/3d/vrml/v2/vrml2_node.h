/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef VRML2_NODE_H
#define VRML2_NODE_H

#include <list>
#include <string>

#include "wrlproc.h"

class WRL2BASE;
class SGNODE;

class WRL2NODE
{
public:
    WRL2NODE();
    virtual ~WRL2NODE();

    // read data via the given file processor and WRL2BASE object
    virtual bool Read( WRLPROC& proc, WRL2BASE* aTopNode ) = 0;

    /**
     * @return the type of this node instance.
     */
    WRL2NODES GetNodeType( void ) const;

    /**
     * @return a pointer to the parent SGNODE of this object or NULL if the object has no
     *         parent (ie. top level transform).
     */
    WRL2NODE* GetParent( void ) const;

    /**
     * Set the parent WRL2NODE of this object.
     *
     * @param aParent [in] is the desired parent node.
     * @param doUnlink indicates that the child must be unlinked from the parent
     * @return true if the operation succeeds of false if the given node is not allowed to be
     *         a parent to the derived object.
     */
    virtual bool SetParent( WRL2NODE* aParent, bool doUnlink = true );

    virtual std::string GetName( void );
    virtual bool SetName( const std::string& aName );

    const char* GetNodeTypeName( WRL2NODES aNodeType ) const;

    /**
     * Search the tree of linked nodes and returns a reference to the first node found with
     * the given name. The reference is then typically added to another node via AddRefNode().
     *
     * @param aNodeName is the name of the node to search for.
     * @param aCaller is a pointer to the node invoking this function.
     * @return is a valid node pointer on success or NULL.
     */
    virtual WRL2NODE* FindNode( const std::string& aNodeName, const WRL2NODE *aCaller );

    virtual bool AddChildNode( WRL2NODE* aNode );

    virtual bool AddRefNode( WRL2NODE* aNode );

    std::string GetError( void );

    /**
     * Produce a representation of the data using the intermediate scenegraph structures of the
     * kicad_3dsg library.
     *
     * @param aParent is a pointer to the parent SG node.
     * @return is non-NULL on success.
     */
    virtual SGNODE* TranslateToSG( SGNODE* aParent ) = 0;

    /**
     * @return The ID based on the given aNodeName or WRL2_INVALID (WRL2_END) if no such node
     *         name exists.
     */
    WRL2NODES getNodeTypeID( const std::string& aNodeName );

    /**
     * Remove references to an owned child.
     *
     * It is invoked by the child upon destruction to ensure that the parent has no invalid
     * references.
     *
     * @param aNode is the child which is being deleted.
     */
    virtual void unlinkChildNode( const WRL2NODE* aNode );

    /**
     * Remove pointers to a referenced node.
     *
     * It is invoked by the referenced node upon destruction to ensure that the referring node
     * has no invalid references.
     *
     * @param aNode is the node which is being deleted.
     */
    virtual void unlinkRefNode( const WRL2NODE* aNode );

    /**
     * Add a pointer to a node which references but does not own this node.
     *
     * Such back-pointers are required to ensure that invalidated references are removed when
     * a node is deleted.
     *
     * @param aNode is the node holding a reference to this object.
     */
    void addNodeRef( WRL2NODE* aNode );

    /**
     * Remove a pointer to a node which references but does not own this node.
     *
     * @param aNode is the node holding a reference to this object
     */
    void delNodeRef( WRL2NODE* aNode );

    /**
     * Determine whether an object should be moved to a different parent during the VRML to
     * SG* translation.
     *
     * @return true if the object does not have a parent which is a logical
     *         container for the object for example if a Shape has a parent which
     *         is a Base node.
     */
    virtual bool isDangling( void ) = 0;

protected:
    WRL2NODE* m_Parent;     // pointer to parent node; may be NULL for top level node
    WRL2NODES m_Type;       // type of VRML node
    std::string m_Name;     // name to use for referencing the node by name

    std::list< WRL2NODE* > m_BackPointers;  // nodes which hold a reference to this
    std::list< WRL2NODE* > m_Children;      // nodes owned by this node
    std::list< WRL2NODE* > m_Refs;          // nodes referenced by this node
    std::string m_error;

    SGNODE* m_sgNode;  // the SGNODE representation of the display data
};

#endif  // VRML2_NODE_H
