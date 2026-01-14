/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <pcbnew_scripting_helpers.h>
#include <pgm_base.h>
#include <background_jobs_monitor.h>
#include <cli_progress_reporter.h>
#include <confirm.h>
#include <kiface_base.h>
#include <kiface_ids.h>
#include <pcb_edit_frame.h>
#include <eda_dde.h>
#include <macros.h>
#include <wx/snglinst.h>
#include <gestfich.h>
#include <paths.h>
#include <pcbnew_settings.h>
#include <footprint_editor_settings.h>
#include <settings/settings_manager.h>
#include <settings/cvpcb_settings.h>
#include <footprint_library_adapter.h>
#include <footprint_edit_frame.h>
#include <footprint_viewer_frame.h>
#include <footprint_chooser_frame.h>
#include <footprint_wizard_frame.h>
#include <footprint_preview_panel.h>
#include <footprint_info_impl.h>
#include <dialogs/dialog_configure_paths.h>
#include <dialogs/panel_grid_settings.h>
#include <panel_display_options.h>
#include <panel_edit_options.h>
#include <panel_fp_editor_field_defaults.h>
#include <panel_fp_editor_graphics_defaults.h>
#include <panel_fp_editor_color_settings.h>
#include <panel_pcbnew_color_settings.h>
#include <panel_pcbnew_action_plugins.h>
#include <panel_pcbnew_display_origin.h>
#include <panel_3D_display_options.h>
#include <panel_3D_opengl_options.h>
#include <panel_3D_raytracing_options.h>
#include <project_pcb.h>
#include <python_scripting.h>
#include <string_utils.h>
#include <thread_pool.h>
#include <trace_helpers.h>
#include <widgets/kistatusbar.h>

#include <wx/tokenzr.h>

#include "invoke_pcb_dialog.h"
#include <wildcards_and_files_ext.h>
#include "pcbnew_jobs_handler.h"

#include <dialogs/panel_toolbar_customization.h>
#include <3d_viewer/toolbars_3d.h>
#include <toolbars_footprint_editor.h>
#include <toolbars_pcb_editor.h>

#include <wx/crt.h>

/* init functions defined by swig */

extern "C" PyObject* PyInit__pcbnew( void );

namespace PCB {

static struct IFACE : public KIFACE_BASE, public UNITS_PROVIDER
{
    // Of course all are virtual overloads, implementations of the KIFACE.

    IFACE( const char* aName, KIWAY::FACE_T aType ) :
            KIFACE_BASE( aName, aType ),
            UNITS_PROVIDER( pcbIUScale, EDA_UNITS::MM )
    {}

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY* aKiway ) override;

    void Reset() override;

    void OnKifaceEnd() override;

    wxWindow* CreateKiWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway, int aCtlBits = 0 ) override
    {
        switch( aClassId )
        {
        case FRAME_PCB_EDITOR:
        {
            auto frame = new PCB_EDIT_FRAME( aKiway, aParent );

            // give the scripting helpers access to our frame
            ScriptingSetPcbEditFrame( frame );

            if( Kiface().IsSingle() )
            {
                // only run this under single_top, not under a project manager.
                frame->CreateServer( KICAD_PCB_PORT_SERVICE_NUMBER );
            }

            return frame;
        }

        case FRAME_FOOTPRINT_EDITOR:
            return new FOOTPRINT_EDIT_FRAME( aKiway, aParent );

        case FRAME_FOOTPRINT_VIEWER:
            return new FOOTPRINT_VIEWER_FRAME( aKiway, aParent );

        case FRAME_FOOTPRINT_CHOOSER:
            return new FOOTPRINT_CHOOSER_FRAME( aKiway, aParent );

        case FRAME_FOOTPRINT_WIZARD:
            return new FOOTPRINT_WIZARD_FRAME( aKiway, aParent, FRAME_T( aClassId ) );

        case FRAME_FOOTPRINT_PREVIEW:
            return FOOTPRINT_PREVIEW_PANEL::New( aKiway, aParent, this );

        case DIALOG_CONFIGUREPATHS:
        {
            DIALOG_CONFIGURE_PATHS dlg( aParent );

            // The dialog's constructor probably failed to set its Kiway because the
            // dynamic_cast fails when aParent was allocated by a separate compilation
            // module.  So set it directly.
            dlg.SetKiway( &dlg, aKiway );

            // Use QuasiModal so that HTML help window will work
            if( dlg.ShowQuasiModal() == wxID_OK )
                aKiway->CommonSettingsChanged( ENVVARS_CHANGED );

            // Dialog has completed; nothing to return.
            return nullptr;
        }

        case DIALOG_PCB_LIBRARY_TABLE:
            InvokePcbLibTableEditor( aKiway, aParent );
            // Dialog has completed; nothing to return.
            return nullptr;

        case PANEL_FP_DISPLAY_OPTIONS:
            return new PANEL_DISPLAY_OPTIONS( aParent, GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" ) );

        case PANEL_FP_GRIDS:
        {
            FOOTPRINT_EDITOR_SETTINGS* cfg = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );
            EDA_BASE_FRAME*            frame = aKiway->Player( FRAME_FOOTPRINT_EDITOR, false );

            if( !frame )
                frame = aKiway->Player( FRAME_FOOTPRINT_VIEWER, false );

            if( !frame )
                frame = aKiway->Player( FRAME_PCB_EDITOR, false );

            if( frame )
                SetUserUnits( frame->GetUserUnits() );

            return new PANEL_GRID_SETTINGS( aParent, this, frame, cfg, FRAME_FOOTPRINT_EDITOR );
        }

        case PANEL_FP_ORIGINS_AXES:
            return new PANEL_PCBNEW_DISPLAY_ORIGIN( aParent, GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" ),
                                                    FRAME_FOOTPRINT_EDITOR );

        case PANEL_FP_EDIT_OPTIONS:
        {
            EDA_BASE_FRAME* frame = aKiway->Player( FRAME_FOOTPRINT_EDITOR, false );

            if( !frame )
                frame = aKiway->Player( FRAME_FOOTPRINT_VIEWER, false );

            if( !frame )
                frame = aKiway->Player( FRAME_PCB_EDITOR, false );

            if( frame )
                SetUserUnits( frame->GetUserUnits() );

            return new PANEL_EDIT_OPTIONS( aParent, this, frame, true );
        }

        case PANEL_FP_DEFAULT_FIELDS:
        {
            EDA_BASE_FRAME* frame = aKiway->Player( FRAME_FOOTPRINT_EDITOR, false );

            if( !frame )
                frame = aKiway->Player( FRAME_FOOTPRINT_VIEWER, false );

            if( !frame )
                frame = aKiway->Player( FRAME_PCB_EDITOR, false );

            if( frame )
                SetUserUnits( frame->GetUserUnits() );

            return new PANEL_FP_EDITOR_FIELD_DEFAULTS( aParent );
        }

        case PANEL_FP_DEFAULT_GRAPHICS_VALUES:
        {
            EDA_BASE_FRAME* frame = aKiway->Player( FRAME_FOOTPRINT_EDITOR, false );

            if( !frame )
                frame = aKiway->Player( FRAME_FOOTPRINT_VIEWER, false );

            if( !frame )
                frame = aKiway->Player( FRAME_PCB_EDITOR, false );

            if( frame )
                SetUserUnits( frame->GetUserUnits() );

            return new PANEL_FP_EDITOR_GRAPHICS_DEFAULTS( aParent, this );
        }

        case PANEL_FP_TOOLBARS:
        {
            APP_SETTINGS_BASE* cfg = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );
            TOOLBAR_SETTINGS*  tb  = GetToolbarSettings<FOOTPRINT_EDIT_TOOLBAR_SETTINGS>( "fpedit-toolbars" );

            std::vector<TOOL_ACTION*>            actions;
            std::vector<ACTION_TOOLBAR_CONTROL*> controls;

            for( TOOL_ACTION* action : ACTION_MANAGER::GetActionList() )
                actions.push_back( action );

            for( ACTION_TOOLBAR_CONTROL* control : ACTION_TOOLBAR::GetCustomControlList( FRAME_FOOTPRINT_EDITOR ) )
                controls.push_back( control );

            return new PANEL_TOOLBAR_CUSTOMIZATION( aParent, cfg, tb, actions, controls );
        }

        case PANEL_FP_COLORS:
            return new PANEL_FP_EDITOR_COLOR_SETTINGS( aParent );

        case PANEL_PCB_DISPLAY_OPTS:
            return new PANEL_DISPLAY_OPTIONS( aParent, GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ) );

        case PANEL_PCB_GRIDS:
        {
            PCBNEW_SETTINGS*  cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );
            EDA_BASE_FRAME*   frame = aKiway->Player( FRAME_PCB_EDITOR, false );

            if( !frame )
                frame = aKiway->Player( FRAME_FOOTPRINT_EDITOR, false );

            if( !frame )
                frame = aKiway->Player( FRAME_FOOTPRINT_VIEWER, false );

            if( frame )
                SetUserUnits( frame->GetUserUnits() );

            return new PANEL_GRID_SETTINGS( aParent, this, frame, cfg, FRAME_PCB_EDITOR );
        }

        case PANEL_PCB_ORIGINS_AXES:
            return new PANEL_PCBNEW_DISPLAY_ORIGIN( aParent, GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ),
                                                    FRAME_PCB_EDITOR );

        case PANEL_PCB_EDIT_OPTIONS:
        {
            EDA_BASE_FRAME* frame = aKiway->Player( FRAME_PCB_EDITOR, false );

            if( !frame )
                frame = aKiway->Player( FRAME_FOOTPRINT_EDITOR, false );

            if( !frame )
                frame = aKiway->Player( FRAME_FOOTPRINT_VIEWER, false );

            if( frame )
                SetUserUnits( frame->GetUserUnits() );

            return new PANEL_EDIT_OPTIONS( aParent, this, frame, false );
        }

        case PANEL_PCB_COLORS:
        {
            BOARD*          board = nullptr;
            EDA_BASE_FRAME* boardProvider = aKiway->Player( FRAME_PCB_EDITOR, false );

            if( boardProvider )
                board = static_cast<PCB_EDIT_FRAME*>( boardProvider )->GetBoard();

            return new PANEL_PCBNEW_COLOR_SETTINGS( aParent, board );
        }

        case PANEL_PCB_TOOLBARS:
        {
            APP_SETTINGS_BASE* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );
            TOOLBAR_SETTINGS*  tb  = GetToolbarSettings<PCB_EDIT_TOOLBAR_SETTINGS>( "pcbnew-toolbars" );

            std::vector<TOOL_ACTION*>            actions;
            std::vector<ACTION_TOOLBAR_CONTROL*> controls;

            for( TOOL_ACTION* action : ACTION_MANAGER::GetActionList() )
                actions.push_back( action );

            for( ACTION_TOOLBAR_CONTROL* control : ACTION_TOOLBAR::GetCustomControlList( FRAME_PCB_EDITOR ) )
                controls.push_back( control );

            return new PANEL_TOOLBAR_CUSTOMIZATION( aParent, cfg, tb, actions, controls );
        }

        case PANEL_PCB_ACTION_PLUGINS:
            return new PANEL_PCBNEW_ACTION_PLUGINS( aParent );

        case PANEL_3DV_DISPLAY_OPTIONS:
            return new PANEL_3D_DISPLAY_OPTIONS( aParent );

        case PANEL_3DV_OPENGL:
            return new PANEL_3D_OPENGL_OPTIONS( aParent );

        case PANEL_3DV_RAYTRACING:
            return new PANEL_3D_RAYTRACING_OPTIONS( aParent );

        case PANEL_3DV_TOOLBARS:
        {
            APP_SETTINGS_BASE* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" );
            TOOLBAR_SETTINGS*  tb  = GetToolbarSettings<EDA_3D_VIEWER_TOOLBAR_SETTINGS>( "3d_viewer-toolbars" );

            std::vector<TOOL_ACTION*>            actions;
            std::vector<ACTION_TOOLBAR_CONTROL*> controls;

            for( TOOL_ACTION* action : ACTION_MANAGER::GetActionList() )
                actions.push_back( action );

            for( ACTION_TOOLBAR_CONTROL* control : ACTION_TOOLBAR::GetCustomControlList( FRAME_PCB_DISPLAY3D ) )
                controls.push_back( control );

            return new PANEL_TOOLBAR_CUSTOMIZATION( aParent, cfg, tb, actions, controls );
        }

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
     * @return the object which must be cast into the know type.
     */
    void* IfaceOrAddress( int aDataId ) override
    {
        switch( aDataId )
        {
        // Return a pointer to the global instance of the footprint list.
        case KIFACE_FOOTPRINT_LIST:
            return (void*) &GFootprintList;

        case KIFACE_FOOTPRINT_LIBRARY_ADAPTER:
            // This is the mechanism by which FOOTPRINT_SELECT_WIDGET can get access to the adapter
            // without directly linking to pcbnew or pcbcommon, going through PROJECT::FootprintLibAdapter
            // TODO this is kind of cursed and needs thought to support multi-project
            return PROJECT_PCB::FootprintLibAdapter( &Pgm().GetSettingsManager().Prj() );

        case KIFACE_SCRIPTING_LEGACY:
            return reinterpret_cast<void*>( PyInit__pcbnew );

        default:
            return nullptr;
        }
    }

    /**
     * Saving a file under a different name is delegated to the various KIFACEs because
     * the project doesn't know the internal format of the various files (which may have
     * paths in them that need updating).
     */
    void SaveFileAs( const wxString& aProjectBasePath, const wxString& aSrcProjectName,
                     const wxString& aNewProjectBasePath, const wxString& aNewProjectName,
                     const wxString& aSrcFilePath, wxString& aErrors ) override;

    int HandleJob( JOB* aJob, REPORTER* aReporter, PROGRESS_REPORTER* aProgressReporter ) override;

    bool HandleJobConfig( JOB* aJob, wxWindow* aParent ) override;

    void PreloadLibraries( KIWAY* aKiway ) override;
    void ProjectChanged() override;
    void CancelPreload( bool aBlock = true ) override;

private:
    std::unique_ptr<PCBNEW_JOBS_HANDLER> m_jobHandler;
    std::shared_ptr<BACKGROUND_JOB>      m_libraryPreloadBackgroundJob;
    std::future<void>                    m_libraryPreloadReturn;
    std::atomic_bool                     m_libraryPreloadInProgress;
    std::atomic_bool                     m_libraryPreloadAbort;

} kiface( "pcbnew", KIWAY::FACE_PCB );

} // namespace


using namespace PCB;


KIFACE_BASE& Kiface() { return kiface; }


// KIFACE_GETTER's actual spelling is a substitution macro found in kiway.h.
// KIFACE_GETTER will not have name mangling due to declaration in kiway.h.
KIFACE_API KIFACE* KIFACE_GETTER( int* aKIFACEversion, int aKiwayVersion, PGM_BASE* aProgram )
{
    return &kiface;
}


/// The global footprint info table.  This is performance-intensive to build so we
/// keep a hash-stamped global version.  Any deviation from the request vs. stored
/// hash will result in it being rebuilt.
FOOTPRINT_LIST_IMPL   GFootprintList;


bool IFACE::OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY* aKiway )
{
    // This is process-level-initialization, not project-level-initialization of the DSO.
    // Do nothing in here pertinent to a project!
    InitSettings( new PCBNEW_SETTINGS );

    SETTINGS_MANAGER& mgr = aProgram->GetSettingsManager();

    mgr.RegisterSettings( new FOOTPRINT_EDITOR_SETTINGS );
    mgr.RegisterSettings( new EDA_3D_VIEWER_SETTINGS );

    // We intentionally register KifaceSettings after FOOTPRINT_EDITOR_SETTINGS and EDA_3D_VIEWER_SETTINGS
    // In legacy configs, many settings were in a single editor config and the migration routine
    // for the main editor file will try and call into the now separate settings stores
    // to move the settings into them
    mgr.RegisterSettings( KifaceSettings() );

    // Register the footprint editor settings as well because they share a KiFACE and need to be
    // loaded prior to use to avoid threading deadlocks
    mgr.RegisterSettings( new CVPCB_SETTINGS );

    start_common( aCtlBits );

    m_jobHandler = std::make_unique<PCBNEW_JOBS_HANDLER>( aKiway );

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


void IFACE::OnKifaceEnd()
{
    end_common();
}


void IFACE::SaveFileAs( const wxString& aProjectBasePath, const wxString& aSrcProjectName,
                        const wxString& aNewProjectBasePath, const wxString& aNewProjectName,
                        const wxString& aSrcFilePath, wxString& aErrors )
{
    wxFileName destFile( aSrcFilePath );
    wxString   destPath = destFile.GetPathWithSep();
    wxUniChar  pathSep = wxFileName::GetPathSeparator();
    wxString   ext = destFile.GetExt();

    if( destPath.StartsWith( aProjectBasePath + pathSep ) )
        destPath.Replace( aProjectBasePath, aNewProjectBasePath, false );

    wxString srcProjectFootprintLib = pathSep + aSrcProjectName + wxT( ".pretty" ) + pathSep;
    wxString newProjectFootprintLib = pathSep + aNewProjectName + wxT( ".pretty" ) + pathSep;

    destPath.Replace( srcProjectFootprintLib, newProjectFootprintLib, true );

    destFile.SetPath( destPath );

    if( ext == FILEEXT::KiCadPcbFileExtension
        || ext == FILEEXT::KiCadPcbFileExtension + FILEEXT::BackupFileSuffix )
    {
        if( destFile.GetName() == aSrcProjectName )
            destFile.SetName( aNewProjectName  );

        CopySexprFile( aSrcFilePath, destFile.GetFullPath(),
                [&]( const std::string& token, wxString& value )
                {
                    if( token == "sheetfile" )
                    {
                        for( const wxString& extension : { wxT( ".sch" ), wxT( ".kicad_sch" ) } )
                        {
                            if( value == aSrcProjectName + extension )
                            {
                                value = aNewProjectName + extension;
                                return true;
                            }
                            else if( value == aProjectBasePath + "/" + aSrcProjectName + extension )
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
    else if( ext == FILEEXT::LegacyPcbFileExtension )
    {
        if( destFile.GetName() == aSrcProjectName )
            destFile.SetName( aNewProjectName );

        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
    }
    else if( ext == FILEEXT::LegacyFootprintLibPathExtension
             || ext == FILEEXT::KiCadFootprintFileExtension )
    {
        // Footprints are not project-specific.  Keep their source names.
        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
    }
    else if( ext == FILEEXT::FootprintAssignmentFileExtension )
    {
        // TODO
    }
    else if( ext == wxT( "rpt" ) )
    {
        // DRC must be the "gold standard".  Since we can't guarantee that there aren't
        // any non-deterministic cases in the save-as algorithm, we don't want to certify
        // the result with the source's DRC report.  Therefore copy it under the old
        // name.
        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
    }
    else if( destFile.GetName() == FILEEXT::FootprintLibraryTableFileName )
    {
        wxFileName    libTableFn( aSrcFilePath );
        LIBRARY_TABLE libTable( libTableFn, LIBRARY_TABLE_SCOPE::PROJECT );
        libTable.SetPath( destFile.GetFullPath() );
        libTable.SetType( LIBRARY_TABLE_TYPE::FOOTPRINT );

        for( LIBRARY_TABLE_ROW& row : libTable.Rows() )
        {
            wxString uri = row.URI();

            uri.Replace( wxT( "/" ) + aSrcProjectName + wxT( ".pretty" ),
                         wxT( "/" ) + aNewProjectName + wxT( ".pretty" ) );

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
        wxFAIL_MSG( wxT( "Unexpected filetype for Pcbnew::SaveFileAs()" ) );
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


void IFACE::PreloadLibraries( KIWAY* aKiway )
{
    constexpr static int interval = 150;
    constexpr static int timeLimit = 120000;

    wxCHECK( aKiway, /* void */ );

    if( m_libraryPreloadInProgress.load() )
        return;

    Pgm().ClearLibraryLoadMessages();

    m_libraryPreloadBackgroundJob =
            Pgm().GetBackgroundJobMonitor().Create( _( "Loading Footprint Libraries" ) );

    auto preload =
        [this, aKiway]() -> void
        {
            std::shared_ptr<BACKGROUND_JOB_REPORTER> reporter =
                    m_libraryPreloadBackgroundJob->m_reporter;

            FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &aKiway->Prj() );

            int elapsed = 0;
            bool aborted = false;

            reporter->Report( _( "Loading Footprint Libraries" ) );
            adapter->AsyncLoad();

            while( true )
            {
                if( m_libraryPreloadAbort.load() )
                {
                    aborted = true;
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

            // Check again after blocking - abort may have been requested while we were waiting
            if( m_libraryPreloadAbort.load() )
                aborted = true;

            // If aborted, skip operations that use the adapter since the project may have changed
            // and the adapter's project reference could be stale. This prevents use-after-free
            // crashes when switching projects during library preload.
            if( !aborted )
            {
                // Collect and report library load errors from adapter
                wxString errors = adapter->GetLibraryLoadErrors();

                wxLogTrace( traceLibraries,
                            "pcbnew PreloadLibraries: adapter errors.IsEmpty()=%d, length=%zu",
                            errors.IsEmpty(), errors.length() );

                if( !errors.IsEmpty() )
                {
                    std::vector<LOAD_MESSAGE> messages =
                            ExtractLibraryLoadErrors( errors, RPT_SEVERITY_ERROR );

                    wxLogTrace( traceLibraries, "  -> adapter: collected %zu messages",
                                messages.size() );

                    if( !messages.empty() )
                        Pgm().AddLibraryLoadMessages( messages );
                }
                else
                {
                    wxLogTrace( traceLibraries, "  -> no errors from footprint adapter" );
                }

                // TODO: Remove once fp-info-cache isn't a thing
                GFootprintList.ReadFootprintFiles( adapter, nullptr, reporter.get() );

                // Also collect errors from GFootprintList
                wxLogTrace( traceLibraries, "  -> GFootprintList.GetErrorCount()=%u",
                            GFootprintList.GetErrorCount() );

                if( GFootprintList.GetErrorCount() > 0 )
                {
                    std::vector<LOAD_MESSAGE> messages =
                            ExtractLibraryLoadErrors( GFootprintList.GetErrorMessages(),
                                                      RPT_SEVERITY_ERROR );

                    wxLogTrace( traceLibraries, "  -> GFootprintList: collected %zu messages",
                                messages.size() );

                    if( !messages.empty() )
                        Pgm().AddLibraryLoadMessages( messages );
                }
                else
                {
                    wxLogTrace( traceLibraries, "  -> no errors from GFootprintList" );
                }
            }
            else
            {
                wxLogTrace( traceLibraries, "pcbnew PreloadLibraries: aborted, skipping footprint processing" );
            }

            m_libraryPreloadAbort.store( false );
            Pgm().GetBackgroundJobMonitor().Remove( m_libraryPreloadBackgroundJob );
            m_libraryPreloadBackgroundJob.reset();
            m_libraryPreloadInProgress.store( false );

            // Only send reload notifications if we weren't aborted
            if( !aborted )
            {
                std::string payload = "";
                aKiway->ExpressMail( FRAME_PCB_EDITOR, MAIL_RELOAD_LIB, payload, nullptr, true );
                aKiway->ExpressMail( FRAME_FOOTPRINT_EDITOR, MAIL_RELOAD_LIB, payload, nullptr, true );
                aKiway->ExpressMail( FRAME_CVPCB, MAIL_RELOAD_LIB, payload, nullptr, true );
            }
        };

    thread_pool& tp = GetKiCadThreadPool();
    m_libraryPreloadInProgress.store( true );
    m_libraryPreloadReturn = tp.submit_task( preload );
}


void IFACE::ProjectChanged()
{
    if( m_libraryPreloadInProgress.load() )
        m_libraryPreloadAbort.store( true );
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
