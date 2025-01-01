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
#include <wx/xml/xml.h>
#include <wx/tokenzr.h>
#include <wx/log.h>
#include "x3d_ops.h"
#include "x3d_coords.h"


X3DCOORDS::X3DCOORDS() : X3DNODE()
{
    m_Type = X3D_COORDINATE;
}


X3DCOORDS::X3DCOORDS( X3DNODE* aParent ) : X3DNODE()
{
    m_Type = X3D_COORDINATE;

    if( nullptr != aParent )
    {
        X3DNODES ptype = aParent->GetNodeType();

        if( X3D_INDEXED_FACE_SET == ptype )
            m_Parent = aParent;
    }

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


X3DCOORDS::~X3DCOORDS()
{
    wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Destroying Coordinate" ) );
}


bool X3DCOORDS::Read( wxXmlNode* aNode, X3DNODE* aTopNode, X3D_DICT& aDict )
{
    if( nullptr == aTopNode || nullptr == aNode )
        return false;

    m_Dict = &aDict;
    wxXmlAttribute* prop;

    for( prop = aNode->GetAttributes(); prop != nullptr; prop = prop->GetNext() )
    {
        const wxString& pname = prop->GetName();

        if( pname == wxT( "DEF" ) )
        {
            m_Name = prop->GetValue();
            m_Dict->AddName( m_Name, this );
        }
        else if( pname == wxT( "point" ) )
        {
            // Save points to vector as doubles
            wxStringTokenizer plist( prop->GetValue() );
            double point = 0.0;
            WRLVEC3F pt;
            int i = 0;

            while( plist.HasMoreTokens() )
            {
                if( plist.GetNextToken().ToCDouble( &point ) )
                {
                    // note: coordinates are multiplied by 2.54 to retain
                    // legacy behavior of 1 X3D unit = 0.1 inch; the SG*
                    // classes expect all units in mm.
                    switch( i % 3 )
                    {
                    case 0:
                        pt.x = point * 2.54;
                        break;

                    case 1:
                        pt.y = point * 2.54;
                        break;

                    case 2:
                        pt.z = point * 2.54;
                        points.push_back( pt );
                        break;

                    }
                }
                else
                {
                    return false;
                }

                ++i;
            }
        }
    }

    if( points.size() < 3 )
        return false;

    if( !SetParent( aTopNode ) )
        return false;

    return true;
}


bool X3DCOORDS::SetParent( X3DNODE* aParent, bool doUnlink )
{
    if( aParent == m_Parent )
        return true;

    if( nullptr != aParent )
    {
        X3DNODES nt = aParent->GetNodeType();

        if( nt != X3D_INDEXED_FACE_SET )
            return false;
    }

    if( nullptr != m_Parent && doUnlink )
        m_Parent->unlinkChildNode( this );

    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );

    return true;
}


bool X3DCOORDS::AddChildNode( X3DNODE* aNode )
{
    return false;
}


bool X3DCOORDS::AddRefNode( X3DNODE* aNode )
{
    return false;
}


void X3DCOORDS::GetCoords( WRLVEC3F*& aCoordList, size_t& aListSize )
{
    if( points.size() < 3 )
    {
        aCoordList = nullptr;
        aListSize = 0;
        return;
    }

    aCoordList = &points[0];
    aListSize = points.size();
}


SGNODE* X3DCOORDS::TranslateToSG( SGNODE* aParent )
{
    return nullptr;
}
