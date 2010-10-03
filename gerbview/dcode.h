/**************/
/* dcode.h */
/**************/

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Jean-Pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2010 Kicad Developers, see change_log.txt for contributors.
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

#ifndef _DCODE_H_
#define _DCODE_H_

#include <vector>
#include <set>

#include "base_struct.h"
class GERBER_DRAW_ITEM;

/**
 * Enum APERTURE_T
 * is the set of all gerber aperture types allowed, according to page 16 of
 * http://gerbv.sourceforge.net/docs/rs274xrevd_e.pdf
 */
enum APERTURE_T {
    APT_CIRCLE  = 'C',      // Flashed shape: Circle with or without hole
    APT_LINE    = 'L',      // tool to draw line. Not used to flash items
    APT_RECT    = 'R',      // Flashed shape: Rectangle with or without hole
    APT_OVAL    = '0',      // Flashed shape: Oval with or without hole
    APT_POLYGON = 'P',      // Flashed shape: Regular polygon (3 to 12 edges)
                            // with or without hole. Can be rotated
    APT_MACRO   = 'M'       // Complex shape given by a macro definition (see AM_PRIMITIVE_ID)
};

// In aperture definition, round, oval and rectangular flashed shapes
// can have a hole (ropund or rectangular)
// this option is stored in .m_DrillShape D_CODE member
enum APERTURE_DEF_HOLETYPE {
    APT_DEF_NO_HOLE = 0,
    APT_DEF_ROUND_HOLE,
    APT_DEF_RECT_HOLE
};

/* define min and max values for D Codes values.
 * note: values >= 0 and > FIRST_DCODE can be used for specila purposes
 */
#define FIRST_DCODE     10
#define LAST_DCODE      999
#define TOOLS_MAX_COUNT (LAST_DCODE + 1)

class APERTURE_MACRO;
class D_CODE;


/**
 * Class DCODE_PARAM
 * holds a parameter for a DCODE or an "aperture macro" as defined within
 * standard RS274X.  The \a value field can be a constant, i.e. "immediate"
 * parameter or it may not be used if this param is going to defer to the
 * referencing aperture macro.  In that case, the \a index field is an index
 * into the aperture macro's parameters.
 */
class DCODE_PARAM
{
public:
    DCODE_PARAM() :
        index( -1 ),
        value( 0.0 )
    {}

    double GetValue( const D_CODE* aDcode ) const;

    void SetValue( double aValue )
    {
        value = aValue;
        index = -1;
    }


    /**
     * Function IsImmediate
     * tests if this DCODE_PARAM holds an immediate parameter or is a pointer
     * into a parameter held by an owning D_CODE.
     */
    bool IsImmediate() const { return index == -1; }

    unsigned GetIndex() const
    {
        return (unsigned) index;
    }


    void SetIndex( int aIndex )
    {
        index = aIndex;
    }


private:
    int    index;       ///< if -1, then \a value field is an immediate value,
                        //   else this is an index into parent's
                        //   D_CODE.m_am_params.
    double value;       ///< if IsImmediate()==true then use the value, else
                        //   not used.
};

typedef std::vector<DCODE_PARAM> DCODE_PARAMS;


/**
 * Class D_CODE
 * holds a gerber DCODE definition.
 */
class D_CODE
{
    friend class DCODE_PARAM;

    APERTURE_MACRO* m_Macro;    ///< no ownership, points to
                                //   GERBER.m_aperture_macros element

    /**
     * parameters used only when this D_CODE holds a reference to an aperture
     * macro, and these parameters would customize the macro.
     */
    DCODE_PARAMS          m_am_params;

    std::vector <wxPoint> m_PolyCorners;    /* Polygon used to draw APT_POLYGON shape and some other
                                             * complex shapes which are converted to polygon
                                             * (shapes with hole )
                                             */

public:
    wxSize                m_Size;           /* Horizontal and vertical dimensions. */
    APERTURE_T            m_Shape;          /* shape ( Line, rectangle, circle , oval .. ) */
    int                   m_Num_Dcode;      /* D code ( >= 10 ) */
    wxSize                m_Drill;          /* dimension of the hole (if any) */
    APERTURE_DEF_HOLETYPE m_DrillShape;     /* shape of the hole (0 = no hole, round = 1, rect = 2) */
    double                m_Rotation;       /* shape rotation in degrees */
    int                   m_EdgesCount;     /* in apeture definition Polygon only: number of edges for the polygon */
    bool                  m_InUse;          /* FALSE if not used */
    bool                  m_Defined;        /* FALSE if not defined */
    wxString              m_SpecialDescr;

public:
    D_CODE( int num_dcode );
    ~D_CODE();
    void Clear_D_CODE_Data();

    void AppendParam( double aValue )
    {
        DCODE_PARAM param;

        param.SetValue( aValue );

        m_am_params.push_back( param );
    }


    void SetMacro( APERTURE_MACRO* aMacro )
    {
        m_Macro = aMacro;
    }


    APERTURE_MACRO* GetMacro() { return m_Macro; }

    /**
     * Function ShowApertureType
     * returns a character string telling what type of aperture type \a aType is.
     * @param aType The aperture type to show.
     */
    static const wxChar* ShowApertureType( APERTURE_T aType );

    /** function DrawFlashedShape
     * Draw the dcode shape for flashed items.
     * When an item is flashed, the DCode shape is the shape of the item
     * @param aClipBox = DC clip box (NULL is no clip)
     * @param aDC = device context
     * @param aColor = the normal color to use
     * @param aAltColor = the color used to draw with "reverse" exposure mode (used in aperture macros only)
     * @param aFilled = true to draw in filled mode, false to draw in skecth mode
     * @param aPosition = the actual shape position
     */
    void                 DrawFlashedShape(  GERBER_DRAW_ITEM* aParent,
                                            EDA_Rect* aClipBox, wxDC* aDC, int aColor, int aAltColor,
                                            wxPoint aShapePos, bool aFilledShape );

    /** function DrawFlashedPolygon
     * a helper function used id ::Draw to draw the polygon stored ion m_PolyCorners
     * Draw some Apertures shapes when they are defined as filled polygons.
     * APT_POLYGON is always a polygon, but some complex shapes are also converted to
     * polygons (shapes with holes, some rotated shapes)
     * @param aClipBox = DC clip box (NULL is no clip)
     * @param aDC = device context
     * @param aColor = the normal color to use
     * @param aFilled = true to draw in filled mode, false to draw in skecth mode
     * @param aPosition = the actual shape position
    */
    void                 DrawFlashedPolygon( EDA_Rect* aClipBox, wxDC* aDC, int aColor,
                                             bool aFilled, const wxPoint& aPosition );

    /** function ConvertShapeToPolygon
     * convert a shape to an equivalent polygon.
     * Arcs and circles are approximated by segments
     * Useful when a shape is not a graphic primitive (shape with hole,
     * rotated shape ... ) and cannot be easily drawn.
     */
    void                 ConvertShapeToPolygon();
};


inline double DCODE_PARAM::GetValue( const D_CODE* aDcode ) const
{
    if( IsImmediate() )
        return value;
    else
    {
        // the first one was numbered 1, not zero, as in $1, see page 19 of spec.
        unsigned ndx = GetIndex() - 1;
        wxASSERT( aDcode );

        // get the parameter from the aDcode
        if( ndx < aDcode->m_am_params.size() )
            return aDcode->m_am_params[ndx].GetValue( NULL );
        else
        {
            wxASSERT( GetIndex() - 1 < aDcode->m_am_params.size() );
            return 0.0;
        }
    }
}


#endif  // ifndef _DCODE_H_
