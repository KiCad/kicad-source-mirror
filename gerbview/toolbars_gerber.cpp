/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/wupdlock.h>
#include <wx/stattext.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <bitmaps.h>
#include <gerbview_id.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <string_utils.h>
#include <tool/actions.h>
#include <tool/action_toolbar.h>
#include <tools/gerbview_actions.h>
#include "widgets/gbr_layer_box_selector.h"
#include "widgets/dcode_selection_box.h"
#include <toolbars_gerber.h>


std::optional<TOOLBAR_CONFIGURATION> GERBVIEW_TOOLBAR_SETTINGS::DefaultToolbarConfig( TOOLBAR_LOC aToolbar )
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    switch( aToolbar )
    {
    // No right toolbar
    case TOOLBAR_LOC::RIGHT:
        return std::nullopt;

    case TOOLBAR_LOC::LEFT:
        config.AppendAction( ACTIONS::selectionTool )
              .AppendAction( ACTIONS::measureTool );

        config.AppendSeparator()
              .AppendAction( ACTIONS::toggleGrid )
              .AppendAction( ACTIONS::togglePolarCoords )
              .AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Units" ) )
                            .AddAction( ACTIONS::millimetersUnits )
                            .AddAction( ACTIONS::inchesUnits )
                            .AddAction( ACTIONS::milsUnits ) )
              .AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Crosshair modes" ) )
                            .AddAction( ACTIONS::cursorSmallCrosshairs )
                            .AddAction( ACTIONS::cursorFullCrosshairs )
                            .AddAction( ACTIONS::cursor45Crosshairs ) );

        config.AppendSeparator()
              .AppendAction( GERBVIEW_ACTIONS::flashedDisplayOutlines )
              .AppendAction( GERBVIEW_ACTIONS::linesDisplayOutlines )
              .AppendAction( GERBVIEW_ACTIONS::polygonsDisplayOutlines )
              .AppendAction( GERBVIEW_ACTIONS::negativeObjectDisplay )
              .AppendAction( GERBVIEW_ACTIONS::dcodeDisplay );

        config.AppendSeparator()
              .AppendAction( GERBVIEW_ACTIONS::toggleForceOpacityMode )
              .AppendAction( GERBVIEW_ACTIONS::toggleXORMode )
              .AppendAction( ACTIONS::highContrastMode )
              .AppendAction( GERBVIEW_ACTIONS::flipGerberView );

        config.AppendSeparator()
              .AppendAction( GERBVIEW_ACTIONS::toggleLayerManager );
        break;

    case TOOLBAR_LOC::TOP_MAIN:
        config.AppendAction( GERBVIEW_ACTIONS::clearAllLayers )
              .AppendAction( GERBVIEW_ACTIONS::reloadAllLayers )
              .AppendAction( GERBVIEW_ACTIONS::openAutodetected )
              .AppendAction( GERBVIEW_ACTIONS::openGerber )
              .AppendAction( GERBVIEW_ACTIONS::openDrillFile );

        config.AppendSeparator()
              .AppendAction( ACTIONS::print );

        config.AppendSeparator()
              .AppendAction( ACTIONS::zoomRedraw )
              .AppendAction( ACTIONS::zoomInCenter )
              .AppendAction( ACTIONS::zoomOutCenter )
              .AppendAction( ACTIONS::zoomFitScreen )
              .AppendAction( ACTIONS::zoomTool );

        config.AppendSeparator()
              .AppendControl( ACTION_TOOLBAR_CONTROLS::layerSelector )
              .AppendControl( GERBVIEW_ACTION_TOOLBAR_CONTROLS::textInfo );
        break;

    case TOOLBAR_LOC::TOP_AUX:
        config.AppendControl( GERBVIEW_ACTION_TOOLBAR_CONTROLS::componentHighlight )
              .AppendSpacer( 5 )
              .AppendControl( GERBVIEW_ACTION_TOOLBAR_CONTROLS::netHighlight )
              .AppendSpacer( 5 )
              .AppendControl( GERBVIEW_ACTION_TOOLBAR_CONTROLS::appertureHighlight )
              .AppendSpacer( 5 )
              .AppendControl( GERBVIEW_ACTION_TOOLBAR_CONTROLS::dcodeSelector )
              .AppendSeparator()
              .AppendControl( ACTION_TOOLBAR_CONTROLS::gridSelect )
              .AppendSeparator()
              .AppendControl( ACTION_TOOLBAR_CONTROLS::zoomSelect );
        break;
    }

    // clang-format on
    return config;
}


void GERBVIEW_FRAME::configureToolbars()
{
    // Base class loads the default settings
    EDA_DRAW_FRAME::configureToolbars();

    // Register factories for the various toolbar controls
    auto layerBoxFactory =
            [this]( ACTION_TOOLBAR* aToolbar )
            {
                if( !m_SelLayerBox )
                {
                    m_SelLayerBox = new GBR_LAYER_BOX_SELECTOR( aToolbar, ID_TOOLBARH_GERBVIEW_SELECT_ACTIVE_LAYER,
                                                                wxDefaultPosition, wxDefaultSize, 0, nullptr );
                }

                m_SelLayerBox->Resync();
                aToolbar->Add( m_SelLayerBox );

                // UI update handler for the control
                aToolbar->Bind( wxEVT_UPDATE_UI,
                                [this]( wxUpdateUIEvent& aEvent )
                                    {
                                        if( m_SelLayerBox->GetCount() )
                                        {
                                            if( m_SelLayerBox->GetSelection() != GetActiveLayer() )
                                                m_SelLayerBox->SetSelection( GetActiveLayer() );
                                        }
                                    },
                                m_SelLayerBox->GetId() );
            };

    RegisterCustomToolbarControlFactory( ACTION_TOOLBAR_CONTROLS::layerSelector, layerBoxFactory );


    auto textInfoFactory =
            [this]( ACTION_TOOLBAR* aToolbar )
            {
                if( !m_TextInfo )
                {
                    m_TextInfo = new wxTextCtrl( aToolbar, ID_TOOLBARH_GERBER_DATA_TEXT_BOX, wxEmptyString,
                                                wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
                }

                aToolbar->Add( m_TextInfo );
            };

    RegisterCustomToolbarControlFactory( GERBVIEW_ACTION_TOOLBAR_CONTROLS::textInfo, textInfoFactory );


    // Creates box to display and choose components:
    // (note, when the m_tbTopAux is recreated, tools are deleted, but controls
    // are not deleted: they are just no longer managed by the toolbar
    auto componentBoxFactory =
            [this]( ACTION_TOOLBAR* aToolbar )
            {
                if( !m_SelComponentBox )
                    m_SelComponentBox = new wxChoice( aToolbar, ID_GBR_AUX_TOOLBAR_PCB_CMP_CHOICE );

                if( !m_cmpText )
                    m_cmpText = new wxStaticText( aToolbar, wxID_ANY, _( "Cmp:" ) + wxS( " " ) );

                m_SelComponentBox->SetToolTip( _("Highlight items belonging to this component") );
                m_cmpText->SetLabel( _( "Cmp:" ) + wxS( " " ) );     // can change when changing the language

                updateComponentListSelectBox();

                aToolbar->Add( m_cmpText );
                aToolbar->Add( m_SelComponentBox );
            };

    RegisterCustomToolbarControlFactory( GERBVIEW_ACTION_TOOLBAR_CONTROLS::componentHighlight, componentBoxFactory );


    // Creates choice box to display net names and highlight selected:
    auto netBoxFactory =
            [this]( ACTION_TOOLBAR* aToolbar )
            {
                if( !m_SelNetnameBox )
                m_SelNetnameBox = new wxChoice( aToolbar, ID_GBR_AUX_TOOLBAR_PCB_NET_CHOICE );

                if( !m_netText )
                    m_netText = new wxStaticText( aToolbar, wxID_ANY, _( "Net:" ) );

                m_SelNetnameBox->SetToolTip( _("Highlight items belonging to this net") );
                m_netText->SetLabel( _( "Net:" ) );     // can change when changing the language

                updateNetnameListSelectBox();

                aToolbar->Add( m_netText );
                aToolbar->Add( m_SelNetnameBox );
            };

    RegisterCustomToolbarControlFactory( GERBVIEW_ACTION_TOOLBAR_CONTROLS::netHighlight, netBoxFactory );


    // Creates choice box to display aperture attributes and highlight selected:
    auto appertureBoxFactory =
            [this]( ACTION_TOOLBAR* aToolbar )
            {
                if( !m_SelAperAttributesBox )
                    m_SelAperAttributesBox = new wxChoice( aToolbar, ID_GBR_AUX_TOOLBAR_PCB_APERATTRIBUTES_CHOICE );

                if( !m_apertText )
                    m_apertText = new wxStaticText( aToolbar, wxID_ANY, _( "Attr:" ) );

                m_SelAperAttributesBox->SetToolTip( _( "Highlight items with this aperture attribute" ) );
                m_apertText->SetLabel( _( "Attr:" ) ); // can change when changing the language

                updateAperAttributesSelectBox();

                aToolbar->Add( m_apertText );
                aToolbar->Add( m_SelAperAttributesBox );
            };

    RegisterCustomToolbarControlFactory( GERBVIEW_ACTION_TOOLBAR_CONTROLS::appertureHighlight, appertureBoxFactory );


    // D-code selection
    auto dcodeSelectorFactory =
            [this]( ACTION_TOOLBAR* aToolbar )
            {
                if( !m_DCodeSelector )
                {
                    m_DCodeSelector = new DCODE_SELECTION_BOX( aToolbar, ID_TOOLBARH_GERBER_SELECT_ACTIVE_DCODE,
                                                               wxDefaultPosition, wxSize( 150, -1 ) );
                }

                if( !m_dcodeText )
                    m_dcodeText = new wxStaticText( aToolbar, wxID_ANY, _( "DCode:" ) );

                m_dcodeText->SetLabel( _( "DCode:" ) );

                updateDCodeSelectBox();

                aToolbar->Add( m_dcodeText );
                aToolbar->Add( m_DCodeSelector );
            };

    RegisterCustomToolbarControlFactory( GERBVIEW_ACTION_TOOLBAR_CONTROLS::dcodeSelector, dcodeSelectorFactory );
}


void GERBVIEW_FRAME::ClearToolbarControl( int aId )
{
    EDA_DRAW_FRAME::ClearToolbarControl( aId );

    switch( aId )
    {
    case ID_TOOLBARH_GERBVIEW_SELECT_ACTIVE_LAYER:
        m_SelLayerBox = nullptr;
        break;
    case ID_TOOLBARH_GERBER_DATA_TEXT_BOX:
        m_TextInfo = nullptr;
        break;
    case ID_GBR_AUX_TOOLBAR_PCB_CMP_CHOICE:
        m_SelComponentBox = nullptr;
        m_cmpText = nullptr;
        break;
    case ID_GBR_AUX_TOOLBAR_PCB_NET_CHOICE:
        m_SelNetnameBox = nullptr;
        m_netText = nullptr;
        break;
    case ID_GBR_AUX_TOOLBAR_PCB_APERATTRIBUTES_CHOICE:
        m_SelAperAttributesBox = nullptr;
        m_apertText = nullptr;
        break;
    case ID_TOOLBARH_GERBER_SELECT_ACTIVE_DCODE:
        m_DCodeSelector = nullptr;
        m_dcodeText = nullptr;
        break;
    }
}


ACTION_TOOLBAR_CONTROL GERBVIEW_ACTION_TOOLBAR_CONTROLS::textInfo( "control.TextInfo",
                                                                   _( "Text info entry" ),
                                                                   _( "Text info entry" ),
                                                                   { FRAME_GERBER } );
ACTION_TOOLBAR_CONTROL GERBVIEW_ACTION_TOOLBAR_CONTROLS::componentHighlight( "control.ComponentHighlight",
                                                                             _( "Component highlight" ),
                                                                             _( "Highlight items belonging to this component" ),
                                                                             { FRAME_GERBER } );
ACTION_TOOLBAR_CONTROL GERBVIEW_ACTION_TOOLBAR_CONTROLS::netHighlight( "control.NetHighlight",
                                                                       _( "Net highlight" ),
                                                                       _( "Highlight items belonging to this net" ),
                                                                       { FRAME_GERBER } );
ACTION_TOOLBAR_CONTROL GERBVIEW_ACTION_TOOLBAR_CONTROLS::appertureHighlight( "control.AppertureHighlight",
                                                                             _( "Aperture highlight" ),
                                                                             _( "Highlight items with this aperture attribute" ),
                                                                             { FRAME_GERBER } );
ACTION_TOOLBAR_CONTROL GERBVIEW_ACTION_TOOLBAR_CONTROLS::dcodeSelector( "control.GerberDcodeSelector",
                                                                        _( "DCode selector" ),
                                                                        _( "Select all items with the selected DCode" ),
                                                                        { FRAME_GERBER } );


#define NO_SELECTION_STRING _( "<No selection>" )


void GERBVIEW_FRAME::updateDCodeSelectBox()
{
    m_DCodeSelector->Clear();

    // Add an empty string to deselect net highlight
    m_DCodeSelector->Append( NO_SELECTION_STRING );

    int layer = GetActiveLayer();
    GERBER_FILE_IMAGE* gerber = GetGbrImage( layer );

    if( !gerber || gerber->GetDcodesCount() == 0 )
    {
        if( m_DCodeSelector->GetSelection() != 0 )
            m_DCodeSelector->SetSelection( 0 );

        return;
    }

    // Build the aperture list of the current layer, and add it to the combo box:
    wxArrayString dcode_list;
    wxString msg;

    double   scale = 1.0;
    wxString units;

    switch( GetUserUnits() )
    {
    case EDA_UNITS::MM:
        scale = gerbIUScale.IU_PER_MM;
        units = wxT( "mm" );
        break;

    case EDA_UNITS::INCH:
        scale = gerbIUScale.IU_PER_MILS * 1000;
        units = wxT( "in" );
        break;

    case EDA_UNITS::MILS:
        scale = gerbIUScale.IU_PER_MILS;
        units = wxT( "mil" );
        break;

    default:
        wxASSERT_MSG( false, wxT( "Invalid units" ) );
    }

    for( const auto& [_, dcode] : gerber->m_ApertureList )
    {
        wxCHECK2( dcode,continue );

        if( !dcode->m_InUse && !dcode->m_Defined )
            continue;

        msg.Printf( wxT( "tool %d [%.3fx%.3f %s] %s" ),
                    dcode->m_Num_Dcode,
                    dcode->m_Size.x / scale, dcode->m_Size.y / scale,
                    units,
                    D_CODE::ShowApertureType( dcode->m_ApertType ) );

        if( !dcode->m_AperFunction.IsEmpty() )
            msg << wxT( ", " ) << dcode->m_AperFunction;

        dcode_list.Add( msg );
    }

    m_DCodeSelector->AppendDCodeList( dcode_list );

    if( dcode_list.size() > 1 )
    {
        wxSize size = m_DCodeSelector->GetBestSize();
        size.x = std::max( size.x, 100 );
        m_DCodeSelector->SetMinSize( size );
        m_auimgr.Update();
    }
}


void GERBVIEW_FRAME::updateComponentListSelectBox()
{
    m_SelComponentBox->Clear();

    // Build the full list of component names from the partial lists stored in each file image
    std::map<wxString, int> full_list;

    for( unsigned layer = 0; layer < GetImagesList()->ImagesMaxCount(); ++layer )
    {
        GERBER_FILE_IMAGE* gerber = GetImagesList()->GetGbrImage( layer );

        if( gerber == nullptr )    // Graphic layer not yet used
            continue;

        full_list.insert( gerber->m_ComponentsList.begin(), gerber->m_ComponentsList.end() );
    }

    // Add an empty string to deselect net highlight
    m_SelComponentBox->Append( NO_SELECTION_STRING );

    // Now copy the list to the choice box
    for( const std::pair<const wxString, int>& entry : full_list )
        m_SelComponentBox->Append( entry.first );

    m_SelComponentBox->SetSelection( 0 );
}


void GERBVIEW_FRAME::updateNetnameListSelectBox()
{
    m_SelNetnameBox->Clear();

    // Build the full list of netnames from the partial lists stored in each file image
    std::map<wxString, int> full_list;

    for( unsigned layer = 0; layer < GetImagesList()->ImagesMaxCount(); ++layer )
    {
        GERBER_FILE_IMAGE* gerber = GetImagesList()->GetGbrImage( layer );

        if( gerber == nullptr )    // Graphic layer not yet used
            continue;

        full_list.insert( gerber->m_NetnamesList.begin(), gerber->m_NetnamesList.end() );
    }

    // Add an empty string to deselect net highlight
    m_SelNetnameBox->Append( NO_SELECTION_STRING );

    // Now copy the list to the choice box
    for( const std::pair<const wxString, int>& entry : full_list )
        m_SelNetnameBox->Append( UnescapeString( entry.first ) );

    m_SelNetnameBox->SetSelection( 0 );
}


void GERBVIEW_FRAME::updateAperAttributesSelectBox()
{
    m_SelAperAttributesBox->Clear();

    // Build the full list of netnames from the partial lists stored in each file image
    std::map<wxString, int> full_list;

    for( unsigned layer = 0; layer < GetImagesList()->ImagesMaxCount(); ++layer )
    {
        GERBER_FILE_IMAGE* gerber = GetImagesList()->GetGbrImage( layer );

        if( gerber == nullptr )    // Graphic layer not yet used
            continue;

        if( gerber->GetDcodesCount() == 0 )
            continue;

        for( const auto &[_, aperture] : gerber->m_ApertureList )
        {
            if( aperture == nullptr )
                continue;

            if( !aperture->m_InUse && !aperture->m_Defined )
                continue;

            if( !aperture->m_AperFunction.IsEmpty() )
                full_list.insert( std::make_pair( aperture->m_AperFunction, 0 ) );
        }
    }

    // Add an empty string to deselect net highlight
    m_SelAperAttributesBox->Append( NO_SELECTION_STRING );

    // Now copy the list to the choice box
    for( const std::pair<const wxString, int>& entry : full_list )
        m_SelAperAttributesBox->Append( entry.first );

    m_SelAperAttributesBox->SetSelection( 0 );
}


void GERBVIEW_FRAME::OnUpdateSelectDCode( wxUpdateUIEvent& aEvent )
{
    if( !m_DCodeSelector )
        return;

    int                layer = GetActiveLayer();
    GERBER_FILE_IMAGE* gerber = GetGbrImage( layer );
    int                selected = gerber ? gerber->m_Selected_Tool : 0;

    aEvent.Enable( gerber != nullptr );

    if( m_DCodeSelector->GetSelectedDCodeId() != selected )
    {
        m_DCodeSelector->SetDCodeSelection( selected );
        // Be sure the selection can be made. If no, set to
        // a correct value
        if( gerber )
            gerber->m_Selected_Tool = m_DCodeSelector->GetSelectedDCodeId();
    }
}
