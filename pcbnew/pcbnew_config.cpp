/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <board_design_settings.h>
#include <drawing_sheet/ds_data_model.h>
#include <filename_resolver.h>
#include <pcbplot.h>
#include <pcb_painter.h>
#include <pgm_base.h>
#include <project.h>
#include <widgets/appearance_controls.h>
#include <widgets/panel_selection_filter.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>


void PCB_EDIT_FRAME::LoadDrawingSheet()
{
    PROJECT_FILE&           project       = Prj().GetProjectFile();

    // Load the drawing sheet from the filename stored in the project
    // If empty, or not existing, the default drawing sheet is loaded.
    FILENAME_RESOLVER resolver;
    resolver.SetProject( &Prj() );
    resolver.SetProgramBase( &Pgm() );

    wxString filename = resolver.ResolvePath( project.m_BoardDrawingSheetFile,
                                              Prj().GetProjectPath(),
                                              { GetBoard()->GetEmbeddedFiles() } );

    wxString msg;

    if( !DS_DATA_MODEL::GetTheInstance().LoadDrawingSheet( filename, &msg ) )
        ShowInfoBarError( msg, true );

}

bool PCB_EDIT_FRAME::LoadProjectSettings()
{
    PROJECT_FILE&           project       = Prj().GetProjectFile();
    PROJECT_LOCAL_SETTINGS& localSettings = Prj().GetLocalSettings();

    BASE_SCREEN::m_DrawingSheetFileName = project.m_BoardDrawingSheetFile;

    // Load render settings that aren't stored in PCB_DISPLAY_OPTIONS

    std::shared_ptr<NET_SETTINGS>& netSettings = project.NetSettings();
    KIGFX::RENDER_SETTINGS*        rs = GetCanvas()->GetView()->GetPainter()->GetSettings();
    KIGFX::PCB_RENDER_SETTINGS*    renderSettings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( rs );

    const NETINFO_LIST& nets = GetBoard()->GetNetInfo();

    std::set<int>& hiddenNets = renderSettings->GetHiddenNets();
    hiddenNets.clear();

    for( const wxString& hidden : localSettings.m_HiddenNets )
    {
        if( NETINFO_ITEM* net = nets.GetNetItem( hidden ) )
            hiddenNets.insert( net->GetNetCode() );
    }

    for( NETINFO_ITEM* net : nets )
    {
        if( localSettings.m_HiddenNetclasses.count( net->GetNetClass()->GetName() ) )
            hiddenNets.insert( net->GetNetCode() );
    }

    std::map<int, KIGFX::COLOR4D>& netColors = renderSettings->GetNetColorMap();
    netColors.clear();

    for( const auto& [netname, color] : netSettings->GetNetColorAssignments() )
    {
        if( color != COLOR4D::UNSPECIFIED )
        {
            if( NETINFO_ITEM* net = GetBoard()->GetNetInfo().GetNetItem( netname ) )
                netColors[ net->GetNetCode() ] = color;
        }
    }

    m_appearancePanel->SetUserLayerPresets( project.m_LayerPresets );
    m_appearancePanel->SetUserViewports( project.m_Viewports );

    PCB_SELECTION_TOOL*       selTool = GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    PCB_SELECTION_FILTER_OPTIONS& filterOpts = selTool->GetFilter();

    filterOpts = localSettings.m_PcbSelectionFilter;
    m_selectionFilterPanel->SetCheckboxesFromFilter( filterOpts );

    PCB_DISPLAY_OPTIONS opts   = GetDisplayOptions();
    opts.m_ContrastModeDisplay = localSettings.m_ContrastModeDisplay;
    opts.m_NetColorMode        = localSettings.m_NetColorMode;
    opts.m_TrackOpacity        = localSettings.m_TrackOpacity;
    opts.m_ViaOpacity          = localSettings.m_ViaOpacity;
    opts.m_PadOpacity          = localSettings.m_PadOpacity;
    opts.m_ZoneOpacity         = localSettings.m_ZoneOpacity;
    opts.m_ZoneDisplayMode     = localSettings.m_ZoneDisplayMode;
    opts.m_ImageOpacity        = localSettings.m_ImageOpacity;
    opts.m_FilledShapeOpacity        = localSettings.m_ShapeOpacity;

    // No refresh here: callers of LoadProjectSettings refresh later
    SetDisplayOptions( opts, false );

    BOARD_DESIGN_SETTINGS& bds   = GetDesignSettings();
    bds.m_UseConnectedTrackWidth = localSettings.m_AutoTrackWidth;

    wxFileName fn( GetCurrentFileName() );
    fn.MakeRelativeTo( Prj().GetProjectPath() );
    LoadWindowState( fn.GetFullPath() );

    return true;
}


void PCB_EDIT_FRAME::SaveProjectLocalSettings()
{
    wxFileName fn = Prj().GetProjectFullName();

    // Check for the filename before checking IsWritable as this
    // will throw errors on bad names.  Here, we just want to not
    // save the Settings if we don't have a name
    if( !fn.IsOk() )
        return;

    if( !fn.IsDirWritable() )
        return;

    PROJECT_FILE& project = Prj().GetProjectFile();

    // save some local settings like appearance control settings
    saveProjectSettings();

    // TODO: Can this be pulled out of BASE_SCREEN?
    project.m_BoardDrawingSheetFile = BASE_SCREEN::m_DrawingSheetFileName;

    project.m_LayerPresets = m_appearancePanel->GetUserLayerPresets();
    project.m_Viewports = m_appearancePanel->GetUserViewports();

    GetBoard()->RecordDRCExclusions();

    // Save render settings that aren't stored in PCB_DISPLAY_OPTIONS

    std::shared_ptr<NET_SETTINGS>& netSettings = project.NetSettings();
    const NETINFO_LIST&            nets = GetBoard()->GetNetInfo();
    KIGFX::RENDER_SETTINGS*        rs = GetCanvas()->GetView()->GetPainter()->GetSettings();
    KIGFX::PCB_RENDER_SETTINGS*    renderSettings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( rs );

    netSettings->ClearNetColorAssignments();

    for( const auto& [ netcode, color ] : renderSettings->GetNetColorMap() )
    {
        if( NETINFO_ITEM* net = nets.GetNetItem( netcode ) )
            netSettings->SetNetColorAssignment( net->GetNetname(), color );
    }

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


void PCB_EDIT_FRAME::saveProjectSettings()
{
    wxFileName fn = Prj().GetProjectFullName();

    // Check for the filename before checking IsWritable as this
    // will throw errors on bad names.  Here, we just want to not
    // save the Settings if we don't have a name
    if( !fn.IsOk() )
        return;

    if( !fn.IsDirWritable() )
        return;

    PROJECT_LOCAL_SETTINGS& localSettings = Prj().GetLocalSettings();

    // Save appearance control settings
    localSettings.m_ActiveLayer       = GetActiveLayer();
    localSettings.m_ActiveLayerPreset = m_appearancePanel->GetActiveLayerPreset();

    const PCB_DISPLAY_OPTIONS& displayOpts = GetDisplayOptions();

    localSettings.m_ContrastModeDisplay = displayOpts.m_ContrastModeDisplay;
    localSettings.m_NetColorMode        = displayOpts.m_NetColorMode;
    localSettings.m_TrackOpacity        = displayOpts.m_TrackOpacity;
    localSettings.m_ViaOpacity          = displayOpts.m_ViaOpacity;
    localSettings.m_PadOpacity          = displayOpts.m_PadOpacity;
    localSettings.m_ZoneOpacity         = displayOpts.m_ZoneOpacity;
    localSettings.m_ZoneDisplayMode     = displayOpts.m_ZoneDisplayMode;
    localSettings.m_ImageOpacity        = displayOpts.m_ImageOpacity;
    localSettings.m_ShapeOpacity        = displayOpts.m_FilledShapeOpacity;

    // Save Design settings
    const BOARD_DESIGN_SETTINGS& bds = GetDesignSettings();
    localSettings.m_AutoTrackWidth   = bds.m_UseConnectedTrackWidth;

    // Net display settings
    const NETINFO_LIST&         nets = GetBoard()->GetNetInfo();
    KIGFX::RENDER_SETTINGS*     rs = GetCanvas()->GetView()->GetPainter()->GetSettings();
    KIGFX::PCB_RENDER_SETTINGS* renderSettings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( rs );

    localSettings.m_HiddenNets.clear();

    for( int netcode : renderSettings->GetHiddenNets() )
    {
        if( NETINFO_ITEM* net = nets.GetNetItem( netcode ) )
            localSettings.m_HiddenNets.emplace_back( net->GetNetname() );
    }

    PCB_SELECTION_TOOL*       selTool = GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    PCB_SELECTION_FILTER_OPTIONS& filterOpts = selTool->GetFilter();

    localSettings.m_PcbSelectionFilter = filterOpts;
}
