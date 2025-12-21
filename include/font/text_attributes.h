/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
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

#ifndef TEXT_ATTRIBUTES_H
#define TEXT_ATTRIBUTES_H

#include <math/vector2d.h>
#include <gal/color4d.h>
#include <geometry/eda_angle.h>
#include <gal/gal.h>


namespace KIFONT
{
class FONT;
};


// Graphic Text alignments:
//
// NB: values -1,0,1 are used in computations, do not change them
//

/// This is API surface mapped to common.types.HorizontalAlignment
enum GR_TEXT_H_ALIGN_T
{
    GR_TEXT_H_ALIGN_LEFT   = -1,
    GR_TEXT_H_ALIGN_CENTER = 0,
    GR_TEXT_H_ALIGN_RIGHT  = 1,
    GR_TEXT_H_ALIGN_INDETERMINATE
};

/// This is API surface mapped to common.types.VertialAlignment
enum GR_TEXT_V_ALIGN_T
{
    GR_TEXT_V_ALIGN_TOP    = -1,
    GR_TEXT_V_ALIGN_CENTER = 0,
    GR_TEXT_V_ALIGN_BOTTOM = 1,
    GR_TEXT_V_ALIGN_INDETERMINATE
};


/**
 * Get the reverse alignment: left-right are swapped, others are unchanged.
 */
constexpr GR_TEXT_H_ALIGN_T GetFlippedAlignment( GR_TEXT_H_ALIGN_T aAlign )
{
    // Could use the -1/1 promise of the enum too.
    switch( aAlign )
    {
    case GR_TEXT_H_ALIGN_LEFT:
        return GR_TEXT_H_ALIGN_RIGHT;
    case GR_TEXT_H_ALIGN_RIGHT:
        return GR_TEXT_H_ALIGN_LEFT;
    case GR_TEXT_H_ALIGN_CENTER:
    case GR_TEXT_H_ALIGN_INDETERMINATE:
        break;
    }
    return aAlign;
};


/**
 * Get the reverse alignment: top-bottom are swapped, others are unchanged.
 */
constexpr GR_TEXT_V_ALIGN_T GetFlippedAlignment( GR_TEXT_V_ALIGN_T aAlign )
{
    switch( aAlign )
    {
    case GR_TEXT_V_ALIGN_BOTTOM:
        return GR_TEXT_V_ALIGN_TOP;
    case GR_TEXT_V_ALIGN_TOP:
        return GR_TEXT_V_ALIGN_BOTTOM;
    case GR_TEXT_V_ALIGN_CENTER:
    case GR_TEXT_V_ALIGN_INDETERMINATE:
        break;
    }
    return aAlign;
};


/**
 * Convert an integral value to horizontal alignment.
 *
 *  * x < 0: Left align
 *  * x == 0: Center
 *  * x > 0: Right align
 */
constexpr GR_TEXT_H_ALIGN_T ToHAlignment( int x )
{
    if( x < 0 )

        return GR_TEXT_H_ALIGN_LEFT;
    else if( x > 0 )
        return GR_TEXT_H_ALIGN_RIGHT;

    return GR_TEXT_H_ALIGN_CENTER;
}


class GAL_API TEXT_ATTRIBUTES
{
public:
    TEXT_ATTRIBUTES( KIFONT::FONT* aFont = nullptr );

    int Compare( const TEXT_ATTRIBUTES& aRhs ) const;

    bool operator==( const TEXT_ATTRIBUTES& aRhs ) const { return Compare( aRhs ) == 0; }
    bool operator>( const TEXT_ATTRIBUTES& aRhs ) const { return Compare( aRhs ) > 0; }
    bool operator<( const TEXT_ATTRIBUTES& aRhs ) const { return Compare( aRhs ) < 0; }

    KIFONT::FONT*     m_Font;
    GR_TEXT_H_ALIGN_T m_Halign;
    GR_TEXT_V_ALIGN_T m_Valign;
    EDA_ANGLE         m_Angle;
    double            m_LineSpacing;
    int               m_StrokeWidth;
    bool              m_Italic;
    bool              m_Bold;
    bool              m_Underlined;
    bool              m_Hover;
    KIGFX::COLOR4D    m_Color;
    bool              m_Mirrored;
    bool              m_Multiline;
    VECTOR2I          m_Size;

    // If true, keep rotation angle between -90...90 degrees for readability
    bool              m_KeepUpright;
    int               m_StoredStrokeWidth;
};


extern GAL_API std::ostream& operator<<( std::ostream& aStream,
                                         const TEXT_ATTRIBUTES& aAttributes );


template<>
struct std::hash<TEXT_ATTRIBUTES>
{
    std::size_t operator()( const TEXT_ATTRIBUTES& aAttributes ) const
    {
        return hash_val( aAttributes.m_Font, aAttributes.m_Halign, aAttributes.m_Valign,
                         aAttributes.m_Angle.AsDegrees(), aAttributes.m_LineSpacing,
                         aAttributes.m_StrokeWidth, aAttributes.m_Italic, aAttributes.m_Bold,
                         aAttributes.m_Underlined, aAttributes.m_Color, aAttributes.m_Mirrored,
                         aAttributes.m_Multiline, aAttributes.m_Size.x, aAttributes.m_Size.y );
    }
};

#endif //TEXT_ATTRIBUTES_H
