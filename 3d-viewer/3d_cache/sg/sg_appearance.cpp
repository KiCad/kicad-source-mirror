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
#include <utility>

#include <3d_cache/sg/sg_appearance.h>
#include <3d_cache/sg/sg_helpers.h>

SGAPPEARANCE::SGAPPEARANCE( SGNODE* aParent ) : SGNODE( aParent)
{
    m_SGtype = S3D::SGTYPE_APPEARANCE;

    // defaults in accord with VRML2.0 spec
    ambient = 0.2;
    shininess = 0.2;
    transparency = 0.0;
    diffuse.SetColor( 0.8, 0.8, 0.8 );

    if( NULL != aParent && S3D::SGTYPE_SHAPE != aParent->GetNodeType() )
    {
        m_Parent = NULL;

#ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] inappropriate parent to SGAPPEARANCE (type ";
        std::cerr << aParent->GetNodeType() << ")\n";
#endif
    }
    else if( NULL != aParent && S3D::SGTYPE_SHAPE == aParent->GetNodeType() )
    {
        m_Parent->AddChildNode( this );
    }

    return;
}


SGAPPEARANCE::~SGAPPEARANCE()
{
    return;
}


bool SGAPPEARANCE::SetParent( SGNODE* aParent, bool notify )
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

    // only a SGSHAPE may be parent to a SGAPPEARANCE
    if( NULL != aParent && S3D::SGTYPE_SHAPE != aParent->GetNodeType() )
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
    if( NULL == aRGBColor )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] NULL pointer passed for aRGBColor\n";
        return false;
    }

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
    if( NULL == aRGBColor )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] NULL pointer passed for aRGBColor\n";
        return false;
    }

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
    if( NULL == aRGBColor )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] NULL pointer passed for aRGBColor\n";
        return false;
    }

    return specular.SetColor( aRGBColor );
}

bool SGAPPEARANCE::SetSpecular( const SGCOLOR& aRGBColor )
{
    return specular.SetColor( aRGBColor );
}


SGNODE* SGAPPEARANCE::FindNode(const char *aNodeName, const SGNODE *aCaller)
{
    if( NULL == aNodeName || 0 == aNodeName[0] )
        return NULL;

    if( !m_Name.compare( aNodeName ) )
        return this;

    return NULL;
}


void SGAPPEARANCE::unlinkChildNode( const SGNODE* aCaller )
{
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] unexpected code branch; node should have no children or refs\n";
    return;
}


void SGAPPEARANCE::unlinkRefNode( const SGNODE* aCaller )
{
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] unexpected code branch; node should have no children or refs\n";
    return;
}


bool SGAPPEARANCE::AddRefNode( SGNODE* aNode )
{
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] this node does not accept children or refs\n";
    return false;
}


bool SGAPPEARANCE::AddChildNode( SGNODE* aNode )
{
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] this node does not accept children or refs\n";
    return false;
}


void SGAPPEARANCE::ReNameNodes( void )
{
    m_written = false;

    // rename this node
    m_Name.clear();
    GetName();
}


bool SGAPPEARANCE::WriteVRML( std::ofstream& aFile, bool aReuseFlag )
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
    S3D::FormatFloat( tmp, ambient );
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


bool SGAPPEARANCE::WriteCache( std::ofstream& aFile, SGNODE* parentNode )
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
    aFile.write( (char*)&ambient, sizeof(ambient) );
    aFile.write( (char*)&shininess, sizeof(shininess) );
    aFile.write( (char*)&transparency, sizeof(transparency) );
    S3D::WriteColor( aFile, diffuse );
    S3D::WriteColor( aFile, emissive );
    S3D::WriteColor( aFile, specular );

    if( aFile.fail() )
        return false;

    return true;
}


bool SGAPPEARANCE::ReadCache( std::ifstream& aFile, SGNODE* parentNode )
{
    aFile.read( (char*)&ambient, sizeof(ambient) );
    aFile.read( (char*)&shininess, sizeof(shininess) );
    aFile.read( (char*)&transparency, sizeof(transparency) );
    S3D::ReadColor( aFile, diffuse );
    S3D::ReadColor( aFile, emissive );
    S3D::ReadColor( aFile, specular );

    if( aFile.fail() )
        return false;

    return true;
}
