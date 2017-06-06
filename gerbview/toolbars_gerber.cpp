/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <class_gerber_file_image.h>
#include <class_gerber_file_image_list.h>
#include <class_gbr_layer_box_selector.h>
#include <class_DCodeSelectionbox.h>
#include <dialog_helpers.h>
#include <bitmaps.h>

#include <wx/wupdlock.h>

void GERBVIEW_FRAME::ReCreateHToolbar( void )
{
    wxString      msg;

    if( m_mainToolBar != NULL )
        return;

    m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // Set up toolbar
    m_mainToolBar->AddTool( ID_GERBVIEW_ERASE_ALL, wxEmptyString,
                            KiBitmap( gerbview_clear_layers_xpm ),
                            _( "Erase all layers" ) );

    m_mainToolBar->AddTool( wxID_FILE, wxEmptyString, KiBitmap( gerber_file_xpm ),
                            _( "Load a new Gerber file on the current layer. Previous data will be deleted" ) );

    m_mainToolBar->AddTool( ID_GERBVIEW_LOAD_DRILL_FILE, wxEmptyString,
                            KiBitmap( gerbview_drill_file_xpm ),
                            _( "Load an excellon drill file on the current layer. Previous data will be deleted" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_GERBVIEW_SET_PAGE_BORDER, wxEmptyString, KiBitmap( sheetset_xpm ),
                            _( "Show/hide frame reference and select paper size for printing" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( wxID_PRINT, wxEmptyString, KiBitmap( print_button_xpm ),
                            _( "Print layers" ) );

    m_mainToolBar->AddSeparator();
    msg = AddHotkeyName( _( "Redraw view" ), GerbviewHokeysDescr, HK_ZOOM_REDRAW,  IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString, KiBitmap( zoom_redraw_xpm ), msg );

    msg = AddHotkeyName( _( "Zoom in" ), GerbviewHokeysDescr, HK_ZOOM_IN,  IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiBitmap( zoom_in_xpm ), msg );

    msg = AddHotkeyName( _( "Zoom out" ), GerbviewHokeysDescr, HK_ZOOM_OUT,  IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiBitmap( zoom_out_xpm ), msg );

    msg = AddHotkeyName( _( "Zoom auto" ), GerbviewHokeysDescr, HK_ZOOM_AUTO,  IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString, KiBitmap( zoom_fit_in_page_xpm ), msg );

    m_mainToolBar->AddTool( ID_ZOOM_SELECTION, wxEmptyString, KiBitmap( zoom_area_xpm ),
                               _( "Zoom to selection" ), wxITEM_CHECK );

    m_mainToolBar->AddSeparator();

    m_SelLayerBox = new GBR_LAYER_BOX_SELECTOR( m_mainToolBar,
                                ID_TOOLBARH_GERBVIEW_SELECT_ACTIVE_LAYER,
                                wxDefaultPosition, wxDefaultSize, 0,NULL);
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

    if( m_auxiliaryToolBar )
    {
        updateComponentListSelectBox();
        updateNetnameListSelectBox();
        updateAperAttributesSelectBox();
        updateDCodeSelectBox();

        // combobox sizes can have changed: apply new best sizes
        wxSize size;
        size = m_SelComponentBox->GetBestSize();
        size.x = std::max( size.x, 100 );
        m_SelComponentBox->SetMinSize( size );

        size = m_SelNetnameBox->GetBestSize();
        size.x = std::max( size.x, 100 );
        m_SelNetnameBox->SetMinSize( size );

        size = m_SelAperAttributesBox->GetBestSize();
        size.x = std::max( size.x, 100 );
        m_SelAperAttributesBox->SetMinSize( size );

        size = m_DCodeSelector->GetBestSize();
        size.x = std::max( size.x, 100 );
        m_DCodeSelector->SetMinSize( size );

        m_auimgr.Update();
        return;
    }

    m_auxiliaryToolBar = new wxAuiToolBar( this, ID_AUX_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                           wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // Creates box to display and choose components:
    wxStaticText* text = new wxStaticText( m_auxiliaryToolBar, wxID_ANY, _("Cmp:") );
    m_auxiliaryToolBar->AddControl( text );
    m_SelComponentBox = new wxChoice( m_auxiliaryToolBar,
                                      ID_GBR_AUX_TOOLBAR_PCB_CMP_CHOICE );
    m_SelComponentBox->SetToolTip( _("Select a component and highlight items belonging to this component") );
    updateComponentListSelectBox();
    m_auxiliaryToolBar->AddControl( m_SelComponentBox );
    m_auxiliaryToolBar->AddSeparator();

    // Creates choice box to display net names and highlight selected:
    text = new wxStaticText( m_auxiliaryToolBar, wxID_ANY, _("Net:") );
    m_auxiliaryToolBar->AddControl( text );
    m_SelNetnameBox = new wxChoice( m_auxiliaryToolBar,
                                    ID_GBR_AUX_TOOLBAR_PCB_NET_CHOICE );
    m_SelNetnameBox->SetToolTip( _("Select a net name and highlight graphic items belonging to this net") );
    m_auxiliaryToolBar->AddControl( m_SelNetnameBox );
    updateNetnameListSelectBox();
    m_auxiliaryToolBar->AddSeparator();

    // Creates choice box to display aperture attributes and highlight selected:
    text = new wxStaticText( m_auxiliaryToolBar, wxID_ANY, _("Attr:") );
    m_auxiliaryToolBar->AddControl( text );
    m_SelAperAttributesBox = new wxChoice( m_auxiliaryToolBar,
                                      ID_GBR_AUX_TOOLBAR_PCB_APERATTRIBUTES_CHOICE );
    m_SelAperAttributesBox->SetToolTip( _("Select an aperture attribute and highlight graphic items having this attribute") );
    m_auxiliaryToolBar->AddControl( m_SelAperAttributesBox );
    updateAperAttributesSelectBox();

    m_auxiliaryToolBar->AddSeparator();
    text = new wxStaticText( m_auxiliaryToolBar, wxID_ANY, _("DCode:") );
    m_auxiliaryToolBar->AddControl( text );
    m_DCodeSelector = new DCODE_SELECTION_BOX( m_auxiliaryToolBar,
                                               ID_TOOLBARH_GERBER_SELECT_ACTIVE_DCODE,
                                               wxDefaultPosition, wxSize( 150, -1 ) );
    updateDCodeSelectBox();
    m_auxiliaryToolBar->AddControl( m_DCodeSelector );

    // after adding the buttons to the toolbar, must call Realize()
    m_auxiliaryToolBar->Realize();
}


void GERBVIEW_FRAME::ReCreateVToolbar( void )
{
    if( m_drawToolBar )
        return;

    m_drawToolBar = new wxAuiToolBar( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_VERTICAL );

    // Set up toolbar
    m_drawToolBar->AddTool( ID_NO_TOOL_SELECTED, wxEmptyString, KiBitmap( cursor_xpm ) );
    m_drawToolBar->AddSeparator();

    m_drawToolBar->Realize();
}


void GERBVIEW_FRAME::ReCreateOptToolbar( void )
{
    if( m_optionsToolBar )
        return;

    // creation of tool bar options
    m_optionsToolBar = new wxAuiToolBar( this, ID_OPT_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                         wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_VERTICAL );

    // TODO: these can be moved to the 'proper' vertical toolbar if and when there are
    // actual tools to put there. That, or I'll get around to implementing configurable
    // toolbars.
    m_optionsToolBar->AddTool( ID_NO_TOOL_SELECTED, wxEmptyString, KiBitmap( cursor_xpm ),
                               wxEmptyString, wxITEM_CHECK );

    m_optionsToolBar->AddSeparator();

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString, KiBitmap( grid_xpm ),
                               _( "Turn grid off" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLAR_COORD, wxEmptyString,
                               KiBitmap( polar_coord_xpm ),
                               _( "Turn polar coordinate on" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               KiBitmap( unit_inch_xpm ),
                               _( "Set units to inches" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               KiBitmap( unit_mm_xpm ),
                               _( "Set units to millimeters" ), wxITEM_CHECK );

#ifndef __APPLE__
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiBitmap( cursor_shape_xpm ),
                               _( "Change cursor shape" ), wxITEM_CHECK );
#endif // !__APPLE__

    m_optionsToolBar->AddSeparator();
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_FLASHED_ITEMS_SKETCH, wxEmptyString,
                               KiBitmap( pad_sketch_xpm ),
                               _( "Show spots in sketch mode" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_LINES_SKETCH, wxEmptyString,
                               KiBitmap( showtrack_xpm ),
                               _( "Show lines in sketch mode" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLYGONS_SKETCH, wxEmptyString,
                               KiBitmap( opt_show_polygon_xpm ),
                               _( "Show polygons in sketch mode" ),
                               wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_NEGATIVE_ITEMS, wxEmptyString,
                               KiBitmap( gerbview_show_negative_objects_xpm ),
                               _( "Show negatives objects in ghost color" ),
                               wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_DCODES, wxEmptyString,
                               KiBitmap( show_dcodenumber_xpm ),
                               _( "Show dcode number" ), wxITEM_CHECK );

    // tools to select draw mode in GerbView
    m_optionsToolBar->AddSeparator();
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GBR_MODE_0, wxEmptyString,
                               KiBitmap( gbr_select_mode0_xpm ),
                               _( "Show layers in raw mode \
(could have problems with negative items when more than one gerber file is shown)" ),
                               wxITEM_CHECK );
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GBR_MODE_1, wxEmptyString,
                               KiBitmap( gbr_select_mode1_xpm ),
                               _( "Show layers in stacked mode \
(show negative items without artifacts, sometimes slow)" ),
                               wxITEM_CHECK );
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GBR_MODE_2, wxEmptyString,
                               KiBitmap( gbr_select_mode2_xpm ),
                               _( "Show layers in transparency mode \
(show negative items without artifacts, sometimes slow)" ),
                               wxITEM_CHECK );

    // Tools to show/hide toolbars:
    m_optionsToolBar->AddSeparator();
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR,
                               wxEmptyString,
                               KiBitmap( layers_manager_xpm ),
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

    int layer = getActiveLayer();
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
    const char* units = g_UserUnit == INCHES ? "mils" : "mm";
    double scale = g_UserUnit == INCHES ? IU_PER_MILS : IU_PER_MM;

    for( int ii = 0; ii < TOOLS_MAX_COUNT; ii++ )
    {
        D_CODE* dcode = gerber->GetDCODE( ii + FIRST_DCODE, false );

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
            D_CODE* aperture = gerber->GetDCODE( ii + FIRST_DCODE, false );

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


void GERBVIEW_FRAME::OnUpdateCoordType( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( m_DisplayOptions.m_DisplayPolarCood );
}

void GERBVIEW_FRAME::OnUpdateFlashedItemsDrawMode( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( !m_DisplayOptions.m_DisplayFlashedItemsFill );
}


void GERBVIEW_FRAME::OnUpdateLinesDrawMode( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( !m_DisplayOptions.m_DisplayLinesFill );
}


void GERBVIEW_FRAME::OnUpdatePolygonsDrawMode( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( !m_DisplayOptions.m_DisplayPolygonsFill );
}


void GERBVIEW_FRAME::OnUpdateShowDCodes( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( IsElementVisible( LAYER_DCODES ) );
}


void GERBVIEW_FRAME::OnUpdateShowNegativeItems( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( IsElementVisible( LAYER_NEGATIVE_OBJECTS ) );
}


void GERBVIEW_FRAME::OnUpdateShowLayerManager( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( m_show_layer_manager_tools );

    if( m_optionsToolBar )
    {
        if( m_show_layer_manager_tools )
            m_optionsToolBar->SetToolShortHelp( aEvent.GetId(), _("Hide layers manager" ) );
        else
            m_optionsToolBar->SetToolShortHelp( aEvent.GetId(), _("Show layers manager" ) );
    }
}


void GERBVIEW_FRAME::OnUpdateSelectDCode( wxUpdateUIEvent& aEvent )
{
    if( !m_DCodeSelector )
        return;

    int layer = getActiveLayer();
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
    if( m_SelLayerBox->GetSelection() != getActiveLayer() )
    {
        m_SelLayerBox->SetSelection( getActiveLayer() );
    }
}
