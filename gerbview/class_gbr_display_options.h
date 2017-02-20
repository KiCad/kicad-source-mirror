/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2016 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file class_gbr_display_options.h
 * @brief Class GBR_DISPLAY_OPTIONS is a helper class to handle display options
 * (filling modes and afew other options
 */

#ifndef CLASS_GBR_DISPLAY_OPTIONS_H
#define CLASS_GBR_DISPLAY_OPTIONS_H

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
    bool    m_DisplayPolarCood;         ///< Option to display coordinates in status bar in X,Y or Polar coords
    bool    m_DisplayDCodes;            ///< Option to show dcode values on items drawn with a dcode tool
    bool    m_DisplayNegativeObjects;   ///< Option to draw negative objects in a specific color
    bool    m_IsPrinting;               ///< true when printing a page, false when drawing on screen
    bool    m_ForceBlackAndWhite;       ///< Option print in blackand white (ont used id draw mode
    COLOR4D m_NegativeDrawColor;        ///< The color used to draw negative objects, usually the
                                        ///< background color, but not always, when negative objects
                                        ///< must be visible
    COLOR4D m_BgDrawColor;              ///< The background color

public:
    GBR_DISPLAY_OPTIONS()
    {
        m_DisplayFlashedItemsFill = true;
        m_DisplayLinesFill = true;
        m_DisplayPolygonsFill = true;
        m_DisplayPolarCood = false;
        m_DisplayDCodes = true;
        m_IsPrinting = false;
        m_DisplayNegativeObjects = false;
        m_ForceBlackAndWhite    = false;
        m_NegativeDrawColor     = COLOR4D( DARKGRAY );
        m_BgDrawColor = COLOR4D::BLACK;
    }
};

#endif    // #ifndef CLASS_GBR_DISPLAY_OPTIONS_H
