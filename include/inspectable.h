/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
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

#include "property_mgr.h"
#include "property.h"

#include <boost/optional.hpp>

/**
 * Class that other classes need to inherit from, in order to be inspectable.
 */
class INSPECTABLE
{
public:
    virtual ~INSPECTABLE()
    {
    }

    bool Set( PROPERTY_BASE* aProperty, wxAny& aValue )
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        TYPE_ID thisType = TYPE_HASH( *this );
        void* object = propMgr.TypeCast( this, thisType, aProperty->OwnerHash() );

        if( object )
            aProperty->setter( object, aValue );

        return object != nullptr;
    }

    template<typename T>
    bool Set( PROPERTY_BASE* aProperty, T aValue )
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        TYPE_ID thisType = TYPE_HASH( *this );
        void* object = propMgr.TypeCast( this, thisType, aProperty->OwnerHash() );

        if( object )
            aProperty->set<T>( object, aValue );

        return object != nullptr;
    }

    template<typename T>
    bool Set( const wxString& aProperty, T aValue )
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        TYPE_ID thisType = TYPE_HASH( *this );
        PROPERTY_BASE* prop = propMgr.GetProperty( thisType, aProperty );
        void* object = nullptr;

        if( prop )
        {
            object = propMgr.TypeCast( this, thisType, prop->OwnerHash() );

            if( object )
                prop->set<T>( object, aValue );
        }

        return object != nullptr;
    }

    wxAny Get( PROPERTY_BASE* aProperty )
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        TYPE_ID thisType = TYPE_HASH( *this );
        void* object = propMgr.TypeCast( this, thisType, aProperty->OwnerHash() );
        return object ? aProperty->getter( object ) : wxAny();
    }

    template<typename T>
    T Get( PROPERTY_BASE* aProperty )
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        TYPE_ID thisType = TYPE_HASH( *this );
        void* object = propMgr.TypeCast( this, thisType, aProperty->OwnerHash() );
        return object ? aProperty->get<T>( object ) : T();
    }

    template<typename T>
    boost::optional<T> Get( const wxString& aProperty )
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        TYPE_ID thisType = TYPE_HASH( *this );
        PROPERTY_BASE* prop = propMgr.GetProperty( thisType, aProperty );
        boost::optional<T> ret;

        if( prop )
        {
            void* object = propMgr.TypeCast( this, thisType, prop->OwnerHash() );

            if( object )
                ret = prop->get<T>( object );
        }

        return ret;
    }
};

#endif /* INSPECTABLE_H */
