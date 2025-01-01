
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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



/*  This source module implements the layer visibility and selection widget
    @todo make bitmap size dependent on the point size.
*/


//#define STAND_ALONE     1   // define to enable test program for LAYER_WIDGET


#include "layer_widget.h"

#include <bitmaps.h>
#include <macros.h>
#include <widgets/indicator_icon.h>
#include <widgets/wx_ellipsized_static_text.h>
#include <widgets/ui_common.h>
#include <wx/checkbox.h>
#include <wx/menu.h>

#include <algorithm>


const wxEventType LAYER_WIDGET::EVT_LAYER_COLOR_CHANGE = wxNewEventType();


/**
 * Reduce the size of the wxFont associated with \a aControl.
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


int LAYER_WIDGET::getDecodedId( int aControlId )
{
    int id = aControlId / LYR_COLUMN_COUNT;    // rounding is OK.
    return id;
}


void LAYER_WIDGET::OnLeftDownLayers( wxMouseEvent& event )
{
    int row;
    int layer;

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


void LAYER_WIDGET::OnRightDownLayer( wxMouseEvent& aEvent, COLOR_SWATCH* aColorSwatch,
                                     const wxString& aLayerName )
{
    wxMenu menu;

    KIUI::AddMenuItem( &menu, ID_CHANGE_LAYER_COLOR,
                       _( "Change Layer Color for" ) + wxS( " " ) + aLayerName,
                       KiBitmap( BITMAPS::color_materials ) );
    menu.AppendSeparator();

    OnLayerRightClick( menu );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED, [aColorSwatch]( wxCommandEvent& event )
                                            {
                                                if( event.GetId() == ID_CHANGE_LAYER_COLOR )
                                                {
                                                    aColorSwatch->GetNewSwatchColor();
                                                }
                                                else
                                                {
                                                    event.Skip();
                                                }
                                            } );

    PopupMenu( &menu );
    passOnFocus();
}


void LAYER_WIDGET::OnLayerSwatchChanged( wxCommandEvent& aEvent )
{
    COLOR_SWATCH* eventSource = static_cast<COLOR_SWATCH*>( aEvent.GetEventObject() );
    COLOR4D       newColor = eventSource->GetSwatchColor();
    int           layer = getDecodedId( eventSource->GetId() );

    // tell the client code.
    OnLayerColorChange( layer, newColor );

    // notify others
    wxCommandEvent event( EVT_LAYER_COLOR_CHANGE );
    wxPostEvent( this, event );

    passOnFocus();
}


void LAYER_WIDGET::OnLayerCheckBox( wxCommandEvent& event )
{
    wxCheckBox* eventSource = (wxCheckBox*) event.GetEventObject();
    int         layer = getDecodedId( eventSource->GetId() );

    OnLayerVisible( layer, eventSource->IsChecked() );
    passOnFocus();
}


void LAYER_WIDGET::OnRightDownRender( wxMouseEvent& aEvent, COLOR_SWATCH* aColorSwatch,
                                      const wxString& aRenderName )
{
    wxMenu menu;

    KIUI::AddMenuItem( &menu, ID_CHANGE_RENDER_COLOR,
                       _( "Change Render Color for" ) + wxS( " " )+ aRenderName,
                       KiBitmap( BITMAPS::color_materials ) );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED,
               [aColorSwatch]( wxCommandEvent& event )
               {
                   if( event.GetId() == ID_CHANGE_RENDER_COLOR )
                       aColorSwatch->GetNewSwatchColor();
                   else
                       event.Skip();
               } );

    PopupMenu( &menu );
    passOnFocus();
}


void LAYER_WIDGET::OnRenderSwatchChanged( wxCommandEvent& aEvent )
{
    COLOR_SWATCH* eventSource = static_cast<COLOR_SWATCH*>( aEvent.GetEventObject() );
    COLOR4D       newColor = eventSource->GetSwatchColor();
    int           id = getDecodedId( eventSource->GetId() );

    if( id == LAYER_PCB_BACKGROUND )
    {
        // Update all swatch backgrounds
        int col = 1;    // bitmap button is column 1 in layers tab

        for( int row = 0; row < GetLayerRowCount(); ++row )
        {
            COLOR_SWATCH* swatch = dynamic_cast<COLOR_SWATCH*>( getLayerComp( row, col ) );

            if( swatch )
                swatch->SetSwatchBackground( newColor );
        }

        col = 0;    // bitmap button is column 0 in render tab

        for( int row = 0; row < GetRenderRowCount(); ++row )
        {
            COLOR_SWATCH* swatch = dynamic_cast<COLOR_SWATCH*>( getRenderComp( row, col ) );

            if( swatch )
                swatch->SetSwatchBackground( newColor );
        }
    }

    // tell the client code.
    OnRenderColorChange( id, newColor );

    passOnFocus();
}


void LAYER_WIDGET::OnRenderCheckBox( wxCommandEvent& event )
{
    wxCheckBox* eventSource = (wxCheckBox*) event.GetEventObject();
    int         id = getDecodedId( eventSource->GetId() );

    OnRenderEnable( id, eventSource->IsChecked() );
    passOnFocus();
}


void LAYER_WIDGET::OnTabChange( wxNotebookEvent& event )
{
//    wxFocusEvent    event( wxEVT_SET_FOCUS );
//    m_FocusOwner->AddPendingEvent( event );

    // Does not work in this context, probably because we have receive control here too early.
    passOnFocus();
}


wxWindow* LAYER_WIDGET::getLayerComp( int aRow, int aColumn ) const
{
    unsigned ndx = aRow * LYR_COLUMN_COUNT + aColumn;

    if( ndx < m_LayersFlexGridSizer->GetChildren().GetCount() )
        return m_LayersFlexGridSizer->GetChildren()[ndx]->GetWindow();

    return nullptr;
}


int LAYER_WIDGET::findLayerRow( int aLayer ) const
{
    int count = GetLayerRowCount();

    for( int row = 0; row < count; ++row )
    {
        // column 0 in the layer scroll window has a wxStaticBitmap, get its ID.
        wxWindow* w = getLayerComp( row, 0 );
        wxASSERT( w );

        if( aLayer == getDecodedId( w->GetId() ) )
            return row;
    }

    return -1;
}


wxWindow* LAYER_WIDGET::getRenderComp( int aRow, int aColumn ) const
{
    int ndx = aRow * RND_COLUMN_COUNT + aColumn;

    if( (unsigned) ndx < m_RenderFlexGridSizer->GetChildren().GetCount() )
        return m_RenderFlexGridSizer->GetChildren()[ndx]->GetWindow();

    return nullptr;
}


int LAYER_WIDGET::findRenderRow( int aId ) const
{
    int count = GetRenderRowCount();

    for( int row = 0; row < count; ++row )
    {
        // column 0 in the layer scroll window has a wxStaticBitmap, get its ID.
        wxWindow* w = getRenderComp( row, 0 );
        wxASSERT( w );

        if( aId == getDecodedId( w->GetId() ) )
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
    col = COLUMN_ICON_ACTIVE;
    auto sbm = new INDICATOR_ICON( m_LayerScrolledWindow, *m_IconProvider,
                                   ROW_ICON_PROVIDER::STATE::OFF, encodeId( col, aSpec.id ) );
    sbm->Bind( wxEVT_LEFT_DOWN, &LAYER_WIDGET::OnLeftDownLayers, this );
    m_LayersFlexGridSizer->wxSizer::Insert( index+col, sbm, 0, flags );

    // column 1 (COLUMN_COLORBM)
    col = COLUMN_COLORBM;

    auto bmb = new COLOR_SWATCH( m_LayerScrolledWindow, aSpec.color, encodeId( col, aSpec.id ),
                                 getBackgroundLayerColor(), aSpec.defaultColor, SWATCH_SMALL );
    bmb->Bind( wxEVT_LEFT_DOWN, &LAYER_WIDGET::OnLeftDownLayers, this );
    bmb->Bind( COLOR_SWATCH_CHANGED, &LAYER_WIDGET::OnLayerSwatchChanged, this );
    bmb->SetToolTip( _( "Left double click or middle click for color change, right click for "
                        "menu" ) );
    m_LayersFlexGridSizer->wxSizer::Insert( index+col, bmb, 0, flags | wxRIGHT, 2 );

    // column 2 (COLUMN_COLOR_LYR_CB)
    col = COLUMN_COLOR_LYR_CB;
    wxCheckBox* cb = new wxCheckBox( m_LayerScrolledWindow, encodeId( col, aSpec.id ),
                                     wxEmptyString );
    cb->SetValue( aSpec.state );
    cb->Bind( wxEVT_COMMAND_CHECKBOX_CLICKED, &LAYER_WIDGET::OnLayerCheckBox, this );
    cb->SetToolTip( _( "Enable this for visibility" ) );
    m_LayersFlexGridSizer->wxSizer::Insert( index+col, cb, 0, flags );

    // column 3 (COLUMN_COLOR_LYRNAME)
    col = COLUMN_COLOR_LYRNAME;
    WX_ELLIPSIZED_STATIC_TEXT* st = new WX_ELLIPSIZED_STATIC_TEXT( m_LayerScrolledWindow,
                                                                   encodeId( col, aSpec.id ),
                                                                   aSpec.rowName, wxDefaultPosition,
                                                                   wxDefaultSize,
                                                                   wxST_ELLIPSIZE_MIDDLE );
    shrinkFont( st, m_PointSize );
    st->Bind( wxEVT_LEFT_DOWN, &LAYER_WIDGET::OnLeftDownLayers, this );
    st->SetToolTip( aSpec.tooltip );
    st->SetMinimumStringLength( m_smallestLayerString );
    m_LayersFlexGridSizer->wxSizer::Insert( index+col, st, 0, flags | wxEXPAND );

    // column 4 (COLUMN_ALPHA_INDICATOR)
    col = COLUMN_ALPHA_INDICATOR;
    sbm = new INDICATOR_ICON( m_LayerScrolledWindow, *m_IconProvider,
                              ROW_ICON_PROVIDER::STATE::OFF, wxID_ANY );
    m_LayersFlexGridSizer->wxSizer::Insert( index+col, sbm, 0, flags );

    // Bind right click eventhandler to all columns
    wxString layerName( aSpec.rowName );

    sbm->Bind( wxEVT_RIGHT_DOWN, [this, bmb, layerName] ( wxMouseEvent& aEvt )
                                 {
                                     OnRightDownLayer( aEvt, bmb, layerName );
                                 } );
    bmb->Bind( wxEVT_RIGHT_DOWN, [this, bmb, layerName] ( wxMouseEvent& aEvt )
                                 {
                                     OnRightDownLayer( aEvt, bmb, layerName );
                                 } );
    cb->Bind( wxEVT_RIGHT_DOWN, [this, bmb, layerName] ( wxMouseEvent& aEvt )
                                {
                                    OnRightDownLayer( aEvt, bmb, layerName );
                                } );
    st->Bind( wxEVT_RIGHT_DOWN, [this, bmb, layerName] ( wxMouseEvent& aEvt )
                                {
                                    OnRightDownLayer( aEvt, bmb, layerName );
                                } );
}


void LAYER_WIDGET::updateLayerRow( int aRow, const wxString& aName )
{
    wxStaticText* label = dynamic_cast<wxStaticText*>( getLayerComp( aRow, COLUMN_COLOR_LYRNAME ) );

    if( label )
        label->SetLabel( aName );

    INDICATOR_ICON* indicator = (INDICATOR_ICON*) getLayerComp( aRow, 0 );

    if( indicator )
    {
        if( aRow == m_CurrentRow )
            indicator->SetIndicatorState( ROW_ICON_PROVIDER::STATE::ON );
        else
            indicator->SetIndicatorState( ROW_ICON_PROVIDER::STATE::OFF );
    }
}


void LAYER_WIDGET::insertRenderRow( int aRow, const ROW& aSpec )
{
    wxASSERT( aRow >= 0 );

    int         col;
    int         index = aRow * RND_COLUMN_COUNT;
    const int   flags = wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT;

    wxString renderName( aSpec.rowName );
    wxCheckBox* cb = nullptr;

    // column 1
    if( !aSpec.spacer )
    {
        col = 1;
        cb = new wxCheckBox( m_RenderScrolledWindow, encodeId( col, aSpec.id ),
                             aSpec.rowName, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
        shrinkFont( cb, m_PointSize );
        cb->SetValue( aSpec.state );
        cb->Enable( aSpec.changeable );
        cb->Bind( wxEVT_COMMAND_CHECKBOX_CLICKED, &LAYER_WIDGET::OnRenderCheckBox, this );
        cb->SetToolTip( aSpec.tooltip );
    }

    // column 0
    col = 0;

    if( aSpec.color != COLOR4D::UNSPECIFIED )
    {
        auto bmb = new COLOR_SWATCH( m_RenderScrolledWindow, aSpec.color, encodeId( col, aSpec.id ),
                                     getBackgroundLayerColor(), aSpec.defaultColor, SWATCH_SMALL );
        bmb->Bind( COLOR_SWATCH_CHANGED, &LAYER_WIDGET::OnRenderSwatchChanged, this );
        bmb->SetToolTip( _( "Left double click or middle click for color change" ) );
        m_RenderFlexGridSizer->wxSizer::Insert( index+col, bmb, 0, flags );

        bmb->Bind( wxEVT_RIGHT_DOWN, [this, bmb, renderName] ( wxMouseEvent& aEvt )
                                     {
                                         OnRightDownRender( aEvt, bmb, renderName );
                                     } );
        cb->Bind( wxEVT_RIGHT_DOWN, [this, bmb, renderName] ( wxMouseEvent& aEvt )
                                    {
                                        OnRightDownRender( aEvt, bmb, renderName );
                                    } );

        // could add a left click handler on the color button that toggles checkbox.
    }
    else    // == -1, no color selection wanted
    {
        // need a place holder within the sizer to keep grid full.
        wxPanel* invisible = new wxPanel( m_RenderScrolledWindow, encodeId( col, aSpec.id ) );
        m_RenderFlexGridSizer->wxSizer::Insert( index+col, invisible, 0, flags );
    }

    // Items have to be inserted in order
    col = 1;

    if( aSpec.spacer )
    {
        wxPanel* invisible = new wxPanel( m_RenderScrolledWindow, wxID_ANY );
        m_RenderFlexGridSizer->wxSizer::Insert( index+col, invisible, 0, flags );
    }
    else
    {
        m_RenderFlexGridSizer->wxSizer::Insert( index+col, cb, 0, flags );
    }
}


void LAYER_WIDGET::passOnFocus()
{
    m_FocusOwner->SetFocus();
}


LAYER_WIDGET::LAYER_WIDGET( wxWindow* aParent, wxWindow* aFocusOwner, wxWindowID id,
                            const wxPoint& pos, const wxSize& size, long style ) :
    wxPanel( aParent, id, pos, size, style ),
    m_smallestLayerString( wxT( "M...M" ) )
{
    m_IconProvider = new ROW_ICON_PROVIDER( KIUI::c_IndicatorSizeDIP, this );

    int pointSize = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ).GetPointSize();
    int screenHeight = wxSystemSettings::GetMetric( wxSYS_SCREEN_Y );

    if( screenHeight <= 900 && pointSize >= FromDIP( KIUI::c_IndicatorSizeDIP ) )
        pointSize = pointSize * 8 / 10;

    m_PointSize = pointSize;

    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP );

    wxFont font = m_notebook->GetFont();

    // change the font size on the notebook's tabs to match aPointSize
    font.SetPointSize( pointSize );
    m_notebook->SetFont( font );

    m_LayerPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxTAB_TRAVERSAL );

    wxBoxSizer* layerPanelSizer;
    layerPanelSizer = new wxBoxSizer( wxVERTICAL );

    m_LayerScrolledWindow = new wxScrolledWindow( m_LayerPanel, wxID_ANY, wxDefaultPosition,
                                                  wxDefaultSize, wxNO_BORDER );
    m_LayerScrolledWindow->SetScrollRate( 5, 5 );
    m_LayersFlexGridSizer = new wxFlexGridSizer( 0, LYR_COLUMN_COUNT, 3, 4 );
    m_LayersFlexGridSizer->SetFlexibleDirection( wxHORIZONTAL );
    m_LayersFlexGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );

    // Make column 3 growable/stretchable
    m_LayersFlexGridSizer->AddGrowableCol( 3, 1 );

    m_LayerScrolledWindow->SetSizer( m_LayersFlexGridSizer );
    m_LayerScrolledWindow->Layout();
    m_LayersFlexGridSizer->Fit( m_LayerScrolledWindow );
    layerPanelSizer->Add( m_LayerScrolledWindow, 1, wxBOTTOM | wxEXPAND | wxLEFT | wxTOP, 2 );

    m_LayerPanel->SetSizer( layerPanelSizer );
    m_LayerPanel->Layout();
    layerPanelSizer->Fit( m_LayerPanel );
    m_notebook->AddPage( m_LayerPanel, _( "Layers" ), true );
    m_RenderingPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                    wxTAB_TRAVERSAL );

    wxBoxSizer* renderPanelSizer;
    renderPanelSizer = new wxBoxSizer( wxVERTICAL );

    m_RenderScrolledWindow = new wxScrolledWindow( m_RenderingPanel, wxID_ANY, wxDefaultPosition,
                                                   wxDefaultSize, wxNO_BORDER );
    m_RenderScrolledWindow->SetScrollRate( 5, 5 );
    m_RenderFlexGridSizer = new wxFlexGridSizer( 0, RND_COLUMN_COUNT, 3, 4 );
    m_RenderFlexGridSizer->SetFlexibleDirection( wxHORIZONTAL );
    m_RenderFlexGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );

    m_RenderScrolledWindow->SetSizer( m_RenderFlexGridSizer );
    m_RenderScrolledWindow->Layout();
    m_RenderFlexGridSizer->Fit( m_RenderScrolledWindow );
    renderPanelSizer->Add( m_RenderScrolledWindow, 1, wxALL | wxEXPAND, 5 );

    m_RenderingPanel->SetSizer( renderPanelSizer );
    m_RenderingPanel->Layout();
    renderPanelSizer->Fit( m_RenderingPanel );
    m_notebook->AddPage( m_RenderingPanel, _( "Items" ), false );

    mainSizer->Add( m_notebook, 1, wxEXPAND, 5 );

    SetSizer( mainSizer );

    m_FocusOwner = aFocusOwner;

    m_CurrentRow = -1;  // hide the arrow initially

    // trap the tab changes so that we can call passOnFocus().
    m_notebook->Bind( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, &LAYER_WIDGET::OnTabChange, this );

    Layout();
}


LAYER_WIDGET::~LAYER_WIDGET()
{
    delete m_IconProvider;
}


wxSize LAYER_WIDGET::GetBestSize() const
{
    // size of m_LayerScrolledWindow --------------
    wxArrayInt widths = m_LayersFlexGridSizer->GetColWidths();
    int totWidth = 0;

    for( int i = 0; i < LYR_COLUMN_COUNT && i < (int)widths.GetCount(); ++i )
        totWidth += widths[i];

    // Account for the parent's frame:
    totWidth += 15;

    /* The minimum height is a small size to properly force computation
     * of the panel's scrollbars (otherwise it will assume it *has* all
     * this space) */
    unsigned totHeight = 32;

    wxSize layerz( totWidth, totHeight );

    layerz += m_LayerPanel->GetWindowBorderSize();

    // size of m_RenderScrolledWindow --------------
    widths = m_RenderFlexGridSizer->GetColWidths();
    totWidth = 0;

    for( int i = 0; i < RND_COLUMN_COUNT && i < (int)widths.GetCount(); ++i )
        totWidth += widths[i];

    // account for the parent's frame, this one has void space of 10 PLUS a border:
    totWidth += 15;

    // For totHeight re-use the previous small one
    wxSize renderz( totWidth, totHeight );

    renderz += m_RenderingPanel->GetWindowBorderSize();

    wxSize clientz( std::max(renderz.x,layerz.x), std::max(renderz.y,layerz.y) );

    return clientz;
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
}


void LAYER_WIDGET::ClearLayerRows()
{
    m_LayersFlexGridSizer->Clear( true );
}


void LAYER_WIDGET::AppendRenderRow( const ROW& aRow )
{
    int nextRow = GetRenderRowCount();
    insertRenderRow( nextRow, aRow );
}


void LAYER_WIDGET::ClearRenderRows()
{
    m_RenderFlexGridSizer->Clear( true );
}


void LAYER_WIDGET::SelectLayerRow( int aRow )
{
    INDICATOR_ICON* oldIndicator = (INDICATOR_ICON*) getLayerComp( m_CurrentRow, 0 );

    if( oldIndicator )
        oldIndicator->SetIndicatorState( ROW_ICON_PROVIDER::STATE::OFF );

    INDICATOR_ICON* newIndicator = (INDICATOR_ICON*) getLayerComp( aRow, 0 );

    if( newIndicator )
    {
        newIndicator->SetIndicatorState( ROW_ICON_PROVIDER::STATE::ON );
    }

    m_CurrentRow = aRow;

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
    wxWindow* w = getLayerComp( m_CurrentRow, 0 );

    if( w )
        return getDecodedId( w->GetId() );

    return UNDEFINED_LAYER;
}


void LAYER_WIDGET::SetLayerVisible( int aLayer, bool isVisible )
{
    setLayerCheckbox( aLayer, isVisible );
    OnLayerVisible( aLayer, isVisible );
}


void LAYER_WIDGET::setLayerCheckbox( int aLayer, bool isVisible )
{
    int row = findLayerRow( aLayer );

    if( row >= 0 )
    {
        wxCheckBox* cb = (wxCheckBox*) getLayerComp( row, COLUMN_COLOR_LYR_CB );
        wxASSERT( cb );
        cb->SetValue( isVisible );      // does not fire an event
    }
}


bool LAYER_WIDGET::IsLayerVisible( int aLayer )
{
    int row = findLayerRow( aLayer );

    if( row >= 0 )
    {
        wxCheckBox* cb = (wxCheckBox*) getLayerComp( row, COLUMN_COLOR_LYR_CB );
        wxASSERT( cb );
        return cb->GetValue();
    }

    return false;
}


void LAYER_WIDGET::SetLayerColor( int aLayer, const COLOR4D& aColor )
{
    int row = findLayerRow( aLayer );

    if( row >= 0 )
    {
        int col = 1;    // bitmap button is column 1
        auto swatch = static_cast<COLOR_SWATCH*>( getLayerComp( row, col ) );
        wxASSERT( swatch );

        swatch->SetSwatchColor( aColor, false );
    }
}


COLOR4D LAYER_WIDGET::GetLayerColor( int aLayer ) const
{
    int row = findLayerRow( aLayer );

    if( row >= 0 )
    {
        const int col = 1;    // bitmap button is column 1
        auto swatch = static_cast<COLOR_SWATCH*>( getLayerComp( row, col ) );
        wxASSERT( swatch );

        return swatch->GetSwatchColor();
    }

    return COLOR4D::UNSPECIFIED;   // it's caller fault, gave me a bad layer
}


COLOR4D LAYER_WIDGET::GetRenderColor( int aRow ) const
{
    int row = aRow;

    if( row >= 0 )
    {
        const int col = 0;    // bitmap button (swatch) is column 0
        auto swatch = static_cast<COLOR_SWATCH*>( getRenderComp( row, col ) );
        wxASSERT( swatch );

        return swatch->GetSwatchColor();
    }

    return COLOR4D::UNSPECIFIED;   // it's caller fault, gave me a bad layer
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
    m_LayerPanel->Layout();
    m_RenderingPanel->Layout();
    FitInside();
}


void LAYER_WIDGET::UpdateLayerIcons()
{
    int rowCount = GetLayerRowCount();

    for( int row = 0; row < rowCount ; row++ )
    {
        INDICATOR_ICON* indicator = (INDICATOR_ICON*) getLayerComp( row, COLUMN_ICON_ACTIVE );

        if( indicator )
        {
            ROW_ICON_PROVIDER::STATE state;

            if( row == m_CurrentRow )
                state = ROW_ICON_PROVIDER::STATE::ON;
            else
                state = ROW_ICON_PROVIDER::STATE::OFF;

            indicator->SetIndicatorState( state );
        }
    }
}
