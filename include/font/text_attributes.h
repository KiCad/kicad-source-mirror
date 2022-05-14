/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <iostream>
#include <wx/log.h>
#include <cmath>
#include <math/vector2d.h>
#include <gal/color4d.h>
#include "../../libs/kimath/include/geometry/eda_angle.h"

class EDA_TEXT;

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
    KIFONT::FONT*     m_Font = nullptr;
    GR_TEXT_H_ALIGN_T m_Halign = GR_TEXT_H_ALIGN_CENTER;
    GR_TEXT_V_ALIGN_T m_Valign = GR_TEXT_V_ALIGN_CENTER;
    EDA_ANGLE         m_Angle = ANGLE_0;
    double            m_LineSpacing = 1.0;
    int               m_StrokeWidth = 0;
    bool              m_Italic = false;
    bool              m_Bold = false;
    bool              m_Underlined = false;
    KIGFX::COLOR4D    m_Color = KIGFX::COLOR4D::UNSPECIFIED;
    bool              m_Visible = true;
    bool              m_Mirrored = false;
    bool              m_Multiline = true;
    VECTOR2I          m_Size;

    /**
     * If true, keep rotation angle between -90...90 degrees for readability
     */
    bool              m_KeepUpright = false;
};


#endif //TEXT_ATTRIBUTES_H
