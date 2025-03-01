/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
 *
 * Other aperture types are aperture macros
 */
#define FIRST_DCODE_VALUE 10    // D_CODE < 10 is a command, D_CODE >= 10 is a tool

class APERTURE
{
public:
    enum APERTURE_TYPE {
        AT_CIRCLE   = 1,        // round aperture, to flash pads
        AT_RECT     = 2,        // rect aperture, to flash pads
        AT_PLOTTING = 3,        // round aperture, to plot lines
        AT_OVAL     = 4,        // oval aperture, to flash pads
        AT_REGULAR_POLY = 5,    // Regular polygon (n vertices, n = 3 .. 12, with rotation)
        AT_REGULAR_POLY3,       // Regular polygon 3 vertices, with rotation
        AT_REGULAR_POLY4,       // Regular polygon 4 vertices, with rotation
        AT_REGULAR_POLY5,       // Regular polygon 5 vertices, with rotation
        AT_REGULAR_POLY6,       // Regular polygon 6 vertices, with rotation
        AT_REGULAR_POLY7,       // Regular polygon 7 vertices, with rotation
        AT_REGULAR_POLY8,       // Regular polygon 8 vertices, with rotation
        AT_REGULAR_POLY9,       // Regular polygon 9 vertices, with rotation
        AT_REGULAR_POLY10,      // Regular polygon 10 vertices, with rotation
        AT_REGULAR_POLY11,      // Regular polygon 11 vertices, with rotation
        AT_REGULAR_POLY12,      // Regular polygon 12 vertices, with rotation
        AM_ROUND_RECT,          // Aperture macro for round rect pads
        AM_ROT_RECT,            // Aperture macro for rotated rect pads
        APER_MACRO_OUTLINE4P,   // Aperture macro for trapezoid pads (outline with 4 corners)
        APER_MACRO_OUTLINE5P,   // Aperture macro for pad polygons with 5 corners (chamfered pads)
        APER_MACRO_OUTLINE6P,   // Aperture macro for pad polygons with 6 corners (chamfered pads)
        APER_MACRO_OUTLINE7P,   // Aperture macro for pad polygons with 7 corners (chamfered pads)
        APER_MACRO_OUTLINE8P,   // Aperture macro for pad polygons with 8 corners (chamfered pads)
        AM_ROTATED_OVAL,        // Aperture macro for rotated oval pads
                                // (not rotated uses a primitive)
        AM_FREE_POLYGON         // Aperture macro to create on the fly a free polygon, with
                                // only one parameter: rotation
    };

    void SetSize( const VECTOR2I& aSize )
    {
        m_Size = aSize;
    }

    const VECTOR2I GetSize()
    {
        return m_Size;
    }

    void SetDiameter( int aDiameter )
    {
        m_Radius = aDiameter/2;
    }

    int GetDiameter()
    {
        // For round primitive, the diameter is the m_Size.x ot m_Size.y
        if( m_Type == AT_CIRCLE || m_Type == AT_PLOTTING )
            return m_Size.x;

        // For rounded shapes (macro apertures), return m_Radius * 2
        // but usually they use the radius (m_Radius)
        return m_Radius*2;
    }

    void SetRegPolyVerticeCount( int aCount )
    {
        if( aCount < 3 )
            aCount = 3;
        else if( aCount > 12 )
            aCount = 12;

        m_Type = (APERTURE_TYPE)(AT_REGULAR_POLY3 - 3 + aCount);
    }

    int GetRegPolyVerticeCount()
    {
        return m_Type - AT_REGULAR_POLY3 + 3;
    }

    void SetRotation( const EDA_ANGLE& aRotation ) { m_Rotation = aRotation; }
    EDA_ANGLE GetRotation() { return m_Rotation; }

    // Type ( Line, rect , circulaire , ovale poly 3 to 12 vertices, aperture macro )
    APERTURE_TYPE m_Type;

    // horiz and Vert size
    VECTOR2I        m_Size;

    // list of corners for polygon shape
    std::vector<VECTOR2I> m_Corners;

    // Radius for polygon and round rect shape
    int           m_Radius;

    // Rotation in degrees
    EDA_ANGLE     m_Rotation;

    // code number ( >= 10 )
    int           m_DCode;

    // the attribute attached to this aperture
    // Only one attribute is allowed by aperture
    // 0 = no specific aperture attribute
    int           m_ApertureAttribute;

    std::string m_CustomAttribute;
};


/** A class to define an aperture macros based on a free polygon, i.e. using a
 * primitive 4 to describe a free polygon with a rotation.
 * the aperture macro has only one parameter: rotation and is defined on the fly
 * for  aGerber file
 */
class APER_MACRO_FREEPOLY
{
public:
    APER_MACRO_FREEPOLY( const std::vector<VECTOR2I>& aPolygon, int aId )
    {
        m_Corners = aPolygon;
        m_Id = aId;
    }

    /**
     * @return true if aPolygon is the same as this, i.e. if the
     * aPolygon is the same as m_Corners
     * @param aOther is the candidate to compare
     */
    bool IsSamePoly( const std::vector<VECTOR2I>& aPolygon ) const;

    /**
     * print the aperture macro definition to aOutput
     * @param aOutput is the FILE to write
     * @param aIu2GbrMacroUnit is the scaling factor from coordinates value to
     * the Gerber file macros units (always mm or inches)
     */
    void Format( FILE * aOutput, double aIu2GbrMacroUnit );

    int CornersCount() const { return (int)m_Corners.size(); }

    std::vector<VECTOR2I> m_Corners;
    int m_Id;
};


class APER_MACRO_FREEPOLY_LIST
{
public:
    APER_MACRO_FREEPOLY_LIST() {}

    void ClearList() { m_AMList.clear(); }

    int AmCount() const { return (int)m_AMList.size(); }

    /**
     * append a new APER_MACRO_FREEPOLY containing the polygon aPolygon to the current list
     */
    void Append( const std::vector<VECTOR2I>& aPolygon );

    /**
     * @return the index in m_AMList of the APER_MACRO_FREEPOLY having the
     * same polygon as aPolygon, or -1
     * @param aCandidate is the polygon candidate to compare
     */
    int FindAm( const std::vector<VECTOR2I>& aPolygon ) const;

    /**
     * print the aperture macro list to aOutput
     * @param aOutput is the FILE to write
     * @param aIu2GbrMacroUnit is the scaling factor from coordinates value to
     * the Gerber file macros units (always mm or inches)
     */
    void Format( FILE * aOutput, double aIu2GbrMacroUnit );

    std::vector<APER_MACRO_FREEPOLY> m_AMList;
};
