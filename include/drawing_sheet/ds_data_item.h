/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef  DS_DATA_ITEM_H
#define  DS_DATA_ITEM_H

#include <math/vector2d.h>
#include <eda_text.h>
#include <bitmap_base.h>
#include "drawing_sheet/ds_draw_item.h"

class DS_DRAW_ITEM_TEXT;            // Forward declaration

#define TB_DEFAULT_TEXTSIZE 1.5     // default drawing sheet text size in mm

namespace KIGFX
{
class VIEW;
}

/**
 * A coordinate is relative to a page corner.
 *
 * Any of the 4 corners can be a reference.  The default is the right bottom corner.
 */
enum CORNER_ANCHOR
{
    RB_CORNER,      // right bottom corner
    RT_CORNER,      // right top corner
    LB_CORNER,      // left bottom corner
    LT_CORNER,      // left top corner
};

enum PAGE_OPTION
{
    ALL_PAGES,
    FIRST_PAGE_ONLY,
    SUBSEQUENT_PAGES
};

/**
 * A coordinate point.
 *
 * The position is always relative to the corner anchor.
 *
 * @note The coordinate is from the anchor point to the opposite corner.
 */
class POINT_COORD
{
public:
    POINT_COORD() { m_Anchor = RB_CORNER; }

    POINT_COORD( const VECTOR2D& aPos, enum CORNER_ANCHOR aAnchor = RB_CORNER )
    {
        m_Pos = aPos;
        m_Anchor = aAnchor;
    }

    VECTOR2D          m_Pos;
    int               m_Anchor;
};


/**
 * Drawing sheet structure type definitions.
 *
 * Basic items are:
 * * segment and rect (defined by 2 points)
 * * text (defined by a coordinate), the text and its justifications
 * * poly polygon defined by a coordinate, and a set of list of corners
 *   ( because we use it for logos, there are more than one polygon
 *   in this description
 */
class DS_DATA_ITEM
{
public:
    enum DS_ITEM_TYPE {
        DS_TEXT,
        DS_SEGMENT,
        DS_RECT,
        DS_POLYPOLYGON,
        DS_BITMAP
    };

    DS_DATA_ITEM( DS_ITEM_TYPE aType );

    virtual ~DS_DATA_ITEM();

    const std::vector<DS_DRAW_ITEM_BASE*>& GetDrawItems() const { return m_drawItems; }

    virtual void SyncDrawItems( DS_DRAW_ITEM_LIST* aCollector, KIGFX::VIEW* aView );

    void SetStart( double aPosx, double aPosy, enum CORNER_ANCHOR aAnchor = RB_CORNER )
    {
        m_Pos.m_Pos.x = aPosx;
        m_Pos.m_Pos.y = aPosy;
        m_Pos.m_Anchor = aAnchor;
    }

    void SetEnd( double aPosx, double aPosy, enum CORNER_ANCHOR aAnchor = RB_CORNER )
    {
        m_End.m_Pos.x = aPosx;
        m_End.m_Pos.y = aPosy;
        m_End.m_Anchor = aAnchor;
    }

    DS_ITEM_TYPE GetType() const { return m_type; }

    /**
     * @return true if the item has a end point (segment; rect) of false (text, polygon).
     */
    PAGE_OPTION GetPage1Option() const { return m_pageOption; }
    void SetPage1Option( PAGE_OPTION aChoice ) { m_pageOption = aChoice; }

    // Coordinate handling
    const VECTOR2I GetStartPosIU( int ii = 0 ) const;
    const VECTOR2I GetEndPosIU( int ii = 0 ) const;
    const VECTOR2D GetStartPos( int ii = 0 ) const;
    const VECTOR2D GetEndPos( int ii = 0 ) const;

    virtual int GetPenSizeIU();

    /**
     * Move item to a new position.
     *
     * @param aPosition the new position of item, in mm.
     */
    void MoveTo( const VECTOR2D& aPosition );

    /**
     * Move item to a new position.
     *
     * @param aPosition the new position of the starting point in graphic units.
     */
    void MoveToIU( const VECTOR2I& aPosition );

    /**
     * Move the starting point of the item to a new position.
     *
     * @param aPosition the new position of the starting point, in mm.
     */
    void MoveStartPointTo( const VECTOR2D& aPosition );

    /**
     * Move the starting point of the item to a new position.
     *
     * @param aPosition is the new position of item in graphic units.
     */
    void MoveStartPointToIU( const VECTOR2I& aPosition );


    /**
     * Move the ending point of the item to a new position.
     *
     * This has meaning only for items defined by 2 points (segments and rectangles).
     *
     * @param aPosition is the new position of the ending point, in mm.
     */
    void MoveEndPointTo( const VECTOR2D& aPosition );

    /**
     * Move the ending point of the item to a new position.
     *
     * This has meaning only for items defined by 2 points (segments and rectangles).
     *
     * @param aPosition is the new position of the ending point in graphic units
     */
    void MoveEndPointToIU( const VECTOR2I& aPosition );

    /**
     * @return true if the item is inside the rectangle defined by the 4 corners, false otherwise.
     */
    virtual bool IsInsidePage( int ii ) const;

    const wxString GetClassName() const;

    wxString       m_Name;               // a name used in drawing sheet editor to identify items
    wxString       m_Info;               // a comment, only useful in drawing sheet editor
    POINT_COORD    m_Pos;
    POINT_COORD    m_End;
    double         m_LineWidth;
    int            m_RepeatCount;        // repeat count for duplicate items
    VECTOR2D       m_IncrementVector;    // for duplicate items: move vector for position increment
    int            m_IncrementLabel;

protected:
    DS_ITEM_TYPE   m_type;
    PAGE_OPTION    m_pageOption;

    std::vector<DS_DRAW_ITEM_BASE*> m_drawItems;
};


class DS_DATA_ITEM_POLYGONS : public DS_DATA_ITEM
{
public:
    DS_DATA_ITEM_POLYGONS( );

    void SyncDrawItems( DS_DRAW_ITEM_LIST* aCollector, KIGFX::VIEW* aView ) override;

    virtual int GetPenSizeIU() override;

    /**
     * Add a corner in corner list.
     *
     * @param aCorner is the item to append.
     */
    void AppendCorner( const VECTOR2D& aCorner )
    {
        m_Corners.push_back( aCorner );
    }

    /**
     * Close the current contour, by storing the index of the last corner of the current
     * polygon in m_polyIndexEnd.
     */
    void CloseContour()
    {
        m_polyIndexEnd.push_back( m_Corners.size() -1 );
    }

    /**
     * @return the count of contours in the poly polygon.
     */
    int GetPolyCount() const { return (int) m_polyIndexEnd.size(); }

    /**
     * @param aContour is the index of the contour.
     * @return the index of the first corner of the contour \a aCountour.
     */
    unsigned GetPolyIndexStart( unsigned aContour ) const
    {
        if( aContour == 0 )
            return 0;
        else
            return m_polyIndexEnd[aContour-1] + 1;
    }

    /**
     * @param aContour is the index of the contour.
     * @return the index of the last corner of the contour \a aCountour.
     */
    unsigned GetPolyIndexEnd( unsigned aContour ) const
    {
        return m_polyIndexEnd[aContour];
    }

    /**
     * @return the coordinate (in mm) of the corner \a aIdx and the repeated item \a aRepeat
     */
    const VECTOR2D GetCornerPosition( unsigned aIdx, int aRepeat = 0 ) const;

    /**
     * @return the coordinate (in draw/plot units) of the corner \a aIdx and the repeated
     *         item \a aRepeat
     */
    const VECTOR2I GetCornerPositionIU( unsigned aIdx, int aRepeat = 0 ) const;

    /**
     * Calculate the bounding box of the set polygons.
     */
    void SetBoundingBox();

    bool IsInsidePage( int ii ) const override;

    EDA_ANGLE             m_Orient;         // Orientation
    std::vector<VECTOR2D> m_Corners;        // corner list

private:
    std::vector<unsigned> m_polyIndexEnd;   // index of the last point of each polygon
    VECTOR2D              m_minCoord;       // min coord of corners, relative to m_Pos
    VECTOR2D              m_maxCoord;       // max coord of corners, relative to m_Pos
};


class DS_DATA_ITEM_TEXT : public DS_DATA_ITEM
{
public:
    DS_DATA_ITEM_TEXT( const wxString& aTextBase );

    void SyncDrawItems( DS_DRAW_ITEM_LIST* aCollector, KIGFX::VIEW* aView ) override;

    virtual int GetPenSizeIU() override;

    /**
     * Try to build text which is an increment of m_TextBase
     * has meaning only if m_TextBase is a basic text (one char)
     * If the basic char is a digit, build a number
     * If the basic char is a letter, use the letter with ASCII code
     * aIncr + (basic char ascc code)
     * @param aIncr = the increment value
     * return the incremented label in m_FullText
     */
    void IncrementLabel( int aIncr );

    /**
     * Calculate m_ConstrainedTextSize from m_TextSize
     * to keep the X size and the full Y size of the text
     * smaller than m_BoundingBoxSize
     * if m_BoundingBoxSize.x or m_BoundingBoxSize.y > 0
     * if m_BoundingBoxSize.x or m_BoundingBoxSize.y == 0
     * the corresponding text size is not constrained
     */
    void SetConstrainedTextSize();

    /**
     * Replace the '\''n' sequence by EOL and the sequence  '\''\' by only one '\'
     * inside m_FullText
     * @return true if the EOL symbol is found or is inserted (multiline text).
     */
    bool ReplaceAntiSlashSequence();

public:
    wxString            m_TextBase;             // The basic text, with format symbols
    wxString            m_FullText;             // The expanded text, shown on screen
    double              m_Orient;               // Orientation in degrees
    GR_TEXT_H_ALIGN_T   m_Hjustify;
    GR_TEXT_V_ALIGN_T   m_Vjustify;
    bool                m_Italic;
    bool                m_Bold;
    KIFONT::FONT*       m_Font;
    VECTOR2D            m_TextSize;
    KIGFX::COLOR4D      m_TextColor;
    VECTOR2D            m_BoundingBoxSize;      // When not null, this is the max size of the
                                                // full text.  The text size will be modified
                                                // to keep the full text inside this bound.
    VECTOR2D            m_ConstrainedTextSize;  // Actual text size, if constrained by
                                                // the m_BoundingBoxSize constraint
};


class BITMAP_BASE;

class DS_DATA_ITEM_BITMAP : public DS_DATA_ITEM
{
public:
    DS_DATA_ITEM_BITMAP( BITMAP_BASE* aImage ) :
            DS_DATA_ITEM( DS_BITMAP )
    {
        m_ImageBitmap = aImage;
    }

    void SyncDrawItems( DS_DRAW_ITEM_LIST* aCollector, KIGFX::VIEW* aView ) override;

    int GetPPI() const;
    void SetPPI( int aBitmapPPI );

public:
    BITMAP_BASE* m_ImageBitmap;
};


#endif      // DS_DATA_ITEM_H
