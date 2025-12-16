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

#include <properties/color4d_variant.h>

COLOR4D_VARIANT_DATA::COLOR4D_VARIANT_DATA() :
        wxVariantData()
{}


COLOR4D_VARIANT_DATA::COLOR4D_VARIANT_DATA( const wxString& aColorStr ) :
        wxVariantData(),
        m_color( aColorStr )
{}


COLOR4D_VARIANT_DATA::COLOR4D_VARIANT_DATA( const KIGFX::COLOR4D& aColor ) :
        wxVariantData(),
        m_color( aColor )
{}


bool COLOR4D_VARIANT_DATA::Eq( wxVariantData& aOther ) const
{
    try
    {
        COLOR4D_VARIANT_DATA& evd = dynamic_cast<COLOR4D_VARIANT_DATA&>( aOther );

        return evd.m_color == m_color;
    }
    catch( std::bad_cast& )
    {
        return false;
    }
}


bool COLOR4D_VARIANT_DATA::Read( wxString& aString )
{
    m_color = KIGFX::COLOR4D( aString );
    return true;
}


bool COLOR4D_VARIANT_DATA::Write( wxString& aString ) const
{
    if( m_color.m_text.has_value() )
        aString = m_color.m_text.value();
    else
        aString = m_color.ToCSSString();

    return true;
}


bool COLOR4D_VARIANT_DATA::GetAsAny( wxAny* aAny ) const
{
    *aAny = m_color;
    return true;
}


wxVariantData* COLOR4D_VARIANT_DATA::VariantDataFactory( const wxAny& aAny )
{
    return new COLOR4D_VARIANT_DATA( aAny.As<KIGFX::COLOR4D>() );
}
