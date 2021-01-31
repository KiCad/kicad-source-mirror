/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <confirm.h>
#include <core/arraydim.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <pcb_edit_frame.h>
#include <3d_viewer/eda_3d_viewer.h>
#include <pgm_base.h>
#include <widgets/msgpanel.h>
#include <fp_lib_table.h>
#include <kiface_i.h>
#include <trace_helpers.h>
#include <lockfile.cpp>
#include <netlist_reader/pcb_netlist.h>
#include <pcbnew_id.h>
#include <io_mgr.h>
#include <wildcards_and_files_ext.h>
#include <tool/tool_manager.h>
#include <board.h>
#include <wx/stdpaths.h>
#include <ratsnest/ratsnest_data.h>
#include <kiplatform/app.h>
#include <widgets/appearance_controls.h>
#include <widgets/infobar.h>
#include <wx/wupdlock.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <paths.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <project/net_settings.h>
#include <plugins/cadstar/cadstar_pcb_archive_plugin.h>
#include <plugins/eagle/eagle_plugin.h>
#include <plugins/kicad/kicad_plugin.h>
#include <dialogs/dialog_imported_layers.h>
#include "footprint_info_impl.h"


//#define     USE_INSTRUMENTATION     1
#define     USE_INSTRUMENTATION     0


/**
 * Show a wxFileDialog asking for a #BOARD filename to open.
 *
 * @param aParent is a wxFrame passed to wxFileDialog.
 * @param aCtl is where to put the OpenProjectFiles() control bits.
 * @param aFileName on entry is a probable choice, on return is the chosen filename.
 * @param aKicadFilesOnly true to list KiCad pcb files plugins only, false to list import plugins.
 * @return  true if chosen, else false if user aborted.
 */
bool AskLoadBoardFileName( wxWindow* aParent, int* aCtl, wxString* aFileName, bool aKicadFilesOnly )
{
    // This is a subset of all PLUGINs which are trusted to be able to
    // load a BOARD. User may occasionally use the wrong plugin to load a
    // *.brd file (since both legacy and eagle use *.brd extension),
    // but eventually *.kicad_pcb will be more common than legacy *.brd files.

    // clang-format off
    static const struct
    {
        const wxString&     filter;
        IO_MGR::PCB_FILE_T  pluginType;
    } loaders[] =
    {
        // Current Kicad board files.
        { PcbFileWildcard(),                    IO_MGR::KICAD_SEXP },

        // Old Kicad board files.
        { LegacyPcbFileWildcard(),              IO_MGR::LEGACY },

        // Import Altium Circuit Maker board files.
        { AltiumCircuitMakerPcbFileWildcard(),  IO_MGR::ALTIUM_CIRCUIT_MAKER },

        // Import Altium Circuit Studio board files.
        { AltiumCircuitStudioPcbFileWildcard(), IO_MGR::ALTIUM_CIRCUIT_STUDIO },

        // Import Altium Designer board files.
        { AltiumDesignerPcbFileWildcard(),      IO_MGR::ALTIUM_DESIGNER },

        // Import Cadstar PCB Archive board files.
        { CadstarPcbArchiveFileWildcard(),      IO_MGR::CADSTAR_PCB_ARCHIVE },

        // Import Eagle board files.
        { EaglePcbFileWildcard(),               IO_MGR::EAGLE },

        // Import PCAD board files.
        { PCadPcbFileWildcard(),                IO_MGR::PCAD },

        // Import Fabmaster board files.
        { FabmasterPcbFileWildcard(),           IO_MGR::FABMASTER },
    };
    // clang-format on

    wxFileName  fileName( *aFileName );
    wxString    fileFilters;

    if( aKicadFilesOnly )
    {
        std::vector<std::string> fileExtensions;

        for( unsigned ii = 0; ii < 2; ++ii )
        {
            if( !fileFilters.IsEmpty() )
                fileFilters += wxChar( '|' );

            fileFilters += wxGetTranslation( loaders[ii].filter );

            PLUGIN::RELEASER plugin( IO_MGR::PluginFind( loaders[ii].pluginType ) );
            wxCHECK( plugin, false );
            fileExtensions.push_back( plugin->GetFileExtension().ToStdString() );
        }

        fileFilters = _( "All KiCad Board Files" ) + AddFileExtListToFilter( fileExtensions ) + "|"
                      + fileFilters;
    }
    else
    {
        wxString allWildcards;

        for( unsigned ii = 2; ii < arrayDim( loaders ); ++ii )
        {
            if( !fileFilters.IsEmpty() )
                fileFilters += wxChar( '|' );

            fileFilters += wxGetTranslation( loaders[ii].filter );

            PLUGIN::RELEASER plugin( IO_MGR::PluginFind( loaders[ii].pluginType ) );
            wxCHECK( plugin, false );
            allWildcards += "*." + formatWildcardExt( plugin->GetFileExtension() ) + ";";
        }

        fileFilters = _( "All supported formats|" ) + allWildcards + "|" + fileFilters;
    }


    wxString    path;
    wxString    name;

    if( fileName.FileExists() )
    {
        path = fileName.GetPath();
        name = fileName.GetFullName();
    }
    else
    {
        path = PATHS::GetDefaultUserProjectsPath();
        // leave name empty
    }

    wxFileDialog dlg( aParent,
                      aKicadFilesOnly ? _( "Open Board File" ) : _( "Import Non KiCad Board File" ),
                      path, name, fileFilters,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_OK )
    {
        // For import option, if Eagle (*.brd files), tell OpenProjectFiles() to use Eagle plugin.
        // It's the only special case because of the duplicate use of the *.brd file extension.
        // Other cases are clear because of unique file extensions.
        *aCtl = aKicadFilesOnly ? 0 : KICTL_EAGLE_BRD;
        *aFileName = dlg.GetPath();
        return true;
    }
    else
        return false;
}


///< Helper widget to select whether a new project should be created for a file when saving
class CREATE_PROJECT_CHECKBOX : public wxPanel
{
public:
    CREATE_PROJECT_CHECKBOX( wxWindow* aParent )
            : wxPanel( aParent )
    {
        m_cbCreateProject = new wxCheckBox( this, wxID_ANY,
                                            _( "Create a new project for this board" ) );
        m_cbCreateProject->SetValue( false );
        m_cbCreateProject->SetToolTip( _( "Creating a project will enable features such as "
                                          "design rules, net classes, and layer presets" ) );

        wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
        sizer->Add( m_cbCreateProject, 0, wxALL, 8 );

        SetSizerAndFit( sizer );
    }

    bool GetValue() const
    {
        return m_cbCreateProject->GetValue();
    }

    static wxWindow* Create( wxWindow* aParent )
    {
        return new CREATE_PROJECT_CHECKBOX( aParent );
    }

protected:
    wxCheckBox* m_cbCreateProject;
};


/**
 * Put up a wxFileDialog asking for a BOARD filename to save.
 *
 * @param aParent is a wxFrame passed to wxFileDialog.
 * @param aFileName on entry is a probable choice, on return is the chosen full filename
 *                  (includes path).
 * @param aCreateProject will be filled with the state of the Create Project? checkbox if relevant.
 * @return true if chosen, else false if user aborted.
 */
bool AskSaveBoardFileName( PCB_EDIT_FRAME* aParent, wxString* aFileName, bool* aCreateProject )
{
    wxString    wildcard =  PcbFileWildcard();
    wxFileName  fn = *aFileName;

    fn.SetExt( KiCadPcbFileExtension );

    wxFileDialog dlg( aParent, _( "Save Board File As" ), fn.GetPath(), fn.GetFullName(), wildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    // Add a "Create a project" checkbox in standalone mode and one isn't loaded
    if( Kiface().IsSingle() && aParent->Prj().IsNullProject() )
        dlg.SetExtraControlCreator( &CREATE_PROJECT_CHECKBOX::Create );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    fn = dlg.GetPath();

    // always enforce filename extension, user may not have entered it.
    fn.SetExt( KiCadPcbFileExtension );

    *aFileName = fn.GetFullPath();

    if( wxWindow* extraControl = dlg.GetExtraControl() )
        *aCreateProject = static_cast<CREATE_PROJECT_CHECKBOX*>( extraControl )->GetValue();
    else if( Kiface().IsSingle() && !aParent->Prj().IsNullProject() )
        *aCreateProject = true;

    return true;
}


void PCB_EDIT_FRAME::OnFileHistory( wxCommandEvent& event )
{
    wxString fn = GetFileFromHistory( event.GetId(), _( "Printed circuit board" ) );

    if( !!fn )
    {
        int open_ctl = 0;

        if( !wxFileName::IsFileReadable( fn ) )
        {
            if( !AskLoadBoardFileName( this, &open_ctl, &fn, true ) )
                return;
        }

        OpenProjectFiles( std::vector<wxString>( 1, fn ), open_ctl );
    }
}


void PCB_EDIT_FRAME::OnClearFileHistory( wxCommandEvent& aEvent )
{
    ClearFileHistory();
}


void PCB_EDIT_FRAME::Files_io( wxCommandEvent& event )
{
    int        id = event.GetId();
    Files_io_from_id( id );
}


bool PCB_EDIT_FRAME::Files_io_from_id( int id )
{
    wxString   msg;

    switch( id )
    {
    case ID_LOAD_FILE:
        {
            int         open_ctl = 0;
            wxString    fileName = Prj().AbsolutePath( GetBoard()->GetFileName() );

            return AskLoadBoardFileName( this, &open_ctl, &fileName, true )
                       && OpenProjectFiles( std::vector<wxString>( 1, fileName ), open_ctl );
        }

    case ID_IMPORT_NON_KICAD_BOARD:
        {
            int         open_ctl = 1;
            wxString    fileName; // = Prj().AbsolutePath( GetBoard()->GetFileName() );

            return AskLoadBoardFileName( this, &open_ctl, &fileName, false )
                       && OpenProjectFiles( std::vector<wxString>( 1, fileName ), open_ctl );
        }

    case ID_MENU_RECOVER_BOARD_AUTOSAVE:
        {
            wxFileName currfn = Prj().AbsolutePath( GetBoard()->GetFileName() );
            wxFileName fn = currfn;

            wxString rec_name = GetAutoSaveFilePrefix() + fn.GetName();
            fn.SetName( rec_name );

            if( !fn.FileExists() )
            {
                msg.Printf( _( "Recovery file \"%s\" not found." ), fn.GetFullPath() );
                DisplayInfoMessage( this, msg );
                return false;
            }

            msg.Printf( _( "OK to load recovery file \"%s\"" ), fn.GetFullPath() );

            if( !IsOK( this, msg ) )
                return false;

            GetScreen()->ClrModify();    // do not prompt the user for changes

            if( OpenProjectFiles( std::vector<wxString>( 1, fn.GetFullPath() ) ) )
            {
                // Re-set the name since name or extension was changed
                GetBoard()->SetFileName( currfn.GetFullPath() );
                UpdateTitle();
                return true;
            }
            return false;
        }

    case ID_NEW_BOARD:
    {
        if( IsContentModified() )
        {
            wxFileName fileName = GetBoard()->GetFileName();
            wxString   saveMsg =
                    _( "Current board will be closed, save changes to \"%s\" before continuing?" );

            if( !HandleUnsavedChanges( this, wxString::Format( saveMsg, fileName.GetFullName() ),
                                       [&]()->bool { return Files_io_from_id( ID_SAVE_BOARD ); } ) )
                return false;
        }
        else if( !GetBoard()->IsEmpty() )
        {
            if( !IsOK( this, _( "Current Board will be closed. Continue?" ) ) )
                return false;
        }

        SaveProjectSettings();

        GetBoard()->ClearProject();

        SETTINGS_MANAGER* mgr = GetSettingsManager();

        mgr->SaveProject( mgr->Prj().GetProjectFullName() );
        mgr->UnloadProject( &mgr->Prj() );

        if( !Clear_Pcb( false ) )
            return false;

        onBoardLoaded();

        LoadProjectSettings();

        OnModify();
        return true;
    }

    case ID_SAVE_BOARD:
        if( !GetBoard()->GetFileName().IsEmpty() )
            return SavePcbFile( Prj().AbsolutePath( GetBoard()->GetFileName() ) );

        KI_FALLTHROUGH;

    case ID_COPY_BOARD_AS:
    case ID_SAVE_BOARD_AS:
        {
            bool addToHistory = false;
            wxString orig_name;
            wxFileName::SplitPath( GetBoard()->GetFileName(), nullptr, nullptr, &orig_name,
                                   nullptr );

            if( orig_name.IsEmpty() )
            {
                addToHistory = true;
                orig_name = _( "noname" );
            }

            wxFileName savePath( Prj().GetProjectFullName() );

            if( !savePath.IsOk() || !savePath.IsDirWritable() )
            {
                savePath = GetMruPath();

                if( !savePath.IsOk() || !savePath.IsDirWritable() )
                    savePath = PATHS::GetDefaultUserProjectsPath();
            }

            wxFileName  fn( savePath.GetPath(), orig_name, KiCadPcbFileExtension );
            wxString    filename = fn.GetFullPath();

            bool createProject = false;

            if( AskSaveBoardFileName( this, &filename, &createProject ) )
            {
                if( id == ID_COPY_BOARD_AS )
                    return SavePcbCopy( filename, createProject );
                else
                    return SavePcbFile( filename, addToHistory, createProject );
            }
            return false;
        }

    default:
        return false;
    }
}


// The KIWAY_PLAYER::OpenProjectFiles() API knows nothing about plugins, so
// determine how to load the BOARD here, with minor assistance from KICTL_EAGLE_BRD
// bit flag.
IO_MGR::PCB_FILE_T plugin_type( const wxString& aFileName, int aCtl )
{
    IO_MGR::PCB_FILE_T  pluginType;

    wxFileName fn = aFileName;

    // Note: file extensions are expected to be in lower case.
    // This is not always true, especially when importing files, so the string
    // comparisons are case insensitive to try to find the suitable plugin.

    if( fn.GetExt().CmpNoCase( IO_MGR::GetFileExtension( IO_MGR::LEGACY ) ) == 0 )
    {
        // both legacy and eagle share a common file extension.
        pluginType = ( aCtl & KICTL_EAGLE_BRD ) ? IO_MGR::EAGLE : IO_MGR::LEGACY;
    }
    else if( fn.GetExt().CmpNoCase( IO_MGR::GetFileExtension( IO_MGR::PCAD ) ) == 0 )
    {
        pluginType = IO_MGR::PCAD;
    }
    else if( fn.GetExt().CmpNoCase( IO_MGR::GetFileExtension( IO_MGR::ALTIUM_DESIGNER ) ) == 0 )
    {
        pluginType = IO_MGR::ALTIUM_DESIGNER;
    }
    else if( fn.GetExt().CmpNoCase( IO_MGR::GetFileExtension( IO_MGR::ALTIUM_CIRCUIT_STUDIO ) ) == 0 )
    {
        pluginType = IO_MGR::ALTIUM_CIRCUIT_STUDIO;
    }
    else if( fn.GetExt().CmpNoCase( IO_MGR::GetFileExtension( IO_MGR::ALTIUM_CIRCUIT_MAKER ) ) == 0 )
    {
        pluginType = IO_MGR::ALTIUM_CIRCUIT_MAKER;
    }
    else if( fn.GetExt().CmpNoCase( IO_MGR::GetFileExtension( IO_MGR::CADSTAR_PCB_ARCHIVE ) ) == 0 )
    {
        pluginType = IO_MGR::CADSTAR_PCB_ARCHIVE;
    }
    else if( fn.GetExt().CmpNoCase( IO_MGR::GetFileExtension( IO_MGR::FABMASTER ) ) == 0 )
    {
        pluginType = IO_MGR::FABMASTER;
    }
    else
    {
        pluginType = IO_MGR::KICAD_SEXP;
    }

    return pluginType;
}


int PCB_EDIT_FRAME::inferLegacyEdgeClearance( BOARD* aBoard )
{
    PCB_LAYER_COLLECTOR collector;

    collector.SetLayerId( Edge_Cuts );
    collector.Collect( aBoard, GENERAL_COLLECTOR::AllBoardItems );

    int  edgeWidth = -1;
    bool mixed = false;

    for( int i = 0; i < collector.GetCount(); i++ )
    {
        if( collector[i]->Type() == PCB_SHAPE_T )
        {
            int itemWidth = static_cast<PCB_SHAPE*>( collector[i] )->GetWidth();

            if( edgeWidth != -1 && edgeWidth != itemWidth )
            {
                mixed = true;
                edgeWidth = std::max( edgeWidth, itemWidth );
            }
            else
            {
                edgeWidth = itemWidth;
            }
        }
    }

    if( mixed )
    {
        // If they had different widths then we can't ensure that fills will be the same.
        wxMessageBox( _( "If the zones on this board are refilled the Copper Edge Clearance "
                         "setting will be used (see Board Setup > Design Rules > Constraints).\n"
                         "This may result in different fills from previous Kicad versions which "
                         "used the line thicknesses of the board boundary on the Edge Cuts "
                          "layer." ),
                      _( "Edge Clearance Warning" ), wxOK | wxICON_WARNING, this );
    }

    return std::max( 0, edgeWidth / 2 );
}


bool PCB_EDIT_FRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    // This is for python:
    if( aFileSet.size() != 1 )
    {
        UTF8 msg = StrPrintf( "Pcbnew:%s() takes only a single filename", __func__ );
        DisplayError( this, msg );
        return false;
    }

    wxString fullFileName( aFileSet[0] );

    if( Kiface().IsSingle() )
    {
        KIPLATFORM::APP::RegisterApplicationRestart( fullFileName );
    }

    // We insist on caller sending us an absolute path, if it does not, we say it's a bug.
    wxASSERT_MSG( wxFileName( fullFileName ).IsAbsolute(), wxT( "Path is not absolute!" ) );

    std::unique_ptr<wxSingleInstanceChecker> lockFile = ::LockFile( fullFileName );

    if( !lockFile )
    {
        wxString msg = wxString::Format( _( "PCB file \"%s\" is already open." ), fullFileName );
        DisplayError( this, msg );
        return false;
    }

    if( IsContentModified() )
    {
        if( !HandleUnsavedChanges( this, _( "The current PCB has been modified.  Save changes?" ),
            [&]()->bool { return SavePcbFile( GetBoard()->GetFileName() ); } ) )
        {
            return false;
        }
    }

    // Release the lock file, until the new file is actually loaded
    ReleaseFile();

    wxFileName pro = fullFileName;
    pro.SetExt( ProjectFileExtension );

    bool is_new = !wxFileName::IsFileReadable( fullFileName );

    // If its a non-existent schematic and caller thinks it exists
    if( is_new && !( aCtl & KICTL_CREATE ) )
    {
        // notify user that fullFileName does not exist, ask if user wants to create it.
        wxString ask = wxString::Format( _( "PCB \"%s\" does not exist.  Do you wish to "
                                             "create it?" ),
                                         fullFileName );
        if( !IsOK( this, ask ) )
            return false;
    }

    // Get rid of any existing warnings about the old board
    GetInfoBar()->Dismiss();

    // Loading a complex project and build data can be time
    // consuming, so display a busy cursor
    wxBusyCursor dummy;

    // No save prompt (we already prompted above), and only reset to a new blank board if new
    Clear_Pcb( false, !is_new );

    IO_MGR::PCB_FILE_T  pluginType = plugin_type( fullFileName, aCtl );

    bool converted =  pluginType != IO_MGR::LEGACY && pluginType != IO_MGR::KICAD_SEXP;

    // Loading a project should only be done under carefully considered circumstances.

    // The calling code should know not to ask me here to change projects unless
    // it knows what consequences that will have on other KIFACEs running and using
    // this same PROJECT.  It can be very harmful if that calling code is stupid.
    SETTINGS_MANAGER* mgr = GetSettingsManager();

    if( pro.GetFullPath() != mgr->Prj().GetProjectFullName() )
    {
        // calls SaveProject
        SaveProjectSettings();

        GetBoard()->ClearProject();
        mgr->UnloadProject( &mgr->Prj() );

        mgr->LoadProject( pro.GetFullPath() );

        // Do not allow saving a project if one doesn't exist.  This normally happens if we are
        // standalone and opening a board that has been moved from its project folder.
        // For converted projects, we don't want to set the read-only flag because we want a project
        // to be saved for the new file in case things like netclasses got migrated.
        if( !pro.Exists() && !converted )
            Prj().SetReadOnly();
    }

    // Clear the cache footprint list which may be project specific
    GFootprintList.Clear();

    if( is_new )
    {
        // Link the existing blank board to the new project
        GetBoard()->SetProject( &Prj() );

        GetBoard()->SetFileName( fullFileName );

        OnModify();
    }
    else
    {
        BOARD* loadedBoard = 0;   // it will be set to non-NULL if loaded OK

        PLUGIN::RELEASER pi( IO_MGR::PluginFind( pluginType ) );

        LAYER_REMAPPABLE_PLUGIN* layerRemappable =
            dynamic_cast< LAYER_REMAPPABLE_PLUGIN* >( (PLUGIN*) pi );
        if ( layerRemappable )
        {
            using namespace std::placeholders;
            layerRemappable->RegisterLayerMappingCallback(
                    std::bind( DIALOG_IMPORTED_LAYERS::GetMapModal, this, _1 ) );
        }

        // This will rename the file if there is an autosave and the user want to recover
		CheckForAutoSaveFile( fullFileName );

        try
        {
            PROPERTIES  props;
            char        xbuf[30];
            char        ybuf[30];

            // EAGLE_PLUGIN can use this info to center the BOARD, but it does not yet.
            sprintf( xbuf, "%d", GetPageSizeIU().x );
            sprintf( ybuf, "%d", GetPageSizeIU().y );

            props["page_width"]  = xbuf;
            props["page_height"] = ybuf;

#if USE_INSTRUMENTATION
            // measure the time to load a BOARD.
            unsigned startTime = GetRunningMicroSecs();
#endif

            loadedBoard = pi->Load( fullFileName, NULL, &props, &Prj() );

#if USE_INSTRUMENTATION
            unsigned stopTime = GetRunningMicroSecs();
            printf( "PLUGIN::Load(): %u usecs\n", stopTime - startTime );
#endif
        }
        catch( const IO_ERROR& ioe )
        {
            if( ioe.Problem() != wxT( "CANCEL" ) )
            {
                wxString msg = wxString::Format( _( "Error loading board file:\n%s" ),
                                                 fullFileName );
                DisplayErrorMessage( this, msg, ioe.What() );
            }

            // We didn't create a new blank board above, so do that now
            Clear_Pcb( false );

            return false;
        }

        SetBoard( loadedBoard );

        if( GFootprintList.GetCount() == 0 )
        {
            GFootprintList.ReadCacheFromFile( Prj().GetProjectPath() + "fp-info-cache" );
        }

        if( loadedBoard->m_LegacyDesignSettingsLoaded )
        {
            Prj().GetProjectFile().NetSettings().ResolveNetClassAssignments( true );

            // Before we had a copper edge clearance setting, the edge line widths could be used
            // as a kludge to control them.  So if there's no setting then infer it from the
            // edge widths.
            if( !loadedBoard->m_LegacyCopperEdgeClearanceLoaded )
            {
                int edgeClearance = inferLegacyEdgeClearance( loadedBoard );
                loadedBoard->GetDesignSettings().SetCopperEdgeClearance( edgeClearance );
            }

            // On save; design settings will be removed from the board
            loadedBoard->SetModified();
        }

        // Move legacy view settings to local project settings
        if( !loadedBoard->m_LegacyVisibleLayers.test( Rescue ) )
        {
            Prj().GetLocalSettings().m_VisibleLayers = loadedBoard->m_LegacyVisibleLayers;
            loadedBoard->SetModified();
        }

        if( !loadedBoard->m_LegacyVisibleItems.test( GAL_LAYER_INDEX( GAL_LAYER_ID_BITMASK_END ) ) )
        {
            Prj().GetLocalSettings().m_VisibleItems = loadedBoard->m_LegacyVisibleItems;
            loadedBoard->SetModified();
        }

        // we should not ask PLUGINs to do these items:
        loadedBoard->BuildListOfNets();
        ResolveDRCExclusions();

        if( loadedBoard->IsModified() )
            OnModify();
        else
            GetScreen()->ClrModify();

        if( ( pluginType == IO_MGR::LEGACY &&
              loadedBoard->GetFileFormatVersionAtLoad() < LEGACY_BOARD_FILE_VERSION ) ||
            ( pluginType == IO_MGR::KICAD_SEXP &&
              loadedBoard->GetFileFormatVersionAtLoad() < SEXPR_BOARD_FILE_VERSION ) )
        {
            m_infoBar->RemoveAllButtons();
            m_infoBar->AddCloseButton();
            m_infoBar->ShowMessage( _( "This file was created by an older version of KiCad. "
                                       "It will be converted to the new format when saved." ),
                                    wxICON_WARNING );
        }

        // Import footprints into a project-specific library
        //==================================================
        // TODO: This should be refactored out of here into somewhere specific to the Project Import
        // E.g. KICAD_MANAGER_FRAME::ImportNonKiCadProject
        if( aCtl & KICTL_IMPORT_LIB )
        {
            wxFileName loadedBoardFn( fullFileName );
            wxString   libNickName = loadedBoardFn.GetName();

            // Extract a footprint library from the design and add it to the fp-lib-table
            // The footprints are saved in a new .pretty library.
            // If this library already exists, all previous footprints will be deleted
            std::vector<FOOTPRINT*> loadedFootprints = pi->GetImportedCachedLibraryFootprints();
            wxString                newLibPath = CreateNewLibrary( libNickName );

            // Only create the new library if CreateNewLibrary succeeded (note that this fails if
            // the library already exists and the user aborts after seeing the warning message
            // which prompts the user to continue with overwrite or abort)
            if( newLibPath.Length() > 0 )
            {
                PLUGIN::RELEASER piSexpr( IO_MGR::PluginFind( IO_MGR::KICAD_SEXP ) );

                for( FOOTPRINT* footprint : loadedFootprints )
                {
                    try
                    {
                        if( !footprint->GetFPID().GetLibItemName().empty() ) // Handle old boards.
                        {
                            footprint->SetReference( "REF**" );
                            piSexpr->FootprintSave( newLibPath, footprint );
                            delete footprint;
                        }
                    }
                    catch( const IO_ERROR& ioe )
                    {
                        wxLogError( wxString::Format( _( "Error occurred when saving footprint "
                                                         "'%s' to the project specific footprint "
                                                         "library: %s" ),
                                                         footprint->GetFPID().GetUniStringLibItemName(),
                                                         ioe.What() ) );
                    }
                }

                FP_LIB_TABLE*   prjlibtable = Prj().PcbFootprintLibs();
                const wxString& project_env = PROJECT_VAR_NAME;
                wxString        rel_path, env_path;

                wxASSERT_MSG( wxGetEnv( project_env, &env_path ), "There is no project variable?" );

                wxString result( newLibPath );
                rel_path = result.Replace( env_path, wxString( "$(" + project_env + ")" ) ) ? result
                                                                                            : "";

                FP_LIB_TABLE_ROW* row = new FP_LIB_TABLE_ROW( libNickName, rel_path,
                                                              wxT( "KiCad" ), wxEmptyString );
                prjlibtable->InsertRow( row );

                wxString tblName = Prj().FootprintLibTblName();

                try
                {
                    Prj().PcbFootprintLibs()->Save( tblName );
                }
                catch( const IO_ERROR& ioe )
                {
                    wxLogError( wxString::Format( _( "Error occurred saving the project specific "
                                                     "footprint library table: %s" ),
                                                     ioe.What() ) );
                }

                // Update footprint LIB_IDs to point to the just imported library
                for( FOOTPRINT* footprint : GetBoard()->Footprints() )
                {
                    LIB_ID libId = footprint->GetFPID();

                    if( libId.GetLibItemName().empty() )
                        continue;

                    libId.SetLibNickname( libNickName );
                    footprint->SetFPID( libId );
                }
            }
        }
    }

    {
        wxFileName fn = fullFileName;

        if( converted )
            fn.SetExt( PcbFileExtension );

        wxString fname = fn.GetFullPath();

        fname.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );

        GetBoard()->SetFileName( fname );
    }

    // Lock the file newly opened:
    m_file_checker.reset( lockFile.release() );

    if( !converted )
        UpdateFileHistory( GetBoard()->GetFileName() );

    // Rebuild list of nets (full ratsnest rebuild)
    GetBoard()->BuildConnectivity();
    Compile_Ratsnest( true );

    // Load project settings after setting up board; some of them depend on the nets list
    LoadProjectSettings();

    // Syncs the UI (appearance panel, etc) with the loaded board and project
    onBoardLoaded();

    // Refresh the 3D view, if any
    EDA_3D_VIEWER* draw3DFrame = Get3DViewerFrame();

    if( draw3DFrame )
        draw3DFrame->NewDisplay();

#if 0 && defined(DEBUG)
    // Output the board object tree to stdout, but please run from command prompt:
    GetBoard()->Show( 0, std::cout );
#endif

    // from EDA_APPL which was first loaded BOARD only:
    {
        /* For an obscure reason the focus is lost after loading a board file
         * when starting up the process.
         * (seems due to the recreation of the layer manager after loading the file)
         * Give focus to main window and Drawpanel
         * must be done for these 2 windows (for an obscure reason ...)
         * Linux specific
         * This is more a workaround than a fix.
         */
        SetFocus();
        GetCanvas()->SetFocus();
    }

    return true;
}


bool PCB_EDIT_FRAME::SavePcbFile( const wxString& aFileName, bool addToHistory,
                                  bool aChangeProject )
{
    // please, keep it simple.  prompting goes elsewhere.
    wxFileName pcbFileName = aFileName;

    if( pcbFileName.GetExt() == LegacyPcbFileExtension )
        pcbFileName.SetExt( KiCadPcbFileExtension );

    if( !IsWritable( pcbFileName ) )
    {
        wxString msg = wxString::Format( _(
            "No access rights to write to file \"%s\"" ),
            pcbFileName.GetFullPath() );

        DisplayError( this, msg );
        return false;
    }

    // TODO: this will break if we ever go multi-board
    wxFileName projectFile( pcbFileName );
    bool       projectFileExists = false;

    projectFile.SetExt( ProjectFileExtension );
    projectFileExists = projectFile.FileExists();

    if( aChangeProject && !projectFileExists )
    {
        // If this is a new board, project filename won't be set yet
        if( projectFile.GetFullPath() != Prj().GetProjectFullName() )
        {
            GetBoard()->ClearProject();

            SETTINGS_MANAGER* mgr = GetSettingsManager();

            mgr->SaveProject( Prj().GetProjectFullName() );
            mgr->UnloadProject( &Prj() );

            // If no project to load then initialize project text vars with board properties
            if( !mgr->LoadProject( projectFile.GetFullPath() ) )
                Prj().GetTextVars() = GetBoard()->GetProperties();

            GetBoard()->SetProject( &Prj() );
        }
    }

    if( projectFileExists )
        GetBoard()->SynchronizeProperties();

    wxFileName tempFile( aFileName );
    tempFile.SetName( wxT( "." ) + tempFile.GetName() );
    tempFile.SetExt( tempFile.GetExt() + wxT( "$" ) );

    GetBoard()->SynchronizeNetsAndNetClasses();

    // Save various DRC parameters, such as violation severities (which may have been
    // edited via the DRC dialog as well as the Board Setup dialog), DRC exclusions, etc.
    SaveProjectSettings();

    GetSettingsManager()->SaveProject();


    wxString    upperTxt;
    wxString    lowerTxt;

    try
    {
        PLUGIN::RELEASER    pi( IO_MGR::PluginFind( IO_MGR::KICAD_SEXP ) );

        wxASSERT( tempFile.IsAbsolute() );

        pi->Save( tempFile.GetFullPath(), GetBoard(), NULL );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _(
                "Error saving board file \"%s\".\n%s" ),
                pcbFileName.GetFullPath(), ioe.What()
                );
        DisplayError( this, msg );

        lowerTxt.Printf( _( "Failed to create temporary file \"%s\"" ), tempFile.GetFullPath() );

        SetMsgPanel( upperTxt, lowerTxt );

        // In case we started a file but didn't fully write it, clean up
        wxRemoveFile( tempFile.GetFullPath() );

        return false;
    }

    // If save succeeded, replace the original with what we just wrote
    if( !wxRenameFile( tempFile.GetFullPath(), pcbFileName.GetFullPath() ) )
    {
        wxString msg = wxString::Format( _(
                "Error saving board file \"%s\".\nFailed to rename temporary file \"%s\"" ),
                pcbFileName.GetFullPath(), tempFile.GetFullPath()
                );
        DisplayError( this, msg );

        lowerTxt.Printf( _( "Failed to rename temporary file \"%s\"" ), tempFile.GetFullPath() );

        SetMsgPanel( upperTxt, lowerTxt );

        return false;
    }

    if( !Kiface().IsSingle() )
    {
        WX_STRING_REPORTER backupReporter( &upperTxt );

        if( GetSettingsManager()->TriggerBackupIfNeeded( backupReporter ) )
            upperTxt.clear();
    }

    GetBoard()->SetFileName( pcbFileName.GetFullPath() );
    UpdateTitle();

    // Put the saved file in File History if requested
    if( addToHistory )
        UpdateFileHistory( GetBoard()->GetFileName() );

    // Delete auto save file on successful save.
    wxFileName autoSaveFileName = pcbFileName;

    autoSaveFileName.SetName( GetAutoSaveFilePrefix() + pcbFileName.GetName() );

    if( autoSaveFileName.FileExists() )
        wxRemoveFile( autoSaveFileName.GetFullPath() );

    lowerTxt.Printf( _( "File \"%s\" saved." ), pcbFileName.GetFullPath() );

    SetStatusText( lowerTxt, 0 );

    // Get rid of the old version conversion warning, or any other dismissable warning :)
    if( m_infoBar->IsShown() && m_infoBar->HasCloseButton() )
        m_infoBar->Dismiss();

    GetScreen()->ClrModify();
    GetScreen()->ClrSave();
    return true;
}


bool PCB_EDIT_FRAME::SavePcbCopy( const wxString& aFileName, bool aCreateProject )
{
    wxFileName  pcbFileName = aFileName;

    // Ensure the file ext is the right ext:
    pcbFileName.SetExt( KiCadPcbFileExtension );

    if( !IsWritable( pcbFileName ) )
    {
        wxString msg = wxString::Format( _(
            "No access rights to write to file \"%s\"" ),
            pcbFileName.GetFullPath() );

        DisplayError( this, msg );
        return false;
    }

    GetBoard()->SynchronizeNetsAndNetClasses();

    try
    {
        PLUGIN::RELEASER    pi( IO_MGR::PluginFind( IO_MGR::KICAD_SEXP ) );

        wxASSERT( pcbFileName.IsAbsolute() );

        pi->Save( pcbFileName.GetFullPath(), GetBoard(), NULL );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _( "Error saving board file \"%s\".\n%s" ),
                                         pcbFileName.GetFullPath(), ioe.What() );
        DisplayError( this, msg );

        return false;
    }

    if( aCreateProject )
    {
        wxFileName projectFile( pcbFileName );
        projectFile.SetExt( ProjectFileExtension );

        if( !projectFile.FileExists() )
        {
            wxString currentProject = Prj().GetProjectFullName();

            SETTINGS_MANAGER* mgr = GetSettingsManager();

            GetBoard()->ClearProject();

            mgr->SaveProject( currentProject );
            mgr->UnloadProject( &Prj() );

            mgr->LoadProject( projectFile.GetFullPath() );
            mgr->SaveProject();

            mgr->UnloadProject( &Prj() );
            mgr->LoadProject( currentProject );

            // If no project to load then initialize project text vars with board properties
            if( !mgr->LoadProject( currentProject ) )
                Prj().GetTextVars() = GetBoard()->GetProperties();

            GetBoard()->SetProject( &Prj() );
        }
    }

    DisplayInfoMessage( this, wxString::Format( _( "Board copied to:\n\"%s\"" ),
                                                pcbFileName.GetFullPath() ) );

    return true;
}


bool PCB_EDIT_FRAME::doAutoSave()
{
    wxFileName tmpFileName;

    if( GetBoard()->GetFileName().IsEmpty() )
    {
        tmpFileName = wxFileName( PATHS::GetDefaultUserProjectsPath(), wxT( "noname" ),
                                  KiCadPcbFileExtension );
        GetBoard()->SetFileName( tmpFileName.GetFullPath() );
    }
    else
    {
        tmpFileName = Prj().AbsolutePath( GetBoard()->GetFileName() );
    }

    wxFileName autoSaveFileName = tmpFileName;

    // Auto save file name is the board file name prepended with autosaveFilePrefix string.
    autoSaveFileName.SetName( GetAutoSaveFilePrefix() + autoSaveFileName.GetName() );

    if( !autoSaveFileName.IsOk() )
        return false;

    // If the board file path is not writable, try writing to a platform specific temp file
    // path.  If that path isn't writable, give up.
    if( !autoSaveFileName.IsDirWritable() )
    {
        autoSaveFileName.SetPath( wxFileName::GetTempDir() );

        if( !autoSaveFileName.IsOk() || !autoSaveFileName.IsDirWritable() )
            return false;
    }

    wxLogTrace( traceAutoSave, "Creating auto save file <" + autoSaveFileName.GetFullPath() + ">" );

    if( SavePcbFile( autoSaveFileName.GetFullPath(), false, false ) )
    {
        GetScreen()->SetModify();
        GetBoard()->SetFileName( tmpFileName.GetFullPath() );
        UpdateTitle();
        m_autoSaveState = false;

        if( !Kiface().IsSingle() &&
            GetSettingsManager()->GetCommonSettings()->m_Backup.backup_on_autosave )
        {
            GetSettingsManager()->TriggerBackupIfNeeded( NULL_REPORTER::GetInstance() );
        }

        return true;
    }

    GetBoard()->SetFileName( tmpFileName.GetFullPath() );

    return false;
}


bool PCB_EDIT_FRAME::importFile( const wxString& aFileName, int aFileType )
{
    switch( (IO_MGR::PCB_FILE_T) aFileType )
    {
    case IO_MGR::CADSTAR_PCB_ARCHIVE:
    case IO_MGR::EAGLE:
        return OpenProjectFiles( std::vector<wxString>( 1, aFileName ),
                                 KICTL_EAGLE_BRD | KICTL_IMPORT_LIB );

    default:
        return false;
    }

    return false;
}

