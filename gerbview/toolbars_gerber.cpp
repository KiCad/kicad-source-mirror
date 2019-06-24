/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>

#include <common.h>
#include <gerbview.h>
#include <gerbview_frame.h>
#include <bitmaps.h>
#include <gerbview_id.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <gbr_layer_box_selector.h>
#include <DCodeSelectionbox.h>
#include <dialog_helpers.h>
#include <bitmaps.h>
#include <kicad_string.h>
#include <wx/wupdlock.h>
#include <tool/actions.h>
#include <tool/action_toolbar.h>
#include <tools/gerbview_actions.h>

void GERBVIEW_FRAME::ReCreateHToolbar( void )
{
    wxString      msg;

    if( m_mainToolBar )
        m_mainToolBar->Clear();
    else
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // Set up toolbar
    m_mainToolBar->AddTool( ID_GERBVIEW_ERASE_ALL, wxEmptyString,
                            KiScaledBitmap( delete_gerber_xpm, this ),
                            _( "Clear all layers" ) );

    m_mainToolBar->AddTool( ID_GERBVIEW_RELOAD_ALL, wxEmptyString,
                            KiScaledBitmap( reload2_xpm, this ),
                            _( "Reload all layers" ) );

    m_mainToolBar->AddTool( wxID_FILE, wxEmptyString, KiScaledBitmap( load_gerber_xpm, this ),
                            _( "Open Gerber file(s) on the current layer. Previous data will be deleted" ) );

    m_mainToolBar->AddTool( ID_GERBVIEW_LOAD_DRILL_FILE, wxEmptyString,
                            KiScaledBitmap( gerbview_drill_file_xpm, this ),
                            _( "Open Excellon drill file(s) on the current layer. Previous data will be deleted" ) );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( wxID_PRINT, wxEmptyString, KiScaledBitmap( print_button_xpm, this ),
                            _( "Print layers" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->Add( ACTIONS::zoomRedraw );
    m_mainToolBar->Add( ACTIONS::zoomInCenter );
    m_mainToolBar->Add( ACTIONS::zoomOutCenter );
    m_mainToolBar->Add( ACTIONS::zoomFitScreen );
    m_mainToolBar->Add( ACTIONS::zoomTool, ACTION_TOOLBAR::TOGGLE );


    KiScaledSeparator( m_mainToolBar, this );

    m_SelLayerBox = new GBR_LAYER_BOX_SELECTOR( m_mainToolBar,
                                                ID_TOOLBARH_GERBVIEW_SELECT_ACTIVE_LAYER,
                                                wxDefaultPosition, wxDefaultSize, 0, NULL );
    m_SelLayerBox->Resync();

    m_mainToolBar->AddControl( m_SelLayerBox );

    m_TextInfo = new wxTextCtrl( m_mainToolBar, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                 wxDefaultSize, wxTE_READONLY );
    m_mainToolBar->AddControl( m_TextInfo );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}


void GERBVIEW_FRAME::ReCreateAuxiliaryToolbar()
{
    wxWindowUpdateLocker dummy( this );
    wxStaticText* text;

    if( !m_auxiliaryToolBar )
        m_auxiliaryToolBar = new ACTION_TOOLBAR( this, ID_AUX_TOOLBAR,
                                                 wxDefaultPosition, wxDefaultSize,
                                                 KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // Creates box to display and choose components:
    if( !m_SelComponentBox )
    {
        m_SelComponentBox = new wxChoice( m_auxiliaryToolBar,
                                          ID_GBR_AUX_TOOLBAR_PCB_CMP_CHOICE );
        m_SelComponentBox->SetToolTip( _("Highlight items belonging to this component") );
        text = new wxStaticText( m_auxiliaryToolBar, wxID_ANY, _( "Cmp: ") );
        m_auxiliaryToolBar->AddControl( text );
        m_auxiliaryToolBar->AddControl( m_SelComponentBox );
        m_auxiliaryToolBar->AddSpacer( 5 );
    }

    // Creates choice box to display net names and highlight selected:
    if( !m_SelNetnameBox )
    {
        m_SelNetnameBox = new wxChoice( m_auxiliaryToolBar,
                                        ID_GBR_AUX_TOOLBAR_PCB_NET_CHOICE );
        m_SelNetnameBox->SetToolTip( _("Highlight items belonging to this net") );
        text = new wxStaticText( m_auxiliaryToolBar, wxID_ANY, _( "Net:" ) );
        m_auxiliaryToolBar->AddControl( text );
        m_auxiliaryToolBar->AddControl( m_SelNetnameBox );
        m_auxiliaryToolBar->AddSpacer( 5 );
    }

    // Creates choice box to display aperture attributes and highlight selected:
    if( !m_SelAperAttributesBox )
    {
        m_SelAperAttributesBox = new wxChoice( m_auxiliaryToolBar,
                                               ID_GBR_AUX_TOOLBAR_PCB_APERATTRIBUTES_CHOICE );
        m_SelAperAttributesBox->SetToolTip( _("Highlight items with this aperture attribute") );
        text = new wxStaticText( m_auxiliaryToolBar, wxID_ANY, _( "Attr:" ) );
        m_auxiliaryToolBar->AddControl( text );
        m_auxiliaryToolBar->AddControl( m_SelAperAttributesBox );
        m_auxiliaryToolBar->AddSpacer( 5 );
    }

    if( !m_DCodeSelector )
    {
        m_DCodeSelector = new DCODE_SELECTION_BOX( m_auxiliaryToolBar,
                                                   ID_TOOLBARH_GERBER_SELECT_ACTIVE_DCODE,
                                                   wxDefaultPosition, wxSize( 150, -1 ) );
        text = new wxStaticText( m_auxiliaryToolBar, wxID_ANY, _( "DCode:" ) );
        m_auxiliaryToolBar->AddControl( text );
        m_auxiliaryToolBar->AddControl( m_DCodeSelector );
    }

    if( !m_gridSelectBox )
    {
        KiScaledSeparator( m_auxiliaryToolBar, this );
        m_gridSelectBox = new wxChoice( m_auxiliaryToolBar, ID_ON_GRID_SELECT,
                                        wxDefaultPosition, wxDefaultSize, 0, nullptr );
        m_auxiliaryToolBar->AddControl( m_gridSelectBox );
    }

    if( !m_zoomSelectBox )
    {
        KiScaledSeparator( m_auxiliaryToolBar, this );
        m_zoomSelectBox = new wxChoice( m_auxiliaryToolBar, ID_ON_ZOOM_SELECT,
                                        wxDefaultPosition, wxDefaultSize, 0, nullptr );
        m_auxiliaryToolBar->AddControl( m_zoomSelectBox );
    }

    updateComponentListSelectBox();
    updateNetnameListSelectBox();
    updateAperAttributesSelectBox();
    updateDCodeSelectBox();
    updateGridSelectBox();
    updateZoomSelectBox();

    // combobox sizes can have changed: apply new best sizes
    auto item = m_auxiliaryToolBar->FindTool( ID_GBR_AUX_TOOLBAR_PCB_CMP_CHOICE );
    item->SetMinSize( m_SelComponentBox->GetBestSize() );

    item = m_auxiliaryToolBar->FindTool( ID_GBR_AUX_TOOLBAR_PCB_NET_CHOICE );
    item->SetMinSize( m_SelNetnameBox->GetBestSize() );

    item = m_auxiliaryToolBar->FindTool( ID_GBR_AUX_TOOLBAR_PCB_APERATTRIBUTES_CHOICE );
    item->SetMinSize( m_SelAperAttributesBox->GetBestSize() );

    item = m_auxiliaryToolBar->FindTool( ID_ON_GRID_SELECT );
    item->SetMinSize( m_gridSelectBox->GetBestSize() );

    item = m_auxiliaryToolBar->FindTool( ID_ON_ZOOM_SELECT );
    item->SetMinSize( m_zoomSelectBox->GetBestSize() );

    // after adding the buttons to the toolbar, must call Realize()
    m_auxiliaryToolBar->Realize();
}


void GERBVIEW_FRAME::ReCreateVToolbar()
{
    if( m_drawToolBar )
        return;

    m_drawToolBar = new ACTION_TOOLBAR( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                        KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    m_optionsToolBar->Add( ACTIONS::selectionTool, ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->AddSeparator();

    m_drawToolBar->Realize();
}


void GERBVIEW_FRAME::ReCreateOptToolbar()
{
    if( m_optionsToolBar )
        m_optionsToolBar->Clear();
    else
        m_optionsToolBar = new ACTION_TOOLBAR( this, ID_OPT_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                               KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    // TODO: these can be moved to the 'proper' vertical toolbar if and when there are
    // actual tools to put there. That, or I'll get around to implementing configurable
    // toolbars.
    m_optionsToolBar->Add( ACTIONS::selectionTool,                    ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::measureTool,                      ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->AddSeparator();
    m_optionsToolBar->Add( ACTIONS::toggleGrid,                       ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::togglePolarCoords,                ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::imperialUnits,                    ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::metricUnits,                      ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::toggleCursorStyle,                ACTION_TOOLBAR::TOGGLE );

    KiScaledSeparator( m_mainToolBar, this );
    m_optionsToolBar->Add( GERBVIEW_ACTIONS::flashedDisplayOutlines,  ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( GERBVIEW_ACTIONS::linesDisplayOutlines,    ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( GERBVIEW_ACTIONS::polygonsDisplayOutlines, ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( GERBVIEW_ACTIONS::negativeObjectDisplay,   ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( GERBVIEW_ACTIONS::dcodeDisplay,            ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( GERBVIEW_ACTIONS::toggleDiffMode,          ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::highContrastMode,                 ACTION_TOOLBAR::TOGGLE );

    // Tools to show/hide toolbars:
    KiScaledSeparator( m_mainToolBar, this );
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR,
                               wxEmptyString,
                               KiScaledBitmap( layers_manager_xpm, this ),
                               _( "Show/hide the layers manager toolbar" ),
                               wxITEM_CHECK );

    m_optionsToolBar->Realize();
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
    const char* units = GetUserUnits() == INCHES ? "mils" : "mm";
    double scale = GetUserUnits() == INCHES ? IU_PER_MILS : IU_PER_MM;

    for( int ii = 0; ii < TOOLS_MAX_COUNT; ii++ )
    {
        D_CODE* dcode = gerber->GetDCODE( ii + FIRST_DCODE );

        if( dcode == NULL )
            continue;

        if( !dcode->m_InUse && !dcode->m_Defined )
            continue;

        msg.Printf( "tool %d [%.3fx%.3f %s] %s",
                    dcode->m_Num_Dcode,
                    dcode->m_Size.y / scale, dcode->m_Size.x / scale,
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

        if( gerber == NULL )    // Graphic layer not yet used
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

        if( gerber == NULL )    // Graphic layer not yet used
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

        if( gerber == NULL )    // Graphic layer not yet used
            continue;

        if( gerber->GetDcodesCount() == 0 )
            continue;

        for( int ii = 0; ii < TOOLS_MAX_COUNT; ii++ )
        {
            D_CODE* aperture = gerber->GetDCODE( ii + FIRST_DCODE );

            if( aperture == NULL )
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

    aEvent.Enable( gerber != NULL );

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


void GERBVIEW_FRAME::SyncToolbars()
{
    KIGFX::GAL_DISPLAY_OPTIONS& galOpts = GetGalDisplayOptions();

#define TOGGLE_TOOL( toolbar, tool ) toolbar->Toggle( tool, IsCurrentTool( tool ) )

    TOGGLE_TOOL( m_mainToolBar, ACTIONS::zoomTool );
    m_mainToolBar->Refresh();

    TOGGLE_TOOL( m_optionsToolBar, ACTIONS::selectionTool );
    m_optionsToolBar->Toggle( ACTIONS::toggleGrid,             IsGridVisible() );
    m_optionsToolBar->Toggle( ACTIONS::metricUnits,            GetUserUnits() != INCHES );
    m_optionsToolBar->Toggle( ACTIONS::imperialUnits,          GetUserUnits() == INCHES );
    m_optionsToolBar->Toggle( ACTIONS::toggleCursorStyle,      !galOpts.m_fullscreenCursor );
    m_optionsToolBar->Toggle( GERBVIEW_ACTIONS::flashedDisplayOutlines,
                                                  !m_DisplayOptions.m_DisplayFlashedItemsFill );
    m_optionsToolBar->Toggle( GERBVIEW_ACTIONS::linesDisplayOutlines,
                                                  !m_DisplayOptions.m_DisplayLinesFill );
    m_optionsToolBar->Toggle( GERBVIEW_ACTIONS::polygonsDisplayOutlines,
                                                  !m_DisplayOptions.m_DisplayPolygonsFill );
    m_optionsToolBar->Toggle( GERBVIEW_ACTIONS::negativeObjectDisplay,
                                                  IsElementVisible( LAYER_NEGATIVE_OBJECTS ) );
    m_optionsToolBar->Toggle( GERBVIEW_ACTIONS::dcodeDisplay,
                                                  IsElementVisible( LAYER_DCODES ) );
    m_optionsToolBar->Toggle( GERBVIEW_ACTIONS::toggleDiffMode,m_DisplayOptions.m_DiffMode );
    m_optionsToolBar->Toggle( ACTIONS::highContrastMode,       m_DisplayOptions.m_HighContrastMode );
    m_optionsToolBar->Refresh();
}
