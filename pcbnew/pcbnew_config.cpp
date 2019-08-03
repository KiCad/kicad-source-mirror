/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcbnew_config.cpp
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <project.h>
#include <pcb_draw_panel_gal.h>
#include <confirm.h>
#include <gestfich.h>
#include <xnode.h>
#include <common.h>
#include <macros.h>
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <plotter.h>
#include <ws_painter.h>
#include <panel_hotkeys_editor.h>
#include <panel_pcbnew_settings.h>
#include <panel_pcbnew_display_options.h>
#include <panel_pcbnew_action_plugins.h>
#include <panel_hotkeys_editor.h>
#include <fp_lib_table.h>
#include <ws_draw_item.h>
#include <ws_data_model.h>
#include <class_board.h>
#include <class_module.h>
#include <pcbplot.h>
#include <pcbnew_id.h>
#include <footprint_viewer_frame.h>
#include <invoke_pcb_dialog.h>
#include <wildcards_and_files_ext.h>
#include <view/view.h>
#include <widgets/paged_dialog.h>


void PCB_EDIT_FRAME::On3DShapeLibWizard( wxCommandEvent& event )
{
#ifdef BUILD_GITHUB_PLUGIN
    Invoke3DShapeLibsDownloaderWizard( this );
#endif
}


void PCB_EDIT_FRAME::InstallPreferences( PAGED_DIALOG* aParent,
                                         PANEL_HOTKEYS_EDITOR* aHotkeysPanel )
{
    wxTreebook* book = aParent->GetTreebook();

    book->AddPage( new PANEL_PCBNEW_SETTINGS( this, aParent ), _( "Pcbnew" ) );
    book->AddSubPage( new PANEL_PCBNEW_DISPLAY_OPTIONS( this, aParent ), _( "Display Options" ) );
#if defined(KICAD_SCRIPTING) && defined(KICAD_SCRIPTING_ACTION_MENU)
    book->AddSubPage( new PANEL_PCBNEW_ACTION_PLUGINS( this, aParent ), _( "Action Plugins" ) );
#endif
    
    aHotkeysPanel->AddHotKeys( GetToolManager() );
}


bool PCB_EDIT_FRAME::LoadProjectSettings()
{
    wxLogDebug( wxT( "Loading project '%s' settings." ),
            GetChars( Prj().GetProjectFullName() ) );

    bool rc = Prj().ConfigLoad( Kiface().KifaceSearch(), GROUP_PCB, GetProjectFileParameters() );

    // Load the page layout decr file, from the filename stored in
    // BASE_SCREEN::m_PageLayoutDescrFileName, read in config project file
    // If empty, or not existing, the default descr is loaded
    WS_DATA_MODEL& pglayout = WS_DATA_MODEL::GetTheInstance();
    wxString filename = WS_DATA_MODEL::MakeFullFileName( BASE_SCREEN::m_PageLayoutDescrFileName,
                                                         Prj().GetProjectPath() );

    pglayout.SetPageLayout( filename );

    return rc;
}


void PCB_EDIT_FRAME::SaveProjectSettings( bool aAskForSave )
{
    wxFileName fn = Prj().GetProjectFullName();

    if( aAskForSave )
    {
        wxFileDialog dlg( this, _( "Save Project File" ),
                          fn.GetPath(), fn.GetFullName(),
                          ProjectFileWildcard(), wxFD_SAVE | wxFD_CHANGE_DIR );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        fn = dlg.GetPath();
    }

    wxString pro_name = fn.GetFullPath();

    Prj().ConfigSave( Kiface().KifaceSearch(), GROUP_PCB, GetProjectFileParameters(), pro_name );
}


PARAM_CFG_ARRAY& PCB_EDIT_FRAME::GetProjectFileParameters()
{
    m_projectFileParams.clear();

    // This one cannot be cached because some settings are going to/from the BOARD,
    // so pointers into that cannot be saved for long.

    m_projectFileParams.push_back( new PARAM_CFG_FILENAME( wxT( "PageLayoutDescrFile" ),
                                                           &BASE_SCREEN::m_PageLayoutDescrFileName ) );

    m_projectFileParams.push_back( new PARAM_CFG_FILENAME( wxT( "LastNetListRead" ),
                                                           &m_lastPath[ LAST_PATH_NETLIST ] ) );

    m_projectFileParams.push_back( new PARAM_CFG_FILENAME( wxT( "LastSTEPExportPath" ),
                                                           &m_lastPath[ LAST_PATH_STEP ] ) );

    m_projectFileParams.push_back( new PARAM_CFG_FILENAME( wxT( "LastIDFExportPath" ),
                                                           &m_lastPath[ LAST_PATH_IDF ] ) );

    m_projectFileParams.push_back( new PARAM_CFG_FILENAME( wxT( "LastVRMLExportPath" ),
                                                           &m_lastPath[ LAST_PATH_VRML ] ) );

    m_projectFileParams.push_back( new PARAM_CFG_FILENAME( wxT( "LastSpecctraDSNExportPath" ),
                                                           &m_lastPath[ LAST_PATH_SPECCTRADSN ] ) );

    m_projectFileParams.push_back( new PARAM_CFG_FILENAME( wxT( "LastGenCADExportPath" ),
                                                           &m_lastPath[ LAST_PATH_GENCAD ] ) );

    GetBoard()->GetDesignSettings().AppendConfigs( GetBoard(), &m_projectFileParams);

    return m_projectFileParams;
}


PARAM_CFG_ARRAY& PCB_EDIT_FRAME::GetConfigurationSettings()
{
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)GetDisplayOptions();

    if( m_configParams.empty() )
    {
        m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "DisplayPolarCoords" ),
                                                        &m_PolarCoords, false ) );
        // Display options and modes:
        m_configParams.push_back( new PARAM_CFG_INT( true, wxT( "ShowNetNamesMode" ),
                                                     &displ_opts->m_DisplayNetNamesMode, 3, 0, 3 ) );
        m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "DisplayTrackFilled" ),
                                                      &displ_opts->m_DisplayPcbTrackFill, true ) );
        m_configParams.push_back( new PARAM_CFG_INT( true, wxT( "TrackDisplayClearance" ),
                                                     (int*) &displ_opts->m_ShowTrackClearanceMode,
                                                     PCB_DISPLAY_OPTIONS::SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS ) );
        m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "PadFill" ),
                                                      &displ_opts->m_DisplayPadFill, true ) );
        m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "ViaFill" ),
                                                      &displ_opts->m_DisplayViaFill, true ) );
        m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "PadAffG" ),
                                                      &displ_opts->m_DisplayPadIsol, true ) );
        m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "PadSNum" ),
                                                      &displ_opts->m_DisplayPadNum, true ) );
        m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "ModAffC" ),
                                                     &displ_opts->m_DisplayModEdgeFill, FILLED ) );
        m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "ModAffT" ),
                                                     &displ_opts->m_DisplayModTextFill, FILLED ) );
        m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "PcbAffT" ),
                                                     &displ_opts->m_DisplayDrawItemsFill, FILLED ) );
        m_configParams.push_back( new PARAM_CFG_INT( true, wxT( "PcbShowZonesMode" ),
                                                     &displ_opts->m_DisplayZonesMode, 0, 0, 2 ) );
        m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "CurvedRatsnestLines" ),
                                                      &displ_opts->m_DisplayRatsnestLinesCurved, false ) );
        m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "ShowRatsnestLines" ),
                                                      &displ_opts->m_ShowGlobalRatsnest, true) );
        m_configParams.push_back( new PARAM_CFG_BOOL( true, wxT( "ShowRatsnestModuleLines" ),
                                                      &displ_opts->m_ShowModuleRatsnest, true) );

        // Miscellaneous:
        m_configParams.push_back( new PARAM_CFG_INT( true, wxT( "RotationAngle" ),
                                                     &m_rotationAngle, 900, 1, 900 ) );
        m_configParams.push_back( new PARAM_CFG_INT( true, wxT( "MaxLnkS" ),
                                                     &displ_opts->m_MaxLinksShowed, 3, 0, 15 ) );
    }

    return m_configParams;
}
