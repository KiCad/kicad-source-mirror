/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 NBEE Embedded Systems SL, Miguel Angel Ajo <miguelangel@ajo.es>
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


/**
 * @file  footprint_wizard.cpp
 * @brief Class FOOTPRINT_WIZARD and FOOTPRINT_WIZARD_LIST
 */

#include "footprint_wizard.h"


FOOTPRINT_WIZARD::~FOOTPRINT_WIZARD()
{
}


void FOOTPRINT_WIZARD::register_wizard()
{
    FOOTPRINT_WIZARD_LIST::register_wizard( this );
}


std::vector<FOOTPRINT_WIZARD*> FOOTPRINT_WIZARD_LIST::m_FootprintWizards;

FOOTPRINT_WIZARD* FOOTPRINT_WIZARD_LIST::GetWizard( int aIndex )
{
    return m_FootprintWizards[aIndex];
}


FOOTPRINT_WIZARD* FOOTPRINT_WIZARD_LIST::GetWizard( const wxString& aName )
{
    int max = GetWizardsCount();

    for( int i = 0; i<max; i++ )
    {
        FOOTPRINT_WIZARD*   wizard = GetWizard( i );

        wxString            name = wizard->GetName();

        if( name.Cmp( aName )==0 )
            return wizard;
    }

    return nullptr;
}


int FOOTPRINT_WIZARD_LIST::GetWizardsCount()
{
    return m_FootprintWizards.size();
}


void FOOTPRINT_WIZARD_LIST::register_wizard( FOOTPRINT_WIZARD* aWizard )
{
    // Search for this entry do not register twice this wizard):
    for( int ii = 0; ii < GetWizardsCount(); ii++ )
    {
        if( aWizard == GetWizard( ii ) )    // Already registered
            return;
    }

    // Search for a wizard with the same name, and remove it if found
    for( int ii = 0; ii < GetWizardsCount(); ii++ )
    {
        FOOTPRINT_WIZARD* wizard = GetWizard( ii );

        if( wizard->GetName() == aWizard->GetName() )
        {
            m_FootprintWizards.erase( m_FootprintWizards.begin() + ii );
            delete wizard;
            break;
        }
    }

    m_FootprintWizards.push_back( aWizard );
}


bool FOOTPRINT_WIZARD_LIST::deregister_object( void* aObject )
{
    int max = GetWizardsCount();

    for( int ii = 0; ii < max; ii++ )
    {
        FOOTPRINT_WIZARD* wizard = GetWizard( ii );

        if( wizard->GetObject() == aObject )
        {
            m_FootprintWizards.erase( m_FootprintWizards.begin() + ii );
            delete wizard;
            return true;
        }
    }

    return false;
}
