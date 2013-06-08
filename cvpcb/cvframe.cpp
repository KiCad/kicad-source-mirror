/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jean-pierre.charras
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file cvframe.cpp
 */

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <macros.h>
#include <confirm.h>
#include <eda_doc.h>
#include <eda_dde.h>
#include <gestfich.h>

#include <cvpcb_mainframe.h>
#include <cvstruct.h>
#include <dialog_cvpcb_config.h>
#include <class_DisplayFootprintsFrame.h>
#include <cvpcb_id.h>
#include <html_messagebox.h>
#include <wildcards_and_files_ext.h>

#include <build_version.h>

#define FRAME_MIN_SIZE_X 450
#define FRAME_MIN_SIZE_Y 300


// option key to close CvPcb after saving files
static const wxString KeepCvpcbOpenEntry( wxT( "KeepCvpcbOpen" ) );
static const wxString FootprintDocFileEntry( wxT( "footprints_doc_file" ) );


BEGIN_EVENT_TABLE( CVPCB_MAINFRAME, EDA_BASE_FRAME )
    EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, CVPCB_MAINFRAME::LoadNetList )

    // Menu events
    EVT_MENU( ID_LOAD_PROJECT, CVPCB_MAINFRAME::LoadNetList )
    EVT_MENU( wxID_SAVE, CVPCB_MAINFRAME::SaveQuitCvpcb )
    EVT_MENU( wxID_SAVEAS, CVPCB_MAINFRAME::SaveQuitCvpcb )
    EVT_MENU( wxID_EXIT, CVPCB_MAINFRAME::OnQuit )
    EVT_MENU( wxID_HELP, CVPCB_MAINFRAME::GetKicadHelp )
    EVT_MENU( wxID_ABOUT, CVPCB_MAINFRAME::GetKicadAbout )
    EVT_MENU( wxID_PREFERENCES, CVPCB_MAINFRAME::ConfigCvpcb )
    EVT_MENU( ID_SAVE_PROJECT, CVPCB_MAINFRAME::SaveProjectFile )
    EVT_MENU( ID_SAVE_PROJECT_AS, CVPCB_MAINFRAME::SaveProjectFile )
    EVT_MENU( ID_CVPCB_CONFIG_KEEP_OPEN_ON_SAVE, CVPCB_MAINFRAME::OnKeepOpenOnSave )

    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END, CVPCB_MAINFRAME::SetLanguage )

    // Toolbar events
    EVT_TOOL( ID_CVPCB_QUIT, CVPCB_MAINFRAME::OnQuit )
    EVT_TOOL( ID_CVPCB_READ_INPUT_NETLIST, CVPCB_MAINFRAME::LoadNetList )
    EVT_TOOL( ID_CVPCB_CREATE_CONFIGWINDOW, CVPCB_MAINFRAME::ConfigCvpcb )
    EVT_TOOL( ID_CVPCB_CREATE_SCREENCMP, CVPCB_MAINFRAME::DisplayModule )
    EVT_TOOL( ID_CVPCB_GOTO_FIRSTNA, CVPCB_MAINFRAME::ToFirstNA )
    EVT_TOOL( ID_CVPCB_GOTO_PREVIOUSNA, CVPCB_MAINFRAME::ToPreviousNA )
    EVT_TOOL( ID_CVPCB_DEL_ASSOCIATIONS, CVPCB_MAINFRAME::DelAssociations )
    EVT_TOOL( ID_CVPCB_AUTO_ASSOCIE, CVPCB_MAINFRAME::AssocieModule )
    EVT_TOOL( ID_PCB_DISPLAY_FOOTPRINT_DOC, CVPCB_MAINFRAME::DisplayDocFile )
    EVT_TOOL( ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST,
              CVPCB_MAINFRAME::OnSelectFilteringFootprint )
    EVT_TOOL( ID_CVPCB_FOOTPRINT_DISPLAY_PIN_FILTERED_LIST,
              CVPCB_MAINFRAME::OnSelectFilteringFootprint )
    EVT_TOOL( ID_CVPCB_FOOTPRINT_DISPLAY_BY_LIBRARY_LIST,
              CVPCB_MAINFRAME::OnSelectFilteringFootprint )

    // Frame events
    EVT_CLOSE( CVPCB_MAINFRAME::OnCloseWindow )
    EVT_SIZE( CVPCB_MAINFRAME::OnSize )

    EVT_UPDATE_UI( ID_CVPCB_CONFIG_KEEP_OPEN_ON_SAVE, CVPCB_MAINFRAME::OnUpdateKeepOpenOnSave )
END_EVENT_TABLE()


#define CVPCB_MAINFRAME_NAME wxT( "CvpcbFrame" )

CVPCB_MAINFRAME::CVPCB_MAINFRAME( const wxString& title, long style ) :
    EDA_BASE_FRAME( NULL, CVPCB_FRAME_TYPE, title, wxDefaultPosition,
                    wxDefaultSize, style, CVPCB_MAINFRAME_NAME )
{
    m_FrameName             = CVPCB_MAINFRAME_NAME;
    m_ListCmp               = NULL;
    m_FootprintList         = NULL;
    m_LibraryList           = NULL;
    m_DisplayFootprintFrame = NULL;
    m_mainToolBar           = NULL;
    m_modified              = false;
    m_isEESchemaNetlist     = false;
    m_KeepCvpcbOpen         = false;
    m_undefinedComponentCnt = 0;
    m_skipComponentSelect   = false;

    /* Name of the document footprint list
     * usually located in share/modules/footprints_doc
     * this is of the responsibility to users to create this file
     * if they want to have a list of footprints
     */
    m_DocModulesFileName = DEFAULT_FOOTPRINTS_LIST_FILENAME;

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_cvpcb_xpm ) );
    SetIcon( icon );

    SetAutoLayout( true );

    LoadSettings();

    if( m_FrameSize.x < FRAME_MIN_SIZE_X )
        m_FrameSize.x = FRAME_MIN_SIZE_X;

    if( m_FrameSize.y < FRAME_MIN_SIZE_Y )
        m_FrameSize.y = FRAME_MIN_SIZE_Y;

    // Set minimal frame width and height
    SetSizeHints( FRAME_MIN_SIZE_X, FRAME_MIN_SIZE_Y, -1, -1, -1, -1 );

    // Framesize and position
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    // create the status bar
    static const int dims[3] = { -1, -1, 250 };

    CreateStatusBar( 3 );
    SetStatusWidths( 3, dims );

    ReCreateMenuBar();
    ReCreateHToolbar();

    // Create list of available modules and components of the schematic
    BuildCmpListBox();
    BuildFOOTPRINTS_LISTBOX();
    BuildLIBRARY_LISTBOX();

    m_auimgr.SetManagedWindow( this );


    EDA_PANEINFO horiz;
    horiz.HorizontalToolbarPane();

    EDA_PANEINFO info;
    info.InfoToolbarPane();


    if( m_mainToolBar )
        m_auimgr.AddPane( m_mainToolBar,
                          wxAuiPaneInfo( horiz ).Name( wxT( "m_mainToolBar" ) ).Top() );

    if( m_ListCmp )
        m_auimgr.AddPane( m_ListCmp,
                          wxAuiPaneInfo( horiz ).Name( wxT( "m_ListCmp" ) ).CentrePane() );

    if( m_LibraryList)
        m_auimgr.AddPane( m_LibraryList,
                          wxAuiPaneInfo( info ).Name( wxT( "m_LibraryList" ) ).
                          Left().BestSize( (int) ( m_FrameSize.x * 0.20 ), m_FrameSize.y ) );

    if( m_FootprintList )
        m_auimgr.AddPane( m_FootprintList,
                          wxAuiPaneInfo( info ).Name( wxT( "m_FootprintList" ) ).
                          Right().BestSize( (int) ( m_FrameSize.x * 0.30 ), m_FrameSize.y ) );

    m_auimgr.Update();
}


CVPCB_MAINFRAME::~CVPCB_MAINFRAME()
{
    m_auimgr.UnInit();
}


void CVPCB_MAINFRAME::LoadSettings()
{
    wxASSERT( wxGetApp().GetSettings() != NULL );

    wxConfig* cfg = wxGetApp().GetSettings();

    EDA_BASE_FRAME::LoadSettings();
    cfg->Read( KeepCvpcbOpenEntry, &m_KeepCvpcbOpen, true );
    cfg->Read( FootprintDocFileEntry, &m_DocModulesFileName,
               DEFAULT_FOOTPRINTS_LIST_FILENAME );
}


void CVPCB_MAINFRAME::SaveSettings()
{
    wxASSERT( wxGetApp().GetSettings() != NULL );

    wxConfig* cfg = wxGetApp().GetSettings();

    EDA_BASE_FRAME::SaveSettings();
    cfg->Write( KeepCvpcbOpenEntry, m_KeepCvpcbOpen );
    cfg->Write( FootprintDocFileEntry, m_DocModulesFileName );

    int state = 0;

    if( m_mainToolBar->GetToolToggled( ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST ) )
        state |= FOOTPRINTS_LISTBOX::BY_COMPONENT;

    if( m_mainToolBar->GetToolToggled( ID_CVPCB_FOOTPRINT_DISPLAY_PIN_FILTERED_LIST ) )
        state |= FOOTPRINTS_LISTBOX::BY_PIN_COUNT;

    if( m_mainToolBar->GetToolToggled( ID_CVPCB_FOOTPRINT_DISPLAY_BY_LIBRARY_LIST ) )
        state |= FOOTPRINTS_LISTBOX::BY_LIBRARY;

    cfg->Write( wxT( FILTERFOOTPRINTKEY ), state );
}


void CVPCB_MAINFRAME::OnSize( wxSizeEvent& event )
{
    event.Skip();
}


void CVPCB_MAINFRAME::OnQuit( wxCommandEvent& event )
{
    Close( true );
}


void CVPCB_MAINFRAME::OnCloseWindow( wxCloseEvent& Event )
{
    int diag;

    if( m_modified )
    {
        wxString msg = _( "Component to Footprint links modified.\nSave before exit ?" );
        int ii = DisplayExitDialog( this, msg );

        switch( ii )
        {
        case wxID_CANCEL:
            Event.Veto();
            return;

        case wxID_NO:
            break;

        case wxID_OK:
        case wxID_YES:
            diag = SaveCmpLinkFile( m_NetlistFileName.GetFullPath() );

            if( diag > 0 )
            {
                m_modified = false;
            }
            else if( diag == 0 )
            {
                if( !IsOK( this, _( "Problem when saving file, exit anyway ?" ) ) )
                {
                    Event.Veto();
                    return;
                }
            }
            break;
        }
    }

    // Close the help frame
    if( wxGetApp().GetHtmlHelpController() )
    {
        if( wxGetApp().GetHtmlHelpController()->GetFrame() )// returns NULL if no help frame active
            wxGetApp().GetHtmlHelpController()->GetFrame()->Close( true );
    }

    if( m_NetlistFileName.IsOk() )
    {
        UpdateFileHistory( m_NetlistFileName.GetFullPath() );
    }

    // Close module display frame
    if( m_DisplayFootprintFrame )
        m_DisplayFootprintFrame->Close( true );

    m_modified = false;
    SaveSettings();
    Destroy();
    return;
}


void CVPCB_MAINFRAME::ChangeFocus( bool aMoveRight )
{
    wxWindow* hasFocus = wxWindow::FindFocus();

    if( aMoveRight )
    {
        if( hasFocus == m_LibraryList )
            m_ListCmp->SetFocus();
        else if( hasFocus == m_ListCmp )
            m_FootprintList->SetFocus();
        else if( hasFocus == m_FootprintList )
            m_LibraryList->SetFocus();
    }
    else
    {
        if( hasFocus == m_LibraryList )
            m_FootprintList->SetFocus();
        else if( hasFocus == m_ListCmp )
            m_LibraryList->SetFocus();
        else if( hasFocus == m_FootprintList )
            m_ListCmp->SetFocus();
    }
}


void CVPCB_MAINFRAME::ToFirstNA( wxCommandEvent& event )
{
    int ii = 0;
    int selection;

    if( m_netlist.IsEmpty() )
        return;

    selection = m_ListCmp->GetSelection();

    if( selection < 0 )
        selection = 0;

    for( unsigned jj = 0;  jj < m_netlist.GetCount();  jj++ )
    {
        if( m_netlist.GetComponent( jj )->GetFootprintName().IsEmpty() && ii > selection )
        {
            m_ListCmp->SetSelection( ii );
            SendMessageToEESCHEMA();
            return;
        }

        ii++;
    }

    m_ListCmp->SetSelection( selection );
}


void CVPCB_MAINFRAME::ToPreviousNA( wxCommandEvent& event )
{
    int ii;
    int selection;

    if( m_netlist.IsEmpty() )
        return;

    ii = m_ListCmp->GetCount() - 1;
    selection = m_ListCmp->GetSelection();

    if( selection < 0 )
        selection = m_ListCmp->GetCount() - 1;

    for( unsigned kk = m_netlist.GetCount() - 1;  kk >= 0;  kk-- )
    {
        if( m_netlist.GetComponent( kk )->GetFootprintName().IsEmpty() && ii < selection )
        {
            m_ListCmp->SetSelection( ii );
            SendMessageToEESCHEMA();
            return;
        }

        ii--;
    }

    m_ListCmp->SetSelection( selection );
}


void CVPCB_MAINFRAME::SaveQuitCvpcb( wxCommandEvent& aEvent )
{
    if( aEvent.GetId() == wxID_SAVEAS )
        m_NetlistFileName.Clear();

    if( SaveCmpLinkFile( m_NetlistFileName.GetFullPath() ) > 0 )
    {
        m_modified = false;

        if( !m_KeepCvpcbOpen )
            Close( true );
    }
}


void CVPCB_MAINFRAME::DelAssociations( wxCommandEvent& event )
{
    wxString Line;

    if( IsOK( this, _( "Delete selections" ) ) )
    {
        m_skipComponentSelect = true;
        m_ListCmp->SetSelection( 0 );

        for( unsigned i = 0;  i < m_netlist.GetCount();  i++ )
        {
            m_netlist.GetComponent( i )->SetFootprintName( wxEmptyString );
            SetNewPkg( wxEmptyString );
        }

        m_skipComponentSelect = false;
        m_ListCmp->SetSelection( 0 );
        m_undefinedComponentCnt = m_netlist.GetCount();
    }

    DisplayStatus();
}


void CVPCB_MAINFRAME::LoadNetList( wxCommandEvent& event )
{
    wxString   oldPath;
    wxFileName newFileName;
    int        id = event.GetId();

    if( id >= wxID_FILE1 && id <= wxID_FILE9 )
    {
        newFileName = GetFileFromHistory( id, _( "Netlist" ) );
    }
    else
    {
        wxFileDialog dlg( this, _( "Open Net List" ), wxGetCwd(),
                          wxEmptyString, NetlistFileWildcard,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        newFileName = dlg.GetPath();
    }

    if( newFileName == m_NetlistFileName )
        return;

    if( m_NetlistFileName.DirExists() )
        oldPath = m_NetlistFileName.GetPath();

    /* Update the library search path list. */
    if( wxGetApp().GetLibraryPathList().Index( oldPath ) != wxNOT_FOUND )
        wxGetApp().GetLibraryPathList().Remove( oldPath );

    wxGetApp().GetLibraryPathList().Insert( newFileName.GetPath(), 0 );
    m_NetlistFileName = newFileName;
    ReadNetListAndLinkFiles();
}


void CVPCB_MAINFRAME::ConfigCvpcb( wxCommandEvent& event )
{
    DIALOG_CVPCB_CONFIG ConfigFrame( this );

    ConfigFrame.ShowModal();
}


void CVPCB_MAINFRAME::OnKeepOpenOnSave( wxCommandEvent& event )
{
    m_KeepCvpcbOpen = event.IsChecked();
}


void CVPCB_MAINFRAME::DisplayModule( wxCommandEvent& event )
{
    CreateScreenCmp();
    m_DisplayFootprintFrame->RedrawScreen( wxPoint( 0, 0 ), false );
}


void CVPCB_MAINFRAME::SetLanguage( wxCommandEvent& event )
{
    EDA_BASE_FRAME::SetLanguage( event );
}


void CVPCB_MAINFRAME::DisplayDocFile( wxCommandEvent& event )
{
    GetAssociatedDocument( this, m_DocModulesFileName, &wxGetApp().GetLibraryPathList() );
}


void CVPCB_MAINFRAME::OnSelectComponent( wxListEvent& event )
{
    if( m_skipComponentSelect )
        return;

    wxString   libraryName;
    COMPONENT* component = NULL;
    int        filter = FOOTPRINTS_LISTBOX::UNFILTERED;

    if( m_mainToolBar->GetToolToggled( ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST ) )
        filter |= FOOTPRINTS_LISTBOX::BY_COMPONENT;

    if( m_mainToolBar->GetToolToggled( ID_CVPCB_FOOTPRINT_DISPLAY_PIN_FILTERED_LIST ) )
        filter |= FOOTPRINTS_LISTBOX::BY_PIN_COUNT;

    if( m_mainToolBar->GetToolToggled( ID_CVPCB_FOOTPRINT_DISPLAY_BY_LIBRARY_LIST ) )
        filter |= FOOTPRINTS_LISTBOX::BY_LIBRARY;

    component = GetSelectedComponent();
    libraryName = m_LibraryList->GetSelectedLibrary();
    m_FootprintList->SetFootprints( m_footprints, libraryName, component, filter );

    if( component == NULL )
        return;

    // Preview of the already assigned footprint.
    // Find the footprint that was already chosen for this component and select it,
    // but only if the selection is made from the component list or the library list.
    // If the selection is made from the footprint list, do not change the current
    // selected footprint.
    if( FindFocus() == m_ListCmp || FindFocus() == m_LibraryList )
    {
        wxString module = component->GetFootprintName();

        bool found = false;

        for( int ii = 0; ii < m_FootprintList->GetCount(); ii++ )
        {
            wxString footprintName;
            wxString msg = m_FootprintList->OnGetItemText( ii, 0 );
            msg.Trim( true );
            msg.Trim( false );
            footprintName = msg.AfterFirst( wxChar( ' ' ) );

            if( module.Cmp( footprintName ) == 0 )
            {
                m_FootprintList->SetSelection( ii, true );
                found = true;
                break;
            }
        }

        if( !found )
        {
            int ii = m_FootprintList->GetSelection();

            if ( ii >= 0 )
                m_FootprintList->SetSelection( ii, false );

            if( m_DisplayFootprintFrame )
            {
                CreateScreenCmp();
            }
        }
    }

    SendMessageToEESCHEMA();
    DisplayStatus();
}


void CVPCB_MAINFRAME::OnSelectFilteringFootprint( wxCommandEvent& event )
{
    wxListEvent l_event;

    OnSelectComponent( l_event );
}


void CVPCB_MAINFRAME::OnUpdateKeepOpenOnSave( wxUpdateUIEvent& event )
{
    event.Check( m_KeepCvpcbOpen );
}


void CVPCB_MAINFRAME::DisplayStatus()
{
    wxString   msg;
    COMPONENT* component;

    if( wxWindow::FindFocus() == m_ListCmp || wxWindow::FindFocus() == m_LibraryList )
    {
        msg.Printf( _( "Components: %d, unassigned: %d" ), (int) m_netlist.GetCount(),
                    m_undefinedComponentCnt );
        SetStatusText( msg, 0 );

        msg.Empty();

        component = GetSelectedComponent();

        if( component )
        {
            for( unsigned ii = 0;  ii < component->GetFootprintFilters().GetCount();  ii++ )
            {
                if( msg.IsEmpty() )
                    msg += component->GetFootprintFilters()[ii];
                else
                    msg += wxT( ", " ) + component->GetFootprintFilters()[ii];
            }

            msg = _( "Filter list: " ) + msg;
        }

        SetStatusText( msg, 1 );
    }
    else
    {
        wxString        footprintName = m_FootprintList->GetSelectedFootprint();
        FOOTPRINT_INFO* module = m_footprints.GetModuleInfo( footprintName );
        wxASSERT( module );

        if( module )
        {
            msg = _( "Description: " ) + module->m_Doc;
            SetStatusText( msg, 0 );

            msg  = _( "Key words: " ) + module->m_KeyWord;
            SetStatusText( msg, 1 );
        }
    }


    msg.Empty();

    if( m_FootprintList )
    {
        if( m_mainToolBar->GetToolToggled( ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST ) )
            msg = _( "key words" );

        if( m_mainToolBar->GetToolToggled( ID_CVPCB_FOOTPRINT_DISPLAY_PIN_FILTERED_LIST ) )
        {
            if( !msg.IsEmpty() )
                msg += wxT( ", " );

            msg += _( "pin count" );
        }

        if( m_mainToolBar->GetToolToggled( ID_CVPCB_FOOTPRINT_DISPLAY_BY_LIBRARY_LIST ) )
        {
            if( !msg.IsEmpty() )
                msg += wxT( ", " );

            msg += _( "library" );
        }

        if( msg.IsEmpty() )
            msg = _( "No filtering" );
        else
            msg = _( "Filtered by " ) + msg;

        msg << wxT( ": " ) << m_FootprintList->GetCount();

        SetStatusText( msg, 2 );
    }
}


bool CVPCB_MAINFRAME::LoadFootprintFiles()
{
    // Check if there are footprint libraries in project file.
    if( m_ModuleLibNames.GetCount() == 0 )
    {
        wxMessageBox( _( "No PCB footprint libraries are listed in the current project file." ),
                      _( "Project File Error" ), wxOK | wxICON_ERROR );
        return false;
    }

    m_footprints.ReadFootprintFiles( m_ModuleLibNames );

    // Display error messages, if any.
    if( !m_footprints.m_filesNotFound.IsEmpty() || !m_footprints.m_filesInvalid.IsEmpty() )
    {
        HTML_MESSAGE_BOX dialog( this, _( "Load Error" ) );

        if( !m_footprints.m_filesNotFound.IsEmpty() )
        {
            wxString message = _( "Some files could not be found!" );
            dialog.MessageSet( message );
            dialog.ListSet( m_footprints.m_filesNotFound );
        }

        // Display if there are invalid files.
        if( !m_footprints.m_filesInvalid.IsEmpty() )
        {
            dialog.MessageSet( _( "Some files are invalid!" ) );
            dialog.ListSet( m_footprints.m_filesInvalid );
        }

        dialog.ShowModal();
    }

    return true;
}


void CVPCB_MAINFRAME::UpdateTitle()
{
    wxString title;

    if( m_NetlistFileName.IsOk() && m_NetlistFileName.FileExists() )
    {
        title = wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion() +
                wxT( " " ) + m_NetlistFileName.GetFullPath();

        if( !m_NetlistFileName.IsFileWritable() )
            title += _( " [Read Only]" );
    }
    else
    {
        title = wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion() +
                wxT( " " ) + _( " [no file]" );
    }

    SetTitle( title );
}


void CVPCB_MAINFRAME::SendMessageToEESCHEMA()
{
    char          cmd[1024];
    int           selection;
    COMPONENT*    Component;

    if( m_netlist.IsEmpty() )
        return;

    selection = m_ListCmp->GetSelection();

    if ( selection < 0 )
        selection = 0;

    if( m_netlist.GetComponent( selection ) == NULL )
        return;

    Component = m_netlist.GetComponent( selection );

    sprintf( cmd, "$PART: \"%s\"", TO_UTF8( Component->GetReference() ) );

    SendCommand( MSG_TO_SCH, cmd );

}


int CVPCB_MAINFRAME::ReadSchematicNetlist()
{
    wxBusyCursor    dummy;           // Shows an hourglass while loading.
    NETLIST_READER* netlistReader;
    wxString        msg;
    wxString        compFootprintLinkFileName;
    wxFileName      fn = m_NetlistFileName;

    // Load the footprint association file if it has already been created.
    fn.SetExt( ComponentFileExtension );

    if( fn.FileExists() && fn.IsFileReadable() )
        compFootprintLinkFileName = fn.GetFullPath();

    m_netlist.Clear();

    try
    {
        netlistReader = NETLIST_READER::GetNetlistReader( &m_netlist,
                                                          m_NetlistFileName.GetFullPath(),
                                                          compFootprintLinkFileName );
        if( netlistReader != NULL )
        {
            std::auto_ptr< NETLIST_READER > nlr( netlistReader );
            netlistReader->LoadNetlist();
        }
        else
            wxMessageBox( _( "Unknown netlist format" ), wxEmptyString, wxOK | wxICON_ERROR );
    }
    catch( IO_ERROR& ioe )
    {
        msg = wxString::Format( _( "Error loading netlist.\n%s" ), ioe.errorText.GetData() );
        wxMessageBox( msg, _( "Netlist Load Error" ), wxOK | wxICON_ERROR );
        return 1;
    }

    // We also remove footprint name if it is "$noname" because this is a dummy name,
    // not the actual name of the footprint.
    for( unsigned ii = 0; ii < m_netlist.GetCount(); ii++ )
    {
        if( m_netlist.GetComponent( ii )->GetFootprintName() == wxT( "$noname" ) )
            m_netlist.GetComponent( ii )->SetFootprintName( wxEmptyString );
    }

    // Sort components by reference:
    m_netlist.SortByReference();

    return 0;
}


// File header.
static char HeaderLinkFile[] = { "Cmp-Mod V01" };


bool CVPCB_MAINFRAME::WriteComponentLinkFile( const wxString& aFullFileName )
{
    COMPONENT*  component;
    FILE*       outputFile;
    wxFileName  fn( aFullFileName );
    wxString    Title = wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion();

    outputFile = wxFopen( fn.GetFullPath(), wxT( "wt" ) );

    if( outputFile == NULL )
        return false;

    int retval = 0;

    /*
     * The header is:
     * Cmp-Mod V01 Created by CvPcb (2012-02-08 BZR 3403)-testing date = 10/02/2012 20:45:59
     * and write block per component like:
     * BeginCmp
     * TimeStamp = /322D3011;
     * Reference = BUS1;
     * ValeurCmp = BUSPC;
     * IdModule  = BUS_PC;
     * EndCmp
     */
    retval |= fprintf( outputFile, "%s", HeaderLinkFile );
    retval |= fprintf( outputFile, " Created by %s", TO_UTF8( Title ) );
    retval |= fprintf( outputFile, " date = %s\n", TO_UTF8( DateAndTime() ) );

    for( unsigned i = 0;  i < m_netlist.GetCount();  i++ )
    {
        component = m_netlist.GetComponent( i );
        retval |= fprintf( outputFile, "\nBeginCmp\n" );
        retval |= fprintf( outputFile, "TimeStamp = %s;\n", TO_UTF8( component->GetTimeStamp() ) );
        retval |= fprintf( outputFile, "Reference = %s;\n", TO_UTF8( component->GetReference() ) );
        retval |= fprintf( outputFile, "ValeurCmp = %s;\n", TO_UTF8( component->GetValue() ) );
        retval |= fprintf( outputFile, "IdModule  = %s;\n",
                           TO_UTF8( component->GetFootprintName() ) );
        retval |= fprintf( outputFile, "EndCmp\n" );
    }

    retval |= fprintf( outputFile, "\nEndListe\n" );
    fclose( outputFile );
    return retval >= 0;
}


void CVPCB_MAINFRAME::CreateScreenCmp()
{
    if( m_DisplayFootprintFrame == NULL )
    {
        m_DisplayFootprintFrame = new DISPLAY_FOOTPRINTS_FRAME( this, _( "Module" ),
                                                                wxPoint( 0, 0 ),
                                                                wxSize( 600, 400 ),
                                                                KICAD_DEFAULT_DRAWFRAME_STYLE );
        m_DisplayFootprintFrame->Show( true );
    }
    else
    {
        if( m_DisplayFootprintFrame->IsIconized() )
             m_DisplayFootprintFrame->Iconize( false );
    }

    m_DisplayFootprintFrame->InitDisplay();
}


void CVPCB_MAINFRAME::BuildFOOTPRINTS_LISTBOX()
{
    wxFont   guiFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );

    if( m_FootprintList == NULL )
    {
        m_FootprintList = new FOOTPRINTS_LISTBOX( this, ID_CVPCB_FOOTPRINT_LIST,
                                                  wxDefaultPosition, wxDefaultSize );
        m_FootprintList->SetFont( wxFont( guiFont.GetPointSize(),
                                          wxFONTFAMILY_MODERN,
                                          wxFONTSTYLE_NORMAL,
                                          wxFONTWEIGHT_NORMAL ) );
    }

    m_FootprintList->SetFootprints( m_footprints, wxEmptyString, NULL,
                                    FOOTPRINTS_LISTBOX::UNFILTERED );
    DisplayStatus();
}


void CVPCB_MAINFRAME::BuildCmpListBox()
{
    wxString    msg;
    COMPONENT*  component;
    wxFont      guiFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );

    if( m_ListCmp == NULL )
    {
        m_ListCmp = new COMPONENTS_LISTBOX( this, ID_CVPCB_COMPONENT_LIST,
                                            wxDefaultPosition, wxDefaultSize );
        m_ListCmp->SetFont( wxFont( guiFont.GetPointSize(),
                                    wxFONTFAMILY_MODERN,
                                    wxFONTSTYLE_NORMAL,
                                    wxFONTWEIGHT_NORMAL ) );
    }

    m_ListCmp->m_ComponentList.Clear();

    for( unsigned i = 0;  i < m_netlist.GetCount();  i++ )
    {
        component = m_netlist.GetComponent( i );

        msg.Printf( CMP_FORMAT, m_ListCmp->GetCount() + 1,
                    GetChars( component->GetReference() ),
                    GetChars( component->GetValue() ),
                    GetChars( component->GetFootprintName() ) );
        m_ListCmp->m_ComponentList.Add( msg );
    }

    m_ListCmp->SetItemCount( m_ListCmp->m_ComponentList.Count() );
    m_ListCmp->SetSelection( 0, true );
}


void CVPCB_MAINFRAME::BuildLIBRARY_LISTBOX()
{
    wxFont   guiFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );

    if( m_LibraryList == NULL )
    {
        m_LibraryList = new LIBRARY_LISTBOX( this, ID_CVPCB_LIBRARY_LIST,
                                             wxDefaultPosition, wxDefaultSize );
        m_LibraryList->SetFont( wxFont( guiFont.GetPointSize(),
                                        wxFONTFAMILY_MODERN,
                                        wxFONTSTYLE_NORMAL,
                                        wxFONTWEIGHT_NORMAL ) );
    }

    m_LibraryList->SetLibraryList( m_ModuleLibNames );
}


COMPONENT* CVPCB_MAINFRAME::GetSelectedComponent()
{
    int selection = m_ListCmp->GetSelection();

    if( selection >= 0 && selection < (int) m_netlist.GetCount() )
        return m_netlist.GetComponent( selection );

    return NULL;
}
