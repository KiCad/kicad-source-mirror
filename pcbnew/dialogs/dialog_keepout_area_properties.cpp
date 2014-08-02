/**
 * @file dialog_keepout_area_properties.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
  * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/wx.h>
#include <fctsys.h>
//#include <pgm_base.h>
#include <kiface_i.h>
#include <confirm.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <zones.h>
#include <base_units.h>

#include <class_zone_settings.h>
#include <class_board.h>
#include <dialog_keepout_area_properties_base.h>

#include <wx/imaglist.h>    // needed for wx/listctrl.h, in wxGTK 2.8.12
#include <wx/listctrl.h>



/**
 * Class DIALOG_KEEPOUT_AREA_PROPERTIES
 * is the derived class from dialog_copper_zone_frame created by wxFormBuilder
 */
class DIALOG_KEEPOUT_AREA_PROPERTIES : public DIALOG_KEEPOUT_AREA_PROPERTIES_BASE
{
public:
    DIALOG_KEEPOUT_AREA_PROPERTIES( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aSettings );

private:
    PCB_BASE_FRAME* m_parent;
    wxConfigBase*       m_config;               ///< Current config
    ZONE_SETTINGS   m_zonesettings;
    ZONE_SETTINGS*  m_ptr;

    std::vector<LAYER_NUM> m_layerId;       ///< Handle the real layer number from layer
                                            ///< name position in m_LayerSelectionCtrl

    /**
     * Function initDialog
     * fills in the dialog controls using the current settings.
     */
    void initDialog();

    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    /**
     * Function AcceptOptionsForKeepOut
     * Test validity of options, and copy options in m_zonesettings, for keepout zones
     * @return bool - false if incorrect options, true if ok.
     */
    bool AcceptOptionsForKeepOut();

    /**
     * Function makeLayerBitmap
     * creates the colored rectangle bitmaps used in the layer selection widget.
     * @param aColor is the color to fill the rectangle with.
     */
    wxBitmap makeLayerBitmap( EDA_COLOR_T aColor );
};


#define LAYER_BITMAP_SIZE_X     20
#define LAYER_BITMAP_SIZE_Y     10

ZONE_EDIT_T InvokeKeepoutAreaEditor( PCB_BASE_FRAME* aCaller, ZONE_SETTINGS* aSettings )
{
    DIALOG_KEEPOUT_AREA_PROPERTIES dlg( aCaller, aSettings );

    ZONE_EDIT_T result = ZONE_EDIT_T( dlg.ShowModal() );

    return result;
}


DIALOG_KEEPOUT_AREA_PROPERTIES::DIALOG_KEEPOUT_AREA_PROPERTIES( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aSettings ) :
    DIALOG_KEEPOUT_AREA_PROPERTIES_BASE( aParent )
{
    m_parent = aParent;
    m_config = Kiface().KifaceSettings();

    m_ptr = aSettings;
    m_zonesettings = *aSettings;

    SetReturnCode( ZONE_ABORT );        // Will be changed on button OK ckick

    initDialog();

    GetSizer()->SetSizeHints( this );
    Center();
}


void DIALOG_KEEPOUT_AREA_PROPERTIES::initDialog()
{
    BOARD* board = m_parent->GetBoard();

    wxString msg;

    if( m_zonesettings.m_Zone_45_Only )
        m_OrientEdgesOpt->SetSelection( 1 );

    switch( m_zonesettings.m_Zone_HatchingStyle )
    {
    case CPolyLine::NO_HATCH:
        m_OutlineAppearanceCtrl->SetSelection( 0 );
        break;

    case CPolyLine::DIAGONAL_EDGE:
        m_OutlineAppearanceCtrl->SetSelection( 1 );
        break;

    case CPolyLine::DIAGONAL_FULL:
        m_OutlineAppearanceCtrl->SetSelection( 2 );
        break;
    }

    // Create one column in m_LayerSelectionCtrl
    wxListItem column0;
    column0.SetId( 0 );
    m_LayerSelectionCtrl->InsertColumn( 0, column0 );

    wxImageList* imageList = new wxImageList( LAYER_BITMAP_SIZE_X, LAYER_BITMAP_SIZE_Y );
    m_LayerSelectionCtrl->AssignImageList( imageList, wxIMAGE_LIST_SMALL );

    // Build copper layer list and append to layer widget
    LSET show = LSET::AllCuMask( board->GetCopperLayerCount() );
    int imgIdx = 0;

    for( LSEQ cu_stack = show.UIOrder();  cu_stack;  ++cu_stack, imgIdx++ )
    {
        LAYER_ID layer = *cu_stack;

        m_layerId.push_back( layer );

        msg = board->GetLayerName( layer );

        EDA_COLOR_T layerColor = board->GetLayerColor( layer );

        imageList->Add( makeLayerBitmap( layerColor ) );

        int itemIndex = m_LayerSelectionCtrl->InsertItem(
                m_LayerSelectionCtrl->GetItemCount(), msg, imgIdx );

        if( m_zonesettings.m_CurrentZone_Layer == layer )
            m_LayerSelectionCtrl->Select( itemIndex );
    }

    m_LayerSelectionCtrl->SetColumnWidth( 0, wxLIST_AUTOSIZE);

    // Init keepout parameters:
    m_cbTracksCtrl->SetValue( m_zonesettings.GetDoNotAllowTracks() );
    m_cbViasCtrl->SetValue( m_zonesettings.GetDoNotAllowVias() );
    m_cbCopperPourCtrl->SetValue( m_zonesettings.GetDoNotAllowCopperPour() );
}

void DIALOG_KEEPOUT_AREA_PROPERTIES::OnCancelClick( wxCommandEvent& event )
{
    EndModal( ZONE_ABORT );
}

void DIALOG_KEEPOUT_AREA_PROPERTIES::OnOkClick( wxCommandEvent& event )
{
    if( AcceptOptionsForKeepOut() )
    {
        *m_ptr = m_zonesettings;
        EndModal( ZONE_OK );
    }
}

bool DIALOG_KEEPOUT_AREA_PROPERTIES::AcceptOptionsForKeepOut()
{
    // Init keepout parameters:
    m_zonesettings.SetIsKeepout( true );
    m_zonesettings.SetDoNotAllowTracks( m_cbTracksCtrl->GetValue() );
    m_zonesettings.SetDoNotAllowVias( m_cbViasCtrl->GetValue() );
    m_zonesettings.SetDoNotAllowCopperPour( m_cbCopperPourCtrl->GetValue() );

    // Test for not allowed items: should have at least one item not allowed:
    if( ! m_zonesettings.GetDoNotAllowTracks() &&
        ! m_zonesettings.GetDoNotAllowVias() &&
        ! m_zonesettings.GetDoNotAllowCopperPour() )
        {
            DisplayError( NULL,
                          _("Tracks, vias and pads are allowed. The keepout is useless" ) );
            return false;
        }

    // Get the layer selection for this zone
    int ii = m_LayerSelectionCtrl->GetFirstSelected();

    if( ii < 0 )
    {
        DisplayError( NULL, _( "No layer selected." ) );
        return false;
    }

    m_zonesettings.m_CurrentZone_Layer = ToLAYER_ID( m_layerId[ii] );
    switch( m_OutlineAppearanceCtrl->GetSelection() )
    {
    case 0:
        m_zonesettings.m_Zone_HatchingStyle = CPolyLine::NO_HATCH;
        break;

    case 1:
        m_zonesettings.m_Zone_HatchingStyle = CPolyLine::DIAGONAL_EDGE;
        break;

    case 2:
        m_zonesettings.m_Zone_HatchingStyle = CPolyLine::DIAGONAL_FULL;
        break;
    }

    if( m_config )
    {
        m_config->Write( ZONE_NET_OUTLINES_HATCH_OPTION_KEY,
                         (long) m_zonesettings.m_Zone_HatchingStyle );
    }

    if( m_OrientEdgesOpt->GetSelection() == 0 )
        m_zonesettings.m_Zone_45_Only = false;
    else
        m_zonesettings.m_Zone_45_Only = true;

    m_zonesettings.m_ZonePriority = 0; //m_PriorityLevelCtrl->GetValue();

    return true;
}

wxBitmap DIALOG_KEEPOUT_AREA_PROPERTIES::makeLayerBitmap( EDA_COLOR_T aColor )
{
    wxBitmap    bitmap( LAYER_BITMAP_SIZE_X, LAYER_BITMAP_SIZE_Y );
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
    iconDC.DrawRectangle( 0, 0, LAYER_BITMAP_SIZE_X, LAYER_BITMAP_SIZE_Y );

    return bitmap;
}
