/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <action_plugin.h>
#include <board.h>
#include <pcb_marker.h>
#include <cstdlib>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <fp_lib_table.h>
#include <io_mgr.h>
#include <kicad_string.h>
#include <pcbnew_scripting_helpers.h>
#include <project.h>
#include <settings/settings_manager.h>
#include <project/project_local_settings.h>
#include <wildcards_and_files_ext.h>

static PCB_EDIT_FRAME* s_PcbEditFrame = NULL;

static SETTINGS_MANAGER* s_SettingsManager = nullptr;

/// A valid app is needed for preventing some asserts when opening the settings manager
static wxApp* s_WxApp = nullptr;


wxApp* GetApp()
{
    if( !s_WxApp )
        s_WxApp = new wxApp();

    return s_WxApp;
}


BOARD* GetBoard()
{
    if( s_PcbEditFrame )
        return s_PcbEditFrame->GetBoard();
    else
        return NULL;
}


void ScriptingSetPcbEditFrame( PCB_EDIT_FRAME* aPcbEditFrame )
{
    s_PcbEditFrame = aPcbEditFrame;
}


BOARD* LoadBoard( wxString& aFileName )
{
    if( aFileName.EndsWith( wxT( ".kicad_pcb" ) ) )
        return LoadBoard( aFileName, IO_MGR::KICAD_SEXP );

    else if( aFileName.EndsWith( wxT( ".brd" ) ) )
        return LoadBoard( aFileName, IO_MGR::LEGACY );

    // as fall back for any other kind use the legacy format
    return LoadBoard( aFileName, IO_MGR::LEGACY );
}


SETTINGS_MANAGER* GetSettingsManager()
{
    if( !s_SettingsManager )
    {
        // Ensure wx system settings stuff is available
        GetApp();
        s_SettingsManager = new SETTINGS_MANAGER( true );
    }

    return s_SettingsManager;
}


PROJECT* GetDefaultProject()
{
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

    PROJECT* project = GetSettingsManager()->GetProject( projectPath );

    if( !project )
    {
        GetSettingsManager()->LoadProject( projectPath );
        project = GetSettingsManager()->GetProject( projectPath );
    }

    // Board cannot be loaded without a project, so create the default project
    if( !project )
        project = GetDefaultProject();

    BOARD* brd = IO_MGR::Load( aFormat, aFileName );

    if( brd )
    {
        brd->SetProject( project );

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


BOARD* CreateEmptyBoard()
{
    BOARD* brd = new BOARD();

    brd->SetProject( GetDefaultProject() );

    return brd;
}


bool SaveBoard( wxString& aFileName, BOARD* aBoard, IO_MGR::PCB_FILE_T aFormat )
{
    aBoard->BuildConnectivity();
    aBoard->SynchronizeNetsAndNetClasses();

    IO_MGR::Save( aFormat, aFileName, aBoard, NULL );

    wxFileName pro = aFileName;
    pro.SetExt( ProjectFileExtension );
    pro.MakeAbsolute();
    wxString projectPath = pro.GetFullPath();

    GetSettingsManager()->SaveProject( pro.GetFullPath() );

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

bool ExportVRML( const wxString& aFullFileName, double aMMtoWRMLunit,
                 bool aExport3DFiles, bool aUseRelativePaths,
                 bool aUsePlainPCB, const wxString& a3D_Subdir,
                 double aXRef, double aYRef )
{
    if( s_PcbEditFrame )
    {
        bool ok = s_PcbEditFrame->ExportVRML_File( aFullFileName, aMMtoWRMLunit,
                                                   aExport3DFiles, aUseRelativePaths,
                                                   aUsePlainPCB, a3D_Subdir, aXRef, aYRef );
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
        auto board = s_PcbEditFrame->GetBoard();
        board->BuildConnectivity();

        // Re-init everything: this is the easy way to do that
        s_PcbEditFrame->ActivateGalCanvas();
        s_PcbEditFrame->GetCanvas()->Refresh();
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
    catch( PARSE_ERROR& pe )
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
