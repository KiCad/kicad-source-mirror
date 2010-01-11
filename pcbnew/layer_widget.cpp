
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

/* no external data knowledge needed or wanted
#include "pcbnew.h"
#include "wxPcbStruct.h"
*/


#define LAYER_COLUMN_COUNT  4


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
 * Function layerId
 * is here to allow saving a layer index within a control as its wxControl id,
 * but to do so in a way that all child wxControl ids within a wxWindow are unique,
 * since this is required by Windows.
 * @see getLayerId()
 */
static int layerId( int aColumn, int aLayer )
{
    int id = aLayer * LAYER_COLUMN_COUNT + aColumn;
    return id;
}


/**
 * Function getLayerId
 * decodes \a aControlId to return a layer.
 */
static int getLayerId( int aControlId )
{
    int layer = aControlId / LAYER_COLUMN_COUNT;    // rounding is OK.
    return layer;
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
 * Struct LAYER_SPEC
 * provides all the data needed to add a layer row to a LAYER_WIDGET
 */
struct LAYER_SPEC
{
    wxString    layerName;
    int         layer;
    int         color;

    LAYER_SPEC( const wxString& aLayerName, int aLayer, int aColor = 0 )
    {
        layerName  = aLayerName;
        layer      = aLayer;
        color = aColor;
    }
};


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
 * <p>
 * void OnColorChange( int aLayer, int aColor );
 * <p>
 * bool OnLayerSelect( int aLayer );
 * <p>
 * void OnLayerVisible( int aLayer, bool isVisible );
 */
class LAYER_WIDGET : public LAYER_PANEL_BASE
{

#define MAX_LAYER_ROWS      64
#define BUTT_SIZE_X         32
#define BUTT_SIZE_Y         22
#define BUTT_VOID           6

protected:
    wxBitmap*       m_BlankBitmap;
    wxBitmap*       m_RightArrowBitmap;
    wxSize          m_BitmapSize;
    int             m_CurrentRow;           ///< selected row of layer list

    static wxBitmap makeBitmap( int aColor )
    {
        // the bitmap will be 8 pixels smaller than the button, leaving a
        // border of 4 pixels on each side.
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
    wxBitmapButton* makeColorButton( int aColor, int aID )
    {
        // dynamically make a wxBitMap and brush it with the appropriate color,
        // then create a wxBitmapButton from it.
        wxBitmap bitmap = makeBitmap( aColor );

        wxBitmapButton* ret = new wxBitmapButton( m_LayerScrolledWindow, aID, bitmap,
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
            // makeLayerId(), and the corresponding decoding is getLayerId()
            int layer = getLayerId( eventSource ->GetId() );
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

            int layer = getLayerId( eventSource->GetId() );

            // tell the client code.
            OnColorChange( layer, newColor );
        }
    }


    /**
     * Function OnRightDownLayers
     * puts up a popup menu for the layer panel.
     */
    void OnRightDownLayers( wxMouseEvent& event )
    {
        // popup menu
        printf( "OnRightDownLayers\n" );
    }


    /**
     * Function OnLayerCheckBox
     * handles the "is layer visible" checkbox and propogates the
     * event to the client's notification function.
     */
    void OnLayerCheckBox( wxCommandEvent& event )
    {
        wxCheckBox* eventSource = (wxCheckBox*) event.GetEventObject();

        int layer = getLayerId( eventSource->GetId() );

        OnLayerVisible( layer, eventSource->IsChecked() );
    }


    /**
     * Function getLayerComp
     * returns the component within the m_LayersFlexGridSizer at aSizerNdx.
     *
     * @param aSizerNdx is the 0 based index into all the wxWindows which have
     *   been added to the m_LayersFlexGridSizer.
     */
    wxWindow* getLayerComp( int aSizerNdx )
    {
        return m_LayersFlexGridSizer->GetChildren()[aSizerNdx]->GetWindow();
    }

    /**
     * Function findLayerRow
     * returns the row index that \a aLayer resides in, or -1 if not found.
     */
    int findLayerRow( int aLayer )
    {
        int count = GetLayerRowCount();
        for( int row=0;  row<count; ++row )
        {
            // column 0 in the layer scroll window has a wxStaticBitmap, get its ID.
            wxStaticBitmap* bm = (wxStaticBitmap*) getLayerComp( row * LAYER_COLUMN_COUNT + 0 );

            if( aLayer == getLayerId( bm->GetId() ))
                return row;
        }

        return -1;
    }

    /**
     * Function insertLayerRow
     * appends or inserts a new row in the layer portion of the widget.
     */
    void insertLayerRow( int aRow, const LAYER_SPEC& aSpec )
    {
        wxASSERT( aRow >= 0 && aRow < MAX_LAYER_ROWS );

        size_t index = aRow * LAYER_COLUMN_COUNT;

        wxSizerFlags    flags;

        flags.Align(wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);

        // column 0
        wxStaticBitmap* sbm = new wxStaticBitmap( m_LayerScrolledWindow, layerId( 0, aSpec.layer ),
                                *m_BlankBitmap, wxDefaultPosition, m_BitmapSize );
        sbm->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnLeftDownLayers ), NULL, this );
        sbm->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnRightDownLayers ), NULL, this );
        m_LayersFlexGridSizer->Insert( index+0,
            new wxSizerItem( sbm, wxSizerFlags().Align( wxALIGN_CENTER_VERTICAL ) ) );

        // column 1
        wxBitmapButton* bmb = makeColorButton( aSpec.color, layerId( 1, aSpec.layer ) );
        bmb->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnLeftDownLayers ), NULL, this );
        bmb->Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnMiddleDownLayerColor ), NULL, this );
        bmb->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnRightDownLayers ), NULL, this );
        bmb->SetToolTip( _("Right click to change layer color, left click to select layer" ) );
        m_LayersFlexGridSizer->Insert( index+1,
            new wxSizerItem( bmb, flags ) );

        // column 2
        wxStaticText* st = new wxStaticText( m_LayerScrolledWindow, layerId( 2, aSpec.layer ), aSpec.layerName );
        st->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnLeftDownLayers ), NULL, this );
        st->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnRightDownLayers ), NULL, this );
        st->SetToolTip( _( "Click here to select this layer" ) );
        m_LayersFlexGridSizer->Insert( index+2,
            new wxSizerItem( st, wxSizerFlags().Align( wxALIGN_CENTER_VERTICAL )) );

        // column 3
        wxCheckBox* cb = new wxCheckBox( m_LayerScrolledWindow, layerId( 3, aSpec.layer ), wxEmptyString );
        cb->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( LAYER_WIDGET::OnLayerCheckBox ), NULL, this );
        cb->SetToolTip( _( "Enable this for visibility" ) );
        m_LayersFlexGridSizer->Insert( index+3, new wxSizerItem( cb, flags ) );
    }


public:

    /** Constructor */
    LAYER_WIDGET( wxWindow* parent ) :
        LAYER_PANEL_BASE( parent )
    {
        m_CurrentRow = 0;

        m_RightArrowBitmap = new wxBitmap( rightarrow_xpm );
        m_BlankBitmap = new wxBitmap( clear_xpm );     // translucent

        m_BitmapSize = wxSize(m_BlankBitmap->GetWidth(), m_BlankBitmap->GetHeight());

        m_LayerScrolledWindow->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnRightDownLayers ), NULL, this );


        AppendLayerRow(  LAYER_SPEC( wxT("layer 1"), 0, RED ) );
        AppendLayerRow(  LAYER_SPEC( wxT("layer 2"), 1, GREEN ) );
        AppendLayerRow(  LAYER_SPEC( wxT("brown_layer"), 2, BROWN ) );
        AppendLayerRow(  LAYER_SPEC( wxT("layer_4_you"), 3, BLUE ) );

        SelectLayerRow( 1 );

        SetMinSize( GetPreferredSize() );
    }


    /**
     * Function GetPreferredSize
     * returns the preferred minimum size, taking into consideration the
     * dynamic content.  Nothing in wxWidgets was reliable enough.
     */
    wxSize GetPreferredSize()
    {
        m_LayersFlexGridSizer->Layout();

        wxArrayInt widths = m_LayersFlexGridSizer->GetColWidths();
        int totWidth = 0;
        for( int i=0;  i<LAYER_COLUMN_COUNT;  ++i )
        {
            totWidth += widths[i] + m_LayersFlexGridSizer->GetHGap();
            printf("widths[%d]:%d\n", i, widths[i] );
        }

        wxArrayInt heights = m_LayersFlexGridSizer->GetRowHeights();
        int totHeight = 0;
        int rowCount = GetLayerRowCount();
        for( int i=0; i<rowCount;  ++i )
        {
            totHeight += heights[i] + m_LayersFlexGridSizer->GetVGap();
            printf("heights[%d]:%d\n", i, heights[i] );
        }

        wxSize layerPanelSize( totWidth, totHeight );

        // this aint done yet, just a place holder for more work.

        return layerPanelSize;
    }


    /**
     * Function GetLayerRowCount
     * returns the number of rows in the layer tab.
     */
    int GetLayerRowCount() const
    {
        int controlCount = m_LayersFlexGridSizer->GetChildren().GetCount();
        return controlCount / LAYER_COLUMN_COUNT;
    }

    /**
     * Function AppendLayerRow
     * appends a new row in the layer portion of the widget.
     */
    void AppendLayerRow( const LAYER_SPEC& aSpec )
    {
        int nextRow = GetLayerRowCount();
        insertLayerRow( nextRow, aSpec );
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
     * Function SelectLayerRow
     * changes the row selection in the layer list to the given row.
     */
    bool SelectLayerRow( int aRow )
    {
        // enable the layer tab at index 0
        m_notebook->ChangeSelection( 0 );

        if( (unsigned) aRow < (unsigned) GetLayerRowCount() )
        {
            int newNdx = LAYER_COLUMN_COUNT * aRow;
            int oldNdx = LAYER_COLUMN_COUNT * m_CurrentRow;

            wxStaticBitmap* oldbm = (wxStaticBitmap*) getLayerComp( oldNdx );
            wxStaticBitmap* newbm = (wxStaticBitmap*) getLayerComp( newNdx );

            oldbm->SetBitmap( *m_BlankBitmap );
            newbm->SetBitmap( *m_RightArrowBitmap );

            m_CurrentRow = aRow;

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

            return true;
        }
        return false;
    }

    /**
     * Function SelectLayer
     * changes the row selection in the layer list to \a aLayer provided.
     */
    bool SelectLayer( int aLayer )
    {
        int row = findLayerRow( aLayer );
        return SelectLayerRow( row );
    }


    //-----<abstract functions>-------------------------------------------

    /**
     * Function OnColorChange
     * is called to notify client code about a layer color change.  Derived
     * classes will handle this accordingly.
     */
    virtual void OnColorChange( int aLayer, int aColor ) = 0;

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

        void OnColorChange( int aLayer, int aColor )
        {
            printf( "OnColorChange( aLayer:%d, aColor:%d )\n", aLayer, aColor );

            // a test trigger only
            if( aLayer == 2 )
            {
                ClearLayerRows();
                printf(" GetLayerRowCount(): %d\n", GetLayerRowCount() );
            }
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
    };


public:
    MYFRAME( wxWindow * parent ) :
        wxFrame( parent, -1, _( "wxAUI Test" ), wxDefaultPosition,
            wxSize( 800, 600 ), wxDEFAULT_FRAME_STYLE )
    {
        // notify wxAUI which frame to use
        m_mgr.SetManagedWindow( this );

        MYLAYERS* layerWidget = new MYLAYERS( this, this );

        wxTextCtrl* text2 = new wxTextCtrl( this, -1, _( "Pane 2 - sample text" ),
                                            wxDefaultPosition, wxSize( 200, 150 ),
                                            wxNO_BORDER | wxTE_MULTILINE );

        wxTextCtrl* text3 = new wxTextCtrl( this, -1, _( "Main content window" ),
                                            wxDefaultPosition, wxSize( 200, 150 ),
                                            wxNO_BORDER | wxTE_MULTILINE );

        // add the panes to the manager
        wxAuiPaneInfo li;
        li.MinSize( layerWidget->GetPreferredSize() );    // ignored on linux
        li.BestSize( layerWidget->GetPreferredSize() );
        li.Left();
        li.MaximizeButton( true );
        li.MinimizeButton( true );
        li.CloseButton( false );
        li.Caption( wxT( "Layers" ) );
        m_mgr.AddPane( layerWidget, li );

        m_mgr.AddPane( text2, wxBOTTOM, wxT( "Pane Number Two" ) );
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
