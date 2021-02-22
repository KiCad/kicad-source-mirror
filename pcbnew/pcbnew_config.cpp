/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_edit_frame.h>
#include <panel_hotkeys_editor.h>
#include <panel_edit_options.h>
#include <panel_pcbnew_color_settings.h>
#include <panel_display_options.h>
#include <panel_pcbnew_action_plugins.h>
#include <panel_pcbnew_display_origin.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <drawing_sheet/ds_data_model.h>
#include <pcbplot.h>
#include <pcb_painter.h>
#include <invoke_pcb_dialog.h>
#include <widgets/appearance_controls.h>
#include <widgets/paged_dialog.h>
#include <widgets/panel_selection_filter.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>



void PCB_EDIT_FRAME::InstallPreferences( PAGED_DIALOG* aParent,
                                         PANEL_HOTKEYS_EDITOR* aHotkeysPanel )
{
    wxTreebook* book = aParent->GetTreebook();

    book->AddPage( new wxPanel( book ), _( "PCB Editor" ) );
    book->AddSubPage( new PANEL_DISPLAY_OPTIONS( this, aParent ), _( "Display Options" ) );
    book->AddSubPage( new PANEL_EDIT_OPTIONS( this, aParent ), _( "Editing Options" ) );
    book->AddSubPage( new PANEL_PCBNEW_COLOR_SETTINGS( this, book ), _( "Colors" ) );
#if defined(KICAD_SCRIPTING) && defined(KICAD_SCRIPTING_ACTION_MENU)
    book->AddSubPage( new PANEL_PCBNEW_ACTION_PLUGINS( this, aParent ), _( "Action Plugins" ) );
#endif
    book->AddSubPage( new PANEL_PCBNEW_DISPLAY_ORIGIN( this, aParent ), _( "Origins & Axes" ) );

    aHotkeysPanel->AddHotKeys( GetToolManager() );
}


bool PCB_EDIT_FRAME::LoadProjectSettings()
{
    PROJECT_FILE&           project       = Prj().GetProjectFile();
    PROJECT_LOCAL_SETTINGS& localSettings = Prj().GetLocalSettings();

    BASE_SCREEN::m_PageLayoutDescrFileName = project.m_BoardPageLayoutDescrFile;

    // Load the drawing sheet description file, from the filename stored in
    // BASE_SCREEN::m_PageLayoutDescrFileName, read in config project file
    // If empty, or not existing, the default descr is loaded
    wxString filename = DS_DATA_MODEL::MakeFullFileName( BASE_SCREEN::m_PageLayoutDescrFileName,
                                                         Prj().GetProjectPath() );

    DS_DATA_MODEL::GetTheInstance().LoadDrawingSheet( filename );

    // Load render settings that aren't stored in PCB_DISPLAY_OPTIONS

    NET_SETTINGS& netSettings = project.NetSettings();
    NETINFO_LIST& nets        = GetBoard()->GetNetInfo();

    KIGFX::PCB_RENDER_SETTINGS* rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
            GetCanvas()->GetView()->GetPainter()->GetSettings() );

    std::set<int>& hiddenNets = rs->GetHiddenNets();
    hiddenNets.clear();

    for( const wxString& hidden : localSettings.m_HiddenNets )
    {
        if( NETINFO_ITEM* net = nets.GetNetItem( hidden ) )
            hiddenNets.insert( net->GetNetCode() );
    }

    std::map<int, KIGFX::COLOR4D>& netColors = rs->GetNetColorMap();
    netColors.clear();

    for( const auto& pair : netSettings.m_PcbNetColors )
    {
        if( pair.second == COLOR4D::UNSPECIFIED )
            continue;

        if( NETINFO_ITEM* net = nets.GetNetItem( pair.first ) )
            netColors[ net->GetNetCode() ] = pair.second;
    }

    std::map<wxString, KIGFX::COLOR4D>& netclassColors = rs->GetNetclassColorMap();
    netclassColors.clear();

    for( const auto& pair : netSettings.m_NetClasses )
        netclassColors[pair.first] = pair.second->GetPcbColor();

    m_appearancePanel->SetUserLayerPresets( project.m_LayerPresets );

    PCB_SELECTION_TOOL*       selTool = GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    SELECTION_FILTER_OPTIONS& filterOpts = selTool->GetFilter();

    filterOpts = localSettings.m_SelectionFilter;
    m_selectionFilterPanel->SetCheckboxesFromFilter( filterOpts );

    PCB_DISPLAY_OPTIONS opts   = GetDisplayOptions();
    opts.m_ContrastModeDisplay = localSettings.m_ContrastModeDisplay;
    opts.m_NetColorMode        = localSettings.m_NetColorMode;
    opts.m_RatsnestMode        = localSettings.m_RatsnestMode;
    opts.m_TrackOpacity        = localSettings.m_TrackOpacity;
    opts.m_ViaOpacity          = localSettings.m_ViaOpacity;
    opts.m_PadOpacity          = localSettings.m_PadOpacity;
    opts.m_ZoneOpacity         = localSettings.m_ZoneOpacity;
    opts.m_ZoneDisplayMode     = localSettings.m_ZoneDisplayMode;
    SetDisplayOptions( opts );

    BOARD_DESIGN_SETTINGS& bds   = GetDesignSettings();
    bds.m_UseConnectedTrackWidth = localSettings.m_AutoTrackWidth;

    wxFileName fn( GetCurrentFileName() );
    fn.MakeRelativeTo( Prj().GetProjectPath() );
    LoadWindowState( fn.GetFullPath() );

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

    project.m_LayerPresets = m_appearancePanel->GetUserLayerPresets();

    RecordDRCExclusions();

    // Save appearance control settings

    localSettings.m_ActiveLayer       = GetActiveLayer();
    localSettings.m_ActiveLayerPreset = m_appearancePanel->GetActiveLayerPreset();

    const PCB_DISPLAY_OPTIONS& displayOpts = GetDisplayOptions();

    localSettings.m_ContrastModeDisplay = displayOpts.m_ContrastModeDisplay;
    localSettings.m_NetColorMode        = displayOpts.m_NetColorMode;
    localSettings.m_RatsnestMode        = displayOpts.m_RatsnestMode;
    localSettings.m_TrackOpacity        = displayOpts.m_TrackOpacity;
    localSettings.m_ViaOpacity          = displayOpts.m_ViaOpacity;
    localSettings.m_PadOpacity          = displayOpts.m_PadOpacity;
    localSettings.m_ZoneOpacity         = displayOpts.m_ZoneOpacity;
    localSettings.m_ZoneDisplayMode     = displayOpts.m_ZoneDisplayMode;

    // Save Design settings
    const BOARD_DESIGN_SETTINGS& bds = GetDesignSettings();
    localSettings.m_AutoTrackWidth   = bds.m_UseConnectedTrackWidth;

    // Save render settings that aren't stored in PCB_DISPLAY_OPTIONS

    KIGFX::PCB_RENDER_SETTINGS* rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
            GetCanvas()->GetView()->GetPainter()->GetSettings() );

    NETINFO_LIST& nets = GetBoard()->GetNetInfo();

    localSettings.m_HiddenNets.clear();

    for( int netcode : rs->GetHiddenNets() )
    {
        if( NETINFO_ITEM* net = nets.GetNetItem( netcode ) )
            localSettings.m_HiddenNets.emplace_back( net->GetNetname() );
    }

    NET_SETTINGS& netSettings = project.NetSettings();

    netSettings.m_PcbNetColors.clear();

    for( const std::pair<const int, KIGFX::COLOR4D>& pair : rs->GetNetColorMap() )
    {
        if( NETINFO_ITEM* net = nets.GetNetItem( pair.first ) )
            netSettings.m_PcbNetColors[net->GetNetname()] = pair.second;
    }

    std::map<wxString, KIGFX::COLOR4D>& netclassColors = rs->GetNetclassColorMap();

    // NOTE: this assumes netclasses will have already been updated, which I think is the case
    for( const std::pair<const wxString, NETCLASSPTR>& pair : netSettings.m_NetClasses )
    {
        if( netclassColors.count( pair.first ) )
            pair.second->SetPcbColor( netclassColors.at( pair.first ) );
    }

    PCB_SELECTION_TOOL*       selTool = GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    SELECTION_FILTER_OPTIONS& filterOpts = selTool->GetFilter();

    localSettings.m_SelectionFilter = filterOpts;

    /**
     * The below automatically saves the project on exit, which is what we want to do if the project
     * already exists.  If the project doesn't already exist, we don't want to create it through
     * this function call, because this will happen automatically when the user exits even if they
     * didn't save a new board with a valid filename (usually an imported board).
     *
     * The explicit save action in PCB_EDIT_FRAME::SavePcbFile will call SaveProject directly,
     * so if the user does choose to save the board, the project file will get created then.
     */
    if( !Prj().IsNullProject() && fn.Exists() )
        GetSettingsManager()->SaveProject();
}
