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

#include <font/text_attributes.h>
#include <font/outline_font.h>


TEXT_ATTRIBUTES::TEXT_ATTRIBUTES( KIFONT::FONT* aFont ) :
    m_Font( aFont ),
    m_Halign( GR_TEXT_H_ALIGN_CENTER ),
    m_Valign( GR_TEXT_V_ALIGN_CENTER ),
    m_Angle( ANGLE_0 ),
    m_LineSpacing( 1.0 ),
    m_StrokeWidth( 0 ),
    m_Italic( false ),
    m_Bold( false ),
    m_Underlined( false ),
    m_Hover( false ),
    m_Color( KIGFX::COLOR4D::UNSPECIFIED ),
    m_Mirrored( false ),
    m_Multiline( true ),
    m_KeepUpright( false ),
    m_StoredStrokeWidth( 0 )
{
}


int TEXT_ATTRIBUTES::Compare( const TEXT_ATTRIBUTES& aRhs ) const
{
    wxString fontName;

    if( m_Font )
        fontName = m_Font->GetName();

    wxString rhsFontName;

    if( aRhs.m_Font )
        rhsFontName = aRhs.m_Font->GetName();

    int retv = fontName.Cmp( rhsFontName );

    if( retv )
        return retv;

    if( m_Size.x != aRhs.m_Size.x )
        return m_Size.x - aRhs.m_Size.x;

    if( m_Size.y != aRhs.m_Size.y )
        return m_Size.y - aRhs.m_Size.y;

    if( m_StrokeWidth != aRhs.m_StrokeWidth )
        return m_StrokeWidth - aRhs.m_StrokeWidth;

    if( m_Angle.AsDegrees() != aRhs.m_Angle.AsDegrees() )
        return m_Angle.AsDegrees() < aRhs.m_Angle.AsDegrees() ? -1 : 1;

    if( m_LineSpacing != aRhs.m_LineSpacing )
        return m_LineSpacing < aRhs.m_LineSpacing ? -1 : 1;

    if( m_Halign != aRhs.m_Halign )
        return m_Halign - aRhs.m_Halign;

    if( m_Valign != aRhs.m_Valign )
        return m_Valign - aRhs.m_Valign;

    if( m_Italic != aRhs.m_Italic )
        return m_Italic - aRhs.m_Italic;

    if( m_Bold != aRhs.m_Bold )
        return m_Bold - aRhs.m_Bold;

    if( m_Underlined != aRhs.m_Underlined )
        return m_Underlined - aRhs.m_Underlined;

    retv = m_Color.Compare( aRhs.m_Color );

    if( retv )
        return retv;

    if( m_Mirrored != aRhs.m_Mirrored )
        return m_Mirrored - aRhs.m_Mirrored;

    if( m_Multiline != aRhs.m_Multiline )
        return m_Multiline - aRhs.m_Multiline;

    return m_KeepUpright - aRhs.m_KeepUpright;
}


std::ostream& operator<<( std::ostream& aStream, const TEXT_ATTRIBUTES& aAttributes )
{
    aStream << "Font: \"";

    if ( aAttributes.m_Font )
        aStream << *aAttributes.m_Font;
    else
        aStream << "UNDEFINED";

    aStream << "\"\n";
    aStream << "Horizontal Alignment: " << aAttributes.m_Halign << std::endl
            << "Vertical Alignment: " << aAttributes.m_Valign << std::endl
            << "Angle: " << aAttributes.m_Angle << std::endl
            << "Line Spacing: " << aAttributes.m_LineSpacing << std::endl
            << "Stroke Width: " << aAttributes.m_StrokeWidth << std::endl
            << "Italic: " << aAttributes.m_Italic << std::endl
            << "Bold: " << aAttributes.m_Bold << std::endl
            << "Underline: " << aAttributes.m_Underlined << std::endl
            << "Color: " << aAttributes.m_Color << std::endl
            << "Mirrored " << aAttributes.m_Mirrored << std::endl
            << "Multilined: " << aAttributes.m_Multiline << std::endl
            << "Size: " << aAttributes.m_Size << std::endl
            << "Keep Upright: " << aAttributes.m_KeepUpright << std::endl;

    return aStream;
}
