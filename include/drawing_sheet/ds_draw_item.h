/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
 *  - bitmaps (also for logos, but they cannot be plot by SVG or GERBER plotters
 *    where we just plot the bounding box)
 */
class DS_DRAW_ITEM_BASE : public EDA_ITEM
{
public:
    virtual ~DS_DRAW_ITEM_BASE() {}

    DS_DATA_ITEM* GetPeer() const { return m_peer; }
    int GetIndexInPeer() const { return m_index; }

    std::vector<int> ViewGetLayers() const override;

    virtual void SetEnd( const VECTOR2I& aPos ) { /* not all types will need this */ }

    virtual int GetPenWidth() const
    {
        if( m_penWidth > 0 )
            return m_penWidth;
        else
            return 1;
    }

    const KIFONT::METRICS& GetFontMetrics() const;

    // The function to print a WS_DRAW_ITEM
    virtual void PrintWsItem( const RENDER_SETTINGS* aSettings )
    {
        PrintWsItem( aSettings, VECTOR2I( 0, 0 ) );
    }

    // More advanced version of DrawWsItem. This is what must be defined in the derived type.
    virtual void PrintWsItem( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset ) = 0;

    // We can't cache bounding boxes because we're recreated for each draw event.  This method
    // can be overridden by items whose real bounding boxes are expensive to calculate.  It is
    // used to determine if we're in the current view, so it can be sloppy.
    virtual const BOX2I GetApproxBBox()
    {
        return GetBoundingBox();
    }

    // Derived types must define GetBoundingBox() as a minimum, and can then override the
    // two HitTest() functions if they need something more specific.
    const BOX2I GetBoundingBox() const override = 0;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override
    {
        // This is just here to prevent annoying compiler warnings about hidden overloaded
        // virtual functions
        return EDA_ITEM::HitTest( aPosition, aAccuracy );
    }

    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

protected:
    DS_DRAW_ITEM_BASE( DS_DATA_ITEM* aPeer, int aIndex, KICAD_T aType ) :
            EDA_ITEM( aType )
    {
        m_peer = aPeer;
        m_index = aIndex;
        m_penWidth = 0;
        m_flags = 0;
    }

protected:
    DS_DATA_ITEM*  m_peer;       // the parent DS_DATA_ITEM item in the DS_DATA_MODEL
    int            m_index;      // the index in the parent's repeat count
    int            m_penWidth;
};


// This class draws a thick segment
class DS_DRAW_ITEM_LINE : public DS_DRAW_ITEM_BASE
{
public:
    DS_DRAW_ITEM_LINE( DS_DATA_ITEM* aPeer, int aIndex, VECTOR2I aStart, VECTOR2I aEnd,
                       int aPenWidth ) :
            DS_DRAW_ITEM_BASE( aPeer, aIndex, WSG_LINE_T )
    {
        m_start     = aStart;
        m_end       = aEnd;
        m_penWidth  = aPenWidth;
    }

    virtual wxString GetClass() const override { return wxT( "DS_DRAW_ITEM_LINE" ); }

    const VECTOR2I& GetStart() const { return m_start; }
    void            SetStart( const VECTOR2I& aPos ) { m_start = aPos; }
    const VECTOR2I& GetEnd() const { return m_end; }
    void            SetEnd( const VECTOR2I& aPos ) override { m_end = aPos; }

    VECTOR2I GetPosition() const override { return GetStart(); }
    void     SetPosition( const VECTOR2I& aPos ) override { SetStart( aPos ); }

    const BOX2I GetBoundingBox() const override;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;

    void PrintWsItem( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset ) override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

private:
    VECTOR2I m_start; // start point of line/rect
    VECTOR2I m_end;   // end point
};


class DS_DRAW_ITEM_POLYPOLYGONS : public DS_DRAW_ITEM_BASE
{
public:
    DS_DRAW_ITEM_POLYPOLYGONS( DS_DATA_ITEM* aPeer, int aIndex, VECTOR2I aPos, int aPenWidth ) :
            DS_DRAW_ITEM_BASE( aPeer, aIndex, WSG_POLY_T )
    {
        m_penWidth = aPenWidth;
        m_pos = aPos;
    }

    virtual wxString GetClass() const override { return wxT( "DS_DRAW_ITEM_POLYPOLYGONS" ); }

    SHAPE_POLY_SET& GetPolygons() { return m_Polygons; }
    VECTOR2I        GetPosition() const override { return m_pos; }
    void            SetPosition( const VECTOR2I& aPos ) override;

    const BOX2I GetBoundingBox() const override;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    void PrintWsItem( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset ) override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

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
    VECTOR2I m_pos; // position of reference point, from the DS_DATA_ITEM_POLYGONS parent
                    // (used only in drawing sheet editor to draw anchors)
};


/**
 * Non filled rectangle with thick segment.
 */
class DS_DRAW_ITEM_RECT : public DS_DRAW_ITEM_BASE
{
public:
    DS_DRAW_ITEM_RECT( DS_DATA_ITEM* aPeer, int aIndex, VECTOR2I aStart, VECTOR2I aEnd,
                       int aPenWidth ) :
            DS_DRAW_ITEM_BASE( aPeer, aIndex, WSG_RECT_T )
    {
        m_start     = aStart;
        m_end       = aEnd;
        m_penWidth  = aPenWidth;
    }

    virtual wxString GetClass() const override { return wxT( "DS_DRAW_ITEM_RECT" ); }

    const VECTOR2I& GetStart() const { return m_start; }
    void            SetStart( const VECTOR2I& aPos ) { m_start = aPos; }
    const VECTOR2I& GetEnd() const { return m_end; }
    void            SetEnd( const VECTOR2I& aPos ) override { m_end = aPos; }

    VECTOR2I GetPosition() const override { return GetStart(); }
    void     SetPosition( const VECTOR2I& aPos ) override { SetStart( aPos ); }

    void PrintWsItem( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset ) override;

    const BOX2I GetBoundingBox() const override;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

private:
    VECTOR2I m_start; // start point of line/rect
    VECTOR2I m_end;   // end point
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

    void            SetPageSize( const VECTOR2I& aSize ) { m_pageSize = aSize; }
    VECTOR2I        GetPageSize() const { return m_pageSize; }

    const VECTOR2I& GetMarkerPos() const { return m_markerPos; }
    void            SetMarkerPos( const VECTOR2I& aPos ) { m_markerPos = aPos; }

    double GetMarkerSize() const { return m_markerSize; }

    VECTOR2I GetPosition() const override { return VECTOR2I( 0, 0 ); }
    void SetPosition( const VECTOR2I& aPos ) override { /* do nothing */ }

    void PrintWsItem( const RENDER_SETTINGS* , const VECTOR2I& ) override { /* do nothing */ }

    const BOX2I GetBoundingBox() const override;
    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override { return false; }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

private:
    VECTOR2I m_markerPos;   // position of the marker
    VECTOR2I m_pageSize;    // full size of the page
    double   m_markerSize;
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
    DS_DRAW_ITEM_TEXT( const EDA_IU_SCALE& aIuScale, DS_DATA_ITEM* aPeer, int aIndex,
                       const wxString& aText, const VECTOR2I& aPos, const VECTOR2I& aSize,
                       int aPenWidth, KIFONT::FONT* aFont,
                       bool aItalic = false, bool aBold = false,
                       const KIGFX::COLOR4D& aColor = KIGFX::COLOR4D::UNSPECIFIED ) :
            DS_DRAW_ITEM_BASE( aPeer, aIndex, WSG_TEXT_T),
            EDA_TEXT( aIuScale, aText )
    {
        SetTextPos( aPos );
        SetTextSize( aSize );
        SetTextThickness( aPenWidth );
        SetFont( aFont );
        SetItalic( aItalic );
        SetBold( aBold );
        SetTextColor( aColor );
    }

    virtual wxString GetClass() const override { return wxT( "DS_DRAW_ITEM_TEXT" ); }

    void PrintWsItem( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset ) override;

    VECTOR2I GetPosition() const override { return GetTextPos(); }
    void     SetPosition( const VECTOR2I& aPos ) override { SetTextPos( aPos ); }

    virtual const BOX2I GetApproxBBox() override;

    const BOX2I GetBoundingBox() const override;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

protected:
    const KIFONT::METRICS& getFontMetrics() const override { return GetFontMetrics(); }
};


/**
 * A bitmap.
 */
class DS_DRAW_ITEM_BITMAP : public DS_DRAW_ITEM_BASE
{
public:
    DS_DRAW_ITEM_BITMAP( DS_DATA_ITEM* aPeer, int aIndex, VECTOR2I aPos ) :
            DS_DRAW_ITEM_BASE( aPeer, aIndex, WSG_BITMAP_T )
    {
        m_pos = aPos;
    }

    ~DS_DRAW_ITEM_BITMAP() {}

    virtual wxString GetClass() const override { return wxT( "DS_DRAW_ITEM_BITMAP" ); }

    VECTOR2I GetPosition() const override { return m_pos; }
    void     SetPosition( const VECTOR2I& aPos ) override { m_pos = aPos; }

    void PrintWsItem( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset ) override;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    const BOX2I GetBoundingBox() const override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

private:
    VECTOR2I m_pos; // position of reference point
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
    DS_DRAW_ITEM_LIST( const EDA_IU_SCALE& aIuScale, int aFlags = 0 ) :
        m_iuScale( aIuScale )
    {
        m_idx = 0;
        m_plotterMilsToIu = 0.0;
        m_penSize = 1;
        m_pageNumber = "1";
        m_sheetCount = 1;
        m_titleBlock = nullptr;
        m_project = nullptr;
        m_isFirstPage = true;
        m_flags = aFlags;
        m_properties = nullptr;
    }

    ~DS_DRAW_ITEM_LIST()
    {
        // Items in the m_graphicList are owned by their respective DS_DATA_ITEMs.
        // for( DS_DRAW_ITEM_BASE* item : m_graphicList )
        //     delete item;
    }

    void SetProject( const PROJECT* aProject ) { m_project = aProject; }

    /**
     * Set the title block (mainly for drawing sheet editor)
     */
    void SetTitleBlock( const TITLE_BLOCK* aTblock ) { m_titleBlock = aTblock; }

    /**
     * Set properties used for text variable resolution.
     */
    void SetProperties( const std::map<wxString, wxString>* aProps ) { m_properties = aProps; }

    /**
     * Set the paper format name (mainly for drawing sheet editor)
     */
    void SetPaperFormat( const wxString& aFormatName ) { m_paperFormat = aFormatName; }

    /**
     * Set the filename to draw/plot
     */
    void SetFileName( const wxString& aFileName ) { m_fileName = aFileName; }

    /**
     * Set the sheet name to draw/plot
     */
    void SetSheetName( const wxString& aSheetName ) { m_sheetName = aSheetName; }

    /**
     * Set the sheet path to draw/plot
     */
    void SetSheetPath( const wxString& aSheetPath ) { m_sheetPath = aSheetPath; }

    /**
     * Set the sheet layer to draw/plot
     */
    void SetSheetLayer( const wxString& aSheetLayer ) { m_sheetLayer = aSheetLayer;  }

    void SetDefaultPenSize( int aPenSize ) { m_penSize = aPenSize; }
    int GetDefaultPenSize() const { return m_penSize; }

    /**
     * Set the scalar to convert pages units (mils) to plot units.
     */
    void SetPlotterMilsToIUfactor( double aMils2Iu ) { m_plotterMilsToIu = aMils2Iu; }

    /**
     * Get the scalar to convert pages units (mils) to draw/plot units.
     *
     * This will be controlled by EITHER the parent frame's EDA_IU_SCALE or the plotter's
     * mils-to-iu factor.
     */
    double GetMilsToIUfactor()
    {
        if( m_plotterMilsToIu > 0.0 )
            return m_plotterMilsToIu;
        else
            return m_iuScale.IU_PER_MILS;
    }

    const EDA_IU_SCALE& GetIuScale() const { return m_iuScale; }

    /**
     * Set the value of the sheet number.
     */
    void SetPageNumber( const wxString& aPageNumber ) { m_pageNumber = aPageNumber; }

    /**
     * Set if the page is the first page.
     */
    void SetIsFirstPage( bool aIsFirstPage ) { m_isFirstPage = aIsFirstPage; }

    /**
     * Set the value of the count of sheets, for basic inscriptions
     */
    void SetSheetCount( int aSheetCount ) { m_sheetCount = aSheetCount; }

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
            return nullptr;
    }

    DS_DRAW_ITEM_BASE* GetNext()
    {
        m_idx++;

        if( m_graphicList.size() > m_idx )
            return m_graphicList[m_idx];
        else
            return nullptr;
    }

    /**
     * Draws the item list created by BuildDrawItemsList
     */
    void Print( const RENDER_SETTINGS* aSettings );

    /**
     * Drawing or plot the drawing sheet.
     *
     * Before calling this function, some parameters should be initialized by calling:
     *   SetPenSize( aPenWidth );
     *   SetSheetNumber( aSheetNumber );
     *   SetSheetCount( aSheetCount );
     *   SetFileName( aFileName );
     *   SetSheetName( aSheetName );
     *   SetSheetPath( aSheetPath );
     *
     * @param aPageInfo The PAGE_INFO, for page size, margins...
     * @param aTitleBlock The sheet title block, for basic inscriptions.
     * @param aColor The color for drawing.
     * @param aAltColor The color for items which need to be "highlighted".
     */
    void BuildDrawItemsList( const PAGE_INFO& aPageInfo, const TITLE_BLOCK& aTitleBlock );

    static void GetTextVars( wxArrayString* aVars );

    /**
     * @return the full text corresponding to the aTextbase, after replacing any text variable
     *         references.
     */
    wxString BuildFullText( const wxString& aTextbase );

protected:
    std::vector <DS_DRAW_ITEM_BASE*> m_graphicList;     // Items to draw/plot
    const EDA_IU_SCALE&              m_iuScale;         // IU scale for drawing
    double                           m_plotterMilsToIu; // IU scale for plotting

    unsigned           m_idx;             // for GetFirst, GetNext functions
    int                m_penSize;         // The default line width for drawings.
                                          // used when an item has a pen size = 0
    bool               m_isFirstPage;     ///< Is this the first page or not.
    int                m_sheetCount;      ///< The number of sheets
                                          // for text variable references, in schematic
    const TITLE_BLOCK* m_titleBlock;      // for text variable references
    wxString           m_paperFormat;     // for text variable references
    wxString           m_fileName;        // for text variable references
    wxString           m_sheetName;       // for text variable references
    wxString           m_sheetPath;       // for text variable references
    wxString           m_pageNumber;      ///< The actual page number displayed in the title block.
    wxString           m_sheetLayer;      // for text variable references
    const PROJECT*     m_project;         // for project-based text variable references
    int                m_flags;

    const std::map<wxString, wxString>* m_properties;    // for text variable references
};


#endif      // DS_DRAW_ITEM_H
