/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <inspectable_impl.h>

#include <wx/propgrid/property.h>


bool INSPECTABLE::Set( PROPERTY_BASE* aProperty, wxAny& aValue, bool aNotify )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    void* object = propMgr.TypeCast( this, TYPE_HASH( *this ), aProperty->OwnerHash() );

    if( object )
    {
        aProperty->setter( object, aValue );

        if( aNotify )
            propMgr.PropertyChanged( this, aProperty );
    }

    return object != nullptr;
}


bool INSPECTABLE::Set( PROPERTY_BASE* aProperty, wxVariant aValue, bool aNotify )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    void*             object = propMgr.TypeCast( this, TYPE_HASH( *this ), aProperty->OwnerHash() );

    if( object )
    {
        wxPGChoices choices = aProperty->GetChoices( this );

        if( choices.GetCount() )
        {
            if( aProperty->TypeHash() == TYPE_HASH( int ) )
                aProperty->set<int>( object, aValue.GetInteger() );
            else
                aProperty->set<wxString>( object, choices.GetLabel( aValue.GetInteger() ) );
        }
        else
        {
            aProperty->set<wxVariant>( object, aValue );
        }

        if( aNotify )
            propMgr.PropertyChanged( this, aProperty );
    }

    return object != nullptr;
}


wxAny INSPECTABLE::Get( PROPERTY_BASE* aProperty ) const
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    const void* object = propMgr.TypeCast( this, TYPE_HASH( *this ), aProperty->OwnerHash() );
    return object ? aProperty->getter( object ) : wxAny();
}
