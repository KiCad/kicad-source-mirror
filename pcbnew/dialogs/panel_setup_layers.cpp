/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Isaac Marino Bavaresco, isaacbavaresco@yahoo.com.br
 * Copyright (C) 2009 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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


#include <confirm.h>
#include <core/arraydim.h>
#include <core/kicad_algo.h>
#include <pcb_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <board.h>
#include <collectors.h>
#include <footprint.h>
#include <layer_ids.h>
#include <pad.h>
#include <pcb_track.h>
#include <panel_setup_layers.h>
#include <board_stackup_manager/panel_board_stackup.h>
#include <dialogs/dialog_items_list.h>

#include <wx/choicdlg.h>
#include <wx/msgdlg.h>
#include <eda_list_dialog.h>

#include <list>
#include <set>


/**
 * Configure a layer checkbox to be mandatory and disabled.
 */
static void mandatoryLayerCbSetup( wxCheckBox& aCheckBox )
{
    aCheckBox.Show();
    aCheckBox.Disable();
    aCheckBox.SetValue( true );
    aCheckBox.SetToolTip( _( "This layer is required and cannot be disabled" ) );
}


PANEL_SETUP_LAYERS::PANEL_SETUP_LAYERS( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_LAYERS_BASE( aParentWindow ),
        m_frame( aFrame ),
        m_physicalStackup( nullptr ),
        m_initialized( false ),
        m_CrtYdFrontCheckBox( nullptr ),
        m_CrtYdFrontName( nullptr ),
        m_CrtYdFrontStaticText( nullptr ),
        m_FabFrontCheckBox( nullptr ),
        m_FabFrontName( nullptr ),
        m_FabFrontStaticText( nullptr ),
        m_AdhesFrontCheckBox( nullptr ),
        m_AdhesFrontName( nullptr ),
        m_AdhesFrontStaticText( nullptr ),
        m_SoldPFrontCheckBox( nullptr ),
        m_SoldPFrontName( nullptr ),
        m_SoldPFrontStaticText( nullptr ),
        m_SilkSFrontCheckBox( nullptr ),
        m_SilkSFrontName( nullptr ),
        m_SilkSFrontStaticText( nullptr ),
        m_MaskFrontCheckBox( nullptr ),
        m_MaskFrontName( nullptr ),
        m_MaskFrontStaticText( nullptr ),
        m_MaskBackCheckBox( nullptr ),
        m_MaskBackName( nullptr ),
        m_MaskBackStaticText( nullptr ),
        m_SilkSBackCheckBox( nullptr ),
        m_SilkSBackName( nullptr ),
        m_SilkSBackStaticText( nullptr ),
        m_SoldPBackCheckBox( nullptr ),
        m_SoldPBackName( nullptr ),
        m_SoldPBackStaticText( nullptr ),
        m_AdhesBackCheckBox( nullptr ),
        m_AdhesBackName( nullptr ),
        m_AdhesBackStaticText( nullptr ),
        m_FabBackCheckBox( nullptr ),
        m_FabBackName( nullptr ),
        m_FabBackStaticText( nullptr ),
        m_CrtYdBackCheckBox( nullptr ),
        m_CrtYdBackName( nullptr ),
        m_CrtYdBackStaticText( nullptr ),
        m_PCBEdgesCheckBox( nullptr ),
        m_PCBEdgesName( nullptr ),
        m_PCBEdgesStaticText( nullptr ),
        m_MarginCheckBox( nullptr ),
        m_MarginName( nullptr ),
        m_MarginStaticText( nullptr ),
        m_Eco1CheckBox( nullptr ),
        m_Eco1Name( nullptr ),
        m_Eco1StaticText( nullptr ),
        m_Eco2CheckBox( nullptr ),
        m_Eco2Name( nullptr ),
        m_Eco2StaticText( nullptr ),
        m_CommentsCheckBox( nullptr ),
        m_CommentsName( nullptr ),
        m_CommentsStaticText( nullptr ),
        m_DrawingsCheckBox( nullptr ),
        m_DrawingsName( nullptr ),
        m_DrawingsStaticText( nullptr )
{
    m_pcb = aFrame->GetBoard();
}


void PANEL_SETUP_LAYERS::initialize_front_tech_layers()
{
    m_CrtYdFrontCheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString,
                                           wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_CrtYdFrontCheckBox, 0,
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT
                                | wxRESERVE_SPACE_EVEN_IF_HIDDEN,
                        5 );

    m_CrtYdFrontName = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( F_CrtYd ),
                                       wxDefaultPosition, wxDefaultSize, 0 );
    m_CrtYdFrontName->SetMinSize( wxSize( 160, -1 ) );

    m_LayersSizer->Add( m_CrtYdFrontName, 0, wxRIGHT | wxEXPAND, 5 );

    m_CrtYdFrontStaticText =
            new wxStaticText( m_LayersListPanel, wxID_ANY, _( "Off-board, testing" ),
                              wxDefaultPosition, wxDefaultSize, 0 );
    m_CrtYdFrontStaticText->Wrap( -1 );
    m_CrtYdFrontStaticText->SetMinSize( wxSize( 150, -1 ) );

    m_LayersSizer->Add( m_CrtYdFrontStaticText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 5 );

    m_FabFrontCheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString,
                                         wxDefaultPosition, wxDefaultSize, 0 );
    m_FabFrontCheckBox->SetToolTip(
            _( "If you want a fabrication layer for the front side of the board" ) );

    m_LayersSizer->Add( m_FabFrontCheckBox, 0,
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT, 5 );

    m_FabFrontName = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( F_Fab ),
                                     wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_FabFrontName, 0, wxEXPAND | wxRIGHT, 5 );

    m_FabFrontStaticText =
            new wxStaticText( m_LayersListPanel, wxID_ANY, _( "Off-board, manufacturing" ),
                              wxDefaultPosition, wxDefaultSize, 0 );
    m_FabFrontStaticText->Wrap( -1 );
    m_LayersSizer->Add( m_FabFrontStaticText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 5 );

    m_AdhesFrontCheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString,
                                           wxDefaultPosition, wxDefaultSize, 0 );
    m_AdhesFrontCheckBox->SetToolTip(
            _( "If you want an adhesive template for the front side of the board" ) );

    m_LayersSizer->Add( m_AdhesFrontCheckBox, 0,
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT, 5 );

    m_AdhesFrontName = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( F_Adhes ),
                                       wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_AdhesFrontName, 0, wxEXPAND | wxRIGHT, 5 );

    m_AdhesFrontStaticText =
            new wxStaticText( m_LayersListPanel, wxID_ANY, _( "On-board, non-copper" ),
                              wxDefaultPosition, wxDefaultSize, 0 );
    m_AdhesFrontStaticText->Wrap( -1 );
    m_LayersSizer->Add( m_AdhesFrontStaticText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 5 );

    m_SoldPFrontCheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString,
                                           wxDefaultPosition, wxDefaultSize, 0 );
    m_SoldPFrontCheckBox->SetToolTip(
            _( "If you want a solder paste layer for front side of the board" ) );

    m_LayersSizer->Add( m_SoldPFrontCheckBox, 0,
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT, 5 );

    m_SoldPFrontName = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( F_Paste ),
                                       wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_SoldPFrontName, 0, wxEXPAND | wxRIGHT, 5 );

    m_SoldPFrontStaticText =
            new wxStaticText( m_LayersListPanel, wxID_ANY, _( "On-board, non-copper" ),
                              wxDefaultPosition, wxDefaultSize, 0 );
    m_SoldPFrontStaticText->Wrap( -1 );
    m_LayersSizer->Add( m_SoldPFrontStaticText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 5 );

    m_SilkSFrontCheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString,
                                           wxDefaultPosition, wxDefaultSize, 0 );
    m_SilkSFrontCheckBox->SetToolTip(
            _( "If you want a silk screen layer for the front side of the board" ) );

    m_LayersSizer->Add( m_SilkSFrontCheckBox, 0,
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT, 5 );

    m_SilkSFrontName = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( F_SilkS ),
                                       wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_SilkSFrontName, 0, wxEXPAND | wxRIGHT, 5 );

    m_SilkSFrontStaticText =
            new wxStaticText( m_LayersListPanel, wxID_ANY, _( "On-board, non-copper" ),
                              wxDefaultPosition, wxDefaultSize, 0 );
    m_SilkSFrontStaticText->Wrap( -1 );
    m_LayersSizer->Add( m_SilkSFrontStaticText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 5 );

    m_MaskFrontCheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString,
                                          wxDefaultPosition, wxDefaultSize, 0 );
    m_MaskFrontCheckBox->SetToolTip(
            _( "If you want a solder mask layer for the front of the board" ) );

    m_LayersSizer->Add( m_MaskFrontCheckBox, 0,
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT, 5 );

    m_MaskFrontName = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( F_Mask ),
                                      wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_MaskFrontName, 0, wxEXPAND | wxRIGHT, 5 );

    m_MaskFrontStaticText =
            new wxStaticText( m_LayersListPanel, wxID_ANY, _( "On-board, non-copper" ),
                              wxDefaultPosition, wxDefaultSize, 0 );
    m_MaskFrontStaticText->Wrap( -1 );
    m_LayersSizer->Add( m_MaskFrontStaticText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 5 );

    mandatoryLayerCbSetup( *m_CrtYdFrontCheckBox );
}


void PANEL_SETUP_LAYERS::initialize_back_tech_layers()
{
    m_MaskBackCheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString,
                                         wxDefaultPosition, wxDefaultSize, 0 );
    m_MaskBackCheckBox->SetToolTip(
            _( "If you want a solder mask layer for the back side of the board" ) );

    m_LayersSizer->Add( m_MaskBackCheckBox, 0,
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT, 5 );

    m_MaskBackName = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( B_Mask ),
                                     wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_MaskBackName, 0, wxEXPAND | wxRIGHT, 5 );

    m_MaskBackStaticText =
            new wxStaticText( m_LayersListPanel, wxID_ANY, _( "On-board, non-copper" ),
                              wxDefaultPosition, wxDefaultSize, 0 );
    m_MaskBackStaticText->Wrap( -1 );
    m_LayersSizer->Add( m_MaskBackStaticText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 5 );

    m_SilkSBackCheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString,
                                          wxDefaultPosition, wxDefaultSize, 0 );
    m_SilkSBackCheckBox->SetToolTip(
            _( "If you want a silk screen layer for the back side of the board" ) );

    m_LayersSizer->Add( m_SilkSBackCheckBox, 0,
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT, 5 );

    m_SilkSBackName = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( B_SilkS ),
                                      wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_SilkSBackName, 0, wxEXPAND | wxRIGHT, 5 );

    m_SilkSBackStaticText =
            new wxStaticText( m_LayersListPanel, wxID_ANY, _( "On-board, non-copper" ),
                              wxDefaultPosition, wxDefaultSize, 0 );
    m_SilkSBackStaticText->Wrap( -1 );
    m_LayersSizer->Add( m_SilkSBackStaticText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 5 );

    m_SoldPBackCheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString,
                                          wxDefaultPosition, wxDefaultSize, 0 );
    m_SoldPBackCheckBox->SetToolTip(
            _( "If you want a solder paste layer for the back side of the board" ) );

    m_LayersSizer->Add( m_SoldPBackCheckBox, 0,
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT, 5 );

    m_SoldPBackName = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( B_Paste ),
                                      wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_SoldPBackName, 0, wxEXPAND | wxRIGHT, 5 );

    m_SoldPBackStaticText =
            new wxStaticText( m_LayersListPanel, wxID_ANY, _( "On-board, non-copper" ),
                              wxDefaultPosition, wxDefaultSize, 0 );
    m_SoldPBackStaticText->Wrap( -1 );
    m_LayersSizer->Add( m_SoldPBackStaticText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 5 );

    m_AdhesBackCheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString,
                                          wxDefaultPosition, wxDefaultSize, 0 );
    m_AdhesBackCheckBox->SetToolTip(
            _( "If you want an adhesive layer for the back side of the board" ) );

    m_LayersSizer->Add( m_AdhesBackCheckBox, 0,
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT, 5 );

    m_AdhesBackName = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( B_Adhes ),
                                      wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_AdhesBackName, 0, wxEXPAND | wxRIGHT, 5 );

    m_AdhesBackStaticText =
            new wxStaticText( m_LayersListPanel, wxID_ANY, _( "On-board, non-copper" ),
                              wxDefaultPosition, wxDefaultSize, 0 );
    m_AdhesBackStaticText->Wrap( -1 );
    m_LayersSizer->Add( m_AdhesBackStaticText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 5 );

    m_FabBackCheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString,
                                        wxDefaultPosition, wxDefaultSize, 0 );
    m_FabBackCheckBox->SetToolTip(
            _( "If you want a fabrication layer for the back side of the board" ) );

    m_LayersSizer->Add( m_FabBackCheckBox, 0,
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT, 5 );

    m_FabBackName = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( B_Fab ),
                                    wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_FabBackName, 0, wxEXPAND | wxRIGHT, 5 );

    m_FabBackStaticText =
            new wxStaticText( m_LayersListPanel, wxID_ANY, _( "Off-board, manufacturing" ),
                              wxDefaultPosition, wxDefaultSize, 0 );
    m_FabBackStaticText->Wrap( -1 );
    m_LayersSizer->Add( m_FabBackStaticText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 5 );


    m_CrtYdBackCheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString,
                                          wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_CrtYdBackCheckBox, 0,
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT
                                | wxRESERVE_SPACE_EVEN_IF_HIDDEN,
                        5 );

    m_CrtYdBackName = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( B_CrtYd ),
                                      wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_CrtYdBackName, 0, wxEXPAND | wxRIGHT, 5 );

    m_CrtYdBackStaticText =
            new wxStaticText( m_LayersListPanel, wxID_ANY, _( "Off-board, testing" ),
                              wxDefaultPosition, wxDefaultSize, 0 );
    m_CrtYdBackStaticText->Wrap( -1 );
    m_LayersSizer->Add( m_CrtYdBackStaticText, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5 );


    m_PCBEdgesCheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString,
                                          wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_PCBEdgesCheckBox, 0,
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT
                                | wxRESERVE_SPACE_EVEN_IF_HIDDEN,
                        5 );

    m_PCBEdgesName = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( Edge_Cuts ),
                                     wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_PCBEdgesName, 0, wxEXPAND | wxRIGHT, 5 );

    m_PCBEdgesStaticText =
            new wxStaticText( m_LayersListPanel, wxID_ANY, _( "Board contour" ),
                              wxDefaultPosition, wxDefaultSize, 0 );
    m_PCBEdgesStaticText->Wrap( -1 );
    m_LayersSizer->Add( m_PCBEdgesStaticText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 5 );

    m_MarginCheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString,
                                       wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_MarginCheckBox, 0,
                        wxLEFT | wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL
                                | wxRESERVE_SPACE_EVEN_IF_HIDDEN,
                        5 );

    m_MarginName = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( Margin ),
                                   wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_MarginName, 0, wxEXPAND | wxRIGHT, 5 );

    m_MarginStaticText =
            new wxStaticText( m_LayersListPanel, wxID_ANY, _( "Board contour setback" ),
                              wxDefaultPosition, wxDefaultSize, 0 );
    m_MarginStaticText->Wrap( -1 );
    m_LayersSizer->Add( m_MarginStaticText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 5 );

    m_Eco1CheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString,
                                     wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_Eco1CheckBox, 0,
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT, 5 );

    m_Eco1Name = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( Eco1_User ),
                                 wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_Eco1Name, 0, wxEXPAND | wxRIGHT, 5 );

    m_Eco1StaticText = new wxStaticText( m_LayersListPanel, wxID_ANY, _( "Auxiliary" ),
                                         wxDefaultPosition, wxDefaultSize, 0 );
    m_Eco1StaticText->Wrap( -1 );
    m_LayersSizer->Add( m_Eco1StaticText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 5 );

    m_Eco2CheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString,
                                     wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_Eco2CheckBox, 0,
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT, 5 );

    m_Eco2Name = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( Eco2_User ),
                                 wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_Eco2Name, 0, wxEXPAND | wxRIGHT, 5 );

    m_Eco2StaticText = new wxStaticText( m_LayersListPanel, wxID_ANY, _( "Auxiliary" ),
                                         wxDefaultPosition, wxDefaultSize, 0 );
    m_Eco2StaticText->Wrap( -1 );
    m_LayersSizer->Add( m_Eco2StaticText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 5 );

    m_CommentsCheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString,
                                         wxDefaultPosition, wxDefaultSize, 0 );
    m_CommentsCheckBox->SetToolTip( _( "If you want a separate layer for comments or notes" ) );

    m_LayersSizer->Add( m_CommentsCheckBox, 0,
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT, 5 );

    m_CommentsName = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( Cmts_User ),
                                     wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_CommentsName, 0, wxEXPAND | wxRIGHT, 5 );

    m_CommentsStaticText = new wxStaticText( m_LayersListPanel, wxID_ANY, _( "Auxiliary" ),
                                             wxDefaultPosition, wxDefaultSize, 0 );
    m_CommentsStaticText->Wrap( -1 );
    m_LayersSizer->Add( m_CommentsStaticText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 5 );

    m_DrawingsCheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString,
                                         wxDefaultPosition, wxDefaultSize, 0 );
    m_DrawingsCheckBox->SetToolTip( _( "If you want a layer for documentation drawings" ) );

    m_LayersSizer->Add( m_DrawingsCheckBox, 0,
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT, 5 );

    m_DrawingsName = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( Dwgs_User ),
                                     wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersSizer->Add( m_DrawingsName, 0, wxEXPAND | wxRIGHT, 5 );

    m_DrawingsStaticText = new wxStaticText( m_LayersListPanel, wxID_ANY, _( "Auxiliary" ),
                                             wxDefaultPosition, wxDefaultSize, 0 );
    m_DrawingsStaticText->Wrap( -1 );
    m_LayersSizer->Add( m_DrawingsStaticText, 0,
                        wxALIGN_CENTER_VERTICAL | wxBOTTOM | wxLEFT | wxRIGHT, 5 );

    mandatoryLayerCbSetup( *m_CrtYdBackCheckBox );
    mandatoryLayerCbSetup( *m_PCBEdgesCheckBox );
    mandatoryLayerCbSetup( *m_MarginCheckBox );
}


void PANEL_SETUP_LAYERS::initialize_layers_controls()
{
    Freeze();
    m_layersControls.clear();
    m_LayersSizer->Clear( true );
    initialize_front_tech_layers();
    m_layersControls[F_CrtYd] = PANEL_SETUP_LAYERS_CTLs( m_CrtYdFrontName, m_CrtYdFrontCheckBox, m_CrtYdFrontStaticText );
    m_layersControls[F_Fab] = PANEL_SETUP_LAYERS_CTLs( m_FabFrontName, m_FabFrontCheckBox, m_FabFrontStaticText );
    m_layersControls[F_Adhes] = PANEL_SETUP_LAYERS_CTLs( m_AdhesFrontName, m_AdhesFrontCheckBox, m_AdhesFrontStaticText );
    m_layersControls[F_Paste] = PANEL_SETUP_LAYERS_CTLs( m_SoldPFrontName, m_SoldPFrontCheckBox, m_SoldPFrontStaticText );
    m_layersControls[F_SilkS] = PANEL_SETUP_LAYERS_CTLs( m_SilkSFrontName, m_SilkSFrontCheckBox, m_SilkSFrontStaticText );
    m_layersControls[F_Mask] = PANEL_SETUP_LAYERS_CTLs( m_MaskFrontName, m_MaskFrontCheckBox, m_MaskFrontStaticText );


    LSET layers = m_enabledLayers;

    for( auto it = layers.copper_layers_begin(); it != layers.copper_layers_end(); ++it )
    {
        wxCheckBox* cb = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
        cb->SetToolTip( _( "Use the Physical Stackup page to change the number of copper layers." ) );
        cb->Disable();

        m_LayersSizer->Add( cb, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

        wxTextCtrl* txt = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( *it ), wxDefaultPosition, wxDefaultSize, 0 );
        txt->SetToolTip( _("Layer Name") );

        m_LayersSizer->Add( txt, 0, wxEXPAND|wxRIGHT, 5 );

        wxArrayString choices;
        choices.Add( _( "signal" ) );
        choices.Add( _( "power plane" ) );
        choices.Add( _( "mixed" ) );
        choices.Add( _( "jumper" ) );
        wxChoice* choice = new wxChoice( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices, 0 );
        choice->SetSelection( 0 );
        choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\n"
                              "Power plane layers are removed from Freerouter's layer menus.") );

        m_LayersSizer->Add( choice, 0, wxRIGHT|wxEXPAND, 5 );
        m_layersControls[*it] = PANEL_SETUP_LAYERS_CTLs( txt, cb, choice );
    }

    initialize_back_tech_layers();
    m_layersControls[B_Mask] = PANEL_SETUP_LAYERS_CTLs( m_MaskBackName, m_MaskBackCheckBox, m_MaskBackStaticText );
    m_layersControls[B_SilkS] = PANEL_SETUP_LAYERS_CTLs( m_SilkSBackName, m_SilkSBackCheckBox, m_SilkSBackStaticText );
    m_layersControls[B_Paste] = PANEL_SETUP_LAYERS_CTLs( m_SoldPBackName, m_SoldPBackCheckBox, m_SoldPBackStaticText );
    m_layersControls[B_Adhes] = PANEL_SETUP_LAYERS_CTLs( m_AdhesBackName, m_AdhesBackCheckBox, m_AdhesBackStaticText );
    m_layersControls[B_Fab] = PANEL_SETUP_LAYERS_CTLs( m_FabBackName, m_FabBackCheckBox, m_FabBackStaticText );
    m_layersControls[B_CrtYd] = PANEL_SETUP_LAYERS_CTLs( m_CrtYdBackName, m_CrtYdBackCheckBox, m_CrtYdBackStaticText );
    m_layersControls[Edge_Cuts] = PANEL_SETUP_LAYERS_CTLs( m_PCBEdgesName, m_PCBEdgesCheckBox, m_PCBEdgesStaticText );
    m_layersControls[Margin] = PANEL_SETUP_LAYERS_CTLs( m_MarginName, m_MarginCheckBox, m_MarginStaticText );
    m_layersControls[Eco1_User] = PANEL_SETUP_LAYERS_CTLs( m_Eco1Name, m_Eco1CheckBox, m_Eco1StaticText );
    m_layersControls[Eco2_User] = PANEL_SETUP_LAYERS_CTLs( m_Eco2Name, m_Eco2CheckBox, m_Eco2StaticText );
    m_layersControls[Cmts_User] = PANEL_SETUP_LAYERS_CTLs( m_CommentsName, m_CommentsCheckBox, m_CommentsStaticText );
    m_layersControls[Dwgs_User] = PANEL_SETUP_LAYERS_CTLs( m_DrawingsName, m_DrawingsCheckBox, m_DrawingsStaticText );

    layers &= LSET::UserDefinedLayersMask();

    for( auto it = layers.non_copper_layers_begin(); it != layers.non_copper_layers_end(); ++it )
    {
        append_user_layer( *it );
    }

    Thaw();
    m_LayersListPanel->FitInside(); // Updates virtual size to fit subwindows, also auto-layouts.
}


void PANEL_SETUP_LAYERS::append_user_layer( PCB_LAYER_ID aLayer )
{
    wxCheckBox* cb = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                     wxDefaultSize, 0 );
    m_LayersSizer->Add( cb, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxLEFT, 5 );

    wxTextCtrl* txt = new wxTextCtrl( m_LayersListPanel, wxID_ANY, LayerName( aLayer ),
                                      wxDefaultPosition, wxDefaultSize, 0 );
    txt->SetToolTip( _( "Layer Name" ) );
    m_LayersSizer->Add( txt, 0, wxEXPAND | wxRIGHT, 5 );

    wxArrayString choices;
    choices.Add( _( "Auxiliary" ) );
    choices.Add( _( "Off-board, front" ) );
    choices.Add( _( "Off-board, back" ) );

    wxChoice* choice = new wxChoice( m_LayersListPanel, wxID_ANY, wxDefaultPosition,
                                     wxDefaultSize, choices, 0 );
    choice->SetSelection( 0 );
    choice->SetToolTip(
            _( "Auxiliary layers do not flip with board side, while back and front layers do." ) );

    m_LayersSizer->Add( choice, 0, wxEXPAND | wxRIGHT, 5 );
    m_layersControls[aLayer] = PANEL_SETUP_LAYERS_CTLs( txt, cb, choice );
}


wxTextCtrl* PANEL_SETUP_LAYERS::getName( PCB_LAYER_ID aLayer )
{
    return m_layersControls[aLayer].name;
}


wxCheckBox* PANEL_SETUP_LAYERS::getCheckBox( PCB_LAYER_ID aLayer )
{
    return m_layersControls[aLayer].checkbox;
}


wxChoice* PANEL_SETUP_LAYERS::getChoice( PCB_LAYER_ID aLayer )
{
    return dynamic_cast<wxChoice*>( m_layersControls[aLayer].choice );
}


bool PANEL_SETUP_LAYERS::TransferDataToWindow()
{
    m_enabledLayers = m_pcb->GetEnabledLayers();

    // Rescue may be enabled, but should not be shown in this dialog
    m_enabledLayers.reset( Rescue );

    initialize_layers_controls();

    setCopperLayerCheckBoxes( m_pcb->GetCopperLayerCount() );

    showBoardLayerNames();
    showSelectedLayerCheckBoxes( m_enabledLayers );

    showLayerTypes();
    setUserDefinedLayerCheckBoxes();

    m_initialized = true;

    return true;
}


void PANEL_SETUP_LAYERS::SyncCopperLayers( int aNumCopperLayers )
{
    BOARD* savedBoard = m_pcb;
    BOARD  temp;

    m_pcb = &temp;
    transferDataFromWindow();

    for( size_t ii = 0; ii < m_enabledLayers.size(); ii++ )
    {
        if( IsCopperLayer( int( ii ) ) )
            m_enabledLayers.reset( ii );
    }

    m_enabledLayers |= LSET::AllCuMask( aNumCopperLayers );

    initialize_layers_controls();
    setCopperLayerCheckBoxes( aNumCopperLayers );

    showBoardLayerNames();
    showSelectedLayerCheckBoxes( m_enabledLayers );

    showLayerTypes();
    setUserDefinedLayerCheckBoxes();

    m_pcb = savedBoard;
}


void PANEL_SETUP_LAYERS::setUserDefinedLayerCheckBoxes()
{
    LSET layers = m_enabledLayers & LSET::UserDefinedLayersMask();

    for( PCB_LAYER_ID layer : layers )
        setLayerCheckBox( layer, m_pcb->IsLayerEnabled( layer ) );
}


void PANEL_SETUP_LAYERS::showBoardLayerNames()
{
    // Set all the board's layer names into the dialog by calling BOARD::LayerName(),
    // which will call BOARD::GetStandardLayerName() for non-coppers.

    for( PCB_LAYER_ID layer : m_enabledLayers )
    {
        wxControl*   ctl = getName( layer );

        if( ctl )
        {
            wxString lname = m_pcb->GetLayerName( layer );

            if( auto textCtl = dynamic_cast<wxTextCtrl*>( ctl ) )
                textCtl->ChangeValue( lname );     // wxTextCtrl
            else
                ctl->SetLabel( lname );         // wxStaticText
        }
    }
}


void PANEL_SETUP_LAYERS::showSelectedLayerCheckBoxes( const LSET& enabledLayers )
{
    for( auto& [layer,ctl] : m_layersControls )
        setLayerCheckBox( layer, enabledLayers.test( layer ) );
}


void PANEL_SETUP_LAYERS::showLayerTypes()
{
    LSET layers = m_enabledLayers & LSET::AllCuMask( m_pcb->GetCopperLayerCount() );

    for( PCB_LAYER_ID cu_layer : m_enabledLayers.CuStack() )
    {
        wxChoice* ctl = getChoice( cu_layer );

        switch ( m_pcb->GetLayerType( cu_layer ) )
        {
            case LT_SIGNAL: ctl->SetSelection( 0 ); break;
            case LT_POWER:  ctl->SetSelection( 1 ); break;
            case LT_MIXED:  ctl->SetSelection( 2 ); break;
            case LT_JUMPER: ctl->SetSelection( 3 ); break;
            default:        ctl->SetSelection( 0 );
        }

    }

    layers = m_enabledLayers & LSET::UserDefinedLayersMask();

    for( PCB_LAYER_ID layer : layers )
    {
        wxChoice* ctl = getChoice( layer );

        switch( m_pcb->GetLayerType( layer ) )
        {
        case LT_AUX:   ctl->SetSelection( 0 ); break;
        case LT_FRONT: ctl->SetSelection( 1 ); break;
        case LT_BACK:  ctl->SetSelection( 2 ); break;
        default:       ctl->SetSelection( 0 ); break;
        }
    }
}


LSET PANEL_SETUP_LAYERS::GetUILayerMask()
{
    LSET layerMaskResult;

    for( auto& [layer, _] : m_layersControls )
    {
        wxCheckBox*  ctl = getCheckBox( layer );

        if( ctl && ctl->IsChecked() )
            layerMaskResult.set( layer );
    }

    return layerMaskResult;
}


void PANEL_SETUP_LAYERS::setLayerCheckBox( PCB_LAYER_ID aLayer, bool isChecked )
{
    PANEL_SETUP_LAYERS_CTLs& ctl = m_layersControls[aLayer];

    if( !ctl.checkbox )
        return;

    ctl.checkbox->SetValue( isChecked );
}


void PANEL_SETUP_LAYERS::setCopperLayerCheckBoxes( int copperCount )
{
    if( copperCount > 0 )
    {
        wxCheckBox* fcu = getCheckBox( F_Cu );
        mandatoryLayerCbSetup( *fcu );
    }

    if( copperCount > 0 )
    {
        wxCheckBox* bcu = getCheckBox( B_Cu );
        mandatoryLayerCbSetup( *bcu );
    }

    LSET layers = m_enabledLayers & LSET::AllCuMask( copperCount );
    layers.reset( F_Cu );
    layers.reset( B_Cu );

    for( PCB_LAYER_ID layer : layers )
    {
        wxCheckBox* cb = getCheckBox( layer );
        mandatoryLayerCbSetup( *cb );
    }

}


bool PANEL_SETUP_LAYERS::transferDataFromWindow()
{
    bool modified = false;
    LSET enabledLayers = GetUILayerMask();

    LSET previousEnabled = m_pcb->GetEnabledLayers();

    if( enabledLayers != previousEnabled )
    {
        m_pcb->SetEnabledLayers( enabledLayers );

        LSET changedLayers = enabledLayers ^ previousEnabled;

        /*
         * Ensure enabled layers are also visible.  This is mainly to avoid mistakes if some
         * enabled layers are not visible when exiting this dialog.
         */
        m_pcb->SetVisibleLayers( m_pcb->GetVisibleLayers() | changedLayers );

        /*
         * Ensure items with through holes have all inner copper layers.  (For historical reasons
         * this is NOT trimmed to the currently-enabled inner layers.)
         */
        for( FOOTPRINT* fp : m_pcb->Footprints() )
        {
            for( PAD* pad : fp->Pads() )
            {
                if( pad->HasHole() && pad->IsOnCopperLayer() )
                    pad->SetLayerSet( pad->GetLayerSet() | LSET::InternalCuMask() );
            }
        }

        // Tracks do not change their layer
        // Vias layers are defined by the starting layer and the ending layer, so
        // they are not modified by adding a layer.
        // So do nothing for tracks/vias

        modified = true;
    }

    for( PCB_LAYER_ID layer : enabledLayers )
    {
        wxString newLayerName = getName( layer )->GetValue();

        if( m_pcb->GetLayerName( layer ) != newLayerName )
        {
            m_pcb->SetLayerName( layer, newLayerName );
            modified = true;
        }

        if( IsCopperLayer( layer ) )
        {
            LAYER_T t;

            switch( getChoice( layer )->GetCurrentSelection() )
            {
            case 0:  t = LT_SIGNAL;    break;
            case 1:  t = LT_POWER;     break;
            case 2:  t = LT_MIXED;     break;
            case 3:  t = LT_JUMPER;    break;
            default: t = LT_UNDEFINED; break;
            }

            if( m_pcb->GetLayerType( layer ) != t )
            {
                m_pcb->SetLayerType( layer, t );
                modified = true;
            }
        }
        else if( layer >= User_1 && !IsCopperLayer( layer ) )
        {
            LAYER_T t;

            switch( getChoice( layer )->GetCurrentSelection() )
            {
            case 0:  t = LT_AUX;       break;
            case 1:  t = LT_FRONT;     break;
            case 2:  t = LT_BACK;      break;
            default: t = LT_UNDEFINED; break;
            }

            if( m_pcb->GetLayerType( layer ) != t )
            {
                m_pcb->SetLayerType( layer, t );
                modified = true;
            }
        }
    }

    LSET layers = enabledLayers & LSET::UserDefinedLayersMask();

    for( PCB_LAYER_ID layer : layers )
    {
        wxString newLayerName = getName( layer )->GetValue();

        if( m_pcb->GetLayerName( layer ) != newLayerName )
        {
            m_pcb->SetLayerName( layer, newLayerName );
            modified = true;
        }
    }

    return modified;
}


bool PANEL_SETUP_LAYERS::TransferDataFromWindow()
{
    if( !testLayerNames() )
        return false;

    // Make sure we have the latest copper layer count
    if( m_physicalStackup )
        SyncCopperLayers( m_physicalStackup->GetCopperLayerCount() );

    wxString  msg;
    bool      modified = false;
    wxWindow* parent = wxGetTopLevelParent( this );

    // Check for removed layers with items which will get deleted from the board.
    LSEQ removedLayers = getRemovedLayersWithItems();

    // Check for non-copper layers in use in footprints, and therefore not removable.
    LSEQ notremovableLayers = getNonRemovableLayers();

    if( !notremovableLayers.empty() )
    {
        for( PCB_LAYER_ID layer : notremovableLayers )
            msg << m_pcb->GetLayerName( layer ) << wxT( "\n" );

        if( !IsOK( parent, wxString::Format( _( "Footprints have some items on removed layers:\n"
                                                "%s\n"
                                                "These items will be no longer accessible\n"
                                                "Do you wish to continue?" ),
                                             msg ) ) )
        {
            return false;
        }
    }

    if( !removedLayers.empty() )
    {
        std::vector<BOARD_ITEM*> items;
        std::vector<wxString>    itemDescriptions;

        for( PCB_LAYER_ID layer : removedLayers )
        {
            PCB_LAYER_COLLECTOR collector;
            collector.SetLayerId( layer );
            collector.Collect( m_pcb, GENERAL_COLLECTOR::BoardLevelItems );

            for( int i = 0; i < collector.GetCount(); i++ )
            {
                BOARD_ITEM* item = collector[i];

                if( item->Type() == PCB_FOOTPRINT_T || item->GetParentFootprint() )
                    continue;

                items.push_back( item );
                itemDescriptions.push_back( item->GetItemDescription( m_frame, true ) );
            }

            for( FOOTPRINT* footprint : m_pcb->Footprints() )
            {
                for( PAD* pad : footprint->Pads() )
                {
                    if( pad->HasExplicitDefinitionForLayer( layer ) )
                    {
                        items.push_back( pad );
                        itemDescriptions.push_back( wxString::Format( _( "Pad %s of %s on %s" ),
                                                                      pad->GetNumber(),
                                                                      footprint->GetReference(),
                                                                      m_pcb->GetLayerName( layer ) ) );
                    }
                }
            }
        }

        if( !items.empty() )
        {
            DIALOG_ITEMS_LIST dlg( parent, _( "Warning" ),
                                   _( "Items have been found on removed layers. This operation will "
                                      "delete all items from removed layers and cannot be undone." ),
                                   _( "Show Details" ) );

            dlg.AddItems( itemDescriptions );

            dlg.SetSelectionCallback(
                    [&]( int index )
                    {
                        if( index >= 0 && index < (int) items.size() )
                        {
                            m_frame->GetToolManager()->RunAction( ACTIONS::selectionClear );
                            m_frame->GetToolManager()->RunAction( ACTIONS::selectItem, static_cast<EDA_ITEM*>( items[index] ) );
                            m_frame->GetCanvas()->Refresh();
                        }
                    } );

            if( dlg.ShowModal() != wxID_OK )
                return false;
        }
    }

    // Delete all objects on layers that have been removed.  Leaving them in copper layers
    // can (will?) result in DRC errors and it pollutes the board file with cruft.
    if( !removedLayers.empty() )
    {
        m_frame->GetToolManager()->RunAction( ACTIONS::selectionClear );

        for( PCB_LAYER_ID layer_id : removedLayers )
            modified |= m_pcb->RemoveAllItemsOnLayer( layer_id );

        // Undo state may have copies of pointers deleted above
        m_frame->ClearUndoRedoList();
    }

    modified |= transferDataFromWindow();

    if( modified )
        m_frame->OnModify();

    return true;
}


bool PANEL_SETUP_LAYERS::testLayerNames()
{
    std::vector<wxString>    names;
    wxTextCtrl*  ctl;

    for( PCB_LAYER_ID layer : LSET::AllLayersMask() )
    {
        // we _can_ rely on m_enabledLayers being current here:

        if( !m_enabledLayers[layer] )
            continue;

        ctl = (wxTextCtrl*) getName( layer );
        wxString name = ctl->GetValue();

        // Check name for legality:
        // 1) Cannot be blank.
        // 2) Cannot have blanks.
        // 3) Cannot have " chars
        // 4) Cannot be 'signal'
        // 5) Must be unique.
        // 6) Cannot have illegal chars in filenames ( some filenames are built from layer names )
        //    like : % $ \ " / :
        wxString badchars = wxFileName::GetForbiddenChars( wxPATH_DOS );
        badchars.Append( '%' );

        if( !name )
        {
            PAGED_DIALOG::GetDialog( this )->SetError( _( "Layer must have a name." ), this, ctl );
            return false;
        }

        if( name.find_first_of( badchars ) != wxString::npos )
        {
            wxString msg = wxString::Format(_( "%s are forbidden in layer names." ), badchars );
            PAGED_DIALOG::GetDialog( this )->SetError( msg, this, ctl );
            return false;
        }

        if( name == wxT( "signal" ) )
        {
            PAGED_DIALOG::GetDialog( this )->SetError( _( "Layer name \"signal\" is reserved." ), this, ctl );
            return false;
        }

        for( const wxString& existingName : names )
        {
            if( name == existingName )
            {
                wxString msg = wxString::Format(_( "Layer name '%s' already in use." ), name );
                PAGED_DIALOG::GetDialog( this )->SetError( msg, this, ctl );
                return false;
            }
        }

        names.push_back( name );
    }

    return true;
}


LSEQ PANEL_SETUP_LAYERS::getRemovedLayersWithItems()
{
    LSEQ removedLayers;
    LSET newLayers = GetUILayerMask();
    LSET curLayers = m_pcb->GetEnabledLayers();

    if( newLayers == curLayers ) // Return an empty list if no change
        return removedLayers;

    for( PCB_LAYER_ID layer_id : curLayers )
    {
        if( !newLayers[layer_id] )
        {
            bool hasItems = m_pcb->HasItemsOnLayer( layer_id );

            if( !hasItems )
            {
                // Check for pads with custom properties on this layer
                for( FOOTPRINT* footprint : m_pcb->Footprints() )
                {
                    for( PAD* pad : footprint->Pads() )
                    {
                        if( pad->HasExplicitDefinitionForLayer( layer_id ) )
                        {
                            hasItems = true;
                            break;
                        }
                    }
                    if( hasItems )
                        break;
                }
            }

            if( hasItems )
                removedLayers.push_back( layer_id );
        }
    }

    return removedLayers;
}


LSEQ PANEL_SETUP_LAYERS::getNonRemovableLayers()
{
    // Build the list of non-copper layers in use in footprints.
    LSEQ inUseLayers;
    LSET newLayers = GetUILayerMask();
    LSET curLayers = m_pcb->GetEnabledLayers();

    if( newLayers == curLayers ) // Return an empty list if no change
        return inUseLayers;

    PCB_LAYER_COLLECTOR collector;

    for( PCB_LAYER_ID layer_id : curLayers )
    {
        if( IsCopperLayer( layer_id ) ) // Copper layers are not taken into account here
            continue;

        if( !newLayers.Contains( layer_id ) )
        {
            collector.SetLayerId( layer_id );
            collector.Collect( m_pcb, GENERAL_COLLECTOR::FootprintItems );

            if( collector.GetCount() != 0 )
                inUseLayers.push_back( layer_id );
        }
    }

    return inUseLayers;
}


void PANEL_SETUP_LAYERS::ImportSettingsFrom( BOARD* aBoard )
{
    BOARD* savedBoard = m_pcb;

    m_pcb = aBoard;
    TransferDataToWindow();

    m_pcb = savedBoard;
}


bool PANEL_SETUP_LAYERS::CheckCopperLayerCount( BOARD* aWorkingBoard, BOARD* aImportedBoard )
{
    /*
     * This function warns users if they are going to delete inner copper layers because
     * they're importing settings from a board with less copper layers than the board
     * already loaded. We want to return "true" as default on the assumption no layer will
     * actually be deleted.
     */
    bool okToDeleteCopperLayers = true;

    // Get the number of copper layers in the loaded board and the "import settings" board
    int currNumLayers = aWorkingBoard->GetCopperLayerCount();
    int newNumLayers  = aImportedBoard->GetCopperLayerCount();

    if( newNumLayers < currNumLayers )
    {
        wxString msg = wxString::Format( _( "Imported settings have fewer copper layers than "
                                            "the current board (%i instead of %i).\n\n"
                                            "Continue and delete the extra inner copper layers "
                                            "from the current board?" ),
                                         newNumLayers,
                                         currNumLayers );

        wxWindow* topLevelParent = wxGetTopLevelParent( this );

        wxMessageDialog dlg( topLevelParent, msg, _( "Inner Layers to Be Deleted" ),
                             wxICON_WARNING | wxSTAY_ON_TOP | wxYES | wxNO | wxNO_DEFAULT );

        if( wxID_ANY == dlg.ShowModal() )
            okToDeleteCopperLayers = false;
    }

    return okToDeleteCopperLayers;
}


void PANEL_SETUP_LAYERS::addUserDefinedLayer( wxCommandEvent& aEvent )
{
    wxArrayString headers;
    headers.Add( _( "Layers" ) );

    // Build the available user-defined layers list:
    std::vector<wxArrayString> list;

    for( PCB_LAYER_ID layer : LSET::UserDefinedLayersMask().Seq() )
    {
        wxCheckBox* checkBox = getCheckBox( layer );

        if( checkBox && checkBox->IsShown() )
            continue;

        wxArrayString available_user_layer;
        available_user_layer.Add( LayerName( layer ) );

        list.emplace_back( available_user_layer );
    }

    if( list.empty() )
    {
        DisplayErrorMessage( PAGED_DIALOG::GetDialog( this ),
                             _( "All user-defined layers have already been added." ) );
        return;
    }

    EDA_LIST_DIALOG dlg( PAGED_DIALOG::GetDialog( this ), _( "Add User-defined Layer" ),
                         headers, list );
    dlg.SetListLabel( _( "Select layer to add:" ) );
    dlg.HideFilter();

    if( dlg.ShowModal() == wxID_ANY || dlg.GetTextSelection().IsEmpty() )
        return;

    PCB_LAYER_ID layer = UNDEFINED_LAYER;

    for( PCB_LAYER_ID layer2 : LSET::UserDefinedLayersMask().Seq() )
    {
        if( LayerName( layer2 ) == dlg.GetTextSelection() )
        {
            layer = layer2;
            break;
        }
    }

    wxCHECK( layer >= User_1, /* void */ );

    m_enabledLayers.set( layer );
    append_user_layer( layer );

    PANEL_SETUP_LAYERS_CTLs& ctl = m_layersControls[layer];

    // All user-defined layers should have a checkbox
    wxASSERT( ctl.checkbox );
    ctl.checkbox->SetValue( true );

    wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( ctl.name );

    wxCHECK( textCtrl, /* void */ );
    textCtrl->ChangeValue( LSET::Name( layer ) );

    wxChoice* userLayerType = dynamic_cast<wxChoice*>( ctl.choice );

    wxCHECK( userLayerType, /* void */ );
    userLayerType->SetSelection( 0 );

    ctl.name->Show( true );
    ctl.checkbox->Show( true );
    ctl.choice->Show( true );

    wxSizeEvent evt_size( m_LayersListPanel->GetSize() );
    m_LayersListPanel->GetEventHandler()->ProcessEvent( evt_size );
}


