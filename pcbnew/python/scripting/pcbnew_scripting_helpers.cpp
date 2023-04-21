/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <action_plugin.h>
#include <board.h>
#include <board_design_settings.h>
#include <pcb_marker.h>
#include <cstdlib>
#include <drawing_sheet/ds_data_model.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <fp_lib_table.h>
#include <core/ignore.h>
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
#include <wx/crt.h>


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
        if( wxFileExists( projectPath ) )
        {
            GetSettingsManager()->LoadProject( projectPath, false );
            project = GetSettingsManager()->GetProject( projectPath );
        }
    }
    else if( s_PcbEditFrame && project == &GetSettingsManager()->Prj() )
    {
        // Project is already loaded?  Then so is the board
        return s_PcbEditFrame->GetBoard();
    }

    // Board cannot be loaded without a project, so create the default project
    if( !project )
        project = GetDefaultProject();

    BASE_SCREEN::m_DrawingSheetFileName = project->GetProjectFile().m_BoardDrawingSheetFile;

    // Load the drawing sheet from the filename stored in BASE_SCREEN::m_DrawingSheetFileName.
    // If empty, or not existing, the default drawing sheet is loaded.
    wxString filename = DS_DATA_MODEL::ResolvePath( BASE_SCREEN::m_DrawingSheetFileName,
                                                    project->GetProjectPath() );

    if( !DS_DATA_MODEL::GetTheInstance().LoadDrawingSheet( filename ) )
        wxFprintf( stderr, _( "Error loading drawing sheet." ) );

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

        for( PCB_MARKER* marker : brd->ResolveDRCExclusions( true ) )
            brd->Add( marker );

        brd->BuildConnectivity();
        brd->BuildListOfNets();
        brd->SynchronizeNetsAndNetClasses( false );
        brd->UpdateUserUnits( brd, nullptr );
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


bool SaveBoard( wxString& aFileName, BOARD* aBoard, IO_MGR::PCB_FILE_T aFormat, bool aSkipSettings )
{
    aBoard->BuildConnectivity();
    aBoard->SynchronizeNetsAndNetClasses( false );

    // Ensure the "C" locale is temporary set, before saving any file
    // It also avoid wxWidget alerts about locale issues, later, when using Python 3
    LOCALE_IO dummy;

    try
    {
        IO_MGR::Save( aFormat, aFileName, aBoard, nullptr );
    }
    catch( ... )
    {
        return false;
    }

    if( !aSkipSettings )
    {
        wxFileName pro = aFileName;
        pro.SetExt( ProjectFileExtension );
        pro.MakeAbsolute();

        GetSettingsManager()->SaveProjectAs( pro.GetFullPath(), aBoard->GetProject() );
    }

    return true;
}


bool SaveBoard( wxString& aFileName, BOARD* aBoard, bool aSkipSettings )
{
    return SaveBoard( aFileName, aBoard, IO_MGR::KICAD_SEXP, aSkipSettings );
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
        s_PcbEditFrame->RebuildAndRefresh();
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


std::deque<BOARD_ITEM*> GetCurrentSelection()
{
    std::deque<BOARD_ITEM*> items;

    if( s_PcbEditFrame )
    {
        SELECTION& selection = s_PcbEditFrame->GetCurrentSelection();

        std::for_each( selection.begin(), selection.end(),
                       [&items]( EDA_ITEM* item )
                       {
                           items.push_back( static_cast<BOARD_ITEM*>( item ) );
                       } );
    }

    return items;
}


bool IsActionRunning()
{
    return ACTION_PLUGINS::IsActionRunning();
}


bool WriteDRCReport( BOARD* aBoard, const wxString& aFileName, EDA_UNITS aUnits,
                     bool aReportAllTrackErrors )
{
    wxCHECK( aBoard, false );

    BOARD_DESIGN_SETTINGS&      bds = aBoard->GetDesignSettings();
    std::shared_ptr<DRC_ENGINE> engine = bds.m_DRCEngine;
    UNITS_PROVIDER              unitsProvider( pcbIUScale, aUnits );

    if( !engine )
    {
        bds.m_DRCEngine = std::make_shared<DRC_ENGINE>( aBoard, &bds );
        engine = bds.m_DRCEngine;
    }

    wxCHECK( engine, false );

    wxFileName fn = aBoard->GetFileName();
    fn.SetExt( DesignRulesFileExtension );
    PROJECT* prj = nullptr;

    if( aBoard->GetProject() )
        prj = aBoard->GetProject();
    else if( s_SettingsManager )
        prj = &s_SettingsManager->Prj();

    wxCHECK( prj, false );

    // Load the global fp-lib-table otherwise we can't check the libs parity
    wxFileName  fn_flp = FP_LIB_TABLE::GetGlobalTableFileName();
    if( fn_flp.FileExists() )
        GFootprintTable.Load( fn_flp.GetFullPath() );

    wxString drcRulesPath = prj->AbsolutePath( fn.GetFullName() );

    // Rebuild The Instance of ENUM_MAP<PCB_LAYER_ID> (layer names list), because the DRC
    // engine can use layer names (canonical and/or user names)
    ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();
    layerEnum.Choices().Clear();
    layerEnum.Undefined( UNDEFINED_LAYER );

    for( LSEQ seq = LSET::AllLayersMask().Seq(); seq; ++seq )
    {
        layerEnum.Map( *seq, LSET::Name( *seq ) );              // Add Canonical name
        layerEnum.Map( *seq, aBoard->GetLayerName( *seq ) );    // Add User name
    }

    try
    {
        engine->InitEngine( drcRulesPath );
    }
    catch( PARSE_ERROR& err )
    {
        fprintf( stderr, "Init DRC engine: err <%s>\n", TO_UTF8( err.What() ) ); fflush( stderr);
        return false;
    }

    std::vector<std::shared_ptr<DRC_ITEM>> footprints;
    std::vector<std::shared_ptr<DRC_ITEM>> unconnected;
    std::vector<std::shared_ptr<DRC_ITEM>> violations;

    engine->SetProgressReporter( nullptr );

    engine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, VECTOR2D aPos, int aLayer )
            {
                if( aItem->GetErrorCode() == DRCE_MISSING_FOOTPRINT
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
        SEVERITY severity = item->GetParent() ? item->GetParent()->GetSeverity()
                                              : bds.GetSeverity( item->GetErrorCode() );
        fprintf( fp, "%s", TO_UTF8( item->ShowReport( &unitsProvider, severity, itemMap ) ) );
    }

    fprintf( fp, "\n** Found %d unconnected pads **\n", static_cast<int>( unconnected.size() ) );

    for( const std::shared_ptr<DRC_ITEM>& item : unconnected )
    {
        SEVERITY severity = bds.GetSeverity( item->GetErrorCode() );
        fprintf( fp, "%s", TO_UTF8( item->ShowReport( &unitsProvider, severity, itemMap ) ) );
    }

    fprintf( fp, "\n** Found %d Footprint errors **\n", static_cast<int>( footprints.size() ) );

    for( const std::shared_ptr<DRC_ITEM>& item : footprints )
    {
        SEVERITY severity = bds.GetSeverity( item->GetErrorCode() );
        fprintf( fp, "%s", TO_UTF8( item->ShowReport( &unitsProvider, severity, itemMap ) ) );
    }

    fprintf( fp, "\n** End of Report **\n" );
    fclose( fp );

    return true;
}
