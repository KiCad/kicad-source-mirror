/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file toolbars_gerber.cpp
 * @brief Build tool bars
 */

#include <fctsys.h>

#include <common.h>
#include <gerbview.h>
#include <gerbview_frame.h>
#include <bitmaps.h>
#include <gerbview_id.h>
#include <hotkeys.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <gbr_layer_box_selector.h>
#include <DCodeSelectionbox.h>
#include <dialog_helpers.h>
#include <bitmaps.h>

#include <wx/wupdlock.h>

void GERBVIEW_FRAME::ReCreateHToolbar( void )
{
    wxString      msg;

    if( m_mainToolBar )
        m_mainToolBar->Clear();
    else
        m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
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

    KiScaledSeparator( m_mainToolBar, this );
    msg = AddHotkeyName( _( "Redraw view" ), GerbviewHokeysDescr, HK_ZOOM_REDRAW,  IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString, KiScaledBitmap( zoom_redraw_xpm, this ), msg );

    msg = AddHotkeyName( _( "Zoom in" ), GerbviewHokeysDescr, HK_ZOOM_IN,  IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiScaledBitmap( zoom_in_xpm, this ), msg );

    msg = AddHotkeyName( _( "Zoom out" ), GerbviewHokeysDescr, HK_ZOOM_OUT,  IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiScaledBitmap( zoom_out_xpm, this ), msg );

    msg = AddHotkeyName( _( "Zoom to fit" ), GerbviewHokeysDescr, HK_ZOOM_AUTO,  IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString,
                            KiScaledBitmap( zoom_fit_in_page_xpm, this ), msg );

    m_mainToolBar->AddTool( ID_ZOOM_SELECTION, wxEmptyString, KiScaledBitmap( zoom_area_xpm, this ),
                            _( "Zoom to selection" ), wxITEM_CHECK );

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
        m_auxiliaryToolBar = new wxAuiToolBar( this, ID_AUX_TOOLBAR, wxDefaultPosition,
                                               wxDefaultSize,
                                               KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // Creates box to display and choose components:
    if( !m_SelComponentBox )
    {
        m_SelComponentBox = new wxComboBox( m_auxiliaryToolBar,
                                          ID_GBR_AUX_TOOLBAR_PCB_CMP_CHOICE, wxEmptyString,
                                          wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY );
        m_SelComponentBox->SetToolTip( _("Highlight items belonging to this component") );
        text = new wxStaticText( m_auxiliaryToolBar, wxID_ANY, _( "Cmp: ") );
        m_auxiliaryToolBar->AddControl( text );
        m_auxiliaryToolBar->AddControl( m_SelComponentBox );
        m_auxiliaryToolBar->AddSpacer( 5 );
    }

    // Creates choice box to display net names and highlight selected:
    if( !m_SelNetnameBox )
    {
        m_SelNetnameBox = new wxComboBox( m_auxiliaryToolBar,
                                        ID_GBR_AUX_TOOLBAR_PCB_NET_CHOICE, wxEmptyString,
                                        wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY );
        m_SelNetnameBox->SetToolTip( _("Highlight items belonging to this net") );
        text = new wxStaticText( m_auxiliaryToolBar, wxID_ANY, _( "Net:" ) );
        m_auxiliaryToolBar->AddControl( text );
        m_auxiliaryToolBar->AddControl( m_SelNetnameBox );
        m_auxiliaryToolBar->AddSpacer( 5 );
    }

    // Creates choice box to display aperture attributes and highlight selected:
    if( !m_SelAperAttributesBox )
    {
        m_SelAperAttributesBox = new wxComboBox( m_auxiliaryToolBar,
                                               ID_GBR_AUX_TOOLBAR_PCB_APERATTRIBUTES_CHOICE, wxEmptyString,
                                               wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY );
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
        m_gridSelectBox = new wxComboBox( m_auxiliaryToolBar, ID_ON_GRID_SELECT, wxEmptyString,
                                          wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY );
        m_auxiliaryToolBar->AddControl( m_gridSelectBox );
    }

    if( !m_zoomSelectBox )
    {
        KiScaledSeparator( m_auxiliaryToolBar, this );
        m_zoomSelectBox = new wxComboBox( m_auxiliaryToolBar, ID_ON_ZOOM_SELECT, wxEmptyString,
                                          wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY );
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


void GERBVIEW_FRAME::ReCreateVToolbar( void )
{
    if( m_drawToolBar )
        return;

    m_drawToolBar = new wxAuiToolBar( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    // Set up toolbar
    m_drawToolBar->AddTool( ID_NO_TOOL_SELECTED, _( "Select item" ),
                            KiScaledBitmap( cursor_xpm, this ) );
    KiScaledSeparator( m_mainToolBar, this );

    m_drawToolBar->Realize();
}


void GERBVIEW_FRAME::ReCreateOptToolbar( void )
{
    if( m_optionsToolBar )
        m_optionsToolBar->Clear();
    else
        m_optionsToolBar = new wxAuiToolBar( this, ID_OPT_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                             KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    // TODO: these can be moved to the 'proper' vertical toolbar if and when there are
    // actual tools to put there. That, or I'll get around to implementing configurable
    // toolbars.
    m_optionsToolBar->AddTool( ID_NO_TOOL_SELECTED, wxEmptyString,
                               KiScaledBitmap( cursor_xpm, this ),
                               wxEmptyString, wxITEM_CHECK );

    if( IsGalCanvasActive() )
    {
        m_optionsToolBar->AddTool( ID_TB_MEASUREMENT_TOOL, wxEmptyString,
                                   KiScaledBitmap( measurement_xpm, this ),
                                   _( "Measure distance between two points" ),
                                   wxITEM_CHECK );
    }

    KiScaledSeparator( m_mainToolBar, this );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString,
                               KiScaledBitmap( grid_xpm, this ),
                               _( "Turn grid off" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLAR_COORD, wxEmptyString,
                               KiScaledBitmap( polar_coord_xpm, this ),
                               _( "Turn polar coordinates on" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               KiScaledBitmap( unit_inch_xpm, this ),
                               _( "Set units to inches" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               KiScaledBitmap( unit_mm_xpm, this ),
                               _( "Set units to millimeters" ), wxITEM_CHECK );

#ifndef __APPLE__
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiScaledBitmap( cursor_shape_xpm, this ),
                               _( "Change cursor shape" ), wxITEM_CHECK );
#else
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiScaledBitmap( cursor_shape_xpm, this ),
                               _( "Change cursor shape (not supported in Legacy Toolset)" ),
                               wxITEM_CHECK  );
#endif

    KiScaledSeparator( m_mainToolBar, this );
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_FLASHED_ITEMS_SKETCH, wxEmptyString,
                               KiScaledBitmap( pad_sketch_xpm, this ),
                               _( "Show flashed items in outline mode" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_LINES_SKETCH, wxEmptyString,
                               KiScaledBitmap( showtrack_xpm, this ),
                               _( "Show lines in outline mode" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLYGONS_SKETCH, wxEmptyString,
                               KiScaledBitmap( opt_show_polygon_xpm, this ),
                               _( "Show polygons in outline mode" ),
                               wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_NEGATIVE_ITEMS, wxEmptyString,
                               KiScaledBitmap( gerbview_show_negative_objects_xpm, this ),
                               _( "Show negatives objects in ghost color" ),
                               wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_DCODES, wxEmptyString,
                               KiScaledBitmap( show_dcodenumber_xpm, this ),
                               _( "Show dcode number" ), wxITEM_CHECK );

    // tools to select draw mode in GerbView, unused in GAL
    if( !IsGalCanvasActive() )
    {
        KiScaledSeparator( m_mainToolBar, this );
        m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GBR_MODE_0, wxEmptyString,
                                   KiScaledBitmap( gbr_select_mode0_xpm, this ),
                                   _( "Show layers in raw mode\n"
        "(could have problems with negative items when more than one gerber file is shown)" ),
                                   wxITEM_RADIO );
        m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GBR_MODE_1, wxEmptyString,
                                   KiScaledBitmap( gbr_select_mode1_xpm, this ),
                                   _( "Show layers in stacked mode\n"
                                      "(show negative items without artifacts)" ),
                                   wxITEM_RADIO );
        m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GBR_MODE_2, wxEmptyString,
                                   KiScaledBitmap( gbr_select_mode2_xpm, this ),
                                   _( "Show layers in transparency mode\n"
                                      "(show negative items without artifacts)" ),
                                   wxITEM_RADIO );
    }
    else
    {
        m_optionsToolBar->AddTool( ID_TB_OPTIONS_DIFF_MODE, wxEmptyString,
                                   KiScaledBitmap( gbr_select_mode2_xpm, this ),
                                   _( "Show layers in diff (compare) mode" ),
                                   wxITEM_CHECK );

        m_optionsToolBar->AddTool( ID_TB_OPTIONS_HIGH_CONTRAST_MODE, wxEmptyString,
                                   KiScaledBitmap( contrast_mode_xpm, this ),
                                   _( "Enable high contrast display mode" ),
                                   wxITEM_CHECK );

    }

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
    for( auto ii = full_list.begin(); ii != full_list.end(); ++ii )
    {
        m_SelComponentBox->Append( ii->first );
    }

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
    for( auto ii = full_list.begin(); ii != full_list.end(); ++ii )
    {
        m_SelNetnameBox->Append( ii->first );
    }

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


void GERBVIEW_FRAME::OnToggleCoordType( wxCommandEvent& aEvent )
{
    m_DisplayOptions.m_DisplayPolarCood = !m_DisplayOptions.m_DisplayPolarCood;

    UpdateStatusBar();
}


void GERBVIEW_FRAME::OnUpdateCoordType( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( m_DisplayOptions.m_DisplayPolarCood );

    if( m_optionsToolBar )
        m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_POLAR_COORD,
                                            m_DisplayOptions.m_DisplayPolarCood ?
                                            _( "Turn on rectangular coordinates" ) :
                                            _( "Turn on polar coordinates" ) );
}


void GERBVIEW_FRAME::OnUpdateFlashedItemsDrawMode( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( !m_DisplayOptions.m_DisplayFlashedItemsFill );

    if( m_optionsToolBar )
        m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_FLASHED_ITEMS_SKETCH,
                                            m_DisplayOptions.m_DisplayFlashedItemsFill ?
                                            _( "Show flashed items in outline mode" ) :
                                            _( "Show flashed items in fill mode" ) );
}


void GERBVIEW_FRAME::OnUpdateLineDrawMode( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( !m_DisplayOptions.m_DisplayLinesFill );

    if( m_optionsToolBar )
        m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_LINES_SKETCH,
                                            m_DisplayOptions.m_DisplayFlashedItemsFill ?
                                            _( "Show lines in outline mode" ) :
                                            _( "Show lines in fill mode" ) );
}


void GERBVIEW_FRAME::OnUpdatePolygonDrawMode( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( !m_DisplayOptions.m_DisplayPolygonsFill );

    if( m_optionsToolBar )
        m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_POLYGONS_SKETCH,
                                            m_DisplayOptions.m_DisplayFlashedItemsFill ?
                                            _( "Show polygons in outline mode" ) :
                                            _( "Show polygons in fill mode" ) );
}


void GERBVIEW_FRAME::OnUpdateShowDCodes( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( IsElementVisible( LAYER_DCODES ) );

    if( m_optionsToolBar )
        m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_DCODES,
                                            IsElementVisible( LAYER_DCODES ) ?
                                            _( "Hide DCodes" ) : _( "Show DCodes" ) );
}


void GERBVIEW_FRAME::OnUpdateShowNegativeItems( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( IsElementVisible( LAYER_NEGATIVE_OBJECTS ) );

    if( m_optionsToolBar )
        m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_NEGATIVE_ITEMS,
                                            IsElementVisible( LAYER_NEGATIVE_OBJECTS ) ?
                                            _( "Show negative objects in normal color" ) :
                                            _( "Show negative objects in ghost color" ) );
}


void GERBVIEW_FRAME::OnUpdateDiffMode( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( m_DisplayOptions.m_DiffMode );

    if( m_optionsToolBar )
        m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_DIFF_MODE,
                                            m_DisplayOptions.m_DiffMode ?
                                            _( "Show layers in normal mode" ) :
                                            _( "Show layers in differential mode" ) );
}


void GERBVIEW_FRAME::OnUpdateHighContrastMode( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( m_DisplayOptions.m_HighContrastMode );

    if( m_optionsToolBar )
        m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_HIGH_CONTRAST_MODE,
                                            m_DisplayOptions.m_HighContrastMode ?
                                            _( "Disable high contrast mode" ) :
                                            _( "Enable high contrast mode" ) );
}


void GERBVIEW_FRAME::OnUpdateShowLayerManager( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( m_show_layer_manager_tools );

    if( m_optionsToolBar )
    {
        if( m_show_layer_manager_tools )
            m_optionsToolBar->SetToolShortHelp( aEvent.GetId(), _( "Hide layers manager" ) );
        else
            m_optionsToolBar->SetToolShortHelp( aEvent.GetId(), _( "Show layers manager" ) );
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
    {
        m_SelLayerBox->SetSelection( GetActiveLayer() );
    }
}
