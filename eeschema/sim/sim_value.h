/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SIM_VALUE_H
#define SIM_VALUE_H

#include <memory>
#include <wx/string.h>

class SIM_VALUE_BASE
{
public:
    enum class TYPE
    {
        BOOL,
        INT,
        FLOAT,
        COMPLEX,
        STRING,

        BOOL_VECTOR,
        INT_VECTOR,
        FLOAT_VECTOR,
        COMPLEX_VECTOR
    };

    static std::unique_ptr<SIM_VALUE_BASE> Create( TYPE aType, wxString aString );
    static std::unique_ptr<SIM_VALUE_BASE> Create( TYPE aType );

    void operator=( const wxString& aString );
    virtual bool operator==( const SIM_VALUE_BASE& aOther ) const = 0;

    virtual void FromString( const wxString& aString ) = 0;
    virtual wxString ToString() const = 0;
};


template <typename T>
class SIM_VALUE : public SIM_VALUE_BASE
{
public:
    SIM_VALUE() = default;
    SIM_VALUE( const T& aValue );

    void FromString( const wxString& aString ) override;
    wxString ToString() const override;

    void operator=( const T& aValue );
    bool operator==( const SIM_VALUE_BASE& aOther ) const override;

private:
    T m_value;
};

#endif /* SIM_VALUE_H */
