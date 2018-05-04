/**
 * @file dialog_keepout_area_properties.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/display.h>
#include <fctsys.h>
#include <kiface_i.h>
#include <confirm.h>
#include <pcbnew.h>
#include <pcb_edit_frame.h>
#include <class_zone.h>
#include <zones.h>
#include <base_units.h>
#include <widgets/color_swatch.h>

#include <zone_settings.h>
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
    wxConfigBase*   m_config;               ///< Current config
    ZONE_SETTINGS   m_zonesettings;         ///< the working copy of zone settings
    ZONE_SETTINGS*  m_ptr;                  ///< the pointer to the zone settings
                                            ///< of the zone to edit

    /**
     * Function initDialog
     * fills in the dialog controls using the current settings.
     */
    void initDialog();

    /**
     * automatically called by wxWidgets before closing the dialog
     */
    bool TransferDataFromWindow() override;

    void OnLayerSelection( wxDataViewEvent& event ) override;

    void OnSizeLayersList( wxSizeEvent& event ) override;

    /**
     * Function AcceptOptionsForKeepOut
     * Test validity of options, and copy options in m_zonesettings, for keepout zones
     * @return bool - false if incorrect options, true if ok.
     */
    bool AcceptOptionsForKeepOut();
};


#ifdef __WXMAC__
const static wxSize LAYER_BITMAP_SIZE( 28, 28 );  // Things get wonky if this isn't square...
#else
const static wxSize LAYER_BITMAP_SIZE( 20, 14 );
#endif


ZONE_EDIT_T InvokeKeepoutAreaEditor( PCB_BASE_FRAME* aCaller, ZONE_SETTINGS* aSettings )
{
    DIALOG_KEEPOUT_AREA_PROPERTIES dlg( aCaller, aSettings );

    ZONE_EDIT_T result = ZONE_ABORT;

    if( dlg.ShowModal() == wxID_OK )
        result = ZONE_OK;

    return result;
}


DIALOG_KEEPOUT_AREA_PROPERTIES::DIALOG_KEEPOUT_AREA_PROPERTIES( PCB_BASE_FRAME* aParent,
                                                                ZONE_SETTINGS* aSettings ) :
    DIALOG_KEEPOUT_AREA_PROPERTIES_BASE( aParent )
{
    m_parent = aParent;
    m_config = Kiface().KifaceSettings();

    m_ptr = aSettings;
    m_zonesettings = *aSettings;

    initDialog();
    m_sdbSizerButtonsOK->SetDefault();

    FinishDialogSettings();
}


void DIALOG_KEEPOUT_AREA_PROPERTIES::initDialog()
{
    BOARD* board = m_parent->GetBoard();
    COLOR4D backgroundColor = m_parent->Settings().Colors().GetLayerColor( LAYER_PCB_BACKGROUND );
    wxString msg;

    if( m_zonesettings.m_Zone_45_Only )
        m_OrientEdgesOpt->SetSelection( 1 );

    switch( m_zonesettings.m_Zone_HatchingStyle )
    {
    case ZONE_CONTAINER::NO_HATCH:
        m_OutlineAppearanceCtrl->SetSelection( 0 );
        break;

    case ZONE_CONTAINER::DIAGONAL_EDGE:
        m_OutlineAppearanceCtrl->SetSelection( 1 );
        break;

    case ZONE_CONTAINER::DIAGONAL_FULL:
        m_OutlineAppearanceCtrl->SetSelection( 2 );
        break;
    }

    // Build copper layer list and append to layer widget
    LSET show = LSET::AllCuMask( board->GetCopperLayerCount() );

    auto* checkColumn = m_layers->AppendToggleColumn( wxEmptyString );
    auto* layerColumn = m_layers->AppendIconTextColumn( wxEmptyString );

    wxVector<wxVariant> row;
    int minNamesWidth = 0;

    for( LSEQ cu_stack = show.UIOrder();  cu_stack;  ++cu_stack )
    {
        PCB_LAYER_ID layer = *cu_stack;

        msg = board->GetLayerName( layer );
        wxSize tsize( GetTextSize( msg, m_layers ) );
        minNamesWidth = std::max( minNamesWidth, tsize.x );

        COLOR4D layerColor = m_parent->Settings().Colors().GetLayerColor( layer );
        wxBitmap bitmap = COLOR_SWATCH::MakeBitmap( layerColor, backgroundColor, LAYER_BITMAP_SIZE );
        wxIcon icon;
        icon.CopyFromBitmap( bitmap );

        row.clear();
        row.push_back( m_zonesettings.m_Layers.test( layer ) );
        row.push_back( wxVariant( wxDataViewIconText( msg, icon ) ) );
        m_layers->AppendItem( row );
    }

    // Init keepout parameters:
    m_cbTracksCtrl->SetValue( m_zonesettings.GetDoNotAllowTracks() );
    m_cbViasCtrl->SetValue( m_zonesettings.GetDoNotAllowVias() );
    m_cbCopperPourCtrl->SetValue( m_zonesettings.GetDoNotAllowCopperPour() );

    checkColumn->SetWidth( 25 );    // if only wxCOL_WIDTH_AUTOSIZE worked on all platforms...
    layerColumn->SetMinWidth( minNamesWidth + LAYER_BITMAP_SIZE.x + 25 );

    // You'd think the fact that m_layers is a list would encourage wxWidgets not to save room
    // for the tree expanders... but you'd be wrong.  Force indent to 0.
    m_layers->SetIndent( 0 );
    m_layers->SetMinSize( wxSize( checkColumn->GetWidth() + layerColumn->GetWidth(), -1 ) );

    m_layers->Update();

    Update();

    m_sdbSizerButtonsOK->Enable( m_zonesettings.m_Layers.count() > 0 );
}


bool DIALOG_KEEPOUT_AREA_PROPERTIES::TransferDataFromWindow()
{
    if( AcceptOptionsForKeepOut() )
    {
        *m_ptr = m_zonesettings;
        return true;
    }

    return false;
}


void DIALOG_KEEPOUT_AREA_PROPERTIES::OnLayerSelection( wxDataViewEvent& event )
{
    if( event.GetColumn() != 0 )
    {
        return;
    }

    wxDataViewItem item = event.GetItem();

    int row = m_layers->ItemToRow( item );
    bool selected = m_layers->GetToggleValue( row, 0 );

    BOARD* board = m_parent->GetBoard();
    LSEQ cu_stack = LSET::AllCuMask( board->GetCopperLayerCount() ).UIOrder();

    if( row >= 0 && row < (int)cu_stack.size() )
    {
        m_zonesettings.m_Layers.set( cu_stack[ row ], selected );
    }

    m_sdbSizerButtonsOK->Enable( m_zonesettings.m_Layers.count() > 0 );
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
                      _("Tracks, vias, and pads are allowed. The keepout is useless" ) );
        return false;
    }

    if( m_zonesettings.m_Layers.count() == 0 )
    {
        DisplayError( NULL, _( "No layers selected." ) );
        return false;
    }

    switch( m_OutlineAppearanceCtrl->GetSelection() )
    {
    case 0:
        m_zonesettings.m_Zone_HatchingStyle = ZONE_CONTAINER::NO_HATCH;
        break;

    case 1:
        m_zonesettings.m_Zone_HatchingStyle = ZONE_CONTAINER::DIAGONAL_EDGE;
        break;

    case 2:
        m_zonesettings.m_Zone_HatchingStyle = ZONE_CONTAINER::DIAGONAL_FULL;
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

    m_zonesettings.m_ZonePriority = 0;  // for a keepout, this param is not used.
                                        // set it to 0
    return true;
}


void DIALOG_KEEPOUT_AREA_PROPERTIES::OnSizeLayersList( wxSizeEvent& event )
{
    int nameColWidth = event.GetSize().GetX() - m_layers->GetColumn( 0 )->GetWidth() - 8;

    m_layers->GetColumn( 1 )->SetWidth( nameColWidth );

    event.Skip();
}


