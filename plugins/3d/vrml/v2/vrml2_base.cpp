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

#include "vrml2_base.h"
#include "vrml2_helpers.h"


WRL2BASE::WRL2BASE( WRL2NODE* aParent ) : WRL2NODE()
{
    m_Type = V2_BASE;
    return;
}


WRL2BASE::~WRL2BASE()
{
    std::list< WRL2NODE* >::iterator sC = m_Children.begin();
    std::list< WRL2NODE* >::iterator eC = m_Children.end();

    while( sC != eC )
    {
        (*sC)->SetParent( NULL );
        delete (*sC);
        ++sC;
    }

    sC.clear();
    return;
}


// functions inherited from WRL2NODE
bool WRL2BASE::SetParent( WRL2NODE* aParent )
{
    #ifdef DEBUG
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] attempting to extract name from virtual base node\n";
    #endif
}


WRL2NODE* WRL2BASE::FindNode( const char *aNodeName, const WRL2NODE *aCaller )
{
    if( NULL == aNodeName || 0 == aNodeName[0] )
        return NULL;

    if( !m_Name.compare( aNodeName ) )
        return this;

    FIND_NODE( aNodeName, m_Children, this );

    return NULL;
}


bool WRL2BASE::AddRefNode( WRL2NODE* aNode )
{
    #ifdef DEBUG
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] attempting to add reference node to WRL2BASE\n";
    #endif

    return false;
}


bool WRL2BASE::AddChildNode( WRL2NODE* aNode )
{
    if( aNode->GetNodeType() == WRL2_BASE )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] attempting to add a base node to another base node\n";
        #endif
        return false;
    }

    std::list< WRL2NODE* >::iterator sC = m_Children.begin();
    std::list< WRL2NODE* >::iterator eC = m_Children.end();

    while( sC != eC )
    {
        if( *sC == aNode )
            return false;
    }

    aNode->SetParent( this );
    m_Children.push_back( aNode );

    return true;
}


const char* WRL2BASE::GetName( void )
{
    #ifdef DEBUG
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] attempting to extract name from virtual base node\n";
    #endif
    return NULL;
    return NULL;
}


bool WRL2BASE::SetName(const char *aName)
{
    #ifdef DEBUG
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] attempting to set name on virtual base node\n";
    #endif
    return false;
}


bool WRL2BASE::Read( WRLPROC& proc )
{
    if( proc.GetVRMLType() != VRML_V2 )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] no open file or file is not a VRML2 file\n";
        #endif
        return false;
    }

    std::string glob;

    while( proc.ReadGlob( glob ) )
    {
        // XXX - Process node name


        xxx;
    } while( !glob.empty() );


    // XXX -
    #warning TO BE IMPLEMENTED
    return false;
}
