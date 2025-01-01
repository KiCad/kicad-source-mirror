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
 * @file vrmlv1_node.h
 * Define the base class for VRML1.0 nodes.
 */


#ifndef VRML1_NODE_H
#define VRML1_NODE_H

#define GLM_FORCE_RADIANS

#include <glm/gtc/matrix_transform.hpp>

#include <list>
#include <map>
#include <string>

#include "wrlproc.h"

class WRL1NODE;

// The dictionary of node DEFs
class NAMEREGISTER
{
public:
    bool AddName( const std::string& aName, WRL1NODE* aNode );
    bool DelName( const std::string& aName, WRL1NODE* aNode );
    WRL1NODE* FindName( const std::string& aName );

private:
    std::map< std::string, WRL1NODE* > reg;
};


class WRL1BASE;
class WRL1MATERIAL;
class WRL1COORDS;
class SGNODE;


// current settings which may affect all subsequent nodes during translation / rendering
struct WRL1STATUS
{
    WRL1STATUS()
    {
        Init();
        return;
    }

    void Init()
    {
        mat = nullptr;
        matbind = WRL1_BINDING::BIND_OVERALL;
        norm = nullptr;
        normbind = WRL1_BINDING::BIND_DEFAULT;
        coord = nullptr;
        txmatrix = glm::scale( glm::mat4( 1.0 ), glm::vec3( 1.0 ) );
        order = WRL1_ORDER::ORD_UNKNOWN;
        creaseLimit = 0.878f;
        return;
    }

    // material
    WRL1MATERIAL* mat;

    // normals
    WRL1NODE* norm;

    // coordinate3
    WRL1COORDS* coord;

    // material binding
    WRL1_BINDING matbind;

    // normal binding
    WRL1_BINDING normbind;

    // transform
    glm::mat4 txmatrix;

    // winding order of vertices
    WRL1_ORDER order;

    // cos( creaseAngle ) defines a boundary for normals smoothing
    float creaseLimit;
};


/**
 * The base class of all VRML1 nodes.
 */
class WRL1NODE
{
public:
    // cancel the dictionary pointer; for internal use only
    void cancelDict( void );

    /**
     * Return the ID based on the given \a aNodeName or WRL1_INVALID (WRL1_END)
     * if no such node name exists.
     */
    WRL1NODES getNodeTypeID( const std::string& aNodeName );

    /**
     * Remove references to an owned child; it is invoked by the child upon destruction
     * to ensure that the parent has no invalid references.
     *
     * @param aNode is the child which is being deleted.
     */
    virtual void unlinkChildNode( const WRL1NODE* aNode );

    /**
     * Remove pointers to a referenced node; it is invoked by the referenced node
     * upon destruction to ensure that the referring node has no invalid references.
     *
     * @param aNode is the node which is being deleted.
     */
    virtual void unlinkRefNode( const WRL1NODE* aNode );

    /**
     * Add a pointer to a node which references, but does not own, this node.
     *
     * Such back-pointers are required to ensure that invalidated references
     * are removed when a node is deleted
     *
     * @param aNode is the node holding a reference to this object.
     */
    void addNodeRef( WRL1NODE* aNode );

    /**
     * Remove a pointer to a node which references, but does not own, this node.
     *
     * @param aNode is the node holding a reference to this object.
     */
    void delNodeRef( WRL1NODE* aNode );

    WRL1NODE( NAMEREGISTER* aDictionary );
    virtual ~WRL1NODE();

    // read data via the given file processor and WRL1BASE object
    virtual bool Read( WRLPROC& proc, WRL1BASE* aTopNode ) = 0;

    /**
     * Return the type of this node instance.
     */
    WRL1NODES GetNodeType( void ) const;

    /**
     * Return a pointer to the parent SGNODE of this object or NULL if the object has no
     * parent (ie. top level transform).
     */
    WRL1NODE* GetParent( void ) const;

    /**
     * Set the parent WRL1NODE of this object.
     *
     * @param aParent [in] is the desired parent node.
     * @param doUnlink indicates that the child must be unlinked from the parent
     * @return true if the operation succeeds; false if the given node is not allowed to
     *         be a parent to the derived object.
     */
    virtual bool SetParent( WRL1NODE* aParent, bool doUnlink = true );

    virtual std::string GetName( void );
    virtual bool SetName( const std::string& aName );

    const char* GetNodeTypeName( WRL1NODES aNodeType ) const;

    size_t GetNItems( void ) const;

    /**
     * Search the tree of linked nodes and returns a reference to the current node with the
     * given name.
     *
     * The reference is then typically added to another node via AddRefNode().
     *
     * @param aNodeName is the name of the node to search for.
     * @return is a valid node pointer on success, otherwise NULL.
     */
    virtual WRL1NODE* FindNode( const std::string& aNodeName );

    virtual bool AddChildNode( WRL1NODE* aNode );

    virtual bool AddRefNode( WRL1NODE* aNode );

    std::string GetError( void );

    /**
     * Produce a representation of the data using the intermediate scenegraph structures of the
     * kicad_3dsg library.
     *
     * @param aParent is a pointer to the parent SG node.
     * @return is non-NULL on success.
     */
    virtual SGNODE* TranslateToSG( SGNODE* aParent, WRL1STATUS* sp ) = 0;

private:
    void addItem( WRL1NODE* aNode );
    void delItem( const WRL1NODE* aNode );

protected:
    WRL1NODE* m_Parent;     // pointer to parent node; may be NULL for top level node
    WRL1NODES m_Type;       // type of VRML node
    std::string m_Name;     // name to use for referencing the node by name

    std::list< WRL1NODE* > m_BackPointers;  // nodes which hold a reference to this
    std::list< WRL1NODE* > m_Children;      // nodes owned by this node
    std::list< WRL1NODE* > m_Refs;          // nodes referenced by this node
    std::list< WRL1NODE* > m_Items;         // all nodes in order of addition
    std::string m_error;

    WRL1STATUS m_current;   // current settings
    SGNODE* m_sgNode;       // the SGNODE representation of the display data

    // note: once a node is orphaned from a base node it must never access
    // the dictionary since there is no guarantee the dictionary had not
    // been destroyed. It may be possible to enforce this rule via the
    // SetParent() routine if we implement a GetDictionary() function so
    // that a node can obtain the dictionary of its parent. Note that the
    // dictionary must be propagated to all children as well - perhaps
    // this is best done via a SetDictionary() function.
    NAMEREGISTER* m_dictionary;
};

#endif  // VRML1_NODE_H
