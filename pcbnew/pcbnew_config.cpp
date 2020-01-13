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

#include <fctsys.h>
#include <kiface_i.h>
#include <project.h>
#include <confirm.h>
#include <gestfich.h>
#include <macros.h>
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <plotter.h>
#include <ws_painter.h>
#include <panel_hotkeys_editor.h>
#include <panel_pcbnew_settings.h>
#include <panel_pcbnew_display_options.h>
#include <panel_pcbnew_action_plugins.h>
#include <fp_lib_table.h>
#include <ws_data_model.h>
#include <class_board.h>
#include <class_module.h>
#include <pcbplot.h>
#include <footprint_viewer_frame.h>
#include <invoke_pcb_dialog.h>
#include <wildcards_and_files_ext.h>
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

    book->AddPage( new wxPanel( book ), _( "Pcbnew" ) );
    book->AddSubPage( new PANEL_PCBNEW_DISPLAY_OPTIONS( this, aParent ), _( "Display Options" ) );
    book->AddSubPage( new PANEL_PCBNEW_SETTINGS( this, aParent ), _( "Editing Options" ) );
#if defined(KICAD_SCRIPTING) && defined(KICAD_SCRIPTING_ACTION_MENU)
    book->AddSubPage( new PANEL_PCBNEW_ACTION_PLUGINS( this, aParent ), _( "Action Plugins" ) );
#endif

    aHotkeysPanel->AddHotKeys( GetToolManager() );
}


bool PCB_EDIT_FRAME::LoadProjectSettings()
{
    wxLogDebug( wxT( "Loading project '%s' settings." ), GetChars( Prj().GetProjectFullName() ) );

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
        wxFileDialog dlg( this, _( "Save Project File" ), fn.GetPath(), fn.GetFullName(),
                          ProjectFileWildcard(), wxFD_SAVE | wxFD_CHANGE_DIR );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        fn = dlg.GetPath();
    }

    wxString pro_name = fn.GetFullPath();

    Prj().ConfigSave( Kiface().KifaceSearch(), GROUP_PCB, GetProjectFileParameters(), pro_name );
}


std::vector<PARAM_CFG*>& PCB_EDIT_FRAME::GetProjectFileParameters()
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
