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


#include <iostream>
#include "3d_cache/sg/sg_faceset.h"
#include "3d_cache/sg/sg_colors.h"
#include "3d_cache/sg/sg_coords.h"
#include "3d_cache/sg/sg_normals.h"
#include "3d_cache/sg/sg_colorindex.h"
#include "3d_cache/sg/sg_coordindex.h"
#include "3d_cache/sg/sg_helpers.h"

SGFACESET::SGFACESET( SGNODE* aParent ) : SGNODE( aParent )
{
    m_SGtype = S3D::SGTYPE_FACESET;
    m_Colors = NULL;
    m_ColorIndices = NULL;
    m_Coords = NULL;
    m_CoordIndices = NULL;
    m_Normals = NULL;
    m_RColors = NULL;
    m_RCoords = NULL;
    m_RNormals = NULL;
    valid = false;
    validated = false;

    if( NULL != aParent && S3D::SGTYPE_SHAPE != aParent->GetNodeType() )
    {
        m_Parent = NULL;

#ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] inappropriate parent to SGFACESET (type ";
        std::cerr << aParent->GetNodeType() << ")\n";
#endif
    }
    else if( NULL != aParent && S3D::SGTYPE_SHAPE == aParent->GetNodeType() )
    {
        m_Parent->AddChildNode( this );
    }

    return;
}


SGFACESET::~SGFACESET()
{
    // drop references
    if( m_RColors )
    {
        m_RColors->delNodeRef( this );
        m_RColors = NULL;
    }

    if( m_RCoords )
    {
        m_RCoords->delNodeRef( this );
        m_RCoords = NULL;
    }

    if( m_RNormals )
    {
        m_RNormals->delNodeRef( this );
        m_RNormals = NULL;
    }

    // delete owned objects
    if( m_Colors )
    {
        m_Colors->SetParent( NULL, false );
        delete m_Colors;
        m_Colors = NULL;
    }

    if( m_Coords )
    {
        m_Coords->SetParent( NULL, false );
        delete m_Coords;
        m_Coords = NULL;
    }

    if( m_Normals )
    {
        m_Normals->SetParent( NULL, false );
        delete m_Normals;
        m_Normals = NULL;
    }

    if( m_ColorIndices )
    {
        m_ColorIndices->SetParent( NULL, false );
        delete m_ColorIndices;
        m_ColorIndices = NULL;
    }

    if( m_CoordIndices )
    {
        m_CoordIndices->SetParent( NULL, false );
        delete m_CoordIndices;
        m_CoordIndices = NULL;
    }

    return;
}


bool SGFACESET::SetParent( SGNODE* aParent, bool notify )
{
    if( NULL != m_Parent )
    {
        if( aParent == m_Parent )
            return true;

        // handle the change in parents
        if( notify )
            m_Parent->unlinkChildNode( this );

        m_Parent = NULL;

        if( NULL == aParent )
            return true;
    }

    // only a SGSHAPE may be parent to a SGFACESET
    if( NULL != aParent && S3D::SGTYPE_SHAPE != aParent->GetNodeType() )
        return false;

    m_Parent = aParent;

    if( m_Parent )
        m_Parent->AddChildNode( this );

    return true;
}


SGNODE* SGFACESET::FindNode(const char *aNodeName, const SGNODE *aCaller)
{
    if( NULL == aNodeName || 0 == aNodeName[0] )
        return NULL;

    if( !m_Name.compare( aNodeName ) )
        return this;

    SGNODE* np = NULL;

    if( m_Colors )
    {
        np = m_Colors->FindNode( aNodeName, this );

        if( np )
            return np;
    }

    if( m_ColorIndices )
    {
        np = m_ColorIndices->FindNode( aNodeName, this );

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
    if( aCaller == m_Parent || NULL == m_Parent )
        return NULL;

    return m_Parent->FindNode( aNodeName, this );
}


void SGFACESET::unlinkNode( const SGNODE* aNode, bool isChild )
{
    if( NULL == aNode )
        return;

    valid = false;
    validated = false;

    if( isChild )
    {
        if( aNode == m_Colors )
        {
            m_Colors = NULL;
            return;
        }

        if( aNode == m_Coords )
        {
            m_Coords = NULL;
            return;
        }

        if( aNode == m_Normals )
        {
            m_Normals = NULL;
            return;
        }

        if( aNode == m_ColorIndices )
        {
            m_ColorIndices = NULL;
            return;
        }

        if( aNode == m_CoordIndices )
        {
            m_CoordIndices = NULL;
            return;
        }
    }
    else
    {
        if( aNode == m_RColors )
        {
            m_RColors = NULL;
            return;
        }

        if( aNode == m_RCoords )
        {
            m_RCoords = NULL;
            return;
        }

        if( aNode == m_RNormals )
        {
            m_RNormals = NULL;
            return;
        }
    }

    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] unlinkNode() did not find its target\n";

    return;
}


void SGFACESET::unlinkChildNode( const SGNODE* aNode )
{
    unlinkNode( aNode, true );
    return;
}


void SGFACESET::unlinkRefNode( const SGNODE* aNode )
{
    unlinkNode( aNode, false );
    return;
}



bool SGFACESET::addNode( SGNODE* aNode, bool isChild )
{
    if( NULL == aNode )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] NULL pointer passed for aNode\n";
        return false;
    }

    valid = false;
    validated = false;

    if( S3D::SGTYPE_COLORS == aNode->GetNodeType() )
    {
        if( m_Colors || m_RColors )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [BUG] assigning multiple Colors nodes\n";
            return false;
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
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [BUG] assigning multiple Coords nodes\n";
            return false;
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
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [BUG] assigning multiple Normals nodes\n";
            return false;
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

    if( S3D::SGTYPE_COLORINDEX == aNode->GetNodeType() )
    {
        if( m_ColorIndices )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [BUG] assigning multiple ColorIndex nodes\n";
            return false;
        }

        m_ColorIndices = (SGCOLORINDEX*)aNode;
        m_ColorIndices->SetParent( this );

        return true;
    }

    if( S3D::SGTYPE_COORDINDEX == aNode->GetNodeType() )
    {
        if( m_CoordIndices )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [BUG] assigning multiple CoordIndex nodes\n";
            return false;
        }

        m_CoordIndices = (SGCOORDINDEX*)aNode;
        m_CoordIndices->SetParent( this );

        return true;
    }

    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] object '" << aNode->GetName();
    std::cerr << "' (type " << aNode->GetNodeType();
    std::cerr << ") is not a valid type for this object (" << aNode->GetNodeType() << ")\n";

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

    if( m_ColorIndices )
        m_ColorIndices->ReNameNodes();

    // rename all Coordinates and Indices
    if( m_Coords )
        m_Coords->ReNameNodes();

    if( m_CoordIndices )
        m_CoordIndices->ReNameNodes();

    // rename all Normals and Indices
    if( m_Normals )
        m_Normals->ReNameNodes();

    return;
}


bool SGFACESET::WriteVRML( std::ofstream& aFile, bool aReuseFlag )
{
    if( ( NULL == m_Coords && NULL == m_RCoords )
        || ( NULL == m_CoordIndices ) )
    {
        return false;
    }

    std::string tmp;

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

    if( m_ColorIndices )
        m_ColorIndices->WriteVRML( aFile, aReuseFlag );

    aFile << "}\n";

    return true;
}


bool SGFACESET::WriteCache( std::ofstream& aFile, SGNODE* parentNode )
{
    if( NULL == parentNode )
    {
        if( NULL == m_Parent )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [BUG] corrupt data; m_aParent is NULL\n";
            return false;
        }

        SGNODE* np = m_Parent;

        while( NULL != np->GetParent() )
            np = np->GetParent();

        return np->WriteCache( aFile, NULL );
    }

    if( parentNode != m_Parent )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] corrupt data; parentNode != m_aParent\n";
        return false;
    }

    if( !aFile.good() )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad stream\n";
        return false;
    }

    aFile << "[" << GetName() << "]";
    #define NITEMS 8
    bool items[NITEMS];
    int i;

    for( i = 0; i < NITEMS; ++i )
        items[i] = 0;

    i = 0;
    if( NULL != m_Coords )
        items[i] = true;

    ++i;
    if( NULL != m_RCoords )
        items[i] = true;

    ++i;
    if( NULL != m_CoordIndices )
        items[i] = true;

    ++i;
    if( NULL != m_Normals )
        items[i] = true;

    ++i;
    if( NULL != m_RNormals )
        items[i] = true;

    ++i;
    if( NULL != m_Colors )
        items[i] = true;

    ++i;
    if( NULL != m_RColors )
        items[i] = true;

    ++i;
    if( NULL != m_ColorIndices )
        items[i] = true;

    for( int i = 0; i < NITEMS; ++i )
        aFile.write( (char*)&items[i], sizeof(bool) );

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

    if( items[7] )
        m_ColorIndices->WriteCache( aFile, this );

    if( aFile.fail() )
        return false;

    return true;
}


bool SGFACESET::ReadCache( std::ifstream& aFile, SGNODE* parentNode )
{
    if( m_Coords || m_RCoords || m_CoordIndices
        || m_Colors || m_RColors || m_ColorIndices
        || m_Normals || m_RNormals )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] non-empty node\n";
        return false;
    }

    #define NITEMS 8
    bool items[NITEMS];

    for( int i = 0; i < NITEMS; ++i )
        aFile.read( (char*)&items[i], sizeof(bool) );

    if( ( items[0] && items[1] ) || ( items[3] && items[4] )
        || ( items[5] && items[6] ) )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] corrupt data; multiple item definitions at position ";
        std::cerr << aFile.tellg() << "\n";
        return false;
    }

    std::string name;

    if( items[0] )
    {
        if( S3D::SGTYPE_COORDS != S3D::ReadTag( aFile, name ) )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data; bad child coords tag at position ";
            std::cerr << aFile.tellg() << "\n";
            return false;
        }

        m_Coords = new SGCOORDS( this );
        m_Coords->SetName( name.c_str() );

        if( !m_Coords->ReadCache( aFile, this ) )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data while reading coords '";
            std::cerr << name << "'\n";
            return false;
        }
    }

    if( items[1] )
    {
        if( S3D::SGTYPE_COORDS != S3D::ReadTag( aFile, name ) )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data; bad ref coords tag at position ";
            std::cerr << aFile.tellg() << "\n";
            return false;
        }

        SGNODE* np = FindNode( name.c_str(), this );

        if( !np )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data: cannot find ref coords '";
            std::cerr << name << "'\n";
            return false;
        }

        if( S3D::SGTYPE_COORDS != np->GetNodeType() )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data: type is not SGCOORDS '";
            std::cerr << name << "'\n";
            return false;
        }

        m_RCoords = (SGCOORDS*)np;
        m_RCoords->addNodeRef( this );
    }

    if( items[2] )
    {
        if( S3D::SGTYPE_COORDINDEX != S3D::ReadTag( aFile, name ) )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data; bad coord index tag at position ";
            std::cerr << aFile.tellg() << "\n";
            return false;
        }

        m_CoordIndices = new SGCOORDINDEX( this );
        m_CoordIndices->SetName( name.c_str() );

        if( !m_CoordIndices->ReadCache( aFile, this ) )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data while reading coord index '";
            std::cerr << name << "'\n";
            return false;
        }
    }

    if( items[3] )
    {
        if( S3D::SGTYPE_NORMALS != S3D::ReadTag( aFile, name ) )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data; bad child normals tag at position ";
            std::cerr << aFile.tellg() << "\n";
            return false;
        }

        m_Normals = new SGNORMALS( this );
        m_Normals->SetName( name.c_str() );

        if( !m_Normals->ReadCache( aFile, this ) )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data while reading normals '";
            std::cerr << name << "'\n";
            return false;
        }
    }

    if( items[4] )
    {
        if( S3D::SGTYPE_NORMALS != S3D::ReadTag( aFile, name ) )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data; bad ref normals tag at position ";
            std::cerr << aFile.tellg() << "\n";
            return false;
        }

        SGNODE* np = FindNode( name.c_str(), this );

        if( !np )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data: cannot find ref normals '";
            std::cerr << name << "'\n";
            return false;
        }

        if( S3D::SGTYPE_NORMALS != np->GetNodeType() )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data: type is not SGNORMALS '";
            std::cerr << name << "'\n";
            return false;
        }

        m_RNormals = (SGNORMALS*)np;
        m_RNormals->addNodeRef( this );
    }

    if( items[5] )
    {
        if( S3D::SGTYPE_COLORS != S3D::ReadTag( aFile, name ) )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data; bad child colors tag at position ";
            std::cerr << aFile.tellg() << "\n";
            return false;
        }

        m_Colors = new SGCOLORS( this );
        m_Colors->SetName( name.c_str() );

        if( !m_Colors->ReadCache( aFile, this ) )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data while reading colors '";
            std::cerr << name << "'\n";
            return false;
        }
    }

    if( items[6] )
    {
        if( S3D::SGTYPE_COLORS != S3D::ReadTag( aFile, name ) )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data; bad ref colors tag at position ";
            std::cerr << aFile.tellg() << "\n";
            return false;
        }

        SGNODE* np = FindNode( name.c_str(), this );

        if( !np )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data: cannot find ref colors '";
            std::cerr << name << "'\n";
            return false;
        }

        if( S3D::SGTYPE_COLORS != np->GetNodeType() )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data: type is not SGCOLORS '";
            std::cerr << name << "'\n";
            return false;
        }

        m_RColors = (SGCOLORS*)np;
        m_RColors->addNodeRef( this );
    }

    if( items[7] )
    {
        if( S3D::SGTYPE_COLORINDEX != S3D::ReadTag( aFile, name ) )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data; bad color index tag at position ";
            std::cerr << aFile.tellg() << "\n";
            return false;
        }

        m_ColorIndices = new SGCOLORINDEX( this );
        m_ColorIndices->SetName( name.c_str() );

        if( !m_ColorIndices->ReadCache( aFile, this ) )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] corrupt data while reading color index '";
            std::cerr << name << "'\n";
            return false;
        }
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
    if( (NULL == m_Coords && NULL == m_RCoords)
        || (NULL == m_Normals && NULL == m_RNormals)
        || (NULL == m_CoordIndices) )
    {
#ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad model; no vertices, vertex indices, or normals\n";
#endif
        validated = true;
        valid = false;
        return false;
    }

    // check that there are >3 vertices
    SGCOORDS* coords = m_Coords;

    if( NULL == coords )
        coords = m_RCoords;

    size_t nCoords = 0;
    SGPOINT* lCoords = NULL;
    coords->GetCoordsList( nCoords, lCoords );

    if( nCoords < 3 )
    {
#ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad model; fewer than 3 vertices\n";
#endif
        validated = true;
        valid = false;
        return false;
    }

    // check that nVertices is divisible by 3 (facets are triangles)
    size_t nCIdx = 0;
    int* lCIdx = NULL;
    m_CoordIndices->GetIndices( nCIdx, lCIdx );

    if( nCIdx < 3 || ( nCIdx % 3 > 0 ) )
    {
#ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad model; no vertex indices or not multiple of 3\n";
#endif
        validated = true;
        valid = false;
        return false;
    }

    // check that vertex[n] >= 0 and < nVertices
    for( size_t i = 0; i < nCIdx; ++i )
    {
        if( lCIdx[i] < 0 || lCIdx[i] >= nCoords )
        {
#ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad model; vertex index out of bounds\n";
#endif
            validated = true;
            valid = false;
            return false;
        }
    }

    // check that there are as many normals as vertices
    size_t nNorms = 0;
    SGVECTOR* lNorms = NULL;
    SGNORMALS* pNorms = m_Normals;

    if( NULL == pNorms )
        pNorms = m_RNormals;

    pNorms->GetNormalList( nNorms, lNorms );

    if( nNorms != nCoords )
    {
#ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad model; number of normals does not match number of vertices\n";
#endif
        validated = true;
        valid = false;
        return false;
    }

    // if there are colors then ensure there are as many colors as vertices
    SGCOLORS* pColors = m_Colors;

    if( NULL == pColors )
        pColors = m_RColors;

    if( NULL != pColors )
    {
        if( NULL == m_ColorIndices )
        {
#ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad model; no color indices\n";
#endif
            validated = true;
            valid = false;
            return false;
        }

        // we must have at least 1 color in the list
        size_t nColor = 0;
        SGCOLOR* pColor = NULL;
        pColors->GetColorList( nColor, pColor );

        if( nColor < 1 )
        {
#ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad model; no colors\n";
#endif
            validated = true;
            valid = false;
            return false;
        }

        size_t nCLIdx = 0;
        int* pCLIdx = NULL;
        m_ColorIndices->GetIndices( nCLIdx, pCLIdx );

        if( nCLIdx != nCoords )
        {
#ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad model; color indices do not match number of vertices\n";
#endif
            validated = true;
            valid = false;
            return false;
        }

        // check that color[n] >= 0 and < nColors
        for( size_t i = 0; i < nCLIdx; ++i )
        {
            if( pCLIdx[i] < 0 || pCLIdx[i] >= nCLIdx )
            {
#ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] bad model; color index out of bounds\n";
#endif
                validated = true;
                valid = false;
                return false;
            }
        }
    }

    validated = true;
    valid = true;
    return true;
}


void SGFACESET::GatherCoordIndices( std::vector< int >& aIndexList )
{
    if( m_CoordIndices )
        m_CoordIndices->GatherCoordIndices( aIndexList );

    return;
}


bool SGFACESET::CalcNormals( SGNODE** aPtr )
{
    SGCOORDS* coords = m_Coords;

    if( m_RCoords )
        coords = m_RCoords;

    if( NULL == coords || coords->coords.empty() )
        return false;

    if( m_Normals && !m_Normals->norms.empty( ) )
        return true;

    if( m_RNormals && !m_RNormals->norms.empty( ) )
        return true;

    return coords->CalcNormals( this, aPtr );
}
