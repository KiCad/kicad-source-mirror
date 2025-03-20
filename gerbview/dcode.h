/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Jean-Pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

/**
 * @file dcode.h
 */

#ifndef _DCODE_H_
#define _DCODE_H_

#include <vector>

#include <gal/color4d.h>
#include <geometry/shape_poly_set.h>

using KIGFX::COLOR4D;

class wxDC;
class GERBER_DRAW_ITEM;


/**
 * The set of all gerber aperture types allowed
 * from ADD dcode command, like %ADD11C,0.304800*% to add a DCode number 11, circle shape
 */
enum APERTURE_T {
    APT_CIRCLE  = 'C',      // Flashed shape: Circle with or without hole
    APT_RECT    = 'R',      // Flashed shape: Rectangle with or without hole
    APT_OVAL    = '0',      // Flashed shape: Oval with or without hole
    APT_POLYGON = 'P',      // Flashed shape: Regular polygon (3 to 12 edges)
                            // with or without hole. Can be rotated
    APT_MACRO   = 'M'       // Complex shape given by a macro definition (see AM_PRIMITIVE_ID)
};

// In aperture definition, round, oval and rectangular flashed shapes
// can have a hole (round or rectangular)
// this option is stored in .m_DrillShape D_CODE member
enum APERTURE_DEF_HOLETYPE {
    APT_DEF_NO_HOLE = 0,
    APT_DEF_ROUND_HOLE,
    APT_DEF_RECT_HOLE
};

/* define min and max values for D Codes values.
 * note: values >= 0 and < FIRST_DCODE can be used for special purposes (plot commands)
 * Revision I1 permits apertures up to 2^31-1.
 */
#define FIRST_DCODE     10
#define LAST_DCODE      0x7FFFFFFF

class APERTURE_MACRO;


/**
 * A gerber DCODE (also called Aperture) definition.
 */
class D_CODE
{
public:
    D_CODE( int num_dcode );
    ~D_CODE();

    /**
     * @return true if aDcodeValue is valid ( >= FIRST_DCODE. )
     * Any value > 0x7FFFFFFF is a negative value for a int and is not acceptable
     * and is < FIRST_DCODE.
     */
    static bool IsValidDcodeValue( int aDcodeValue )
    {
        return aDcodeValue >= FIRST_DCODE;
    }

    void Clear_D_CODE_Data();

    /**
     * Add a parameter to the D_CODE parameter list.
     *
     * Used to customize the corresponding aperture macro.
     */
    void AppendParam( double aValue )
    {
        m_am_params.push_back( aValue );
    }

    /**
     * Return the number of parameters stored in parameter list.
     */
    unsigned GetParamCount() const
    {
       return  m_am_params.size();
    }

    /**
     * Return a parameter stored in parameter list.
     *
     * @param aIdx is the index of parameter.
     * for n parameters from the Dcode definition, aIdx = 1 .. n, not 0
     */
    double GetParam( unsigned aIdx ) const
    {
        wxASSERT( aIdx <= m_am_params.size() );

        if( aIdx <= m_am_params.size() )
            return  m_am_params[aIdx - 1];
        else
            return 0;
    }

    void SetMacro( APERTURE_MACRO* aMacro )
    {
        m_Macro = aMacro;
    }

    APERTURE_MACRO* GetMacro() const { return m_Macro; }

    /**
     * Return a character string telling what type of aperture type \a aType is.
     *
     * @param aType is the aperture type to show.
     */
    static const wxChar* ShowApertureType( APERTURE_T aType );

    /**
     * Draw the dcode shape for flashed items.
     *
     * When an item is flashed, the DCode shape is the shape of the item.
     *
     * @param aParent is the #GERBER_DRAW_ITEM being drawn.
     * @param aDC is the device context.
     * @param aColor is the normal color to use.
     * @param aShapePos is the actual shape position
     * @param aFilledShape set to true to draw in filled mode, false to draw in sketch mode
     */
    void DrawFlashedShape( const GERBER_DRAW_ITEM* aParent, wxDC* aDC,
                           const COLOR4D& aColor,
                           const VECTOR2I& aShapePos, bool aFilledShape );

    /**
     * A helper function used to draw the polygon stored in m_PolyCorners.
     *
     * Draw some Apertures shapes when they are defined as filled polygons. APT_POLYGON is
     * always a polygon, but some complex shapes are also converted to polygons (shapes with
     * holes, some rotated shapes).
     *
     * @param aParent is the #GERBER_DRAW_ITEM being drawn.
     * @param aDC is the device context.
     * @param aColor is the normal color to use.
     * @param aFilled set to true to draw in filled mode, false to draw in sketch mode.
     * @param aPosition is the actual shape position.
     */
    void DrawFlashedPolygon( const GERBER_DRAW_ITEM* aParent, wxDC* aDC,
                             const COLOR4D& aColor,
                             bool aFilled, const VECTOR2I& aPosition );

    /**
     * Convert a shape to an equivalent polygon.
     *
     * Arcs and circles are approximated by segments.  Useful when a shape is not a graphic
     * primitive (shape with hole, rotated shape ... ) and cannot be easily drawn.
     * @param aParent is the #GERBER_DRAW_ITEM using this DCode.
     * Not used in all shapes, used for APT_MACRO
     */
    void ConvertShapeToPolygon( const GERBER_DRAW_ITEM* aParent );

    /**
     * Calculate a value that can be used to evaluate the size of text when displaying the
     * D-Code of an item.
     *
     * Due to the complexity of some shapes, one cannot calculate the "size" of a shape (only
     * a bounding box) but here, the "dimension" of the shape is the diameter of the primitive
     * or for lines the width of the line if the shape is a line.
     *
     * @param aParent is the parent GERBER_DRAW_ITEM which is actually drawn.
     * @return a dimension, or -1 if no dim to calculate.
     */
    int GetShapeDim( GERBER_DRAW_ITEM* aParent );

public:
    VECTOR2I              m_Size;           ///< Horizontal and vertical dimensions.
    APERTURE_T            m_ApertType;      ///< Aperture type ( Line, rectangle, circle,
                                            ///< oval poly, macro )
    int                   m_Num_Dcode;      ///< D code value ( >= 10 )
    VECTOR2I              m_Drill;          ///< dimension of the hole (if any) (drill file)
    APERTURE_DEF_HOLETYPE m_DrillShape;     ///< shape of the hole (0 = no hole, round = 1,
                                            ///< rect = 2).
    EDA_ANGLE             m_Rotation;       ///< shape rotation
    int                   m_EdgesCount;     ///< in aperture definition Polygon only:
                                            ///< number of edges for the polygon
    bool                  m_InUse;          ///< false if the aperture (previously defined)
                                            ///< is not used to draw something
    bool                  m_Defined;        ///< false if the aperture is not defined in the header
    wxString              m_AperFunction;   ///< the aperture attribute (created by a
                                            ///< %TA.AperFunction command).
                                            ///< attached to the D_CODE
    SHAPE_POLY_SET        m_Polygon;        /* Polygon used to draw APT_POLYGON shape and some other
                                             * complex shapes which are converted to polygon
                                             * (shapes with hole )
                                             */

private:
    APERTURE_MACRO* m_Macro;    ///< no ownership, points to GERBER.m_aperture_macros element.

    /**
     * parameters used only when this D_CODE holds a reference to an aperture
     * macro, and these parameters would customize the macro.
     */
    std::vector<double>   m_am_params;
};


#endif  // ifndef _DCODE_H_
