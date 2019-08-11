/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Copyright (C) 2004-2019 KiCad Developers, see change_log.txt for contributors.
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

#include "kicad_id.h"
#include "pgm_kicad.h"
#include "tree_project_frame.h"
#include <bitmaps.h>
#include <build_version.h>
#include <executable_names.h>
#include <gestfich.h>
#include <kiway.h>
#include <kiway_express.h>
#include <kiway_player.h>
#include <panel_hotkeys_editor.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/tool_manager.h>
#include <tools/kicad_manager_actions.h>
#include <tools/kicad_manager_control.h>
#include <wildcards_and_files_ext.h>

#ifdef __WXMAC__
#include <MacTypes.h>
#include <ApplicationServices/ApplicationServices.h>
#endif

#include "kicad_manager_frame.h"


#define TREE_FRAME_WIDTH_ENTRY     wxT( "LeftWinWidth" )

#define SEP()   wxFileName::GetPathSeparator()

// Not really useful, provided to save/restore params in project config file,
// (Add them in s_KicadManagerParams if any)
// Used also to create new .pro files from the kicad.pro template file
// for new projects
#define     GeneralGroupName            wxT( "/general" )

PARAM_CFG_ARRAY     s_KicadManagerParams;


// Menubar and toolbar event table
BEGIN_EVENT_TABLE( KICAD_MANAGER_FRAME, EDA_BASE_FRAME )
    // Window events
    EVT_SIZE( KICAD_MANAGER_FRAME::OnSize )
    EVT_CLOSE( KICAD_MANAGER_FRAME::OnCloseWindow )

    // Menu events
    EVT_MENU( wxID_EXIT, KICAD_MANAGER_FRAME::OnExit )
    EVT_MENU( ID_EDIT_LOCAL_FILE_IN_TEXT_EDITOR, KICAD_MANAGER_FRAME::OnOpenFileInTextEditor )
    EVT_MENU( ID_BROWSE_IN_FILE_EXPLORER, KICAD_MANAGER_FRAME::OnBrowseInFileExplorer )
    EVT_MENU( ID_SAVE_AND_ZIP_FILES, KICAD_MANAGER_FRAME::OnArchiveFiles )
    EVT_MENU( ID_READ_ZIP_ARCHIVE, KICAD_MANAGER_FRAME::OnUnarchiveFiles )
    EVT_MENU( ID_IMPORT_EAGLE_PROJECT, KICAD_MANAGER_FRAME::OnImportEagleFiles )

    // Range menu events
    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END,
                    KICAD_MANAGER_FRAME::language_change )

    EVT_MENU_RANGE( ID_FILE1, ID_FILEMAX, KICAD_MANAGER_FRAME::OnFileHistory )

    // Special functions
    EVT_MENU( ID_INIT_WATCHED_PATHS, KICAD_MANAGER_FRAME::OnChangeWatchedPaths )
END_EVENT_TABLE()


KICAD_MANAGER_FRAME::KICAD_MANAGER_FRAME( wxWindow* parent, const wxString& title,
                                          const wxPoint& pos, const wxSize&   size ) :
        EDA_BASE_FRAME( parent, KICAD_MAIN_FRAME_T, title, pos, size,
                        KICAD_DEFAULT_DRAWFRAME_STYLE, KICAD_MANAGER_FRAME_NAME, &::Kiway ),
        m_leftWin( nullptr ),
        m_launcher( nullptr ),
        m_messagesBox( nullptr ),
        m_mainToolBar( nullptr )
{
    m_active_project = false;
    m_leftWinWidth = 250;       // Default value
    m_AboutTitle = "KiCad";

    // Create the status line (bottom of the frame)
    static const int dims[3] = { -1, -1, 100 };

    CreateStatusBar( 3 );
    SetStatusWidths( 3, dims );

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_kicad_xpm ) );
    SetIcon( icon );

    // Give the last size and pos to main window
    LoadSettings( config() );
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    // Left window: is the box which display tree project
    m_leftWin = new TREE_PROJECT_FRAME( this );

    // Add the wxTextCtrl showing all messages from KiCad:
    m_messagesBox = new wxTextCtrl( this, wxID_ANY, wxEmptyString,
                                    wxDefaultPosition, wxDefaultSize,
                                    wxTE_MULTILINE | wxTE_READONLY | wxBORDER_NONE );

    // Create the manager
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( nullptr, nullptr, nullptr, this );
    m_actions = new KICAD_MANAGER_ACTIONS();

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new KICAD_MANAGER_CONTROL );
    m_toolManager->InitTools();

    RecreateBaseHToolbar();
    RecreateLauncher();
    ReCreateMenuBar();

    m_auimgr.SetManagedWindow( this );

    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );

    // BestSize() does not always set the actual pane size of m_leftWin to the required value.
    // It happens when m_leftWin is too large (roughly > 1/3 of the kicad manager frame width.
    // (Well, BestSize() sets the best size... not the window size)
    // A trick is to use MinSize() to set the required pane width,
    // and after give a reasonable MinSize value
    m_auimgr.AddPane( m_leftWin, EDA_PANE().Palette().Name( "ProjectTree" ).Left().Layer(3)
                      .CaptionVisible( false ).PaneBorder( false )
                      .MinSize( m_leftWinWidth, -1 ).BestSize( m_leftWinWidth, -1 ) );

    m_auimgr.AddPane( m_launcher, EDA_PANE().HToolbar().Name( "Launcher" ).Top().Layer(1) );

    m_auimgr.AddPane( m_messagesBox, EDA_PANE().Messages().Name( "MsgPanel" ).Center() );

    m_auimgr.Update();

    // Now the actual m_leftWin size is set, give it a reasonable min width
    m_auimgr.GetPane( m_leftWin ).MinSize( 200, -1 );

    SetTitle( wxString( "KiCad " ) + GetBuildVersion() );
}


KICAD_MANAGER_FRAME::~KICAD_MANAGER_FRAME()
{
    // Ensure there are no active tools
    if( m_toolManager )
        m_toolManager->DeactivateTool();

    delete m_actions;
    delete m_toolManager;

    m_auimgr.UnInit();
}


wxConfigBase* KICAD_MANAGER_FRAME::config()
{
    wxConfigBase* ret = PgmTop().PgmSettings();
    wxASSERT( ret );
    return ret;
}


void KICAD_MANAGER_FRAME::SetProjectFileName( const wxString& aFullProjectProFileName )
{
    // ensure file name is absolute:
    wxFileName fn( aFullProjectProFileName );

    if( !fn.IsAbsolute() )
        fn.MakeAbsolute();

    Prj().SetProjectFullName( fn.GetFullPath() );

    SetTitle( wxString( "KiCad " ) + GetBuildVersion() );
    wxString title = GetTitle() + " " + fn.GetFullPath();

    if( !fn.IsDirWritable() )
        title += _( " [Read Only]" );

    SetTitle( title );
}


const wxString KICAD_MANAGER_FRAME::GetProjectFileName()
{
    return  Prj().GetProjectFullName();
}


const wxString KICAD_MANAGER_FRAME::SchFileName()
{
   wxFileName   fn( GetProjectFileName() );

   fn.SetExt( SchematicFileExtension );
   return fn.GetFullPath();
}


const wxString KICAD_MANAGER_FRAME::PcbFileName()
{
   wxFileName   fn( GetProjectFileName() );

   fn.SetExt( PcbFileExtension );
   return fn.GetFullPath();
}


const wxString KICAD_MANAGER_FRAME::PcbLegacyFileName()
{
   wxFileName   fn( GetProjectFileName() );

   fn.SetExt( LegacyPcbFileExtension );
   return fn.GetFullPath();
}


void KICAD_MANAGER_FRAME::ReCreateTreePrj()
{
    m_leftWin->ReCreateTreePrj();
}


const SEARCH_STACK& KICAD_MANAGER_FRAME::sys_search()
{
    return PgmTop().SysSearch();
}


wxString KICAD_MANAGER_FRAME::help_name()
{
    return PgmTop().GetHelpFileName();
}


void KICAD_MANAGER_FRAME::PrintMsg( const wxString& aText )
{
    m_messagesBox->AppendText( aText );
}


void KICAD_MANAGER_FRAME::OnSize( wxSizeEvent& event )
{
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();

    event.Skip();
}


void KICAD_MANAGER_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    if( Kiway().PlayersClose( false ) )
    {
        int px, py;

        if( !GetProjectFileName().empty() )
            UpdateFileHistory( GetProjectFileName(), &PgmTop().GetFileHistory() );

        if( !IsIconized() )   // save main frame position and size
        {
            GetPosition( &px, &py );
            m_FramePos.x = px;
            m_FramePos.y = py;

            GetSize( &px, &py );
            m_FrameSize.x = px;
            m_FrameSize.y = py;
        }

        Event.SetCanVeto( true );

        m_leftWin->Show( false );

        Destroy();
    }
}


void KICAD_MANAGER_FRAME::OnExit( wxCommandEvent& event )
{
    Close( true );
}


void KICAD_MANAGER_FRAME::LoadProject( const wxFileName& aProjectFileName )
{
    // The project file should be valid by the time we get here or something has gone wrong.
    if( !aProjectFileName.Exists() )
        return;

    // Any open KIFACE's must be closed if they are not part of the new project.
    // (We never want a KIWAY_PLAYER open on a KIWAY that isn't in the same project.)
    // User is prompted here to close those KIWAY_PLAYERs:
    if( !Kiway().PlayersClose( false ) )
        return;

    // Save the project file for the currently loaded project.
    if( m_active_project )
        Prj().ConfigLoad( PgmTop().SysSearch(), GeneralGroupName, s_KicadManagerParams );

    m_active_project = true;
    ClearMsg();
    SetProjectFileName( aProjectFileName.GetFullPath() );
    Prj().ConfigLoad( PgmTop().SysSearch(), GeneralGroupName, s_KicadManagerParams );

    if( aProjectFileName.IsDirWritable() )
        SetMruPath( Prj().GetProjectPath() ); // Only set MRU path if we have write access. Why?

    UpdateFileHistory( aProjectFileName.GetFullPath(), &PgmTop().GetFileHistory() );

    m_leftWin->ReCreateTreePrj();

    SyncToolbars();

    // Rebuild the list of watched paths.
    // however this is possible only when the main loop event handler is running,
    // so we use it to run the rebuild function.
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED, ID_INIT_WATCHED_PATHS );

    wxPostEvent( this, cmd );

    PrintPrjInfo();
}


void KICAD_MANAGER_FRAME::CreateNewProject( const wxFileName& aProjectFileName )
{
    wxCHECK_RET( aProjectFileName.DirExists() && aProjectFileName.IsDirWritable(),
                 "Project folder must exist and be writable to create a new project." );

    // Init project filename.  This clears all elements from the project object.
    SetProjectFileName( aProjectFileName.GetFullPath() );

    // Copy kicad.pro file from template folder.
    if( !aProjectFileName.FileExists() )
    {
        wxString srcFileName = sys_search().FindValidPath( "kicad.pro" );

        // Create a minimal project (.pro) file if the template project file could not be copied.
        if( !wxFileName::FileExists( srcFileName )
            || !wxCopyFile( srcFileName, aProjectFileName.GetFullPath() ) )
        {
            Prj().ConfigSave( PgmTop().SysSearch(), GeneralGroupName, s_KicadManagerParams );
        }
    }

    // Ensure a "stub" for a schematic root sheet and a board exist.
    // It will avoid messages from the schematic editor or the board editor to create a new file
    // And forces the user to create main files under the right name for the project manager
    wxFileName fn( aProjectFileName.GetFullPath() );
    fn.SetExt( SchematicFileExtension );

    // If a <project>.sch file does not exist, create a "stub" file ( minimal schematic file )
    if( !fn.FileExists() )
    {
        wxFile file( fn.GetFullPath(), wxFile::write );

        if( file.IsOpened() )
            file.Write( wxT( "EESchema Schematic File Version 2\n"
                             "EELAYER 25 0\nEELAYER END\n$EndSCHEMATC\n" ) );

        // wxFile dtor will close the file
    }

    // If a <project>.kicad_pcb or <project>.brd file does not exist,
    // create a .kicad_pcb "stub" file
    fn.SetExt( KiCadPcbFileExtension );
    wxFileName leg_fn( fn );
    leg_fn.SetExt( LegacyPcbFileExtension );

    if( !fn.FileExists() && !leg_fn.FileExists() )
    {
        wxFile file( fn.GetFullPath(), wxFile::write );

        if( file.IsOpened() )
            file.Write( wxT( "(kicad_pcb (version 4) (host kicad \"dummy file\") )\n" ) );

        // wxFile dtor will close the file
    }
}


void KICAD_MANAGER_FRAME::OnOpenFileInTextEditor( wxCommandEvent& event )
{
    // show all files in file dialog (in Kicad all files are editable texts):
    wxString wildcard = AllFilesWildcard();

    wxString default_dir = Prj().GetProjectPath();

    wxFileDialog dlg( this, _( "Load File to Edit" ), default_dir,
                      wxEmptyString, wildcard, wxFD_OPEN );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxString filename = wxT( "\"" );
    filename += dlg.GetPath() + wxT( "\"" );

    if( !dlg.GetPath().IsEmpty() &&  !Pgm().GetEditorName().IsEmpty() )
        m_toolManager->RunAction( KICAD_MANAGER_ACTIONS::openTextEditor, true, &filename );
}


void KICAD_MANAGER_FRAME::OnBrowseInFileExplorer( wxCommandEvent& event )
{
    // open project directory in host OS's file explorer
    wxString project_dir = Prj().GetProjectPath();

#ifdef __WXMAC__
    wxString msg;

    // Quote in case there are spaces in the path.
    msg.Printf( "open \"%s\"", project_dir );

    system( msg.c_str() );
#else
    // Quote in case there are spaces in the path.
    AddDelimiterString( project_dir );

    wxLaunchDefaultApplication( project_dir );
#endif
}


void KICAD_MANAGER_FRAME::RefreshProjectTree()
{
    m_leftWin->ReCreateTreePrj();
}


void KICAD_MANAGER_FRAME::language_change( wxCommandEvent& event )
{
    int id = event.GetId();
    Kiway().SetLanguage( id );
}


void KICAD_MANAGER_FRAME::ShowChangedLanguage()
{
    // call my base class
    EDA_BASE_FRAME::ShowChangedLanguage();

    // tooltips in toolbars
    RecreateBaseHToolbar();
    RecreateLauncher();

    PrintPrjInfo();
}


void KICAD_MANAGER_FRAME::CommonSettingsChanged( bool aEnvVarsChanged )
{
    int historySize;
    Pgm().CommonSettings()->Read( FILE_HISTORY_SIZE_KEY, &historySize, DEFAULT_FILE_HISTORY_SIZE );
    PgmTop().GetFileHistory().SetMaxFiles( (unsigned) std::max( 0, historySize ) );
}


void KICAD_MANAGER_FRAME::ClearMsg()
{
    m_messagesBox->Clear();
}


void KICAD_MANAGER_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );
    aCfg->Read( TREE_FRAME_WIDTH_ENTRY, &m_leftWinWidth );
}


void KICAD_MANAGER_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );
    aCfg->Write( TREE_FRAME_WIDTH_ENTRY, m_leftWin->GetSize().x );
}


void KICAD_MANAGER_FRAME::InstallPreferences( PAGED_DIALOG* aParent,
                                              PANEL_HOTKEYS_EDITOR* aHotkeysPanel  )
{
    aHotkeysPanel->AddHotKeys( GetToolManager() );
}





void KICAD_MANAGER_FRAME::PrintPrjInfo()
{
    wxString msg = wxString::Format( _( "Project name:\n%s\n" ),
                        GetChars( GetProjectFileName() ) );
    PrintMsg( msg );
}


