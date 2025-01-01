/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2016 Jean-Pierre Charras  jp.charras at wanadoo.fr
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

#ifndef GBR_DISPLAY_OPTIONS_H
#define GBR_DISPLAY_OPTIONS_H

using KIGFX::COLOR4D;

/**
 * @note Some of these parameters are used only for printing, some others only
 * for drawing on screen.
 */

class GBR_DISPLAY_OPTIONS
{
public:
    bool    m_DisplayFlashedItemsFill;  ///< Option to draw flashed items (filled/sketch)
    bool    m_DisplayLinesFill;         ///< Option to draw line items (filled/sketch)
    bool    m_DisplayPolygonsFill;      ///< Option to draw polygons (filled/sketch)
    bool    m_DisplayPageLimits;
    bool    m_IsPrinting;               ///< true when printing a page, false when drawing on screen
    bool    m_ForceOpacityMode;         ///< Display layers in transparency (alpha channel) forced mode
    bool    m_XORMode;                  ///< Display layers in exclusive-or mode
    bool    m_HighContrastMode;         ///< High contrast mode (dim un-highlighted objects)
    bool    m_FlipGerberView;           ///< Display as a mirror image
    COLOR4D m_NegativeDrawColor;        ///< The color used to draw negative objects, usually the
                                        ///< background color, but not always, when negative objects
                                        ///< must be visible
    double m_OpacityModeAlphaValue;     ///< the alpha channel (opacity) value in opacity forced mode

public:
    GBR_DISPLAY_OPTIONS()
    {
        m_DisplayFlashedItemsFill = true;
        m_DisplayLinesFill = true;
        m_DisplayPolygonsFill = true;
        m_DisplayPageLimits = false;
        m_IsPrinting = false;
        m_NegativeDrawColor = COLOR4D( DARKGRAY );
        m_ForceOpacityMode = false;
        m_OpacityModeAlphaValue = 0.6;
        m_XORMode = false;
        m_HighContrastMode = false;
        m_FlipGerberView = false;
    }
};

#endif    // GBR_DISPLAY_OPTIONS_H
