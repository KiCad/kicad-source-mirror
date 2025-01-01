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

#ifndef KICAD_EDA_ANGLE_VARIANT_H
#define KICAD_EDA_ANGLE_VARIANT_H

#include <geometry/eda_angle.h>

#include <wx/variant.h>

class EDA_ANGLE_VARIANT_DATA : public wxVariantData
{
public:
    EDA_ANGLE_VARIANT_DATA();

    EDA_ANGLE_VARIANT_DATA( double aAngleDegrees );

    EDA_ANGLE_VARIANT_DATA( const EDA_ANGLE& aAngle );

    bool Eq( wxVariantData& aOther ) const override;

    wxString GetType() const override { return wxT( "EDA_ANGLE" ); }

    bool Read( wxString& aString ) override;

    bool Write( wxString& aString ) const override;

    bool GetAsAny( wxAny* aAny ) const override;

    static wxVariantData* VariantDataFactory( const wxAny& aAny );

    const EDA_ANGLE& Angle() { return m_angle; }

    void SetAngle( const EDA_ANGLE& aAngle ) { m_angle = aAngle;  }

protected:
    EDA_ANGLE m_angle;
};

#endif //KICAD_EDA_ANGLE_VARIANT_H
