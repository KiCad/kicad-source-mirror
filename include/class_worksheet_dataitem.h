/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @file class_worksheet_dataitem.h
 * @brief description of graphic items and texts to build a title block
 */

#ifndef  CLASS_WORKSHEET_DATA_ITEM_H
#define  CLASS_WORKSHEET_DATA_ITEM_H

#include <math/vector2d.h>
#include <eda_text.h>
#include <class_bitmap_base.h>

class WS_DRAW_ITEM_TEXT;            // Forward declaration

#define TB_DEFAULT_TEXTSIZE 1.5     // default worksheet text size in mm

// Text attributes set in m_flags (ORed bits)
#define USE_BOLD 1                  // has meaning for texts
#define USE_THICK_LINE 1            // equivalent to bold for lines
#define USE_ITALIC (1<<1)           // has meaning for texts
#define USE_ALT_COLOR (1<<2)
#define SELECTED_STATE (1<<3)       // When set, use the hight light color to draw item
#define NEW_ITEM (1<<4)             // Set for new items which can be deleted
                                    // by an abort command
#define LOCATE_STARTPOINT (1<<5)    // Used in locate function:set by locate function
                                    // if the start point is located
#define LOCATE_ENDPOINT (1<<6)      // Used in locate function:set by locate function
                                    // if the end point is located
#define PAGE1OPTION (3<<7)          // flag to manage items drawn or not drawn only
                                    // on page 1: NONE = item on all pages
#define PAGE1OPTION_NONE (0<<7)     //  NONE = item on all pages
#define PAGE1OPTION_PAGE1ONLY   (1<<7)     //  = item only on page 1
#define PAGE1OPTION_NOTONPAGE1  (2<<7)     //  = item on all pages but page 1

// A coordinate is relative to a page corner.
// Any of the 4 corners can be a reference.
// The default is the right bottom corner
enum corner_anchor
{
    RB_CORNER,      // right bottom corner
    RT_CORNER,      // right top corner
    LB_CORNER,      // left bottom corner
    LT_CORNER,      // left top corner
};

// a coordinate point
// The position is always relative to the corner anchor
// Note the coordinate is from the anchor point
// to the opposite corner.
class POINT_COORD
{
public:
    DPOINT            m_Pos;
    int               m_Anchor;
public:
    POINT_COORD() { m_Anchor = RB_CORNER; }
    POINT_COORD( DPOINT aPos, enum corner_anchor aAnchor = RB_CORNER )
    {
        m_Pos = aPos;
        m_Anchor = aAnchor;
    }
};


// Work sheet structure type definitions.
// Basic items are:
// * segment and rect (defined by 2 points)
// * text (defined by a coordinate), the text and its justifications
// * poly polygon defined by a coordinate, and a set of list of corners
//   ( because we use it for logos, there are more than one polygon
//   in this description
class WORKSHEET_DATAITEM
{
public:
    enum WS_ItemType {
        WS_TEXT,
        WS_SEGMENT,
        WS_RECT,
        WS_POLYPOLYGON,
        WS_BITMAP
    };

protected:
    WS_ItemType       m_type;
    int               m_flags;

public:
    wxString          m_Name;               // a item name used in page layout
                                            // editor to identify items
    wxString          m_Info;               // a comment, only useful in page
                                            // layout editor
    POINT_COORD       m_Pos;
    POINT_COORD       m_End;
    double            m_LineWidth;
    int               m_RepeatCount;        // repeat count for duplicate items
    DPOINT            m_IncrementVector;    // For duplicate items: move vector
                                            // for position increment
    int               m_IncrementLabel;

    // These variables are static, because these values are common to all
    // instances of WORKSHEET_DATAITEM.
    // They are default or common values.
    static double     m_WSunits2Iu;         // conversion factor between
                                            // ws units (mils) and draw/plot units
    static DPOINT     m_RB_Corner;          // cordinates of the right bottom corner
                                            // (ws units)
    static DPOINT     m_LT_Corner;          // cordinates of the left top corner
                                            // (ws units)
    static double     m_DefaultLineWidth;   // Default line width,
                                            // when not defined inside a line
                                            // or a rect
    static DSIZE      m_DefaultTextSize;    // Default text size,
                                            // when not defined inside a tbtext
    static double     m_DefaultTextThickness;// Default text thickness,
                                            // when not defined inside a tbtext
    static bool       m_SpecialMode;        // Used in page layout editor
                                            // When set to true, base texts
                                            // instead of full texts are displayed
    static EDA_COLOR_T m_Color;             // the default color to draw items
    static EDA_COLOR_T m_AltColor;          // an alternate color to draw items
    static EDA_COLOR_T m_SelectedColor;     // the color to draw selected items
                                            // (used in page layout editor


public:
    WORKSHEET_DATAITEM( WS_ItemType aType );

    virtual ~WORKSHEET_DATAITEM() {}

    void SetStart( double aPosx, double aPosy, enum corner_anchor aAnchor = RB_CORNER )
    {
        m_Pos.m_Pos.x = aPosx;
        m_Pos.m_Pos.y = aPosy;
        m_Pos.m_Anchor = aAnchor;
    }

    void SetEnd( double aPosx, double aPosy, enum corner_anchor aAnchor = RB_CORNER )
    {
        m_End.m_Pos.x = aPosx;
        m_End.m_Pos.y = aPosy;
        m_End.m_Anchor = aAnchor;
    }

    // Accessors:
    WS_ItemType GetType() const { return m_type; }
    int GetFlags() const { return m_flags; }
    void SetFlags( int aMask ) { m_flags |= aMask; }
    void ClearFlags( int aMask ) { m_flags &= ~aMask; }

    /**
     * @return true if the item has a end point (segment; rect)
     * of false (text, polugon)
     */
    virtual bool HasEndPoint() { return true; }

    /**
     * @return 0 if the item has no specific option for page 1
     * 1  if the item is only on page 1
     * -1  if the item is not on page 1
     */
    int GetPage1Option();

    /**
     * Set the option for page 1
     * @param aChoice = 0 if the item has no specific option for page 1
     * > 0  if the item is only on page 1
     * < 0  if the item is not on page 1
     */
    void SetPage1Option( int aChoice );

    // Coordinate handling
    const wxPoint GetStartPosUi( int ii = 0 ) const;
    const wxPoint GetEndPosUi( int ii = 0 ) const;
    const DPOINT GetStartPos( int ii = 0 ) const;
    const DPOINT GetEndPos( int ii = 0 ) const;
    virtual int GetPenSizeUi()
    {
        if( m_LineWidth )
            return KiROUND( m_LineWidth * m_WSunits2Iu );
        else
            return KiROUND( m_DefaultLineWidth * m_WSunits2Iu );
    }

    static int GetMarkerSizeUi()
    {
        return KiROUND( 0.5 * m_WSunits2Iu );
    }

    /**
     * move item to a new position
     * @param aPosition = the new position of item, in mm
     */
    void MoveTo( DPOINT aPosition );

    /**
     * move item to a new position
     * @param aPosition = the new position of the starting point in graphic units
     */
    void MoveToUi( wxPoint aPosition );

    /**
     * move the starting point of the item to a new position
     * @param aPosition = the new position of the starting point, in mm
     */
    void MoveStartPointTo( DPOINT aPosition );

    /**
     * move the starting point of the item to a new position
     * @param aPosition = the new position of item in graphic units
     */
    void MoveStartPointToUi( wxPoint aPosition );


    /**
     * move the ending point of the item to a new position
     * has meaning only for items defined by 2 points
     * (segments and rectangles)
     * @param aPosition = the new position of the ending point, in mm
     */
    void MoveEndPointTo( DPOINT aPosition );

    /**
     * move the ending point of the item to a new position
     * has meaning only for items defined by 2 points
     * (segments and rectangles)
     * @param aPosition = the new position of the ending point in graphic units
     */
    void MoveEndPointToUi( wxPoint aPosition );

    /**
     * @return true if the item is inside the rectangle defined by the
     * 4 corners, false otherwise.
     */
    virtual bool IsInsidePage( int ii ) const;

    const wxString GetClassName() const;

    /**
     * @return true if the selected state on ON
     */
    bool IsSelected() { return (m_flags & SELECTED_STATE) != 0; }

    /**
     * Function SetSelected
     * Toggles on/off the selected flag (used in edition functions
     * @param aState = the flag value
     */
    void SetSelected( bool aState )
    {
        if( aState )
            m_flags |= SELECTED_STATE;
        else
            m_flags &= ~SELECTED_STATE;
    }

    bool UseAltColor() {return m_flags & USE_ALT_COLOR; }

    EDA_COLOR_T GetItemColor()
    {
        if( IsSelected() )
            return m_SelectedColor;

        if( UseAltColor() )
            return m_AltColor;

        return m_Color;
    }
};


class WORKSHEET_DATAITEM_POLYPOLYGON : public WORKSHEET_DATAITEM
{
public:
    double            m_Orient;             //  Orientation in degrees
    std::vector<DPOINT> m_Corners;          // corner list

private:
    std::vector<unsigned> m_polyIndexEnd;   // index of the last point of each polygon
    DPOINT            m_minCoord;           // min coord of corners, relative to m_Pos
    DPOINT            m_maxCoord;           // max coord of corners, relative to m_Pos

public:
    WORKSHEET_DATAITEM_POLYPOLYGON( );

    virtual int GetPenSizeUi()
    {
        return KiROUND( m_LineWidth * m_WSunits2Iu );
    }

   /**
     * @return false  (no end point)
     */
    virtual bool HasEndPoint() { return false; };

    /**
     * add a corner in corner list
     * @param aCorner: the item to append
     */
    void AppendCorner( const DPOINT& aCorner )
    {
        m_Corners.push_back( aCorner );
    }

    /**
     * Closes the current contour, by storing the index of the last corner
     * of the current polygon in m_polyIndexEnd.
     */
    void CloseContour()
    {
        m_polyIndexEnd.push_back( m_Corners.size() -1 );
    }

    /**
     * @return the count of contours in the poly polygon
     */
    int GetPolyCount() const { return (int) m_polyIndexEnd.size(); }

    /**
     * @return the index of the first corner of the contour aCountour
     * @param aContour = the index of the contour
     */
    unsigned GetPolyIndexStart( unsigned aContour) const
    {
        if( aContour == 0 )
            return 0;
        else
            return m_polyIndexEnd[aContour-1] + 1;
    }

    /**
     * @return the index of the last corner of the contour aCountour
     * @param aContour = the index of the contour
     */
    unsigned GetPolyIndexEnd( unsigned aContour) const
    {
        return m_polyIndexEnd[aContour];
    }

    /**
     * @return the coordinate (in mm) of the corner aIdx,
     * for the repeated item aRepeat
     */
    const DPOINT GetCornerPosition( unsigned aIdx, int aRepeat = 0 ) const;

    /**
     * @return the coordinate (in draw/plot units) of the corner aIdx,
     * for the repeated item aRepeat
     */
    const wxPoint GetCornerPositionUi( unsigned aIdx, int aRepeat = 0 ) const;

    /**
     * calculate the bounding box of the set  polygons
     */
    void SetBoundingBox();

    bool IsInsidePage( int ii ) const;
};


class WORKSHEET_DATAITEM_TEXT : public WORKSHEET_DATAITEM
{
public:
    wxString            m_TextBase;             // The basic text, with format symbols
    wxString            m_FullText;             // The expanded text, shown on screen
    double              m_Orient;               //  Orientation in degrees
    EDA_TEXT_HJUSTIFY_T m_Hjustify;
    EDA_TEXT_VJUSTIFY_T m_Vjustify;
    DSIZE               m_TextSize;
    DSIZE               m_BoundingBoxSize;      // When not null, this is the max
                                                // size of the full text.
                                                // the text size will be modified
                                                // to keep the full text insite this
                                                // bound.
    DSIZE               m_ConstrainedTextSize;  // Actual text size, if constrained by
                                                // the m_BoundingBoxSize constraint


public:
    WORKSHEET_DATAITEM_TEXT( const wxString& aTextBase );

    /**
     * @return false  (no end point)
     */
    virtual bool HasEndPoint() { return false; };

    virtual int GetPenSizeUi()
    {
        if( m_LineWidth )
            return KiROUND( m_LineWidth * m_WSunits2Iu );
        else
            return KiROUND( m_DefaultTextThickness * m_WSunits2Iu );
    }

    /**
     * move item to a new position
     * @param aPosition = the new position of item
     */
    void MoveTo( DPOINT aPosition );

    /**
     * transfert the text justification and orientation
     * to aGText
     */
    void TransfertSetupToGraphicText(  WS_DRAW_ITEM_TEXT* aGText );

    /**
     * Try to build text wihich is an increment of m_TextBase
     * has meaning only if m_TextBase is a basic text (one char)
     * If the basic char is a digit, build a number
     * If the basic char is a letter, use the letter with ascii code
     * aIncr + (basic char ascc code)
     * @param aIncr = the increment value
     * return the incremented label in m_FullText
     */
    void IncrementLabel( int aIncr );

    /**
     * Calculates m_ConstrainedTextSize from m_TextSize
     * to keep the X size and the full Y size of the text
     * smaller than m_BoundingBoxSize
     * if m_BoundingBoxSize.x or m_BoundingBoxSize.y > 0
     * if m_BoundingBoxSize.x or m_BoundingBoxSize.y == 0
     * the corresponding text size is not constrained
     */
    void SetConstrainedTextSize();


    /** Replace the '\''n' sequence by EOL
     * and the sequence  '\''\' by only one '\'
     * inside m_FullText
     * @return true if the EOL symbol is found or is inserted (multiline text)
     */
    bool ReplaceAntiSlashSequence();

    /**
     * @return true is a bold font should be selected
     */
    bool IsBold() { return (m_flags & USE_BOLD) != 0; }

    /**
     * Function SetBold
     * Toggles on/off the bold option flag
     * @param aState = the bold option value
     */
    void SetBold( bool aState )
    {
        if( aState )
            m_flags |= USE_BOLD;
        else
            m_flags &= ~USE_BOLD;
    }

    /**
     * @return true is an italic font should be selected
     */
    bool IsItalic() const { return (m_flags & USE_ITALIC) != 0; }

    /**
     * Function SetItalic
     * Toggles on/off the italic option flag
     * @param aState = the italic option value
     */
    void SetItalic( bool aState )
    {
        if( aState )
            m_flags |= USE_ITALIC;
        else
            m_flags &= ~USE_ITALIC;
    }
};


class BITMAP_BASE;
class WORKSHEET_DATAITEM_BITMAP : public WORKSHEET_DATAITEM
{
public:
    BITMAP_BASE* m_ImageBitmap;

public:
    WORKSHEET_DATAITEM_BITMAP(BITMAP_BASE* aImage)
        : WORKSHEET_DATAITEM( WS_BITMAP )
    {
        m_ImageBitmap = aImage;
    }

    /**
     * @return false  (no end point)
     */
    virtual bool HasEndPoint() { return false; }

    /**
     * @return the PPI of the bitmap
     */
    int GetPPI() const;

    /**
     * adjust the PPI of the bitmap
     * @param aBitmapPPI = the ned PPI for the bitmap
     */
    void SetPPI( int aBitmapPPI );

    /**
     * set the pixel scale factor of the bitmap
     * this factor depend on the application internal unit
     * and the pixel per inch bitmap factor
     * the pixel scale factor is the pixel size to application internal unit
     * and should be initialized before printing or drawing the bitmap
     */
    void SetPixelScaleFactor();
};


#endif      // CLASS_WORKSHEET_DATA_ITEM_H
