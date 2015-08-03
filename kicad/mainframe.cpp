/**
 * @file kicad/mainframe.cpp
 * @brief KICAD_MANAGER_FRAME is the KiCad main frame.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Copyright (C) 2004-2015 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <pgm_kicad.h>
#include <kiway.h>
#include <kiway_player.h>
#include <confirm.h>
#include <gestfich.h>
#include <macros.h>

#include <kicad.h>
#include <tree_project_frame.h>
#include <wildcards_and_files_ext.h>
#include <menus_helpers.h>
#include <dialog_hotkeys_editor.h>


#define TREE_FRAME_WIDTH_ENTRY     wxT( "LeftWinWidth" )

KICAD_MANAGER_FRAME::KICAD_MANAGER_FRAME( wxWindow* parent,
        const wxString& title, const wxPoint&  pos, const wxSize&   size ) :
    EDA_BASE_FRAME( parent, KICAD_MAIN_FRAME_T, title, pos, size,
                    KICAD_DEFAULT_DRAWFRAME_STYLE, KICAD_MANAGER_FRAME_NAME )
{
    m_active_project = false;
    m_leftWinWidth = 60;
    m_manager_Hokeys_Descr = NULL;

    // Create the status line (bottom of the frame
    static const int dims[3] = { -1, -1, 100 };

    CreateStatusBar( 3 );
    SetStatusWidths( 3, dims );

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_kicad_xpm ) );
    SetIcon( icon );

    // Give the last sise and pos to main window
    LoadSettings( config() );
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    // Left window: is the box which display tree project
    m_LeftWin = new TREE_PROJECT_FRAME( this );

    // Right top Window: buttons to launch applications
    m_Launcher = new LAUNCHER_PANEL( this );

    // Add the wxTextCtrl showing all messages from KiCad:
    m_MessagesBox = new wxTextCtrl( this, wxID_ANY, wxEmptyString,
                                    wxDefaultPosition, wxDefaultSize,
                                    wxTE_MULTILINE | wxSUNKEN_BORDER | wxTE_READONLY );

    RecreateBaseHToolbar();
    ReCreateMenuBar();

    m_auimgr.SetManagedWindow( this );

    EDA_PANEINFO horiztb;
    horiztb.HorizontalToolbarPane();

    EDA_PANEINFO info;
    info.InfoToolbarPane();

    m_auimgr.AddPane( m_mainToolBar,
                      wxAuiPaneInfo( horiztb ).Name( wxT( "m_mainToolBar" ) ).Top() );

    m_auimgr.AddPane( m_LeftWin,
                      wxAuiPaneInfo(info).Name( wxT( "m_LeftWin" ) ).Left().
                      BestSize( m_leftWinWidth, -1 ).
                      Layer( 1 ) );

    m_auimgr.AddPane( m_Launcher, wxTOP );
    m_auimgr.GetPane( m_Launcher).CaptionVisible( false ).Row(1)
        .BestSize( -1, m_Launcher->GetPanelHeight() ).PaneBorder( false ).Resizable( false );

    m_auimgr.AddPane( m_MessagesBox,
                      wxAuiPaneInfo().Name( wxT( "m_MessagesBox" ) ).CentrePane().Layer( 2 ) );

    m_auimgr.GetPane( m_LeftWin ).MinSize( wxSize( 80, -1) );
    m_auimgr.GetPane( m_LeftWin ).BestSize(wxSize(m_leftWinWidth, -1) );

    m_auimgr.Update();
}


KICAD_MANAGER_FRAME::~KICAD_MANAGER_FRAME()
{
    m_auimgr.UnInit();
}


wxConfigBase* KICAD_MANAGER_FRAME::config()
{
    wxConfigBase* ret = Pgm().PgmSettings();
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
    return Pgm().SysSearch();
}


wxString KICAD_MANAGER_FRAME::help_name()
{
    return Pgm().GetHelpFileName();
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
    if( Kiway.PlayersClose( false ) )
    {
        int px, py;

        UpdateFileHistory( GetProjectFileName(), &Pgm().GetFileHistory() );

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
}


void KICAD_MANAGER_FRAME::OnExit( wxCommandEvent& event )
{
    Close( true );
}


void KICAD_MANAGER_FRAME::TERMINATE_HANDLER::OnTerminate( int pid, int status )
{
    wxString msg = wxString::Format( _( "%s closed [pid=%d]\n" ),
            GetChars( appName ), pid );

    ( (KICAD_MANAGER_FRAME*) Pgm().App().GetTopWindow() )->PrintMsg( msg );

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
        wxString msg = wxString::Format( _( "%s opened [pid=%ld]\n" ),
                GetChars( execFile ), pid );

        PrintMsg( msg );
    }
    else
    {
        delete callback;
    }
}


void KICAD_MANAGER_FRAME::RunEeschema( const wxString& aProjectSchematicFileName )
{
    KIWAY_PLAYER* frame = Kiway.Player( FRAME_SCH, false );

    // Please: note: DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::initBuffers() calls
    // Kiway.Player( FRAME_SCH, true )
    // therefore, the schematic editor is sometimes running, but the schematic project
    // is not loaded, if the library editor was called, and the dialog field editor was used.
    // On linux, it happens the first time the schematic editor is launched, if
    // library editor was running, and the dialog field editor was open
    // On Windows, it happens always after the library editor was called,
    // and the dialog field editor was used
    if( !frame )
    {
        frame = Kiway.Player( FRAME_SCH, true );
    }

    if( !frame->IsShown() ) // the frame exists, (created by the dialog field editor)
                            // but no project loaded.
    {
        frame->OpenProjectFiles( std::vector<wxString>( 1, aProjectSchematicFileName ) );
        frame->Show( true );
    }

    // On Windows, Raise() does not bring the window on screen, when iconized or not shown
    // On linux, Raise() brings the window on screen, but this code works fine
    if( frame->IsIconized() )
        frame->Iconize( false );

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
    KIWAY_PLAYER* frame = Kiway.Player( FRAME_SCH_LIB_EDITOR, false );

    if( !frame )
    {
        frame = Kiway.Player( FRAME_SCH_LIB_EDITOR, true );
        // frame->OpenProjectFiles( std::vector<wxString>( 1, aProjectSchematicFileName ) );
        frame->Show( true );
    }

    // On Windows, Raise() does not bring the window on screen, when iconized
    if( frame->IsIconized() )
        frame->Iconize( false );

    frame->Raise();
}


void KICAD_MANAGER_FRAME::RunPcbNew( const wxString& aProjectBoardFileName )
{
    KIWAY_PLAYER* frame = Kiway.Player( FRAME_PCB, true );

    // a pcb frame can be already existing, but not yet used.
    // this is the case when running the footprint editor, or the footprint viewer first
    // if the frame is not visible, the board is not yet loaded
    if( !frame->IsVisible() )
    {
        frame->OpenProjectFiles( std::vector<wxString>( 1, aProjectBoardFileName ) );
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
    KIWAY_PLAYER* frame = Kiway.Player( FRAME_PCB_MODULE_EDITOR, false );

    if( !frame )
    {
        frame = Kiway.Player( FRAME_PCB_MODULE_EDITOR, true );
//        frame->OpenProjectFiles( std::vector<wxString>( 1, aProjectBoardFileName ) );
        frame->Show( true );
    }

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


// Dead code: Cvpcb can be run only from the schematic editor now,
// This is due to the fact the footprint field of components in schematics
// are now always set by Cvpcb.
// ( The idea is to drop the .cmp files to avoid to have 2 places were
// footprints are stored, but only one: the schematic )
/*
void KICAD_MANAGER_FRAME::OnRunCvpcb( wxCommandEvent& event )
{
    wxFileName fn( GetProjectFileName() );

    fn.SetExt( NetlistFileExtension );

    KIWAY_PLAYER* frame = Kiway.Player( FRAME_CVPCB, false );
    if( !frame )
    {
        frame = Kiway.Player( FRAME_CVPCB, true );
        frame->OpenProjectFiles( std::vector<wxString>( 1, fn.GetFullPath() ) );
        frame->Show( true );
    }

    frame->Raise();
}
*/


#include <wx/filefn.h>
void KICAD_MANAGER_FRAME::OnRunGerbview( wxCommandEvent& event )
{
    // Gerbview is called without any file to open, because we do not know
    // the list and the name of files to open (if any...).
    // however we run it in the path of the project
    wxString cwd = wxGetCwd();
    wxSetWorkingDirectory( Prj().GetProjectPath() );
    Execute( this, GERBVIEW_EXE, wxEmptyString );
    wxSetWorkingDirectory( cwd );
}


void KICAD_MANAGER_FRAME::OnOpenTextEditor( wxCommandEvent& event )
{
    wxString editorname = Pgm().GetEditorName();

    if( !editorname.IsEmpty() )
        Execute( this, editorname, wxEmptyString );
}


void KICAD_MANAGER_FRAME::OnOpenFileInTextEditor( wxCommandEvent& event )
{
    wxString mask( wxT( "*" ) );

#ifdef __WINDOWS__
    mask += wxT( ".*" );
#endif

    mask = _( "Text file (" ) + mask + wxT( ")|" ) + mask;
    wxString default_dir = Prj().GetProjectPath();

    wxFileDialog dlg( this, _( "Load File to Edit" ), default_dir,
                      wxEmptyString, mask, wxFD_OPEN );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxString filename = wxT( "\"" );
    filename += dlg.GetPath() + wxT( "\"" );

    if( !dlg.GetPath().IsEmpty() &&  !Pgm().GetEditorName().IsEmpty() )
        Execute( this, Pgm().GetEditorName(), filename );
}


void KICAD_MANAGER_FRAME::OnRefresh( wxCommandEvent& event )
{
    m_LeftWin->ReCreateTreePrj();
}


void KICAD_MANAGER_FRAME::language_change( wxCommandEvent& event )
{
    int id = event.GetId();

    Kiway.SetLanguage( id );
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


/**
 * a minor helper function:
 * Prints the Current Project full name on the text panel.
 */
void KICAD_MANAGER_FRAME::PrintPrjInfo()
{
    wxString msg = wxString::Format( _( "Project name:\n%s\n" ),
                        GetChars( GetProjectFileName() ) );
    PrintMsg( msg );
}

void KICAD_MANAGER_FRAME::Process_Config( wxCommandEvent& event )
{
    int        id = event.GetId();
    wxFileName fn;

    switch( id )
    {
    // Hotkey IDs
    case ID_PREFERENCES_HOTKEY_SHOW_EDITOR:
        InstallHotkeyFrame( this, m_manager_Hokeys_Descr );
        break;

    case ID_PREFERENCES_HOTKEY_EXPORT_CONFIG:
        ExportHotkeyConfigToFile( m_manager_Hokeys_Descr, wxT( "kicad" ) );
        break;

    case ID_PREFERENCES_HOTKEY_IMPORT_CONFIG:
        ImportHotkeyConfigFromFile( m_manager_Hokeys_Descr, wxT( "kicad" ) );
        break;

    case ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST:
        // Display current hotkey list for LibEdit.
        DisplayHotkeyList( this, m_manager_Hokeys_Descr );
        break;

    default:
        wxFAIL_MSG( wxT( "KICAD_MANAGER_FRAME::Process_Config error" ) );
        break;
    }
}


void KICAD_MANAGER_FRAME::OnConfigurePaths( wxCommandEvent& aEvent )
{
    Pgm().ConfigurePaths( this );
}
