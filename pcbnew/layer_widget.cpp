
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


//#define STAND_ALONE     1   // define to enable test program for LAYER_WIDGET
// also enable KICAD_AUIMANAGER and KICAD_AUITOOLBAR in ccmake to
// build this test program

#include <wx/wx.h>
#include <wx/statbmp.h>

#include "macros.h"
#include "common.h"
#include "colors.h"

#include "layer_widget.h"

#include "pcbstruct.h"      // IsValidCopperLayerIndex()


#define LYR_COLUMN_COUNT        4           ///< Layer tab column count
#define RND_COLUMN_COUNT        2           ///< Rendering tab column count

#define BUTT_SIZE_X             32
#define BUTT_SIZE_Y             22
#define BUTT_VOID               6


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
    wxString txt;
    txt.Printf( wxT("0x%08x"), aColor );
    return txt;
}


wxBitmap LAYER_WIDGET::makeBitmap( int aColor )
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

wxBitmapButton* LAYER_WIDGET::makeColorButton( wxWindow* aParent, int aColor, int aID )
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

void LAYER_WIDGET::OnLeftDownLayers( wxMouseEvent& event )
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

    passOnFocus();
}


void LAYER_WIDGET::OnMiddleDownLayerColor( wxMouseEvent& event )
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

    passOnFocus();
}


void LAYER_WIDGET::OnRightDownLayers( wxMouseEvent& event )
{
    wxMenu          menu;

    // menu text is capitalized:
    // http://library.gnome.org/devel/hig-book/2.20/design-text-labels.html.en#layout-capitalization
    menu.Append( new wxMenuItem( &menu, ID_SHOW_ALL_COPPERS,
        _("Show All Cu") ) );

    menu.Append( new wxMenuItem( &menu, ID_SHOW_NO_COPPERS,
        _( "Hide All Cu" ) ) );

    PopupMenu( &menu );

    passOnFocus();
}

void LAYER_WIDGET::OnPopupSelection( wxCommandEvent& event )
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
        int lastCu = -1;
        rowCount = GetLayerRowCount();
        for( int row=rowCount-1;  row>=0;  --row )
        {
            wxCheckBox* cb = (wxCheckBox*) getLayerComp( row*LYR_COLUMN_COUNT + 3 );
            int layer = getDecodedId( cb->GetId() );
            if( IsValidCopperLayerIndex( layer ) )
            {
                lastCu = row;
                break;
            }
        }

        for( int row=0;  row<rowCount;  ++row )
        {
            wxCheckBox* cb = (wxCheckBox*) getLayerComp( row*LYR_COLUMN_COUNT + 3 );
            int layer = getDecodedId( cb->GetId() );

            if( IsValidCopperLayerIndex( layer ) )
            {
                cb->SetValue( visible );

                bool isLastCopperLayer = (row==lastCu);

                OnLayerVisible( layer, visible, isLastCopperLayer );

                if( isLastCopperLayer )
                    break;
            }
        }
        break;
    }
}


void LAYER_WIDGET::OnLayerCheckBox( wxCommandEvent& event )
{
    wxCheckBox* eventSource = (wxCheckBox*) event.GetEventObject();
    int layer = getDecodedId( eventSource->GetId() );
    OnLayerVisible( layer, eventSource->IsChecked() );
    passOnFocus();
}


void LAYER_WIDGET::OnMiddleDownRenderColor( wxMouseEvent& event )
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
    passOnFocus();
}

void LAYER_WIDGET::OnRenderCheckBox( wxCommandEvent& event )
{
    wxCheckBox* eventSource = (wxCheckBox*) event.GetEventObject();
    int id = getDecodedId( eventSource->GetId() );
    OnRenderEnable( id, eventSource->IsChecked() );
    passOnFocus();
}


wxWindow* LAYER_WIDGET::getLayerComp( int aSizerNdx )
{
    if( (unsigned) aSizerNdx < m_LayersFlexGridSizer->GetChildren().GetCount() )
        return m_LayersFlexGridSizer->GetChildren()[aSizerNdx]->GetWindow();
    return NULL;
}


int LAYER_WIDGET::findLayerRow( int aLayer )
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


void LAYER_WIDGET::insertLayerRow( int aRow, const ROW& aSpec )
{
    wxASSERT( aRow >= 0 && aRow < MAX_LAYER_ROWS );

    int         col;
    int         index = aRow * LYR_COLUMN_COUNT;
    const int   flags = wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT;

    // column 0
    col = 0;
    wxStaticBitmap* sbm = new wxStaticBitmap( m_LayerScrolledWindow, encodeId( col, aSpec.id ),
                            *m_BlankBitmap, wxDefaultPosition, m_BitmapSize );
    sbm->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnLeftDownLayers ), NULL, this );
    sbm->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnRightDownLayers ), NULL, this );
    m_LayersFlexGridSizer->wxSizer::Insert( index+col, sbm, 0, flags );

    // column 1
    col = 1;
    wxBitmapButton* bmb = makeColorButton( m_LayerScrolledWindow, aSpec.color, encodeId( col, aSpec.id ) );
    bmb->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnLeftDownLayers ), NULL, this );
    bmb->Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnMiddleDownLayerColor ), NULL, this );
    bmb->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnRightDownLayers ), NULL, this );
    bmb->SetToolTip( _("Left click to select, middle click for color change, right click for menu" ) );
    m_LayersFlexGridSizer->wxSizer::Insert( index+col, bmb, 0, flags );

    // column 2
    col = 2;
    wxStaticText* st = new wxStaticText( m_LayerScrolledWindow, encodeId( col, aSpec.id ), aSpec.rowName );
    st->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnLeftDownLayers ), NULL, this );
    st->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnRightDownLayers ), NULL, this );
    st->SetToolTip( aSpec.tooltip );
    m_LayersFlexGridSizer->wxSizer::Insert( index+col, st, 0, flags );

    // column 3
    col = 3;
    wxCheckBox* cb = new wxCheckBox( m_LayerScrolledWindow, encodeId( col, aSpec.id ), wxEmptyString );
    cb->SetValue( aSpec.state );
    cb->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( LAYER_WIDGET::OnLayerCheckBox ), NULL, this );
    cb->SetToolTip( _( "Enable this for visibility" ) );
    m_LayersFlexGridSizer->wxSizer::Insert( index+col, cb, 0, flags );
}


void LAYER_WIDGET::insertRenderRow( int aRow, const ROW& aSpec )
{
    wxASSERT( aRow >= 0 && aRow < MAX_LAYER_ROWS );

    int         col;
    int         index = aRow * RND_COLUMN_COUNT;
    const int   flags = wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT;

    // column 0
    col = 0;
    if( aSpec.color != -1 )
    {
        wxBitmapButton* bmb = makeColorButton( m_RenderScrolledWindow, aSpec.color, encodeId( col, aSpec.id ) );
        bmb->Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnMiddleDownRenderColor ), NULL, this );
        bmb->SetToolTip( _("Middle click for color change" ) );
        m_RenderFlexGridSizer->wxSizer::Insert( index+col, bmb, 0, flags );

        // could add a left click handler on the color button that toggles checkbox.
    }
    else    // == -1, no color selection wanted
    {
        // need a place holder within the sizer to keep grid full.
        wxPanel* invisible = new wxPanel( m_RenderScrolledWindow );
        m_RenderFlexGridSizer->wxSizer::Insert( index+col, invisible, 0, flags );
    }

    // column 1
    col = 1;
    wxCheckBox* cb = new wxCheckBox( m_RenderScrolledWindow, encodeId( col, aSpec.id ),
                        aSpec.rowName, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    cb->SetValue( aSpec.state );
    cb->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED,
        wxCommandEventHandler( LAYER_WIDGET::OnRenderCheckBox ), NULL, this );
    cb->SetToolTip( aSpec.tooltip );
    m_RenderFlexGridSizer->wxSizer::Insert( index+col, cb, 0, flags );
}


void LAYER_WIDGET::passOnFocus()
{
    wxWindow* parent = GetParent();
    parent->SetFocus();

//    printf( "passOnFocus() %p %p\n", parent, m_OriginalParent );
}


//-----<public>-------------------------------------------------------

LAYER_WIDGET::LAYER_WIDGET( wxWindow* parent ) :
    LAYER_PANEL_BASE( parent )
{
    m_OriginalParent = parent;

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


wxSize LAYER_WIDGET::GetBestSize() const
{
    // size of m_LayerScrolledWindow --------------
    wxArrayInt widths = m_LayersFlexGridSizer->GetColWidths();
    int totWidth = 0;
    if( widths.GetCount() )
    {
        for( int i=0;  i<LYR_COLUMN_COUNT;  ++i )
        {
            totWidth += widths[i] + m_LayersFlexGridSizer->GetHGap();
            // printf("widths[%d]:%d\n", i, widths[i] );
        }
    }
    // Account for the parent's frame:
    totWidth += 10;


    wxArrayInt heights = m_LayersFlexGridSizer->GetRowHeights();
    int totHeight = 0;
    if( heights.GetCount() )
    {
        int rowCount = GetLayerRowCount();
        for( int i=0; i<rowCount;  ++i )
        {
            totHeight += heights[i] + m_LayersFlexGridSizer->GetVGap();
            // printf("heights[%d]:%d\n", i, heights[i] );
        }

        totHeight += 2 * heights[0]; // use 2 row heights to approximate tab height
    }
    else
        totHeight += 20;        // not used except before adding rows.

    wxSize layerz( totWidth, totHeight );


    // size of m_RenderScrolledWindow --------------
    widths = m_RenderFlexGridSizer->GetColWidths();
    totWidth = 0;
    if( widths.GetCount() )
    {
        for( int i=0;  i<RND_COLUMN_COUNT;  ++i )
        {
            totWidth += widths[i] + m_RenderFlexGridSizer->GetHGap();
            // printf("widths[%d]:%d\n", i, widths[i] );
        }
    }
    // account for the parent's frame, this one has void space of 10 PLUS a border:
    totWidth += 20;


    heights = m_RenderFlexGridSizer->GetRowHeights();
    totHeight = 0;
    if( heights.GetCount() )
    {
        int rowCount = GetRenderRowCount();
        for( int i=0; i<rowCount && i<(int)heights.GetCount();  ++i )
        {
            totHeight += heights[i] + m_RenderFlexGridSizer->GetVGap();
            // printf("heights[%d]:%d\n", i, heights[i] );
        }
        totHeight += 2 * heights[0]; // use 2 row heights to approximate tab height
    }
    else
        totHeight += 20;    // not used except before adding rows

    wxSize renderz( totWidth, totHeight );

    return wxSize( max(renderz.x,layerz.x), max(renderz.y,layerz.y) );
}


int LAYER_WIDGET::GetLayerRowCount() const
{
    int controlCount = m_LayersFlexGridSizer->GetChildren().GetCount();
    return controlCount / LYR_COLUMN_COUNT;
}


int LAYER_WIDGET::GetRenderRowCount() const
{
    int controlCount = m_RenderFlexGridSizer->GetChildren().GetCount();
    return controlCount / RND_COLUMN_COUNT;
}


void LAYER_WIDGET::AppendLayerRow( const ROW& aRow )
{
    int nextRow = GetLayerRowCount();
    insertLayerRow( nextRow, aRow );
    UpdateLayouts();
}


void LAYER_WIDGET::ClearLayerRows()
{
    m_LayersFlexGridSizer->Clear( true );
}


void LAYER_WIDGET::AppendRenderRow( const ROW& aRow )
{
    int nextRow = GetRenderRowCount();
    insertRenderRow( nextRow, aRow );
    UpdateLayouts();
}


void LAYER_WIDGET::ClearRenderRows()
{
    m_RenderFlexGridSizer->Clear( true );
}


void LAYER_WIDGET::SelectLayerRow( int aRow )
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

        // Make sure the desired layer row is visible.
        // It seems that as of 2.8.2, setting the focus does this.
        // I don't expect the scrolling to be needed at all because
        // the minimum window size may end up being established so that the
        // scroll bars will not be visible.
        getLayerComp( newNdx + 1 /* 1 is column */ )->SetFocus();
    }

    // give the focus back to the app.
    passOnFocus();
}


void LAYER_WIDGET::SelectLayer( int aLayer )
{
    int row = findLayerRow( aLayer );
    SelectLayerRow( row );
}


int LAYER_WIDGET::GetSelectedLayer()
{
    // column 0 in the layer scroll window has a wxStaticBitmap, get its ID.
    wxStaticBitmap* bm = (wxStaticBitmap*) getLayerComp( m_CurrentRow * LYR_COLUMN_COUNT );
    if( bm )
        return getDecodedId( bm->GetId() );

    return -1;
}


void LAYER_WIDGET::SetLayerVisible( int aLayer, bool isVisible )
{
    int row = findLayerRow( aLayer );
    if( row >= 0 )
    {
        wxCheckBox* cb = (wxCheckBox*) getLayerComp( row * LYR_COLUMN_COUNT + 3 );
        wxASSERT( cb );
        cb->SetValue( isVisible );      // does not fire an event
    }
}

void LAYER_WIDGET::UpdateLayouts()
{
    m_LayersFlexGridSizer->Layout();
    m_RenderFlexGridSizer->Layout();
    FitInside();
}

#if defined(STAND_ALONE)

#include <wx/aui/aui.h>


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

        void OnLayerVisible( int aLayer, bool isVisible, bool isFinal )
        {
            printf( "OnLayerVisible( aLayer:%d, isVisible:%d isFinal:%d)\n", aLayer, isVisible, isFinal );
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
        wxFrame( parent, -1, wxT( "wxAUI Test" ), wxDefaultPosition,
            wxSize( 800, 600 ), wxDEFAULT_FRAME_STYLE )
    {
        // notify wxAUI which frame to use
        m_mgr.SetManagedWindow( this );

        MYLAYERS* lw = new MYLAYERS( this, this );

        // add some layer rows
        static const LAYER_WIDGET::ROW layerRows[] = {
            LAYER_WIDGET::ROW( wxT("layer 1"), 0, RED, wxT("RED"), false ),
            LAYER_WIDGET::ROW( wxT("layer 2"), 1, GREEN, wxT("GREEN"), true ),
            LAYER_WIDGET::ROW( wxT("brown_layer"), 2, BROWN, wxT("BROWN"), true ),
            LAYER_WIDGET::ROW( wxT("layer_4_you"), 3, BLUE, wxT("BLUE"), false ),
        };

        lw->AppendLayerRows( layerRows, DIM(layerRows) );

        // add some render rows
        static const LAYER_WIDGET::ROW renderRows[] = {
            LAYER_WIDGET::ROW( wxT("With Very Large Ears"), 0, -1, wxT("Spock here") ),
            LAYER_WIDGET::ROW( wxT("With Legs"), 1, YELLOW ),
            LAYER_WIDGET::ROW( wxT("With Oval Eyes"), 1, BROWN, wxT("My eyes are upon you") ),
        };

        lw->AppendRenderRows( renderRows, DIM(renderRows) );

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


        wxTextCtrl* text2 = new wxTextCtrl( this, -1, wxT( "Pane 2 - sample text" ),
                                            wxDefaultPosition, wxSize( 200, 150 ),
                                            wxNO_BORDER | wxTE_MULTILINE );
        m_mgr.AddPane( text2, wxBOTTOM, wxT( "Pane Number Two" ) );

        wxTextCtrl* text3 = new wxTextCtrl( this, -1, wxT( "Main content window" ),
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
