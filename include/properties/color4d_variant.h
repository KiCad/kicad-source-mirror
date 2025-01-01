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

#ifndef KICAD_COLOR4D_VARIANT_H
#define KICAD_COLOR4D_VARIANT_H

#include <gal/color4d.h>
#include <wx/variant.h>

class COLOR4D_VARIANT_DATA : public wxVariantData
{
public:
    COLOR4D_VARIANT_DATA();

    COLOR4D_VARIANT_DATA( const wxString& aColorStr );

    COLOR4D_VARIANT_DATA( const KIGFX::COLOR4D& aColor );

    bool Eq( wxVariantData& aOther ) const override;

    wxString GetType() const override { return wxT( "COLOR4D" ); }

    bool Read( wxString& aString ) override;

    bool Write( wxString& aString ) const override;

    bool GetAsAny( wxAny* aAny ) const override;

    static wxVariantData* VariantDataFactory( const wxAny& aAny );

    const KIGFX::COLOR4D& Color() { return m_color; }

    void SetColor( const KIGFX::COLOR4D& aColor ) { m_color = aColor; }

protected:
    KIGFX::COLOR4D m_color;
};

#endif //KICAD_COLOR4D_VARIANT_H
