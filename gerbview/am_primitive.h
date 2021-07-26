/**
 * @file aperture_macro.h
 */

#ifndef AM_PRIMITIVE_H
#define AM_PRIMITIVE_H

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Jean-Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <vector>
#include <set>

#include <am_param.h>
#include <eda_rect.h>

class SHAPE_POLY_SET;

/*
 *  An aperture macro defines a complex shape and is a list of aperture primitives.
 *  Each aperture primitive defines a simple shape (circle, rect, regular polygon...)
 *  Inside a given aperture primitive, a fixed list of parameters defines info
 *  about the shape: size, thickness, number of vertex ...
 *
 *  Each parameter can be an immediate value or a deferred value.
 *  When value is deferred, it is defined when the aperture macro is instanced by
 *  an ADD macro command
 *  Note also a deferred parameter can be defined in aperture macro,
 *  but outside aperture primitives. Example
 *  %AMRECTHERM*
 *  $4=$3/2*    parameter $4 is half value of parameter $3
 *  21,1,$1-$3,$2-$3,0-$1/2-$4,0-$2/2-$4,0*
 *  For the aperture primitive, parameters $1 to $3 will be defined in ADD command,
 *  and $4 is defined inside the macro
 *
 *  Each basic shape can be a positive shape or a negative shape.
 *  a negative shape is "local" to the whole shape.
 *  It must be seen like a hole in the shape, and not like a standard negative object.
 */

/**
 * The set of all "aperture macro primitives" (primitive numbers).
 *
 * See Table 3 in http://gerbv.sourceforge.net/docs/rs274xrevd_e.pdf aperture macro primitives
 * are basic shapes which can be combined to create a complex shape.  This complex shape is
 * flashed.
 */
enum AM_PRIMITIVE_ID {
    AMP_UNKNOWN = -1,           // A value for uninitialized AM_PRIMITIVE.
    AMP_COMMENT = 0,            // A primitive description is not really a primitive, this is a
                                // comment
    AMP_CIRCLE  = 1,            // Circle. (diameter and position)
    AMP_LINE2   = 2,            // Line with rectangle ends. (Width, start and end pos + rotation)
    AMP_LINE20  = 20,           // Same as AMP_LINE2
    AMP_LINE_CENTER = 21,       // Rectangle. (height, width and center pos + rotation)
    AMP_LINE_LOWER_LEFT = 22,   // Rectangle. (height, width and left bottom corner pos + rotation)
    AMP_EOF     = 3,            // End Of File marker: not really a shape
    AMP_OUTLINE = 4,            // Free polyline (n corners + rotation)
    AMP_POLYGON = 5,            // Closed regular polygon(diameter, number of vertices (3 to 10),
                                // rotation)
    AMP_MOIRE   = 6,            // A cross hair with n concentric circles + rotation
    AMP_THERMAL = 7             // Thermal shape (pos, outer and inner diameter, cross hair
                                // thickness + rotation)
};


/**
 * An aperture macro primitive as given in Table 3 of
 * http://gerbv.sourceforge.net/docs/rs274xrevd_e.pdf
 */
class AM_PRIMITIVE
{
public:
    AM_PRIMITIVE_ID primitive_id;       ///< The primitive type
    AM_PARAMS       params;             ///< A sequence of parameters used by
                                        //   the primitive
    bool            m_GerbMetric;       // units for this primitive:
                                        // false = Inches, true = metric

    AM_PRIMITIVE( bool aGerbMetric, AM_PRIMITIVE_ID aId = AMP_UNKNOWN )
    {
        primitive_id = aId;
        m_GerbMetric = aGerbMetric;
    }


    ~AM_PRIMITIVE() {}

    /**
     * @return true if the first parameter is not 0 (it can be only 0 or 1).
     * Some but not all primitives use the first parameter as an exposure control.
     * Others are always ON.
     * In a aperture macro shape, a basic primitive with exposure off is a hole in the shape
     * it is NOT a negative shape
     */
    bool  IsAMPrimitiveExposureOn( const GERBER_DRAW_ITEM* aParent ) const;

    /* Draw functions: */

    /**
     * Calculate a value that can be used to evaluate the size of text when displaying the
     * D-Code of an item.
     *
     * Due to the complexity of the shape of some primitives one cannot calculate the "size"
     * of a shape (only a bounding box) but here, the "dimension" of the shape is the diameter
     * of the primitive or for lines the width of the line.
     *
     * @param aParent is the parent GERBER_DRAW_ITEM which is actually drawn
     * @return a dimension, or -1 if no dim to calculate
     */
    int  GetShapeDim( const GERBER_DRAW_ITEM* aParent );

    /**
     * Draw (in fact generate the actual polygonal shape of) the primitive shape of an aperture
     * macro instance.
     *
     * @param aParent is the parent GERBER_DRAW_ITEM which is actually drawn.
     * @param aShapeBuffer is a SHAPE_POLY_SET to put the shape converted to a polygon.
     * @param aShapePos is the actual shape position.
     */
    void DrawBasicShape( const GERBER_DRAW_ITEM* aParent,
                         SHAPE_POLY_SET& aShapeBuffer,
                         const wxPoint& aShapePos );

private:
    /**
     * Convert a shape to an equivalent polygon.
     *
     * Arcs and circles are approximated by segments.  Useful when a shape is not a graphic
     * primitive (shape with hole, rotated shape ... ) and cannot be easily drawn.
     *
     * @note Some schapes conbining circles and solid lines (rectangles), only rectangles are
     *       converted because circles are very easy to draw (no rotation problem) so convert
     *       them in polygons and draw them as polygons is not a good idea.
     */
    void ConvertShapeToPolygon( const GERBER_DRAW_ITEM* aParent, std::vector<wxPoint>& aBuffer );
};


typedef std::vector<AM_PRIMITIVE> AM_PRIMITIVES;

/**
 * Support the "aperture macro" defined within standard RS274X.
 */
struct APERTURE_MACRO
{
    /**
     * Usually, parameters are defined inside the aperture primitive using immediate mode or
     * deferred mode.
     *
     * In deferred mode the value is defined in a DCODE that want to use the aperture macro.
     * Some parameters are defined outside the aperture primitive and are local to the aperture
     * macro.
     *
     * @return the value of a deferred parameter defined inside the aperture macro.
     * @param aDcode is the D_CODE that uses this aperture macro and define deferred parameters.
     * @param aParamId is the param id (defined by $3 or $5 ..) to evaluate.
     */
    double GetLocalParam( const D_CODE* aDcode, unsigned aParamId ) const;


    /**
     * Calculate the primitive shape for flashed items.
     *
     * When an item is flashed, this is the shape of the item.
     *
     * @param aParent is the parent #GERBER_DRAW_ITEM which is actually drawn.
     * @return the shape of the item.
     */
    SHAPE_POLY_SET* GetApertureMacroShape( const GERBER_DRAW_ITEM* aParent,
                                           const wxPoint& aShapePos );

   /**
     * Draw the primitive shape for flashed items.
     *
     * When an item is flashed, this is the shape of the item.
     *
     * @param aParent is the parent GERBER_DRAW_ITEM which is actually drawn.
     * @param aClipBox is DC clip box (NULL is no clip).
     * @param aDC is the device context.
     * @param aColor is the color of shape.
     * @param aShapePos is the actual shape position.
     * @param aFilledShape set to true to draw in filled mode, false to draw in sketch mode.
     */
    void DrawApertureMacroShape( GERBER_DRAW_ITEM* aParent, EDA_RECT* aClipBox, wxDC* aDC,
                                 const COLOR4D& aColor, const wxPoint& aShapePos,
                                 bool aFilledShape );

    /**
     * Calculate a value that can be used to evaluate the size of text when displaying the
     * D-Code of an item.
     *
     * Due to the complexity of a shape using many primitives one cannot calculate the "size" of
     * a shape (only abounding box) but most of aperture macro are using one or few primitives
     * and the "dimension" of the shape is the diameter of the primitive (or the max diameter of
     * primitives).
     *
     * @param aParent is the parent #GERBER_DRAW_ITEM which is actually drawn.
     * @return a dimension, or -1 if no dim to calculate.
     */
    int  GetShapeDim( GERBER_DRAW_ITEM* aParent );

    /// Return the bounding box of the shape.
    EDA_RECT GetBoundingBox() const
    {
        return m_boundingBox;
    }

    wxString      name;             ///< The name of the aperture macro
    AM_PRIMITIVES primitives;       ///< A sequence of AM_PRIMITIVEs

    /*  A deferred parameter can be defined in aperture macro,
     *  but outside aperture primitives. Example
     *  %AMRECTHERM*
     *  $4=$3/2*    parameter $4 is half value of parameter $3
     * m_localparamStack handle a list of local deferred parameters
     */
    AM_PARAMS m_localparamStack;

    SHAPE_POLY_SET m_shape;     ///< The shape of the item, calculated by GetApertureMacroShape
    EDA_RECT m_boundingBox;     ///< The bounding box of the item, calculated by
                                ///< GetApertureMacroShape.

};


/**
 * Used by std:set<APERTURE_MACRO> instantiation which uses APERTURE_MACRO.name as its key.
 */
struct APERTURE_MACRO_less_than
{
    // a "less than" test on two APERTURE_MACROs (.name wxStrings)
    bool operator()( const APERTURE_MACRO& am1, const APERTURE_MACRO& am2 ) const
    {
        return am1.name.Cmp( am2.name ) < 0;  // case specific wxString compare
    }
};


/**
 * A sorted collection of APERTURE_MACROS whose key is the name field in the APERTURE_MACRO.
 */
typedef std::set<APERTURE_MACRO, APERTURE_MACRO_less_than> APERTURE_MACRO_SET;
typedef std::pair<APERTURE_MACRO_SET::iterator, bool>      APERTURE_MACRO_SET_PAIR;


#endif  // ifndef AM_PRIMITIVE_H
