/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <gerbview.h>
#include <gerbview_frame.h>
#include <bitmaps.h>
#include <gerbview_id.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <dialog_helpers.h>
#include <string_utils.h>
#include <wx/wupdlock.h>
#include <tool/actions.h>
#include <tool/action_toolbar.h>
#include <tools/gerbview_actions.h>
#include "widgets/gbr_layer_box_selector.h"
#include "widgets/dcode_selection_box.h"


void GERBVIEW_FRAME::ReCreateHToolbar()
{
    // Note:
    // To rebuild the aui toolbar, the more easy way is to clear ( calling m_mainToolBar.Clear() )
    // all wxAuiToolBarItems.
    // However the wxAuiToolBarItems are not the owners of controls managed by
    // them ( m_TextInfo and m_SelLayerBox ), and therefore do not delete them
    // So we do not recreate them after clearing the tools.

    if( m_mainToolBar )
    {
        m_mainToolBar->ClearToolbar();
    }
    else
    {
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT |
                                            wxAUI_TB_HORIZONTAL );
        m_mainToolBar->SetAuiManager( &m_auimgr );
    }

    // Set up toolbar
    m_mainToolBar->Add( GERBVIEW_ACTIONS::clearAllLayers );
    m_mainToolBar->Add( GERBVIEW_ACTIONS::reloadAllLayers );
    m_mainToolBar->Add( GERBVIEW_ACTIONS::openGerber );
    m_mainToolBar->Add( GERBVIEW_ACTIONS::openDrillFile );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::print );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::zoomRedraw );
    m_mainToolBar->Add( ACTIONS::zoomInCenter );
    m_mainToolBar->Add( ACTIONS::zoomOutCenter );
    m_mainToolBar->Add( ACTIONS::zoomFitScreen );
    m_mainToolBar->Add( ACTIONS::zoomTool, ACTION_TOOLBAR::TOGGLE, ACTION_TOOLBAR::CANCEL );


    m_mainToolBar->AddScaledSeparator( this );

    if( !m_SelLayerBox )
        m_SelLayerBox = new GBR_LAYER_BOX_SELECTOR( m_mainToolBar,
                                                    ID_TOOLBARH_GERBVIEW_SELECT_ACTIVE_LAYER,
                                                    wxDefaultPosition, wxDefaultSize, 0, nullptr );

    m_SelLayerBox->Resync();
    m_mainToolBar->AddControl( m_SelLayerBox );

    if( !m_TextInfo )
        m_TextInfo = new wxTextCtrl( m_mainToolBar, ID_TOOLBARH_GERBER_DATA_TEXT_BOX, wxEmptyString, wxDefaultPosition,
                                     wxDefaultSize, wxTE_READONLY );

    m_mainToolBar->AddControl( m_TextInfo );

    m_mainToolBar->UpdateControlWidth( ID_TOOLBARH_GERBVIEW_SELECT_ACTIVE_LAYER );
    m_mainToolBar->UpdateControlWidth( ID_TOOLBARH_GERBER_DATA_TEXT_BOX );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->KiRealize();
}


void GERBVIEW_FRAME::ReCreateAuxiliaryToolbar()
{
    wxWindowUpdateLocker dummy( this );

    if( m_auxiliaryToolBar )
    {
        m_auxiliaryToolBar->ClearToolbar();
    }
    else
    {
        m_auxiliaryToolBar = new ACTION_TOOLBAR( this, ID_AUX_TOOLBAR,
                                                 wxDefaultPosition, wxDefaultSize,
                                                 KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );
        m_auxiliaryToolBar->SetAuiManager( &m_auimgr );
    }

    // Creates box to display and choose components:
    // (note, when the m_auxiliaryToolBar is recreated, tools are deleted, but controls
    // are not deleted: they are just no longer managed by the toolbar
    if( !m_SelComponentBox )
        m_SelComponentBox = new wxChoice( m_auxiliaryToolBar,
                                          ID_GBR_AUX_TOOLBAR_PCB_CMP_CHOICE );

    if( !m_cmpText )
        m_cmpText = new wxStaticText( m_auxiliaryToolBar, wxID_ANY, _( "Cmp:" ) + wxS( " " ) );

    m_SelComponentBox->SetToolTip( _("Highlight items belonging to this component") );
    m_cmpText->SetLabel( _( "Cmp:" ) + wxS( " " ) );     // can change when changing the language
    m_auxiliaryToolBar->AddControl( m_cmpText );
    m_auxiliaryToolBar->AddControl( m_SelComponentBox );
    m_auxiliaryToolBar->AddSpacer( 5 );

    // Creates choice box to display net names and highlight selected:
    if( !m_SelNetnameBox )
        m_SelNetnameBox = new wxChoice( m_auxiliaryToolBar,
                                        ID_GBR_AUX_TOOLBAR_PCB_NET_CHOICE );

    if( !m_netText )
        m_netText = new wxStaticText( m_auxiliaryToolBar, wxID_ANY, _( "Net:" ) );

    m_SelNetnameBox->SetToolTip( _("Highlight items belonging to this net") );
    m_netText->SetLabel( _( "Net:" ) );     // can change when changing the language
    m_auxiliaryToolBar->AddControl( m_netText );
    m_auxiliaryToolBar->AddControl( m_SelNetnameBox );
    m_auxiliaryToolBar->AddSpacer( 5 );

    // Creates choice box to display aperture attributes and highlight selected:
    if( !m_SelAperAttributesBox )
        m_SelAperAttributesBox = new wxChoice( m_auxiliaryToolBar,
                                               ID_GBR_AUX_TOOLBAR_PCB_APERATTRIBUTES_CHOICE );

    if( !m_apertText )
        m_apertText = new wxStaticText( m_auxiliaryToolBar, wxID_ANY, _( "Attr:" ) );

    m_SelAperAttributesBox->SetToolTip( _( "Highlight items with this aperture attribute" ) );
    m_apertText->SetLabel( _( "Attr:" ) ); // can change when changing the language
    m_auxiliaryToolBar->AddControl( m_apertText );
    m_auxiliaryToolBar->AddControl( m_SelAperAttributesBox );
    m_auxiliaryToolBar->AddSpacer( 5 );

    if( !m_DCodeSelector )
        m_DCodeSelector = new DCODE_SELECTION_BOX( m_auxiliaryToolBar,
                                                   ID_TOOLBARH_GERBER_SELECT_ACTIVE_DCODE,
                                                   wxDefaultPosition, wxSize( 150, -1 ) );

    if( !m_dcodeText )
        m_dcodeText = new wxStaticText( m_auxiliaryToolBar, wxID_ANY, _( "DCode:" ) );

    m_dcodeText->SetLabel( _( "DCode:" ) );
    m_auxiliaryToolBar->AddControl( m_dcodeText );
    m_auxiliaryToolBar->AddControl( m_DCodeSelector );

    if( !m_gridSelectBox )
    {
        m_gridSelectBox = new wxChoice( m_auxiliaryToolBar, ID_ON_GRID_SELECT,
                                        wxDefaultPosition, wxDefaultSize, 0, nullptr );
    }

    m_auxiliaryToolBar->AddScaledSeparator( this );
    m_auxiliaryToolBar->AddControl( m_gridSelectBox );

    if( !m_zoomSelectBox )
    {
        m_zoomSelectBox = new wxChoice( m_auxiliaryToolBar, ID_ON_ZOOM_SELECT,
                                        wxDefaultPosition, wxDefaultSize, 0, nullptr );
    }

    m_auxiliaryToolBar->AddScaledSeparator( this );
    m_auxiliaryToolBar->AddControl( m_zoomSelectBox );

    updateComponentListSelectBox();
    updateNetnameListSelectBox();
    updateAperAttributesSelectBox();
    updateDCodeSelectBox();
    UpdateGridSelectBox();
    UpdateZoomSelectBox();

    // Go through and ensure the comboboxes are the correct size, since the strings in the
    // box could have changed widths.
    m_auxiliaryToolBar->UpdateControlWidth( ID_GBR_AUX_TOOLBAR_PCB_CMP_CHOICE );
    m_auxiliaryToolBar->UpdateControlWidth( ID_GBR_AUX_TOOLBAR_PCB_NET_CHOICE );
    m_auxiliaryToolBar->UpdateControlWidth( ID_GBR_AUX_TOOLBAR_PCB_APERATTRIBUTES_CHOICE );
    m_auxiliaryToolBar->UpdateControlWidth( ID_ON_GRID_SELECT );
    m_auxiliaryToolBar->UpdateControlWidth( ID_ON_ZOOM_SELECT );

    // after adding the buttons to the toolbar, must call Realize()
    m_auxiliaryToolBar->KiRealize();
}


void GERBVIEW_FRAME::ReCreateVToolbar()
{
    // This toolbar isn't used currently
}


void GERBVIEW_FRAME::ReCreateOptToolbar()
{
    if( m_optionsToolBar )
    {
        m_optionsToolBar->ClearToolbar();
    }
    else
    {
        m_optionsToolBar = new ACTION_TOOLBAR( this, ID_OPT_TOOLBAR, wxDefaultPosition,
                                               wxDefaultSize,
                                               KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );
        m_optionsToolBar->SetAuiManager( &m_auimgr );
    }

    // TODO: these can be moved to the 'proper' vertical toolbar if and when there are
    // actual tools to put there. That, or I'll get around to implementing configurable
    // toolbars.
    m_optionsToolBar->Add( ACTIONS::selectionTool,                    ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::measureTool,                      ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->AddScaledSeparator( this );
    m_optionsToolBar->Add( ACTIONS::toggleGrid,                       ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::togglePolarCoords,                ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::inchesUnits,                      ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::milsUnits,                        ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::millimetersUnits,                 ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::toggleCursorStyle,                ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->AddScaledSeparator( this );
    m_optionsToolBar->Add( GERBVIEW_ACTIONS::flashedDisplayOutlines,  ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( GERBVIEW_ACTIONS::linesDisplayOutlines,    ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( GERBVIEW_ACTIONS::polygonsDisplayOutlines, ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( GERBVIEW_ACTIONS::negativeObjectDisplay,   ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( GERBVIEW_ACTIONS::dcodeDisplay,            ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( GERBVIEW_ACTIONS::toggleDiffMode,          ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::highContrastMode,                 ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( GERBVIEW_ACTIONS::flipGerberView,          ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->AddScaledSeparator( this );
    m_optionsToolBar->Add( GERBVIEW_ACTIONS::toggleLayerManager,      ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->KiRealize();
}


void GERBVIEW_FRAME::UpdateToolbarControlSizes()
{
    if( m_mainToolBar )
    {
        // Update the item widths
        m_mainToolBar->UpdateControlWidth( ID_TOOLBARH_GERBVIEW_SELECT_ACTIVE_LAYER );
        m_mainToolBar->UpdateControlWidth( ID_TOOLBARH_GERBER_DATA_TEXT_BOX );
    }

    if( m_auxiliaryToolBar )
    {
        // Update the item widths
        m_auxiliaryToolBar->UpdateControlWidth( ID_GBR_AUX_TOOLBAR_PCB_CMP_CHOICE );
        m_auxiliaryToolBar->UpdateControlWidth( ID_GBR_AUX_TOOLBAR_PCB_NET_CHOICE );
        m_auxiliaryToolBar->UpdateControlWidth( ID_GBR_AUX_TOOLBAR_PCB_APERATTRIBUTES_CHOICE );
        m_auxiliaryToolBar->UpdateControlWidth( ID_ON_GRID_SELECT );
        m_auxiliaryToolBar->UpdateControlWidth( ID_ON_ZOOM_SELECT );
    }
}


#define NO_SELECTION_STRING _("<No selection>")


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
    case EDA_UNITS::MILLIMETRES:
        scale = IU_PER_MM;
        units = "mm";
        break;

    case EDA_UNITS::INCHES:
        scale = IU_PER_MILS * 1000;
        units = "in";
        break;

    case EDA_UNITS::MILS:
        scale = IU_PER_MILS;
        units = "mil";
        break;

    default:
        wxASSERT_MSG( false, "Invalid units" );
    }

    for( int ii = 0; ii < TOOLS_MAX_COUNT; ii++ )
    {
        D_CODE* dcode = gerber->GetDCODE( ii + FIRST_DCODE );

        if( dcode == nullptr )
            continue;

        if( !dcode->m_InUse && !dcode->m_Defined )
            continue;

        msg.Printf( "tool %d [%.3fx%.3f %s] %s",
                    dcode->m_Num_Dcode,
                    dcode->m_Size.x / scale, dcode->m_Size.y / scale,
                    units,
                    D_CODE::ShowApertureType( dcode->m_Shape )
                    );
        if( !dcode->m_AperFunction.IsEmpty() )
            msg << ", " << dcode->m_AperFunction;

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
    for( auto& ii : full_list )
        m_SelComponentBox->Append( ii.first );

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
    for( auto& ii : full_list )
        m_SelNetnameBox->Append( UnescapeString( ii.first ) );

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

        for( int ii = 0; ii < TOOLS_MAX_COUNT; ii++ )
        {
            D_CODE* aperture = gerber->GetDCODE( ii + FIRST_DCODE );

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
    for( auto ii = full_list.begin(); ii != full_list.end(); ++ii )
    {
        m_SelAperAttributesBox->Append( ii->first );
    }

    m_SelAperAttributesBox->SetSelection( 0 );
}


void GERBVIEW_FRAME::OnUpdateDrawMode( wxUpdateUIEvent& aEvent )
{
    switch( aEvent.GetId() )
    {
    case ID_TB_OPTIONS_SHOW_GBR_MODE_0:
        aEvent.Check( GetDisplayMode() == 0 );
        break;

    case ID_TB_OPTIONS_SHOW_GBR_MODE_1:
        aEvent.Check( GetDisplayMode() == 1 );
        break;

    case ID_TB_OPTIONS_SHOW_GBR_MODE_2:
        aEvent.Check( GetDisplayMode() == 2 );
        break;

    default:
        break;
    }
}


void GERBVIEW_FRAME::OnUpdateSelectDCode( wxUpdateUIEvent& aEvent )
{
    if( !m_DCodeSelector )
        return;

    int layer = GetActiveLayer();
    GERBER_FILE_IMAGE* gerber = GetGbrImage( layer );
    int selected = ( gerber ) ? gerber->m_Selected_Tool : 0;

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


void GERBVIEW_FRAME::OnUpdateLayerSelectBox( wxUpdateUIEvent& aEvent )
{
    if( m_SelLayerBox->GetSelection() != GetActiveLayer() )
        m_SelLayerBox->SetSelection( GetActiveLayer() );
}

