/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2017 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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


#include <iostream>
#include <sstream>
#include <wx/log.h>

#include "3d_cache/sg/sg_faceset.h"
#include "3d_cache/sg/sg_colors.h"
#include "3d_cache/sg/sg_coords.h"
#include "3d_cache/sg/sg_normals.h"
#include "3d_cache/sg/sg_coordindex.h"
#include "3d_cache/sg/sg_helpers.h"


SGFACESET::SGFACESET( SGNODE* aParent ) : SGNODE( aParent )
{
    m_SGtype = S3D::SGTYPE_FACESET;
    m_Colors = nullptr;
    m_Coords = nullptr;
    m_CoordIndices = nullptr;
    m_Normals = nullptr;
    m_RColors = nullptr;
    m_RCoords = nullptr;
    m_RNormals = nullptr;
    valid = false;
    validated = false;

    if( nullptr != aParent && S3D::SGTYPE_SHAPE != aParent->GetNodeType() )
    {
        m_Parent = nullptr;

        wxLogTrace( MASK_3D_SG,
                    wxT( "%s:%s:%d * [BUG] inappropriate parent to SGFACESET (type %s)" ),
                    __FILE__, __FUNCTION__, __LINE__,
                    aParent->GetNodeType() );
    }
    else if( nullptr != aParent && S3D::SGTYPE_SHAPE == aParent->GetNodeType() )
    {
        m_Parent->AddChildNode( this );
    }
}


SGFACESET::~SGFACESET()
{
    // drop references
    if( m_RColors )
    {
        m_RColors->delNodeRef( this );
        m_RColors = nullptr;
    }

    if( m_RCoords )
    {
        m_RCoords->delNodeRef( this );
        m_RCoords = nullptr;
    }

    if( m_RNormals )
    {
        m_RNormals->delNodeRef( this );
        m_RNormals = nullptr;
    }

    // delete owned objects
    if( m_Colors )
    {
        m_Colors->SetParent( nullptr, false );
        delete m_Colors;
        m_Colors = nullptr;
    }

    if( m_Coords )
    {
        m_Coords->SetParent( nullptr, false );
        delete m_Coords;
        m_Coords = nullptr;
    }

    if( m_Normals )
    {
        m_Normals->SetParent( nullptr, false );
        delete m_Normals;
        m_Normals = nullptr;
    }

    if( m_CoordIndices )
    {
        m_CoordIndices->SetParent( nullptr, false );
        delete m_CoordIndices;
        m_CoordIndices = nullptr;
    }
}


bool SGFACESET::SetParent( SGNODE* aParent, bool notify )
{
    if( nullptr != m_Parent )
    {
        if( aParent == m_Parent )
            return true;

        // handle the change in parents
        if( notify )
            m_Parent->unlinkChildNode( this );

        m_Parent = nullptr;

        if( nullptr == aParent )
            return true;
    }

    // only a SGSHAPE may be parent to a SGFACESET
    if( nullptr != aParent && S3D::SGTYPE_SHAPE != aParent->GetNodeType() )
        return false;

    m_Parent = aParent;

    if( m_Parent )
        m_Parent->AddChildNode( this );

    return true;
}


SGNODE* SGFACESET::FindNode(const char *aNodeName, const SGNODE *aCaller)
{
    if( nullptr == aNodeName || 0 == aNodeName[0] )
        return nullptr;

    if( !m_Name.compare( aNodeName ) )
        return this;

    SGNODE* np = nullptr;

    if( m_Colors )
    {
        np = m_Colors->FindNode( aNodeName, this );

        if( np )
            return np;
    }

    if( m_Coords )
    {
        np = m_Coords->FindNode( aNodeName, this );

        if( np )
            return np;
    }

    if( m_CoordIndices )
    {
        np = m_CoordIndices->FindNode( aNodeName, this );

        if( np )
            return np;
    }

    if( m_Normals )
    {
        np = m_Normals->FindNode( aNodeName, this );

        if( np )
            return np;
    }

    // query the parent if appropriate
    if( aCaller == m_Parent || nullptr == m_Parent )
        return nullptr;

    return m_Parent->FindNode( aNodeName, this );
}


void SGFACESET::unlinkNode( const SGNODE* aNode, bool isChild )
{
    if( nullptr == aNode )
        return;

    valid = false;
    validated = false;

    if( isChild )
    {
        if( aNode == m_Colors )
        {
            m_Colors = nullptr;
            return;
        }

        if( aNode == m_Coords )
        {
            m_Coords = nullptr;
            return;
        }

        if( aNode == m_Normals )
        {
            m_Normals = nullptr;
            return;
        }

        if( aNode == m_CoordIndices )
        {
            m_CoordIndices = nullptr;
            return;
        }
    }
    else
    {
        if( aNode == m_RColors )
        {
            delNodeRef( this );
            m_RColors = nullptr;
            return;
        }

        if( aNode == m_RCoords )
        {
            delNodeRef( this );
            m_RCoords = nullptr;
            return;
        }

        if( aNode == m_RNormals )
        {
            delNodeRef( this );
            m_RNormals = nullptr;
            return;
        }
    }

    wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] unlinkNode() did not find its target" ),
                __FILE__, __FUNCTION__, __LINE__ );
}


void SGFACESET::unlinkChildNode( const SGNODE* aNode )
{
    unlinkNode( aNode, true );
}


void SGFACESET::unlinkRefNode( const SGNODE* aNode )
{
    unlinkNode( aNode, false );
}



bool SGFACESET::addNode( SGNODE* aNode, bool isChild )
{
    wxCHECK( aNode, false );

    valid = false;
    validated = false;

    if( S3D::SGTYPE_COLORS == aNode->GetNodeType() )
    {
        if( m_Colors || m_RColors )
        {
            if( aNode != m_Colors && aNode != m_RColors )
            {
                wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] assigning multiple Colors nodes" ),
                            __FILE__, __FUNCTION__, __LINE__ );

                return false;
            }

            return true;
        }

        if( isChild )
        {
            m_Colors = (SGCOLORS*)aNode;
            m_Colors->SetParent( this );
        }
        else
        {
            m_RColors = (SGCOLORS*)aNode;
            m_RColors->addNodeRef( this );
        }

        return true;
    }

    if( S3D::SGTYPE_COORDS == aNode->GetNodeType() )
    {
        if( m_Coords || m_RCoords )
        {
            if( aNode != m_Coords && aNode != m_RCoords )
            {
                wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] assigning multiple Colors nodes" ),
                            __FILE__, __FUNCTION__, __LINE__ );

                return false;
            }

            return true;
        }

        if( isChild )
        {
            m_Coords = (SGCOORDS*)aNode;
            m_Coords->SetParent( this );
        }
        else
        {
            m_RCoords = (SGCOORDS*)aNode;
            m_RCoords->addNodeRef( this );
        }

        return true;
    }

    if( S3D::SGTYPE_NORMALS == aNode->GetNodeType() )
    {
        if( m_Normals || m_RNormals )
        {
            if( aNode != m_Normals && aNode != m_RNormals )
            {
                wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] assigning multiple Normals nodes" ),
                            __FILE__, __FUNCTION__, __LINE__ );

                return false;
            }

            return true;
        }

        if( isChild )
        {
            m_Normals = (SGNORMALS*)aNode;
            m_Normals->SetParent( this );
        }
        else
        {
            m_RNormals = (SGNORMALS*)aNode;
            m_RNormals->addNodeRef( this );
        }

        return true;
    }

    if( S3D::SGTYPE_COORDINDEX == aNode->GetNodeType() )
    {
        if( m_CoordIndices )
        {
            if( aNode != m_CoordIndices )
            {
                wxLogTrace( MASK_3D_SG,
                            wxT( "%s:%s:%d * [BUG] assigning multiple CoordIndex nodes" ),
                            __FILE__, __FUNCTION__, __LINE__ );

                return false;
            }

            return true;
        }

        m_CoordIndices = (SGCOORDINDEX*)aNode;
        m_CoordIndices->SetParent( this );

        return true;
    }

    wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] object type '%s' is not a valid type for "
                                 "this object '%d'" ),
                __FILE__, __FUNCTION__, __LINE__,
                aNode->GetName(),
                aNode->GetNodeType() );

    return false;
}


bool SGFACESET::AddRefNode( SGNODE* aNode )
{
    return addNode( aNode, false );
}


bool SGFACESET::AddChildNode( SGNODE* aNode )
{
    return addNode( aNode, true );
}


void SGFACESET::ReNameNodes( void )
{
    m_written = false;

    // rename this node
    m_Name.clear();
    GetName();

    // rename all Colors and Indices
    if( m_Colors )
        m_Colors->ReNameNodes();

    // rename all Coordinates and Indices
    if( m_Coords )
        m_Coords->ReNameNodes();

    if( m_CoordIndices )
        m_CoordIndices->ReNameNodes();

    // rename all Normals and Indices
    if( m_Normals )
        m_Normals->ReNameNodes();
}


bool SGFACESET::WriteVRML( std::ostream& aFile, bool aReuseFlag )
{
    if( ( nullptr == m_Coords && nullptr == m_RCoords ) || ( nullptr == m_CoordIndices ) )
    {
        return false;
    }

    if( aReuseFlag )
    {
        if( !m_written )
        {
            aFile << " geometry DEF " << GetName() << " IndexedFaceSet {\n";
            m_written = true;
        }
        else
        {
            aFile << "USE " << GetName() << "\n";
            return true;
        }
    }
    else
    {
        aFile << " geometry IndexedFaceSet {\n";
    }

    if( m_Coords )
        m_Coords->WriteVRML( aFile, aReuseFlag );

    if( m_RCoords )
        m_RCoords->WriteVRML( aFile, aReuseFlag );

    if( m_CoordIndices )
        m_CoordIndices->WriteVRML( aFile, aReuseFlag );

    if( m_Normals || m_RNormals )
        aFile << "  normalPerVertex TRUE\n";

    if( m_Normals )
        m_Normals->WriteVRML( aFile, aReuseFlag );

    if( m_RNormals )
        m_RNormals->WriteVRML( aFile, aReuseFlag );

    if( m_Colors )
        m_Colors->WriteVRML( aFile, aReuseFlag );

    if( m_RColors )
        m_RColors->WriteVRML( aFile, aReuseFlag );

    aFile << "}\n";

    return true;
}


bool SGFACESET::WriteCache( std::ostream& aFile, SGNODE* parentNode )
{
    if( nullptr == parentNode )
    {
        wxCHECK( m_Parent, false );

        SGNODE* np = m_Parent;

        while( nullptr != np->GetParent() )
            np = np->GetParent();

        if( np->WriteCache( aFile, nullptr ) )
        {
            m_written = true;
            return true;
        }

        return false;
    }

    wxCHECK( parentNode == m_Parent, false );

    if( !aFile.good() )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] bad stream" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    // check if any references are unwritten and swap parents if so
    if( nullptr != m_RCoords && !m_RCoords->isWritten() )
        m_RCoords->SwapParent( this );

    if( nullptr != m_RNormals && !m_RNormals->isWritten() )
        m_RNormals->SwapParent( this );

    if( nullptr != m_RColors && !m_RColors->isWritten() )
        m_RColors->SwapParent( this );

    aFile << "[" << GetName() << "]";
    #define NITEMS 7
    bool items[NITEMS];
    int i;

    for( i = 0; i < NITEMS; ++i )
        items[i] = 0;

    i = 0;
    if( nullptr != m_Coords )
        items[i] = true;

    ++i;
    if( nullptr != m_RCoords )
        items[i] = true;

    ++i;
    if( nullptr != m_CoordIndices )
        items[i] = true;

    ++i;
    if( nullptr != m_Normals )
        items[i] = true;

    ++i;
    if( nullptr != m_RNormals )
        items[i] = true;

    ++i;
    if( nullptr != m_Colors )
        items[i] = true;

    ++i;
    if( nullptr != m_RColors )
        items[i] = true;

    for( int jj = 0; jj < NITEMS; ++jj )
        aFile.write( (char*) &items[jj], sizeof( bool ) );

    if( items[0] )
        m_Coords->WriteCache( aFile, this );

    if( items[1] )
        aFile << "[" << m_RCoords->GetName() << "]";

    if( items[2] )
        m_CoordIndices->WriteCache( aFile, this );

    if( items[3] )
        m_Normals->WriteCache( aFile, this );

    if( items[4] )
        aFile << "[" << m_RNormals->GetName() << "]";

    if( items[5] )
        m_Colors->WriteCache( aFile, this );

    if( items[6] )
        aFile << "[" << m_RColors->GetName() << "]";

    if( aFile.fail() )
        return false;

    m_written = true;
    return true;
}


bool SGFACESET::ReadCache( std::istream& aFile, SGNODE* parentNode )
{
    if( m_Coords || m_RCoords || m_CoordIndices || m_Colors || m_RColors || m_Normals
      || m_RNormals )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] non-empty node" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    #define NITEMS 7
    bool items[NITEMS];

    for( int i = 0; i < NITEMS; ++i )
        aFile.read( (char*) &items[i], sizeof( bool ) );

    if( ( items[0] && items[1] ) || ( items[3] && items[4] ) || ( items[5] && items[6] ) )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; multiple item definitions "
                                     "at position %d" ),
                    __FILE__, __FUNCTION__, __LINE__,
                    static_cast<int>( aFile.tellg() ) );

        return false;
    }

    std::string name;

    if( items[0] )
    {
        if( S3D::SGTYPE_COORDS != S3D::ReadTag( aFile, name ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; bad child coords tag at "
                                         "position %d" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<int>( aFile.tellg() ) );

            return false;
        }

        m_Coords = new SGCOORDS( this );
        m_Coords->SetName( name.c_str() );

        if( !m_Coords->ReadCache( aFile, this ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; corrupt data while "
                                         "reading coords '%s'" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        name );

            return false;
        }
    }

    if( items[1] )
    {
        if( S3D::SGTYPE_COORDS != S3D::ReadTag( aFile, name ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; bad ref coords tag at "
                                         "position %d" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<int>( aFile.tellg() ) );

            return false;
        }

        SGNODE* np = FindNode( name.c_str(), this );

        if( !np )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; cannot find ref "
                                         "coords '%s'" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        name );

            return false;
        }

        if( S3D::SGTYPE_COORDS != np->GetNodeType() )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; type is not SGCOORDS "
                                         "'%s'" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        name );

            return false;
        }

        m_RCoords = (SGCOORDS*)np;
        m_RCoords->addNodeRef( this );
    }

    if( items[2] )
    {
        if( S3D::SGTYPE_COORDINDEX != S3D::ReadTag( aFile, name ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; bad coord index tag at "
                                         "position %d" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<int>( aFile.tellg() ) );

            return false;
        }

        m_CoordIndices = new SGCOORDINDEX( this );
        m_CoordIndices->SetName( name.c_str() );

        if( !m_CoordIndices->ReadCache( aFile, this ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data while reading coord "
                                         "index '%s'" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        name );

            return false;
        }
    }

    if( items[3] )
    {
        if( S3D::SGTYPE_NORMALS != S3D::ReadTag( aFile, name ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; bad child normals tag "
                                         "at position %d" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<int>( aFile.tellg() ) );

            return false;
        }

        m_Normals = new SGNORMALS( this );
        m_Normals->SetName( name.c_str() );

        if( !m_Normals->ReadCache( aFile, this ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data while reading normals "
                                         "'%s'" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        name );

            return false;
        }
    }

    if( items[4] )
    {
        if( S3D::SGTYPE_NORMALS != S3D::ReadTag( aFile, name ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; bad ref normals tag at "
                                         "position %d" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<int>( aFile.tellg() ) );

            return false;
        }

        SGNODE* np = FindNode( name.c_str(), this );

        if( !np )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt: cannot find ref normals "
                                         "'%s'" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        name );

            return false;
        }

        if( S3D::SGTYPE_NORMALS != np->GetNodeType() )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt: type is not SGNORMALS '%s'" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        name );

            return false;
        }

        m_RNormals = (SGNORMALS*)np;
        m_RNormals->addNodeRef( this );
    }

    if( items[5] )
    {
        if( S3D::SGTYPE_COLORS != S3D::ReadTag( aFile, name ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; bad child colors tag "
                                         "at position %d" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<int>( aFile.tellg() ) );

            return false;
        }

        m_Colors = new SGCOLORS( this );
        m_Colors->SetName( name.c_str() );

        if( !m_Colors->ReadCache( aFile, this ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data while reading colors "
                                         "'%s'" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        name );

            return false;
        }
    }

    if( items[6] )
    {
        if( S3D::SGTYPE_COLORS != S3D::ReadTag( aFile, name ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; bad ref colors tag at "
                                         "position %d" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<int>( aFile.tellg() ) );

            return false;
        }

        SGNODE* np = FindNode( name.c_str(), this );

        if( !np )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data: cannot find ref colors "
                                         "'%s'" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        name );

            return false;
        }

        if( S3D::SGTYPE_COLORS != np->GetNodeType() )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data: type is not SGCOLORS "
                                         "'%s'" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        name );

            return false;
        }

        m_RColors = (SGCOLORS*)np;
        m_RColors->addNodeRef( this );
    }

    if( aFile.fail() )
        return false;

    return true;
}


bool SGFACESET::validate( void )
{
    // verify the integrity of this object's data
    if( validated )
        return valid;

    // ensure we have at least coordinates and their normals
    if( ( nullptr == m_Coords && nullptr == m_RCoords )
      || ( nullptr == m_Normals && nullptr == m_RNormals )
      || ( nullptr == m_CoordIndices ) )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] bad model; no vertices, vertex indices, "
                                     "or normals" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        validated = true;
        valid = false;
        return false;
    }

    // check that there are >3 vertices
    SGCOORDS* coords = m_Coords;

    if( nullptr == coords )
        coords = m_RCoords;

    size_t nCoords = 0;
    SGPOINT* lCoords = nullptr;
    coords->GetCoordsList( nCoords, lCoords );

    if( nCoords < 3 )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] bad model; fewer than 3 vertices" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        validated = true;
        valid = false;
        return false;
    }

    // check that nVertices is divisible by 3 (facets are triangles)
    size_t nCIdx = 0;
    int* lCIdx = nullptr;
    m_CoordIndices->GetIndices( nCIdx, lCIdx );

    if( nCIdx < 3 || ( nCIdx % 3 > 0 ) )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] bad model; no vertex indices or not "
                                     "multiple of 3" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        validated = true;
        valid = false;
        return false;
    }

    // check that vertex[n] >= 0 and < nVertices
    for( size_t i = 0; i < nCIdx; ++i )
    {
        if( lCIdx[i] < 0 || lCIdx[i] >= (int)nCoords )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] bad model; vertex index out of "
                                         "bounds" ),
                        __FILE__, __FUNCTION__, __LINE__ );

            validated = true;
            valid = false;
            return false;
        }
    }

    // check that there are as many normals as vertices
    size_t nNorms = 0;
    SGVECTOR* lNorms = nullptr;
    SGNORMALS* pNorms = m_Normals;

    if( nullptr == pNorms )
        pNorms = m_RNormals;

    pNorms->GetNormalList( nNorms, lNorms );

    if( nNorms != nCoords )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] bad model; number of normals (%ul) does "
                                     "not match number of vertices (%ul)" ),
                    __FILE__, __FUNCTION__, __LINE__,
                    static_cast<unsigned long>( nNorms ),
                    static_cast<unsigned long>( nCoords ) );

        validated = true;
        valid = false;
        return false;
    }

    // if there are colors then ensure there are as many colors as vertices
    SGCOLORS* pColors = m_Colors;

    if( nullptr == pColors )
        pColors = m_RColors;

    if( nullptr != pColors )
    {
        // we must have at least as many colors as vertices
        size_t nColor = 0;
        SGCOLOR* pColor = nullptr;
        pColors->GetColorList( nColor, pColor );
    }

    validated = true;
    valid = true;
    return true;
}


void SGFACESET::GatherCoordIndices( std::vector< int >& aIndexList )
{
    if( m_CoordIndices )
        m_CoordIndices->GatherCoordIndices( aIndexList );
}


bool SGFACESET::CalcNormals( SGNODE** aPtr )
{
    SGCOORDS* coords = m_Coords;

    if( m_RCoords )
        coords = m_RCoords;

    if( nullptr == coords || coords->coords.empty() )
        return false;

    if( m_Normals && !m_Normals->norms.empty( ) )
        return true;

    if( m_RNormals && !m_RNormals->norms.empty( ) )
        return true;

    return coords->CalcNormals( this, aPtr );
}
