
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010 KiCad Developers, see change_log.txt for contributors.
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
    @todo make the bitmapbutton a staticbitmap, and make dependent on the point size.
*/


//#define STAND_ALONE     1   // define to enable test program for LAYER_WIDGET


#include <layer_widget.h>

#include <macros.h>
#include <common.h>
#include <colors.h>
#include <wx/colour.h>

#define BUTT_SIZE_X             20
#define BUTT_SIZE_Y             18
#define BUTT_VOID               4

/* XPM
 * This bitmap is used for not selected layers
 */
static const char * clear_xpm[] = {
"10 14 1 1",
" 	c None",
"          ",
"          ",
"          ",
"          ",
"          ",
"          ",
"          ",
"          ",
"          ",
"          ",
"          ",
"          ",
"          ",
"          "};

/* XPM
 * This bitmap can be used to show a not selected layer
 * with special property (mainly not selected layers not in use in GerbView)
 */
static const char * clear_alternate_xpm[] = {
"10 14 4 1",
"       c None",
"X      c #008080",
"o      c GREEN",
"O      c #00B080",
"          ",
"          ",
"          ",
"          ",
"    X     ",
"   XXX    ",
"  XXXXX   ",
" OOOOOOO  ",
"  ooooo   ",
"   ooo    ",
"    o     ",
"          ",
"          ",
"          "};


/* XPM
 * This bitmap  is used for a normale selected layer
 */
static const char * rightarrow_xpm[] = {
"10 14 4 1",
"       c None",
"X      c #8080ff",
"o      c BLUE",
"O      c gray56",
"  X       ",
"  XX      ",
"  XXX     ",
"  XXXX    ",
"  XXXXX   ",
"  XXXXXX  ",
"  XXXXXXX ",
"  oooooooO",
"  ooooooO ",
"  oooooO  ",
"  ooooO   ",
"  oooO    ",
"  ooO     ",
"  oO      "};

/* XPM
 * This bitmap can be used to show the selected layer
 * with special property (mainly a layer in use in GerbView)
 */
static const char * rightarrow_alternate_xpm[] = {
"10 14 5 1",
"       c None",
".      c #00B000",
"X      c #8080ff",
"o      c BLUE",
"O      c gray56",
"..X       ",
"..XX      ",
"..XXX     ",
"..XXXX    ",
"..XXXXX   ",
"..XXXXXX  ",
"..XXXXXXX ",
"..oooooooO",
"..ooooooO ",
"..oooooO  ",
"..ooooO   ",
"..oooO    ",
"..ooO     ",
"..oO      "};


/**
 * Function makeColorTxt
 * returns a string containing the numeric value of the color.
 * in a form like 0x00000000.  (Color is currently an index, not RGB).
 */
static wxString makeColorTxt( EDA_COLOR_T aColor )
{
    wxString txt;
    txt.Printf( wxT("0x%08x"), aColor );
    return txt;
}


/**
 * Function shrinkFont
 * reduces the size of the wxFont associated with \a aControl
 */
static void shrinkFont( wxWindow* aControl, int aPointSize )
{
    wxFont font = aControl->GetFont();
    font.SetPointSize( aPointSize );
    aControl->SetFont( font );              // need this?
}


int LAYER_WIDGET::encodeId( int aColumn, int aId )
{
    int id = aId * LYR_COLUMN_COUNT + aColumn;
    return id;
}


LAYER_NUM LAYER_WIDGET::getDecodedId( int aControlId )
{
    int id = aControlId / LYR_COLUMN_COUNT;    // rounding is OK.
    return id;
}


wxBitmap LAYER_WIDGET::makeBitmap( EDA_COLOR_T aColor )
{
    // the bitmap will be BUTT_VOID*2 pixels smaller than the button, leaving a
    // border of BUTT_VOID pixels on each side.
    wxBitmap    bitmap( BUTT_SIZE_X - 2 * BUTT_VOID, BUTT_SIZE_Y - 2 * BUTT_VOID );
    wxBrush     brush;
    wxMemoryDC  iconDC;

    iconDC.SelectObject( bitmap );

    brush.SetColour( MakeColour( aColor ) );

#if wxCHECK_VERSION( 3, 0, 0 )
    brush.SetStyle( wxBRUSHSTYLE_SOLID );
#else
    brush.SetStyle( wxSOLID );
#endif

    iconDC.SetBrush( brush );

    iconDC.DrawRectangle( 0, 0, BUTT_SIZE_X - 2 * BUTT_VOID, BUTT_SIZE_Y - 2 * BUTT_VOID );

    return bitmap;
}


wxBitmapButton* LAYER_WIDGET::makeColorButton( wxWindow* aParent, EDA_COLOR_T aColor, int aID )
{
    // dynamically make a wxBitMap and brush it with the appropriate color,
    // then create a wxBitmapButton from it.
    wxBitmap bitmap = makeBitmap( aColor );

#ifndef __WXMAC__
    wxBitmapButton* ret = new wxBitmapButton( aParent, aID, bitmap,
        wxDefaultPosition, wxSize(BUTT_SIZE_X, BUTT_SIZE_Y), wxBORDER_RAISED );
#else
    wxBitmapButton* ret = new wxBitmapButton( aParent, aID, bitmap,
        wxDefaultPosition, wxSize(BUTT_SIZE_X, BUTT_SIZE_Y));
#endif
    // save the color value in the name, no where else to put it.
    ret->SetName( makeColorTxt( aColor ) );
    return ret;
}


void LAYER_WIDGET::OnLeftDownLayers( wxMouseEvent& event )
{
    int row;
    LAYER_NUM layer;

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

        layer = getDecodedId( getLayerComp( row, 0 )->GetId() );
    }

    else
    {
        // all nested controls on a given row will have their ID encoded with
        // encodeId(), and the corresponding decoding is getDecodedId()
        int id = eventSource->GetId();
        layer  = getDecodedId( id );
        row    = findLayerRow( layer );
    }

    if( OnLayerSelect( layer ) )    // if client allows this change.
        SelectLayerRow( row );

    passOnFocus();
}


void LAYER_WIDGET::OnMiddleDownLayerColor( wxMouseEvent& event )
{
    wxBitmapButton* eventSource = (wxBitmapButton*) event.GetEventObject();

    wxString colorTxt = eventSource->GetName();

    EDA_COLOR_T oldColor = ColorFromInt( strtoul( TO_UTF8(colorTxt), NULL, 0 ) );
    EDA_COLOR_T newColor = DisplayColorFrame( this, oldColor );

    if( newColor >= 0 )
    {
        eventSource->SetName( makeColorTxt( newColor ) );

        wxBitmap bm = makeBitmap( newColor );
        eventSource->SetBitmapLabel( bm );

        LAYER_NUM layer = getDecodedId( eventSource->GetId() );

        // tell the client code.
        OnLayerColorChange( layer, newColor );
    }

    passOnFocus();
}


void LAYER_WIDGET::OnLayerCheckBox( wxCommandEvent& event )
{
    wxCheckBox* eventSource = (wxCheckBox*) event.GetEventObject();
    LAYER_NUM layer = getDecodedId( eventSource->GetId() );
    OnLayerVisible( layer, eventSource->IsChecked() );
    passOnFocus();
}


void LAYER_WIDGET::OnMiddleDownRenderColor( wxMouseEvent& event )
{
    wxBitmapButton* eventSource = (wxBitmapButton*) event.GetEventObject();

    wxString colorTxt = eventSource->GetName();

    EDA_COLOR_T oldColor = ColorFromInt( strtoul( TO_UTF8(colorTxt), NULL, 0 ) );
    EDA_COLOR_T newColor = DisplayColorFrame( this, oldColor );

    if( newColor >= 0 )
    {
        eventSource->SetName( makeColorTxt( newColor ) );

        wxBitmap bm = makeBitmap( newColor );
        eventSource->SetBitmapLabel( bm );

        LAYER_NUM id = getDecodedId( eventSource->GetId() );

        // tell the client code.
        OnRenderColorChange( id, newColor );
    }
    passOnFocus();
}


void LAYER_WIDGET::OnRenderCheckBox( wxCommandEvent& event )
{
    wxCheckBox* eventSource = (wxCheckBox*) event.GetEventObject();
    LAYER_NUM id = getDecodedId( eventSource->GetId() );
    OnRenderEnable( id, eventSource->IsChecked() );
    passOnFocus();
}


void LAYER_WIDGET::OnTabChange( wxNotebookEvent& event )
{
//    wxFocusEvent    event( wxEVT_SET_FOCUS );
//    m_FocusOwner->AddPendingEvent( event );

    passOnFocus();      // does not work in this context, probably because we have receive control here too early.
}


wxWindow* LAYER_WIDGET::getLayerComp( int aRow, int aColumn ) const
{
    unsigned ndx = aRow * LYR_COLUMN_COUNT + aColumn;
    if( ndx < m_LayersFlexGridSizer->GetChildren().GetCount() )
        return m_LayersFlexGridSizer->GetChildren()[ndx]->GetWindow();
    return NULL;
}


int LAYER_WIDGET::findLayerRow( LAYER_NUM aLayer ) const
{
    int count = GetLayerRowCount();
    for( int row=0;  row<count;  ++row )
    {
        // column 0 in the layer scroll window has a wxStaticBitmap, get its ID.
        wxWindow* w = getLayerComp( row, 0 );
        wxASSERT( w );

        if( aLayer == getDecodedId( w->GetId() ))
            return row;
    }
    return -1;
}


wxWindow* LAYER_WIDGET::getRenderComp( int aRow, int aColumn ) const
{
    int ndx = aRow * RND_COLUMN_COUNT + aColumn;
    if( (unsigned) ndx < m_RenderFlexGridSizer->GetChildren().GetCount() )
        return m_RenderFlexGridSizer->GetChildren()[ndx]->GetWindow();
    return NULL;
}


int LAYER_WIDGET::findRenderRow( int aId ) const
{
    int count = GetRenderRowCount();
    for( int row=0;  row<count;  ++row )
    {
        // column 0 in the layer scroll window has a wxStaticBitmap, get its ID.
        wxWindow* w = getRenderComp( row, 0 );
        wxASSERT( w );

        if( aId == getDecodedId( w->GetId() ))
            return row;
    }
    return -1;
}


void LAYER_WIDGET::insertLayerRow( int aRow, const ROW& aSpec )
{
    wxASSERT( aRow >= 0 );

    int         col;
    int         index = aRow * LYR_COLUMN_COUNT;
    const int   flags = wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT;

    // column 0
    col = 0;
    wxStaticBitmap* sbm = new wxStaticBitmap( m_LayerScrolledWindow, encodeId( col, aSpec.id ),
                            useAlternateBitmap(aRow) ? *m_BlankAlternateBitmap : *m_BlankBitmap,
                            wxDefaultPosition, m_BitmapSize );
    sbm->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnLeftDownLayers ), NULL, this );
    m_LayersFlexGridSizer->wxSizer::Insert( index+col, sbm, 0, flags );

    // column 1
    col = 1;
    wxBitmapButton* bmb = makeColorButton( m_LayerScrolledWindow, aSpec.color, encodeId( col, aSpec.id ) );
    bmb->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnLeftDownLayers ), NULL, this );
    bmb->Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnMiddleDownLayerColor ), NULL, this );
    bmb->SetToolTip( _("Left click to select, middle click for color change, right click for menu" ) );
    m_LayersFlexGridSizer->wxSizer::Insert( index+col, bmb, 0, flags );

    // column 2
    col = 2;
    wxStaticText* st = new wxStaticText( m_LayerScrolledWindow, encodeId( col, aSpec.id ), aSpec.rowName );
    shrinkFont( st, m_PointSize );
    st->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnLeftDownLayers ), NULL, this );
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
    wxASSERT( aRow >= 0 );

    int         col;
    int         index = aRow * RND_COLUMN_COUNT;
    const int   flags = wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT;

    // column 0
    col = 0;
    if( aSpec.color != -1 )
    {
        wxBitmapButton* bmb = makeColorButton( m_RenderScrolledWindow, aSpec.color, encodeId( col, aSpec.id ) );
        bmb->Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( LAYER_WIDGET::OnMiddleDownRenderColor ), NULL, this );
        bmb->SetToolTip( _( "Middle click for color change" ) );
        m_RenderFlexGridSizer->wxSizer::Insert( index+col, bmb, 0, flags );

        // could add a left click handler on the color button that toggles checkbox.
    }
    else    // == -1, no color selection wanted
    {
        // need a place holder within the sizer to keep grid full.
        wxPanel* invisible = new wxPanel( m_RenderScrolledWindow, encodeId( col, aSpec.id ) );
        m_RenderFlexGridSizer->wxSizer::Insert( index+col, invisible, 0, flags );
    }

    // column 1
    col = 1;
    wxCheckBox* cb = new wxCheckBox( m_RenderScrolledWindow, encodeId( col, aSpec.id ),
                        aSpec.rowName, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    shrinkFont( cb, m_PointSize );
    cb->SetValue( aSpec.state );
    cb->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED,
        wxCommandEventHandler( LAYER_WIDGET::OnRenderCheckBox ), NULL, this );
    cb->SetToolTip( aSpec.tooltip );
    m_RenderFlexGridSizer->wxSizer::Insert( index+col, cb, 0, flags );
}


void LAYER_WIDGET::passOnFocus()
{
    m_FocusOwner->SetFocus();
}


//-----<public>-------------------------------------------------------

LAYER_WIDGET::LAYER_WIDGET( wxWindow* aParent, wxWindow* aFocusOwner, int aPointSize,
        wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) :
    wxPanel( aParent, id, pos, size, style )
{
    wxBoxSizer* boxSizer = new wxBoxSizer( wxVERTICAL );

    m_notebook = new wxAuiNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP );

    wxFont font = m_notebook->GetFont();

    if( aPointSize == -1 )
    {
        m_PointSize = font.GetPointSize();
    }
    else
    {
        m_PointSize = aPointSize;

        // change the font size on the notebook's tabs to match aPointSize
        font.SetPointSize( aPointSize );
        m_notebook->SetFont( font );
        m_notebook->SetNormalFont( font );
        m_notebook->SetSelectedFont( font );
        m_notebook->SetMeasuringFont( font );
    }

    m_LayerPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

    wxBoxSizer* bSizer3;
    bSizer3 = new wxBoxSizer( wxVERTICAL );

    m_LayerScrolledWindow = new wxScrolledWindow( m_LayerPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER );
    m_LayerScrolledWindow->SetScrollRate( 5, 5 );
    m_LayersFlexGridSizer = new wxFlexGridSizer( 0, 4, 0, 1 );
    m_LayersFlexGridSizer->SetFlexibleDirection( wxHORIZONTAL );
    m_LayersFlexGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    m_LayerScrolledWindow->SetSizer( m_LayersFlexGridSizer );
    m_LayerScrolledWindow->Layout();
    m_LayersFlexGridSizer->Fit( m_LayerScrolledWindow );
    bSizer3->Add( m_LayerScrolledWindow, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 2 );

    m_LayerPanel->SetSizer( bSizer3 );
    m_LayerPanel->Layout();
    bSizer3->Fit( m_LayerPanel );
    m_notebook->AddPage( m_LayerPanel, _( "Layer" ), true );
    m_RenderingPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

    wxBoxSizer* bSizer4;
    bSizer4 = new wxBoxSizer( wxVERTICAL );

    m_RenderScrolledWindow = new wxScrolledWindow( m_RenderingPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER );
    m_RenderScrolledWindow->SetScrollRate( 5, 5 );
    m_RenderFlexGridSizer = new wxFlexGridSizer( 0, 2, 0, 1 );
    m_RenderFlexGridSizer->SetFlexibleDirection( wxHORIZONTAL );
    m_RenderFlexGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );

    m_RenderScrolledWindow->SetSizer( m_RenderFlexGridSizer );
    m_RenderScrolledWindow->Layout();
    m_RenderFlexGridSizer->Fit( m_RenderScrolledWindow );
    bSizer4->Add( m_RenderScrolledWindow, 1, wxALL|wxEXPAND, 5 );

    m_RenderingPanel->SetSizer( bSizer4 );
    m_RenderingPanel->Layout();
    bSizer4->Fit( m_RenderingPanel );
    m_notebook->AddPage( m_RenderingPanel, _( "Render" ), false );

    boxSizer->Add( m_notebook, 1, wxEXPAND | wxALL, 5 );

    SetSizer( boxSizer );

    m_FocusOwner = aFocusOwner;

    m_CurrentRow = -1;  // hide the arrow initially

    m_RightArrowBitmap = new wxBitmap( rightarrow_xpm );
    m_RightArrowAlternateBitmap = new wxBitmap( rightarrow_alternate_xpm );

    m_BlankBitmap = new wxBitmap( clear_xpm );     // translucent
    m_BlankAlternateBitmap = new wxBitmap( clear_alternate_xpm );
    m_BitmapSize = wxSize(m_BlankBitmap->GetWidth(), m_BlankBitmap->GetHeight());

    // trap the tab changes so that we can call passOnFocus().
    m_notebook->Connect( -1, wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED,
        wxNotebookEventHandler( LAYER_WIDGET::OnTabChange ), NULL, this );

    Layout();
}


wxSize LAYER_WIDGET::GetBestSize() const
{
#if 0
    wxSize layerz  = m_LayersFlexGridSizer->GetMinSize();
    wxSize renderz = m_RenderFlexGridSizer->GetMinSize();

    wxSize  clientz( std::max(renderz.x,layerz.x), std::max(renderz.y,layerz.y) );

    return ClientToWindowSize( clientz );

#else

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

    layerz += m_LayerPanel->GetWindowBorderSize();


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

    renderz += m_RenderingPanel->GetWindowBorderSize();

    wxSize clientz( std::max(renderz.x,layerz.x), std::max(renderz.y,layerz.y) );

//    wxSize diffz( GetSize() - GetClientSize() );
//    clientz += diffz;

    return clientz;

#endif

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
    m_notebook->SetSelection( 0 );

    wxStaticBitmap* oldbm = (wxStaticBitmap*) getLayerComp( m_CurrentRow, 0 );
    if( oldbm )
        oldbm->SetBitmap( useAlternateBitmap(m_CurrentRow) ? *m_BlankAlternateBitmap : *m_BlankBitmap );

    wxStaticBitmap* newbm = (wxStaticBitmap*) getLayerComp( aRow, 0 );
    if( newbm )
    {
        newbm->SetBitmap( useAlternateBitmap(aRow) ? *m_RightArrowAlternateBitmap : *m_RightArrowBitmap );

        // Make sure the desired layer row is visible.
        // It seems that as of 2.8.2, setting the focus does this.
        // I don't expect the scrolling to be needed at all because
        // the minimum window size may end up being established so that the
        // scroll bars will not be visible.
        getLayerComp( aRow, 1 )->SetFocus();
    }

    m_CurrentRow = aRow;

    // give the focus back to the app.
    passOnFocus();
}


void LAYER_WIDGET::SelectLayer( LAYER_NUM aLayer )
{
    int row = findLayerRow( aLayer );
    SelectLayerRow( row );
}


LAYER_NUM LAYER_WIDGET::GetSelectedLayer()
{
    wxWindow* w = getLayerComp( m_CurrentRow, 0 );
    if( w )
        return getDecodedId( w->GetId() );

    return UNDEFINED_LAYER;
}


void LAYER_WIDGET::SetLayerVisible( LAYER_NUM aLayer, bool isVisible )
{
    int row = findLayerRow( aLayer );
    if( row >= 0 )
    {
        wxCheckBox* cb = (wxCheckBox*) getLayerComp( row, 3 );
        wxASSERT( cb );
        cb->SetValue( isVisible );      // does not fire an event
    }
}


bool LAYER_WIDGET::IsLayerVisible( LAYER_NUM aLayer )
{
    int row = findLayerRow( aLayer );
    if( row >= 0 )
    {
        wxCheckBox* cb = (wxCheckBox*) getLayerComp( row, 3 );
        wxASSERT( cb );
        return cb->GetValue();
    }
    return false;
}


void LAYER_WIDGET::SetLayerColor( LAYER_NUM aLayer, EDA_COLOR_T aColor )
{
    int row = findLayerRow( aLayer );
    if( row >= 0 )
    {
        int col = 1;    // bitmap button is column 1
        wxBitmapButton* bmb = (wxBitmapButton*) getLayerComp( row, col );
        wxASSERT( bmb );

        wxBitmap bm = makeBitmap( aColor );

        bmb->SetBitmapLabel( bm );
        bmb->SetName( makeColorTxt( aColor ) ); // save color value in name as string
    }
}


EDA_COLOR_T LAYER_WIDGET::GetLayerColor( LAYER_NUM aLayer ) const
{
    int row = findLayerRow( aLayer );
    if( row >= 0 )
    {
        int col = 1;    // bitmap button is column 1
        wxBitmapButton* bmb = (wxBitmapButton*) getLayerComp( row, col );
        wxASSERT( bmb );

        wxString colorTxt = bmb->GetName();
        return ColorFromInt( strtoul( TO_UTF8(colorTxt), NULL, 0 ) );
    }

    return UNSPECIFIED_COLOR;   // it's caller fault, gave me a bad layer
}


void LAYER_WIDGET::SetRenderState( int aId, bool isSet )
{
    int row = findRenderRow( aId );
    if( row >= 0 )
    {
        int col = 1;    // checkbox is column 1
        wxCheckBox* cb = (wxCheckBox*) getRenderComp( row, col );
        wxASSERT( cb );
        cb->SetValue( isSet );  // does not fire an event
    }
}


bool LAYER_WIDGET::GetRenderState( int aId )
{
    int row = findRenderRow( aId );
    if( row >= 0 )
    {
        int col = 1;    // checkbox is column 1
        wxCheckBox* cb = (wxCheckBox*) getRenderComp( row, col );
        wxASSERT( cb );
        return cb->GetValue();
    }
    return false;   // the value of a non-existent row
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
    public:
        // your constructor could take a BOARD argument.  here I leave it
        // out because this source module wants to know nothing of BOARDs
        // to maximize re-use.
        MYLAYERS( wxWindow* aParent ) :
            LAYER_WIDGET( aParent, aParent  )
        {
        }

        void OnLayerColorChange( int aLayer, EDA_COLOR_T aColor )
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

        bool OnLayerSelect( LAYER aLayer )
        {
            printf( "OnLayerSelect( aLayer:%d )\n", aLayer );
            return true;
        }

        void OnLayerVisible( LAYER aLayer, bool isVisible, bool isFinal )
        {
            printf( "OnLayerVisible( aLayer:%d, isVisible:%d isFinal:%d)\n", aLayer, isVisible, isFinal );
        }

        void OnRenderColorChange( int aId, EDA_COLOR_T aColor )
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

        MYLAYERS* lw = new MYLAYERS( this );

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
