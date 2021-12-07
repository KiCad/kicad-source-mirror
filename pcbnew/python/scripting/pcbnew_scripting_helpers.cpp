/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcbnew_scripting_helpers.cpp
 * @brief Scripting helper functions for pcbnew functionality
 */

#include <Python.h>
#undef HAVE_CLOCK_GETTIME  // macro is defined in Python.h and causes redefine warning

#include "pcbnew_scripting_helpers.h"

#include <tool/tool_manager.h>
#include <action_plugin.h>
#include <board.h>
#include <board_design_settings.h>
#include <pcb_marker.h>
#include <cstdlib>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <fp_lib_table.h>
#include <ignore.h>
#include <io_mgr.h>
#include <string_utils.h>
#include <macros.h>
#include <pcbnew_scripting_helpers.h>
#include <project.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <specctra.h>
#include <project/project_local_settings.h>
#include <wildcards_and_files_ext.h>
#include <locale_io.h>
#include <wx/app.h>


static PCB_EDIT_FRAME* s_PcbEditFrame = nullptr;
static SETTINGS_MANAGER* s_SettingsManager = nullptr;


BOARD* GetBoard()
{
    if( s_PcbEditFrame )
        return s_PcbEditFrame->GetBoard();
    else
        return nullptr;
}


void ScriptingSetPcbEditFrame( PCB_EDIT_FRAME* aPcbEditFrame )
{
    s_PcbEditFrame = aPcbEditFrame;
}


BOARD* LoadBoard( wxString& aFileName )
{
    if( aFileName.EndsWith( KiCadPcbFileExtension ) )
        return LoadBoard( aFileName, IO_MGR::KICAD_SEXP );
    else if( aFileName.EndsWith( LegacyPcbFileExtension ) )
        return LoadBoard( aFileName, IO_MGR::LEGACY );

    // as fall back for any other kind use the legacy format
    return LoadBoard( aFileName, IO_MGR::LEGACY );
}


SETTINGS_MANAGER* GetSettingsManager()
{
    if( !s_SettingsManager )
    {
        if( s_PcbEditFrame )
        {
            s_SettingsManager = s_PcbEditFrame->GetSettingsManager();
        }
        else
        {
            // Ensure wx system settings stuff is available
            ignore_unused( wxTheApp );
            s_SettingsManager = new SETTINGS_MANAGER( true );
        }
    }

    return s_SettingsManager;
}


PROJECT* GetDefaultProject()
{
    // For some reasons, LoadProject() needs a C locale, so ensure we have the right locale
    // This is mainly when running QA Python tests
    LOCALE_IO dummy;

    PROJECT* project = GetSettingsManager()->GetProject( "" );

    if( !project )
    {
        GetSettingsManager()->LoadProject( "" );
        project = GetSettingsManager()->GetProject( "" );
    }

    return project;
}


BOARD* LoadBoard( wxString& aFileName, IO_MGR::PCB_FILE_T aFormat )
{
    wxFileName pro = aFileName;
    pro.SetExt( ProjectFileExtension );
    pro.MakeAbsolute();
    wxString projectPath = pro.GetFullPath();

    // Ensure the "C" locale is temporary set, before reading any file
    // It also avoid wxWidget alerts about locale issues, later, when using Python 3
    LOCALE_IO dummy;

    PROJECT* project = GetSettingsManager()->GetProject( projectPath );

    if( !project )
    {
        GetSettingsManager()->LoadProject( projectPath, false );
        project = GetSettingsManager()->GetProject( projectPath );
    }
    else if( s_PcbEditFrame && project == &GetSettingsManager()->Prj() )
    {
        // Project is already loaded?  Then so is the board
        return s_PcbEditFrame->GetBoard();
    }

    // Board cannot be loaded without a project, so create the default project
    if( !project )
        project = GetDefaultProject();

    BOARD* brd = IO_MGR::Load( aFormat, aFileName );

    if( brd )
    {
        brd->SetProject( project );

        if( brd->m_LegacyDesignSettingsLoaded )
            project->GetProjectFile().NetSettings().RebuildNetClassAssignments();

        // Move legacy view settings to local project settings
        if( !brd->m_LegacyVisibleLayers.test( Rescue ) )
            project->GetLocalSettings().m_VisibleLayers = brd->m_LegacyVisibleLayers;

        if( !brd->m_LegacyVisibleItems.test( GAL_LAYER_INDEX( GAL_LAYER_ID_BITMASK_END ) ) )
            project->GetLocalSettings().m_VisibleItems = brd->m_LegacyVisibleItems;

        BOARD_DESIGN_SETTINGS& bds = brd->GetDesignSettings();
        bds.m_DRCEngine = std::make_shared<DRC_ENGINE>( brd, &bds );

        try
        {
            wxFileName rules = pro;
            rules.SetExt( DesignRulesFileExtension );
            bds.m_DRCEngine->InitEngine( rules );
        }
        catch( ... )
        {
            // Best efforts...
        }

        for( PCB_MARKER* marker : brd->ResolveDRCExclusions() )
            brd->Add( marker );

        brd->BuildConnectivity();
        brd->BuildListOfNets();
        brd->SynchronizeNetsAndNetClasses();
    }

    return brd;
}


BOARD* NewBoard( wxString& aFileName )
{
    wxFileName boardFn = aFileName;
    wxFileName proFn   = aFileName;
    proFn.SetExt( ProjectFileExtension );
    proFn.MakeAbsolute();

    wxString projectPath = proFn.GetFullPath();

    // Ensure the "C" locale is temporary set, before reading any file
    // It also avoids wxWidgets alerts about locale issues, later, when using Python 3
    LOCALE_IO dummy;

    GetSettingsManager()->LoadProject( projectPath, false );
    PROJECT* project = GetSettingsManager()->GetProject( projectPath );

    BOARD* brd = new BOARD();

    brd->SetProject( project );
    BOARD_DESIGN_SETTINGS& bds = brd->GetDesignSettings();
    bds.m_DRCEngine            = std::make_shared<DRC_ENGINE>( brd, &bds );

    SaveBoard( aFileName, brd );

    return brd;
}


BOARD* CreateEmptyBoard()
{
    // Creating a new board is not possible if running inside KiCad
    if( s_PcbEditFrame )
        return nullptr;

    BOARD* brd = new BOARD();

    brd->SetProject( GetDefaultProject() );

    return brd;
}


bool SaveBoard( wxString& aFileName, BOARD* aBoard, IO_MGR::PCB_FILE_T aFormat )
{
    aBoard->BuildConnectivity();
    aBoard->SynchronizeNetsAndNetClasses();

    try
    {
        IO_MGR::Save( aFormat, aFileName, aBoard, nullptr );
    }
    catch( ... )
    {
        return false;
    }

    wxFileName pro = aFileName;
    pro.SetExt( ProjectFileExtension );
    pro.MakeAbsolute();
    wxString projectPath = pro.GetFullPath();

    GetSettingsManager()->SaveProjectAs( pro.GetFullPath() );

    return true;
}


bool SaveBoard( wxString& aFileName, BOARD* aBoard )
{
    return SaveBoard( aFileName, aBoard, IO_MGR::KICAD_SEXP );
}


FP_LIB_TABLE* GetFootprintLibraryTable()
{
    BOARD* board = GetBoard();

    if( !board )
        return nullptr;

    PROJECT* project = board->GetProject();

    if( !project )
        return nullptr;

    return project->PcbFootprintLibs();
}


wxArrayString GetFootprintLibraries()
{
    wxArrayString footprintLibraryNames;

    FP_LIB_TABLE* tbl = GetFootprintLibraryTable();

    if( !tbl )
        return footprintLibraryNames;

    for( const wxString& name : tbl->GetLogicalLibs() )
        footprintLibraryNames.Add( name );

    return footprintLibraryNames;
}


wxArrayString GetFootprints( const wxString& aNickName )
{
    wxArrayString footprintNames;

    FP_LIB_TABLE* tbl = GetFootprintLibraryTable();

    if( !tbl )
        return footprintNames;

    tbl->FootprintEnumerate( footprintNames, aNickName, true );

    return footprintNames;
}


bool ExportSpecctraDSN( wxString& aFullFilename )
{
    if( s_PcbEditFrame )
    {
        bool ok = s_PcbEditFrame->ExportSpecctraFile( aFullFilename );
        return ok;
    }
    else
    {
        return false;
    }
}


bool ExportSpecctraDSN( BOARD* aBoard, wxString& aFullFilename )
{
    try
    {
        ExportBoardToSpecctraFile( aBoard, aFullFilename );
    }
    catch( ... )
    {
        return false;
    }

    return true;
}


bool ExportVRML( const wxString& aFullFileName, double aMMtoWRMLunit, bool aExport3DFiles,
                 bool aUseRelativePaths, const wxString& a3D_Subdir, double aXRef, double aYRef )
{
    if( s_PcbEditFrame )
    {
        bool ok = s_PcbEditFrame->ExportVRML_File( aFullFileName, aMMtoWRMLunit,
                                                   aExport3DFiles, aUseRelativePaths,
                                                   a3D_Subdir, aXRef, aYRef );
        return ok;
    }
    else
    {
        return false;
    }
}

bool ImportSpecctraSES( wxString& aFullFilename )
{
    if( s_PcbEditFrame )
    {
        bool ok = s_PcbEditFrame->ImportSpecctraSession( aFullFilename );
        return ok;
    }
    else
    {
        return false;
    }
}


bool ExportFootprintsToLibrary( bool aStoreInNewLib, const wxString& aLibName, wxString* aLibPath )
{
    if( s_PcbEditFrame )
    {
        s_PcbEditFrame->ExportFootprintsToLibrary( aStoreInNewLib, aLibName, aLibPath );
        return true;
    }
    else
    {
        return false;
    }
}

void Refresh()
{
    if( s_PcbEditFrame )
    {
        TOOL_MANAGER*       toolMgr = s_PcbEditFrame->GetToolManager();
        BOARD*              board = s_PcbEditFrame->GetBoard();
        PCB_DRAW_PANEL_GAL* canvas = s_PcbEditFrame->GetCanvas();

        canvas->SyncLayersVisibility( board );

        canvas->GetView()->Clear();
        canvas->GetView()->InitPreview();
        canvas->GetGAL()->SetGridOrigin( VECTOR2D( board->GetDesignSettings().GetGridOrigin() ) );
        canvas->DisplayBoard( board );

        // allow tools to re-add their view items (selection previews, grids, etc.)
        if( toolMgr )
            toolMgr->ResetTools( TOOL_BASE::GAL_SWITCH );

        // reload the drawing-sheet
        s_PcbEditFrame->SetPageSettings( board->GetPageSettings() );

        board->BuildConnectivity();

        canvas->Refresh();
    }
}


void UpdateUserInterface()
{
    if( s_PcbEditFrame )
        s_PcbEditFrame->UpdateUserInterface();
}


int GetUserUnits()
{
    if( s_PcbEditFrame )
        return static_cast<int>( s_PcbEditFrame->GetUserUnits() );

    return -1;
}


bool IsActionRunning()
{
    return ACTION_PLUGINS::IsActionRunning();
}


bool WriteDRCReport( BOARD* aBoard, const wxString& aFileName, EDA_UNITS aUnits,
                     bool aReportAllTrackErrors )
{
    wxCHECK( aBoard, false );

    BOARD_DESIGN_SETTINGS& bds = aBoard->GetDesignSettings();
    std::shared_ptr<DRC_ENGINE> engine = bds.m_DRCEngine;

    if( !engine )
    {
        bds.m_DRCEngine = std::make_shared<DRC_ENGINE>( aBoard, &bds );
        engine = bds.m_DRCEngine;
    }

    wxCHECK( engine, false );

    wxFileName fn = aBoard->GetFileName();
    fn.SetExt( DesignRulesFileExtension );
    wxString drcRulesPath = s_SettingsManager->Prj().AbsolutePath( fn.GetFullName() );

    try
    {
        engine->InitEngine( drcRulesPath );
    }
    catch( PARSE_ERROR& )
    {
        return false;
    }

    std::vector<std::shared_ptr<DRC_ITEM>> footprints;
    std::vector<std::shared_ptr<DRC_ITEM>> unconnected;
    std::vector<std::shared_ptr<DRC_ITEM>> violations;

    engine->SetProgressReporter( nullptr );

    engine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, wxPoint aPos )
            {
                if(    aItem->GetErrorCode() == DRCE_MISSING_FOOTPRINT
                    || aItem->GetErrorCode() == DRCE_DUPLICATE_FOOTPRINT
                    || aItem->GetErrorCode() == DRCE_EXTRA_FOOTPRINT
                    || aItem->GetErrorCode() == DRCE_NET_CONFLICT )
                {
                    footprints.push_back( aItem );
                }
                else if( aItem->GetErrorCode() == DRCE_UNCONNECTED_ITEMS )
                {
                    unconnected.push_back( aItem );
                }
                else
                {
                    violations.push_back( aItem );
                }
            } );

    engine->RunTests( aUnits, aReportAllTrackErrors, false );
    engine->ClearViolationHandler();

    // TODO: Unify this with DIALOG_DRC::writeReport

    FILE* fp = wxFopen( aFileName, wxT( "w" ) );

    if( fp == nullptr )
        return false;

    std::map<KIID, EDA_ITEM*> itemMap;
    aBoard->FillItemMap( itemMap );

    fprintf( fp, "** Drc report for %s **\n", TO_UTF8( aBoard->GetFileName() ) );

    wxDateTime now = wxDateTime::Now();

    fprintf( fp, "** Created on %s **\n", TO_UTF8( now.Format( wxT( "%F %T" ) ) ) );

    fprintf( fp, "\n** Found %d DRC violations **\n", static_cast<int>( violations.size() ) );

    for( const std::shared_ptr<DRC_ITEM>& item : violations )
    {
        SEVERITY severity = bds.GetSeverity( item->GetErrorCode() );
        fprintf( fp, "%s", TO_UTF8( item->ShowReport( aUnits, severity, itemMap ) ) );
    }

    fprintf( fp, "\n** Found %d unconnected pads **\n", static_cast<int>( unconnected.size() ) );

    for( const std::shared_ptr<DRC_ITEM>& item : unconnected )
    {
        SEVERITY severity = bds.GetSeverity( item->GetErrorCode() );
        fprintf( fp, "%s", TO_UTF8( item->ShowReport( aUnits, severity, itemMap ) ) );
    }

    fprintf( fp, "\n** Found %d Footprint errors **\n", static_cast<int>( footprints.size() ) );

    for( const std::shared_ptr<DRC_ITEM>& item : footprints )
    {
        SEVERITY severity = bds.GetSeverity( item->GetErrorCode() );
        fprintf( fp, "%s", TO_UTF8( item->ShowReport( aUnits, severity, itemMap ) ) );
    }

    fprintf( fp, "\n** End of Report **\n" );
    fclose( fp );

    return true;
}
