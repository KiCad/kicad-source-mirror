/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file x3d_base.h
 * declares base class of X3D tree
 */


#ifndef X3D_BASE_H
#define X3D_BASE_H

#include <list>
#include <map>
#include <string>
#include <vector>
#include <wx/string.h>

class X3DNODE;
class SGNODE;
class wxXmlNode;

typedef std::vector< wxXmlNode* > NODE_LIST;


// a class to hold the dictionary of node DEFs
class X3D_DICT
{
public:
    bool AddName( const wxString& aName, X3DNODE* aNode );
    bool DelName( const wxString& aName, X3DNODE* aNode );
    X3DNODE* FindName( const wxString& aName );

private:
    std::map< wxString, X3DNODE* > reg;
};

enum X3DNODES
{
    X3D_TRANSFORM = 0,  // Transform or Group node
    X3D_SWITCH,
    X3D_SHAPE,
    X3D_APPEARANCE,
    X3D_INDEXED_FACE_SET,
    X3D_COORDINATE,
    X3D_INVALID,
    X3D_END = X3D_INVALID
};

/**
 * The base class of all X3D nodes.
 */
class X3DNODE
{
public:

    /**
     * Remove references to an owned child; it is invoked by the child upon destruction
     * to ensure that the parent has no invalid references.
     *
     * @param aNode is the child which is being deleted.
     */
    virtual void unlinkChildNode( const X3DNODE* aNode );

    /**
     * Remove pointers to a referenced node; it is invoked by the referenced node
     * upon destruction to ensure that the referring node has no invalid references.
     *
     * @param aNode is the node which is being deleted.
     */
    virtual void unlinkRefNode( const X3DNODE* aNode );

    /**
     * Add a pointer to a node which references, but does not own, this node.
     *
     * Such back-pointers are required to ensure that invalidated references
     * are removed when a node is deleted.
     *
     * @param aNode is the node holding a reference to this object.
     */
    void addNodeRef( X3DNODE* aNode );

    /**
     * Remove a pointer to a node which references, but does not own, this node.
     *
     * @param aNode is the node holding a reference to this object.
     */
    void delNodeRef( X3DNODE* aNode );

    X3DNODE();
    virtual ~X3DNODE();

    // read data and return TRUE on success
    virtual bool Read( wxXmlNode* aNode, X3DNODE* aTopNode, X3D_DICT& aDict ) = 0;

    /**
     * Return the type of this node instance.
     */
    X3DNODES GetNodeType( void ) const;

    /**
     * Return a pointer to the parent node of this object or NULL if the object has no
     * parent (ie. top level transform).
     */
    X3DNODE* GetParent( void ) const;

    /**
     * Return the name of this object.
     */
    wxString GetName( void ) const;

    /**
     * Set the parent X3DNODE of this object.
     *
     * @param aParent [in] is the desired parent node.
     * @param doUnlink indicates that the child must be unlinked from the parent
     * @return true if the operation succeeds or false if the given node is not allowed to
     *         be a parent to the derived object.
     */
    virtual bool SetParent( X3DNODE* aParent, bool doUnlink = true ) = 0;

    virtual bool AddChildNode( X3DNODE* aNode ) = 0;

    virtual bool AddRefNode( X3DNODE* aNode ) = 0;

    std::string GetError( void );

    /**
     * Produce a representation of the data using the intermediate scenegraph structures of
     * the kicad_3dsg library.
     *
     * @param aParent is a pointer to the parent SG node/
     * @return is non-NULL on success.
     */
    virtual SGNODE* TranslateToSG( SGNODE* aParent ) = 0;

protected:
    X3DNODE* m_Parent;  // pointer to parent node; may be NULL for top level node
    X3DNODES m_Type;    // type of node
    X3D_DICT* m_Dict;   // reference to dictionary

    std::list< X3DNODE* > m_BackPointers;   // nodes which hold a reference to this
    std::list< X3DNODE* > m_Children;       // nodes owned by this node
    std::list< X3DNODE* > m_Refs;           // nodes referenced by this node
    std::string m_error;

    wxString m_Name;    // name to use for referencing the node by name
    SGNODE* m_sgNode;   // the SGNODE representation of the display data
};

#endif  // X3D_BASE_H
