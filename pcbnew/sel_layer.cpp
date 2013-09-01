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
    BOARD * m_brd;

public:
    PCB_LAYER_SELECTOR( BOARD* aBrd ):LAYER_SELECTOR()
    {
        m_brd = aBrd;
    }

protected:
    // Returns true if the layer id is enabled (i.e. is it should be displayed)
    bool IsLayerEnabled( LAYER_NUM aLayer ) const
    {
        return m_brd->IsLayerEnabled( aLayer );
    }

    // Returns a color index from the layer id
    // Virtual function
    EDA_COLOR_T GetLayerColor( LAYER_NUM aLayer ) const
    {
        return m_brd->GetLayerColor( aLayer );
    }

    // Returns the name of the layer id
    // Virtual function
    wxString GetLayerName( LAYER_NUM aLayer ) const
    {
        return m_brd->GetLayerName( aLayer );
    }

};

/*
 * This class display a pcb layers list in adialog,
 * to select one layer from this list
 */
class PCB_ONE_LAYER_SELECTOR : public PCB_LAYER_SELECTOR,
                               public DIALOG_LAYER_SELECTION_BASE
{
    LAYER_NUM m_layerSelected;
    LAYER_NUM m_minLayer;
    LAYER_NUM m_maxLayer;

public:
    PCB_ONE_LAYER_SELECTOR( wxWindow* aParent, BOARD * aBrd,
                        LAYER_NUM aDefaultLayer,
                        LAYER_NUM aMinLayer = -1, LAYER_NUM aMaxLayer = -1,
                        bool aClearTool = false )
        :PCB_LAYER_SELECTOR( aBrd ), DIALOG_LAYER_SELECTION_BASE( aParent )
        {
            m_layerSelected = (int) aDefaultLayer;
            // When not needed, remove the "Clear" button
            m_buttonClear->Show( aClearTool );
            m_minLayer = aMinLayer;
            m_maxLayer = aMaxLayer;
            BuildList();
            Layout();
            GetSizer()->SetSizeHints(this);
            SetFocus();
        }

    LAYER_NUM GetLayerSelection() { return m_layerSelected; }

private:
    // Event handlers
    void OnLeftGridClick( wxGridEvent& event );
    void OnRightGridClick( wxGridEvent& event );
	void OnClearSelection( wxCommandEvent& event )
    {
        m_layerSelected = NB_PCB_LAYERS;
        EndModal( NB_PCB_LAYERS );
    }

    void BuildList();
};

// Build the layers list
// Column position by function:
#define SELECT_COLNUM 0
#define COLOR_COLNUM 1
#define LAYERNAME_COLNUM 2
#define LAYERID_COLNUM 3
static DECLARE_LAYERS_ORDER_LIST( layertranscode );

void PCB_ONE_LAYER_SELECTOR::BuildList()
{
    m_leftGridLayers->SetColFormatNumber( LAYERID_COLNUM );
    m_rightGridLayers->SetColFormatNumber( LAYERID_COLNUM );
    m_leftGridLayers->HideCol( LAYERID_COLNUM );
    m_rightGridLayers->HideCol( LAYERID_COLNUM );
    m_leftGridLayers->SetColSize( COLOR_COLNUM, 20 );
    m_rightGridLayers->SetColSize( COLOR_COLNUM, 20 );

    // Select a not show cell, to avoid a wrong cell selection for user
    m_leftGridLayers->GoToCell( 0, LAYERID_COLNUM );
    m_rightGridLayers->GoToCell( 0, LAYERID_COLNUM );

    int left_row = 0;
    int right_row = 0;
    wxString layernum;
    wxString   layername;
    for( LAYER_NUM i = FIRST_LAYER; i < NB_LAYERS; ++i )
    {
        LAYER_NUM  layerid = i;

        if( m_layerorder )
            layerid = layertranscode[i];

        if( ! IsLayerEnabled( layerid ) )
            continue;

        if( m_minLayer >= 0 && layerid < m_minLayer )
            continue;

        if( m_maxLayer >= 0 && layerid > m_maxLayer )
            continue;

        wxColour color = MakeColour( GetLayerColor( layerid ) );
        layername = GetLayerName( layerid );
        layernum.Printf( wxT("%d"), layerid );

        if( layerid <= LAST_COPPER_LAYER )
        {
            if( left_row )
                m_leftGridLayers->AppendRows( 1 );

            m_leftGridLayers->SetCellBackgroundColour ( left_row, COLOR_COLNUM,
                                                        color );
            m_leftGridLayers->SetCellValue( left_row, LAYERNAME_COLNUM,
                                            layername );
            m_leftGridLayers->SetCellValue( left_row, LAYERID_COLNUM,
                                            layernum );

            if( m_layerSelected == layerid )
            {
                m_leftGridLayers->SetCellValue( left_row, SELECT_COLNUM,
                                                wxT("X") );
                m_leftGridLayers->SetCellBackgroundColour ( left_row, SELECT_COLNUM,
                                                        color );
            }

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
            m_rightGridLayers->SetCellValue( right_row, LAYERID_COLNUM,
                                             layernum );

            if( m_layerSelected == layerid )
            {
                m_rightGridLayers->SetCellValue( right_row, SELECT_COLNUM,
                                                 wxT("X") );
                m_rightGridLayers->SetCellBackgroundColour ( right_row, SELECT_COLNUM,
                                                         color );
            }

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

void PCB_ONE_LAYER_SELECTOR::OnLeftGridClick( wxGridEvent& event )
{
    wxString text = m_leftGridLayers->GetCellValue(event.GetRow(), LAYERID_COLNUM);
    long layer;
    text.ToLong( &layer );
    m_layerSelected = layer;
    EndModal( 1 );
}

void PCB_ONE_LAYER_SELECTOR::OnRightGridClick( wxGridEvent& event )
{
    wxString text = m_rightGridLayers->GetCellValue(event.GetRow(), LAYERID_COLNUM);
    long layer;
    text.ToLong( &layer );
    m_layerSelected = layer;
    EndModal( 2 );
}

/** Install the dialog box for layer selection
 * @param aDefaultLayer = Preselection (NB_PCB_LAYERS for "(Deselect)" layer)
 * @param aMinlayer = min layer id value (-1 if no min value)
 * @param aMaxLayer = max layer id value (-1 if no max value)
 * @param aDeselectTool = display a "Clear" button when true
 * @return new layer value (NB_PCB_LAYERS when "(Deselect)" radiobutton selected),
 *                         or -1 if canceled
 *
 * Providing the option to also display a "Clear" button makes the
 * "Swap Layers" command more "user friendly",
 * by permitting any layer to be "deselected" immediately after its
 * corresponding radiobutton has been clicked on. (It would otherwise be
 * necessary to first cancel the "Select Layer:" dialog box (invoked after a
 * different radiobutton is clicked on) prior to then clicking on the
 * "Clear" button provided within the "Swap Layers:"
 * or "Layer selection:" dialog box).
 */
LAYER_NUM PCB_BASE_FRAME::SelectLayer( LAYER_NUM  aDefaultLayer,
                                       LAYER_NUM  aMinlayer,
                                       LAYER_NUM  aMaxLayer,
                                       bool aDeselectTool )
{
    PCB_ONE_LAYER_SELECTOR dlg( this, GetBoard(),
                                aDefaultLayer, aMinlayer, aMaxLayer,
                                aDeselectTool );
    dlg.ShowModal();
    LAYER_NUM layer = dlg.GetLayerSelection();
    return layer;
}


/*
 * This class display a double pcb copper layers list in a dialog,
 * to select a layer pair from this list
 */
class SELECT_COPPER_LAYERS_PAIR_DIALOG: public PCB_LAYER_SELECTOR,
                                        public DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE
{
private:
    BOARD* m_brd;
    LAYER_NUM m_frontLayer;
    LAYER_NUM m_backLayer;
    int m_leftRowSelected;
    int m_rightRowSelected;

public:
    SELECT_COPPER_LAYERS_PAIR_DIALOG( wxWindow* aParent, BOARD * aPcb,
                                      LAYER_NUM aFrontLayer, LAYER_NUM aBackLayer );

    void GetLayerPair( LAYER_NUM& aFrontLayer, LAYER_NUM& aBackLayer )
    {
        aFrontLayer = m_frontLayer;
        aBackLayer = m_backLayer;
    }

private:
    void OnLeftGridClick( wxGridEvent& event );
    void OnRightGridClick( wxGridEvent& event );

    void OnOkClick( wxCommandEvent& event )
    {
        EndModal( wxID_OK );
    }

    void OnCancelClick( wxCommandEvent& event )
    {
        EndModal( wxID_CANCEL );
    }

    void BuildList();

};

/* Display a list of two copper layers to choose a pair of layers
 * for auto-routing, vias ...
 */
void PCB_BASE_FRAME::SelectLayerPair()
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

SELECT_COPPER_LAYERS_PAIR_DIALOG::
    SELECT_COPPER_LAYERS_PAIR_DIALOG( wxWindow* aParent, BOARD * aPcb,
                                      LAYER_NUM aFrontLayer, LAYER_NUM aBackLayer) :
    PCB_LAYER_SELECTOR( aPcb ),
    DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE( aParent )
{
    m_frontLayer = aFrontLayer;
    m_backLayer = aBackLayer;
    m_leftRowSelected = 0;
    m_rightRowSelected = 0;
    BuildList();
    SetFocus();
    GetSizer()->SetSizeHints( this );
    Center();
}

void SELECT_COPPER_LAYERS_PAIR_DIALOG::BuildList()
{
    m_leftGridLayers->SetColFormatNumber( LAYERID_COLNUM );
    m_rightGridLayers->SetColFormatNumber( LAYERID_COLNUM );
    m_leftGridLayers->HideCol( LAYERID_COLNUM );
    m_rightGridLayers->HideCol( LAYERID_COLNUM );
    m_leftGridLayers->SetColSize( COLOR_COLNUM, 20 );
    m_rightGridLayers->SetColSize( COLOR_COLNUM, 20 );

    // Select a not show cell, to avoid a wrong cell selection for user
    m_leftGridLayers->GoToCell( 0, LAYERID_COLNUM );
    m_rightGridLayers->GoToCell( 0, LAYERID_COLNUM );

    int row = 0;
    wxString layernum;
    wxString   layername;
    for( LAYER_NUM i = FIRST_LAYER; i < NB_LAYERS; ++i )
    {
        LAYER_NUM  layerid = i;

        if( m_layerorder )
            layerid = layertranscode[i];

        if( ! IsLayerEnabled( layerid ) )
            continue;

        if( layerid > LAST_COPPER_LAYER )
            continue;

        wxColour color = MakeColour( GetLayerColor( layerid ) );
        layername = GetLayerName( layerid );
        layernum.Printf( wxT("%d"), layerid );

        if( row )
            m_leftGridLayers->AppendRows( 1 );
        m_leftGridLayers->SetCellBackgroundColour ( row, COLOR_COLNUM,
                                                    color );
        m_leftGridLayers->SetCellValue( row, LAYERNAME_COLNUM,
                                        layername );
        m_leftGridLayers->SetCellValue( row, LAYERID_COLNUM,
                                        layernum );

        if( m_frontLayer == layerid )
        {
            m_leftGridLayers->SetCellValue( row, SELECT_COLNUM,
                                            wxT("X") );
            m_leftGridLayers->SetCellBackgroundColour( row, SELECT_COLNUM,
                                                       color );
            m_leftRowSelected = row;
        }

        if( row )
            m_rightGridLayers->AppendRows( 1 );
        m_rightGridLayers->SetCellBackgroundColour ( row, COLOR_COLNUM,
                                                     color );
        m_rightGridLayers->SetCellValue( row, LAYERNAME_COLNUM,
                                         layername );
        m_rightGridLayers->SetCellValue( row, LAYERID_COLNUM,
                                         layernum );

        if( m_backLayer == layerid )
        {
            m_rightGridLayers->SetCellValue( row, SELECT_COLNUM,
                                             wxT("X") );
            m_rightGridLayers->SetCellBackgroundColour ( row, SELECT_COLNUM,
                                                     color );
            m_rightRowSelected = row;
        }

        row++;
    }

    m_leftGridLayers->AutoSizeColumn(LAYERNAME_COLNUM);
    m_rightGridLayers->AutoSizeColumn(LAYERNAME_COLNUM);
    m_leftGridLayers->AutoSizeColumn(SELECT_COLNUM);
    m_rightGridLayers->AutoSizeColumn(SELECT_COLNUM);
}

void SELECT_COPPER_LAYERS_PAIR_DIALOG::OnLeftGridClick( wxGridEvent& event )
{
    int row = event.GetRow();
    wxString text = m_leftGridLayers->GetCellValue( row, LAYERID_COLNUM );
    long layer;
    text.ToLong( &layer );

    if( m_frontLayer == layer )
        return;

    m_leftGridLayers->SetCellValue( m_leftRowSelected, SELECT_COLNUM,
                                     wxEmptyString );
    m_leftGridLayers->SetCellBackgroundColour ( m_leftRowSelected, SELECT_COLNUM,
                    m_leftGridLayers->GetDefaultCellBackgroundColour() );

    m_frontLayer = layer;
    m_leftRowSelected = row;
    m_leftGridLayers->SetCellValue( row, SELECT_COLNUM,
                                     wxT("X") );
    m_leftGridLayers->SetCellBackgroundColour( row, SELECT_COLNUM,
                        MakeColour( GetLayerColor( layer ) ) );

}

void SELECT_COPPER_LAYERS_PAIR_DIALOG::OnRightGridClick( wxGridEvent& event )
{
    int row = event.GetRow();
    wxString text = m_rightGridLayers->GetCellValue( row, LAYERID_COLNUM );
    long layer;
    text.ToLong( &layer );

    if(  m_backLayer == layer )
        return;

    m_rightGridLayers->SetCellValue( m_rightRowSelected, SELECT_COLNUM,
                                     wxEmptyString );
    m_rightGridLayers->SetCellBackgroundColour ( m_rightRowSelected, SELECT_COLNUM,
                m_rightGridLayers->GetDefaultCellBackgroundColour() );

    m_backLayer = layer;
    m_rightRowSelected = row;
    m_rightGridLayers->SetCellValue( row, SELECT_COLNUM,
                                     wxT("X") );
    m_rightGridLayers->SetCellBackgroundColour ( row, SELECT_COLNUM,
                                MakeColour( GetLayerColor( layer ) ) );
}
