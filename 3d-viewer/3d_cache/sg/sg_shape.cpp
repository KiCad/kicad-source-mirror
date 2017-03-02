/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2017 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
    m_Appearance = NULL;
    m_RAppearance = NULL;
    m_FaceSet = NULL;
    m_RFaceSet = NULL;

    if( NULL != aParent && S3D::SGTYPE_TRANSFORM != aParent->GetNodeType() )
    {
        m_Parent = NULL;

#ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] inappropriate parent to SGSHAPE (type ";
        ostr << aParent->GetNodeType() << ")";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
#endif
    }
    else if( NULL != aParent && S3D::SGTYPE_TRANSFORM == aParent->GetNodeType() )
    {
        m_Parent->AddChildNode( this );
    }

    return;
}


SGSHAPE::~SGSHAPE()
{
    // drop references
    if( m_RAppearance )
    {
        m_RAppearance->delNodeRef( this );
        m_RAppearance = NULL;
    }

    if( m_RFaceSet )
    {
        m_RFaceSet->delNodeRef( this );
        m_RFaceSet = NULL;
    }

    // delete objects
    if( m_Appearance )
    {
        m_Appearance->SetParent( NULL, false );
        delete m_Appearance;
        m_Appearance = NULL;
    }

    if( m_FaceSet )
    {
        m_FaceSet->SetParent( NULL, false );
        delete m_FaceSet;
        m_FaceSet = NULL;
    }

    return;
}


bool SGSHAPE::SetParent( SGNODE* aParent, bool notify )
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

    // only a SGTRANSFORM may be parent to a SGSHAPE
    if( NULL != aParent && S3D::SGTYPE_TRANSFORM != aParent->GetNodeType() )
        return false;

    m_Parent = aParent;

    if( m_Parent )
        m_Parent->AddChildNode( this );

    return true;
}


SGNODE* SGSHAPE::FindNode(const char *aNodeName, const SGNODE *aCaller)
{
    if( NULL == aNodeName || 0 == aNodeName[0] )
        return NULL;

    if( !m_Name.compare( aNodeName ) )
        return this;

    SGNODE* tmp = NULL;

    if( NULL != m_Appearance )
    {
        tmp = m_Appearance->FindNode( aNodeName, this );

        if( tmp )
        {
            return tmp;
        }
    }

    if( NULL != m_FaceSet )
    {
        tmp = m_FaceSet->FindNode( aNodeName, this );

        if( tmp )
        {
            return tmp;
        }
    }

    // query the parent if appropriate
    if( aCaller == m_Parent || NULL == m_Parent )
        return NULL;

    return m_Parent->FindNode( aNodeName, this );
}


void SGSHAPE::unlinkNode( const SGNODE* aNode, bool isChild )
{
    if( NULL == aNode )
        return;

    if( isChild )
    {
        if( aNode == m_Appearance )
        {
            m_Appearance = NULL;
            return;
        }

        if( aNode == m_FaceSet )
        {
            m_FaceSet = NULL;
            return;
        }
    }
    else
    {
        if( aNode == m_RAppearance )
        {
            delNodeRef( this );
            m_RAppearance = NULL;
            return;
        }

        if( aNode == m_RFaceSet )
        {
            delNodeRef( this );
            m_RFaceSet = NULL;
            return;
        }
    }

    #ifdef DEBUG
    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] unlinkNode() did not find its target";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    return;
}


void SGSHAPE::unlinkChildNode( const SGNODE* aNode )
{
    unlinkNode( aNode, true );
    return;
}


void SGSHAPE::unlinkRefNode( const SGNODE* aNode )
{
    unlinkNode( aNode, false );
    return;
}


bool SGSHAPE::addNode( SGNODE* aNode, bool isChild )
{
    if( NULL == aNode )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] NULL pointer passed for aNode";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    if( S3D::SGTYPE_APPEARANCE == aNode->GetNodeType() )
    {
        if( m_Appearance || m_RAppearance )
        {
            if( aNode != m_Appearance && aNode != m_RAppearance )
            {
                #ifdef DEBUG
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [BUG] assigning multiple Appearance nodes";
                wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
                #endif

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
                #ifdef DEBUG
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [BUG] assigning multiple FaceSet nodes";
                wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
                #endif

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

    #ifdef DEBUG
    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] object '" << aNode->GetName();
        ostr << "' is not a valid type for this object (" << aNode->GetNodeType() << ")";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

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

    return;
}


bool SGSHAPE::WriteVRML( std::ostream& aFile, bool aReuseFlag )
{
    if( !m_Appearance && !m_RAppearance
        && !m_FaceSet && !m_RFaceSet )
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
    if( NULL == parentNode )
    {
        if( NULL == m_Parent )
        {
            #ifdef DEBUG
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] corrupt data; m_aParent is NULL";
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
            #endif

            return false;
        }

        SGNODE* np = m_Parent;

        while( NULL != np->GetParent() )
            np = np->GetParent();

        if( np->WriteCache( aFile, NULL ) )
        {
            m_written = true;
            return true;
        }

        return false;
    }

    if( parentNode != m_Parent )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] corrupt data; parentNode != m_aParent";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    if( !aFile.good() )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [INFO] bad stream";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    // check if any references are unwritten and swap parents if so
    if( NULL != m_RAppearance && !m_RAppearance->isWritten() )
        m_RAppearance->SwapParent(this);

    if( NULL != m_RFaceSet && !m_RFaceSet->isWritten() )
        m_RFaceSet->SwapParent( this );

    aFile << "[" << GetName() << "]";
    #define NITEMS 4
    bool items[NITEMS];
    int i;

    for( i = 0; i < NITEMS; ++i )
        items[i] = 0;

    i = 0;
    if( NULL != m_Appearance )
        items[i] = true;

    ++i;
    if( NULL != m_RAppearance )
        items[i] = true;

    ++i;
    if( NULL != m_FaceSet )
        items[i] = true;

    ++i;
    if( NULL != m_RFaceSet )
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
    if( m_Appearance || m_RAppearance || m_FaceSet || m_RFaceSet )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] non-empty node";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    #define NITEMS 4
    bool items[NITEMS];

    for( int i = 0; i < NITEMS; ++i )
        aFile.read( (char*)&items[i], sizeof(bool) );

    if( ( items[0] && items[1] ) || ( items[2] && items[3] ) )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [INFO] corrupt data; multiple item definitions at position ";
        ostr << aFile.tellg();
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    std::string name;

    if( items[0] )
    {
        if( S3D::SGTYPE_APPEARANCE != S3D::ReadTag( aFile, name ) )
        {
            #ifdef DEBUG
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] corrupt data; bad child apperance tag at position ";
            ostr << aFile.tellg();
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
            #endif

            return false;
        }

        m_Appearance = new SGAPPEARANCE( this );
        m_Appearance->SetName( name.c_str() );

        if( !m_Appearance->ReadCache( aFile, this ) )
        {
            #ifdef DEBUG
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] corrupt data while reading appearance '";
            ostr << name << "'";
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
            #endif

            return false;
        }
    }

    if( items[1] )
    {
        if( S3D::SGTYPE_APPEARANCE != S3D::ReadTag( aFile, name ) )
        {
            #ifdef DEBUG
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] corrupt data; bad ref appearance tag at position ";
            ostr << aFile.tellg();
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
            #endif

            return false;
        }

        SGNODE* np = FindNode( name.c_str(), this );

        if( !np )
        {
            #ifdef DEBUG
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] corrupt data: cannot find ref appearance '";
            ostr << name << "'";
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
            #endif

            return false;
        }

        if( S3D::SGTYPE_APPEARANCE != np->GetNodeType() )
        {
            #ifdef DEBUG
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] corrupt data: type is not SGAPPEARANCE '";
            ostr << name << "'";
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
            #endif

            return false;
        }

        m_RAppearance = (SGAPPEARANCE*)np;
        m_RAppearance->addNodeRef( this );
    }

    if( items[2] )
    {
        if( S3D::SGTYPE_FACESET != S3D::ReadTag( aFile, name ) )
        {
            #ifdef DEBUG
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] corrupt data; bad child face set tag at position ";
            ostr << aFile.tellg();
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
            #endif

            return false;
        }

        m_FaceSet = new SGFACESET( this );
        m_FaceSet->SetName( name.c_str() );

        if( !m_FaceSet->ReadCache( aFile, this ) )
        {
            #ifdef DEBUG
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] corrupt data while reading face set '";
            ostr << name << "'";
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
            #endif

            return false;
        }
    }

    if( items[3] )
    {
        if( S3D::SGTYPE_FACESET != S3D::ReadTag( aFile, name ) )
        {
            #ifdef DEBUG
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] corrupt data; bad ref face set tag at position ";
            ostr << aFile.tellg();
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
            #endif

            return false;
        }

        SGNODE* np = FindNode( name.c_str(), this );

        if( !np )
        {
            #ifdef DEBUG
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] corrupt data: cannot find ref face set '";
            ostr << name << "'";
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
            #endif

            return false;
        }

        if( S3D::SGTYPE_FACESET != np->GetNodeType() )
        {
            #ifdef DEBUG
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] corrupt data: type is not SGFACESET '";
            ostr << name << "'";
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
            #endif

            return false;
        }

        m_RFaceSet = (SGFACESET*)np;
        m_RFaceSet->addNodeRef( this );
    }

    if( aFile.fail() )
        return false;

    return true;
}


bool SGSHAPE::Prepare( const glm::dmat4* aTransform,
    S3D::MATLIST& materials, std::vector< SMESH >& meshes )
{
    SMESH m;
    S3D::INIT_SMESH( m );

    SGAPPEARANCE* pa = m_Appearance;
    SGFACESET* pf = m_FaceSet;

    if( NULL == pa )
        pa = m_RAppearance;

    if( NULL == pf )
        pf = m_RFaceSet;

    // no face sets = nothing to render, which is valid though pointless
    if( NULL == pf )
        return true;

    if( !pf->validate() )
    {
#ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [INFO] bad model; inconsistent data";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
#endif
        return true;
    }

    if( NULL == pa )
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

    if( NULL == pc )
        pc = pf->m_RColors;

    if( NULL == pv )
        pv = pf->m_RCoords;

    if( NULL == pn )
        pn = pf->m_RNormals;

    // set the vertex points and indices
    size_t nCoords = 0;
    SGPOINT* pCoords = NULL;
    pv->GetCoordsList( nCoords, pCoords );

    size_t nColors = 0;
    SGCOLOR* pColors = NULL;

    if( pc )
    {
        // check the vertex colors
        pc->GetColorList( nColors, pColors );

        if( nColors < nCoords )
        {
            #ifdef DEBUG
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad model; not enough colors per vertex (";
            ostr << nColors << " vs " << nCoords << ")";
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
            #endif
            return true;
        }
    }

    // set the vertex indices
    size_t nvidx = 0;
    int*   lv = NULL;
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
            indexmap.insert( std::pair< int, unsigned int >( lv[i], vertices.size() ) );
            vertices.push_back( lv[i] );
        }
    }

    if( vertices.size() < 3 )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [INFO] bad model; not enough vertices";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif
        return true;
    }

    // construct the final vertex/color list
    SFVEC3F* lColors = NULL;
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
        lvidx[i] = mit->second;
    }

    m.m_FaceIdxSize = (unsigned int )nvidx;
    m.m_FaceIdx = lvidx;

    // set the per-vertex normals
    size_t nNorms = 0;
    SGVECTOR* pNorms = NULL;
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
