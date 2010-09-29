/*******************************************************************/
/*	class_gerber_draw_item.h: definitions relatives to tracks, vias and zones */
/*******************************************************************/

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
    void SetNext( EDA_BaseStruct* aNext )       { Pnext = aNext; }
    void SetBack( EDA_BaseStruct* aBack )       { Pback = aBack; }


public:
    int     m_Shape;                        // Shape and type of this gerber item
    wxPoint m_Start;                        // Line or arc start point or position of the shape
                                            // for flashed items
    wxPoint m_End;                          // Line or arc end point
    wxPoint m_ArcCentre;                    // for arcs only: Centre of arc
    std::vector <wxPoint> m_PolyCorners;    // list of corners for polygons (G36 to G37 coordinates)
                                            // or for complex shapes which are converted to polygon
    wxSize  m_Size;                         // Flashed shapes size of the shape
                                            // Lines : m_Size.x = m_Size.y = line width
    bool    m_Flashed;                      // True for flashed items
    int     m_DCode;                        // DCode used to draw this item.
                                            // 0 for items that do not use DCodes (polygons)
                                            // or when unknown and normal values are 10 to 999
                                            // values 0 to 9 can be used for special purposes

public:
    GERBER_DRAW_ITEM( BOARD_ITEM* aParent );
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


    /**
     * Function Move
     * move this object.
     * @param const wxPoint& aMoveVector - the move vector for this object.
     */
    void Move( const wxPoint& aMoveVector );

    /**
     * Function GetPosition
     * returns the position of this object.
     * @return const wxPoint& - The position of this object.
     */
    wxPoint& GetPosition()
    {
        return m_Start;  // it had to be start or end.
    }


    /**
     * Function GetDcodeDescr
     * returns the GetDcodeDescr of this object, or NULL.
     * @return D_CODE* - a pointer to the DCode description (for flashed items).
     */
    D_CODE*  GetDcodeDescr();

    EDA_Rect GetBoundingBox();

    /* Display on screen: */
    void     Draw( WinEDA_DrawPanel* aPanel,
                   wxDC*             aDC,
                   int               aDrawMode,
                   const wxPoint&    aOffset = ZeroOffset );

    /** function DrawGbrPoly
     * a helper function used id ::Draw to draw the polygon stored ion m_PolyCorners
     */
    void DrawGbrPoly( EDA_Rect*      aClipBox,
                      wxDC*          aDC, int      aColor,
                      const wxPoint& aOffset, bool aFilledShape );

    /* divers */
    int Shape() const { return m_Shape; }

    /**
     * Function DisplayInfo
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_BaseStruct.
     * Display info about the track segment and the full track length
     * @param frame A WinEDA_DrawFrame in which to print status information.
     */
    void     DisplayInfo( WinEDA_DrawFrame* frame );

    wxString ShowGBRShape();

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool     HitTest( const wxPoint& refPos );

    /**
     * Function HitTest (overlayed)
     * tests if the given wxRect intersect this object.
     * For now, an ending point must be inside this rect.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool     HitTest( EDA_Rect& refArea );

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT( "GERBER_DRAW_ITEM" );
    }


    bool Save( FILE* aFile ) const;

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
