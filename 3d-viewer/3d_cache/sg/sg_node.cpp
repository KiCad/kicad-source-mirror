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

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>
#include <wx/log.h>

#include "3d_cache/sg/sg_node.h"
#include "plugins/3dapi/c3dmodel.h"


static const std::string node_names[S3D::SGTYPE_END + 1] = {
    "TXFM",
    "APP",
    "COL",
    "COLIDX",
    "FACE",
    "COORD",
    "COORDIDX",
    "NORM",
    "SHAPE",
    "INVALID"
};


static unsigned int node_counts[S3D::SGTYPE_END] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };


char const* S3D::GetNodeTypeName( S3D::SGTYPES aType ) noexcept
{
    return node_names[aType].c_str();
}


static void getNodeName( S3D::SGTYPES nodeType, std::string& aName )
{
    if( nodeType < 0 || nodeType >= S3D::SGTYPE_END )
    {
        aName = node_names[S3D::SGTYPE_END];
        return;
    }

    unsigned int seqNum = node_counts[nodeType];
    ++node_counts[nodeType];

    std::ostringstream ostr;
    ostr << node_names[nodeType] << "_" << seqNum;
    aName = ostr.str();
}


SGNODE::SGNODE( SGNODE* aParent )
{
    m_Parent = aParent;
    m_Association = nullptr;
    m_written = false;
    m_SGtype = S3D::SGTYPE_END;
}


SGNODE::~SGNODE()
{
    if( m_Parent )
        m_Parent->unlinkChildNode( this );

    if( m_Association )
        *m_Association = nullptr;

    std::list< SGNODE* >::iterator sBP = m_BackPointers.begin();
    std::list< SGNODE* >::iterator eBP = m_BackPointers.end();

    while( sBP != eBP )
    {
        (*sBP)->unlinkRefNode( this );
        ++sBP;
    }
}


S3D::SGTYPES SGNODE::GetNodeType( void ) const noexcept
{
    return m_SGtype;
}


SGNODE* SGNODE::GetParent( void ) const noexcept
{
    return m_Parent;
}


bool SGNODE::SwapParent( SGNODE* aNewParent )
{
    if( aNewParent == m_Parent )
        return true;

    if( nullptr == aNewParent )
        return false;

    if( nullptr == m_Parent )
    {
        if( aNewParent->AddChildNode( this ) )
            return true;

        return false;
    }

    if( aNewParent->GetNodeType() != m_Parent->GetNodeType() )
        return false;

    SGNODE* oldParent = m_Parent;
    m_Parent->unlinkChildNode( this );
    m_Parent = nullptr;
    aNewParent->unlinkRefNode( this );
    aNewParent->AddChildNode( this );
    oldParent->AddRefNode( this );

    return true;
}


const char* SGNODE::GetName( void )
{
    if( m_Name.empty() )
        getNodeName( m_SGtype, m_Name );

    return m_Name.c_str();
}


void SGNODE::SetName( const char* aName )
{
    if( nullptr == aName || 0 == aName[0] )
        getNodeName( m_SGtype, m_Name );
    else
        m_Name = aName;
}


const char* SGNODE::GetNodeTypeName( S3D::SGTYPES aNodeType ) const noexcept
{
    return node_names[aNodeType].c_str();
}


void SGNODE::addNodeRef( SGNODE* aNode )
{
    if( nullptr == aNode )
        return;

    std::list< SGNODE* >::iterator np =
        std::find( m_BackPointers.begin(), m_BackPointers.end(), aNode );

    if( np != m_BackPointers.end() )
        return;

    m_BackPointers.push_back( aNode );
}


void SGNODE::delNodeRef( const SGNODE* aNode )
{
    if( nullptr == aNode )
        return;

    std::list< SGNODE* >::iterator np =
        std::find( m_BackPointers.begin(), m_BackPointers.end(), aNode );

    if( np != m_BackPointers.end() )
    {
        m_BackPointers.erase( np );
        return;
    }

    wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] delNodeRef() did not find its target, this "
                                 "node type %d, referenced node type %d" ),
                __FILE__, __FUNCTION__, __LINE__,
                m_SGtype,
                aNode->GetNodeType() );
}


void SGNODE::AssociateWrapper( SGNODE** aWrapperRef ) noexcept
{
    wxCHECK( aWrapperRef && *aWrapperRef == this, /* void */ );

    // if there is an existing association then break it and emit a warning
    // just in case the behavior is undesired
    if( m_Association )
    {
        *m_Association = nullptr;

        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [WARNING] association being broken with "
                                     "previous wrapper" ),
                    __FILE__, __FUNCTION__, __LINE__ );
    }

    m_Association = aWrapperRef;
}

void SGNODE::DisassociateWrapper( SGNODE** aWrapperRef ) noexcept
{
    if( !m_Association )
        return;

    wxCHECK( aWrapperRef, /* void */ );

    wxCHECK( *aWrapperRef == *m_Association && aWrapperRef == m_Association, /* void */ );

    m_Association = nullptr;
}


void SGNODE::ResetNodeIndex( void ) noexcept
{
    for( int i = 0; i < (int)S3D::SGTYPE_END; ++i )
        node_counts[i] = 1;
}


bool S3D::GetMatIndex( MATLIST& aList, SGNODE* aNode, int& aIndex )
{
    aIndex = 0;

    wxCHECK( aNode && S3D::SGTYPE_APPEARANCE == aNode->GetNodeType(), false );

    SGAPPEARANCE* node = (SGAPPEARANCE*)aNode;

    std::map< SGAPPEARANCE const*, int >::iterator it = aList.matmap.find( node );

    if( it != aList.matmap.end() )
    {
        aIndex = it->second;
        return true;
    }

    int idx = (int)aList.matorder.size();
    aList.matorder.push_back( node );
    aList.matmap.emplace( node, idx );
    aIndex = idx;

    return true;
}


void S3D::INIT_SMATERIAL( SMATERIAL& aMaterial )
{
    aMaterial = {};
}


void S3D::INIT_SMESH( SMESH& aMesh ) noexcept
{
    aMesh = {};
}


void S3D::INIT_S3DMODEL( S3DMODEL& aModel ) noexcept
{
    aModel = {};
}


void S3D::FREE_SMESH( SMESH& aMesh ) noexcept
{
    if( nullptr != aMesh.m_Positions )
    {
        delete [] aMesh.m_Positions;
        aMesh.m_Positions = nullptr;
    }

    if( nullptr != aMesh.m_Normals )
    {
        delete [] aMesh.m_Normals;
        aMesh.m_Normals = nullptr;
    }

    if( nullptr != aMesh.m_Texcoords )
    {
        delete [] aMesh.m_Texcoords;
        aMesh.m_Texcoords = nullptr;
    }

    if( nullptr != aMesh.m_Color )
    {
        delete [] aMesh.m_Color;
        aMesh.m_Color = nullptr;
    }

    if( nullptr != aMesh.m_FaceIdx )
    {
        delete [] aMesh.m_FaceIdx;
        aMesh.m_FaceIdx = nullptr;
    }

    aMesh.m_VertexSize = 0;
    aMesh.m_FaceIdxSize = 0;
    aMesh.m_MaterialIdx = 0;
}


void S3D::FREE_S3DMODEL( S3DMODEL& aModel )
{
    if( nullptr != aModel.m_Materials )
    {
        delete [] aModel.m_Materials;
        aModel.m_Materials = nullptr;
    }

    aModel.m_MaterialsSize = 0;

    if( nullptr != aModel.m_Meshes )
    {
        for( unsigned int i = 0; i < aModel.m_MeshesSize; ++i )
            FREE_SMESH( aModel.m_Meshes[i] );

        delete [] aModel.m_Meshes;
        aModel.m_Meshes = nullptr;
    }

    aModel.m_MeshesSize = 0;
}
