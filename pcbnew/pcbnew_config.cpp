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
#include <confirm.h>
#include <gestfich.h>
#include <macros.h>
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <plotter.h>
#include <panel_hotkeys_editor.h>
#include <panel_edit_options.h>
#include <panel_pcbnew_color_settings.h>
#include <panel_display_options.h>
#include <panel_pcbnew_action_plugins.h>
#include <tool/tool_manager.h>
#include <tools/selection_tool.h>
#include <fp_lib_table.h>
#include <ws_data_model.h>
#include <class_board.h>
#include <class_module.h>
#include <pcbplot.h>
#include <pcb_painter.h>
#include <footprint_viewer_frame.h>
#include <invoke_pcb_dialog.h>
#include <wildcards_and_files_ext.h>
#include <widgets/paged_dialog.h>
#include <widgets/panel_selection_filter.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>


void PCB_EDIT_FRAME::On3DShapeLibWizard( wxCommandEvent& event )
{
#ifdef BUILD_GITHUB_PLUGIN
    Invoke3DShapeLibsDownloaderWizard( this );
#endif
}


void PCB_EDIT_FRAME::InstallPreferences( PAGED_DIALOG* aParent,
                                         PANEL_HOTKEYS_EDITOR* aHotkeysPanel )
{
    PAGED_TREEBOOK* book = aParent->GetTreebook();

    book->AddGroupEntry( _( "Pcbnew" ) );
    book->AddSubPage( new PANEL_DISPLAY_OPTIONS( this, aParent ), _( "Display Options" ) );
    book->AddSubPage( new PANEL_PCBNEW_COLOR_SETTINGS( this, book ), _( "Colors" ) );
    book->AddSubPage( new PANEL_EDIT_OPTIONS( this, aParent ), _( "Editing Options" ) );
#if defined(KICAD_SCRIPTING) && defined(KICAD_SCRIPTING_ACTION_MENU)
    book->AddSubPage( new PANEL_PCBNEW_ACTION_PLUGINS( this, aParent ), _( "Action Plugins" ) );
#endif

    aHotkeysPanel->AddHotKeys( GetToolManager() );
}


bool PCB_EDIT_FRAME::LoadProjectSettings()
{
    wxLogDebug( wxT( "Loading project '%s' settings." ), GetChars( Prj().GetProjectFullName() ) );

    PROJECT_FILE&           project       = Prj().GetProjectFile();
    PROJECT_LOCAL_SETTINGS& localSettings = Prj().GetLocalSettings();

    BASE_SCREEN::m_PageLayoutDescrFileName = project.m_BoardPageLayoutDescrFile;

    // Load the page layout decr file, from the filename stored in
    // BASE_SCREEN::m_PageLayoutDescrFileName, read in config project file
    // If empty, or not existing, the default descr is loaded
    WS_DATA_MODEL& pglayout = WS_DATA_MODEL::GetTheInstance();
    wxString filename = WS_DATA_MODEL::MakeFullFileName( BASE_SCREEN::m_PageLayoutDescrFileName,
                                                         Prj().GetProjectPath() );

    pglayout.SetPageLayout( filename );

    KIGFX::PCB_RENDER_SETTINGS* rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
            GetCanvas()->GetView()->GetPainter()->GetSettings() );

    NETINFO_LIST& nets = GetBoard()->GetNetInfo();
    std::set<int> hiddenNets;

    for( const wxString& hidden : localSettings.m_HiddenNets )
    {
        if( NETINFO_ITEM* net = nets.GetNetItem( hidden ) )
            hiddenNets.insert( net->GetNet() );
    }

    rs->LoadNetSettings( project.NetSettings(), nets, hiddenNets );

    SELECTION_FILTER_OPTIONS& filterOpts = GetToolManager()->GetTool<SELECTION_TOOL>()->GetFilter();

    filterOpts = localSettings.m_SelectionFilter;
    m_selectionFilterPanel->SetCheckboxesFromFilter( filterOpts );

    PCB_DISPLAY_OPTIONS opts   = GetDisplayOptions();
    opts.m_ContrastModeDisplay = localSettings.m_ContrastModeDisplay;
    SetDisplayOptions( opts );

    SetActiveLayer( localSettings.m_ActiveLayer );

    return true;
}


void PCB_EDIT_FRAME::SaveProjectSettings()
{
    wxFileName fn = Prj().GetProjectFullName();

    // Check for the filename before checking IsWritable as this
    // will throw errors on bad names.  Here, we just want to not
    // save the Settings if we don't have a name
    if( !fn.IsOk() )
        return;

    if( !fn.IsDirWritable() )
        return;

    PROJECT_FILE&           project       = Prj().GetProjectFile();
    PROJECT_LOCAL_SETTINGS& localSettings = Prj().GetLocalSettings();

    // TODO: Can this be pulled out of BASE_SCREEN?
    project.m_BoardPageLayoutDescrFile = BASE_SCREEN::m_PageLayoutDescrFileName;

    RecordDRCExclusions();

    localSettings.m_ActiveLayer = GetActiveLayer();

    localSettings.m_ContrastModeDisplay = GetDisplayOptions().m_ContrastModeDisplay;

    KIGFX::PCB_RENDER_SETTINGS* rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
            GetCanvas()->GetView()->GetPainter()->GetSettings() );

    NETINFO_LIST& nets = GetBoard()->GetNetInfo();

    localSettings.m_HiddenNets.clear();

    for( int netcode : rs->GetHiddenNets() )
    {
        if( NETINFO_ITEM* net = nets.GetNetItem( netcode ) )
            localSettings.m_HiddenNets.emplace_back( net->GetNetname() );
    }

    SELECTION_FILTER_OPTIONS& filterOpts = GetToolManager()->GetTool<SELECTION_TOOL>()->GetFilter();
    localSettings.m_SelectionFilter      = filterOpts;

    GetSettingsManager()->SaveProject();
}
