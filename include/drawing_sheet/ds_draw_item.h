/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef  DS_DRAW_ITEM_H
#define  DS_DRAW_ITEM_H

#include <core/typeinfo.h>
#include <math/vector2d.h>
#include <eda_text.h>
#include "widgets/msgpanel.h"
#include <geometry/shape_poly_set.h>
#include <eda_item.h>
#include <eda_units.h>

#include <algorithm>
#include <vector>

class DS_DATA_ITEM;
class TITLE_BLOCK;
class PAGE_INFO;
class EDA_ITEM;
class EDA_DRAW_FRAME;
class PROJECT;

/**
 * Base class to handle basic graphic items.
 *
 * Used to draw and/or plot:
 *  - title blocks and frame references
 *  - segments
 *  - rect
 *  - polygons (for logos)
 *  - graphic texts
 *  - bitmaps (also for logos, but they cannot be plot by SVG, GERBER or HPGL plotters
 *    where we just plot the bounding box)
 */
class DS_DRAW_ITEM_BASE : public EDA_ITEM
{
public:
    virtual ~DS_DRAW_ITEM_BASE() {}

    DS_DATA_ITEM* GetPeer() const { return m_peer; }
    int GetIndexInPeer() const { return m_index; }

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    virtual void SetEnd( wxPoint aPos ) { /* not all types will need this */ }

    virtual int GetPenWidth() const
    {
        if( m_penWidth > 0 )
            return m_penWidth;
        else
            return 1;
    }

    // The function to print a WS_DRAW_ITEM
    virtual void PrintWsItem( const RENDER_SETTINGS* aSettings )
    {
        PrintWsItem( aSettings, wxPoint( 0, 0 ) );
    }

    // More advanced version of DrawWsItem. This is what must be defined in the derived type.
    virtual void PrintWsItem( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset ) = 0;

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

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, MSG_PANEL_ITEMS& aList ) override;

protected:
    DS_DRAW_ITEM_BASE( DS_DATA_ITEM* aPeer, int aIndex, KICAD_T aType ) :
            EDA_ITEM( aType )
    {
        m_peer = aPeer;
        m_index = aIndex;
        m_penWidth = 0;
        m_flags = 0;
    }

    DS_DATA_ITEM*  m_peer;       // the parent DS_DATA_ITEM item in the DS_DATA_MODEL
    int            m_index;      // the index in the parent's repeat count
    int            m_penWidth;
};


// This class draws a thick segment
class DS_DRAW_ITEM_LINE : public DS_DRAW_ITEM_BASE
{
public:
    DS_DRAW_ITEM_LINE( DS_DATA_ITEM* aPeer, int aIndex, wxPoint aStart, wxPoint aEnd,
                       int aPenWidth ) :
            DS_DRAW_ITEM_BASE( aPeer, aIndex, WSG_LINE_T )
    {
        m_start     = aStart;
        m_end       = aEnd;
        m_penWidth  = aPenWidth;
    }

    virtual wxString GetClass() const override { return wxT( "DS_DRAW_ITEM_LINE" ); }

    const wxPoint&  GetStart() const { return m_start; }
    void SetStart( wxPoint aPos ) { m_start = aPos; }
    const wxPoint&  GetEnd() const { return m_end; }
    void SetEnd( wxPoint aPos ) override { m_end = aPos; }

    wxPoint GetPosition() const override { return GetStart(); }
    void SetPosition( const wxPoint& aPos ) override { SetStart( aPos ); }

    const EDA_RECT GetBoundingBox() const override;
    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;

    void PrintWsItem( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset ) override;

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

private:
    wxPoint m_start;    // start point of line/rect
    wxPoint m_end;      // end point
};


class DS_DRAW_ITEM_POLYPOLYGONS : public DS_DRAW_ITEM_BASE
{
public:
    DS_DRAW_ITEM_POLYPOLYGONS( DS_DATA_ITEM* aPeer, int aIndex, wxPoint aPos, int aPenWidth ) :
            DS_DRAW_ITEM_BASE( aPeer, aIndex, WSG_POLY_T )
    {
        m_penWidth = aPenWidth;
        m_pos = aPos;
    }

    virtual wxString GetClass() const override { return wxT( "DS_DRAW_ITEM_POLYPOLYGONS" ); }

    SHAPE_POLY_SET& GetPolygons() { return m_Polygons; }
    wxPoint GetPosition() const override { return m_pos; }
    void SetPosition( const wxPoint& aPos ) override;

    const EDA_RECT GetBoundingBox() const override;
    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    void PrintWsItem( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset ) override;

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

public:
    /**
     * The list of polygons.
     *
     * Because these polygons are only for drawing purposes, each polygon is expected to
     * have no holes just a main outline.
     */
    SHAPE_POLY_SET m_Polygons;


private:
    wxPoint m_pos;      // position of reference point, from the DS_DATA_ITEM_POLYGONS parent
                        // (used only in drawing sheet editor to draw anchors)
};


/**
 * Non filled rectangle with thick segment.
 */
class DS_DRAW_ITEM_RECT : public DS_DRAW_ITEM_BASE
{
public:
    DS_DRAW_ITEM_RECT( DS_DATA_ITEM* aPeer, int aIndex, wxPoint aStart, wxPoint aEnd,
                       int aPenWidth ) :
            DS_DRAW_ITEM_BASE( aPeer, aIndex, WSG_RECT_T )
    {
        m_start     = aStart;
        m_end       = aEnd;
        m_penWidth  = aPenWidth;
    }

    virtual wxString GetClass() const override { return wxT( "DS_DRAW_ITEM_RECT" ); }

    const wxPoint&  GetStart() const { return m_start; }
    void SetStart( wxPoint aPos ) { m_start = aPos; }
    const wxPoint&  GetEnd() const { return m_end; }
    void SetEnd( wxPoint aPos ) override { m_end = aPos; }

    wxPoint GetPosition() const override { return GetStart(); }
    void SetPosition( const wxPoint& aPos ) override { SetStart( aPos ); }

    void PrintWsItem( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset ) override;

    const EDA_RECT GetBoundingBox() const override;
    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

private:
    wxPoint m_start;    // start point of line/rect
    wxPoint m_end;      // end point
};


/**
 * A rectangle with thick segment showing the page limits and a marker showing the coordinate
 * origin.
 *
 * This only a draw item only.  Therefore m_peer ( the parent DS_DATA_ITEM item in the
 * DS_DATA_MODEL) is always a nullptr.
 */
class DS_DRAW_ITEM_PAGE : public DS_DRAW_ITEM_BASE
{
public:
    DS_DRAW_ITEM_PAGE( int aPenWidth, double aMarkerSize ) :
            DS_DRAW_ITEM_BASE( nullptr, 0, WSG_PAGE_T )
    {
        m_penWidth  = aPenWidth;
        m_markerSize = aMarkerSize;
    }

    virtual wxString GetClass() const override { return wxT( "DS_DRAW_ITEM_PAGE" ); }

    void SetPageSize( wxSize aSize ) { m_pageSize = aSize; }
    wxSize GetPageSize() const { return m_pageSize; }
    const wxPoint& GetMarkerPos() const { return m_markerPos; }
    void SetMarkerPos( wxPoint aPos ) { m_markerPos = aPos; }
    double GetMarkerSize() const { return m_markerSize; }

    wxPoint GetPosition() const override { return wxPoint( 0, 0 ); }
    void SetPosition( const wxPoint& aPos ) override { /* do nothing */ }

    void PrintWsItem( const RENDER_SETTINGS* , const wxPoint& ) override { /* do nothing */ }

    const EDA_RECT GetBoundingBox() const override;
    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override { return false; }

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

private:
    wxPoint m_markerPos;    // position of the marker
    wxSize  m_pageSize;     // full size of the page
    double m_markerSize;
};


/**
 * A graphic text.
 *
 * It is derived from an #EDA_TEXT, so it handle all characteristics of this graphic text
 * (justification, rotation ... ).
 */
class DS_DRAW_ITEM_TEXT : public DS_DRAW_ITEM_BASE, public EDA_TEXT
{
public:
    DS_DRAW_ITEM_TEXT( DS_DATA_ITEM* aPeer, int aIndex, wxString& aText, wxPoint aPos,
                       wxSize aSize, int aPenWidth, bool aItalic = false,
                       bool aBold = false ) :
            DS_DRAW_ITEM_BASE( aPeer, aIndex, WSG_TEXT_T),
            EDA_TEXT( aText )
    {
        SetTextPos( aPos );
        SetTextSize( aSize );
        SetTextThickness( aPenWidth );
        SetItalic( aItalic );
        SetBold( aBold );
    }

    virtual wxString GetClass() const override { return wxT( "DS_DRAW_ITEM_TEXT" ); }

    void PrintWsItem( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset ) override;

    void SetTextAngle( double aAngle ) override;

    wxPoint GetPosition() const override { return GetTextPos(); }
    void SetPosition( const wxPoint& aPos ) override { SetTextPos( aPos ); }

    const EDA_RECT GetBoundingBox() const override;
    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif
};


/**
 * A bitmap.
 */
class DS_DRAW_ITEM_BITMAP : public DS_DRAW_ITEM_BASE
{
public:
    DS_DRAW_ITEM_BITMAP( DS_DATA_ITEM* aPeer, int aIndex, wxPoint aPos ) :
            DS_DRAW_ITEM_BASE( aPeer, aIndex, WSG_BITMAP_T )
    {
        m_pos = aPos;
    }

    ~DS_DRAW_ITEM_BITMAP() {}

    virtual wxString GetClass() const override { return wxT( "DS_DRAW_ITEM_BITMAP" ); }

    wxPoint GetPosition() const override { return m_pos; }
    void SetPosition( const wxPoint& aPos ) override { m_pos = aPos; }

    void PrintWsItem( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset ) override;

    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;
    const EDA_RECT GetBoundingBox() const override;

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

private:
    wxPoint m_pos;                  // position of reference point
};


/**
 * Store the list of graphic items:
 * rect, lines, polygons and texts to draw/plot
 * the title block and frame references, and parameters to
 * draw/plot them
 */
class DS_DRAW_ITEM_LIST
{
public:
    DS_DRAW_ITEM_LIST()
    {
        m_idx = 0;
        m_milsToIu = 1.0;
        m_penSize = 1;
        m_pageNumber = "1";
        m_sheetCount = 1;
        m_sheetLayer = nullptr;
        m_titleBlock = nullptr;
        m_paperFormat = nullptr;
        m_project = nullptr;
        m_isFirstPage = true;
    }

    ~DS_DRAW_ITEM_LIST()
    {
        // Items in the m_graphicList are owned by their respective WORKSHEET_DATAITEMs.
        // for( DS_DRAW_ITEM_BASE* item : m_graphicList )
        //     delete item;
    }

    void SetProject( const PROJECT* aProject ) { m_project = aProject; }

    /**
     * Set the title block (mainly for drawing sheet editor)
     */
    void SetTitleBlock( const TITLE_BLOCK* aTblock ) { m_titleBlock = aTblock; }

    /**
     * Set the paper format name (mainly for drawing sheet editor)
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
     * Set the scalar to convert pages units (mils) to draw/plot units
     */
    void SetMilsToIUfactor( double aMils2Iu )
    {
        m_milsToIu = aMils2Iu;
    }

    /**
     * Get the scalar to convert pages units (mils) to draw/plot units
     */
    double GetMilsToIUfactor() { return m_milsToIu; }

    /**
     * Set the value of the sheet number.
     */
    void SetPageNumber( const wxString& aPageNumber )
    {
        m_pageNumber = aPageNumber;
    }

    /**
     * Set if the page is the first page.
     */
    void SetIsFirstPage( bool aIsFirstPage ) { m_isFirstPage = aIsFirstPage; }

    /**
     * Set the value of the count of sheets, for basic inscriptions
     */
    void SetSheetCount( int aSheetCount )
    {
        m_sheetCount = aSheetCount;
    }

    void Append( DS_DRAW_ITEM_BASE* aItem )
    {
        m_graphicList.push_back( aItem );
    }

    void Remove( DS_DRAW_ITEM_BASE* aItem )
    {
        auto newEnd = std::remove( m_graphicList.begin(), m_graphicList.end(), aItem );
        m_graphicList.erase( newEnd, m_graphicList.end() );
    }

    DS_DRAW_ITEM_BASE* GetFirst()
    {
        m_idx = 0;

        if( m_graphicList.size() )
            return m_graphicList[0];
        else
            return NULL;
    }

    DS_DRAW_ITEM_BASE* GetNext()
    {
        m_idx++;

        if( m_graphicList.size() > m_idx )
            return m_graphicList[m_idx];
        else
            return NULL;
    }

    void GetAllItems( std::vector<DS_DRAW_ITEM_BASE*>* aList )
    {
        *aList = m_graphicList;
    }

    /**
     * Draws the item list created by BuildDrawItemsList
     */
    void Print( const RENDER_SETTINGS* aSettings );

    /**
     * Drawing or plot the page layout.
     *
     * Before calling this function, some parameters should be initialized by calling:
     *   SetPenSize( aPenWidth );
     *   SetMilsToIUfactor( aMils2Iu );
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
    void BuildDrawItemsList( const PAGE_INFO& aPageInfo, const TITLE_BLOCK& aTitleBlock );

    static void GetTextVars( wxArrayString* aVars );

    /**
     * Return the full text corresponding to the aTextbase,
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

protected:
    std::vector <DS_DRAW_ITEM_BASE*> m_graphicList;     // Items to draw/plot
    unsigned           m_idx;             // for GetFirst, GetNext functions
    double             m_milsToIu;        // the scalar to convert pages units ( mils)
                                          // to draw/plot units.
    int                m_penSize;         // The default line width for drawings.
                                          // used when an item has a pen size = 0
    bool               m_isFirstPage;     ///< Is this the first page or not.
    int                m_sheetCount;      ///< The number of sheets
                                          // for basic inscriptions, in schematic
    const TITLE_BLOCK* m_titleBlock;      // for basic inscriptions
    const wxString*    m_paperFormat;     // for basic inscriptions
    wxString           m_fileName;        // for basic inscriptions
    wxString           m_sheetFullName;   // for basic inscriptions
    wxString           m_pageNumber;      ///< The actual page number displayed in the title block.
    const wxString*    m_sheetLayer;      // for basic inscriptions
    const PROJECT*     m_project;         // for project-based variable substitutions
};


#endif      // DS_DRAW_ITEM_H
