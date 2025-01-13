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

#include <properties/eda_angle_variant.h>


EDA_ANGLE_VARIANT_DATA::EDA_ANGLE_VARIANT_DATA() :
        wxVariantData()
{}


EDA_ANGLE_VARIANT_DATA::EDA_ANGLE_VARIANT_DATA( double aAngleDegrees ) :
        wxVariantData(),
        m_angle( aAngleDegrees, DEGREES_T )
{}


EDA_ANGLE_VARIANT_DATA::EDA_ANGLE_VARIANT_DATA( const EDA_ANGLE& aAngle ) :
        wxVariantData(),
        m_angle( aAngle )
{}


bool EDA_ANGLE_VARIANT_DATA::Eq( wxVariantData& aOther ) const
{
    try
    {
        EDA_ANGLE_VARIANT_DATA& evd = dynamic_cast<EDA_ANGLE_VARIANT_DATA&>( aOther );

        return evd.m_angle == m_angle;
    }
    catch( std::bad_cast& )
    {
        return false;
    }
}


bool EDA_ANGLE_VARIANT_DATA::Read( wxString& aString )
{
    double val;

    if( !aString.ToDouble( &val ) )
        return false;

    m_angle = EDA_ANGLE( val, DEGREES_T );
    return true;
}


bool EDA_ANGLE_VARIANT_DATA::Write( wxString& aString ) const
{
    aString = wxString::Format( wxT( "%g\u00B0" ), m_angle.AsDegrees() );
    return true;
}


bool EDA_ANGLE_VARIANT_DATA::GetAsAny( wxAny* aAny ) const
{
    *aAny = m_angle.AsDegrees();
    return true;
}


wxVariantData* EDA_ANGLE_VARIANT_DATA::VariantDataFactory( const wxAny& aAny )
{
    return new EDA_ANGLE_VARIANT_DATA( aAny.As<EDA_ANGLE>() );
}


