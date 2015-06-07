/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jean-pierre.charras
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
#include <build_version.h>
#include <kiway_express.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <macros.h>
#include <confirm.h>
#include <eda_doc.h>
#include <eda_dde.h>
#include <gestfich.h>
#include <html_messagebox.h>
#include <wildcards_and_files_ext.h>
#include <fp_lib_table.h>
#include <netlist_reader.h>

#include <cvpcb_mainframe.h>
#include <cvpcb.h>
#include <cvstruct.h>
#include <invoke_pcb_dialog.h>
#include <class_DisplayFootprintsFrame.h>
#include <cvpcb_id.h>


#define FRAME_MIN_SIZE_X 450
#define FRAME_MIN_SIZE_Y 300


// option key to close CvPcb after saving files
static const wxString KeepCvpcbOpenEntry( wxT( "KeepCvpcbOpen" ) );
static const wxString FootprintDocFileEntry( wxT( "footprints_doc_file" ) );


BEGIN_EVENT_TABLE( CVPCB_MAINFRAME, KIWAY_PLAYER )

    // Menu events
    EVT_MENU( wxID_SAVE, CVPCB_MAINFRAME::SaveQuitCvpcb )
    EVT_MENU( wxID_EXIT, CVPCB_MAINFRAME::OnQuit )
    EVT_MENU( wxID_HELP, CVPCB_MAINFRAME::GetKicadHelp )
    EVT_MENU( wxID_ABOUT, CVPCB_MAINFRAME::GetKicadAbout )
    EVT_MENU( ID_SAVE_PROJECT, CVPCB_MAINFRAME::SaveProjectFile )
    EVT_MENU( ID_PREFERENCES_CONFIGURE_PATHS, CVPCB_MAINFRAME::OnConfigurePaths )
    EVT_MENU( ID_CVPCB_CONFIG_KEEP_OPEN_ON_SAVE, CVPCB_MAINFRAME::OnKeepOpenOnSave )
    EVT_MENU( ID_CVPCB_EQUFILES_LIST_EDIT, CVPCB_MAINFRAME::OnEditEquFilesList )

    // Toolbar events
    EVT_TOOL( ID_CVPCB_QUIT, CVPCB_MAINFRAME::OnQuit )

    EVT_TOOL( ID_CVPCB_LIB_TABLE_EDIT, CVPCB_MAINFRAME::OnEditFootprintLibraryTable )
    EVT_TOOL( ID_CVPCB_CREATE_SCREENCMP, CVPCB_MAINFRAME::DisplayModule )
    EVT_TOOL( ID_CVPCB_GOTO_FIRSTNA, CVPCB_MAINFRAME::ToFirstNA )
    EVT_TOOL( ID_CVPCB_GOTO_PREVIOUSNA, CVPCB_MAINFRAME::ToPreviousNA )
    EVT_TOOL( ID_CVPCB_DEL_ASSOCIATIONS, CVPCB_MAINFRAME::DelAssociations )
    EVT_TOOL( ID_CVPCB_AUTO_ASSOCIE, CVPCB_MAINFRAME::AutomaticFootprintMatching )
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


CVPCB_MAINFRAME::CVPCB_MAINFRAME( KIWAY* aKiway, wxWindow* aParent ) :
    KIWAY_PLAYER( aKiway, aParent, FRAME_CVPCB, wxT( "CvPCB" ), wxDefaultPosition,
        wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, CVPCB_MAINFRAME_NAME )
{
    m_compListBox           = NULL;
    m_footprintListBox      = NULL;
    m_libListBox            = NULL;
    m_mainToolBar           = NULL;
    m_modified              = false;
    m_isEESchemaNetlist     = false;
    m_KeepCvpcbOpen         = false;
    m_undefinedComponentCnt = 0;
    m_skipComponentSelect   = false;
    m_NetlistFileExtension  = wxT( "net" );

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

    LoadSettings( config() );

    if( m_FrameSize.x < FRAME_MIN_SIZE_X )
        m_FrameSize.x = FRAME_MIN_SIZE_X;

    if( m_FrameSize.y < FRAME_MIN_SIZE_Y )
        m_FrameSize.y = FRAME_MIN_SIZE_Y;

    // Set minimal frame width and height
    SetSizeHints( FRAME_MIN_SIZE_X, FRAME_MIN_SIZE_Y, -1, -1, -1, -1 );

    // Frame size and position
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

    UpdateTitle();

    EDA_PANEINFO horiz;
    horiz.HorizontalToolbarPane();

    EDA_PANEINFO info;
    info.InfoToolbarPane();


    if( m_mainToolBar )
        m_auimgr.AddPane( m_mainToolBar,
                          wxAuiPaneInfo( horiz ).Name( wxT( "m_mainToolBar" ) ).Top() );

    if( m_compListBox )
        m_auimgr.AddPane( m_compListBox,
                          wxAuiPaneInfo( horiz ).Name( wxT( "m_compListBox" ) ).CentrePane() );

    if( m_libListBox)
        m_auimgr.AddPane( m_libListBox,
                          wxAuiPaneInfo( info ).Name( wxT( "m_libListBox" ) ).
                          Left().BestSize( (int) ( m_FrameSize.x * 0.20 ), m_FrameSize.y ) );

    if( m_footprintListBox )
        m_auimgr.AddPane( m_footprintListBox,
                          wxAuiPaneInfo( info ).Name( wxT( "m_footprintListBox" ) ).
                          Right().BestSize( (int) ( m_FrameSize.x * 0.30 ), m_FrameSize.y ) );

    m_auimgr.Update();
}


CVPCB_MAINFRAME::~CVPCB_MAINFRAME()
{
    m_auimgr.UnInit();
}


void CVPCB_MAINFRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    aCfg->Read( KeepCvpcbOpenEntry, &m_KeepCvpcbOpen, true );
    aCfg->Read( FootprintDocFileEntry, &m_DocModulesFileName,
                DEFAULT_FOOTPRINTS_LIST_FILENAME );
}


void CVPCB_MAINFRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );

    aCfg->Write( KeepCvpcbOpenEntry, m_KeepCvpcbOpen );
    aCfg->Write( FootprintDocFileEntry, m_DocModulesFileName );

    int state = 0;

    if( m_mainToolBar->GetToolToggled( ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST ) )
        state |= FOOTPRINTS_LISTBOX::BY_COMPONENT;

    if( m_mainToolBar->GetToolToggled( ID_CVPCB_FOOTPRINT_DISPLAY_PIN_FILTERED_LIST ) )
        state |= FOOTPRINTS_LISTBOX::BY_PIN_COUNT;

    if( m_mainToolBar->GetToolToggled( ID_CVPCB_FOOTPRINT_DISPLAY_BY_LIBRARY_LIST ) )
        state |= FOOTPRINTS_LISTBOX::BY_LIBRARY;

    aCfg->Write( wxT( FILTERFOOTPRINTKEY ), state );
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

        case wxID_YES:
            SaveEdits();
            break;
        }
    }

    // Close module display frame
    if( GetFpViewerFrame() )
        GetFpViewerFrame()->Close( true );

    m_modified = false;

    Destroy();
    return;
}


void CVPCB_MAINFRAME::ChangeFocus( bool aMoveRight )
{
    wxWindow* hasFocus = wxWindow::FindFocus();

    if( aMoveRight )
    {
        if( hasFocus == m_libListBox )
            m_compListBox->SetFocus();
        else if( hasFocus == m_compListBox )
            m_footprintListBox->SetFocus();
        else if( hasFocus == m_footprintListBox )
            m_libListBox->SetFocus();
    }
    else
    {
        if( hasFocus == m_libListBox )
            m_footprintListBox->SetFocus();
        else if( hasFocus == m_compListBox )
            m_libListBox->SetFocus();
        else if( hasFocus == m_footprintListBox )
            m_compListBox->SetFocus();
    }
}


void CVPCB_MAINFRAME::ToFirstNA( wxCommandEvent& event )
{
    if( m_netlist.IsEmpty() )
        return;

    long selection = m_compListBox->GetFirstSelected();

    if( selection < 0 )
        selection = -1;     // We will start to 0 for the first search , if no item selected

    for( unsigned jj = selection+1; jj < m_netlist.GetCount(); jj++ )
    {
        if( m_netlist.GetComponent( jj )->GetFPID().empty() )
        {
            m_compListBox->SetSelection( wxNOT_FOUND, false );  // Remove all selections
            m_compListBox->SetSelection( jj );
            SendMessageToEESCHEMA();
            return;
        }
    }
}


void CVPCB_MAINFRAME::ToPreviousNA( wxCommandEvent& event )
{
    if( m_netlist.IsEmpty() )
        return;

    int selection = m_compListBox->GetFirstSelected();

    if( selection < 0 )
        selection = m_compListBox->GetCount();
    else
        while( m_compListBox->GetNextSelected( selection ) >= 0 )
            selection =  m_compListBox->GetNextSelected( selection );

    for( int kk = selection-1; kk >= 0; kk-- )
    {
        if( m_netlist.GetComponent( kk )->GetFPID().empty() )
        {
            m_compListBox->SetSelection( wxNOT_FOUND, false );  // Remove all selections
            m_compListBox->SetSelection( kk );
            SendMessageToEESCHEMA();
            return;
        }
    }
}


void CVPCB_MAINFRAME::SaveQuitCvpcb( wxCommandEvent& aEvent )
{
    SaveEdits();

    m_modified = false;

    if( !m_KeepCvpcbOpen )
        Close( true );
}


void CVPCB_MAINFRAME::DelAssociations( wxCommandEvent& event )
{
    wxString Line;

    if( IsOK( this, _( "Delete selections" ) ) )
    {
        m_skipComponentSelect = true;
        m_compListBox->SetSelection( 0 );

        for( unsigned i = 0;  i < m_netlist.GetCount();  i++ )
        {
            FPID fpid;

            m_netlist.GetComponent( i )->SetFPID( fpid );
            SetNewPkg( wxEmptyString );
        }

        m_skipComponentSelect = false;
        m_compListBox->SetSelection( 0 );
        m_undefinedComponentCnt = m_netlist.GetCount();
    }

    DisplayStatus();
}


/* Remove in favor of Kiway messaging method of sending netlist
void CVPCB_MAINFRAME::LoadNetList( wxCommandEvent& event )
{
    int        id = event.GetId();
    wxFileName newFileName;

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

    OpenProjectFiles( std::vector<wxString>( 1, newFileName.GetFullPath() ) );
}
*/


bool CVPCB_MAINFRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    return true;
}


void CVPCB_MAINFRAME::OnEditFootprintLibraryTable( wxCommandEvent& aEvent )
{
    bool    tableChanged = false;
    int     r = InvokePcbLibTableEditor( this, &GFootprintTable, Prj().PcbFootprintLibs() );

    if( r & 1 )
    {
        wxString fileName = FP_LIB_TABLE::GetGlobalTableFileName();

        try
        {
            GFootprintTable.Save( fileName );
            tableChanged = true;
        }
        catch( const IO_ERROR& ioe )
        {
            wxString msg = wxString::Format(
                    _( "Error occurred saving the global footprint library table:\n'%s'\n%s" ),
                    GetChars( fileName ),
                    GetChars( ioe.errorText )
                    );
            wxMessageBox( msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
        }
    }

    if( r & 2 )
    {
        wxString fileName = Prj().FootprintLibTblName();

        try
        {
            Prj().PcbFootprintLibs()->Save( fileName );
            tableChanged = true;
        }
        catch( const IO_ERROR& ioe )
        {
            wxString msg = wxString::Format(
                    _( "Error occurred saving the project footprint library table:\n'%s'\n%s" ),
                    GetChars( fileName ),
                    GetChars( ioe.errorText )
                    );
            wxMessageBox( msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
        }
    }

    if( tableChanged )
    {
        BuildLIBRARY_LISTBOX();
        m_footprints.ReadFootprintFiles( Prj().PcbFootprintLibs() );
    }
}


void CVPCB_MAINFRAME::OnKeepOpenOnSave( wxCommandEvent& event )
{
    m_KeepCvpcbOpen = event.IsChecked();
}


void CVPCB_MAINFRAME::DisplayModule( wxCommandEvent& event )
{
    CreateScreenCmp();
    GetFpViewerFrame()->RedrawScreen( wxPoint( 0, 0 ), false );
}


void CVPCB_MAINFRAME::DisplayDocFile( wxCommandEvent& event )
{
    GetAssociatedDocument( this, m_DocModulesFileName, &Kiface().KifaceSearch() );
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
    libraryName = m_libListBox->GetSelectedLibrary();
    m_footprintListBox->SetFootprints( m_footprints, libraryName, component, filter );

    // Tell AuiMgr that objects are changed !
    if( m_auimgr.GetManagedWindow() )   // Be sure Aui Manager is initialized
                                        // (could be not the case when starting CvPcb
        m_auimgr.Update();

    if( component == NULL )
        return;

    // Preview of the already assigned footprint.
    // Find the footprint that was already chosen for this component and select it,
    // but only if the selection is made from the component list or the library list.
    // If the selection is made from the footprint list, do not change the current
    // selected footprint.
    if( FindFocus() == m_compListBox || FindFocus() == m_libListBox )
    {
        wxString module = FROM_UTF8( component->GetFPID().Format().c_str() );

        bool found = false;

        for( int ii = 0; ii < m_footprintListBox->GetCount(); ii++ )
        {
            wxString footprintName;
            wxString msg = m_footprintListBox->OnGetItemText( ii, 0 );
            msg.Trim( true );
            msg.Trim( false );
            footprintName = msg.AfterFirst( wxChar( ' ' ) );

            if( module.Cmp( footprintName ) == 0 )
            {
                m_footprintListBox->SetSelection( ii, true );
                found = true;
                break;
            }
        }

        if( !found )
        {
            int ii = m_footprintListBox->GetSelection();

            if ( ii >= 0 )
                m_footprintListBox->SetSelection( ii, false );

            if( GetFpViewerFrame() )
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

    if( wxWindow::FindFocus() == m_compListBox || wxWindow::FindFocus() == m_libListBox )
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
        wxString footprintName = m_footprintListBox->GetSelectedFootprint();

        FOOTPRINT_INFO* module = m_footprints.GetModuleInfo( footprintName );

        if( module )    // can be NULL if no netlist loaded
        {
            msg = _( "Description: " ) + module->GetDoc();
            SetStatusText( msg, 0 );

            msg  = _( "Key words: " ) + module->GetKeywords();
            SetStatusText( msg, 1 );
        }
    }

    msg.Empty();
    wxString filters;

    if( m_footprintListBox )
    {
        if( m_mainToolBar->GetToolToggled( ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST ) )
            filters = _( "key words" );

        if( m_mainToolBar->GetToolToggled( ID_CVPCB_FOOTPRINT_DISPLAY_PIN_FILTERED_LIST ) )
        {
            if( !filters.IsEmpty() )
                filters += wxT( "+" );

            filters += _( "pin count" );
        }

        if( m_mainToolBar->GetToolToggled( ID_CVPCB_FOOTPRINT_DISPLAY_BY_LIBRARY_LIST ) )
        {
            if( !filters.IsEmpty() )
                filters += wxT( "+" );

            filters += _( "library" );
        }

        if( filters.IsEmpty() )
            msg = _( "No filtering" );
        else
            msg.Printf( _( "Filtered by %s" ), GetChars( filters ) );

        msg << wxT( ": " ) << m_footprintListBox->GetCount();

        SetStatusText( msg, 2 );
    }
}


bool CVPCB_MAINFRAME::LoadFootprintFiles()
{
    FP_LIB_TABLE* fptbl = Prj().PcbFootprintLibs();

    // Check if there are footprint libraries in the footprint library table.
    if( !fptbl || !fptbl->GetLogicalLibs().size() )
    {
        wxMessageBox( _( "No PCB footprint libraries are listed in the current footprint "
                         "library table." ), _( "Configuration Error" ), wxOK | wxICON_ERROR );
        return false;
    }

    m_footprints.ReadFootprintFiles( fptbl );

    if( m_footprints.GetErrorCount() )
    {
        m_footprints.DisplayErrors( this );
    }

    return true;
}


void CVPCB_MAINFRAME::UpdateTitle()
{
    wxString    title = wxString::Format( wxT( "Cvpcb %s  " ), GetChars( GetBuildVersion() ) );
    PROJECT&    prj = Prj();
    wxFileName fn = prj.GetProjectFullName();

    if( fn.IsOk() && !prj.GetProjectFullName().IsEmpty() && fn.FileExists() )
    {
        title += wxString::Format( _("Project: '%s'"),
                                   GetChars( fn.GetFullPath() )
                                 );

        if( !fn.IsFileWritable() )
            title += _( " [Read Only]" );
    }
    else
        title += _( "[no project]" );

    SetTitle( title );
}


void CVPCB_MAINFRAME::SendMessageToEESCHEMA()
{
    if( m_netlist.IsEmpty() )
        return;

    int selection = m_compListBox->GetSelection();

    if ( selection < 0 )
        selection = 0;

    if( m_netlist.GetComponent( selection ) == NULL )
        return;

    COMPONENT* component = m_netlist.GetComponent( selection );

    std::string packet = StrPrintf( "$PART: \"%s\"", TO_UTF8( component->GetReference() ) );

    if( Kiface().IsSingle() )
        SendCommand( MSG_TO_SCH, packet.c_str() );
    else
        Kiway().ExpressMail( FRAME_SCH, MAIL_CROSS_PROBE, packet, this );
}


int CVPCB_MAINFRAME::ReadSchematicNetlist( const std::string& aNetlist )
{
    STRING_LINE_READER*     strrdr = new STRING_LINE_READER( aNetlist, "Eeschema via Kiway" );
    KICAD_NETLIST_READER    netrdr( strrdr, &m_netlist );

    m_netlist.Clear();

    try
    {
        netrdr.LoadNetlist();
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _( "Error loading netlist.\n%s" ), ioe.errorText.GetData() );
        wxMessageBox( msg, _( "Netlist Load Error" ), wxOK | wxICON_ERROR );
        return 1;
    }

    // We also remove footprint name if it is "$noname" because this is a dummy name,
    // not the actual name of the footprint.
    for( unsigned ii = 0; ii < m_netlist.GetCount(); ii++ )
    {
        if( m_netlist.GetComponent( ii )->GetFPID().GetFootprintName() == std::string( "$noname" ) )
            m_netlist.GetComponent( ii )->SetFPID( FPID( wxEmptyString ) );
    }

    // Sort components by reference:
    m_netlist.SortByReference();

    return 0;
}


void CVPCB_MAINFRAME::CreateScreenCmp()
{
    DISPLAY_FOOTPRINTS_FRAME* fpframe = GetFpViewerFrame();

    if( !fpframe )
    {
        fpframe = new DISPLAY_FOOTPRINTS_FRAME( &Kiway(), this );
        fpframe->Show( true );
    }
    else
    {
        if( fpframe->IsIconized() )
             fpframe->Iconize( false );

        // The display footprint window might be buried under some other
        // windows, so CreateScreenCmp() on an existing window would not
        // show any difference, leaving the user confused.
        // So we want to put it to front, second after our CVPCB_MAINFRAME.
        // We do this by a little dance of bringing it to front then the main
        // frame back.
        fpframe->Raise();   // Make sure that is visible.
        Raise();            // .. but still we want the focus.
    }

    fpframe->InitDisplay();
}


void CVPCB_MAINFRAME::BuildFOOTPRINTS_LISTBOX()
{
    wxFont   guiFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );

    if( m_footprintListBox == NULL )
    {
        m_footprintListBox = new FOOTPRINTS_LISTBOX( this, ID_CVPCB_FOOTPRINT_LIST,
                                                     wxDefaultPosition, wxDefaultSize );
        m_footprintListBox->SetFont( wxFont( guiFont.GetPointSize(),
                                             wxFONTFAMILY_MODERN,
                                             wxFONTSTYLE_NORMAL,
                                             wxFONTWEIGHT_NORMAL ) );
    }

    m_footprintListBox->SetFootprints( m_footprints, wxEmptyString, NULL,
                                       FOOTPRINTS_LISTBOX::UNFILTERED );
    DisplayStatus();
}


void CVPCB_MAINFRAME::BuildCmpListBox()
{
    wxString    msg;
    COMPONENT*  component;
    wxFont      guiFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );

    if( m_compListBox == NULL )
    {
        m_compListBox = new COMPONENTS_LISTBOX( this, ID_CVPCB_COMPONENT_LIST,
                                                wxDefaultPosition, wxDefaultSize );
        m_compListBox->SetFont( wxFont( guiFont.GetPointSize(),
                                        wxFONTFAMILY_MODERN,
                                        wxFONTSTYLE_NORMAL,
                                        wxFONTWEIGHT_NORMAL ) );
    }

    m_compListBox->m_ComponentList.Clear();

    for( unsigned i = 0;  i < m_netlist.GetCount();  i++ )
    {
        component = m_netlist.GetComponent( i );

        msg.Printf( CMP_FORMAT, m_compListBox->GetCount() + 1,
                    GetChars( component->GetReference() ),
                    GetChars( component->GetValue() ),
                    GetChars( FROM_UTF8( component->GetFPID().Format().c_str() ) ) );
        m_compListBox->m_ComponentList.Add( msg );
    }

    if( m_compListBox->m_ComponentList.Count() )
    {
        m_compListBox->SetItemCount( m_compListBox->m_ComponentList.Count() );
        m_compListBox->SetSelection( 0, true );
        m_compListBox->RefreshItems( 0L, m_compListBox->m_ComponentList.Count()-1 );
        m_compListBox->UpdateWidth();
    }
}


void CVPCB_MAINFRAME::BuildLIBRARY_LISTBOX()
{
    wxFont   guiFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );

    if( m_libListBox == NULL )
    {
        m_libListBox = new LIBRARY_LISTBOX( this, ID_CVPCB_LIBRARY_LIST,
                                            wxDefaultPosition, wxDefaultSize );
        m_libListBox->SetFont( wxFont( guiFont.GetPointSize(),
                                       wxFONTFAMILY_MODERN,
                                       wxFONTSTYLE_NORMAL,
                                       wxFONTWEIGHT_NORMAL ) );
    }

    FP_LIB_TABLE* tbl = Prj().PcbFootprintLibs();

    if( tbl )
    {
        wxArrayString libNames;

        std::vector< wxString > libNickNames = tbl->GetLogicalLibs();

        for( unsigned ii = 0; ii < libNickNames.size(); ii++ )
            libNames.Add( libNickNames[ii] );

        m_libListBox->SetLibraryList( libNames );
    }
}


COMPONENT* CVPCB_MAINFRAME::GetSelectedComponent()
{
    int selection = m_compListBox->GetSelection();

    if( selection >= 0 && selection < (int) m_netlist.GetCount() )
        return m_netlist.GetComponent( selection );

    return NULL;
}


DISPLAY_FOOTPRINTS_FRAME* CVPCB_MAINFRAME::GetFpViewerFrame()
{
    // returns the Footprint Viewer frame, if exists, or NULL
    return (DISPLAY_FOOTPRINTS_FRAME*) wxWindow::FindWindowByName( FOOTPRINTVIEWER_FRAME_NAME );
}


void CVPCB_MAINFRAME::OnConfigurePaths( wxCommandEvent& aEvent )
{
    Pgm().ConfigurePaths( this );
}


void CVPCB_MAINFRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
{
    const std::string& payload = mail.GetPayload();

    DBG(printf( "%s: %s\n", __func__, payload.c_str() );)

    switch( mail.Command() )
    {
    case MAIL_EESCHEMA_NETLIST:
        ReadNetListAndLinkFiles( payload );
        /* @todo
        Go into SCH_EDIT_FRAME::OnOpenCvpcb( wxCommandEvent& event ) and trim GNL_ALL down.
        */
        break;

    default:
        ;       // ignore most
    }
}
