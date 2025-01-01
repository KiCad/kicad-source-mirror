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

#include <vector>
#include <set>

#include <am_param.h>

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
    AMP_OUTLINE = 4,            // Free polyline (n corners + rotation)
    AMP_POLYGON = 5,            // Closed regular polygon(diameter, number of vertices (3 to 10),
                                // rotation)
    AMP_MOIRE   = 6,            // A cross hair with n concentric circles + rotation (deprecated in 2021)
    AMP_THERMAL = 7             // Thermal shape (pos, outer and inner diameter, cross hair
                                // thickness + rotation)
};


/**
 * An aperture macro primitive as given in gerber layer format doc. See
 * https://www.ucamco.com/en/news/gerber-layer-format-specification-revision-????
 */
class AM_PRIMITIVE
{
public:
    AM_PRIMITIVE_ID m_Primitive_id;     ///< The primitive type
    AM_PARAMS       m_Params;           ///< A sequence of parameters used by the primitive
    bool            m_GerbMetric;       // units for this primitive:
                                        // false = Inches, true = metric
    int             m_LocalParamLevel;  // count of local param defined inside a aperture macro
                                        // local param stack when this primitive is put in
                                        // aperture macro primitive stack list
                                        // not used outside a aperture macro

    AM_PRIMITIVE( bool aGerbMetric, AM_PRIMITIVE_ID aId = AMP_UNKNOWN )
    {
        m_Primitive_id = aId;
        m_GerbMetric = aGerbMetric;
        m_LocalParamLevel = 0;
    }


    ~AM_PRIMITIVE() {}

    /**
     * @return true if the first parameter is not 0 (it can be only 0 or 1).
     * Some but not all primitives use the first parameter as an exposure control.
     * Others are always ON.
     * In a aperture macro shape, a basic primitive with exposure off is a hole in the shape
     * it is NOT a negative shape
     */
    bool  IsAMPrimitiveExposureOn( APERTURE_MACRO* aApertMacro ) const;

    /**
     * Generate the polygonal shape of the primitive shape of an aperture
     * macro instance.
     *
     * @param aApertMacro is the aperture macro using this primitive.
     * @param aShapeBuffer is a SHAPE_POLY_SET to put the shape converted to a polygon.
     */
#if 0
    void ConvertBasicShapeToPolygon( const D_CODE* aDcode,
                                     SHAPE_POLY_SET& aShapeBuffer );
#endif
    void ConvertBasicShapeToPolygon( APERTURE_MACRO* aApertMacro,
                                     SHAPE_POLY_SET& aShapeBuffer );

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
    //void ConvertShapeToPolygon( const D_CODE* aDcode, std::vector<VECTOR2I>& aBuffer );
    void ConvertShapeToPolygon( APERTURE_MACRO* aApertMacroe, std::vector<VECTOR2I>& aBuffer );
};


#endif  // ifndef AM_PRIMITIVE_H
