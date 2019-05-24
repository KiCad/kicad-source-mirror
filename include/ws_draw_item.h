/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

class WORKSHEET_DATAITEM;
class TITLE_BLOCK;
class PAGE_INFO;

#define TB_DEFAULT_TEXTSIZE             1.5  // default worksheet text size in mm

/*
 * Helper classes to handle basic graphic items used to draw/plot
 * title blocks and frame references
 * segments
 * rect
 * polygons (for logos)
 * graphic texts
 * bitmaps, also for logos, but they cannot be plot by SVG, GERBER or HPGL plotters (in
 * which case only the bounding box is plotted)
 */
class WS_DRAW_ITEM_BASE : public EDA_ITEM     // This basic class, not directly usable.
{
protected:
    WORKSHEET_DATAITEM*  m_peer;    // an unique identifier, used as link
                                    // to the parent WORKSHEET_DATAITEM item,
                                    // in page layout editor

    WS_DRAW_ITEM_BASE( WORKSHEET_DATAITEM* aPeer, KICAD_T aType ) :
            EDA_ITEM( aType )
    {
        m_peer = aPeer;
        m_Flags = 0;
    }

public:
    virtual ~WS_DRAW_ITEM_BASE() {}

    WORKSHEET_DATAITEM* GetPeer() const { return m_peer; }

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    virtual const wxPoint GetPosition() const = 0;
    virtual void SetPosition( wxPoint aPos ) = 0;
    virtual void SetEnd( wxPoint aPos ) { /* not all types will need this */ }

    /** The function to draw a WS_DRAW_ITEM
     */
    virtual void DrawWsItem( EDA_RECT* aClipBox, wxDC* aDC, COLOR4D aColor )
    {
        wxPoint offset( 0, 0 );
        DrawWsItem( aClipBox, aDC, offset, UNSPECIFIED_DRAWMODE, aColor );
    }

    /// More advanced version of DrawWsItem. This is what must be
    /// defined in the derived type.
    virtual void DrawWsItem( EDA_RECT* aClipBox, wxDC* aDC, const wxPoint& aOffset,
                             GR_DRAWMODE aDrawMode, COLOR4D aColor ) = 0;

    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override
    {
        // This is just here to prevent annoying compiler warnings about hidden overloaded
        // virtual functions
        return EDA_ITEM::HitTest( aPosition, aAccuracy );
    }

    /**
     * Virtual function
     * return true if the rect aRect intersects on the item
     */
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
    WS_DRAW_ITEM_LINE( WORKSHEET_DATAITEM* aPeer, wxPoint aStart, wxPoint aEnd, int aPenWidth ) :
        WS_DRAW_ITEM_BASE( aPeer, WSG_LINE_T )
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

    /** The function to draw a WS_DRAW_ITEM_LINE
     */
    void DrawWsItem( EDA_RECT* aClipBox, wxDC* aDC, const wxPoint& aOffset, GR_DRAWMODE aDrawMode,
                     COLOR4D aColor ) override;

    /**
     * Virtual function
     * return true if the point aPosition is on the line
     */
    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif
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
    WS_DRAW_ITEM_POLYGON( WORKSHEET_DATAITEM* aPeer, wxPoint aPos, bool aFill, int aPenWidth ) :
        WS_DRAW_ITEM_BASE( aPeer, WSG_POLY_T )
    {
        m_penWidth = aPenWidth;
        m_fill = aFill;
        m_pos = aPos;
    }

    virtual wxString GetClass() const override { return wxT( "WS_DRAW_ITEM_POLYGON" ); }

    // Accessors:
    int GetPenWidth() const { return m_penWidth; }
    bool IsFilled() const { return m_fill; }
    const wxPoint GetPosition() const override { return m_pos; }
    void SetPosition( wxPoint aPos ) override { m_pos = aPos; }

    /** The function to draw a WS_DRAW_ITEM_POLYGON
     */
    void DrawWsItem( EDA_RECT* aClipBox, wxDC* aDC, const wxPoint& aOffset, GR_DRAWMODE aDrawMode,
                     COLOR4D aColor ) override;

    /**
     * Virtual function
     * return true if the point aPosition is inside one polygon
     */
    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;

    /**
     * Virtual function
     * return true if the rect aRect intersects on the item
     */
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

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
    WS_DRAW_ITEM_RECT( WORKSHEET_DATAITEM* aPeer, wxPoint aStart, wxPoint aEnd, int aPenWidth ) :
            WS_DRAW_ITEM_BASE( aPeer, WSG_RECT_T )
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

    /** The function to draw a WS_DRAW_ITEM_RECT
     */
    void DrawWsItem( EDA_RECT* aClipBox, wxDC* aDC, const wxPoint& aOffset, GR_DRAWMODE aDrawMode,
                     COLOR4D aColor ) override;

    /**
     * Virtual function
     * return true if the point aPosition is on one edge of the rectangle
     */
    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;

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
    WS_DRAW_ITEM_TEXT( WORKSHEET_DATAITEM* aPeer, wxString& aText, wxPoint aPos, wxSize aSize,
                       int aPenWidth, bool aItalic = false, bool aBold = false ) :
            WS_DRAW_ITEM_BASE( aPeer, WSG_TEXT_T),
            EDA_TEXT( aText )
    {
        SetTextPos( aPos );
        SetTextSize( aSize );
        SetThickness( aPenWidth );
        SetItalic( aItalic );
        SetBold( aBold );
    }

    virtual wxString GetClass() const override { return wxT( "WS_DRAW_ITEM_TEXT" ); }

    /** The function to draw a WS_DRAW_ITEM_TEXT
     */
    void DrawWsItem( EDA_RECT* aClipBox, wxDC* aDC, const wxPoint& aOffset, GR_DRAWMODE aDrawMode,
                     COLOR4D aColor ) override;

    // Accessors:
    int GetPenWidth() { return GetThickness(); }

    void SetTextAngle( double aAngle )
    {
        EDA_TEXT::SetTextAngle( NormalizeAngle360Min( aAngle ) );
    }

    const wxPoint GetPosition() const override { return GetTextPos(); }
    void SetPosition( wxPoint aPos ) override { SetTextPos( aPos ); }

    /**
     * Virtual function
     * return true if the point aPosition is inside one polygon
     */
    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;

    /**
     * Virtual function
     * return true if the rect aRect intersects on the item
     */
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
    WS_DRAW_ITEM_BITMAP( WORKSHEET_DATAITEM* aPeer, wxPoint aPos ) :
            WS_DRAW_ITEM_BASE( aPeer, WSG_BITMAP_T )
    {
        m_pos = aPos;
    }

    WS_DRAW_ITEM_BITMAP() :
            WS_DRAW_ITEM_BASE( nullptr, WSG_BITMAP_T )
    {
    }

    ~WS_DRAW_ITEM_BITMAP() {}

    virtual wxString GetClass() const override { return wxT( "WS_DRAW_ITEM_BITMAP" ); }

    const wxPoint GetPosition() const override { return m_pos; }
    void SetPosition( wxPoint aPos ) override { m_pos = aPos; }

    /** The function to draw a WS_DRAW_ITEM_BITMAP
     */
    void DrawWsItem( EDA_RECT* aClipBox, wxDC* aDC, const wxPoint& aOffset, GR_DRAWMODE aDrawMode,
                     COLOR4D aColor ) override;

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
     * Sets up the WORKSHEET_DATAITEM globals for generating drawItems.
     */
    static void SetupDrawEnvironment( const PAGE_INFO& aPageInfo );

    /**
     * Draws the item list created by BuildWorkSheetGraphicList
     * @param aClipBox = the clipping rect, or NULL if no clipping
     * @param aDC = the current Device Context
     */
    void Draw( EDA_RECT* aClipBox, wxDC* aDC, COLOR4D aColor );

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
     * static function: returns the instance of WORKSHEET_LAYOUT used in the application
     */
    static WORKSHEET_LAYOUT& GetTheInstance();

    /**
     * static function: Set an alternate instance of WORKSHEET_LAYOUT
     * mainly used in page setting dialog
     * @param aLayout = the alternate page layout; if null restore the basic page layout
     */
    static void SetAltInstance( WORKSHEET_LAYOUT* aLayout = NULL );

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

    void Append( WORKSHEET_DATAITEM* aItem );
    void Remove( WORKSHEET_DATAITEM* aItem );

    /**
     * @return the index of aItem, or -1 if does not exist
     */
    int GetItemIndex( WORKSHEET_DATAITEM* aItem ) const;

    /**
     * @return the item from its index aIdx, or NULL if does not exist
     */
    WORKSHEET_DATAITEM* GetItem( unsigned aIdx ) const;

    /**
     * @return a reference to the items.
     */
    std::vector<WORKSHEET_DATAITEM*>& GetItems() { return m_list; }

    /**
     * @return the item count
     */
    unsigned GetCount() const { return m_list.size(); }

    void SetDefaultLayout();
    void SetEmptyLayout();

    /**
     * Returns a string containing the empty layout shape
     */
    static wxString EmptyLayout();

    /**
     * Returns a string containing the empty layout shape
     */
    static wxString DefaultLayout();

    /**
     * Populates the list with a custom layout, or
     * the default layout, if no custom layout available
     * @param aFullFileName = the custom page layout description file.
     * if empty, loads the file defined by KICAD_WKSFILE
     * and if its is not defined, uses the default internal description
     * @param Append = if true: do not delete old layout, and load only
       aFullFileName.
     */
    void SetPageLayout( const wxString& aFullFileName = wxEmptyString, bool Append = false );

    /**
     * Populates the list from a S expr description stored in a string
     * @param aPageLayout = the S expr string
     * @param aAppend Do not delete old layout if true and append \a aPageLayout
     *               the existing one.
       @param aSource is the layout source description.
     */
    void SetPageLayout( const char* aPageLayout, bool aAppend = false,
                        const wxString& aSource = wxT( "Sexpr_string" )  );

    /**
     * @return a short filename  from a full filename:
     * if the path is the current project path, or if the path
     * is the same as kicad.pro (in template), returns the shortname
     * else do nothing and returns a full filename
     * @param aFullFileName = the full filename, which can be a relative
     * @param aProjectPath = the curr project absolute path (can be empty)
     */
    static const wxString MakeShortFileName( const wxString& aFullFileName,
                                             const wxString& aProjectPath );

    /**
     * Static function
     * @return a full filename from a short filename.
     * @param aShortFileName = the short filename, which can be a relative
     * @param aProjectPath = the curr project absolute path (can be empty)
     * or absolute path, and can include env variable reference ( ${envvar} expression )
     * if the short filename path is relative, it is expected relative to the project path
     * or (if aProjectPath is empty or if the file does not exist)
     * relative to kicad.pro (in template)
     * If aShortFileName is absolute return aShortFileName
     */
    static const wxString MakeFullFileName( const wxString& aShortFileName,
                                            const wxString& aProjectPath );
};

#endif      // WS_DRAW_ITEM_H
