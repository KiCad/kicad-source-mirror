/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file ifsg_api.h
 * defines the API calls for the manipulation of SG* classes
 */

#ifndef IFSG_API_H
#define IFSG_API_H

#include "plugins/3dapi/sg_types.h"
#include "plugins/3dapi/sg_base.h"
#include "plugins/3dapi/c3dmodel.h"

class SGNODE;
class SCENEGRAPH;
struct S3D_POINT;

namespace S3D
{
    /**
     * Retrieve version information of the kicad_3dsg library.
     */
    SGLIB_API void GetLibVersion( unsigned char* Major, unsigned char* Minor,
                                  unsigned char* Patch, unsigned char* Revision ) noexcept;

    // functions to extract information from SGNODE pointers
    SGLIB_API S3D::SGTYPES GetSGNodeType( SGNODE* aNode );
    SGLIB_API SGNODE* GetSGNodeParent( SGNODE* aNode );
    SGLIB_API bool AddSGNodeRef( SGNODE* aParent, SGNODE* aChild );
    SGLIB_API bool AddSGNodeChild( SGNODE* aParent, SGNODE* aChild );
    SGLIB_API void AssociateSGNodeWrapper( SGNODE* aObject, SGNODE** aRefPtr );

    /**
     * Return the normal vector of a triangle described by vertices p1, p2, p3.
     */
    SGLIB_API SGVECTOR CalcTriNorm( const SGPOINT& p1, const SGPOINT& p2, const SGPOINT& p3 );

    /**
     * Write the SGNODE tree to a binary cache file.
     *
     * @param aFileName is the name of the file to write
     * @param overwrite must be set to true to overwrite an existing file
     * @param aNode is any node within the node tree which is to be written
     * @return true on success
     */
    SGLIB_API bool WriteCache( const char* aFileName, bool overwrite, SGNODE* aNode,
        const char* aPluginInfo );

    /**
     * Read a binary cache file and creates an SGNODE tree.
     *
     * @param aFileName is the name of the binary cache file to be read
     * @return NULL on failure, on success a pointer to the top level SCENEGRAPH node;
     * if desired this node can be associated with an IFSG_TRANSFORM wrapper via
     * the IFSG_TRANSFORM::Attach() function.
     */
    SGLIB_API SGNODE* ReadCache( const char* aFileName, void* aPluginMgr,
        bool (*aTagCheck)( const char*, void* ) );

    /**
     * Write out the given node and its subnodes to a VRML2 file.
     *
     * @param filename is the name of the output file
     * @param overwrite should be set to true to overwrite an existing VRML file
     * @param aTopNode is a pointer to a SCENEGRAPH object representing the VRML scene
     * @param reuse should be set to true to make use of VRML DEF/USE features
     * @return true on success
     */
    SGLIB_API bool WriteVRML( const char* filename, bool overwrite, SGNODE* aTopNode,
                    bool reuse, bool renameNodes );

    // NOTE: The following functions are used in combination to create a VRML
    // assembly which may use various instances of each SG* representation of a module.
    // A typical use case would be:
    // 1. invoke 'ResetNodeIndex()' to reset the global node name indices
    // 2. for each model pointer provided by 'S3DCACHE->Load()', invoke 'RenameNodes()' once;
    //    this ensures that all nodes have a unique name to present to the final output file.
    //    Internally, RenameNodes() will only rename the given node and all Child subnodes;
    //    nodes which are only referenced will not be renamed. Using the pointer supplied
    //    by 'S3DCACHE->Load()' ensures that all nodes but the returned node (top node) are
    //    children of at least one node, so all nodes are given unique names.
    // 3. if SG* trees are created independently of S3DCACHE->Load() the user must invoke
    //    RenameNodes() as appropriate to ensure that all nodes have a unique name
    // 4. create an assembly structure by creating new IFSG_TRANSFORM nodes as appropriate
    //    for each instance of a component; the component base model as returned by
    //    S3DCACHE->Load() may be added to these IFSG_TRANSFORM nodes via 'AddRefNode()';
    //    set the offset, rotation, etc of the IFSG_TRANSFORM node to ensure correct
    // 5. Ensure that all new IFSG_TRANSFORM nodes are placed as child nodes within a
    //    top level IFSG_TRANSFORM node in preparation for final node naming and output
    // 6. Invoke RenameNodes() on the top level assembly node
    // 7. Invoke WriteVRML() as normal, with renameNodes = false, to write the entire assembly
    //    structure to a single VRML file
    // 8. Clean up by deleting any extra IFSG_TRANSFORM wrappers and their underlying SG*
    //    classes which have been created solely for the assembly output

    /**
     * Reset the global SG* class indices.
     *
     * @param aNode may be any valid SGNODE
     */
    SGLIB_API void ResetNodeIndex( SGNODE* aNode );

    /**
     * Rename a node and all children nodes based on the current values of the global SG*
     * class indices.
     *
     * @param aNode is a top level node
     */
    SGLIB_API void RenameNodes( SGNODE* aNode );

    /**
     * Delete the given SG* class node.
     *
     * This function makes it possible to safely delete an SG* node without associating the
     * node with its corresponding IFSG* wrapper.
     */
    SGLIB_API void DestroyNode( SGNODE* aNode ) noexcept;

    // NOTE: The following functions facilitate the creation and destruction
    // of data structures for rendering

    /**
     * Create an #S3DMODEL representation of \a aNode (raw data, no transforms).
     *
     * @param aNode is the node to be transcribed into an #S3DMODEL representation
     * @return an #S3DMODEL representation of aNode on success, otherwise NULL
     */
    SGLIB_API S3DMODEL* GetModel( SCENEGRAPH* aNode );

    /**
     * Free memory used by an S3DMODEL structure and sets the pointer to the structure to NULL.
     */
    SGLIB_API void Destroy3DModel( S3DMODEL** aModel );

    /**
     * Free memory used internally by an #S3DMODEL structure.
     */
    SGLIB_API void Free3DModel( S3DMODEL& aModel );

    /**
     * Free memory used internally by an #SMESH structure.
     */
    SGLIB_API void Free3DMesh( SMESH& aMesh );

    /**
     * Create and initialize an #S3DMODEL structure.
     */
    SGLIB_API S3DMODEL* New3DModel( void );

    /**
     * Initializes an #SMATERIAL structure.
     */
    SGLIB_API void Init3DMaterial( SMATERIAL& aMat );

    /**
     * Create and initialize an #SMESH structure.
     */
    SGLIB_API void Init3DMesh( SMESH& aMesh );
}

#endif  // IFSG_API_H
