/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef STD_OPTIONAL_VARIANT_H
#define STD_OPTIONAL_VARIANT_H

#include <optional>
#include <wx/variant.h>

/*
 * Wrappers to allow use of std::optional<int> and std::optional<double> with the wxVariant
 * system.
 */

class STD_OPTIONAL_INT_VARIANT_DATA : public wxVariantData
{
public:
    STD_OPTIONAL_INT_VARIANT_DATA();

    STD_OPTIONAL_INT_VARIANT_DATA( std::optional<int> aValue );

    bool Eq( wxVariantData& aOther ) const override;

    wxString GetType() const override { return wxT( "std::optional<int>" ); }

    bool GetAsAny( wxAny* aAny ) const override
    {
        *aAny = m_value;
        return true;
    }

    std::optional<int> Value() const
    {
        return m_value;
    }

    static wxVariantData* VariantDataFactory( const wxAny& aAny )
    {
        return new STD_OPTIONAL_INT_VARIANT_DATA( aAny.As<std::optional<int>>() );
    }

protected:
    std::optional<int> m_value;
};


class STD_OPTIONAL_DOUBLE_VARIANT_DATA : public wxVariantData
{
public:
    STD_OPTIONAL_DOUBLE_VARIANT_DATA();

    STD_OPTIONAL_DOUBLE_VARIANT_DATA( std::optional<double> aValue );

    bool Eq( wxVariantData& aOther ) const override;

    wxString GetType() const override { return wxT( "std::optional<double>" ); }

    bool GetAsAny( wxAny* aAny ) const override
    {
        *aAny = m_value;
        return true;
    }

    std::optional<double> Value() const
    {
        return m_value;
    }

    static wxVariantData* VariantDataFactory( const wxAny& aAny )
    {
        return new STD_OPTIONAL_DOUBLE_VARIANT_DATA( aAny.As<std::optional<double>>() );
    }

protected:
    std::optional<double> m_value;
};

#endif //STD_OPTIONAL_VARIANT_H
