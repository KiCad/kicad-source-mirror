/**
 * @file class_aperture_macro.h
 */

#ifndef _APERTURE_MACRO_H_
#define _APERTURE_MACRO_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Jean-Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2010 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <base_struct.h>
#include <class_am_param.h>

class SHAPE_POLY_SET;

/*
 *  An aperture macro defines a complex shape and is a list of aperture primitives.
 *  Each aperture primitive defines a simple shape (circle, rect, regular polygon...)
 *  Inside a given aperture primitive, a fixed list of parameters defines info
 *  about the shape: size, thickness, number of vertex ...
 *
 *  Each parameter can be an immediate value or a defered value.
 *  When value is defered, it is defined when the aperture macro is instancied by
 *  an ADD macro command
 *  Note also a defered parameter can be defined in aperture macro,
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
 * Enum AM_PRIMITIVE_ID
 * is the set of all "aperture macro primitives" (primitive numbers).  See
 * Table 3 in http://gerbv.sourceforge.net/docs/rs274xrevd_e.pdf
 * aperture macro primitives are basic shapes which can be combined to create a complex shape
 * This complex shape is flashed.
 */
enum AM_PRIMITIVE_ID {
    AMP_UNKNOWN = -1,           // A value for uninitialized AM_PRIMITIVE.
    AMP_COMMENT = 0,            // A primitive description is not really a primitive, this is a comment
    AMP_CIRCLE  = 1,            // Circle. (diameter and position)
    AMP_LINE2   = 2,            // Line with rectangle ends. (Width, start and end pos + rotation)
    AMP_LINE20  = 20,           // Same as AMP_LINE2
    AMP_LINE_CENTER = 21,       // Rectangle. (height, width and center pos + rotation)
    AMP_LINE_LOWER_LEFT = 22,   // Rectangle. (height, width and left bottom corner pos + rotation)
    AMP_EOF     = 3,            // End Of File marquer: not really a shape
    AMP_OUTLINE = 4,            // Free polyline (n corners + rotation)
    AMP_POLYGON = 5,            // Closed regular polygon(diameter, number of vertices (3 to 10), rotation)
    AMP_MOIRE   = 6,            // A cross hair with n concentric circles + rotation
    AMP_THERMAL = 7             // Thermal shape (pos, outer and inner diameter, cross hair thickness + rotation)
};


/**
 * Struct AM_PRIMITIVE
 * holds an aperture macro primitive as given in Table 3 of
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
public: AM_PRIMITIVE( bool aGerbMetric, AM_PRIMITIVE_ID aId = AMP_UNKNOWN )
    {
        primitive_id = aId;
        m_GerbMetric = aGerbMetric;
    }


    ~AM_PRIMITIVE() {}

    /**
     * Function IsAMPrimitiveExposureOn
     * @return true if the first parameter is not 0 (it can be only 0 or 1).
     * Some but not all primitives use the first parameter as an exposure control.
     * Others are always ON.
     * In a aperture macro shape, a basic primitive with exposure off is a hole in the shape
     * it is NOT a negative shape
     */
    bool  IsAMPrimitiveExposureOn( GERBER_DRAW_ITEM* aParent ) const;

    /* Draw functions: */

    /** GetShapeDim
     * Calculate a value that can be used to evaluate the size of text
     * when displaying the D-Code of an item
     * due to the complexity of the shape of some primitives
     * one cannot calculate the "size" of a shape (only a bounding box)
     * but here, the "dimension" of the shape is the diameter of the primitive
     * or for lines the width of the line
     * @param aParent = the parent GERBER_DRAW_ITEM which is actually drawn
     * @return a dimension, or -1 if no dim to calculate
     */
    int  GetShapeDim( GERBER_DRAW_ITEM* aParent );

    /**
     * Function drawBasicShape
     * Draw (in fact generate the actual polygonal shape of) the primitive shape of an aperture macro instance.
     * @param aParent = the parent GERBER_DRAW_ITEM which is actually drawn
     * @param aShapeBuffer = a SHAPE_POLY_SET to put the shape converted to a polygon
     * @param aShapePos = the actual shape position
     */
    void DrawBasicShape( GERBER_DRAW_ITEM* aParent, SHAPE_POLY_SET& aShapeBuffer,
                         wxPoint aShapePos );
private:

    /**
     * Function ConvertShapeToPolygon
     * convert a shape to an equivalent polygon.
     * Arcs and circles are approximated by segments
     * Useful when a shape is not a graphic primitive (shape with hole,
     * rotated shape ... ) and cannot be easily drawn.
     */
    void ConvertShapeToPolygon( GERBER_DRAW_ITEM* aParent, std::vector<wxPoint>& aBuffer );
};


typedef std::vector<AM_PRIMITIVE> AM_PRIMITIVES;

/**
 * Struct APERTURE_MACRO
 * helps support the "aperture macro" defined within standard RS274X.
 */
struct APERTURE_MACRO
{
    wxString      name;             ///< The name of the aperture macro
    AM_PRIMITIVES primitives;       ///< A sequence of AM_PRIMITIVEs

    /*  A defered parameter can be defined in aperture macro,
     *  but outside aperture primitives. Example
     *  %AMRECTHERM*
     *  $4=$3/2*    parameter $4 is half value of parameter $3
     * m_localparamStack handle a list of local defered parameters
     */
    AM_PARAMS m_localparamStack;

    /**
     * function GetLocalParam
     * Usually, parameters are defined inside the aperture primitive
     * using immediate mode or defered mode.
     * in defered mode the value is defined in a DCODE that want to use the aperture macro.
     * But some parameters are defined outside the aperture primitive
     * and are local to the aperture macro
     * @return the value of a defered parameter defined inside the aperture macro
     * @param aDcode = the D_CODE that uses this apertur macro and define defered parameters
     * @param aParamId = the param id (defined by $3 or $5 ..) to evaluate
     */
    double GetLocalParam( const D_CODE* aDcode, unsigned aParamId ) const;

   /**
     * Function DrawApertureMacroShape
     * Draw the primitive shape for flashed items.
     * When an item is flashed, this is the shape of the item
     * @param aParent = the parent GERBER_DRAW_ITEM which is actually drawn
     * @param aClipBox = DC clip box (NULL is no clip)
     * @param aDC = device context
     * @param aColor = the color of shape
     * @param aShapePos = the actual shape position
     * @param aFilledShape = true to draw in filled mode, false to draw in skecth mode
     */
    void DrawApertureMacroShape( GERBER_DRAW_ITEM* aParent, EDA_RECT* aClipBox, wxDC* aDC,
                                 COLOR4D aColor, wxPoint aShapePos, bool aFilledShape );

    /**
     * Function GetShapeDim
     * Calculate a value that can be used to evaluate the size of text
     * when displaying the D-Code of an item
     * due to the complexity of a shape using many primitives
     * one cannot calculate the "size" of a shape (only abounding box)
     * but most of aperture macro are using one or few primitives
     * and the "dimension" of the shape is the diameter of the primitive
     * (or the max diameter of primitives)
     * @param aParent = the parent GERBER_DRAW_ITEM which is actually drawn
     * @return a dimension, or -1 if no dim to calculate
     */
    int  GetShapeDim( GERBER_DRAW_ITEM* aParent );
};


/**
 * Struct APERTURE_MACRO_less_than
 * is used by std:set<APERTURE_MACRO> instantiation which uses
 * APERTURE_MACRO.name as its key.
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
 * Type APERTURE_MACRO_SET
 * is a sorted collection of APERTURE_MACROS whose key is the name field in
 * the APERTURE_MACRO.
 */
typedef std::set<APERTURE_MACRO, APERTURE_MACRO_less_than> APERTURE_MACRO_SET;
typedef std::pair<APERTURE_MACRO_SET::iterator, bool>      APERTURE_MACRO_SET_PAIR;


#endif  // ifndef _APERTURE_MACRO_H_
