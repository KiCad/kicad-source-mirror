/*****************************/
/*	class_gerber_draw_item.h */
/*****************************/

#ifndef CLASS_GERBER_DRAW_ITEM_H
#define CLASS_GERBER_DRAW_ITEM_H

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 <Jean-Pierre Charras>
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

#include "base_struct.h"
#include "class_board_item.h"
class GERBER_IMAGE;

/* Shapes id for basic shapes ( .m_Shape member ) */
enum Gbr_Basic_Shapes {
    GBR_SEGMENT = 0,        // usual segment : line with rounded ends
    GBR_ARC,                // Arcs (with rounded ends)
    GBR_CIRCLE,             // ring
    GBR_POLYGON,            // polygonal shape
    GBR_SPOT_CIRCLE,        // flashed shape: round shape (can have hole)
    GBR_SPOT_RECT,          // flashed shape: rectangular shape can have hole)
    GBR_SPOT_OVAL,          // flashed shape: oval shape
    GBR_SPOT_POLY,          // flashed shape: regulat polygon, 3 to 12 edges
    GBR_SPOT_MACRO,         // complex shape described by a macro
    GBR_LAST                // last value for this list
};

/***/

class GERBER_DRAW_ITEM : public BOARD_ITEM
{
    // make SetNext() and SetBack() private so that they may not be called from anywhere.
    // list management is done on GERBER_DRAW_ITEMs using DLIST<GERBER_DRAW_ITEM> only.
private:
    void SetNext( EDA_ITEM* aNext )       { Pnext = aNext; }
    void SetBack( EDA_ITEM* aBack )       { Pback = aBack; }


public:
    bool    m_UnitsMetric;                  /* store here the gerber units (inch/mm).
                                             *  Used only to calculate aperture macros shapes sizes  */
    int     m_Shape;                        // Shape and type of this gerber item
    wxPoint m_Start;                        // Line or arc start point or position of the shape
                                            // for flashed items
    wxPoint m_End;                          // Line or arc end point
    wxPoint m_ArcCentre;                    // for arcs only: Centre of arc
    std::vector <wxPoint> m_PolyCorners;    // list of corners for polygons (G36 to G37 coordinates)
                                            // or for complex shapes which are converted to polygon
    wxSize  m_Size;                         // Flashed shapes: size of the shape
                                            // Lines : m_Size.x = m_Size.y = line width
    bool    m_Flashed;                      // True for flashed items
    int     m_DCode;                        // DCode used to draw this item.
                                            // 0 for items that do not use DCodes (polygons)
                                            // or when unknown and normal values are 10 to 999
                                            // values 0 to 9 can be used for special purposes
    GERBER_IMAGE* m_imageParams;            /* main GERBER info for this item
                                             * Note: some params stored in this class are common
                                             * to the whole gerber file (i.e) the whole graphic layer
                                             * and some can change when reaging the file, so they
                                             * are stored inside this item
                                             * there is no redundancy for these parameters
                                             */
private:
    // These values are used to draw this item, according to gerber layers parameters
    // Because they can change inside a gerber image, thery are stored here
    // for each item
    bool        m_LayerNegative;            // TRUE = item in negative Layer
    bool        m_swapAxis;                 // false if A = X, B = Y; true if A =Y, B = Y
    bool        m_mirrorA;                  // true: mirror / axe A
    bool        m_mirrorB;                  // true: mirror / axe B
    wxRealPoint m_drawScale;                // A and B scaling factor
    wxPoint     m_layerOffset;              // Offset for A and B axis, from OF parameter
    double      m_lyrRotation;              // Fine rotation, from OR parameter, in degrees

public:
    GERBER_DRAW_ITEM( BOARD_ITEM* aParent, GERBER_IMAGE* aGerberparams );
    GERBER_DRAW_ITEM( const GERBER_DRAW_ITEM& aSource );
    ~GERBER_DRAW_ITEM();

    /**
     * Function Copy
     * will copy this object
     * the corresponding type.
     * @return - GERBER_DRAW_ITEM*
     */
    GERBER_DRAW_ITEM* Copy() const;

    GERBER_DRAW_ITEM* Next() const { return (GERBER_DRAW_ITEM*) Pnext; }
    GERBER_DRAW_ITEM* Back() const { return (GERBER_DRAW_ITEM*) Pback; }

    int ReturnMaskLayer()
    {
        return 1 << m_Layer;
    }

    bool GetLayerPolarity()
    {
        return m_LayerNegative;
    }

    /**
     * Function HasNegativeItems
     * @return true if this item or at least one shape (when using aperture macros
     *    must be drawn in background color
     * used to optimize screen refresh (when no items are in background color
     * refresh can be faster)
     */
    bool HasNegativeItems();

    /**
     * Function SetLayerParameters
     * Initialize parameters from Image and Layer parameters
     * found in the gerber file:
     *   m_UnitsMetric,
     *   m_MirrorA, m_MirrorB,
     *   m_DrawScale, m_DrawOffset
     */
    void SetLayerParameters( );

    void SetLayerPolarity( bool aNegative)
    {
        m_LayerNegative = aNegative;
    }

    /**
     * Function MoveAB
     * move this object.
     * @param aMoveVector - the move vector for this object.
     */
    void MoveAB( const wxPoint& aMoveVector );

     /**
     * Function MoveXY
     * move this object.
     * @param aMoveVector - the move vector for this object, in XY gerber axis.
     */
    void MoveXY( const wxPoint& aMoveVector );

    /**
     * Function GetPosition
     * returns the position of this object.
     * @return const wxPoint& - The position of this object.
     * This function exists mainly to satisfy the virtual GetPosition() in parent class
     */
    wxPoint& GetPosition()
    {
        return m_Start;  // it had to be start or end.
    }

    /**
     * Function GetABPosition
     * returns the image position of aPosition for this object.
     * Image position is the value of aPosition, modified by image parameters:
     * offsets, axis selection, scale, rotation
     * @param aXYPosition = position in X,Y gerber axis
     * @return const wxPoint - The given position in plotter A,B axis.
     */
    wxPoint GetABPosition( const wxPoint& aXYPosition ) const;

    /**
     * Function GetXYPosition
     * returns the image position of aPosition for this object.
     * Image position is the value of aPosition, modified by image parameters:
     * offsets, axis selection, scale, rotation
     * @param aABPosition = position in A,B plotter axis
     * @return const wxPoint - The given position in X,Y axis.
     */
    wxPoint GetXYPosition( const wxPoint& aABPosition );

    /**
     * Function GetDcodeDescr
     * returns the GetDcodeDescr of this object, or NULL.
     * @return D_CODE* - a pointer to the DCode description (for flashed items).
     */
    D_CODE*  GetDcodeDescr();

    EDA_Rect GetBoundingBox() const;

    /* Display on screen: */
    void     Draw( EDA_DRAW_PANEL* aPanel,
                   wxDC*           aDC,
                   int             aDrawMode,
                   const wxPoint&  aOffset = ZeroOffset );

    /**
     * Function ConvertSegmentToPolygon
     * convert a line to an equivalent polygon.
     * Useful when a line is plotted using a rectangular pen.
     * In this case, the usual segment plot function cannot be used
     */
    void ConvertSegmentToPolygon( );

    /**
     * Function DrawGbrPoly
     * a helper function used to draw the polygon stored in m_PolyCorners
     */
    void DrawGbrPoly( EDA_Rect* aClipBox,
                      wxDC* aDC, int aColor,
                      const wxPoint& aOffset, bool aFilledShape );

    /* divers */
    int Shape() const { return m_Shape; }

    /**
     * Function DisplayInfo
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_ITEM.
     * Display info about this GERBER item
     * @param frame A EDA_DRAW_FRAME in which to print status information.
     */
    void     DisplayInfo( EDA_DRAW_FRAME* frame );

    wxString ShowGBRShape();

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param aRefPos a wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool     HitTest( const wxPoint& aRefPos );

    /**
     * Function HitTest (overlayed)
     * tests if the given wxRect intersect this object.
     * For now, an ending point must be inside this rect.
     * @param aRefArea a wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool     HitTest( EDA_Rect& aRefArea );

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT( "GERBER_DRAW_ITEM" );
    }


    bool         Save( FILE* aFile ) const;

#if defined(DEBUG)

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    virtual void Show( int nestLevel, std::ostream& os );

#endif
};

#endif /* CLASS_GERBER_DRAW_ITEM_H */
