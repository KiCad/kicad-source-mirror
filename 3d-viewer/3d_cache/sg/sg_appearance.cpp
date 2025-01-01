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
#include <utility>
#include <sstream>
#include <wx/log.h>

#include "3d_cache/sg/sg_appearance.h"
#include "3d_cache/sg/sg_helpers.h"


SGAPPEARANCE::SGAPPEARANCE( SGNODE* aParent ) : SGNODE( aParent )
{
    m_SGtype = S3D::SGTYPE_APPEARANCE;

    // defaults in accord with VRML2.0 spec
    ambient.SetColor( 0.05317f, 0.17879f, 0.01804f );
    shininess = 0.2f;
    transparency = 0.0f;
    diffuse.SetColor( 0.8f, 0.8f, 0.8f );

    if( nullptr != aParent && S3D::SGTYPE_SHAPE != aParent->GetNodeType() )
    {
        m_Parent = nullptr;

        wxLogTrace( MASK_3D_SG,
                    wxT( "%s:%s:%d * [BUG] inappropriate parent to SGAPPEARANCE (type %s )" ),
                    __FILE__, __FUNCTION__, __LINE__, aParent->GetNodeType() );
    }
    else if( nullptr != aParent && S3D::SGTYPE_SHAPE == aParent->GetNodeType() )
    {
        m_Parent->AddChildNode( this );
    }
}


SGAPPEARANCE::~SGAPPEARANCE()
{
}


bool SGAPPEARANCE::SetParent( SGNODE* aParent, bool notify )
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

    // only a SGSHAPE may be parent to a SGAPPEARANCE
    if( nullptr != aParent && S3D::SGTYPE_SHAPE != aParent->GetNodeType() )
        return false;

    m_Parent = aParent;

    if( m_Parent )
        m_Parent->AddChildNode( this );

    return true;
}


bool SGAPPEARANCE::SetEmissive( float aRVal, float aGVal, float aBVal )
{
    return emissive.SetColor( aRVal, aGVal, aBVal );
}


bool SGAPPEARANCE::SetEmissive( const SGCOLOR* aRGBColor )
{
    wxCHECK_MSG( aRGBColor, false, wxT( "NULL pointer passed for aRGBColor" ) );

    return emissive.SetColor( aRGBColor );
}


bool SGAPPEARANCE::SetEmissive( const SGCOLOR& aRGBColor )
{
    return emissive.SetColor( aRGBColor );
}


bool SGAPPEARANCE::SetDiffuse( float aRVal, float aGVal, float aBVal )
{
    return diffuse.SetColor( aRVal, aGVal, aBVal );
}


bool SGAPPEARANCE::SetDiffuse( const SGCOLOR* aRGBColor )
{
    wxCHECK_MSG( aRGBColor, false, wxT( "NULL pointer passed for aRGBColor" ) );

    return diffuse.SetColor( aRGBColor );
}


bool SGAPPEARANCE::SetDiffuse( const SGCOLOR& aRGBColor )
{
    return diffuse.SetColor( aRGBColor );
}


bool SGAPPEARANCE::SetSpecular( float aRVal, float aGVal, float aBVal )
{
    return specular.SetColor( aRVal, aGVal, aBVal );
}


bool SGAPPEARANCE::SetSpecular( const SGCOLOR* aRGBColor )
{
    wxCHECK_MSG( aRGBColor, false, wxT( "NULL pointer passed for aRGBColor" ) );

    return specular.SetColor( aRGBColor );
}

bool SGAPPEARANCE::SetSpecular( const SGCOLOR& aRGBColor )
{
    return specular.SetColor( aRGBColor );
}


bool SGAPPEARANCE::SetAmbient( float aRVal, float aGVal, float aBVal )
{
    return ambient.SetColor( aRVal, aGVal, aBVal );
}


bool SGAPPEARANCE::SetAmbient( const SGCOLOR* aRGBColor )
{
    wxCHECK_MSG( aRGBColor, false, wxT( "NULL pointer passed for aRGBColor" ) );

    return ambient.SetColor( aRGBColor );
}


bool SGAPPEARANCE::SetAmbient( const SGCOLOR& aRGBColor )
{
    return ambient.SetColor( aRGBColor );
}


SGNODE* SGAPPEARANCE::FindNode( const char* aNodeName, const SGNODE* aCaller) noexcept
{
    if( nullptr == aNodeName || 0 == aNodeName[0] )
        return nullptr;

    if( !m_Name.compare( aNodeName ) )
        return this;

    return nullptr;
}


void SGAPPEARANCE::unlinkChildNode( const SGNODE* aCaller ) noexcept
{
    wxCHECK_MSG( aCaller, /* void */,
                 wxT( "unexpected code branch; node should have no children or refs" ) );
}


void SGAPPEARANCE::unlinkRefNode( const SGNODE* aCaller ) noexcept
{
    wxCHECK_MSG( aCaller, /* void */,
                 wxT( "unexpected code branch; node should have no children or refs" ) );
}


bool SGAPPEARANCE::AddRefNode( SGNODE* aNode ) noexcept
{
    wxCHECK_MSG( aNode, false, wxT( "this node does not accept children or refs" ) );

    // This is redundant but it keeps gcc from generating a warning on debug builds.
    return false;
}


bool SGAPPEARANCE::AddChildNode( SGNODE* aNode ) noexcept
{
    wxCHECK_MSG( aNode, false, wxT( "this node does not accept children or refs" ) );

    // This is redundant but it keeps gcc from generating a warning on debug builds.
    return false;
}


void SGAPPEARANCE::ReNameNodes( void )
{
    m_written = false;

    // rename this node
    m_Name.clear();
    GetName();
}


bool SGAPPEARANCE::WriteVRML( std::ostream& aFile, bool aReuseFlag )
{
    if( aReuseFlag )
    {
        if( !m_written )
        {
            aFile << " appearance DEF " << GetName() << " Appearance {\n";
            m_written = true;
        }
        else
        {
            aFile << " appearance USE " << GetName() << "\n";
            return true;
        }
    }
    else
    {
        aFile << " appearance Appearance {\n";
    }

    aFile << "  material Material {\n";

    std::string tmp;
    float ambr, ambg, ambb;
    ambient.GetColor( ambr, ambg, ambb );
    float amb = ( 0.212671 * ambr + 0.71516 * ambg + 0.072169 * ambb );
    diffuse.GetColor( ambr, ambg, ambb );
    float den = ( 0.212671 * ambr + 0.71516 * ambg + 0.072169 * ambb );

    if( den < 0.004f )
        den = 0.004f;

    amb /= den;

    if( amb > 1.0f )
        amb = 1.0f;

    S3D::FormatFloat( tmp, amb );
    aFile << "   ambientIntensity " << tmp << "\n";

    float red, green, blue;
    diffuse.GetColor( red, green, blue );
    S3D::FormatFloat( tmp, red );
    aFile << "   diffuseColor " << tmp << " ";
    S3D::FormatFloat( tmp, green );
    aFile << tmp << " ";
    S3D::FormatFloat( tmp, blue);
    aFile << tmp << "\n";

    emissive.GetColor( red, green, blue );
    S3D::FormatFloat( tmp, red );
    aFile << "   emissiveColor " << tmp << " ";
    S3D::FormatFloat( tmp, green );
    aFile << tmp << " ";
    S3D::FormatFloat( tmp, blue);
    aFile << tmp << "\n";

    S3D::FormatFloat( tmp, shininess );
    aFile << "   shininess " << tmp << "\n";

    specular.GetColor( red, green, blue );
    S3D::FormatFloat( tmp, red );
    aFile << "   specularColor " << tmp << " ";
    S3D::FormatFloat( tmp, green );
    aFile << tmp << " ";
    S3D::FormatFloat( tmp, blue);
    aFile << tmp << "\n";

    S3D::FormatFloat( tmp, transparency );
    aFile << "   transparency " << tmp << "\n";

    aFile << "} }\n";

    return true;
}


bool SGAPPEARANCE::WriteCache( std::ostream& aFile, SGNODE* parentNode )
{
    if( nullptr == parentNode )
    {
        wxCHECK_MSG( m_Parent, false, wxT( "corrupt data; m_aParent is NULL" ) );

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

    wxCHECK_MSG( parentNode == m_Parent, false, wxT( "corrupt data; parentNode != m_aParent" ) );

    if( !aFile.good() )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] bad stream" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    aFile << "[" << GetName() << "]";
    S3D::WriteColor( aFile, ambient );
    aFile.write( (char*) &shininess, sizeof( shininess ) );
    aFile.write( (char*) &transparency, sizeof( transparency ) );
    S3D::WriteColor( aFile, diffuse );
    S3D::WriteColor( aFile, emissive );
    S3D::WriteColor( aFile, specular );

    if( aFile.fail() )
        return false;

    m_written = true;
    return true;
}


bool SGAPPEARANCE::ReadCache( std::istream& aFile, SGNODE* parentNode )
{
    S3D::ReadColor( aFile, ambient );
    aFile.read( (char*) &shininess, sizeof( shininess ) );
    aFile.read( (char*) &transparency, sizeof( transparency ) );
    S3D::ReadColor( aFile, diffuse );
    S3D::ReadColor( aFile, emissive );
    S3D::ReadColor( aFile, specular );

    if( aFile.fail() )
        return false;

    return true;
}
