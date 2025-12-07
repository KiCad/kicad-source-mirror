/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
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
#include <footprint_library_adapter.h>
#include <core/ignore.h>
#include <pcb_io/pcb_io_mgr.h>
#include <string_utils.h>
#include <filename_resolver.h>
#include <macros.h>
#include <pcbnew_scripting_helpers.h>
#include <pgm_base.h>
#include <project.h>
#include <project_pcb.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <specctra.h>
#include <project/project_local_settings.h>
#include <wildcards_and_files_ext.h>
#include <wx/app.h>
#include <wx/crt.h>
#include <wx/image.h>

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


void ScriptingOnDestructPcbEditFrame( PCB_EDIT_FRAME* aPcbEditFrame )
{
    if( s_PcbEditFrame == aPcbEditFrame )
        s_PcbEditFrame = nullptr;
}


BOARD* LoadBoard( const wxString& aFileName, bool aSetActive )
{
    if( aFileName.EndsWith( FILEEXT::KiCadPcbFileExtension ) )
        return LoadBoard( aFileName, PCB_IO_MGR::KICAD_SEXP, aSetActive );
    else if( aFileName.EndsWith( FILEEXT::LegacyPcbFileExtension ) )
        return LoadBoard( aFileName, PCB_IO_MGR::LEGACY, aSetActive );

    // as fall back for any other kind use the legacy format
    return LoadBoard( aFileName, PCB_IO_MGR::LEGACY, aSetActive );
}


BOARD* LoadBoard( const wxString& aFileName )
{
    return LoadBoard( aFileName, false );
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
            s_SettingsManager = new SETTINGS_MANAGER();
        }
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

BOARD* LoadBoard( const wxString& aFileName, PCB_IO_MGR::PCB_FILE_T aFormat )
{
    return LoadBoard( aFileName, aFormat, false );
}


BOARD* LoadBoard( const wxString& aFileName, PCB_IO_MGR::PCB_FILE_T aFormat, bool aSetActive )
{
    wxFileName pro = aFileName;
    pro.SetExt( FILEEXT::ProjectFileExtension );
    pro.MakeAbsolute();
    wxString projectPath = pro.GetFullPath();

    // Ensure image handlers are loaded, because a board can include bitmap images
    // using various formats.
    // By default only the BMP handler is available.
    wxInitAllImageHandlers();

    PROJECT* project = GetSettingsManager()->GetProject( projectPath );

    if( !project )
    {
        if( wxFileExists( projectPath ) )
        {
            // cli
            GetSettingsManager()->LoadProject( projectPath, aSetActive );
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

    BOARD* brd = nullptr;
    try
    {
        brd = PCB_IO_MGR::Load( aFormat, aFileName );
    }
    catch( ... )
    {
        brd = nullptr;
    }

    if( brd )
    {
        // Load the drawing sheet from the filename stored in BASE_SCREEN::m_DrawingSheetFileName.
        // If empty, or not existing, the default drawing sheet is loaded.
        FILENAME_RESOLVER resolver;
        resolver.SetProject( project );

        // a PGM_BASE* process can be nullptr when running from a python script
        // So use PgmOrNull() instead of &Pgm() to initialize the resolver
        resolver.SetProgramBase( PgmOrNull() );

        wxString filename = resolver.ResolvePath( BASE_SCREEN::m_DrawingSheetFileName,
                                                  project->GetProjectPath(),
                                                  { brd->GetEmbeddedFiles() } );

        wxString msg;

        if( !DS_DATA_MODEL::GetTheInstance().LoadDrawingSheet( filename, &msg ) )
        {
            wxFprintf( stderr, _( "Error loading drawing sheet '%s': %s" ),
                       BASE_SCREEN::m_DrawingSheetFileName, msg );
        }

        // JEY TODO: move this global to the board
        ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();

        layerEnum.Choices().Clear();
        layerEnum.Undefined( UNDEFINED_LAYER );

        for( PCB_LAYER_ID layer : LSET::AllLayersMask() )
        {
            // Canonical name
            layerEnum.Map( layer, LSET::Name( layer ) );

            // User name
            layerEnum.Map( layer, brd->GetLayerName( layer ) );
        }

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
            rules.SetExt( FILEEXT::DesignRulesFileExtension );
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
        brd->SynchronizeNetsAndNetClasses( true );
        brd->UpdateUserUnits( brd, nullptr );
    }

    return brd;
}


BOARD* NewBoard( wxString& aFileName )
{
    wxFileName boardFn = aFileName;
    wxFileName proFn   = aFileName;
    proFn.SetExt( FILEEXT::ProjectFileExtension );
    proFn.MakeAbsolute();

    wxString projectPath = proFn.GetFullPath();

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


bool SaveBoard( wxString& aFileName, BOARD* aBoard, PCB_IO_MGR::PCB_FILE_T aFormat, bool aSkipSettings )
{
    aBoard->BuildConnectivity();
    aBoard->SynchronizeNetsAndNetClasses( false );

    try
    {
        PCB_IO_MGR::Save( aFormat, aFileName, aBoard, nullptr );
    }
    catch( ... )
    {
        return false;
    }

    if( !aSkipSettings )
    {
        wxFileName pro = aFileName;
        pro.SetExt( FILEEXT::ProjectFileExtension );
        pro.MakeAbsolute();

        GetSettingsManager()->SaveProjectAs( pro.GetFullPath(), aBoard->GetProject() );
    }

    return true;
}


bool SaveBoard( wxString& aFileName, BOARD* aBoard, bool aSkipSettings )
{
    return SaveBoard( aFileName, aBoard, PCB_IO_MGR::KICAD_SEXP, aSkipSettings );
}


FOOTPRINT_LIBRARY_ADAPTER* getFootprintAdapter()
{
    BOARD* board = GetBoard();

    if( !board )
        return nullptr;

    PROJECT* project = board->GetProject();

    if( !project )
        return nullptr;

    return PROJECT_PCB::FootprintLibAdapter( project );
}


wxArrayString GetFootprintLibraries()
{
    wxArrayString footprintLibraryNames;

    FOOTPRINT_LIBRARY_ADAPTER* adapter = getFootprintAdapter();

    if( !adapter )
        return footprintLibraryNames;

    for( const wxString& name : adapter->GetLibraryNames() )
        footprintLibraryNames.Add( name );

    return footprintLibraryNames;
}


wxArrayString GetFootprints( const wxString& aNickName )
{
    wxArrayString footprintNames;

    FOOTPRINT_LIBRARY_ADAPTER* adapter = getFootprintAdapter();

    if( !adapter )
        return footprintNames;

    std::vector<wxString> names = adapter->GetFootprintNames( aNickName, true );
    footprintNames.assign( names.begin(), names.end() );

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
        DSN::ExportBoardToSpecctraFile( aBoard, aFullFilename );
    }
    catch( ... )
    {
        return false;
    }

    return true;
}


bool ExportVRML( const wxString& aFullFileName, double aMMtoWRMLunit, bool aIncludeUnspecified,
                 bool aIncludeDNP, bool aExport3DFiles,
                 bool aUseRelativePaths, const wxString& a3D_Subdir, double aXRef, double aYRef )
{
    if( s_PcbEditFrame )
    {
        bool ok = s_PcbEditFrame->ExportVRML_File( aFullFileName, aMMtoWRMLunit,
                                                   aIncludeUnspecified, aIncludeDNP,
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

bool ImportSpecctraSES( BOARD* aBoard, wxString& aFullFilename )
{
    try
    {
        DSN::ImportSpecctraSession( aBoard, aFullFilename );
    }
    catch( ... )
    {
        return false;
    }

    return true;
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
                           if( item->IsBOARD_ITEM() )
                               items.push_back( static_cast<BOARD_ITEM*>( item ) );
                       } );
    }

    return items;
}

void FocusOnItem( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer )
{
    if( s_PcbEditFrame )
    {
        s_PcbEditFrame->FocusOnItem( aItem, aLayer );
    }
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
    fn.SetExt( FILEEXT::DesignRulesFileExtension );
    PROJECT* prj = nullptr;

    if( aBoard->GetProject() )
        prj = aBoard->GetProject();
    else if( s_SettingsManager )
        prj = &s_SettingsManager->Prj();

    wxCHECK( prj, false );

    wxString drcRulesPath = prj->AbsolutePath( fn.GetFullName() );

    // Rebuild The Instance of ENUM_MAP<PCB_LAYER_ID> (layer names list), because the DRC
    // engine can use layer names (canonical and/or user names)
    ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();
    layerEnum.Choices().Clear();
    layerEnum.Undefined( UNDEFINED_LAYER );

    for( PCB_LAYER_ID layer : LSET::AllLayersMask() )
    {
        layerEnum.Map( layer, LSET::Name( layer ) );              // Add Canonical name
        layerEnum.Map( layer, aBoard->GetLayerName( layer ) );    // Add User name
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
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2D& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( aItem->GetErrorCode() == DRCE_MISSING_FOOTPRINT
                    || aItem->GetErrorCode() == DRCE_DUPLICATE_FOOTPRINT
                    || aItem->GetErrorCode() == DRCE_EXTRA_FOOTPRINT
                    || aItem->GetErrorCode() == DRCE_NET_CONFLICT
                    || aItem->GetErrorCode() == DRCE_SCHEMATIC_PARITY
                    || aItem->GetErrorCode() == DRCE_SCHEMATIC_FIELDS_PARITY
                    || aItem->GetErrorCode() == DRCE_FOOTPRINT_FILTERS )
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

    aBoard->RecordDRCExclusions();
    aBoard->DeleteMARKERs( true, true );
    engine->RunTests( aUnits, aReportAllTrackErrors, false );
    engine->ClearViolationHandler();

    // Update the exclusion status on any excluded markers that still exist.
    aBoard->ResolveDRCExclusions( false );

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

wxString GetLanguage()
{
    if( s_PcbEditFrame )
        return GetSettingsManager()->GetCommonSettings()->m_System.language;
    else
        return "";
}
