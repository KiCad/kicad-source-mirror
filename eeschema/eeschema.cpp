/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <core/json_serializers.h>
#include <pgm_base.h>
#include <kiface_base.h>
#include <background_jobs_monitor.h>
#include <cli_progress_reporter.h>
#include <confirm.h>
#include <gestfich.h>
#include <eda_dde.h>
#include "eeschema_jobs_handler.h"
#include "eeschema_helpers.h"
#include <eeschema_settings.h>
#include <sch_edit_frame.h>
#include <libraries/symbol_library_adapter.h>
#include <symbol_edit_frame.h>
#include <symbol_viewer_frame.h>
#include <symbol_chooser_frame.h>
#include <dialogs/panel_grid_settings.h>
#include <dialogs/panel_simulator_preferences.h>
#include <dialogs/panel_design_block_lib_table.h>
#include <dialogs/panel_sym_lib_table.h>
#include <kiway.h>
#include <project_sch.h>
#include <richio.h>
#include <settings/settings_manager.h>
#include <symbol_editor_settings.h>
#include <sexpr/sexpr.h>
#include <sexpr/sexpr_parser.h>
#include <string_utils.h>
#include <trace_helpers.h>
#include <thread_pool.h>
#include <kiface_ids.h>
#include <widgets/kistatusbar.h>
#include <netlist_exporters/netlist_exporter_kicad.h>
#include <wx/ffile.h>
#include <wx/tokenzr.h>
#include <wildcards_and_files_ext.h>

#include <schematic.h>
#include <connection_graph.h>
#include <panel_template_fieldnames.h>
#include <panel_eeschema_color_settings.h>
#include <panel_sch_data_sources.h>
#include <panel_sym_color_settings.h>
#include <panel_eeschema_editing_options.h>
#include <panel_eeschema_annotation_options.h>
#include <panel_sym_editing_options.h>
#include <dialogs/panel_base_display_options.h>
#include <panel_eeschema_display_options.h>
#include <panel_sym_display_options.h>
#include <sim/simulator_frame.h>

#include <dialogs/panel_toolbar_customization.h>
#include <toolbars_sch_editor.h>
#include <toolbars_symbol_editor.h>

#include <wx/crt.h>

// The main sheet of the project
SCH_SHEET*  g_RootSheet = nullptr;


namespace SCH {


// TODO: This should move out of this file
static std::unique_ptr<SCHEMATIC> readSchematicFromFile( const std::string& aFilename )
{
    SCH_IO* pi = SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD );
    std::unique_ptr<SCHEMATIC> schematic = std::make_unique<SCHEMATIC>( nullptr );

    SETTINGS_MANAGER& manager = Pgm().GetSettingsManager();

    // TODO: this must load the schematic's project, not a default project.  At the very minimum
    // variable resolution won't work without the project, but there might also be issues with
    // netclasses, etc.
    manager.LoadProject( "" );
    schematic->Reset();
    schematic->SetProject( &manager.Prj() );
    SCH_SHEET* rootSheet = pi->LoadSchematicFile( aFilename, schematic.get() );

    if( !rootSheet )
        return nullptr;

    schematic->SetTopLevelSheets( { rootSheet } );

    SCH_SCREENS screens( schematic->Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
        screen->UpdateLocalLibSymbolLinks();

    SCH_SHEET_LIST sheets = schematic->Hierarchy();

    // Restore all of the loaded symbol instances from the root sheet screen.
    sheets.UpdateSymbolInstanceData( schematic->RootScreen()->GetSymbolInstances() );

    if( schematic->RootScreen()->GetFileFormatVersionAtLoad() < 20230221 )
    {
        for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
            screen->FixLegacyPowerSymbolMismatches();
    }

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
        screen->MigrateSimModels();

    sheets.AnnotatePowerSymbols();

    // NOTE: This is required for multi-unit symbols to be correct
    for( SCH_SHEET_PATH& sheet : sheets )
        sheet.UpdateAllScreenReferences();

    // TODO: this must handle SchematicCleanup somehow.  The original version didn't because
    // it knew that QA test cases were saved in a clean state.

    // TODO: does this need to handle PruneOrphanedSymbolInstances() and
    // PruneOrphanedSheetInstances()?

    schematic->ConnectionGraph()->Recalculate( sheets, true );

    return schematic;
}


// TODO: This should move out of this file
bool generateSchematicNetlist( const wxString& aFilename, std::string& aNetlist )
{
    std::unique_ptr<SCHEMATIC> schematic = readSchematicFromFile( aFilename.ToStdString() );
    NETLIST_EXPORTER_KICAD exporter( schematic.get() );
    STRING_FORMATTER formatter;

    exporter.Format( &formatter, GNL_ALL | GNL_OPT_KICAD );
    aNetlist = formatter.GetString();

    return true;
}


static struct IFACE : public KIFACE_BASE, public UNITS_PROVIDER
{
    // Of course all are virtual overloads, implementations of the KIFACE.

    IFACE( const char* aName, KIWAY::FACE_T aType ) :
            KIFACE_BASE( aName, aType ),
            UNITS_PROVIDER( schIUScale, EDA_UNITS::MM ),
            m_libraryPreloadInProgress( false )
    {}

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY* aKiway ) override;

    void Reset() override;

    void OnKifaceEnd() override;

    wxWindow* CreateKiWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway, int aCtlBits = 0 ) override
    {
        switch( aClassId )
        {
        case FRAME_SCH:
        {
            SCH_EDIT_FRAME* frame = new SCH_EDIT_FRAME( aKiway, aParent );

            EESCHEMA_HELPERS::SetSchEditFrame( frame );

            if( Kiface().IsSingle() )
            {
                // only run this under single_top, not under a project manager.
                frame->CreateServer( KICAD_SCH_PORT_SERVICE_NUMBER );
            }

            return frame;
        }

        case FRAME_SCH_SYMBOL_EDITOR:
            return new SYMBOL_EDIT_FRAME( aKiway, aParent );

        case FRAME_SIMULATOR:
        {
            try
            {
                SIMULATOR_FRAME* frame = new SIMULATOR_FRAME( aKiway, aParent );
                return frame;
            }
            catch( const SIMULATOR_INIT_ERR& )
            {
                // catch the init err exception as we don't want it to bubble up
                // its going to be some ngspice install issue but we don't want to log that
                return nullptr;
            }
        }

        case FRAME_SCH_VIEWER:
            return new SYMBOL_VIEWER_FRAME( aKiway, aParent );

        case FRAME_SYMBOL_CHOOSER:
        {
            bool cancelled = false;
            SYMBOL_CHOOSER_FRAME* chooser = new SYMBOL_CHOOSER_FRAME( aKiway, aParent, cancelled );

            if( cancelled )
            {
                chooser->Destroy();
                return nullptr;
            }

            return chooser;
        }

        case DIALOG_SCH_LIBRARY_TABLE:
            InvokeSchEditSymbolLibTable( aKiway, aParent );
            // Dialog has completed; nothing to return.
            return nullptr;

        case DIALOG_DESIGN_BLOCK_LIBRARY_TABLE:
            InvokeEditDesignBlockLibTable( aKiway, aParent );
            // Dialog has completed; nothing to return.
            return nullptr;

        case PANEL_SYM_DISP_OPTIONS:
            return new PANEL_SYM_DISPLAY_OPTIONS( aParent, GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" ) );

        case PANEL_SYM_EDIT_GRIDS:
        {
            APP_SETTINGS_BASE* cfg = GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" );
            EDA_BASE_FRAME*    frame = aKiway->Player( FRAME_SCH_SYMBOL_EDITOR, false );

            if( !frame )
                frame = aKiway->Player( FRAME_SCH_VIEWER, false );

            if( !frame )
                frame = aKiway->Player( FRAME_SCH, false );

            if( frame )
                SetUserUnits( frame->GetUserUnits() );

            return new PANEL_GRID_SETTINGS( aParent, this, frame, cfg, FRAME_SCH_SYMBOL_EDITOR );
        }

        case PANEL_SYM_EDIT_OPTIONS:
        {
            EDA_BASE_FRAME* frame = aKiway->Player( FRAME_SCH_SYMBOL_EDITOR, false );

            if( !frame )
                frame = aKiway->Player( FRAME_SCH_VIEWER, false );

            if( !frame )
                frame = aKiway->Player( FRAME_SCH, false );

            if( frame )
                SetUserUnits( frame->GetUserUnits() );

            return new PANEL_SYM_EDITING_OPTIONS( aParent, this, frame );
        }

        case PANEL_SYM_TOOLBARS:
        {
            APP_SETTINGS_BASE* cfg = GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" );
            TOOLBAR_SETTINGS*  tb  = GetToolbarSettings<SYMBOL_EDIT_TOOLBAR_SETTINGS>( "symbol_editor-toolbars" );

            std::vector<TOOL_ACTION*>            actions;
            std::vector<ACTION_TOOLBAR_CONTROL*> controls;

            for( TOOL_ACTION* action : ACTION_MANAGER::GetActionList() )
                actions.push_back( action );

            for( ACTION_TOOLBAR_CONTROL* control : ACTION_TOOLBAR::GetCustomControlList( FRAME_SCH_SYMBOL_EDITOR ) )
                controls.push_back( control );

            return new PANEL_TOOLBAR_CUSTOMIZATION( aParent, cfg, tb, actions, controls );
        }

        case PANEL_SYM_COLORS:
            return new PANEL_SYM_COLOR_SETTINGS( aParent );

        case PANEL_SCH_DISP_OPTIONS:
            return new PANEL_EESCHEMA_DISPLAY_OPTIONS( aParent, GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) );

        case PANEL_SCH_GRIDS:
        {
            EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );
            EDA_BASE_FRAME*    frame = aKiway->Player( FRAME_SCH, false );

            if( !frame )
                frame = aKiway->Player( FRAME_SCH_SYMBOL_EDITOR, false );

            if( !frame )
                frame = aKiway->Player( FRAME_SCH_VIEWER, false );

            if( frame )
                SetUserUnits( frame->GetUserUnits() );

            return new PANEL_GRID_SETTINGS( aParent, this, frame, cfg, FRAME_SCH );
        }

        case PANEL_SCH_EDIT_OPTIONS:
        {
            EDA_BASE_FRAME* frame = aKiway->Player( FRAME_SCH, false );

            if( !frame )
                frame = aKiway->Player( FRAME_SCH_SYMBOL_EDITOR, false );

            if( !frame )
                frame = aKiway->Player( FRAME_SCH_VIEWER, false );

            if( frame )
                SetUserUnits( frame->GetUserUnits() );

            return new PANEL_EESCHEMA_EDITING_OPTIONS( aParent, this, frame );
        }

        case PANEL_SCH_TOOLBARS:
        {
            APP_SETTINGS_BASE* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );
            TOOLBAR_SETTINGS*  tb  = GetToolbarSettings<SCH_EDIT_TOOLBAR_SETTINGS>( "eeschema-toolbars" );

            std::vector<TOOL_ACTION*>            actions;
            std::vector<ACTION_TOOLBAR_CONTROL*> controls;

            for( TOOL_ACTION* action : ACTION_MANAGER::GetActionList() )
                actions.push_back( action );

            for( ACTION_TOOLBAR_CONTROL* control : ACTION_TOOLBAR::GetCustomControlList( FRAME_SCH ) )
                controls.push_back( control );

            return new PANEL_TOOLBAR_CUSTOMIZATION( aParent, cfg, tb, actions, controls );
        }

        case PANEL_SCH_COLORS:
            return new PANEL_EESCHEMA_COLOR_SETTINGS( aParent );

        case PANEL_SCH_FIELD_NAME_TEMPLATES:
            return new PANEL_TEMPLATE_FIELDNAMES( aParent, nullptr );

        case PANEL_SCH_DATA_SOURCES:
        {
            EDA_BASE_FRAME* frame = aKiway->Player( FRAME_SCH, false );

            if( !frame )
                frame = aKiway->Player( FRAME_SCH_SYMBOL_EDITOR, false );

            if( !frame )
                frame = aKiway->Player( FRAME_SCH_VIEWER, false );

            return new class PANEL_SCH_DATA_SOURCES( aParent, frame );
        }

        case PANEL_SCH_SIMULATOR:
            return new PANEL_SIMULATOR_PREFERENCES( aParent );

        default:
            return nullptr;
        }
    }

    /**
     * Return a pointer to the requested object.
     *
     * The safest way to use this is to retrieve a pointer to a static instance of an interface,
     * similar to how the KIFACE interface is exported.  But if you know what you are doing use
     * it to retrieve anything you want.
     *
     * @param aDataId identifies which object you want the address of.
     * @return the object requested and must be cast into the know type.
     */
    void* IfaceOrAddress( int aDataId ) override
    {
        switch( aDataId )
        {
            case KIFACE_NETLIST_SCHEMATIC:
                return (void*) generateSchematicNetlist;
        }

        return nullptr;
    }

    /**
     * Saving a file under a different name is delegated to the various KIFACEs because
     * the project doesn't know the internal format of the various files (which may have
     * paths in them that need updating).
     */
    void SaveFileAs( const wxString& aProjectBasePath, const wxString& aProjectName,
                     const wxString& aNewProjectBasePath, const wxString& aNewProjectName,
                     const wxString& aSrcFilePath, wxString& aErrors ) override;


    int HandleJob( JOB* aJob, REPORTER* aReporter, PROGRESS_REPORTER* aProgressReporter ) override;

    bool HandleJobConfig( JOB* aJob, wxWindow* aParent ) override;

    void PreloadLibraries( KIWAY* aKiway ) override;
    void CancelPreload( bool aBlock = true ) override;
    void ProjectChanged() override;

private:
    std::unique_ptr<EESCHEMA_JOBS_HANDLER> m_jobHandler;
    std::shared_ptr<BACKGROUND_JOB>        m_libraryPreloadBackgroundJob;
    std::future<void>                      m_libraryPreloadReturn;
    std::atomic_bool                       m_libraryPreloadInProgress;
    std::atomic_bool                       m_libraryPreloadAbort;

} kiface( "eeschema", KIWAY::FACE_SCH );

} // namespace

using namespace SCH;


KIFACE_BASE& Kiface() { return kiface; }


// KIFACE_GETTER's actual spelling is a substitution macro found in kiway.h.
// KIFACE_GETTER will not have name mangling due to declaration in kiway.h.
KIFACE_API KIFACE* KIFACE_GETTER(  int* aKIFACEversion, int aKiwayVersion, PGM_BASE* aProgram )
{
    return &kiface;
}


bool IFACE::OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY* aKiway )
{
    // This is process-level-initialization, not project-level-initialization of the DSO.
    // Do nothing in here pertinent to a project!
    InitSettings( new EESCHEMA_SETTINGS );

    // Register the symbol editor settings as well because they share a KiFACE and need to be
    // loaded prior to use to avoid threading deadlocks
    SYMBOL_EDITOR_SETTINGS* symSettings = new SYMBOL_EDITOR_SETTINGS();
    aProgram->GetSettingsManager().RegisterSettings( symSettings ); // manager takes ownership

    // We intentionally register KifaceSettings after SYMBOL_EDITOR_SETTINGS
    // In legacy configs, many settings were in a single editor config nd the migration routine
    // for the main editor file will try and call into the now separate settings stores
    // to move the settings into them
    aProgram->GetSettingsManager().RegisterSettings( KifaceSettings() );

    start_common( aCtlBits );

    m_jobHandler = std::make_unique<EESCHEMA_JOBS_HANDLER>( aKiway );

    if( m_start_flags & KFCTL_CLI )
    {
        m_jobHandler->SetReporter( &CLI_REPORTER::GetInstance() );
        m_jobHandler->SetProgressReporter( &CLI_PROGRESS_REPORTER::GetInstance() );
    }

    return true;
}


void IFACE::Reset()
{
}


void IFACE::PreloadLibraries( KIWAY* aKiway )
{
    constexpr static int interval = 150;
    constexpr static int timeLimit = 120000;

    wxCHECK( aKiway, /* void */ );

    if( m_libraryPreloadInProgress.load() )
        return;

    Pgm().ClearLibraryLoadMessages();

    m_libraryPreloadBackgroundJob =
            Pgm().GetBackgroundJobMonitor().Create( _( "Loading Symbol Libraries" ) );

    auto preload =
        [this, aKiway]() -> void
        {
            std::shared_ptr<BACKGROUND_JOB_REPORTER> reporter =
                    m_libraryPreloadBackgroundJob->m_reporter;

            SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &aKiway->Prj() );

            int elapsed = 0;

            reporter->Report( _( "Loading Symbol Libraries" ) );
            adapter->AsyncLoad();

            while( true )
            {
                if( m_libraryPreloadAbort.load() )
                {
                    m_libraryPreloadAbort.store( false );
                    break;
                }

                std::this_thread::sleep_for( std::chrono::milliseconds( interval ) );

                if( std::optional<float> loadStatus = adapter->AsyncLoadProgress() )
                {
                    float progress = *loadStatus;
                    reporter->SetCurrentProgress( progress );

                    if( progress >= 1 )
                        break;
                }
                else
                {
                    reporter->SetCurrentProgress( 1 );
                    break;
                }

                elapsed += interval;

                if( elapsed > timeLimit )
                    break;
            }

            adapter->BlockUntilLoaded();

            // Collect library load errors for async reporting
            wxString errors = adapter->GetLibraryLoadErrors();

            wxLogTrace( traceLibraries, "eeschema PreloadLibraries: errors.IsEmpty()=%d, length=%zu",
                        errors.IsEmpty(), errors.length() );

            std::vector<LOAD_MESSAGE> messages =
                    ExtractLibraryLoadErrors( errors, RPT_SEVERITY_ERROR );

            if( !messages.empty() )
            {
                wxLogTrace( traceLibraries, "  -> collected %zu messages, calling AddLibraryLoadMessages",
                            messages.size() );
                Pgm().AddLibraryLoadMessages( messages );
            }
            else
            {
                wxLogTrace( traceLibraries, "  -> no errors from symbol libraries" );
            }

            Pgm().GetBackgroundJobMonitor().Remove( m_libraryPreloadBackgroundJob );
            m_libraryPreloadBackgroundJob.reset();
            m_libraryPreloadInProgress.store( false );

            std::string payload = "";
            aKiway->ExpressMail( FRAME_SCH, MAIL_RELOAD_LIB, payload, nullptr, true );
            aKiway->ExpressMail( FRAME_SCH_SYMBOL_EDITOR, MAIL_RELOAD_LIB, payload, nullptr, true );
            aKiway->ExpressMail( FRAME_SCH_VIEWER, MAIL_RELOAD_LIB, payload, nullptr, true );
        };

    thread_pool& tp = GetKiCadThreadPool();
    m_libraryPreloadInProgress.store( true );
    m_libraryPreloadReturn = tp.submit_task( preload );
}


void IFACE::CancelPreload( bool aBlock )
{
    if( m_libraryPreloadInProgress.load() )
    {
        m_libraryPreloadAbort.store( true );

        if( aBlock )
            m_libraryPreloadReturn.wait();
    }
}


void IFACE::ProjectChanged()
{
    if( m_libraryPreloadInProgress.load() )
        m_libraryPreloadAbort.store( true );
}


void IFACE::OnKifaceEnd()
{
    end_common();
}


void IFACE::SaveFileAs( const wxString& aProjectBasePath, const wxString& aProjectName,
                        const wxString& aNewProjectBasePath, const wxString& aNewProjectName,
                        const wxString& aSrcFilePath, wxString& aErrors )
{
    wxFileName destFile( aSrcFilePath );
    wxString   destPath = destFile.GetPathWithSep();
    wxUniChar  pathSep = wxFileName::GetPathSeparator();
    wxString   ext = destFile.GetExt();

    if( destPath.StartsWith( aProjectBasePath + pathSep ) )
        destPath.Replace( aProjectBasePath, aNewProjectBasePath, false );

    destFile.SetPath( destPath );

    if( ext == FILEEXT::LegacySchematicFileExtension
        || ext == FILEEXT::LegacySchematicFileExtension + FILEEXT::BackupFileSuffix
        || ext == FILEEXT::KiCadSchematicFileExtension
        || ext == FILEEXT::KiCadSchematicFileExtension + FILEEXT::BackupFileSuffix )
    {
        if( destFile.GetName() == aProjectName )
        {
            destFile.SetName( aNewProjectName  );
        }
        else if( destFile.GetName() == aNewProjectName )
        {
            wxString msg;

            if( !aErrors.empty() )
                aErrors += wxS( "\n" );

            msg.Printf( _( "Cannot copy file '%s' as it will be overwritten by the new root "
                           "sheet file." ), destFile.GetFullPath() );
            aErrors += msg;
            return;
        }

        CopySexprFile( aSrcFilePath, destFile.GetFullPath(),
                [&]( const std::string& token, wxString& value ) -> bool
                {
                    if( token == "project" && value == aProjectName )
                    {
                        value = aNewProjectName;
                        return true;
                    }

                    return false;
                },
                aErrors );
    }
    else if( ext == FILEEXT::SchematicSymbolFileExtension )
    {
        // Symbols are not project-specific.  Keep their source names.
        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
    }
    else if( ext == FILEEXT::LegacySymbolLibFileExtension
             || ext == FILEEXT::LegacySymbolDocumentFileExtension
             || ext == FILEEXT::KiCadSymbolLibFileExtension )
    {
        if( destFile.GetName() == aProjectName + wxS( "-cache" ) )
            destFile.SetName( aNewProjectName + wxS( "-cache" ) );

        if( destFile.GetName() == aProjectName + wxS( "-rescue" ) )
            destFile.SetName( aNewProjectName + wxS( "-rescue" ) );

        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
    }
    else if( ext == FILEEXT::NetlistFileExtension )
    {
        if( destFile.GetName() == aProjectName )
            destFile.SetName( aNewProjectName );

        CopySexprFile( aSrcFilePath, destFile.GetFullPath(),
                [&]( const std::string& token, wxString& value ) -> bool
                {
                    if( token == "source" )
                    {
                        for( const wxString& extension : { wxString( wxT( ".sch" ) ), wxString( wxT( ".kicad_sch" ) ) } )
                        {
                            if( value == aProjectName + extension )
                            {
                                value = aNewProjectName + extension;
                                return true;
                            }
                            else if( value == aProjectBasePath + "/" + aProjectName + extension )
                            {
                                value = aNewProjectBasePath + "/" + aNewProjectName + extension;
                                return true;
                            }
                            else if( value.StartsWith( aProjectBasePath ) )
                            {
                                value.Replace( aProjectBasePath, aNewProjectBasePath, false );
                                return true;
                            }
                        }
                    }

                    return false;
                },
                aErrors );
    }
    else if( destFile.GetName() == FILEEXT::SymbolLibraryTableFileName )
    {
        wxFileName    libTableFn( aSrcFilePath );
        LIBRARY_TABLE libTable( libTableFn, LIBRARY_TABLE_SCOPE::PROJECT );
        libTable.SetPath( destFile.GetFullPath() );
        libTable.SetType( LIBRARY_TABLE_TYPE::SYMBOL );

        for( LIBRARY_TABLE_ROW& row : libTable.Rows() )
        {
            wxString uri = row.URI();

            uri.Replace( wxS( "/" ) + aProjectName + wxS( "-cache.lib" ),
                         wxS( "/" ) + aNewProjectName + wxS( "-cache.lib" ) );
            uri.Replace( wxS( "/" ) + aProjectName + wxS( "-rescue.lib" ),
                         wxS( "/" ) + aNewProjectName +  wxS( "-rescue.lib" ) );
            uri.Replace( wxS( "/" ) + aProjectName + wxS( ".lib" ),
                         wxS( "/" ) + aNewProjectName + wxS( ".lib" ) );

            row.SetURI( uri );
        }

        libTable.Save().map_error(
                [&]( const LIBRARY_ERROR& aError )
                {
                        wxString msg;

                        if( !aErrors.empty() )
                            aErrors += wxT( "\n" );

                        msg.Printf( _( "Cannot copy file '%s'." ), destFile.GetFullPath() );
                        aErrors += msg;
                } );
    }
    else
    {
        wxFAIL_MSG( wxS( "Unexpected filetype for Eeschema::SaveFileAs()" ) );
    }
}


int IFACE::HandleJob( JOB* aJob, REPORTER* aReporter, PROGRESS_REPORTER* aProgressReporter )
{
    return m_jobHandler->RunJob( aJob, aReporter, aProgressReporter );
}


bool IFACE::HandleJobConfig( JOB* aJob, wxWindow* aParent )
{
    return m_jobHandler->HandleJobConfig( aJob, aParent );
}
