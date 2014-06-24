/**
 * @file sel_layer.cpp
 * @brief dialogs for one layer selection and a layer pair selection.
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <fctsys.h>
#include <common.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <wxBasePcbFrame.h>
#include <pcbcommon.h>
#include <class_layer_box_selector.h>
#include <class_board.h>
#include <dialogs/dialog_layer_selection_base.h>


/* classes to display a layer list using a wxGrid.
 */
class PCB_LAYER_SELECTOR: public LAYER_SELECTOR
{
public:
    PCB_LAYER_SELECTOR( BOARD* aBrd ) :
        LAYER_SELECTOR()
    {
        m_brd = aBrd;
    }

protected:
    BOARD*  m_brd;

    // Returns true if the layer id is enabled (i.e. is it should be displayed)
    bool IsLayerEnabled( LAYER_NUM aLayer ) const
    {
        return m_brd->IsLayerEnabled( LAYER_ID( aLayer ) );
    }

    // Returns a color index from the layer id
    // Virtual function
    EDA_COLOR_T GetLayerColor( LAYER_NUM aLayer ) const
    {
        return m_brd->GetLayerColor( LAYER_ID( aLayer ) );
    }

    // Returns the name of the layer id
    wxString GetLayerName( LAYER_NUM aLayer ) const      // overrides LAYER_SELECTOR
    {
        return m_brd->GetLayerName( LAYER_ID( aLayer ) );
    }
};


/*
 * This class display a pcb layers list in a dialog,
 * to select one layer from this list
 */
class PCB_ONE_LAYER_SELECTOR : public PCB_LAYER_SELECTOR,
                               public DIALOG_LAYER_SELECTION_BASE
{
    LAYER_ID    m_layerSelected;
    LSET        m_notAllowedLayersMask;

    std::vector<LAYER_ID> m_layersIdLeftColumn;
    std::vector<LAYER_ID> m_layersIdRightColumn;

public:
    PCB_ONE_LAYER_SELECTOR( wxWindow* aParent, BOARD * aBrd,
                        LAYER_ID aDefaultLayer,
                        LSET aNotAllowedLayersMask );

    LAYER_NUM GetLayerSelection()   { return m_layerSelected; }

private:
    // Event handlers
    void OnLeftGridCellClick( wxGridEvent& event );
    void OnRightGridCellClick( wxGridEvent& event );

    void buildList();
};


PCB_ONE_LAYER_SELECTOR::PCB_ONE_LAYER_SELECTOR( wxWindow* aParent,
        BOARD* aBrd, LAYER_ID aDefaultLayer, LSET aNotAllowedLayersMask )
    : PCB_LAYER_SELECTOR( aBrd ), DIALOG_LAYER_SELECTION_BASE( aParent )
{
    m_layerSelected = aDefaultLayer;
    m_notAllowedLayersMask = aNotAllowedLayersMask;
    buildList();
    Layout();
    GetSizer()->SetSizeHints( this );
    SetFocus();
}


// Build the layers list
// Column position by function:
#define SELECT_COLNUM       0
#define COLOR_COLNUM        1
#define LAYERNAME_COLNUM    2

void PCB_ONE_LAYER_SELECTOR::buildList()
{
    // Hide layerid column which is used only to know the layer id
    // not to be shown in dialogs
    m_leftGridLayers->SetColSize( COLOR_COLNUM, 20 );
    m_rightGridLayers->SetColSize( COLOR_COLNUM, 20 );

    int         left_row = 0;
    int         right_row = 0;
    wxString    layername;

    for( LSEQ ui_seq = m_brd->GetEnabledLayers().UIOrder();  ui_seq;  ++ui_seq )
    {
        LAYER_ID  layerid = *ui_seq;

        if( m_notAllowedLayersMask[layerid] )
            continue;

        wxColour color = MakeColour( GetLayerColor( layerid ) );
        layername = GetLayerName( layerid );

        if( IsCopperLayer( layerid ) )
        {
            if( left_row )
                m_leftGridLayers->AppendRows( 1 );

            m_leftGridLayers->SetCellBackgroundColour ( left_row, COLOR_COLNUM,
                                                        color );
            m_leftGridLayers->SetCellValue( left_row, LAYERNAME_COLNUM,
                                            layername );

            if( m_layerSelected == layerid )
            {
                m_leftGridLayers->SetCellValue( left_row, SELECT_COLNUM,
                                                wxT("X") );
                m_leftGridLayers->SetCellBackgroundColour ( left_row, SELECT_COLNUM,
                                                        color );
                m_leftGridLayers->SetGridCursor( left_row, LAYERNAME_COLNUM );
            }

            m_layersIdLeftColumn.push_back( layerid );
            left_row++;
        }
        else
        {
            if( right_row )
                m_rightGridLayers->AppendRows( 1 );

            m_rightGridLayers->SetCellBackgroundColour ( right_row, COLOR_COLNUM,
                                                         color );
            m_rightGridLayers->SetCellValue( right_row, LAYERNAME_COLNUM,
                                             layername );

            if( m_layerSelected == layerid )
            {
                m_rightGridLayers->SetCellValue( right_row, SELECT_COLNUM,
                                                 wxT("X") );
                m_rightGridLayers->SetCellBackgroundColour ( right_row, SELECT_COLNUM,
                                                         color );
                m_rightGridLayers->SetGridCursor( right_row, LAYERNAME_COLNUM );
            }

            m_layersIdRightColumn.push_back( layerid );
            right_row++;
        }
    }

    // Show only populated lists:
    if( left_row <= 0 )
        m_leftGridLayers->Show( false );

    if( right_row <= 0 )
        m_rightGridLayers->Show( false );

    m_leftGridLayers->AutoSizeColumn(LAYERNAME_COLNUM);
    m_rightGridLayers->AutoSizeColumn(LAYERNAME_COLNUM);
    m_leftGridLayers->AutoSizeColumn(SELECT_COLNUM);
    m_rightGridLayers->AutoSizeColumn(SELECT_COLNUM);
}


void PCB_ONE_LAYER_SELECTOR::OnLeftGridCellClick( wxGridEvent& event )
{
    m_layerSelected = m_layersIdLeftColumn[ event.GetRow() ];
    m_leftGridLayers->SetGridCursor( event.GetRow(), LAYERNAME_COLNUM );
    EndModal( 1 );
}


void PCB_ONE_LAYER_SELECTOR::OnRightGridCellClick( wxGridEvent& event )
{
    m_layerSelected = m_layersIdRightColumn[ event.GetRow() ];
    m_rightGridLayers->SetGridCursor( event.GetRow(), LAYERNAME_COLNUM );
    EndModal( 2 );
}


LAYER_ID PCB_BASE_FRAME::SelectLayer( LAYER_ID aDefaultLayer,
        LSET aNotAllowedLayersMask, wxPoint aDlgPosition )
{
    PCB_ONE_LAYER_SELECTOR dlg( this, GetBoard(), aDefaultLayer, aNotAllowedLayersMask );

    if( aDlgPosition != wxDefaultPosition )
    {
        wxSize dlgSize = dlg.GetSize();
        aDlgPosition.x -= dlgSize.x/2;
        aDlgPosition.y -= dlgSize.y/2;
        dlg.SetPosition( aDlgPosition );
    }

    dlg.ShowModal();

    LAYER_ID layer = (LAYER_ID) dlg.GetLayerSelection();
    return layer;
}


/**
 * Class SELECT_COPPER_LAYERS_PAIR_DIALOG
 * displays a double pcb copper layers list in a dialog,
 * to select a layer pair from these lists
 */
class SELECT_COPPER_LAYERS_PAIR_DIALOG: public PCB_LAYER_SELECTOR,
                                        public DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE
{
private:
    LAYER_ID    m_frontLayer;
    LAYER_ID    m_backLayer;
    int         m_leftRowSelected;
    int         m_rightRowSelected;

    std::vector<LAYER_ID> m_layersId;

public:
    SELECT_COPPER_LAYERS_PAIR_DIALOG( wxWindow* aParent, BOARD* aPcb,
                                      LAYER_ID aFrontLayer, LAYER_ID aBackLayer );

    void GetLayerPair( LAYER_ID& aFrontLayer, LAYER_ID& aBackLayer )
    {
        aFrontLayer = m_frontLayer;
        aBackLayer = m_backLayer;
    }

private:
    void OnLeftGridCellClick( wxGridEvent& event );
    void OnRightGridCellClick( wxGridEvent& event );

    void OnOkClick( wxCommandEvent& event )
    {
        EndModal( wxID_OK );
    }

    void OnCancelClick( wxCommandEvent& event )
    {
        EndModal( wxID_CANCEL );
    }

    void buildList();
    void SetGridCursor( wxGrid* aGrid, int aRow, bool aEnable );
};


void PCB_BASE_FRAME::SelectCopperLayerPair()
{
    PCB_SCREEN* screen = GetScreen();

    SELECT_COPPER_LAYERS_PAIR_DIALOG dlg( this, GetBoard(),
                                         screen->m_Route_Layer_TOP,
                                         screen->m_Route_Layer_BOTTOM );

    if( dlg.ShowModal() == wxID_OK )
    {
        dlg.GetLayerPair( screen->m_Route_Layer_TOP, screen->m_Route_Layer_BOTTOM );

        // select the same layer for both layers is allowed (normal in some boards)
        // but could be a mistake. So display an info message
        if( screen->m_Route_Layer_TOP == screen->m_Route_Layer_BOTTOM )
            DisplayInfoMessage( this,
                                _( "Warning: The Top Layer and Bottom Layer are same." ) );
    }

    m_canvas->MoveCursorToCrossHair();
}


SELECT_COPPER_LAYERS_PAIR_DIALOG::SELECT_COPPER_LAYERS_PAIR_DIALOG(
        wxWindow* aParent, BOARD * aPcb, LAYER_ID aFrontLayer, LAYER_ID aBackLayer) :
    PCB_LAYER_SELECTOR( aPcb ),
    DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE( aParent )
{
    m_frontLayer = aFrontLayer;
    m_backLayer = aBackLayer;
    m_leftRowSelected = 0;
    m_rightRowSelected = 0;
    buildList();
    SetFocus();
    GetSizer()->SetSizeHints( this );
    Center();
}


void SELECT_COPPER_LAYERS_PAIR_DIALOG::buildList()
{
    m_leftGridLayers->SetColSize( COLOR_COLNUM, 20 );
    m_rightGridLayers->SetColSize( COLOR_COLNUM, 20 );

    // Select a not show cell, to avoid a wrong cell selection for user

    int         row = 0;
    wxString    layername;

    for( LSEQ ui_seq = m_brd->GetEnabledLayers().UIOrder();  ui_seq;  ++ui_seq )
    {
        LAYER_ID  layerid = *ui_seq;

        if( !IsCopperLayer( layerid ) )
            break;

        wxColour color = MakeColour( GetLayerColor( layerid ) );

        layername = GetLayerName( layerid );

        if( row )
            m_leftGridLayers->AppendRows( 1 );

        m_leftGridLayers->SetCellBackgroundColour( row, COLOR_COLNUM, color );
        m_leftGridLayers->SetCellValue( row, LAYERNAME_COLNUM, layername );
        m_layersId.push_back( layerid );

        if( m_frontLayer == layerid )
        {
            SetGridCursor( m_leftGridLayers, row, true );
            m_leftRowSelected = row;
        }

        if( row )
            m_rightGridLayers->AppendRows( 1 );
        m_rightGridLayers->SetCellBackgroundColour ( row, COLOR_COLNUM,
                                                     color );
        m_rightGridLayers->SetCellValue( row, LAYERNAME_COLNUM,
                                         layername );

        if( m_backLayer == layerid )
        {
            SetGridCursor( m_rightGridLayers, row, true );
            m_rightRowSelected = row;
        }

        row++;
    }

    m_leftGridLayers->AutoSizeColumn(LAYERNAME_COLNUM);
    m_rightGridLayers->AutoSizeColumn(LAYERNAME_COLNUM);
    m_leftGridLayers->AutoSizeColumn(SELECT_COLNUM);
    m_rightGridLayers->AutoSizeColumn(SELECT_COLNUM);
}


void SELECT_COPPER_LAYERS_PAIR_DIALOG::SetGridCursor( wxGrid* aGrid, int aRow,
                                                      bool aEnable )
{
    if( aEnable )
    {
        LAYER_ID  layerid = m_layersId[aRow];
        wxColour color = MakeColour( GetLayerColor( layerid ) );
        aGrid->SetCellValue( aRow, SELECT_COLNUM, wxT("X") );
        aGrid->SetCellBackgroundColour( aRow, SELECT_COLNUM, color );
        aGrid->SetGridCursor( aRow, LAYERNAME_COLNUM );
    }
    else
    {
        aGrid->SetCellValue( aRow, SELECT_COLNUM, wxEmptyString );
        aGrid->SetCellBackgroundColour( aRow, SELECT_COLNUM,
                                        aGrid->GetDefaultCellBackgroundColour() );
        aGrid->SetGridCursor( aRow, LAYERNAME_COLNUM );
    }
}


void SELECT_COPPER_LAYERS_PAIR_DIALOG::OnLeftGridCellClick( wxGridEvent& event )
{
    int         row = event.GetRow();
    LAYER_ID    layer = m_layersId[row];

    if( m_frontLayer == layer )
        return;

    SetGridCursor( m_leftGridLayers, m_leftRowSelected, false );
    m_frontLayer = layer;
    m_leftRowSelected = row;
    SetGridCursor( m_leftGridLayers, m_leftRowSelected, true );
}


void SELECT_COPPER_LAYERS_PAIR_DIALOG::OnRightGridCellClick( wxGridEvent& event )
{
    int         row = event.GetRow();
    LAYER_ID    layer = m_layersId[row];

    if( m_backLayer == layer )
        return;

    SetGridCursor( m_rightGridLayers, m_rightRowSelected, false );
    m_backLayer = layer;
    m_rightRowSelected = row;
    SetGridCursor( m_rightGridLayers, m_rightRowSelected, true );
}
