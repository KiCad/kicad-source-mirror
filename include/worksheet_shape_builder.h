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
 * @file worksheet_shape_builder.h
 * @brief classes and function to generate graphics to plt or draw titles blocks
 * and frame references
 */

#ifndef  WORKSHEET_SHAPE_BUILDER_H
#define  WORKSHEET_SHAPE_BUILDER_H

#include <math/vector2d.h>
#include <eda_text.h>
#include <eda_text.h>
#include <class_bitmap_base.h>

class WORKSHEET_DATAITEM;        // Forward declaration
class TITLE_BLOCK;

#define TB_DEFAULT_TEXTSIZE             1.5  // default worksheet text size in mm

/*
 * Helper classes to handle basic graphic items used to raw/plot
 * title blocks and frame references
 * segments
 * rect
 * polygons (for logos)
 * graphic texts
 * bitmaps, also for logos, but they cannot be plot by SVG, GERBER
 * and HPGL plotters (In this case, only the bounding box is plotted)
 */
class WS_DRAW_ITEM_BASE     // This basic class, not directly usable.
{
public:
    enum WS_DRAW_TYPE {
        wsg_line, wsg_rect, wsg_poly, wsg_text, wsg_bitmap
    };
    int m_Flags;                    // temporary flgs used in page layout editor
                                    // to locate the item;

protected:
    WS_DRAW_TYPE    m_type; // wsg_line, wsg_rect, wsg_poly, wsg_text
    EDA_COLOR_T     m_color;
    WORKSHEET_DATAITEM*  m_parent;  // an unique identifier, used as link
                                    // to the parent WORKSHEET_DATAITEM item,
                                    // in page layout editor

    WS_DRAW_ITEM_BASE( WORKSHEET_DATAITEM*  aParent,
                       WS_DRAW_TYPE aType, EDA_COLOR_T aColor )
    {
        m_type  = aType;
        m_color = aColor;
        m_parent = aParent;
        m_Flags = 0;
    }

public:
    virtual ~WS_DRAW_ITEM_BASE() {}

    // Accessors:
    EDA_COLOR_T GetColor() const { return m_color; }
    WS_DRAW_TYPE GetType() const { return m_type; };

    WORKSHEET_DATAITEM* GetParent() const { return m_parent; }

    /** The function to draw a WS_DRAW_ITEM
     */
    virtual void DrawWsItem( EDA_RECT* aClipBox, wxDC* aDC ) = 0;

    /**
     * Abstract function: should exist for derived items
     * return true if the point aPosition is on the item
     */
    virtual bool HitTest( const wxPoint& aPosition) const = 0;

    /**
     * Abstract function: should exist for derived items
     * return true if the point aPosition is near the starting point of this item,
     * for items defined by 2 points (segments, rect)
     * or the position of the item, for items having only one point
     * (texts or polygons)
     * the maxi dist is WORKSHEET_DATAITEM::GetMarkerSizeUi()/2
     */
    virtual bool HitTestStartPoint( const wxPoint& aPosition) = 0;

    /**
     * return true if the point aPosition is near the ending point of this item
     * This is avirtual function which should be overriden for items defien by
     * 2 points
     * the maxi dist is WORKSHEET_DATAITEM::GetMarkerSizeUi()/2
     */
    virtual bool HitTestEndPoint( const wxPoint& aPosition)
    {
        return false;
    }
};

// This class draws a thick segment
class WS_DRAW_ITEM_LINE : public WS_DRAW_ITEM_BASE
{
    wxPoint m_start;    // start point of line/rect
    wxPoint m_end;      // end point
    int     m_penWidth;

public:
    WS_DRAW_ITEM_LINE( WORKSHEET_DATAITEM* aParent,
                       wxPoint aStart, wxPoint aEnd,
                       int aPenWidth, EDA_COLOR_T aColor ) :
        WS_DRAW_ITEM_BASE( aParent, wsg_line, aColor )
    {
        m_start     = aStart;
        m_end       = aEnd;
        m_penWidth  = aPenWidth;
    }

    // Accessors:
    int GetPenWidth() const { return m_penWidth; }
    const wxPoint&  GetStart() const { return m_start; }
    const wxPoint&  GetEnd() const { return m_end; }

    /** The function to draw a WS_DRAW_ITEM_LINE
     */
    virtual void DrawWsItem( EDA_RECT* aClipBox, wxDC* aDC );

    /**
     * Virtual function
     * return true if the point aPosition is on the line
     */
    virtual bool HitTest( const wxPoint& aPosition) const;

    /**
     * return true if the point aPosition is on the starting point of this item.
     */
    virtual bool HitTestStartPoint( const wxPoint& aPosition);

    /**
     * return true if the point aPosition is on the ending point of this item
     * This is avirtual function which should be overriden for items defien by
     * 2 points
     */
    virtual bool HitTestEndPoint( const wxPoint& aPosition);
};

// This class draws a polygon
class WS_DRAW_ITEM_POLYGON : public WS_DRAW_ITEM_BASE
{
    wxPoint m_pos;      // position of reference point, from the
                        // WORKSHEET_DATAITEM_POLYPOLYGON parent
                        // (used only in page layout editor to draw anchors)
    int m_penWidth;
    bool m_fill;

public:
    std::vector <wxPoint> m_Corners;

public:
    WS_DRAW_ITEM_POLYGON( WORKSHEET_DATAITEM* aParent, wxPoint aPos,
                          bool aFill, int aPenWidth, EDA_COLOR_T aColor ) :
        WS_DRAW_ITEM_BASE( aParent, wsg_poly, aColor )
    {
        m_penWidth = aPenWidth;
        m_fill = aFill;
        m_pos = aPos;
    }

    // Accessors:
    int GetPenWidth() const { return m_penWidth; }
    bool IsFilled() const { return m_fill; }
    const wxPoint& GetPosition() const { return m_pos; }

    /** The function to draw a WS_DRAW_ITEM_POLYGON
     */
    virtual void DrawWsItem( EDA_RECT* aClipBox, wxDC* aDC );

    /**
     * Virtual function
     * return true if the point aPosition is inside one polygon
     */
    virtual bool HitTest( const wxPoint& aPosition) const;

    /**
     * return true if the point aPosition is on the starting point of this item.
     */
    virtual bool HitTestStartPoint( const wxPoint& aPosition);
};

// This class draws a not filled rectangle with thick segment
class WS_DRAW_ITEM_RECT : public WS_DRAW_ITEM_LINE
{
public:
    WS_DRAW_ITEM_RECT( WORKSHEET_DATAITEM* aParent,
                       wxPoint aStart, wxPoint aEnd,
                       int aPenWidth, EDA_COLOR_T aColor ) :
        WS_DRAW_ITEM_LINE( aParent, aStart, aEnd, aPenWidth, aColor )
    {
        m_type = wsg_rect;
    }

    /** The function to draw a WS_DRAW_ITEM_RECT
     */
    virtual void DrawWsItem( EDA_RECT* aClipBox, wxDC* aDC );

    /**
     * Virtual function
     * return true if the point aPosition is on one edge of the rectangle
     */
    virtual bool HitTest( const wxPoint& aPosition) const;

    /**
     * return true if the point aPosition is on the starting point of this item.
     */
    virtual bool HitTestStartPoint( const wxPoint& aPosition);

    /**
     * return true if the point aPosition is on the ending point of this item
     * This is avirtual function which should be overriden for items defien by
     * 2 points
     */
    virtual bool HitTestEndPoint( const wxPoint& aPosition);
};

// This class draws a graphic text.
// it is derived from an EDA_TEXT, so it handle all caracteristics
// of this graphic text (justification, rotation ... )
class WS_DRAW_ITEM_TEXT : public WS_DRAW_ITEM_BASE, public EDA_TEXT
{
public:
    WS_DRAW_ITEM_TEXT( WORKSHEET_DATAITEM* aParent,
                       wxString& aText, wxPoint aPos, wxSize aSize,
                       int aPenWidth, EDA_COLOR_T aColor,
                       bool aItalic = false, bool aBold = false );

    /** The function to draw a WS_DRAW_ITEM_TEXT
     */
    virtual void DrawWsItem( EDA_RECT* aClipBox, wxDC* aDC );

    // Accessors:
    int GetPenWidth() { return GetThickness(); }

    /**
     * Virtual function
     * return true if the point aPosition is on the text
     */
    virtual bool HitTest( const wxPoint& aPosition) const;

    /**
     * return true if the point aPosition is on the starting point of this item.
     */
    virtual bool HitTestStartPoint( const wxPoint& aPosition);
};

// This class draws a bitmap.
class WS_DRAW_ITEM_BITMAP : public WS_DRAW_ITEM_BASE
{
    wxPoint m_pos;                  // position of reference point

public:
    WS_DRAW_ITEM_BITMAP( WORKSHEET_DATAITEM* aParent, wxPoint aPos )
        :WS_DRAW_ITEM_BASE( aParent, wsg_bitmap, UNSPECIFIED_COLOR )
    {
        m_pos = aPos;
    }

    WS_DRAW_ITEM_BITMAP()
        :WS_DRAW_ITEM_BASE( NULL, wsg_bitmap, UNSPECIFIED_COLOR )
    {
    }

    ~WS_DRAW_ITEM_BITMAP() {}

    /** The function to draw a WS_DRAW_ITEM_BITMAP
     */
    virtual void DrawWsItem( EDA_RECT* aClipBox, wxDC* aDC );

    /**
     * Virtual function
     * return true if the point aPosition is on bitmap
     */
    virtual bool HitTest( const wxPoint& aPosition) const;

    /**
     * return true if the point aPosition is on the reference point of this item.
     */
    virtual bool HitTestStartPoint( const wxPoint& aPosition);

    const wxPoint& GetPosition() { return m_pos; }
};

/*
 * this class stores the list of graphic items:
 * rect, lines, polygons and texts to draw/plot
 * the title block and frame references, and parameters to
 * draw/plot them
 */
class WS_DRAW_ITEM_LIST
{
protected:
    std::vector <WS_DRAW_ITEM_BASE*> m_graphicList;     // Items to draw/plot
    unsigned m_idx;             // for GetFirst, GetNext functions
    wxPoint  m_LTmargin;        // The left top margin in mils of the page layout.
    wxPoint  m_RBmargin;        // The right bottom margin in mils of the page layout.
    wxSize   m_pageSize;        // the page size in mils
    double   m_milsToIu;        // the scalar to convert pages units ( mils)
                                // to draw/plot units.
    int      m_penSize;         // The default line width for drawings.
                                // used when an item has a pen size = 0
    int      m_sheetNumber;     // the value of the sheet number, for basic inscriptions
    int      m_sheetCount;      // the value of the number of sheets, in schematic
                                // for basic inscriptions, in schematic
    const TITLE_BLOCK* m_titleBlock;    // for basic inscriptions
    const wxString* m_paperFormat;      // for basic inscriptions
    wxString        m_fileName;         // for basic inscriptions
    const wxString* m_sheetFullName;    // for basic inscriptions


public:
    WS_DRAW_ITEM_LIST()
    {
        m_idx = 0;
        m_milsToIu = 1.0;
        m_penSize = 1;
        m_sheetNumber = 1;
        m_sheetCount = 1;
        m_titleBlock = NULL;
        m_paperFormat = NULL;
        m_sheetFullName = NULL;
    }

    ~WS_DRAW_ITEM_LIST()
    {
        for( unsigned ii = 0; ii < m_graphicList.size(); ii++ )
            delete m_graphicList[ii];
    }

    /**
     * Set the filename to draw/plot
     * @param aFileName = the text to display by the "filename" format
     */
    void SetFileName( const wxString & aFileName )
    {
        m_fileName = aFileName;
    }

    /**
     * Set the sheet name to draw/plot
     * @param aSheetName = the text to draw/plot by the "sheetname" format
     */
    void SetSheetName( const wxString & aSheetName )
    {
        m_sheetFullName = &aSheetName;
    }

    /** Function SetPenSize
     * Set the default pen size to draw/plot lines and texts
     * @param aPenSize the thickness of lines
     */
    void SetPenSize( int aPenSize )
    {
        m_penSize = aPenSize;
    }

    /** Function SetMilsToIUfactor
     * Set the scalar to convert pages units ( mils) to draw/plot units
     * @param aScale the conversion factor
     */
    void SetMilsToIUfactor( double aScale )
    {
        m_milsToIu = aScale;
    }

    /** Function SetPageSize
     * Set the size of the page layout
     * @param aPageSize size (in mils) of the page layout.
     */
    void SetPageSize( const wxSize& aPageSize )
    {
        m_pageSize = aPageSize;
    }

    /**
     * Function SetSheetNumber
     * Set the value of the sheet number, for basic inscriptions
     * @param aSheetNumber the number to display.
     */
    void SetSheetNumber( int aSheetNumber )
    {
        m_sheetNumber = aSheetNumber;
    }

    /**
     * Function SetSheetCount
     * Set the value of the count of sheets, for basic inscriptions
     * @param aSheetCount the number of esheets to display.
     */
    void SetSheetCount( int aSheetCount )
    {
        m_sheetCount = aSheetCount;
    }

    /** Function SetMargins
     * Set the left top margin and the right bottom margin
     * of the page layout
     * @param aLTmargin The left top margin of the page layout.
     * @param aRBmargin The right bottom margin of the page layout.
     */
    void SetMargins( const wxPoint& aLTmargin, const wxPoint& aRBmargin )
    {
        m_LTmargin = aLTmargin;
        m_RBmargin = aRBmargin;
    }

    void Append( WS_DRAW_ITEM_BASE* aItem )
    {
        m_graphicList.push_back( aItem );
    }

    WS_DRAW_ITEM_BASE* GetFirst()
    {
        m_idx = 0;

        if( m_graphicList.size() )
            return m_graphicList[0];
        else
            return NULL;
    }

    WS_DRAW_ITEM_BASE* GetNext()
    {
        m_idx++;

        if( m_graphicList.size() > m_idx )
            return m_graphicList[m_idx];
        else
            return NULL;
    }

    /**
     * Draws the item list created by BuildWorkSheetGraphicList
     * @param aClipBox = the clipping rect, or NULL if no clipping
     * @param aDC = the current Device Context
     */
    void Draw( EDA_RECT* aClipBox, wxDC* aDC );

    /**
     * Function BuildWorkSheetGraphicList is a core function for
     * drawing or plotting the page layout with
     * the frame and the basic inscriptions.
     * It populates the list of basic graphic items to draw or plot.
     * currently lines, rect, polygons and texts
     * before calling this function, some parameters should be initialized:
     * by calling:
     *   SetPenSize( aPenWidth );
     *   SetMilsToIUfactor( aScalar );
     *   SetSheetNumber( aSheetNumber );
     *   SetSheetCount( aSheetCount );
     *   SetFileName( aFileName );
     *   SetSheetName( aFullSheetName );
     *
     * @param aPageInfo The PAGE_INFO, for page size, margins...
     * @param aTitleBlock The sheet title block, for basic inscriptions.
     * @param aColor The color for drawing.
     * @param aAltColor The color for items which need to be "hightlighted".
     */
    void BuildWorkSheetGraphicList( const PAGE_INFO& aPageInfo,
                                    const TITLE_BLOCK& aTitleBlock,
                                    EDA_COLOR_T aColor, EDA_COLOR_T aAltColor );
    /**
     * Function BuildFullText
     * returns the full text corresponding to the aTextbase,
     * after replacing format symbols by the corresponding value
     *
     * Basic texts in Ki_WorkSheetData struct use format notation
     * like "Title %T" to identify at run time the full text
     * to display.
     * Currently format identifier is % followed by a letter or 2 letters
     *
     * %% = replaced by %
     * %K = Kicad version
     * %Z = paper format name (A4, USLetter)
     * %Y = company name
     * %D = date
     * %R = revision
     * %S = sheet number
     * %N = number of sheets
     * %Cx = comment (x = 0 to 9 to identify the comment)
     * %F = filename
     * %P = sheet path or sheet full name
     * %T = title
     * Other fields like Developer, Verifier, Approver could use %Cx
     * and are seen as comments for format
     *
     * @param aTextbase = the text with format symbols
     * @return the text, after replacing the format symbols by the actual value
     */
    wxString BuildFullText( const wxString& aTextbase );

    /**
     * Locate graphic items in m_graphicList at location aPosition
     * @param aList = the list of items found
     * @param aPosition the position (in user units) to locate items
     */
    void Locate(std::vector <WS_DRAW_ITEM_BASE*>& aList, const wxPoint& aPosition);
};


/**
 * WORKSHEET_LAYOUT handles the graphic items list to draw/plot
 * the title block and other items (page references ...
 */
class WORKSHEET_LAYOUT
{
    std::vector <WORKSHEET_DATAITEM*> m_list;
    bool m_allowVoidList;   // If false, the default page layout
                            // will be loaded the first time
                            // WS_DRAW_ITEM_LIST::BuildWorkSheetGraphicList
                            // is run (useful mainly for page layout editor)
    double m_leftMargin;    // the left page margin in mm
    double m_rightMargin;   // the right page margin in mm
    double m_topMargin;     // the top page margin in mm
    double m_bottomMargin;  // the bottom page margin in mm

public:
    WORKSHEET_LAYOUT();
    ~WORKSHEET_LAYOUT() {ClearList(); }

    /**
     * static function: returns the instance of WORKSHEET_LAYOUT
     * used in the application
     */
    static WORKSHEET_LAYOUT& GetTheInstance()
    {
        extern WORKSHEET_LAYOUT wksTheInstance;
        return wksTheInstance;
    }

    // Accessors:
    double GetLeftMargin() { return m_leftMargin; }
    double GetRightMargin() { return m_rightMargin; }
    double GetTopMargin() { return m_topMargin; }
    double GetBottomMargin() { return m_bottomMargin; }

    void SetLeftMargin( double aMargin );
    void SetRightMargin( double aMargin );
    void SetTopMargin( double aMargin );
    void SetBottomMargin( double aMargin );

    /**
     * In Kicad applications, a page layout description is needed
     * So if the list is empty, a default description is loaded,
     * the first time a page layout is drawn.
     * However, in page layout editor, an empty list is acceptable.
     * AllowVoidList allows or not the empty list
     */
    void AllowVoidList( bool Allow ) { m_allowVoidList = Allow; }

    /**
     * @return true if an empty list is allowed
     * (mainly allowed for page layout editor).
     */
    bool VoidListAllowed() { return m_allowVoidList; }

    /**
     * erase the list of items
     */
    void ClearList();

    /**
     * Save the description in a file
     * @param aFullFileName the filename of the file to created
     */
    void Save( const wxString& aFullFileName );

    /**
     * Save the description in a buffer
     * @param aOutputString = a wxString to store the S expr string
     */
    void SaveInString( wxString& aOutputString );

    /**
     * Add an item to the list of items
     */
    void Append( WORKSHEET_DATAITEM* aItem )
    {
        m_list.push_back( aItem );
    }

    /**
     *Insert an item to the list of items at position aIdx
     */
    void Insert( WORKSHEET_DATAITEM* aItem, unsigned aIdx );

    /**
     *Remove the item to the list of items at position aIdx
     */
    bool  Remove( unsigned aIdx );

    /**
     *Remove the item to the list of items at position aIdx
     */
    bool  Remove( WORKSHEET_DATAITEM* aItem );

    /**
     * @return the index of aItem, or -1 if does not exist
     */
    int GetItemIndex( WORKSHEET_DATAITEM* aItem ) const;

    /**
     * @return the item from its index aIdx, or NULL if does not exist
     */
    WORKSHEET_DATAITEM* GetItem( unsigned aIdx ) const;

    /**
     * @return the item count
     */
    unsigned GetCount() const { return m_list.size(); }

    /**
     * Fills the list with the default layout shape
     */
    void SetDefaultLayout();

    /**
     * Populates the list with a custom layout, or
     * the default layout, if no custom layout available
     * @param aFullFileName = the custom page layout description file.
     * if empty, loads the file defined by KICAD_WKSFILE
     * and if its is not defined, uses the default internal description
     * @param Append = if true: do not delete old layout, and load only
       aFullFileName.
     */
    void SetPageLayout( const wxString& aFullFileName = wxEmptyString,
                        bool Append = false );

    /**
     * Populates the list from a S expr description stored in a string
     * @param aPageLayout = the S expr string
     * @param Append Do not delete old layout if true and append \a aPageLayout
     *               the existing one.
     */
    void SetPageLayout( const char* aPageLayout, bool Append = false );

    /**
     * @return a short filename  from a full filename:
     * if the path is the current path, or if the path
     * is the same as kicad.pro (in template), returns the shortname
     * else do nothing and returns a full filename
     */
    static const wxString MakeShortFileName( const wxString& aFullFileName );

    /**
     * @return a full filename from a short filename,
     * if the short filename path is void
     * In this case the path is the same as kicad.pro (in template)
     * else return the short filename (which have an absolute os relative path
      */
    static const wxString MakeFullFileName( const wxString& aShortFileName );
};

#endif      // WORKSHEET_SHAPE_BUILDER_H
