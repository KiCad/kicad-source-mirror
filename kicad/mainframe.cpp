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

/**
 * @file kicad/mainframe.cpp
 * @brief KICAD_MANAGER_FRAME is the KiCad main frame.
 */


#include <draw_frame.h>
#include <panel_hotkeys_editor.h>
#include <gestfich.h>
#include <kiway.h>
#include <kiway_player.h>
#include <wildcards_and_files_ext.h>
#include <bitmaps.h>
#include <executable_names.h>
#include <build_version.h>
#include <dialog_configure_paths.h>
#include <dialog_edit_library_tables.h>
#include <launch_ext.h>
#include "pgm_kicad.h"
#include "tree_project_frame.h"

#ifdef __WXMAC__
#include <MacTypes.h>
#include <ApplicationServices/ApplicationServices.h>
#endif

#include "kicad.h"


#define TREE_FRAME_WIDTH_ENTRY     wxT( "LeftWinWidth" )

KICAD_MANAGER_FRAME::KICAD_MANAGER_FRAME( wxWindow* parent,
        const wxString& title, const wxPoint&  pos, const wxSize&   size ) :
    EDA_BASE_FRAME( parent, KICAD_MAIN_FRAME_T, title, pos, size,
                    KICAD_DEFAULT_DRAWFRAME_STYLE, KICAD_MANAGER_FRAME_NAME ),
    KIWAY_HOLDER( &::Kiway )
{
    m_active_project = false;
    m_leftWinWidth = 60;
    m_manager_Hotkeys_Descr = NULL;
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
    m_LeftWin = new TREE_PROJECT_FRAME( this );

    // Right top Window: buttons to launch applications
    m_Launcher = new LAUNCHER_PANEL( this );

    // Add the wxTextCtrl showing all messages from KiCad:
    m_MessagesBox = new wxTextCtrl( this, wxID_ANY, wxEmptyString,
                                    wxDefaultPosition, wxDefaultSize,
                                    wxTE_MULTILINE | wxTE_READONLY | wxBORDER_NONE );

    RecreateBaseHToolbar();
    ReCreateMenuBar();

    m_auimgr.SetManagedWindow( this );

    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );

    m_auimgr.AddPane( m_LeftWin, EDA_PANE().Palette().Name( "ProjectTree" ).Left().Layer(3)
                      .CaptionVisible( false ).PaneBorder( false )
                      .MinSize( 150, -1 ).BestSize( m_leftWinWidth, -1 ) );

    m_auimgr.AddPane( m_Launcher, EDA_PANE().HToolbar().Name( "Launcher" ).Top().Layer(1)
                      .MinSize( m_Launcher->GetPanelWidth(), m_Launcher->GetPanelHeight() ) );

    m_auimgr.AddPane( m_MessagesBox, EDA_PANE().Messages().Name( "MsgPanel" ).Center() );

    m_auimgr.Update();

    SetTitle( wxString( "KiCad " ) + GetBuildVersion() );
}


KICAD_MANAGER_FRAME::~KICAD_MANAGER_FRAME()
{
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
    m_LeftWin->ReCreateTreePrj();
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
    m_MessagesBox->AppendText( aText );
}


void KICAD_MANAGER_FRAME::OnSize( wxSizeEvent& event )
{
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();

    event.Skip();
}


void KICAD_MANAGER_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
#ifdef _WINDOWS_
    // For some obscure reason, on Windows, when killing Kicad from the Windows task manager
    // if a editor frame (schematic, library, board editor or fp editor) is open and has
    // some edition to save, OnCloseWindow is run twice *at the same time*, creating race
    // conditions between OnCloseWindow() code.
    // Therefore I added (JPC) a ugly hack to discard the second call (unwanted) during
    // execution of the first call (only one call is right).
    // Note also if there is no change made in editors, this behavior does not happen.
    static std::atomic<unsigned int> lock_close_event( 0 );

    if( ++lock_close_event > 1 )    // Skip extra calls
    {
        return;
    }
#endif

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

        m_LeftWin->Show( false );

        Destroy();
    }

#ifdef _WINDOWS_
    lock_close_event = 0;   // Reenable event management
#endif
}


void KICAD_MANAGER_FRAME::OnExit( wxCommandEvent& event )
{
    Close( true );
}


void KICAD_MANAGER_FRAME::TERMINATE_HANDLER::OnTerminate( int pid, int status )
{
    wxString msg = wxString::Format( _( "%s closed [pid=%d]\n" ),
            GetChars( m_appName ), pid );

    wxWindow* window = wxWindow::FindWindowByName( KICAD_MANAGER_FRAME_NAME );

    if( window )    // Should always happen.
    {
        // Be sure the kicad frame manager is found
        // This dynamic cast is not really mandatory, but ...
        KICAD_MANAGER_FRAME* frame = dynamic_cast<KICAD_MANAGER_FRAME*> (window);

        if( frame )
            frame->PrintMsg( msg );
    }

    delete this;
}


void KICAD_MANAGER_FRAME::Execute( wxWindow* frame, const wxString& execFile,
                                   wxString params )
{
    if( params.size() )
        AddDelimiterString( params );

    TERMINATE_HANDLER* callback = new TERMINATE_HANDLER( execFile );

    long pid = ExecuteFile( frame, execFile, params, callback );

    if( pid > 0 )
    {
        wxString msg = wxString::Format( _( "%s %s opened [pid=%ld]\n" ),
                                         GetChars( execFile ), GetChars( params ), pid );

        PrintMsg( msg );

#ifdef __WXMAC__
        msg.Printf( "osascript -e 'activate application \"%s\"' ", execFile );
        system( msg.c_str() );
#endif
    }
    else
    {
        delete callback;
    }
}


void KICAD_MANAGER_FRAME::RunEeschema( const wxString& aProjectSchematicFileName )
{
    KIWAY_PLAYER* frame;

    // Prevent multiple KiFace loading at one time
    if( !m_loading.try_lock() )
        return;

    const std::lock_guard<std::mutex> lock( m_loading, std::adopt_lock );

    try
    {
        frame = Kiway().Player( FRAME_SCH, true );
    }
    catch( const IO_ERROR& err )
    {
        wxMessageBox( _( "Eeschema failed to load:\n" ) + err.What(),
                      _( "KiCad Error" ), wxOK | wxICON_ERROR, this );
        return;
    }

    if( !frame->IsShown() ) // A hidden frame might not have the project loaded.
    {
        if( !frame->OpenProjectFiles( std::vector<wxString>( 1, aProjectSchematicFileName ) ) )
            return;

        frame->Show( true );
    }

    // On Windows, Raise() does not bring the window on screen, when iconized or not shown
    // On linux, Raise() brings the window on screen, but this code works fine
    if( frame->IsIconized() )
    {
        frame->Iconize( false );
        // If an iconized frame was created by Pcbnew, Iconize( false ) is not enough
        // to show the frame at its normal size: Maximize should be called.
        frame->Maximize( false );
    }

    frame->Raise();
}


void KICAD_MANAGER_FRAME::OnRunEeschema( wxCommandEvent& event )
{
    wxFileName fn( GetProjectFileName() );
    fn.SetExt( SchematicFileExtension );
    RunEeschema( fn.GetFullPath() );
}


void KICAD_MANAGER_FRAME::OnRunSchLibEditor( wxCommandEvent& event )
{
    KIWAY_PLAYER* frame;

    // Prevent multiple KiFace loading at one time
    if( !m_loading.try_lock() )
        return;

    const std::lock_guard<std::mutex> lock( m_loading, std::adopt_lock );

    try
    {
        frame = Kiway().Player( FRAME_SCH_LIB_EDITOR, true );
    }
    catch( const IO_ERROR& err )
    {
        wxMessageBox( _( "Component library editor failed to load:\n" ) + err.What(),
                      _( "KiCad Error" ), wxOK | wxICON_ERROR, this );
        return;
    }

    if( !frame->IsShown() )
        frame->Show( true );

    // On Windows, Raise() does not bring the window on screen, when iconized
    if( frame->IsIconized() )
        frame->Iconize( false );

    frame->Raise();
}


void KICAD_MANAGER_FRAME::RunPcbNew( const wxString& aProjectBoardFileName )
{
    KIWAY_PLAYER* frame;

    // Prevent multiple KiFace loading at one time
    if( !m_loading.try_lock() )
        return;

    const std::lock_guard<std::mutex> lock( m_loading, std::adopt_lock );

    try
    {
        frame = Kiway().Player( FRAME_PCB, true );
    }
    catch( const IO_ERROR& err )
    {
        wxMessageBox( _( "Pcbnew failed to load:\n" ) + err.What(), _( "KiCad Error" ),
                      wxOK | wxICON_ERROR, this );
        return;
    }

    if( !frame->IsVisible() )   // A hidden frame might not have the board loaded.
    {
        if( !frame->OpenProjectFiles( std::vector<wxString>( 1, aProjectBoardFileName ) ) )
            return;

        frame->Show( true );
    }

    // On Windows, Raise() does not bring the window on screen, when iconized
    if( frame->IsIconized() )
        frame->Iconize( false );

    frame->Raise();
}


void KICAD_MANAGER_FRAME::OnRunPcbNew( wxCommandEvent& event )
{
    wxFileName  kicad_board( PcbFileName() );
    wxFileName  legacy_board( PcbLegacyFileName() );

    wxFileName& board = ( !legacy_board.FileExists() || kicad_board.FileExists() ) ?
                            kicad_board : legacy_board;

    RunPcbNew( board.GetFullPath() );
}


void KICAD_MANAGER_FRAME::OnRunPcbFpEditor( wxCommandEvent& event )
{
    KIWAY_PLAYER* frame;

    // Prevent multiple KiFace loading at one time
    if( !m_loading.try_lock() )
        return;

    const std::lock_guard<std::mutex> lock( m_loading, std::adopt_lock );

    try
    {
        frame = Kiway().Player( FRAME_PCB_MODULE_EDITOR, true );
    }
    catch( const IO_ERROR& err )
    {
        wxMessageBox( _( "Footprint library editor failed to load:\n" ) + err.What(),
                      _( "KiCad Error" ), wxOK | wxICON_ERROR, this );
        return;
    }

    if( !frame->IsShown() )
        frame->Show( true );

    // On Windows, Raise() does not bring the window on screen, when iconized
    if( frame->IsIconized() )
        frame->Iconize( false );

    frame->Raise();
}


void KICAD_MANAGER_FRAME::OnRunBitmapConverter( wxCommandEvent& event )
{
    Execute( this, BITMAPCONVERTER_EXE );
}


void KICAD_MANAGER_FRAME::OnRunPcbCalculator( wxCommandEvent& event )
{
    Execute( this, PCB_CALCULATOR_EXE );
}


void KICAD_MANAGER_FRAME::OnRunPageLayoutEditor( wxCommandEvent& event )
{
    Execute( this, PL_EDITOR_EXE );
}



void KICAD_MANAGER_FRAME::OnRunGerbview( wxCommandEvent& event )
{
    // Gerbview is called without any file to open, because we do not know
    // the list and the name of files to open (if any...).
    // however we run it in the path of the project
    Execute( this, GERBVIEW_EXE, Prj().GetProjectPath() );
}


void KICAD_MANAGER_FRAME::OnOpenTextEditor( wxCommandEvent& event )
{
    wxString editorname = Pgm().GetEditorName();

    if( !editorname.IsEmpty() )
        Execute( this, editorname, wxEmptyString );
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
        Execute( this, Pgm().GetEditorName(), filename );
}

void KICAD_MANAGER_FRAME::OnBrowseInFileExplorer( wxCommandEvent& event )
{
    // open project directory in host OS's file explorer
    LaunchExternal( Prj().GetProjectPath() );
}


void KICAD_MANAGER_FRAME::OnRefresh( wxCommandEvent& event )
{
    m_LeftWin->ReCreateTreePrj();
}


void KICAD_MANAGER_FRAME::language_change( wxCommandEvent& event )
{
    int id = event.GetId();
    Kiway().SetLanguage( id );
}


void KICAD_MANAGER_FRAME::CommonSettingsChanged()
{
    int historySize;
    Pgm().CommonSettings()->Read( FILE_HISTORY_SIZE_KEY, &historySize, DEFAULT_FILE_HISTORY_SIZE );
    PgmTop().GetFileHistory().SetMaxFiles( (unsigned) std::max( 0, historySize ) );
}


void KICAD_MANAGER_FRAME::ClearMsg()
{
    m_MessagesBox->Clear();
}


void KICAD_MANAGER_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );
    aCfg->Read( TREE_FRAME_WIDTH_ENTRY, &m_leftWinWidth );
}


void KICAD_MANAGER_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );
    aCfg->Write( TREE_FRAME_WIDTH_ENTRY, m_LeftWin->GetSize().x );
}


void KICAD_MANAGER_FRAME::PrintPrjInfo()
{
    wxString msg = wxString::Format( _( "Project name:\n%s\n" ),
                        GetChars( GetProjectFileName() ) );
    PrintMsg( msg );
}


void KICAD_MANAGER_FRAME::OnShowHotkeys( wxCommandEvent& event )
{
    DisplayHotkeyList( this, m_manager_Hotkeys_Descr );
}


void KICAD_MANAGER_FRAME::OnConfigurePaths( wxCommandEvent& aEvent )
{
    DIALOG_CONFIGURE_PATHS dlg( this, nullptr );
    dlg.ShowModal();
}


void KICAD_MANAGER_FRAME::OnEditSymLibTable( wxCommandEvent& aEvent )
{
    try     // _eeschema.kiface must be available: it contains the configure dialog.
    {
        KIFACE* kiface = Kiway().KiFACE( KIWAY::FACE_SCH );
        kiface->CreateWindow( this, DIALOG_SCH_LIBRARY_TABLE, &Kiway() );
    }
    catch( ... )
    {
        // Do nothing here.
        // A error message is displayed after trying to load _pcbnew.kiface.
    }
}


void KICAD_MANAGER_FRAME::OnEditFpLibTable( wxCommandEvent& aEvent )
{
    try     // _pcbnew.kiface must be available: it contains the configure dialog.
    {
        KIFACE* kiface = Kiway().KiFACE( KIWAY::FACE_PCB );
        kiface->CreateWindow( this, DIALOG_PCB_LIBRARY_TABLE, &Kiway() );
    }
    catch( ... )
    {
        // Do nothing here.
        // A error message is displayed after trying to load _pcbnew.kiface.
    }
}


void KICAD_MANAGER_FRAME::OnPreferences( wxCommandEvent& aEvent )
{
    ShowPreferences( m_manager_Hotkeys_Descr, m_manager_Hotkeys_Descr, wxT( "kicad" ) );
}
