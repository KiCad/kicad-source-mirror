/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef  WS_DRAW_ITEM_H
#define  WS_DRAW_ITEM_H

#include <math/vector2d.h>
#include <eda_text.h>
#include <eda_text.h>
#include <bitmap_base.h>
#include "msgpanel.h"
#include <geometry/shape_poly_set.h>

class WS_DATA_ITEM;
class TITLE_BLOCK;
class PAGE_INFO;

/*
 * Helper classes to handle basic graphic items used to draw/plot
 *   title blocks and frame references
 *   segments
 *   rect
 *   polygons (for logos)
 *   graphic texts
 *   bitmaps (also for logos, but they cannot be plot by SVG, GERBER or HPGL plotters
 *            where we just plot the bounding box)
 */
class WS_DRAW_ITEM_BASE : public EDA_ITEM     // This basic class, not directly usable.
{
protected:
    WS_DATA_ITEM*  m_peer;       // the parent WS_DATA_ITEM item in the WS_DATA_MODEL
    int            m_index;      // the index in the parent's repeat count

    WS_DRAW_ITEM_BASE( WS_DATA_ITEM* aPeer, int aIndex, KICAD_T aType ) :
            EDA_ITEM( aType )
    {
        m_peer = aPeer;
        m_index = aIndex;
        m_Flags = 0;
    }

public:
    virtual ~WS_DRAW_ITEM_BASE() {}

    WS_DATA_ITEM* GetPeer() const { return m_peer; }
    int GetIndexInPeer() const { return m_index; }

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    virtual const wxPoint GetPosition() const = 0;
    virtual void SetPosition( wxPoint aPos ) = 0;
    virtual void SetEnd( wxPoint aPos ) { /* not all types will need this */ }

    // The function to print a WS_DRAW_ITEM
    virtual void PrintWsItem( wxDC* aDC, COLOR4D aColor )
    {
        PrintWsItem( aDC, wxPoint( 0, 0 ), aColor );
    }

    // More advanced version of DrawWsItem. This is what must be defined in the derived type.
    virtual void PrintWsItem( wxDC* aDC, const wxPoint& aOffset, COLOR4D aColor ) = 0;

    // Derived types must define GetBoundingBox() as a minimum, and can then override the
    // two HitTest() functions if they need something more specific.
    const EDA_RECT GetBoundingBox() const override = 0;

    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override
    {
        // This is just here to prevent annoying compiler warnings about hidden overloaded
        // virtual functions
        return EDA_ITEM::HitTest( aPosition, aAccuracy );
    }

    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    void GetMsgPanelInfo( EDA_UNITS_T aUnits, MSG_PANEL_ITEMS& aList ) override;
};


// This class draws a thick segment
class WS_DRAW_ITEM_LINE : public WS_DRAW_ITEM_BASE
{
    wxPoint m_start;    // start point of line/rect
    wxPoint m_end;      // end point
    int     m_penWidth;

public:
    WS_DRAW_ITEM_LINE( WS_DATA_ITEM* aPeer, int aIndex, wxPoint aStart, wxPoint aEnd,
                       int aPenWidth ) :
        WS_DRAW_ITEM_BASE( aPeer, aIndex, WSG_LINE_T )
    {
        m_start     = aStart;
        m_end       = aEnd;
        m_penWidth  = aPenWidth;
    }

    virtual wxString GetClass() const override { return wxT( "WS_DRAW_ITEM_LINE" ); }

    // Accessors:
    int GetPenWidth() const { return m_penWidth; }
    const wxPoint&  GetStart() const { return m_start; }
    void SetStart( wxPoint aPos ) { m_start = aPos; }
    const wxPoint&  GetEnd() const { return m_end; }
    void SetEnd( wxPoint aPos ) override { m_end = aPos; }

    const wxPoint GetPosition() const override { return GetStart(); }
    void SetPosition( wxPoint aPos ) override { SetStart( aPos ); }

    const EDA_RECT GetBoundingBox() const override;
    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;

    void PrintWsItem( wxDC* aDC, const wxPoint& aOffset, COLOR4D aColor ) override;

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif
};

// This class draws a polygon
class WS_DRAW_ITEM_POLYPOLYGONS : public WS_DRAW_ITEM_BASE
{
    wxPoint m_pos;      // position of reference point, from the
                        // WS_DATA_ITEM_POLYGONS parent
                        // (used only in page layout editor to draw anchors)
    int m_penWidth;

public:
    /** The list of polygons. Because these polygons are only for drawing purposes,
     * each polygon is expected having no holes, jusst a main outline
     */
    SHAPE_POLY_SET m_Polygons;

public:
    WS_DRAW_ITEM_POLYPOLYGONS( WS_DATA_ITEM* aPeer, int aIndex, wxPoint aPos, int aPenWidth ) :
            WS_DRAW_ITEM_BASE( aPeer, aIndex, WSG_POLY_T )
    {
        m_penWidth = aPenWidth;
        m_pos = aPos;
    }

    virtual wxString GetClass() const override { return wxT( "WS_DRAW_ITEM_POLYPOLYGONS" ); }

    // Accessors:
    SHAPE_POLY_SET& GetPolygons() { return m_Polygons; }
    int GetPenWidth() const { return m_penWidth; }
    const wxPoint GetPosition() const override { return m_pos; }
    void SetPosition( wxPoint aPos ) override;

    const EDA_RECT GetBoundingBox() const override;
    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    void PrintWsItem( wxDC* aDC, const wxPoint& aOffset, COLOR4D aColor ) override;

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif
};

// This class draws a not filled rectangle with thick segment
class WS_DRAW_ITEM_RECT : public WS_DRAW_ITEM_BASE
{
    wxPoint m_start;    // start point of line/rect
    wxPoint m_end;      // end point
    int     m_penWidth;

public:
    WS_DRAW_ITEM_RECT( WS_DATA_ITEM* aPeer, int aIndex, wxPoint aStart, wxPoint aEnd,
                       int aPenWidth ) :
            WS_DRAW_ITEM_BASE( aPeer, aIndex, WSG_RECT_T )
    {
        m_start     = aStart;
        m_end       = aEnd;
        m_penWidth  = aPenWidth;
    }

    virtual wxString GetClass() const override { return wxT( "WS_DRAW_ITEM_RECT" ); }

    // Accessors:
    int GetPenWidth() const { return m_penWidth; }
    const wxPoint&  GetStart() const { return m_start; }
    void SetStart( wxPoint aPos ) { m_start = aPos; }
    const wxPoint&  GetEnd() const { return m_end; }
    void SetEnd( wxPoint aPos ) override { m_end = aPos; }

    const wxPoint GetPosition() const override { return GetStart(); }
    void SetPosition( wxPoint aPos ) override { SetStart( aPos ); }

    void PrintWsItem( wxDC* aDC, const wxPoint& aOffset, COLOR4D aColor ) override;

    const EDA_RECT GetBoundingBox() const override;
    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif
};


// This class draws a rectangle with thick segment showing the page limits
// and a marker showing the coord origin. This only a draw item only.
// Therefore m_peer ( the parent WS_DATA_ITEM item in the WS_DATA_MODEL) is always a nullptr.
class WS_DRAW_ITEM_PAGE : public WS_DRAW_ITEM_BASE
{
    wxPoint m_markerPos;    // position of the marker
    wxSize  m_pageSize;     // full size of the page
    int     m_penWidth;
    double m_markerSize;

public:
    WS_DRAW_ITEM_PAGE( int aPenWidth, double aMarkerSize ) :
            WS_DRAW_ITEM_BASE( nullptr, 0, WSG_PAGE_T )
    {
        m_penWidth  = aPenWidth;
        m_markerSize = aMarkerSize;
    }

    virtual wxString GetClass() const override { return wxT( "WS_DRAW_ITEM_PAGE" ); }

    // Accessors:
    int GetPenWidth() const { return m_penWidth; }
    void SetPageSize( wxSize aSize ) { m_pageSize = aSize; }
    wxSize GetPageSize() const { return m_pageSize; }
    const wxPoint& GetMarkerPos() const { return m_markerPos; }
    void SetMarkerPos( wxPoint aPos ) { m_markerPos = aPos; }
    double GetMarkerSize() const { return m_markerSize; }

    const wxPoint GetPosition() const override { return wxPoint( 0, 0 ); }
    void SetPosition( wxPoint aPos ) override { /* do nothing */ }

    void PrintWsItem( wxDC* aDC, const wxPoint& aOffset, COLOR4D aColor ) override { /* do nothing */ }

    const EDA_RECT GetBoundingBox() const override;
    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override { return false; }

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif
};

// This class draws a graphic text.
// it is derived from an EDA_TEXT, so it handle all caracteristics
// of this graphic text (justification, rotation ... )
class WS_DRAW_ITEM_TEXT : public WS_DRAW_ITEM_BASE, public EDA_TEXT
{
public:
    WS_DRAW_ITEM_TEXT( WS_DATA_ITEM* aPeer, int aIndex, wxString& aText, wxPoint aPos,
                       wxSize aSize, int aPenWidth, bool aItalic = false, bool aBold = false ) :
            WS_DRAW_ITEM_BASE( aPeer, aIndex, WSG_TEXT_T),
            EDA_TEXT( aText )
    {
        SetTextPos( aPos );
        SetTextSize( aSize );
        SetThickness( aPenWidth );
        SetItalic( aItalic );
        SetBold( aBold );
    }

    virtual wxString GetClass() const override { return wxT( "WS_DRAW_ITEM_TEXT" ); }

    void PrintWsItem( wxDC* aDC, const wxPoint& aOffset, COLOR4D aColor ) override;

    // Accessors:
    int GetPenWidth() { return GetThickness(); }

    void SetTextAngle( double aAngle )
    {
        EDA_TEXT::SetTextAngle( NormalizeAngle360Min( aAngle ) );
    }

    const wxPoint GetPosition() const override { return GetTextPos(); }
    void SetPosition( wxPoint aPos ) override { SetTextPos( aPos ); }

    const EDA_RECT GetBoundingBox() const override;
    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif
};

// This class draws a bitmap.
class WS_DRAW_ITEM_BITMAP : public WS_DRAW_ITEM_BASE
{
    wxPoint m_pos;                  // position of reference point

public:
    WS_DRAW_ITEM_BITMAP( WS_DATA_ITEM* aPeer, int aIndex, wxPoint aPos ) :
            WS_DRAW_ITEM_BASE( aPeer, aIndex, WSG_BITMAP_T )
    {
        m_pos = aPos;
    }

    ~WS_DRAW_ITEM_BITMAP() {}

    virtual wxString GetClass() const override { return wxT( "WS_DRAW_ITEM_BITMAP" ); }

    const wxPoint GetPosition() const override { return m_pos; }
    void SetPosition( wxPoint aPos ) override { m_pos = aPos; }

    void PrintWsItem( wxDC* aDC, const wxPoint& aOffset, COLOR4D aColor ) override;

    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;
    const EDA_RECT GetBoundingBox() const override;

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif
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
    wxString        m_sheetFullName;    // for basic inscriptions
    const wxString* m_sheetLayer;       // for basic inscriptions


public:
    WS_DRAW_ITEM_LIST()
    {
        m_idx = 0;
        m_milsToIu = 1.0;
        m_penSize = 1;
        m_sheetNumber = 1;
        m_sheetCount = 1;
        m_sheetLayer = nullptr;
        m_titleBlock = nullptr;
        m_paperFormat = nullptr;
    }

    ~WS_DRAW_ITEM_LIST()
    {
        // Items in the m_graphicList are owned by their respective WORKSHEET_DATAITEMs.
        // for( WS_DRAW_ITEM_BASE* item : m_graphicList )
        //     delete item;
    }

    /**
     * Set the title block (mainly for page layout editor)
     */
    void SetTitleBlock( const TITLE_BLOCK* aTblock ) { m_titleBlock = aTblock; }

    /**
     * Set the paper format name (mainly for page layout editor)
     */
    void SetPaperFormat( const wxString* aFormatName ) { m_paperFormat = aFormatName; }

    /**
     * Set the filename to draw/plot
     */
    void SetFileName( const wxString& aFileName )
    {
        m_fileName = aFileName;
    }

    /**
     * Set the sheet name to draw/plot
     */
    void SetSheetName( const wxString& aSheetName )
    {
        m_sheetFullName = aSheetName;
    }

    /**
     * Set the sheet layer to draw/plot
     */
    void SetSheetLayer( const wxString& aSheetLayer )
    {
        m_sheetLayer = &aSheetLayer;
    }

    void SetDefaultPenSize( int aPenSize ) { m_penSize = aPenSize; }
    int GetDefaultPenSize() const { return m_penSize; }

    /**
     * Function SetMilsToIUfactor
     * Set the scalar to convert pages units (mils) to draw/plot units
     */
    void SetMilsToIUfactor( double aScale )
    {
        m_milsToIu = aScale;
    }

    /**
     * Function SetSheetNumber
     * Set the value of the sheet number, for basic inscriptions
     */
    void SetSheetNumber( int aSheetNumber )
    {
        m_sheetNumber = aSheetNumber;
    }

    /**
     * Function SetSheetCount
     * Set the value of the count of sheets, for basic inscriptions
     */
    void SetSheetCount( int aSheetCount )
    {
        m_sheetCount = aSheetCount;
    }

    void Append( WS_DRAW_ITEM_BASE* aItem )
    {
        m_graphicList.push_back( aItem );
    }

    void Remove( WS_DRAW_ITEM_BASE* aItem )
    {
        auto newEnd = std::remove( m_graphicList.begin(), m_graphicList.end(), aItem );
        m_graphicList.erase( newEnd, m_graphicList.end() );
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

    void GetAllItems( std::vector<WS_DRAW_ITEM_BASE*>* aList )
    {
        *aList = m_graphicList;
    }

    /**
     * Draws the item list created by BuildWorkSheetGraphicList
     * @param aDC = the current Device Context
     */
    void Print( wxDC* aDC, COLOR4D aColor );

    /**
     * Function BuildWorkSheetGraphicList is a core function for drawing or plotting the
     * page layout with the frame and the basic inscriptions.
     *
     * Before calling this function, some parameters should be initialized by calling:
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
     * @param aAltColor The color for items which need to be "highlighted".
     */
    void BuildWorkSheetGraphicList( const PAGE_INFO& aPageInfo, const TITLE_BLOCK& aTitleBlock );

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
};


#endif      // WS_DRAW_ITEM_H
