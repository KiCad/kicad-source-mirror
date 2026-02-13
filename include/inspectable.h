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

#include <wx/any.h>
#include <wx/string.h>
#include <wx/variant.h>

#include <optional>

class PROPERTY_BASE;

/**
 * Class that other classes need to inherit from, in order to be inspectable.
 */
class INSPECTABLE
{
public:
    virtual ~INSPECTABLE()
    {
    }

    bool Set( PROPERTY_BASE* aProperty, wxAny& aValue, bool aNotify = true );

    template <typename T>
    bool Set( PROPERTY_BASE* aProperty, T aValue, bool aNotify = true );

    bool Set( PROPERTY_BASE* aProperty, wxVariant aValue, bool aNotify = true );

    template <typename T>
    bool Set( const wxString& aProperty, T aValue, bool aNotify = true );

    wxAny Get( PROPERTY_BASE* aProperty ) const;

    template <typename T>
    T Get( PROPERTY_BASE* aProperty ) const;

    template <typename T>
    std::optional<T> Get( const wxString& aProperty ) const;
};

#endif /* INSPECTABLE_H */
