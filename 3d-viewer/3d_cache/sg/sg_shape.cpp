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

#include "3d_cache/sg/sg_shape.h"
#include "3d_cache/sg/sg_faceset.h"
#include "3d_cache/sg/sg_appearance.h"
#include "3d_cache/sg/sg_helpers.h"
#include "3d_cache/sg/sg_coordindex.h"
#include "3d_cache/sg/sg_coords.h"
#include "3d_cache/sg/sg_colors.h"
#include "3d_cache/sg/sg_normals.h"


SGSHAPE::SGSHAPE( SGNODE* aParent ) : SGNODE( aParent )
{
    m_SGtype = S3D::SGTYPE_SHAPE;
    m_Appearance = nullptr;
    m_RAppearance = nullptr;
    m_FaceSet = nullptr;
    m_RFaceSet = nullptr;

    if( nullptr != aParent && S3D::SGTYPE_TRANSFORM != aParent->GetNodeType() )
    {
        m_Parent = nullptr;

        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] inappropriate parent to SGSHAPE (type %d)" ),
                    __FILE__, __FUNCTION__, __LINE__, aParent->GetNodeType() );
    }
    else if( nullptr != aParent && S3D::SGTYPE_TRANSFORM == aParent->GetNodeType() )
    {
        m_Parent->AddChildNode( this );
    }
}


SGSHAPE::~SGSHAPE()
{
    // drop references
    if( m_RAppearance )
    {
        m_RAppearance->delNodeRef( this );
        m_RAppearance = nullptr;
    }

    if( m_RFaceSet )
    {
        m_RFaceSet->delNodeRef( this );
        m_RFaceSet = nullptr;
    }

    // delete objects
    if( m_Appearance )
    {
        m_Appearance->SetParent( nullptr, false );
        delete m_Appearance;
        m_Appearance = nullptr;
    }

    if( m_FaceSet )
    {
        m_FaceSet->SetParent( nullptr, false );
        delete m_FaceSet;
        m_FaceSet = nullptr;
    }
}


bool SGSHAPE::SetParent( SGNODE* aParent, bool notify )
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

    // only a SGTRANSFORM may be parent to a SGSHAPE
    if( nullptr != aParent && S3D::SGTYPE_TRANSFORM != aParent->GetNodeType() )
        return false;

    m_Parent = aParent;

    if( m_Parent )
        m_Parent->AddChildNode( this );

    return true;
}


SGNODE* SGSHAPE::FindNode( const char* aNodeName, const SGNODE* aCaller )
{
    if( nullptr == aNodeName || 0 == aNodeName[0] )
        return nullptr;

    if( !m_Name.compare( aNodeName ) )
        return this;

    SGNODE* tmp = nullptr;

    if( nullptr != m_Appearance )
    {
        tmp = m_Appearance->FindNode( aNodeName, this );

        if( tmp )
        {
            return tmp;
        }
    }

    if( nullptr != m_FaceSet )
    {
        tmp = m_FaceSet->FindNode( aNodeName, this );

        if( tmp )
        {
            return tmp;
        }
    }

    // query the parent if appropriate
    if( aCaller == m_Parent || nullptr == m_Parent )
        return nullptr;

    return m_Parent->FindNode( aNodeName, this );
}


void SGSHAPE::unlinkNode( const SGNODE* aNode, bool isChild )
{
    if( nullptr == aNode )
        return;

    if( isChild )
    {
        if( aNode == m_Appearance )
        {
            m_Appearance = nullptr;
            return;
        }

        if( aNode == m_FaceSet )
        {
            m_FaceSet = nullptr;
            return;
        }
    }
    else
    {
        if( aNode == m_RAppearance )
        {
            delNodeRef( this );
            m_RAppearance = nullptr;
            return;
        }

        if( aNode == m_RFaceSet )
        {
            delNodeRef( this );
            m_RFaceSet = nullptr;
            return;
        }
    }

    wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] unlinkNode() did not find its target" ),
                __FILE__, __FUNCTION__, __LINE__ );
}


void SGSHAPE::unlinkChildNode( const SGNODE* aNode )
{
    unlinkNode( aNode, true );
}


void SGSHAPE::unlinkRefNode( const SGNODE* aNode )
{
    unlinkNode( aNode, false );
}


bool SGSHAPE::addNode( SGNODE* aNode, bool isChild )
{
    wxCHECK( aNode, false );

    if( S3D::SGTYPE_APPEARANCE == aNode->GetNodeType() )
    {
        if( m_Appearance || m_RAppearance )
        {
            if( aNode != m_Appearance && aNode != m_RAppearance )
            {
                wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] assigning multiple Appearance "
                                             "nodes" ),
                            __FILE__, __FUNCTION__, __LINE__ );

                return false;
            }

            return true;
        }

        if( isChild )
        {
            m_Appearance = (SGAPPEARANCE*)aNode;
            m_Appearance->SetParent( this );
        }
        else
        {
            m_RAppearance = (SGAPPEARANCE*)aNode;
            m_RAppearance->addNodeRef( this );
        }

        return true;
    }

    if( S3D::SGTYPE_FACESET == aNode->GetNodeType() )
    {
        if( m_FaceSet || m_RFaceSet )
        {
            if( aNode != m_FaceSet && aNode != m_RFaceSet )
            {
                wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] assigning multiple FaceSet nodes" ),
                            __FILE__, __FUNCTION__, __LINE__ );

                return false;
            }

            return true;
        }

        if( isChild )
        {
            m_FaceSet = (SGFACESET*)aNode;
            m_FaceSet->SetParent( this );
        }
        else
        {
            m_RFaceSet = (SGFACESET*)aNode;
            m_RFaceSet->addNodeRef( this );
        }

        return true;
    }

    wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] object %s is not a valid type for this "
                                 "object (%d)" ),
                __FILE__, __FUNCTION__, __LINE__, aNode->GetName(), aNode->GetNodeType() );

    return false;
}


bool SGSHAPE::AddRefNode( SGNODE* aNode )
{
    return addNode( aNode, false );
}


bool SGSHAPE::AddChildNode( SGNODE* aNode )
{
    return addNode( aNode, true );
}


void SGSHAPE::ReNameNodes( void )
{
    m_written = false;

    // rename this node
    m_Name.clear();
    GetName();

    // rename Appearance
    if( m_Appearance )
        m_Appearance->ReNameNodes();

    // rename FaceSet
    if( m_FaceSet )
        m_FaceSet->ReNameNodes();
}


bool SGSHAPE::WriteVRML( std::ostream& aFile, bool aReuseFlag )
{
    if( !m_Appearance && !m_RAppearance && !m_FaceSet && !m_RFaceSet )
    {
        return false;
    }

    if( aReuseFlag )
    {
        if( !m_written )
        {
            aFile << "DEF " << GetName() << " Shape {\n";
            m_written = true;
        }
        else
        {
            aFile << " USE " << GetName() << "\n";
            return true;
        }
    }
    else
    {
        aFile << " Shape {\n";
    }

    if( m_Appearance )
        m_Appearance->WriteVRML( aFile, aReuseFlag );

    if( m_RAppearance )
        m_RAppearance->WriteVRML( aFile, aReuseFlag );

    if( m_FaceSet )
        m_FaceSet->WriteVRML( aFile, aReuseFlag );

    if( m_RFaceSet )
        m_RFaceSet->WriteVRML( aFile, aReuseFlag );

    aFile << "}\n";

    return true;
}


bool SGSHAPE::WriteCache( std::ostream& aFile, SGNODE* parentNode )
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
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] bad stream" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    // check if any references are unwritten and swap parents if so
    if( nullptr != m_RAppearance && !m_RAppearance->isWritten() )
        m_RAppearance->SwapParent(this);

    if( nullptr != m_RFaceSet && !m_RFaceSet->isWritten() )
        m_RFaceSet->SwapParent( this );

    aFile << "[" << GetName() << "]";
    #define NITEMS 4
    bool items[NITEMS];
    int i;

    for( i = 0; i < NITEMS; ++i )
        items[i] = 0;

    i = 0;

    if( nullptr != m_Appearance )
        items[i] = true;

    ++i;

    if( nullptr != m_RAppearance )
        items[i] = true;

    ++i;

    if( nullptr != m_FaceSet )
        items[i] = true;

    ++i;

    if( nullptr != m_RFaceSet )
        items[i] = true;

    for( int jj = 0; jj < NITEMS; ++jj )
        aFile.write( (char*)&items[jj], sizeof(bool) );

    if( items[0] )
        m_Appearance->WriteCache( aFile, this );

    if( items[1] )
        aFile << "[" << m_RAppearance->GetName() << "]";

    if( items[2] )
        m_FaceSet->WriteCache( aFile, this );

    if( items[3] )
        aFile << "[" << m_RFaceSet->GetName() << "]";

    if( aFile.fail() )
        return false;

    m_written = true;
    return true;
}


bool SGSHAPE::ReadCache( std::istream& aFile, SGNODE* parentNode )
{
    wxCHECK( m_Appearance == nullptr && m_RAppearance == nullptr && m_FaceSet == nullptr &&
             m_RFaceSet == nullptr, false );

    #define NITEMS 4
    bool items[NITEMS];

    for( int i = 0; i < NITEMS; ++i )
        aFile.read( (char*)&items[i], sizeof(bool) );

    if( ( items[0] && items[1] ) || ( items[2] && items[3] ) )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; multiple item definitions "
                                     "at position %ul" ),
                    __FILE__, __FUNCTION__, __LINE__,
                    static_cast<unsigned long>( aFile.tellg() ) );

        return false;
    }

    std::string name;

    if( items[0] )
    {
        if( S3D::SGTYPE_APPEARANCE != S3D::ReadTag( aFile, name ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; bad child appearance "
                                         "tag at position %ul" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<unsigned long>( aFile.tellg() ) );

            return false;
        }

        m_Appearance = new SGAPPEARANCE( this );
        m_Appearance->SetName( name.c_str() );

        if( !m_Appearance->ReadCache( aFile, this ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data while reading appearance "
                                         "'%s'" ),
                        __FILE__, __FUNCTION__, __LINE__, name );

            return false;
        }
    }

    if( items[1] )
    {
        if( S3D::SGTYPE_APPEARANCE != S3D::ReadTag( aFile, name ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; bad ref appearance tag "
                                         "at position %ul" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<unsigned long>( aFile.tellg() ) );

            return false;
        }

        SGNODE* np = FindNode( name.c_str(), this );

        if( !np )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data: cannot find ref "
                                         "appearance '%s'" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        name );

            return false;
        }

        if( S3D::SGTYPE_APPEARANCE != np->GetNodeType() )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data: type is not "
                                         "SGAPPEARANCE '%s'" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        name );

            return false;
        }

        m_RAppearance = (SGAPPEARANCE*)np;
        m_RAppearance->addNodeRef( this );
    }

    if( items[2] )
    {
        if( S3D::SGTYPE_FACESET != S3D::ReadTag( aFile, name ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; bad child face set tag "
                                         "at position %ul" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<unsigned long>( aFile.tellg() ) );

            return false;
        }

        m_FaceSet = new SGFACESET( this );
        m_FaceSet->SetName( name.c_str() );

        if( !m_FaceSet->ReadCache( aFile, this ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data while reading face set "
                                         "'%s'" ),
                        __FILE__, __FUNCTION__, __LINE__, name );

            return false;
        }
    }

    if( items[3] )
    {
        if( S3D::SGTYPE_FACESET != S3D::ReadTag( aFile, name ) )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data; bad ref face set tag at "
                                         "position %ul" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        static_cast<unsigned long>( aFile.tellg() ) );

            return false;
        }

        SGNODE* np = FindNode( name.c_str(), this );

        if( !np )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data: cannot find ref face "
                                         "set '%s'" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        name );

            return false;
        }

        if( S3D::SGTYPE_FACESET != np->GetNodeType() )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] corrupt data: type is not SGFACESET "
                                         "'%s'" ),
                        __FILE__, __FUNCTION__, __LINE__,
                        name );

            return false;
        }

        m_RFaceSet = (SGFACESET*)np;
        m_RFaceSet->addNodeRef( this );
    }

    if( aFile.fail() )
        return false;

    return true;
}


bool SGSHAPE::Prepare( const glm::dmat4* aTransform, S3D::MATLIST& materials,
                       std::vector< SMESH >& meshes )
{
    SMESH m;
    S3D::INIT_SMESH( m );

    SGAPPEARANCE* pa = m_Appearance;
    SGFACESET* pf = m_FaceSet;

    if( nullptr == pa )
        pa = m_RAppearance;

    if( nullptr == pf )
        pf = m_RFaceSet;

    // no face sets = nothing to render, which is valid though pointless
    if( nullptr == pf )
        return true;

    if( !pf->validate() )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] bad model; inconsistent data" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return true;
    }

    if( nullptr == pa )
    {
        m.m_MaterialIdx = 0;
    }
    else
    {
        int idx;

        if( !S3D::GetMatIndex( materials, pa, idx ) )
        {
            m.m_MaterialIdx = 0;
        }
        else
        {
            m.m_MaterialIdx = idx;
        }
    }

    SGCOLORS* pc = pf->m_Colors;
    SGCOORDS* pv = pf->m_Coords;
    SGCOORDINDEX* vidx = pf->m_CoordIndices;
    SGNORMALS* pn = pf->m_Normals;

    if( nullptr == pc )
        pc = pf->m_RColors;

    if( nullptr == pv )
        pv = pf->m_RCoords;

    if( nullptr == pn )
        pn = pf->m_RNormals;

    // set the vertex points and indices
    size_t nCoords = 0;
    SGPOINT* pCoords = nullptr;
    pv->GetCoordsList( nCoords, pCoords );

    size_t nColors = 0;
    SGCOLOR* pColors = nullptr;

    if( pc )
    {
        // check the vertex colors
        pc->GetColorList( nColors, pColors );

        if( nColors < nCoords )
        {
            wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] bad model; not enough colors per "
                                         "vertex (%ul vs %ul)" ),
                        __FILE__, __FUNCTION__, __LINE__, static_cast<unsigned long>( nColors ),
                        static_cast<unsigned long>( nCoords ) );

            return true;
        }
    }

    // set the vertex indices
    size_t nvidx = 0;
    int*   lv = nullptr;
    vidx->GetIndices( nvidx, lv );

    // note: reduce the vertex set to include only the referenced vertices
    std::vector< int > vertices;            // store the list of temp vertex indices
    std::map< int, unsigned int > indexmap; // map temp vertex to true vertex
    std::map< int, unsigned int >::iterator mit;

    for( unsigned int i = 0; i < nvidx; ++i )
    {
        mit = indexmap.find( lv[i] );

        if( mit == indexmap.end() )
        {
            indexmap.emplace( lv[i], vertices.size() );
            vertices.push_back( lv[i] );
        }
    }

    if( vertices.size() < 3 )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] bad model; not enough vertices" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return true;
    }

    // construct the final vertex/color list
    SFVEC3F* lColors = nullptr;
    SFVEC3F* lCoords = new SFVEC3F[ vertices.size() ];
    int ti;

    if( pc )
    {
        lColors = new SFVEC3F[vertices.size()];
        m.m_Color = lColors;
    }

    if( pc )
    {
        for( size_t i = 0; i < vertices.size(); ++i )
        {
            ti = vertices[i];
            glm::dvec4 pt( pCoords[ti].x, pCoords[ti].y, pCoords[ti].z, 1.0 );
            pt = (*aTransform) * pt;
            pColors[ti].GetColor( lColors[i].x, lColors[i].y, lColors[i].z );
            lCoords[i] = SFVEC3F( pt.x, pt.y, pt.z );
        }
    }
    else
    {
        for( size_t i = 0; i < vertices.size(); ++i )
        {
            ti = vertices[i];
            glm::dvec4 pt( pCoords[ti].x, pCoords[ti].y, pCoords[ti].z, 1.0 );
            pt = (*aTransform) * pt;
            lCoords[i] = SFVEC3F( pt.x, pt.y, pt.z );
        }
    }

    m.m_VertexSize = (unsigned int) vertices.size();
    m.m_Positions = lCoords;
    unsigned int* lvidx = new unsigned int[ nvidx ];

    for( unsigned int i = 0; i < nvidx; ++i )
    {
        mit = indexmap.find( lv[i] );

        if( mit != indexmap.end() )
            lvidx[i] = mit->second;
    }

    m.m_FaceIdxSize = (unsigned int )nvidx;
    m.m_FaceIdx = lvidx;

    // set the per-vertex normals
    size_t nNorms = 0;
    SGVECTOR* pNorms = nullptr;
    double x, y, z;

    pn->GetNormalList( nNorms, pNorms );
    SFVEC3F* lNorms = new SFVEC3F[ vertices.size() ];

    for( size_t i = 0; i < vertices.size(); ++i )
    {
        ti = vertices[i];
        pNorms[ti].GetVector( x, y, z );
        glm::dvec4 pt( x, y, z, 0.0 );
        pt = (*aTransform) * pt;

        lNorms[i] = SFVEC3F( pt.x, pt.y, pt.z );
    }

    m.m_Normals = lNorms;
    meshes.push_back( m );

    return true;
}
