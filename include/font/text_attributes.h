/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "../../libs/kimath/include/geometry/eda_angle.h"


namespace KIFONT
{
class FONT;
};


// Graphic Text alignments:
//
// NB: values -1,0,1 are used in computations, do not change them
//

enum GR_TEXT_H_ALIGN_T
{
    GR_TEXT_H_ALIGN_LEFT   = -1,
    GR_TEXT_H_ALIGN_CENTER = 0,
    GR_TEXT_H_ALIGN_RIGHT  = 1
};

enum GR_TEXT_V_ALIGN_T
{
    GR_TEXT_V_ALIGN_TOP    = -1,
    GR_TEXT_V_ALIGN_CENTER = 0,
    GR_TEXT_V_ALIGN_BOTTOM = 1
};


#define TO_HJUSTIFY( x ) static_cast<GR_TEXT_H_ALIGN_T>( x )
#define TO_VJUSTIFY( x ) static_cast<GR_TEXT_V_ALIGN_T>( x )


class TEXT_ATTRIBUTES
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
    KIGFX::COLOR4D    m_Color;
    bool              m_Visible;
    bool              m_Mirrored;
    bool              m_Multiline;
    VECTOR2I          m_Size;

    /**
     * If true, keep rotation angle between -90...90 degrees for readability
     */
    bool              m_KeepUpright;
};


extern std::ostream& operator<<( std::ostream& aStream, const TEXT_ATTRIBUTES& aAttributes );


template<>
struct std::hash<TEXT_ATTRIBUTES>
{
    std::size_t operator()( const TEXT_ATTRIBUTES& aAttributes ) const
    {
        return hash_val( aAttributes.m_Font, aAttributes.m_Halign, aAttributes.m_Valign,
                         aAttributes.m_Angle.AsDegrees(), aAttributes.m_LineSpacing,
                         aAttributes.m_StrokeWidth, aAttributes.m_Italic, aAttributes.m_Bold,
                         aAttributes.m_Underlined, aAttributes.m_Color, aAttributes.m_Visible,
                         aAttributes.m_Mirrored, aAttributes.m_Multiline, aAttributes.m_Size.x,
                         aAttributes.m_Size.y );
    }
};

#endif //TEXT_ATTRIBUTES_H
