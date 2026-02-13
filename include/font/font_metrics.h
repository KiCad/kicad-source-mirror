/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef FONT_METRICS_H_
#define FONT_METRICS_H_

#include <gal/gal.h>

namespace KIFONT
{

class GAL_API METRICS
{
public:
    /**
     * Compute the vertical position of an overbar.  This is the distance between the text
     * baseline and the overbar.
     */
    double GetOverbarVerticalPosition( double aGlyphHeight ) const
    {
        return aGlyphHeight * m_OverbarHeight;
    }

    /**
     * Compute the vertical position of an underline.  This is the distance between the text
     * baseline and the underline.
     */
    double GetUnderlineVerticalPosition( double aGlyphHeight ) const
    {
        return aGlyphHeight * m_UnderlineOffset;
    }

    double GetInterline( double aFontHeight ) const
    {
        return aFontHeight * m_InterlinePitch;
    }

    static const METRICS& Default();

public:
    double m_InterlinePitch  =  1.68;
    double m_OverbarHeight   =  1.23;
    double m_UnderlineOffset = -0.16;
};

} // namespace KIFONT

#endif // FONT_METRICS_H_
