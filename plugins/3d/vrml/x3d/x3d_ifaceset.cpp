/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
#include <cmath>
#include <wx/log.h>
#include <wx/xml/xml.h>
#include <wx/tokenzr.h>
#include "x3d_ops.h"
#include "x3d_ifaceset.h"
#include "x3d_coords.h"
#include "plugins/3dapi/ifsg_all.h"
#include "wrlfacet.h"


X3DIFACESET::X3DIFACESET() : X3DNODE()
{
    m_Type = X3D_INDEXED_FACE_SET;
    coord = nullptr;
    init();
}


X3DIFACESET::X3DIFACESET( X3DNODE* aParent ) : X3DNODE()
{
    m_Type = X3D_INDEXED_FACE_SET;
    coord = nullptr;
    init();

    if( nullptr != aParent )
    {
        X3DNODES ptype = aParent->GetNodeType();

        if( X3D_SHAPE == ptype )
            m_Parent = aParent;
    }

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


X3DIFACESET::~X3DIFACESET()
{
    wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Destroying IndexedFaceSet." ) );
}


void X3DIFACESET::init()
{
    coord = nullptr;

    ccw = true;
    creaseAngle = 0.733f;   // approx 42 degrees; this is larger than VRML spec.
    creaseLimit = 0.74317f; // cos( 0.733 )
}


void X3DIFACESET::readFields( wxXmlNode* aNode )
{
    // DEF
    // ccw
    // creaseAngle
    // coordIndex

    wxXmlAttribute* prop;

    for( prop = aNode->GetAttributes(); prop != nullptr; prop = prop->GetNext() )
    {
        const wxString& pname = prop->GetName();

        if( pname == wxT( "DEF" ) )
        {
            m_Name = prop->GetValue();
            m_Dict->AddName( m_Name, this );
        }
        else if( pname == wxT( "ccw" ) )
        {
            X3D::ParseSFBool( prop->GetValue(), ccw );
        }
        else if( pname == wxT( "creaseAngle" ) )
        {
            X3D::ParseSFFloat( prop->GetValue(), creaseAngle );

            if( creaseAngle < 0.0f )
                creaseAngle = 0.0f;
            else if( creaseAngle > static_cast<float>( M_PI * 0.34f ) )
                creaseAngle = static_cast<float>( M_PI / 3.0f );

            creaseLimit = cosf( creaseAngle );
        }
        else if( pname == wxT( "coordIndex" ) )
        {
            wxStringTokenizer indices( prop->GetValue() );

            while( indices.HasMoreTokens() )
            {
                long index = 0;
                indices.GetNextToken().ToLong( &index );
                coordIndex.push_back( (int) index );
            }
        }
    }
}


bool X3DIFACESET::Read( wxXmlNode* aNode, X3DNODE* aTopNode, X3D_DICT& aDict )
{
    if( nullptr == aTopNode || nullptr == aNode )
        return false;

    m_Dict = &aDict;
    readFields( aNode );
    bool ok = false;

    for( wxXmlNode* child = aNode->GetChildren(); child != nullptr; child = child->GetNext() )
    {
        if( child->GetName() == wxT( "Coordinate" ) )
            ok = X3D::ReadCoordinates( child, this, aDict );

    }

    if( false == ok )
        return false;

    if( !SetParent( aTopNode ) )
        return false;

    return true;
}


bool X3DIFACESET::SetParent( X3DNODE* aParent, bool doUnlink )
{
    if( aParent == m_Parent )
        return true;

    if( nullptr != aParent )
    {
        X3DNODES nt = aParent->GetNodeType();

        if( nt != X3D_SHAPE )
            return false;
    }

    if( nullptr != m_Parent && doUnlink )
        m_Parent->unlinkChildNode( this );

    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );

    return true;
}


bool X3DIFACESET::AddChildNode( X3DNODE* aNode )
{
    if( nullptr == aNode )
        return false;

    if( aNode->GetNodeType() != X3D_COORDINATE )
        return false;

    if( aNode == coord )
        return true;

    if( nullptr != coord )
        return false;

    m_Children.push_back( aNode );
    coord = aNode;

    if( aNode->GetParent() != this )
        aNode->SetParent( this );

    return true;
}


bool X3DIFACESET::AddRefNode( X3DNODE* aNode )
{
    if( nullptr == aNode )
        return false;

    if( aNode->GetNodeType() != X3D_COORDINATE )
        return false;

    if( aNode == coord )
        return true;

    if( nullptr != coord )
        return false;

    m_Refs.push_back( aNode );
    aNode->addNodeRef( this );
    coord = aNode;
    return true;
}


SGNODE* X3DIFACESET::TranslateToSG( SGNODE* aParent )
{
    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    if( nullptr != aParent && ptype != S3D::SGTYPE_SHAPE )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( " * [BUG] IndexedFaceSet does not have a valid Shape parent "
                         "(parent ID: %d)" ), ptype );

        return nullptr;
    }

    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Translating IndexedFaceSet with %zu children, %zu references, "
                     "%zu back pointers, and %zu coordinate indices." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size(), coordIndex.size() );

    if( m_sgNode )
    {
        if( nullptr != aParent )
        {
            if( nullptr == S3D::GetSGNodeParent( m_sgNode )
                && !S3D::AddSGNodeChild( aParent, m_sgNode ) )
            {
                return nullptr;
            }
            else if( aParent != S3D::GetSGNodeParent( m_sgNode )
                     && !S3D::AddSGNodeRef( aParent, m_sgNode ) )
            {
                return nullptr;
            }
        }

        return m_sgNode;
    }

    size_t vsize = coordIndex.size();

    if( nullptr == coord || vsize < 3 )
        return nullptr;

    WRLVEC3F* pcoords;
    size_t coordsize;
    ((X3DCOORDS*) coord)->GetCoords( pcoords, coordsize );

    if( coordsize < 3 )
        return nullptr;

    // check that all indices are valid
    for( size_t idx = 0; idx < vsize; ++idx )
    {
        if( coordIndex[idx] < 0 )
            continue;

        if( coordIndex[idx] >= (int)coordsize )
            return nullptr;
    }

    SHAPE   lShape;
    FACET*  fp = nullptr;
    size_t  iCoord;
    int     idx;        // coordinate index

    // no per-vertex colors; we can save a few CPU cycles
    for( iCoord = 0; iCoord < vsize; ++iCoord )
    {
        idx = coordIndex[iCoord];

        if( idx < 0 )
        {
            if( nullptr != fp )
            {
                if( fp->HasMinPoints() )
                    fp = nullptr;
                else
                    fp->Init();
            }

            continue;
        }

        // if the coordinate is bad then skip it
        if( idx >= (int)coordsize )
            continue;

        if( nullptr == fp )
            fp = lShape.NewFacet();

        // push the vertex value and index
        fp->AddVertex( pcoords[idx], idx );
    }

    SGNODE* np = nullptr;

    if( ccw )
        np = lShape.CalcShape( aParent, nullptr, WRL1_ORDER::ORD_CCW, creaseLimit, true );
    else
        np = lShape.CalcShape( aParent, nullptr, WRL1_ORDER::ORD_CLOCKWISE, creaseLimit, true );

    return np;
}
