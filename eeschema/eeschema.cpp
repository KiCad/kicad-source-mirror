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

#include <pgm_base.h>
#include <kiface_base.h>
#include <cli_progress_reporter.h>
#include <confirm.h>
#include <gestfich.h>
#include <eda_dde.h>
#include "eeschema_jobs_handler.h"
#include "eeschema_helpers.h"
#include <eeschema_settings.h>
#include <sch_edit_frame.h>
#include <design_block_lib_table.h>
#include <symbol_edit_frame.h>
#include <symbol_viewer_frame.h>
#include <symbol_chooser_frame.h>
#include <symbol_lib_table.h>
#include <dialogs/dialog_global_design_block_lib_table_config.h>
#include <dialogs/dialog_global_sym_lib_table_config.h>
#include <dialogs/panel_grid_settings.h>
#include <dialogs/panel_simulator_preferences.h>
#include <dialogs/panel_design_block_lib_table.h>
#include <dialogs/panel_sym_lib_table.h>
#include <kiway.h>
#include <settings/settings_manager.h>
#include <symbol_editor_settings.h>
#include <sexpr/sexpr.h>
#include <sexpr/sexpr_parser.h>
#include <kiface_ids.h>
#include <netlist_exporters/netlist_exporter_kicad.h>
#include <wx/ffile.h>
#include <wildcards_and_files_ext.h>

#include <schematic.h>
#include <connection_graph.h>
#include <panel_template_fieldnames.h>
#include <panel_eeschema_color_settings.h>
#include <panel_sym_color_settings.h>
#include <panel_eeschema_editing_options.h>
#include <panel_eeschema_annotation_options.h>
#include <panel_sym_editing_options.h>
#include <dialogs/panel_gal_display_options.h>
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
    schematic->SetRoot( pi->LoadSchematicFile( aFilename, schematic.get() ) );
    schematic->CurrentSheet().push_back( &schematic->Root() );

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
            UNITS_PROVIDER( schIUScale, EDA_UNITS::MM )
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

            for( ACTION_TOOLBAR_CONTROL* control : ACTION_TOOLBAR::GetCustomControlList() )
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

            for( ACTION_TOOLBAR_CONTROL* control : ACTION_TOOLBAR::GetCustomControlList() )
                controls.push_back( control );

            return new PANEL_TOOLBAR_CUSTOMIZATION( aParent, cfg, tb, actions, controls );
        }

        case PANEL_SCH_COLORS:
            return new PANEL_EESCHEMA_COLOR_SETTINGS( aParent );

        case PANEL_SCH_FIELD_NAME_TEMPLATES:
            return new PANEL_TEMPLATE_FIELDNAMES( aParent, nullptr );

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

private:
    bool loadGlobalLibTable();
    bool loadGlobalDesignBlockLibTable();

    std::unique_ptr<EESCHEMA_JOBS_HANDLER> m_jobHandler;

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

    if( !loadGlobalLibTable() || !loadGlobalDesignBlockLibTable() )
    {
        // we didnt get anywhere deregister the settings
        aProgram->GetSettingsManager().FlushAndRelease( symSettings, false );
        aProgram->GetSettingsManager().FlushAndRelease( KifaceSettings(), false );
        return false;
    }

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
    loadGlobalLibTable();
}


bool IFACE::loadGlobalLibTable()
{
    wxFileName fn = SYMBOL_LIB_TABLE::GetGlobalTableFileName();

    if( !fn.FileExists() )
    {
        if( !( m_start_flags & KFCTL_CLI ) )
        {
            // Ensure the splash screen does not hide the dialog:
            Pgm().HideSplash();

            DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG symDialog( nullptr );

            if( symDialog.ShowModal() != wxID_OK )
                return false;
        }
    }
    else
    {
        try
        {
            // The global table is not related to a specific project.  All projects
            // will use the same global table.  So the KIFACE::OnKifaceStart() contract
            // of avoiding anything project specific is not violated here.
            if( !SYMBOL_LIB_TABLE::LoadGlobalTable( SYMBOL_LIB_TABLE::GetGlobalLibTable() ) )
                return false;
        }
        catch( const IO_ERROR& ioe )
        {
            // if we are here, a incorrect global symbol library table was found.
            // Incorrect global symbol library table is not a fatal error:
            // the user just has to edit the (partially) loaded table.
            wxString msg =
                    _( "An error occurred attempting to load the global symbol library table.\n"
                       "Please edit this global symbol library table in Preferences menu." );

            DisplayErrorMessage( nullptr, msg, ioe.What() );
        }
    }

    return true;
}


bool IFACE::loadGlobalDesignBlockLibTable()
{
    try
    {
        wxFileName fn = DESIGN_BLOCK_LIB_TABLE::GetGlobalTableFileName();

        if( !fn.FileExists() )
        {
            DESIGN_BLOCK_LIB_TABLE emptyTable;
            emptyTable.Save( fn.GetFullPath() );
        }

        // The global table is not related to a specific project.  All projects
        // will use the same global table.  So the KIFACE::OnKifaceStart() contract
        // of avoiding anything project specific is not violated here.
        if( !DESIGN_BLOCK_LIB_TABLE::LoadGlobalTable(
                    DESIGN_BLOCK_LIB_TABLE::GetGlobalLibTable() ) )
            return false;
    }
    catch( const IO_ERROR& ioe )
    {
        // if we are here, a incorrect global design block library table was found.
        // Incorrect global design block library table is not a fatal error:
        // the user just has to edit the (partially) loaded table.
        wxString msg =
                _( "An error occurred attempting to load the global design block library table.\n"
                   "Please edit this global design block library table in Preferences menu." );

        DisplayErrorMessage( nullptr, msg, ioe.What() );
    }

    return true;
}


void IFACE::OnKifaceEnd()
{
    end_common();
}


static void traverseSEXPR( SEXPR::SEXPR* aNode,
                           const std::function<void( SEXPR::SEXPR* )>& aVisitor )
{
    aVisitor( aNode );

    if( aNode->IsList() )
    {
        for( unsigned i = 0; i < aNode->GetNumberOfChildren(); i++ )
            traverseSEXPR( aNode->GetChild( i ), aVisitor );
    }
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

        // Sheet paths when auto-generated are relative to the root, so those will stay
        // pointing to whatever they were pointing at.
        // The author can create their own absolute and relative sheet paths.  Absolute
        // sheet paths aren't an issue, and relative ones will continue to work as long
        // as the author didn't include any '..'s.  If they did, it's still not clear
        // whether they should be adjusted or not (as the author may be duplicating an
        // entire tree with several projects within it), so we leave this as an exercise
        // to the author.

        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
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
        bool success = false;

        if( destFile.GetName() == aProjectName )
            destFile.SetName( aNewProjectName  );

        try
        {
            SEXPR::PARSER parser;
            std::unique_ptr<SEXPR::SEXPR> sexpr( parser.ParseFromFile( TO_UTF8( aSrcFilePath ) ) );

            traverseSEXPR( sexpr.get(), [&]( SEXPR::SEXPR* node )
                {
                    if( node->IsList() && node->GetNumberOfChildren() > 1
                            && node->GetChild( 0 )->IsSymbol()
                            && node->GetChild( 0 )->GetSymbol() == "source" )
                    {
                        auto pathNode = dynamic_cast<SEXPR::SEXPR_STRING*>( node->GetChild( 1 ) );
                        auto symNode = dynamic_cast<SEXPR::SEXPR_SYMBOL*>( node->GetChild( 1 ) );
                        wxString path;

                        if( pathNode )
                            path = pathNode->m_value;
                        else if( symNode )
                            path = symNode->m_value;

                        if( path == aProjectName + wxS( ".sch" ) )
                            path = aNewProjectName + wxS( ".sch" );
                        else if( path == aProjectBasePath + "/" + aProjectName + wxS( ".sch" ) )
                            path = aNewProjectBasePath + "/" + aNewProjectName + wxS( ".sch" );
                        else if( path.StartsWith( aProjectBasePath ) )
                            path.Replace( aProjectBasePath, aNewProjectBasePath, false );

                        if( pathNode )
                            pathNode->m_value = path;
                        else if( symNode )
                            symNode->m_value = path;
                    }
                } );

            wxFFile destNetList( destFile.GetFullPath(), "wb" );

            if( destNetList.IsOpened() )
                success = destNetList.Write( sexpr->AsString( 0 ) );

            // wxFFile dtor will close the file
        }
        catch( ... )
        {
            success = false;
        }

        if( !success )
        {
            wxString msg;

            if( !aErrors.empty() )
                aErrors += wxS( "\n" );

            msg.Printf( _( "Cannot copy file '%s'." ), destFile.GetFullPath() );
            aErrors += msg;
        }
    }
    else if( destFile.GetName() == FILEEXT::SymbolLibraryTableFileName )
    {
        SYMBOL_LIB_TABLE symbolLibTable;
        symbolLibTable.Load( aSrcFilePath );

        for( unsigned i = 0; i < symbolLibTable.GetCount(); i++ )
        {
            LIB_TABLE_ROW& row = symbolLibTable.At( i );
            wxString       uri = row.GetFullURI();

            uri.Replace( wxS( "/" ) + aProjectName + wxS( "-cache.lib" ),
                         wxS( "/" ) + aNewProjectName + wxS( "-cache.lib" ) );
            uri.Replace( wxS( "/" ) + aProjectName + wxS( "-rescue.lib" ),
                         wxS( "/" ) + aNewProjectName +  wxS( "-rescue.lib" ) );
            uri.Replace( wxS( "/" ) + aProjectName + wxS( ".lib" ),
                         wxS( "/" ) + aNewProjectName + wxS( ".lib" ) );

            row.SetFullURI( uri );
        }

        try
        {
            symbolLibTable.Save( destFile.GetFullPath() );
        }
        catch( ... )
        {
            wxString msg;

            if( !aErrors.empty() )
                aErrors += "\n";

            msg.Printf( _( "Cannot copy file '%s'." ), destFile.GetFullPath() );
            aErrors += msg;
        }
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
