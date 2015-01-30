/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file modeditoptions.cpp
 * @brief Pcbnew footprint (module) editor options.
 */

#include <fctsys.h>
#include <class_drawpanel.h>

#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <module_editor_frame.h>
#include <class_board_design_settings.h>
#include <layers_id_colors_and_visibility.h>

#include <pcbnew_id.h>


void FOOTPRINT_EDIT_FRAME::OnSelectOptionToolbar( wxCommandEvent& event )
{
    int        id = event.GetId();
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)GetDisplayOptions();
    bool state = m_optionsToolBar->GetToolToggled( id );

    switch( id )
    {
    case ID_TB_OPTIONS_SHOW_PADS_SKETCH:
        displ_opts->m_DisplayPadFill = !state;
        m_canvas->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_VIAS_SKETCH:
        displ_opts->m_DisplayViaFill = !state;
        m_canvas->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH:
        displ_opts->m_DisplayModText = state ? SKETCH : FILLED;
        m_canvas->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH:
        displ_opts->m_DisplayModEdge = state ? SKETCH : FILLED;
        m_canvas->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE:
        displ_opts->m_ContrastModeDisplay = state;
        m_canvas->Refresh( );
        break;

    default:
        wxMessageBox( wxT( "FOOTPRINT_EDIT_FRAME::OnSelectOptionToolbar error" ) );
        break;
    }
}


PARAM_CFG_ARRAY& FOOTPRINT_EDIT_FRAME::GetConfigurationSettings()
{
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)GetDisplayOptions();
    BOARD_DESIGN_SETTINGS& settings = GetDesignSettings();

    if( m_configSettings.empty() )
    {
        // Display options:
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "FpEditorUnits" ),
                                                       (int*)&g_UserUnit, MILLIMETRES ) );
        m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "FpEditorDisplayPolarCoords" ),
                                                        &displ_opts->m_DisplayPolarCood, false ) );
        m_configSettings.push_back( new PARAM_CFG_BOOL( true, wxT( "FpEditorPadDisplayMode" ),
                                                        &displ_opts->m_DisplayPadFill, true ) );
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "FpEditorGraphicLinesDisplayMode" ),
                                                       &displ_opts->m_DisplayModEdge, FILLED, 0, 2 ) );
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "FpEditorTextsDisplayMode" ),
                                                       &displ_opts->m_DisplayModText, FILLED, 0, 2 ) );
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "FpEditorTextsDisplayMode" ),
                                                       &displ_opts->m_DisplayModText, FILLED, 0, 2 ) );
        m_configSettings.push_back( new PARAM_CFG_WXSTRING( true, wxT( "FpEditorTextsRefDefaultText" ),
                                                       &settings.m_RefDefaultText, wxT( "REF**" ) ) );

        // design settings
        m_configSettings.push_back( new PARAM_CFG_INT_WITH_SCALE( true, wxT( "FpEditorGrlineWidth" ),
                                                       &settings.m_ModuleSegmentWidth,
                                                       Millimeter2iu( 0.15 ),
                                                       Millimeter2iu( 0.01 ), Millimeter2iu( 100.0 ),
                                                       NULL, 1/IU_PER_MM ) );
        m_configSettings.push_back( new PARAM_CFG_INT_WITH_SCALE( true, wxT( "FpEditorTextsDefaultSizeH" ),
                                                       &settings.m_ModuleTextSize.x,
                                                       Millimeter2iu( 1.5 ),
                                                       Millimeter2iu( 0.01 ), Millimeter2iu( 100.0 ),
                                                       NULL, 1/IU_PER_MM ) );
        m_configSettings.push_back( new PARAM_CFG_INT_WITH_SCALE( true, wxT( "FpEditorTextsDefaultSizeV" ),
                                                       &settings.m_ModuleTextSize.y,
                                                       Millimeter2iu( 1.5 ),
                                                       Millimeter2iu(0.01), Millimeter2iu( 100.0 ),
                                                       NULL, 1/IU_PER_MM ) );
        m_configSettings.push_back( new PARAM_CFG_INT_WITH_SCALE( true, wxT( "FpEditorTextsDefaultThickness" ),
                                                       &settings.m_ModuleTextWidth,
                                                       Millimeter2iu( 0.15 ),
                                                       Millimeter2iu( 0.01 ), Millimeter2iu( 20.0 ),
                                                       NULL, 1/IU_PER_MM ) );

        m_configSettings.push_back( new PARAM_CFG_WXSTRING( true,
                                        wxT( "FpEditorRefDefaultText" ),
                                        &settings.m_RefDefaultText, wxT( "REF**" ) ) );
        m_configSettings.push_back( new PARAM_CFG_BOOL( true,
                                        wxT( "FpEditorRefDefaultVisibility" ),
                                        &settings.m_RefDefaultVisibility, true ) );
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "FpEditorRefDefaultLayer" ),
                                        &settings.m_RefDefaultlayer,
                                        int( F_SilkS ), int( F_SilkS ), int( F_Fab ) ) );

        m_configSettings.push_back( new PARAM_CFG_WXSTRING( true, wxT( "FpEditorValueDefaultText" ),
                                                       &settings.m_ValueDefaultText, wxT( "" ) ) );
        m_configSettings.push_back( new PARAM_CFG_BOOL( true,
                                        wxT( "FpEditorValueDefaultVisibility" ),
                                        &settings.m_ValueDefaultVisibility, true ) );
        m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "FpEditorValueDefaultLayer" ),
                                        &settings.m_ValueDefaultlayer,
                                        int( F_Fab ), int( F_SilkS ), int( F_Fab ) ) );
    }

    return m_configSettings;
}

