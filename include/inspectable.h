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

#ifndef INSPECTABLE_H
#define INSPECTABLE_H

#include <core/wx_stl_compat.h>

#include <properties/property_mgr.h>
#include <properties/property.h>

#include <optional>

/**
 * Class that other classes need to inherit from, in order to be inspectable.
 */
class INSPECTABLE
{
public:
    virtual ~INSPECTABLE()
    {
    }

    bool Set( PROPERTY_BASE* aProperty, wxAny& aValue, bool aNotify = true )
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

    template<typename T>
    bool Set( PROPERTY_BASE* aProperty, T aValue, bool aNotify = true )
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        void*             object = propMgr.TypeCast( this, TYPE_HASH( *this ), aProperty->OwnerHash() );

        if( object )
        {
            aProperty->set<T>( object, aValue );

            if( aNotify )
                propMgr.PropertyChanged( this, aProperty );
        }

        return object != nullptr;
    }

    bool Set( PROPERTY_BASE* aProperty, wxVariant aValue, bool aNotify = true )
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        void*             object = propMgr.TypeCast( this, TYPE_HASH( *this ), aProperty->OwnerHash() );

        if( object )
        {
            wxPGChoices choices = aProperty->GetChoices( this );

            if( choices.GetCount()  )
            {
                // If the property type is int, use the choice value directly
                // Otherwise, convert to the choice label string
                if( aProperty->TypeHash() == TYPE_HASH( int ) )
                    aProperty->set<int>( object, aValue.GetInteger() );
                else
                    aProperty->set<wxString>( object, choices.GetLabel( aValue.GetInteger() ) );
            }
            else
                aProperty->set<wxVariant>( object, aValue );

            if( aNotify )
                propMgr.PropertyChanged( this, aProperty );
        }

        return object != nullptr;
    }

    template<typename T>
    bool Set( const wxString& aProperty, T aValue, bool aNotify = true )
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        TYPE_ID thisType = TYPE_HASH( *this );
        PROPERTY_BASE* prop = propMgr.GetProperty( thisType, aProperty );
        void* object = nullptr;

        if( prop )
        {
            object = propMgr.TypeCast( this, thisType, prop->OwnerHash() );

            if( object )
            {
                prop->set<T>( object, aValue );

                if( aNotify )
                    propMgr.PropertyChanged( this, prop );
            }
        }

        return object != nullptr;
    }

    wxAny Get( PROPERTY_BASE* aProperty ) const
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        const void* object = propMgr.TypeCast( this, TYPE_HASH( *this ), aProperty->OwnerHash() );
        return object ? aProperty->getter( object ) : wxAny();
    }

    template<typename T>
    T Get( PROPERTY_BASE* aProperty ) const
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        const void* object = propMgr.TypeCast( this, TYPE_HASH( *this ), aProperty->OwnerHash() );

        if( !object )
            throw std::runtime_error( "Could not cast INSPECTABLE to the requested type" );

        return aProperty->get<T>( object );
    }

    template<typename T>
    std::optional<T> Get( const wxString& aProperty ) const
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        TYPE_ID thisType = TYPE_HASH( *this );
        PROPERTY_BASE* prop = propMgr.GetProperty( thisType, aProperty );
        std::optional<T> ret;

        if( prop )
        {
            const void* object = propMgr.TypeCast( this, thisType, prop->OwnerHash() );

            if( object )
                ret = prop->get<T>( object );
        }

        return ret;
    }
};

#endif /* INSPECTABLE_H */
