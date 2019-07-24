/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <build_version.h>
#include <confirm.h>
#include <eda_dde.h>
#include <fctsys.h>
#include <fp_lib_table.h>
#include <html_messagebox.h>
#include <invoke_pcb_dialog.h>
#include <kiface_i.h>
#include <kiface_ids.h>
#include <kiway_express.h>
#include <macros.h>
#include <netlist_reader.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/conditional_menu.h>
#include <tool/tool_manager.h>
#include <widgets/progress_reporter.h>
#include <wx/statline.h>

#include <cvpcb.h>
#include <cvpcb_association.h>
#include <cvpcb_id.h>
#include <cvpcb_mainframe.h>
#include <display_footprints_frame.h>
#include <listboxes.h>
#include <tools/cvpcb_actions.h>
#include <tools/cvpcb_control.h>

wxSize const FRAME_MIN_SIZE_DU( 350, 250 );
wxSize const FRAME_DEFAULT_SIZE_DU( 450, 300 );

///@{
/// \ingroup config

static const wxString FilterFootprintEntry = "FilterFootprint";
///@}

BEGIN_EVENT_TABLE( CVPCB_MAINFRAME, KIWAY_PLAYER )

    // Control events
    EVT_MENU( wxID_EXIT, CVPCB_MAINFRAME::OnQuit )
    EVT_BUTTON( wxID_OK, CVPCB_MAINFRAME::OnOK )
    EVT_BUTTON( wxID_CANCEL, CVPCB_MAINFRAME::OnCancel )

    // Toolbar events
    EVT_TEXT( ID_CVPCB_FILTER_TEXT_EDIT, CVPCB_MAINFRAME::OnEnterFilteringText )

    // Frame events
    EVT_CLOSE( CVPCB_MAINFRAME::OnCloseWindow )
    EVT_SIZE( CVPCB_MAINFRAME::OnSize )

END_EVENT_TABLE()


#define CVPCB_MAINFRAME_NAME wxT( "CvpcbFrame" )


CVPCB_MAINFRAME::CVPCB_MAINFRAME( KIWAY* aKiway, wxWindow* aParent ) :
    KIWAY_PLAYER( aKiway, aParent, FRAME_CVPCB, _( "Assign Footprints" ), wxDefaultPosition,
                  wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, CVPCB_MAINFRAME_NAME )
{
    m_compListBox           = NULL;
    m_footprintListBox      = NULL;
    m_libListBox            = NULL;
    m_mainToolBar           = NULL;
    m_modified              = false;
    m_skipComponentSelect   = false;
    m_filteringOptions      = FOOTPRINTS_LISTBOX::UNFILTERED_FP_LIST;
    m_tcFilterString        = NULL;
    m_FootprintsList        = FOOTPRINT_LIST::GetInstance( Kiway() );
    m_initialized           = false;

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_cvpcb_xpm ) );
    SetIcon( icon );

    SetAutoLayout( true );

    LoadSettings( config() );

    wxSize const frame_min( ConvertDialogToPixels( FRAME_MIN_SIZE_DU ) );

    SetSizeHints( frame_min );

    // Frame size and position
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    setupTools();
    ReCreateMenuBar();
    ReCreateHToolbar();

    // Create list of available modules and components of the schematic
    BuildCmpListBox();
    BuildFOOTPRINTS_LISTBOX();
    BuildLIBRARY_LISTBOX();

    m_auimgr.SetManagedWindow( this );

    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );

    m_auimgr.AddPane( m_libListBox, EDA_PANE().Palette().Name( "Libraries" ).Left().Layer(1)
                      .Caption( _( "Footprint Libraries" ) )
                      .BestSize( (int) ( m_FrameSize.x * 0.20 ), m_FrameSize.y ) );

    m_auimgr.AddPane( m_compListBox, EDA_PANE().Palette().Name( "Components" ).Center().Layer(0)
                      .Caption( _( "Symbol : Footprint Assignments" ) ) );

    m_auimgr.AddPane( m_footprintListBox, EDA_PANE().Palette().Name( "Footprints" ).Right().Layer(1)
                      .Caption( _( "Filtered Footprints" ) )
                      .BestSize( (int) ( m_FrameSize.x * 0.30 ), m_FrameSize.y ) );

    // Build the bottom panel, to display 2 status texts and the buttons:
    auto bottomPanel = new wxPanel( this );
    auto panelSizer = new wxBoxSizer( wxVERTICAL );

    wxFlexGridSizer* fgSizerStatus = new wxFlexGridSizer( 2, 1, 0, 0 );
    fgSizerStatus->SetFlexibleDirection( wxBOTH );
    fgSizerStatus->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    m_statusLine1 = new wxStaticText( bottomPanel, wxID_ANY, wxEmptyString );
    fgSizerStatus->Add( m_statusLine1, 0, 0, 5 );

    m_statusLine2 = new wxStaticText( bottomPanel, wxID_ANY, wxEmptyString );
    fgSizerStatus->Add( m_statusLine2, 0, wxBOTTOM, 3 );

    panelSizer->Add( fgSizerStatus, 1, wxEXPAND|wxLEFT, 2 );

    wxStaticLine* staticline1 = new wxStaticLine( bottomPanel );
    panelSizer->Add( staticline1, 0, wxEXPAND, 5 );

    wxFont statusFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    statusFont.SetSymbolicSize( wxFONTSIZE_SMALL );
    m_statusLine1->SetFont( statusFont );
    m_statusLine2->SetFont( statusFont );

    // Add buttons:
    auto buttonsSizer = new wxBoxSizer( wxHORIZONTAL );
    auto sdbSizer = new wxStdDialogButtonSizer();

    m_saveAndContinue = new wxButton( bottomPanel, ID_SAVE_PROJECT,
                                      _( "Apply, Save Schematic && Continue" ) );
    buttonsSizer->Add( m_saveAndContinue, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 20 );

    auto sdbSizerOK = new wxButton( bottomPanel, wxID_OK );
    sdbSizer->AddButton( sdbSizerOK );
    auto sdbSizerCancel = new wxButton( bottomPanel, wxID_CANCEL );
    sdbSizer->AddButton( sdbSizerCancel );
    sdbSizer->Realize();

    buttonsSizer->Add( sdbSizer, 0, 0, 5 );
    panelSizer->Add( buttonsSizer, 0, wxALIGN_RIGHT|wxALL, 5 );

    bottomPanel->SetSizer( panelSizer );
    bottomPanel->Fit();

    sdbSizerOK->SetDefault();

    m_auimgr.AddPane( bottomPanel, EDA_PANE().HToolbar().Name( "Buttons" ).Bottom().Layer(6) );

    m_auimgr.Update();
    m_initialized = true;

    // Connect Events
    m_footprintListBox->Connect( wxEVT_RIGHT_DOWN,
                                 wxMouseEventHandler( CVPCB_MAINFRAME::OnFootprintRightClick ),
                                 NULL, this );
    m_compListBox->Connect( wxEVT_RIGHT_DOWN,
                            wxMouseEventHandler( CVPCB_MAINFRAME::OnComponentRightClick ),
                            NULL, this );

    // Use Bind for this one to allow the lambda expression
    m_saveAndContinue->Bind( wxEVT_COMMAND_BUTTON_CLICKED,
            [this]( wxCommandEvent& )
            {
                this->GetToolManager()->RunAction( CVPCB_ACTIONS::saveAssociations );
            } );

    // Ensure the toolbars are sync'd properly so the filtering options display correct
    SyncToolbars();
}


CVPCB_MAINFRAME::~CVPCB_MAINFRAME()
{
    // Disconnect Events
    m_footprintListBox->Disconnect( wxEVT_RIGHT_DOWN,
                                    wxMouseEventHandler( CVPCB_MAINFRAME::OnFootprintRightClick ),
                                    NULL, this );

    m_auimgr.UnInit();
}


void CVPCB_MAINFRAME::setupTools()
{
    // Create the manager
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( nullptr, nullptr, nullptr, this );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new CVPCB_CONTROL );
    m_toolManager->InitTools();

    CVPCB_CONTROL* tool = m_toolManager->GetTool<CVPCB_CONTROL>();

    // Create the context menu for the component list box
    m_componentContextMenu = new ACTION_MENU( true );
    m_componentContextMenu->SetTool( tool );
    m_componentContextMenu->Add( CVPCB_ACTIONS::showFootprintViewer );

    // Create the context menu for the footprint list box
    m_footprintContextMenu = new ACTION_MENU( true );
    m_footprintContextMenu->SetTool( tool );
    m_footprintContextMenu->Add( CVPCB_ACTIONS::showFootprintViewer );
}

void CVPCB_MAINFRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    wxSize const frame_default( ConvertDialogToPixels( FRAME_DEFAULT_SIZE_DU ) );

    if( m_FrameSize == wxDefaultSize )
        m_FrameSize = frame_default;

    aCfg->Read( FilterFootprintEntry, &m_filteringOptions, FOOTPRINTS_LISTBOX::UNFILTERED_FP_LIST );
}


void CVPCB_MAINFRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );

    aCfg->Write( FilterFootprintEntry, m_filteringOptions );
}


void CVPCB_MAINFRAME::OnSize( wxSizeEvent& event )
{
    event.Skip();
}


void CVPCB_MAINFRAME::OnCloseWindow( wxCloseEvent& Event )
{
    if( m_modified )
    {
        if( !HandleUnsavedChanges( this, _( "Symbol to Footprint links have been modified. "
                                            "Save changes?" ),
                                   [&]()->bool { return SaveFootprintAssociation( false ); } ) )
        {
            Event.Veto();
            return;
        }
    }

    // Close module display frame
    if( GetFootprintViewerFrame() )
        GetFootprintViewerFrame()->Close( true );

    m_modified = false;

    // clear highlight symbol in schematic:
    SendMessageToEESCHEMA( true );

    // Delete window
    Destroy();
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


void CVPCB_MAINFRAME::ToNextNA()
{
    if( m_netlist.IsEmpty() )
        return;

    int first_selected = m_compListBox->GetFirstSelected();

    if( first_selected < 0 )
        first_selected = -1;     // We will start to 0 for the first search , if no item selected

    int candidate = -1;

    for( int jj = first_selected+1; jj < (int)m_netlist.GetCount(); jj++ )
    {
        if( m_netlist.GetComponent( jj )->GetFPID().empty() )
        {
            candidate = jj;
            break;
        }
    }

    if( candidate >= 0 )
    {
        m_compListBox->DeselectAll();
        m_compListBox->SetSelection( candidate );
        SendMessageToEESCHEMA();
    }
}


void CVPCB_MAINFRAME::ToPreviousNA()
{
    if( m_netlist.IsEmpty() )
        return;

    int first_selected = m_compListBox->GetFirstSelected();

    if( first_selected < 0 )
        first_selected = m_compListBox->GetCount();

    int candidate = -1;

    for( int jj = first_selected-1; jj >= 0; jj-- )
    {
        if( m_netlist.GetComponent( jj )->GetFPID().empty() )
        {
            candidate = jj;
            break;
        }
    }

    if( candidate >= 0 )
    {
        m_compListBox->DeselectAll();
        m_compListBox->SetSelection( candidate );
        SendMessageToEESCHEMA();
    }
}


void CVPCB_MAINFRAME::OnOK( wxCommandEvent& aEvent )
{
    SaveFootprintAssociation( false );

    Close( true );
}


void CVPCB_MAINFRAME::OnCancel( wxCommandEvent& event )
{
    // Throw away modifications on a Cancel
    m_modified = false;

    Close( false );
}


void CVPCB_MAINFRAME::OnQuit( wxCommandEvent& event )
{
    Close( false );
}


void CVPCB_MAINFRAME::AssociateFootprint( const CVPCB_ASSOCIATION& aAssociation )
{
    // Ensure there is data to work with
    COMPONENT* component;

    if( m_netlist.IsEmpty() )
        return;

    component = m_netlist.GetComponent( aAssociation.GetComponentIndex() );

    if( component == NULL )
        return;

    LIB_ID fpid = aAssociation.GetNewFootprint();

    // Test for validity of the requested footprint
    if( !fpid.empty() && !fpid.IsValid() )
    {
        wxString msg =
                wxString::Format( _( "\"%s\" is not a valid footprint." ), fpid.Format().wx_str() );
        DisplayErrorMessage( this, msg );
    }

    // Set the new footprint
    component->SetFPID( fpid );

    // create the new component description and set it
    wxString description = wxString::Format( CMP_FORMAT, aAssociation.GetComponentIndex() + 1,
            GetChars( component->GetReference() ), GetChars( component->GetValue() ),
            GetChars( FROM_UTF8( component->GetFPID().Format().c_str() ) ) );
    m_compListBox->SetString( aAssociation.GetComponentIndex(), description );

    // Mark the data as being modified
    m_modified = true;

    // Update the statusbar and refresh the list
    DisplayStatus();
    m_compListBox->Refresh();
}


void CVPCB_MAINFRAME::DeleteAll()
{
    if( IsOK( this, _( "Delete all associations?" ) ) )
    {
        m_skipComponentSelect = true;

        // Remove all selections to avoid issues when setting the fpids
        m_compListBox->DeselectAll();

        for( unsigned i = 0;  i < m_netlist.GetCount();  i++ )
        {
            AssociateFootprint( CVPCB_ASSOCIATION( i, LIB_ID() ) );
        }

        // Remove all selections after setting the fpids
        m_compListBox->DeselectAll();

        m_skipComponentSelect = false;
        m_compListBox->SetSelection( 0 );
    }

    DisplayStatus();
}


bool CVPCB_MAINFRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    return true;
}


void CVPCB_MAINFRAME::OnComponentRightClick( wxMouseEvent& event )
{
    PopupMenu( m_componentContextMenu );
}


void CVPCB_MAINFRAME::OnFootprintRightClick( wxMouseEvent& event )
{
    PopupMenu( m_footprintContextMenu );
}


void CVPCB_MAINFRAME::OnSelectComponent( wxListEvent& event )
{
    if( m_skipComponentSelect )
        return;

    wxString   libraryName;
    COMPONENT* component = GetSelectedComponent();
    libraryName = m_libListBox->GetSelectedLibrary();

    m_footprintListBox->SetFootprints( *m_FootprintsList, libraryName, component,
                                       m_currentSearchPattern, m_filteringOptions);

    if( component && component->GetFPID().IsValid() )
        m_footprintListBox->SetSelectedFootprint( component->GetFPID() );
    else
        m_footprintListBox->SetSelection( m_footprintListBox->GetSelection(), false );

    refreshAfterComponentSearch (component);
}


void CVPCB_MAINFRAME::refreshAfterComponentSearch( COMPONENT* component )
{
    // Tell AuiMgr that objects are changed !
    if( m_auimgr.GetManagedWindow() )   // Be sure Aui Manager is initialized
                                        // (could be not the case when starting CvPcb
        m_auimgr.Update();

    if( component == NULL )
    {
        DisplayStatus();
        return;
    }

    // Preview of the already assigned footprint.
    // Find the footprint that was already chosen for this component and select it,
    // but only if the selection is made from the component list or the library list.
    // If the selection is made from the footprint list, do not change the current
    // selected footprint.
    if( FindFocus() == m_compListBox || FindFocus() == m_libListBox )
    {
        wxString module = FROM_UTF8( component->GetFPID().Format().c_str() );

        m_footprintListBox->SetSelection( m_footprintListBox->GetSelection(), false );

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
                break;
            }
        }

        if( GetFootprintViewerFrame() )
            m_toolManager->RunAction( CVPCB_ACTIONS::showFootprintViewer, true );
    }

    SendMessageToEESCHEMA();
    DisplayStatus();
}


void CVPCB_MAINFRAME::SetFootprintFilter(
        FOOTPRINTS_LISTBOX::FP_FILTER_T aFilter, CVPCB_MAINFRAME::CVPCB_FILTER_ACTION aAction )
{
    int option = FOOTPRINTS_LISTBOX::UNFILTERED_FP_LIST;

    // Extract the needed information about the filter
    switch( aFilter )
    {
    case FOOTPRINTS_LISTBOX::FILTERING_BY_NAME:
        // Extract the current search patten when needed
        m_currentSearchPattern = m_tcFilterString->GetValue();

    case FOOTPRINTS_LISTBOX::UNFILTERED_FP_LIST:
    case FOOTPRINTS_LISTBOX::FILTERING_BY_PIN_COUNT:
    case FOOTPRINTS_LISTBOX::FILTERING_BY_LIBRARY:
    case FOOTPRINTS_LISTBOX::FILTERING_BY_COMPONENT_KEYWORD:
        option = aFilter;
    }

    // Apply the filter accordingly
    switch( aAction )
    {
    case CVPCB_MAINFRAME::FILTER_DISABLE:
        m_filteringOptions &= ~option;
        break;

    case CVPCB_MAINFRAME::FILTER_ENABLE:
        m_filteringOptions |= option;
        break;

    case CVPCB_MAINFRAME::FILTER_TOGGLE:
        m_filteringOptions ^= option;
        break;
    }

    wxListEvent l_event;
    OnSelectComponent( l_event );
}


void CVPCB_MAINFRAME::OnEnterFilteringText( wxCommandEvent& aEvent )
{
    // Called when changing the filter string in main toolbar.
    // If the option FOOTPRINTS_LISTBOX::FILTERING_BY_NAME is set, update the list of
    // available footprints which match the filter

    m_currentSearchPattern = m_tcFilterString->GetValue();

    if( ( m_filteringOptions & FOOTPRINTS_LISTBOX::FILTERING_BY_NAME ) == 0 )
        return;

    wxListEvent l_event;
    OnSelectComponent( l_event );
}


void CVPCB_MAINFRAME::DisplayStatus()
{
    if( !m_initialized )
        return;

    wxString   filters, msg;
    COMPONENT* component = GetSelectedComponent();

    if( ( m_filteringOptions & FOOTPRINTS_LISTBOX::FILTERING_BY_COMPONENT_KEYWORD ) )
    {
        msg.Empty();

        if( component )
        {
            for( unsigned ii = 0;  ii < component->GetFootprintFilters().GetCount();  ii++ )
            {
                if( msg.IsEmpty() )
                    msg += component->GetFootprintFilters()[ii];
                else
                    msg += wxT( ", " ) + component->GetFootprintFilters()[ii];
            }
        }

        filters += _( "key words" ) + wxString::Format( wxT( " (%s)" ), msg );
    }

    if( ( m_filteringOptions & FOOTPRINTS_LISTBOX::FILTERING_BY_PIN_COUNT ) )
    {
        msg.Empty();

        if( component )
            msg = wxString::Format( wxT( "%i" ), component->GetPinCount() );

        if( !filters.IsEmpty() )
            filters += wxT( ", " );

        filters += _( "pin count" ) + wxString::Format( wxT( " (%s)" ), msg );
    }

    if( ( m_filteringOptions & FOOTPRINTS_LISTBOX::FILTERING_BY_LIBRARY ) )
    {
        msg = m_libListBox->GetSelectedLibrary();

        if( !filters.IsEmpty() )
            filters += wxT( ", " );

        filters += _( "library" ) + wxString::Format( wxT( " (%s)" ), msg );
    }

    if( ( m_filteringOptions & FOOTPRINTS_LISTBOX::FILTERING_BY_NAME ) )
    {
        if( !filters.IsEmpty() )
            filters += wxT( ", " );

        filters += _( "search text" );
    }

    if( filters.IsEmpty() )
        msg = _( "No filtering" );
    else
        msg.Printf( _( "Filtered by %s" ), GetChars( filters ) );

    msg << wxT( ": " ) << m_footprintListBox->GetCount();

    SetStatusText( msg );


    msg.Empty();
    wxString footprintName = GetSelectedFootprint();

    FOOTPRINT_INFO* module = m_FootprintsList->GetModuleInfo( footprintName );

    if( module )    // can be NULL if no netlist loaded
    {
        msg = wxString::Format( _( "Description: %s;  Key words: %s" ),
                                module->GetDescription(),
                                module->GetKeywords() );
    }

    SetStatusText( msg, 1 );
}


bool CVPCB_MAINFRAME::LoadFootprintFiles()
{
    FP_LIB_TABLE* fptbl = Prj().PcbFootprintLibs( Kiway() );

    // Check if there are footprint libraries in the footprint library table.
    if( !fptbl || !fptbl->GetLogicalLibs().size() )
    {
        wxMessageBox( _( "No PCB footprint libraries are listed in the current footprint "
                         "library table." ), _( "Configuration Error" ), wxOK | wxICON_ERROR );
        return false;
    }

    WX_PROGRESS_REPORTER progressReporter( this, _( "Loading Footprint Libraries" ), 2 );

    m_FootprintsList->ReadFootprintFiles( fptbl, nullptr, &progressReporter );

    if( m_FootprintsList->GetErrorCount() )
    {
        m_FootprintsList->DisplayErrors( this );
    }

    return true;
}


void CVPCB_MAINFRAME::SendMessageToEESCHEMA( bool aClearHighligntOnly )
{
    if( m_netlist.IsEmpty() )
        return;

    // clear highlight of previously selected components (if any):
    // Selecting a non existing symbol clears any previously highlighted symbols
    std::string packet = "$CLEAR: \"HIGHLIGHTED\"";

    if( Kiface().IsSingle() )
        SendCommand( MSG_TO_SCH, packet.c_str() );
    else
        Kiway().ExpressMail( FRAME_SCH, MAIL_CROSS_PROBE, packet, this );

    if( aClearHighligntOnly )
        return;

    int selection = m_compListBox->GetSelection();

    if ( selection < 0 )    // Nothing selected
        return;

    if( m_netlist.GetComponent( selection ) == NULL )
        return;

    // Now highlight the selected component:
    COMPONENT* component = m_netlist.GetComponent( selection );

    packet = StrPrintf( "$PART: \"%s\"", TO_UTF8( component->GetReference() ) );

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
        wxString msg = wxString::Format( _( "Error loading schematic.\n%s" ),
                                         ioe.What().GetData() );
        wxMessageBox( msg, _( "Load Error" ), wxOK | wxICON_ERROR );
        return 1;
    }

    // We also remove footprint name if it is "$noname" because this is a dummy name,
    // not the actual name of the footprint.
    for( unsigned ii = 0; ii < m_netlist.GetCount(); ii++ )
    {
        if( m_netlist.GetComponent( ii )->GetFPID().GetLibItemName() == std::string( "$noname" ) )
            m_netlist.GetComponent( ii )->SetFPID( LIB_ID() );
    }

    // Sort components by reference:
    m_netlist.SortByReference();

    return 0;
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

    m_footprintListBox->SetFootprints( *m_FootprintsList, wxEmptyString, NULL,
            wxEmptyString, FOOTPRINTS_LISTBOX::UNFILTERED_FP_LIST );
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

    FP_LIB_TABLE* tbl = Prj().PcbFootprintLibs( Kiway() );

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


std::vector<unsigned int> CVPCB_MAINFRAME::GetSelectedComponentIndices()
{
    std::vector<unsigned int> idx;

    // Check to see if anything is selected
    if( m_compListBox->GetSelectedItemCount() < 1 )
        return idx;

    // Get the components
    int lastIdx = m_compListBox->GetFirstSelected();
    idx.emplace_back( lastIdx );

    lastIdx = m_compListBox->GetNextSelected( lastIdx );
    while( lastIdx > 0 )
    {
        idx.emplace_back( lastIdx );
        lastIdx = m_compListBox->GetNextSelected( lastIdx );
    }

    return idx;
}


DISPLAY_FOOTPRINTS_FRAME* CVPCB_MAINFRAME::GetFootprintViewerFrame()
{
    // returns the Footprint Viewer frame, if exists, or NULL
    return dynamic_cast<DISPLAY_FOOTPRINTS_FRAME*>
            ( wxWindow::FindWindowByName( FOOTPRINTVIEWER_FRAME_NAME ) );
}


wxString CVPCB_MAINFRAME::GetSelectedFootprint()
{
    // returns the LIB_ID of the selected footprint in footprint listview
    // or a empty string
    return m_footprintListBox->GetSelectedFootprint();
}


void CVPCB_MAINFRAME::SetStatusText( const wxString& aText, int aNumber )
{
    wxASSERT( aNumber < 2 );

    if( aNumber == 1 )
        m_statusLine2->SetLabel( aText );
    else
        m_statusLine1->SetLabel( aText );
}


void CVPCB_MAINFRAME::ShowChangedLanguage()
{
    EDA_BASE_FRAME::ShowChangedLanguage();
    ReCreateHToolbar();
    DisplayStatus();
}


void CVPCB_MAINFRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
{
    const std::string& payload = mail.GetPayload();

    //DBG(printf( "%s: %s\n", __func__, payload.c_str() );)

    switch( mail.Command() )
    {
    case MAIL_EESCHEMA_NETLIST:
        ReadNetListAndFpFiles( payload );
        /* @todo
        Go into SCH_EDIT_FRAME::OnOpenCvpcb( wxCommandEvent& event ) and trim GNL_ALL down.
        */
        break;

    default:
        ;       // ignore most
    }
}
