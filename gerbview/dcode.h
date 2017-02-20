/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Jean-Pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2010 KiCad Developers, see change_log.txt for contributors.
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

#include <base_struct.h>
#include <gal/color4d.h>

using KIGFX::COLOR4D;

class wxDC;
class GERBER_DRAW_ITEM;


/**
 * Enum APERTURE_T
 * is the set of all gerber aperture types allowed, according to page 16 of
 * http://gerbv.sourceforge.net/docs/rs274xrevd_e.pdf
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
 * note: values >= 0 and > FIRST_DCODE can be used for special purposes
 */
#define FIRST_DCODE     10
#define LAST_DCODE      999
#define TOOLS_MAX_COUNT (LAST_DCODE + 1)

struct APERTURE_MACRO;


/**
 * Class D_CODE
 * holds a gerber DCODE (also called Aperture) definition.
 */
class D_CODE
{
private:
    APERTURE_MACRO* m_Macro;    ///< no ownership, points to
                                //   GERBER.m_aperture_macros element
    /**
     * parameters used only when this D_CODE holds a reference to an aperture
     * macro, and these parameters would customize the macro.
     */
    std::vector<double>   m_am_params;

    std::vector <wxPoint> m_PolyCorners;    /* Polygon used to draw APT_POLYGON shape and some other
                                             * complex shapes which are converted to polygon
                                             * (shapes with hole )
                                             */

public:
    wxSize                m_Size;           ///< Horizontal and vertical dimensions.
    APERTURE_T            m_Shape;          ///< shape ( Line, rectangle, circle , oval .. )
    int                   m_Num_Dcode;      ///< D code value ( >= 10 )
    wxSize                m_Drill;          ///< dimension of the hole (if any) (draill file)
    APERTURE_DEF_HOLETYPE m_DrillShape;     ///< shape of the hole (0 = no hole, round = 1, rect = 2) */
    double                m_Rotation;       ///< shape rotation in degrees
    int                   m_EdgesCount;     ///< in aperture definition Polygon only:
                                            ///< number of edges for the polygon
    bool                  m_InUse;          ///< false if the aperure (previously defined)
                                            ///< is not used to draw something
    bool                  m_Defined;        ///< false if the aperture is not defined in the header
    wxString              m_AperFunction;   ///< the aperture attribute (created by a %TA.AperFunction command)
                                            ///< attached to the D_CODE


public:
    D_CODE( int num_dcode );
    ~D_CODE();
    void Clear_D_CODE_Data();

    /**
     * AppendParam()
     * Add a parameter to the D_CODE parameter list.
     * used to customize the corresponding aperture macro
     */
    void AppendParam( double aValue )
    {
        m_am_params.push_back( aValue );
    }

    /**
     * GetParamCount()
     * Returns the number of parameters stored in parameter list.
     */
    unsigned GetParamCount() const
    {
       return  m_am_params.size();
    }

    /**
     * GetParam()
     * Returns a parameter stored in parameter list.
     * @param aIdx = index of parameter
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
     * Function ShowApertureType
     * returns a character string telling what type of aperture type \a aType is.
     * @param aType The aperture type to show.
     */
    static const wxChar* ShowApertureType( APERTURE_T aType );

    /**
     * Function DrawFlashedShape
     * Draw the dcode shape for flashed items.
     * When an item is flashed, the DCode shape is the shape of the item
     * @param aParent = the GERBER_DRAW_ITEM being drawn
     * @param aClipBox = DC clip box (NULL is no clip)
     * @param aDC = device context
     * @param aColor = the normal color to use
     * @param aShapePos = the actual shape position
     * @param aFilledShape = true to draw in filled mode, false to draw in sketch mode
     */
    void DrawFlashedShape( GERBER_DRAW_ITEM* aParent, EDA_RECT* aClipBox,
                           wxDC* aDC, COLOR4D aColor,
                           wxPoint aShapePos, bool aFilledShape );

    /**
     * Function DrawFlashedPolygon
     * a helper function used to draw the polygon stored ion m_PolyCorners
     * Draw some Apertures shapes when they are defined as filled polygons.
     * APT_POLYGON is always a polygon, but some complex shapes are also converted to
     * polygons (shapes with holes, some rotated shapes)
     * @param aParent = the GERBER_DRAW_ITEM being drawn
     * @param aClipBox = DC clip box (NULL is no clip)
     * @param aDC = device context
     * @param aColor = the normal color to use
     * @param aFilled = true to draw in filled mode, false to draw in sketch mode
     * @param aPosition = the actual shape position
     */
    void DrawFlashedPolygon( GERBER_DRAW_ITEM* aParent,
                             EDA_RECT* aClipBox, wxDC* aDC, COLOR4D aColor,
                             bool aFilled, const wxPoint& aPosition );

    /**
     * Function ConvertShapeToPolygon
     * convert a shape to an equivalent polygon.
     * Arcs and circles are approximated by segments
     * Useful when a shape is not a graphic primitive (shape with hole,
     * rotated shape ... ) and cannot be easily drawn.
     */
    void ConvertShapeToPolygon();

    /**
     * Function GetShapeDim
     * calculates a value that can be used to evaluate the size of text
     * when displaying the D-Code of an item
     * due to the complexity of some shapes,
     * one cannot calculate the "size" of a shape (only a bounding box)
     * but here, the "dimension" of the shape is the diameter of the primitive
     * or for lines the width of the line if the shape is a line
     * @param aParent = the parent GERBER_DRAW_ITEM which is actually drawn
     * @return a dimension, or -1 if no dim to calculate
     */
    int GetShapeDim( GERBER_DRAW_ITEM* aParent );
};


#endif  // ifndef _DCODE_H_
