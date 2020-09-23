/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2016-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * Plotting engine (Gerber)
 *
 * @file gbr_plotter_apertures.h
 */

#pragma once


/* Class to handle a D_CODE when plotting a board using Standard Aperture Templates
 * (complex apertures need aperture macros to be flashed)
 * 5 types:
 * Circle (round)
 * Rectangle
 * Obround (oval)
 * regular polygon
 *
 * We need round apertures to plot lines, so we also defined a aperture type for plotting
 */
#define FIRST_DCODE_VALUE 10    // D_CODE < 10 is a command, D_CODE >= 10 is a tool

class APERTURE
{
public:
    enum APERTURE_TYPE {
        AT_CIRCLE   = 1,    // round aperture, to flash pads
        AT_RECT     = 2,    // rect aperture, to flash pads
        AT_PLOTTING = 3,    // round aperture, to plot lines
        AT_OVAL     = 4,    // oval aperture, to flash pads
        AT_REGULAR_POLY = 5,// Regular polygon (n vertices, n = 3 .. 12, with rotation)
        AT_REGULAR_POLY3,   // Regular polygon 3 vertices, with rotation
        AT_REGULAR_POLY4,   // Regular polygon 4 vertices, with rotation
        AT_REGULAR_POLY5,   // Regular polygon 5 vertices, with rotation
        AT_REGULAR_POLY6,   // Regular polygon 6 vertices, with rotation
        AT_REGULAR_POLY7,   // Regular polygon 7 vertices, with rotation
        AT_REGULAR_POLY8,   // Regular polygon 8 vertices, with rotation
        AT_REGULAR_POLY9,   // Regular polygon 9 vertices, with rotation
        AT_REGULAR_POLY10,  // Regular polygon 10 vertices, with rotation
        AT_REGULAR_POLY11,  // Regular polygon 11 vertices, with rotation
        AT_REGULAR_POLY12,  // Regular polygon 12 vertices, with rotation
    };

    void SetSize( const wxSize& aSize )
    {
        m_Size = aSize;
    }

    const wxSize GetSize()
    {
        return m_Size;
    }

    void SetDiameter( int aDiameter )
    {
        m_Size.x = aDiameter;
    }

    int GetDiameter()
    {
        return m_Size.x;
    }

    void SetVerticeCount( int aCount )
    {
        if( aCount < 3 )
            aCount = 3;
        else if( aCount > 12 )
            aCount = 12;

        m_Type = (APERTURE_TYPE)(AT_REGULAR_POLY3 - 3 + aCount);
    }

    int GetVerticeCount()
    {
        return m_Type - AT_REGULAR_POLY3 + 3;
    }

    void SetRotation( double aRotDegree )
    {
        // The rotation is stored in 1/1000 degree
        m_Size.y = int( aRotDegree * 1000.0 );
    }

    double GetRotation()
    {
        // The rotation is stored in 1/1000 degree
        return m_Size.y / 1000.0;
    }

    // Type ( Line, rect , circulaire , ovale poly 3 to 12 vertices )
    APERTURE_TYPE m_Type;

    // horiz and Vert size, or diameter and rotation for regular polygon
    // The diameter (for  circle and polygons) is stored in m_Size.x
    // the rotation is stored in m_Size.y in 1/1000 degree
    wxSize        m_Size;

    // code number ( >= 10 )
    int           m_DCode;

    // the attribute attached to this aperture
    // Only one attribute is allowed by aperture
    // 0 = no specific aperture attribute
    int           m_ApertureAttribute;
};
