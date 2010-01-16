
/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010 Kicad Developers, see change_log.txt for contributors.
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



/*  This source module implements the layer visibility and selection widget
*/


#define STAND_ALONE     1   // define to enable test program for LAYER_WIDGET
// also enable KICAD_AUIMANAGER and KICAD_AUITOOLBAR in ccmake to
// build this test program

#include <wx/wx.h>
#include <wx/statbmp.h>
#include <wx/aui/aui.h>

#include "macros.h"
#include "common.h"

#include "layer_panel_base.h"
#include "colors.h"

#include "pcbstruct.h"      // IsValidCopperLayerIndex()

/* no external data knowledge needed or wanted
#include "pcbnew.h"
#include "wxPcbStruct.h"
*/


#define LYR_COLUMN_COUNT        4           ///< Layer tab column count
#define RND_COLUMN_COUNT        3           ///< Rendering tab column count

#define ID_SHOW_ALL_COPPERS     wxID_HIGHEST
#define ID_SHOW_NO_COPPERS      (wxID_HIGHEST+1)


/* XPM */
static const char * clear_xpm[] = {
"28 14 1 1",
" 	c None",
"                            ",
"                            ",
"                            ",
"                            ",
"                            ",
"                            ",
"                            ",
"                            ",
"                            ",
"                            ",
"                            ",
"                            ",
"                            ",
"                            "};

/* XPM */
static const char * rightarrow_xpm[] = {
"28 14 5 1",
" 	c None",
".	c white",
"X	c #8080ff",
"o	c BLUE",
"O	c gray56",
"                   .X       ",
"                   .XX      ",
"                   .XXX     ",
"                   .XXXX    ",
"    ................XXXXX   ",
"    XXXXXXXXXXXXXXXXXXXXXX  ",
"    XXXXXXXXXXXXXXXXXXXXXXX ",
"    oooooooooooooooooooooooO",
"    ooooooooooooooooooooooO ",
"     OOOOOOOOOOOOOOOoooooO  ",
"                    ooooO   ",
"                    oooO    ",
"                    ooO     ",
"                    oO      "};


/**
 * Function encodeId
 * is here to allow saving a layer index within a control as its wxControl id,
 * but to do so in a way that all child wxControl ids within a wxWindow are unique,
 * since this is required by Windows.
 * @see getDecodedId()
 */
static int encodeId( int aColumn, int aId )
{
    int id = aId * LYR_COLUMN_COUNT + aColumn;
    return id;
}


/**
 * Function getDecodedId
 * decodes \a aControlId to original un-encoded value.
 */
static int getDecodedId( int aControlId )
{
    int id = aControlId / LYR_COLUMN_COUNT;    // rounding is OK.
    return id;
}


/**
 * Function makeColorTxt
 * returns a string containing the numeric value of the color.
 * in a form like 0x00000000.  (Color is currently an index, not RGB).
 */
static wxString makeColorTxt( int aColor )
{
    char colorValue[64];
    sprintf( colorValue, "0x%08x", aColor );
    return wxString( CONV_FROM_UTF8(colorValue) );
}


/**
 * Class LAYER_WIDGET
 * is abstract and is derived from a wxFormBuilder maintained class called
 * LAYER_PANEL_BASE.  It is used to manage a list of layers, with the notion of
 * a "current" layer, and layer specific visibility control.  You must derive from
 * it to use it so you can implement the abstract functions which recieve the
 * events.  Each layer is given its own color, and that color can be changed
 * within the UI provided here.  This widget knows nothing of the client code, meaning
 * it has no knowledge of a BOARD or anything.  To use it you must derive from
 * this class and implement the abstract functions:
 * <p> void OnLayerColorChange( int aLayer, int aColor );
 * <p> bool OnLayerSelect( int aLayer );
 * <p> void OnLayerVisible( int aLayer, bool isVisible );
 * <p> void OnRenderColorChange( int id, int aColor );
 * <p> void OnRenderEnable( int id, bool isEnabled );
 */
class LAYER_WIDGET : public LAYER_PANEL_BASE
{

public:

    /**
     * Struct ROW
     * provides all the data needed to add a row to a LAYER_WIDGET.  This is
     * part of the public API for a LAYER_WIDGET.
     */
    struct ROW
    {
        wxString    rowName;
        int         id;         // either a layer or
        int         color;      // -1 if none.
        bool        state;      // initial wxCheckBox state

        ROW( const wxString& aRowName, int aId, int aColor = 0, bool aState = true )
        {
            rowName = aRowName;
            id      = aId;
            color   = aColor;
            state   = aState;
        }
    };


protected:

#define MAX_LAYER_ROWS      64
#define BUTT_SIZE_X         32
#define BUTT_SIZE_Y         22
#define BUTT_VOID           6

    wxBitmap*       m_BlankBitmap;
    wxBitmap*       m_RightArrowBitmap;
    wxSize          m_BitmapSize;
    int             m_CurrentRow;           ///< selected row of layer list

    static wxBitmap makeBitmap( int aColor )
    {
        // the bitmap will be BUTT_VOID*2 pixels smaller than the button, leaving a
        // border of BUTT_VOID pixels on each side.
        wxBitmap    bitmap( BUTT_SIZE_X - 2 * BUTT_VOID, BUTT_SIZE_Y - 2 * BUTT_VOID );
        wxBrush     brush;
        wxMemoryDC  iconDC;

        iconDC.SelectObject( bitmap );

        brush.SetColour( MakeColour( aColor ) );
        brush.SetStyle( wxSOLID );
        iconDC.SetBrush( brush );

        iconDC.DrawRectangle( 0, 0, BUTT_SIZE_X - 2 * BUTT_VOID, BUTT_SIZE_Y - 2 * BUTT_VOID );

        return bitmap;
    }


    /**
     * Function makeColorButton
     * creates a wxBitmapButton and assigns it a solid color and a control ID
     */
    wxBitmapButton* makeColorButton( wxWindow* aParent, int aColor, int aID )
    {
        // dynamically make a wxBitMap and brush it with the appropriate color,
        // then create a wxBitmapButton from it.
        wxBitmap bitmap = makeBitmap( aColor );

        wxBitmapButton* ret = new wxBitmapButton( aParent, aID, bitmap,
            wxDefaultPosition, wxSize(BUTT_SIZE_X, BUTT_SIZE_Y), wxBORDER_RAISED );

        // save the color value in the name, no where else to put it.
        ret->SetName( makeColorTxt( aColor ) );
        return ret;
    }

    void OnLeftDownLayers( wxMouseEvent& event )
    {
        int row;

        wxWindow* eventSource = (wxWindow*) event.GetEventObject();

        // if mouse event is coming from the m_LayerScrolledWindow and not one
        // of its children, we have to find the row manually based on y coord.
        if( eventSource == m_LayerScrolledWindow )
        {
            int y = event.GetY();

            wxArrayInt heights = m_LayersFlexGridSizer->GetRowHeights();

            int height = 0;

            int rowCount = GetLayerRowCount();
            for( row = 0;  row<rowCount;  ++row )
            {
                if( y < height + heights[row] )
                    break;

                height += heights[row];
            }

            if( row >= rowCount )
                row = rowCount - 1;
        }

        else
        {
            // all nested controls on a given row will have their ID encoded with
            // encodeId(), and the corresponding decoding is getDecodedId()
            int layer = getDecodedId( eventSource ->GetId() );
            row   = findLayerRow( layer );
        }

        if( OnLayerSelect( row ) )    // if client allows this change.
            SelectLayerRow( row );
    }

    /**
     * Function OnMiddleDownLayerColor
     * is called only from a color button when user right clicks.
     */
    void OnMiddleDownLayerColor( wxMouseEvent& event )
    {
        wxBitmapButton* eventSource = (wxBitmapButton*) event.GetEventObject();

        wxString colorTxt = eventSource->GetName();

        int oldColor = strtoul( CONV_TO_UTF8(colorTxt), NULL, 0 );
        int newColor = DisplayColorFrame( this, oldColor );

        if( newColor >= 0 )
        {
            eventSource->SetName( makeColorTxt( newColor ) );

            wxBitmap bm = makeBitmap( newColor );
            eventSource->SetBitmapLabel( bm );

            int layer = getDecodedId( eventSource->GetId() );

            // tell the client code.
            OnLayerColorChange( layer, newColor );
        }
    }


    /**
     * Function OnRightDownLayers
     * puts up a popup menu for the layer panel.
     */
    void OnRightDownLayers( wxMouseEvent& event )
    {
        wxMenu          menu;

        // menu text is capitalized:
        // http://library.gnome.org/devel/hig-book/2.20/design-text-labels.html.en#layout-capitalization
        menu.Append( new wxMenuItem( &menu, ID_SHOW_ALL_COPPERS,
            _("Show All Copper Layers") ) );

        menu.Append( new wxMenuItem( &menu, ID_SHOW_NO_COPPERS,
            _( "Show No Copper Layers" ) ) );

        PopupMenu( &menu );
    }

    void OnPopupSelection( wxCommandEvent& event )
    {
        int     rowCount;
        int     menuId = event.GetId();
        bool    visible;

        switch( menuId )
        {
        case ID_SHOW_ALL_COPPERS:
            visible = true;
            goto L_change_coppers;

        case ID_SHOW_NO_COPPERS:
            visible = false;
        L_change_coppers:
            rowCount = GetLayerRowCount();
            for( int row=0;  row<rowCount;  ++row )
            {
                wxCheckBox* cb = (wxCheckBox*) getLayerComp( row*LYR_COLUMN_COUNT + 3 );
                int layer = getDecodedId( cb->GetId() );

                if( IsValidCopperLayerIndex( layer ) )
                {
                    cb->SetValue( visible );
                    OnLayerVisible( layer, visible );
                }
            }
            break;
        }
    }


    /**
     * Function OnLayerCheckBox
     * handles the "is layer visible" checkbox and propogates the
     * event to the client's notification function.
     */
    void OnLayerCheckBox( wxCommandEvent& event )
    {
        wxCheckBox* eventSource = (wxCheckBox*) event.GetEventObject();
        int layer = getDecodedId( eventSource->GetId() );
        OnLayerVisible( layer, eventSource->IsChecked() );
    }


    void OnMiddleDownRenderColor( wxMouseEvent& event )
    {
        wxBitmapButton* eventSource = (wxBitmapButton*) event.GetEventObject();

        wxString colorTxt = eventSource->GetName();

        int oldColor = strtoul( CONV_TO_UTF8(colorTxt), NULL, 0 );
        int newColor = DisplayColorFrame( this, oldColor );

        if( newColor >= 0 )
        {
            eventSource->SetName( makeColorTxt( newColor ) );

            wxBitmap bm = makeBitmap( newColor );
            eventSource->SetBitmapLabel( bm );

            int id = getDecodedId( eventSource->GetId() );

            // tell the client code.
            OnRenderColorChange( id, newColor );
        }
    }

    void OnRenderCheckBox( wxCommandEvent& event )
    {
        wxCheckBox* eventSource = (wxCheckBox*) event.GetEventObject();
        int id = getDecodedId( eventSource->GetId() );
        OnRenderEnable( id, eventSource->IsChecked() );
    }


    /**
     * Function getLayerComp
     * returns the component within the m_LayersFlexGridSizer at aSizerNdx or
     * NULL if \a aSizerNdx is out of range.
     *
     * @param aSizerNdx is the 0 based index into all the wxWindows which have
     *   been added to the m_LayersFlexGridSizer.
     */
    wxWindow* getLayerComp( int aSizerNdx )
    {
        if( (unsigned) aSizerNdx < m_LayersFlexGridSizer->GetChildren().GetCount() )
            return m_LayersFlexGridSizer->GetChildren()[aSizerNdx]->GetWindow();
        return NULL;
    }

    /**
     * Function findLayerRow
     * returns the row index that \a aLayer resides in, or -1 if not found.
     */
    int findLayerRow( int aLayer )
    {
        int count = GetLayerRowCount();
        for( int row=0;  row<count;  ++row )
        {
            // column 0 in the layer scroll window has a wxStaticBitmap, get its ID.
            wxStaticBitmap* bm = (wxStaticBitmap*) getLayerComp( row * LYR_COLUMN_COUNT );

            if( aLayer == getDecodedId( bm->GetId() ))
                return row;
        }
        return -1;
    }

    /**
     * Function insertLayerRow
     * appends or inserts a new row in the layer portion of the widget.
     */
    void insertLayerRow( int aRow, const ROW& aSpec )
    {
        int col;

        wxASSERT( aRow >= 0 && aRow < MAX_LAYER_ROWS );

        size_t index = aRow * LYR_COLUMN_COUNT;

        wxSizerFlags    flags;

        flags.Align(wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);

        // column 0
        col = 0;
        wxStaticBitmap* sbm = new wxStaticBitmap( m_LayerScrolledWindow, encodeId( col, aSpec.id ),
                                *m_BlankBitmap, wxDefaultPosition, m_BitmapSize );
        sbm->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnLeftDownLayers ), NULL, this );
        sbm->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnRightDownLayers ), NULL, this );
        m_LayersFlexGridSizer->Insert( index+col,
            new wxSizerItem( sbm, wxSizerFlags().Align( wxALIGN_CENTER_VERTICAL ) ) );

        // column 1
        col = 1;
        wxBitmapButton* bmb = makeColorButton( m_LayerScrolledWindow, aSpec.color, encodeId( col, aSpec.id ) );
        bmb->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnLeftDownLayers ), NULL, this );
        bmb->Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnMiddleDownLayerColor ), NULL, this );
        bmb->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnRightDownLayers ), NULL, this );
        bmb->SetToolTip( _("Left click to select, middle click for color change, right click for menu" ) );
        m_LayersFlexGridSizer->Insert( index+col, new wxSizerItem( bmb, flags ) );

        // column 2
        col = 2;
        wxStaticText* st = new wxStaticText( m_LayerScrolledWindow, encodeId( col, aSpec.id ), aSpec.rowName );
        st->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnLeftDownLayers ), NULL, this );
        st->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnRightDownLayers ), NULL, this );
        st->SetToolTip( _( "Click here to select this layer" ) );
        m_LayersFlexGridSizer->Insert( index+col,
            new wxSizerItem( st, wxSizerFlags().Align( wxALIGN_CENTER_VERTICAL )) );

        // column 3
        col = 3;
        wxCheckBox* cb = new wxCheckBox( m_LayerScrolledWindow, encodeId( col, aSpec.id ), wxEmptyString );
        cb->SetValue( aSpec.state );
        cb->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( LAYER_WIDGET::OnLayerCheckBox ), NULL, this );
        cb->SetToolTip( _( "Enable this for visibility" ) );
        m_LayersFlexGridSizer->Insert( index+col, new wxSizerItem( cb, flags ) );
    }

    void insertRenderRow( int aRow, const ROW& aSpec )
    {
        int col;

        wxASSERT( aRow >= 0 && aRow < MAX_LAYER_ROWS );

        size_t index = aRow * RND_COLUMN_COUNT;

        wxSizerFlags    flags;

        flags.Align(wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);

        // column 0
        col = 0;
        wxBitmapButton* bmb = makeColorButton( m_RenderScrolledWindow, aSpec.color, encodeId( col, aSpec.id ) );
        bmb->Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnMiddleDownRenderColor ), NULL, this );
        bmb->SetToolTip( _("Middle click for color change" ) );
        m_RenderFlexGridSizer->Insert( index+col, new wxSizerItem( bmb, flags ) );

#if 0
        // column 1
        col = 1;
        wxCheckBox* cb = new wxCheckBox( m_RenderScrolledWindow, encodeId( col, aSpec.id ), aSpec.rowName,
            wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
        cb->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( LAYER_WIDGET::OnRenderCheckBox ), NULL, this );
//        cb->SetToolTip( _( "Enable this for visibility" ) );
        m_RenderFlexGridSizer->Insert( index+col, new wxSizerItem( cb, flags ) );
#else
        // column 1
        col = 1;
        wxCheckBox* cb = new wxCheckBox( m_RenderScrolledWindow, encodeId( col, aSpec.id ), wxEmptyString );
        cb->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( LAYER_WIDGET::OnRenderCheckBox ), NULL, this );
//        cb->SetToolTip( _( "Enable this for visibility" ) );
        m_RenderFlexGridSizer->Insert( index+col, new wxSizerItem( cb, flags ) );

        // column 2
        col = 2;
        wxStaticText* st = new wxStaticText( m_RenderScrolledWindow, encodeId( col, aSpec.id ), aSpec.rowName );
        m_RenderFlexGridSizer->Insert( index+col,
            new wxSizerItem( st, wxSizerFlags().Align( wxALIGN_CENTER_VERTICAL )) );
#endif
    }


public:

    /** Constructor */
    LAYER_WIDGET( wxWindow* parent ) :
        LAYER_PANEL_BASE( parent )
    {
        m_CurrentRow = -1;

        m_RightArrowBitmap = new wxBitmap( rightarrow_xpm );

        m_BlankBitmap = new wxBitmap( clear_xpm );     // translucent
        m_BitmapSize = wxSize(m_BlankBitmap->GetWidth(), m_BlankBitmap->GetHeight());

        // handle the popup menu over the layer window
        m_LayerScrolledWindow->Connect( wxEVT_RIGHT_DOWN,
            wxMouseEventHandler( LAYER_WIDGET::OnRightDownLayers ), NULL, this );

        // since Popupmenu() calls this->ProcessEvent() we must call this->Connect()
        // and not m_LayerScrolledWindow->Connect()
        Connect( ID_SHOW_ALL_COPPERS, ID_SHOW_NO_COPPERS,
                wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler( LAYER_WIDGET::OnPopupSelection ), NULL, this );
    }


    /**
     * Function GetBestSize
     * returns the preferred minimum size, taking into consideration the
     * dynamic content.  Nothing in wxWidgets was reliable enough.
     */
    wxSize GetBestSize() const
    {
        // size of m_LayerScrolledWindow --------------
        wxArrayInt widths = m_LayersFlexGridSizer->GetColWidths();
        int totWidth = 0;
        for( int i=0;  i<LYR_COLUMN_COUNT && i<(int)widths.GetCount();  ++i )
        {
            totWidth += widths[i] + m_LayersFlexGridSizer->GetHGap();
            // printf("widths[%d]:%d\n", i, widths[i] );
        }

        wxArrayInt heights = m_LayersFlexGridSizer->GetRowHeights();
        int totHeight = 0;
        int rowCount = GetLayerRowCount();
        for( int i=0; i<rowCount && i<(int)heights.GetCount();  ++i )
        {
            totHeight += heights[i] + m_LayersFlexGridSizer->GetVGap();
            // printf("heights[%d]:%d\n", i, heights[i] );
        }

        // Account for the parent's frame:
        totWidth += 10;

        if( heights.GetCount() )
            totHeight += 2 * heights[0]; // use 2 row heights to approximate tab height
        else
            totHeight += 20;        // not used except before adding rows.

        wxSize layerz( totWidth, totHeight );

        // size of m_RenderScrolledWindow --------------
        widths = m_RenderFlexGridSizer->GetColWidths();
        totWidth = 0;
        for( int i=0;  i<RND_COLUMN_COUNT && i<(int)widths.GetCount();  ++i )
        {
            totWidth += widths[i] + m_RenderFlexGridSizer->GetHGap();
            // printf("widths[%d]:%d\n", i, widths[i] );
        }

        heights = m_RenderFlexGridSizer->GetRowHeights();
        totHeight = 0;
        rowCount = GetRenderRowCount();
        for( int i=0; i<rowCount && i<(int)heights.GetCount();  ++i )
        {
            totHeight += heights[i] + m_RenderFlexGridSizer->GetVGap();
            // printf("heights[%d]:%d\n", i, heights[i] );
        }

        // account for the parent's frame, this one void space of 10 PLUS a border:
        totWidth += 20;

        if( heights.GetCount() )
            totHeight += 2 * heights[0]; // use 2 row heights to approximate tab height
        else
            totHeight += 20;    // not used except before adding rows

        wxSize renderz( totWidth, totHeight );

        return wxSize( max(renderz.x,layerz.x), max(renderz.y,layerz.y) );
    }


    /**
     * Function GetLayerRowCount
     * returns the number of rows in the layer tab.
     */
    int GetLayerRowCount() const
    {
        int controlCount = m_LayersFlexGridSizer->GetChildren().GetCount();
        return controlCount / LYR_COLUMN_COUNT;
    }

    /**
     * Function GetRenderRowCount
     * returns the number of rows in the render tab.
     */
    int GetRenderRowCount() const
    {
        int controlCount = m_RenderFlexGridSizer->GetChildren().GetCount();
        return controlCount / RND_COLUMN_COUNT;
    }

    /**
     * Function AppendLayerRow
     * appends a new row in the layer portion of the widget.  The user must
     * ensure that ROW::id is unique for all existing rows on Windows.
     */
    void AppendLayerRow( const ROW& aRow )
    {
        int nextRow = GetLayerRowCount();
        insertLayerRow( nextRow, aRow );
        FitInside();
    }

    /**
     * Function ClearLayerRows
     * empties out the layer rows.
     */
    void ClearLayerRows()
    {
        m_LayerScrolledWindow->DestroyChildren();
    }

    /**
     * Function AppendRenderRow
     * appends a new row in the render portion of the widget.  The user must
     * ensure that ROW::id is unique for all existing rows on Windows.
     */
    void AppendRenderRow( const ROW& aRow )
    {
        int nextRow = GetRenderRowCount();
        insertRenderRow( nextRow, aRow );
        FitInside();
    }

    /**
     * Function ClearRenderRows
     * empties out the render rows.
     */
    void ClearRenderRows()
    {
        m_RenderScrolledWindow->DestroyChildren();
    }

    /**
     * Function SelectLayerRow
     * changes the row selection in the layer list to the given row.
     */
    void SelectLayerRow( int aRow )
    {
        // enable the layer tab at index 0
        m_notebook->ChangeSelection( 0 );

        int oldNdx = LYR_COLUMN_COUNT * m_CurrentRow;
        int newNdx = LYR_COLUMN_COUNT * aRow;

        m_CurrentRow = aRow;

        wxStaticBitmap* oldbm = (wxStaticBitmap*) getLayerComp( oldNdx );
        if( oldbm )
            oldbm->SetBitmap( *m_BlankBitmap );

        wxStaticBitmap* newbm = (wxStaticBitmap*) getLayerComp( newNdx );
        if( newbm )
        {
            newbm->SetBitmap( *m_RightArrowBitmap );

            // Change the focus to the wxBitmapButton in column 1 for this row.
            // We really do not need or want the focus, but because we get focus
            // and it changes the appearance of these wxBitmapButtons, if any focused
            // button is going to look different, we want it to be the current
            // row.
            getLayerComp( newNdx + 1 /* 1 is column */ )->SetFocus();

            // Make sure the desired layer row is visible.
            // It seems that as of 2.8.2, setting the focus
            // does this and generally I don't expect the scrolling to be needed at all because
            // the minimum window size may end up being established by the number of layers.
        }
    }


    /**
     * Function SelectLayer
     * changes the row selection in the layer list to \a aLayer provided.
     */
    void SelectLayer( int aLayer )
    {
        int row = findLayerRow( aLayer );
        SelectLayerRow( row );
    }


    /**
     * Function GetSelectedLayer
     * returns the selected layer or -1 if none.
     */
    int GetSelectedLayer()
    {
        // column 0 in the layer scroll window has a wxStaticBitmap, get its ID.
        wxStaticBitmap* bm = (wxStaticBitmap*) getLayerComp( m_CurrentRow * LYR_COLUMN_COUNT );
        if( bm )
            return getDecodedId( bm->GetId() );

        return -1;
    }

    /**
     * Function SetLayerVisible
     * sets \a aLayer visible or not.  This does not invoke OnLayerVisible().
     */
    void SetLayerVisible( int aLayer, bool isVisible )
    {
        int row = findLayerRow( aLayer );
        if( row >= 0 )
        {
            wxCheckBox* cb = (wxCheckBox*) getLayerComp( row * LYR_COLUMN_COUNT + 3 );
            wxASSERT( cb );
            cb->SetValue( isVisible );      // does not fire an event
        }
    }

    //-----<abstract functions>-------------------------------------------

    /**
     * Function OnLayerColorChange
     * is called to notify client code about a layer color change.  Derived
     * classes will handle this accordingly.
     */
    virtual void OnLayerColorChange( int aLayer, int aColor ) = 0;

    /**
     * Function OnLayerSelect
     * is called to notify client code whenever the user selects a different
     * layer.  Derived classes will handle this accordingly, and can deny
     * the change by returning false.
     */
    virtual bool OnLayerSelect( int aLayer ) = 0;

    /**
     * Function OnLayerVisible
     * is called to notify client code about a layer visibility change.
     */
    virtual void OnLayerVisible( int aLayer, bool isVisible ) = 0;

    /**
     * Function OnRenderColorChange
     * is called to notify client code whenever the user changes a rendering
     * color.
     * @param aId is the same id that was established in a Rendering row
     * via the AddRenderRow() function.
     */
    virtual void OnRenderColorChange( int aId, int aColor ) = 0;

    /**
     * Function OnRenderEnable
     * is called to notify client code whenever the user changes an rendering
     * enable in one of the rendering checkboxes.
     * @param aId is the same id that was established in a Rendering row
     * via the AddRenderRow() function.
     * @param isEnabled is the state of the checkbox, true if checked.
     */
    virtual void OnRenderEnable( int aId, bool isEnabled ) = 0;

    //-----</abstract functions>------------------------------------------
};


#if defined(STAND_ALONE)

/**
 * Class MYFRAME
 * is a test class here to exercise the LAYER_WIDGET and explore use cases.
 * @see http://www.kirix.com/labs/wxaui/screenshots.html
 * for ideas.
 */
class MYFRAME : public wxFrame
{
    // example of how to derive from LAYER_WIDGET in order to provide the
    // abstract methods.
    class MYLAYERS : public LAYER_WIDGET
    {
        MYFRAME*    frame;

    public:
        // your constructor could take a BOARD argument.  here I leave it
        // out because this source module wants to know nothing of BOARDs
        // to maximize re-use.
        MYLAYERS( wxWindow* aParent, MYFRAME* aFrame ) :
            LAYER_WIDGET( aParent ),
            frame( aFrame )
        {
        }

        void OnLayerColorChange( int aLayer, int aColor )
        {
            printf( "OnLayerColorChange( aLayer:%d, aColor:%d )\n", aLayer, aColor );

            /* a test trigger only
            if( aLayer == 2 )
            {
                ClearLayerRows();
                printf(" GetLayerRowCount(): %d\n", GetLayerRowCount() );
            }
            */
        }

        bool OnLayerSelect( int aLayer )
        {
            printf( "OnLayerSelect( aLayer:%d )\n", aLayer );
            return true;
        }

        void OnLayerVisible( int aLayer, bool isVisible )
        {
            printf( "OnLayerVisible( aLayer:%d, isVisible:%d )\n", aLayer, isVisible );
        }

        void OnRenderColorChange( int aId, int aColor )
        {
            printf( "OnRenderColorChange( aId:%d, aColor:%d )\n", aId, aColor );
        }

        void OnRenderEnable( int aId, bool isEnabled )
        {
            printf( "OnRenderEnable( aId:%d, isEnabled:%d )\n", aId, isEnabled );
        }
    };


public:
    MYFRAME( wxWindow * parent ) :
        wxFrame( parent, -1, _( "wxAUI Test" ), wxDefaultPosition,
            wxSize( 800, 600 ), wxDEFAULT_FRAME_STYLE )
    {
        // notify wxAUI which frame to use
        m_mgr.SetManagedWindow( this );

        MYLAYERS* lw = new MYLAYERS( this, this );

        lw->AppendLayerRow( LAYER_WIDGET::ROW( wxT("layer 1"), 0, RED, false ) );
        lw->AppendLayerRow( LAYER_WIDGET::ROW( wxT("layer 2"), 1, GREEN ) );
        lw->AppendLayerRow( LAYER_WIDGET::ROW( wxT("brown_layer"), 2, BROWN ) );
        lw->AppendLayerRow( LAYER_WIDGET::ROW( wxT("layer_4_you"), 3, BLUE, false ) );

        lw->AppendRenderRow( LAYER_WIDGET::ROW( wxT("With Very Large Ears"), 0, GREEN ) );
        lw->AppendRenderRow( LAYER_WIDGET::ROW( wxT("With Legs"), 1, YELLOW ) );

        lw->SelectLayerRow( 1 );

        wxAuiPaneInfo li;
        li.MinSize( lw->GetBestSize() );
        li.BestSize( lw->GetBestSize() );
        li.Left();
//        li.MaximizeButton( true );
//        li.MinimizeButton( true );
        li.CloseButton( false );
        li.Caption( wxT( "Layers" ) );
        m_mgr.AddPane( lw, li );


        wxTextCtrl* text2 = new wxTextCtrl( this, -1, _( "Pane 2 - sample text" ),
                                            wxDefaultPosition, wxSize( 200, 150 ),
                                            wxNO_BORDER | wxTE_MULTILINE );
        m_mgr.AddPane( text2, wxBOTTOM, wxT( "Pane Number Two" ) );

        wxTextCtrl* text3 = new wxTextCtrl( this, -1, _( "Main content window" ),
                                            wxDefaultPosition, wxSize( 200, 150 ),
                                            wxNO_BORDER | wxTE_MULTILINE );
        m_mgr.AddPane( text3, wxCENTER );

        // tell the manager to "commit" all the changes just made
        m_mgr.Update();
    }

    ~MYFRAME()
    {
        // deinitialize the frame manager
        m_mgr.UnInit();
    }

private:
    wxAuiManager m_mgr;
};


// our normal wxApp-derived class, as usual
class MyApp : public wxApp
{
public:

    bool OnInit()
    {
        wxFrame* frame = new MYFRAME( NULL );

        SetTopWindow( frame );
        frame->Show();
        return true;
    }
};

DECLARE_APP( MyApp );
IMPLEMENT_APP( MyApp );

#endif  // STAND_ALONE
