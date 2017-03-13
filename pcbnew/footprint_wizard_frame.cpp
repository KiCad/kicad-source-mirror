/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2015 Miguel Angel Ajo Pelayo <miguelangel@nbee.es>
 * Copyright (C) 2012-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2017 KiCad Developers, see change_log.txt for contributors.
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
 * @file footprint_wizard_frame.cpp
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <class_drawpanel.h>
#include <wxPcbStruct.h>
#include <pcbnew.h>
#include <3d_viewer/eda_3d_viewer.h>
#include <msgpanel.h>
#include <macros.h>
#include <bitmaps.h>

#include <class_board.h>
#include <class_module.h>
#include <module_editor_frame.h>

#include <pcbnew_id.h>
#include "footprint_wizard_frame.h"
#include <footprint_info.h>
#include <wx/grid.h>
#include <wx/tokenzr.h>
#include <wx/numformatter.h>

#include <hotkeys.h>
#include <wildcards_and_files_ext.h>
#include <base_units.h>


BEGIN_EVENT_TABLE( FOOTPRINT_WIZARD_FRAME, EDA_DRAW_FRAME )

    // Window events
    EVT_CLOSE( FOOTPRINT_WIZARD_FRAME::OnCloseWindow )
    EVT_SIZE( FOOTPRINT_WIZARD_FRAME::OnSize )
    EVT_ACTIVATE( FOOTPRINT_WIZARD_FRAME::OnActivate )

     // Toolbar events
    EVT_TOOL( ID_FOOTPRINT_WIZARD_SELECT_WIZARD,
              FOOTPRINT_WIZARD_FRAME::SelectCurrentWizard )

    EVT_TOOL( ID_FOOTPRINT_WIZARD_RESET_TO_DEFAULT,
              FOOTPRINT_WIZARD_FRAME::DefaultParameters )

    EVT_TOOL( ID_FOOTPRINT_WIZARD_NEXT,
              FOOTPRINT_WIZARD_FRAME::Process_Special_Functions )

    EVT_TOOL( ID_FOOTPRINT_WIZARD_PREVIOUS,
              FOOTPRINT_WIZARD_FRAME::Process_Special_Functions )

    EVT_TOOL( ID_FOOTPRINT_WIZARD_DONE,
              FOOTPRINT_WIZARD_FRAME::ExportSelectedFootprint )

    EVT_TOOL( ID_FOOTPRINT_WIZARD_SHOW_3D_VIEW,
              FOOTPRINT_WIZARD_FRAME::Show3D_Frame )

    // listbox events

    EVT_LISTBOX( ID_FOOTPRINT_WIZARD_PAGE_LIST, FOOTPRINT_WIZARD_FRAME::ClickOnPageList )
    EVT_GRID_CMD_CELL_CHANGED( ID_FOOTPRINT_WIZARD_PARAMETER_LIST,
                               FOOTPRINT_WIZARD_FRAME::ParametersUpdated )

    EVT_MENU( ID_SET_RELATIVE_OFFSET, FOOTPRINT_WIZARD_FRAME::OnSetRelativeOffset )
END_EVENT_TABLE()


#define FOOTPRINT_WIZARD_FRAME_NAME wxT( "FootprintWizard" )

/* Note: our FOOTPRINT_WIZARD_FRAME is always modal.
 * Note:
 * On windows, when the frame with type wxFRAME_FLOAT_ON_PARENT is displayed
 * its parent frame is sometimes brought to the foreground when closing the
 * LIB_VIEW_FRAME frame.
 * If it still happens, it could be better to use wxSTAY_ON_TOP
 * instead of wxFRAME_FLOAT_ON_PARENT
 */
#ifdef __WINDOWS__
#define MODAL_MODE_EXTRASTYLE wxFRAME_FLOAT_ON_PARENT   // could be wxSTAY_ON_TOP if issues
#else
#define MODAL_MODE_EXTRASTYLE wxFRAME_FLOAT_ON_PARENT
#endif

FOOTPRINT_WIZARD_FRAME::FOOTPRINT_WIZARD_FRAME( KIWAY* aKiway,
        wxWindow* aParent, FRAME_T aFrameType ) :
    PCB_BASE_FRAME( aKiway, aParent, aFrameType, _( "Footprint Wizard" ),
                wxDefaultPosition, wxDefaultSize,
                aParent ? KICAD_DEFAULT_DRAWFRAME_STYLE | MODAL_MODE_EXTRASTYLE
                          : KICAD_DEFAULT_DRAWFRAME_STYLE | wxSTAY_ON_TOP,
                FOOTPRINT_WIZARD_FRAME_NAME )
{
    wxASSERT( aFrameType == FRAME_PCB_FOOTPRINT_WIZARD_MODAL );

    // This frame is always show modal:
    SetModal( true );

    m_messagesFrame = NULL;     // This windows will be created the first time a wizard is loaded
    m_showAxis      = true;     // true to draw axis.

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( module_wizard_xpm) );
    SetIcon( icon );

    m_hotkeysDescrList = g_Module_Viewer_Hokeys_Descr;
    m_wizardName.Empty();

    SetBoard( new BOARD() );

    // Ensure all layers and items are visible:
    GetBoard()->SetVisibleAlls();
    SetScreen( new PCB_SCREEN( GetPageSizeIU() ) );
    GetScreen()->m_Center = true;      // Center coordinate origins on screen.

    LoadSettings( config() );

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    // Set some display options here, because the FOOTPRINT_WIZARD_FRAME
    // does not have a config menu to do that:
    DISPLAY_OPTIONS* disp_opts = (DISPLAY_OPTIONS*) GetDisplayOptions();
    disp_opts->m_DisplayPadIsol = false;
    disp_opts->m_DisplayPadNum = true;
    GetBoard()->SetElementVisibility( LAYER_NO_CONNECTS, false );

    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    ReCreateHToolbar();
    ReCreateVToolbar();

    // Creates the parameter pages list
    m_pageList = new wxListBox( this, ID_FOOTPRINT_WIZARD_PAGE_LIST,
                                wxDefaultPosition, wxDefaultSize,
                                0, NULL, wxLB_HSCROLL );

    // Creates the The list of parameters for the current parameter page
    initParameterGrid();

    ReCreatePageList();

    DisplayWizardInfos();

    m_auimgr.SetManagedWindow( this );

    EDA_PANEINFO horiztb;
    horiztb.HorizontalToolbarPane();

    EDA_PANEINFO    info;
    info.InfoToolbarPane();

    EDA_PANEINFO    mesg;
    mesg.MessageToolbarPane();

    // Manage main toolbal
    m_auimgr.AddPane( m_mainToolBar, wxAuiPaneInfo( horiztb ).
                      Name( wxT ("m_mainToolBar" ) ).Top().Row( 0 ) );

    // Manage the left window (list of parameter pages)
    EDA_PANEINFO paneList;
    paneList.InfoToolbarPane().Name( wxT( "m_pageList" ) ).Left().Row( 0 );
    m_auimgr.AddPane( m_pageList, wxAuiPaneInfo( paneList ) );

    // Manage the parameters grid editor for the current parameter page
    EDA_PANEINFO panePrms;
    panePrms.InfoToolbarPane().Name( wxT( "m_parameterGrid" ) ).Left().Row( 1 );
    m_auimgr.AddPane( m_parameterGrid, wxAuiPaneInfo( panePrms ) );

    // Manage the draw panel
    m_auimgr.AddPane( m_canvas,
                      wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    // Manage the message panel
    m_auimgr.AddPane( m_messagePanel,
                      wxAuiPaneInfo( mesg ).Name( wxT( "MsgPanel" ) ).Bottom().Layer(1) );

    // Gives a min size and the last saved size to left windows
    m_auimgr.GetPane( m_pageList ).MinSize( wxSize(60, -1 ) );
    m_auimgr.GetPane( m_pageList ).BestSize( wxSize(m_pageListWidth, -1) );

    m_auimgr.GetPane( m_parameterGrid ).MinSize( wxSize( 120, -1 ) );
    m_auimgr.GetPane( m_parameterGrid ).BestSize( wxSize(m_parameterGridWidth, -1) );

    m_auimgr.Update();

    // Now Drawpanel is sized, we can use BestZoom to show the component (if any)
#ifdef USE_WX_GRAPHICS_CONTEXT
    GetScreen()->SetZoom( BestZoom() );
#else
    Zoom_Automatique( false );
#endif

    // Do not Run a dialog here: on some Window Managers, it creates issues.
    // Reason: the FOOTPRINT_WIZARD_FRAME is run as modal;
    // It means the call to FOOTPRINT_WIZARD_FRAME::ShowModal will change the
    // Event Loop Manager, and stop the one created by the dialog.
    // It does not happen on all W.M., perhaps due to the way the order events are called
//    SelectFootprintWizard();
}


FOOTPRINT_WIZARD_FRAME::~FOOTPRINT_WIZARD_FRAME()
{
    EDA_3D_VIEWER* draw3DFrame = Get3DViewerFrame();

    if( draw3DFrame )
        draw3DFrame->Destroy();
}


void FOOTPRINT_WIZARD_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    if( m_messagesFrame )
        m_messagesFrame->SaveSettings();

    SaveSettings( config() );

    if( IsModal() )
    {
        // Only dismiss a modal frame once, so that the return values set by
        // the prior DismissModal() are not bashed for ShowModal().
        if( !IsDismissed() )
            DismissModal( false );
    }
    else
    {
        Destroy();
    }
}


void FOOTPRINT_WIZARD_FRAME::ExportSelectedFootprint( wxCommandEvent& aEvent )
{
    DismissModal( true );
    Close();
}

void FOOTPRINT_WIZARD_FRAME::OnGridSize( wxSizeEvent& aSizeEvent )
{
    // Resize the parameter columns
    ResizeParamColumns();

    aSizeEvent.Skip();
}

void FOOTPRINT_WIZARD_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();

    SizeEv.Skip();
}


void FOOTPRINT_WIZARD_FRAME::OnSetRelativeOffset( wxCommandEvent& event )
{
    GetScreen()->m_O_Curseur = GetCrossHairPosition();
    UpdateStatusBar();
}

void  FOOTPRINT_WIZARD_FRAME::initParameterGrid()
{
    // Prepare the grid where parameters are displayed
    m_parameterGrid = new wxGrid( this, ID_FOOTPRINT_WIZARD_PARAMETER_LIST );
    m_parameterGrid->CreateGrid( 0, 3 );

    // Columns
    m_parameterGrid->SetColLabelValue( WIZ_COL_NAME, _( "Parameter" ) );
    m_parameterGrid->SetColLabelValue( WIZ_COL_VALUE, _( "Value" ) );
    m_parameterGrid->SetColLabelValue( WIZ_COL_UNITS, _( "Units" ) );

    m_parameterGrid->SetColLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );
    m_parameterGrid->AutoSizeColumns();

    // Rows
    m_parameterGrid->AutoSizeRows();
    m_parameterGrid->SetRowLabelSize( 25 );
    m_parameterGrid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );

    m_parameterGrid->DisableDragGridSize();
    m_parameterGrid->DisableDragColSize();

    m_parameterGrid->Connect( wxEVT_SIZE, wxSizeEventHandler(FOOTPRINT_WIZARD_FRAME::OnGridSize), NULL, this );
}


void FOOTPRINT_WIZARD_FRAME::ReCreatePageList()
{
    if( m_pageList == NULL )
        return;

    FOOTPRINT_WIZARD* footprintWizard = GetMyWizard();

    if( !footprintWizard )
        return;

    m_pageList->Clear();
    int max_page = footprintWizard->GetNumParameterPages();

    for( int i = 0; i<max_page; i++ )
    {
        wxString name = footprintWizard->GetParameterPageName( i );
        m_pageList->Append( name );
    }

    m_pageList->SetSelection( 0, true );

    ReCreateParameterList();
    ReCreateHToolbar();
    DisplayWizardInfos();
    m_canvas->Refresh();
}


void FOOTPRINT_WIZARD_FRAME::ReCreateParameterList()
{
    if( m_parameterGrid == NULL )
        return;

    FOOTPRINT_WIZARD* footprintWizard = GetMyWizard();

    if( footprintWizard == NULL )
        return;

    int page = m_pageList->GetSelection();

    if( page<0 )
        return;

    m_parameterGrid->ClearGrid();

    // Get the list of names, values, types, hints and designators
    wxArrayString designatorsList    = footprintWizard->GetParameterDesignators( page );
    wxArrayString namesList          = footprintWizard->GetParameterNames( page );
    wxArrayString valuesList         = footprintWizard->GetParameterValues( page );
    wxArrayString typesList          = footprintWizard->GetParameterTypes( page );
    wxArrayString hintsList          = footprintWizard->GetParameterHints( page );

    // Dimension the wxGrid
    if( m_parameterGrid->GetNumberRows() > 0 )
        m_parameterGrid->DeleteRows( 0, m_parameterGrid->GetNumberRows() );

    m_parameterGrid->AppendRows( namesList.size() );

    wxString designator, name, value, units, hint;

    for( unsigned int i = 0; i< namesList.size(); i++ )
    {
        designator  = designatorsList[i];
        name        = namesList[i];
        value       = valuesList[i];
        units       = typesList[i];
        hint        = hintsList[i];

        m_parameterGrid->SetRowLabelValue( i, designator );

        // Set the 'Name'
        m_parameterGrid->SetCellValue( i, WIZ_COL_NAME, name );
        m_parameterGrid->SetReadOnly( i, WIZ_COL_NAME );
        m_parameterGrid->SetCellAlignment( i, WIZ_COL_NAME, wxALIGN_LEFT, wxALIGN_CENTRE );

        // Set the editor type of the

        // Boolean parameters can be displayed using a checkbox
        if( units == WIZARD_PARAM_UNITS_BOOL )
        {
            wxGridCellBoolEditor *boolEditor = new wxGridCellBoolEditor;
            boolEditor->UseStringValues( "1", "0" );
            m_parameterGrid->SetCellEditor( i, WIZ_COL_VALUE, boolEditor );
            m_parameterGrid->SetCellRenderer( i, WIZ_COL_VALUE, new wxGridCellBoolRenderer );
        }
        // Parameters that can be selected from a list of multiple options
        else if( units.Contains( "," ) )  // Indicates list of available options
        {
            wxStringTokenizer tokenizer( units, "," );
            wxArrayString options;

            while( tokenizer.HasMoreTokens() )
            {
                options.Add( tokenizer.GetNextToken() );
            }

            m_parameterGrid->SetCellEditor( i, WIZ_COL_VALUE, new wxGridCellChoiceEditor( options ) );

            units = wxT( "" );
        }
        // Integer parameters
        else if( units == WIZARD_PARAM_UNITS_INTEGER )
        {
            m_parameterGrid->SetCellEditor( i, WIZ_COL_VALUE, new wxGridCellNumberEditor );
        }
        // Non-integer numerical parameters
        else if( ( units == WIZARD_PARAM_UNITS_MM )      ||
                 ( units == WIZARD_PARAM_UNITS_MILS )    ||
                 ( units == WIZARD_PARAM_UNITS_FLOAT )   ||
                 ( units == WIZARD_PARAM_UNITS_RADIANS ) ||
                 ( units == WIZARD_PARAM_UNITS_DEGREES ) ||
                 ( units == WIZARD_PARAM_UNITS_PERCENT ) )
        {
            m_parameterGrid->SetCellEditor( i, WIZ_COL_VALUE, new wxGridCellFloatEditor );

            // Convert separators to the locale-specific character
            value.Replace( ",", wxNumberFormatter::GetDecimalSeparator() );
            value.Replace( ".", wxNumberFormatter::GetDecimalSeparator() );
        }

        // Set the 'Units'
        m_parameterGrid->SetCellValue( i, WIZ_COL_UNITS, units );
        m_parameterGrid->SetReadOnly( i, WIZ_COL_UNITS );
        m_parameterGrid->SetCellAlignment( i, WIZ_COL_UNITS, wxALIGN_LEFT, wxALIGN_CENTRE );

        // Set the 'Value'
        m_parameterGrid->SetCellValue( i, WIZ_COL_VALUE, value );
        m_parameterGrid->SetCellAlignment( i, WIZ_COL_VALUE, wxALIGN_CENTRE, wxALIGN_CENTRE );
    }

    ResizeParamColumns();
}

void FOOTPRINT_WIZARD_FRAME::ResizeParamColumns()
{

    // Parameter grid is not yet configured
    if( ( m_parameterGrid == NULL ) || ( m_parameterGrid->GetNumberCols() == 0 ) )
        return;

    // first auto-size the columns to ensure enough space around text
    m_parameterGrid->AutoSizeColumns();

    // Auto-size the value column
    int width = m_parameterGrid->GetClientSize().GetWidth() -
                m_parameterGrid->GetRowLabelSize() -
                m_parameterGrid->GetColSize( WIZ_COL_NAME ) -
                m_parameterGrid->GetColSize( WIZ_COL_UNITS );

    if( width > m_parameterGrid->GetColMinimalAcceptableWidth() )
    {
        m_parameterGrid->SetColSize( WIZ_COL_VALUE, width );
    }
}


void FOOTPRINT_WIZARD_FRAME::ClickOnPageList( wxCommandEvent& event )
{
    int ii = m_pageList->GetSelection();

    if( ii < 0 )
        return;

    ReCreateParameterList();
    m_canvas->Refresh();
    DisplayWizardInfos();
}


#define PAGE_LIST_WIDTH_KEY  wxT( "Fpwizard_Pagelist_width" )
#define PARAMLIST_WIDTH_KEY wxT( "Fpwizard_Paramlist_width" )


void FOOTPRINT_WIZARD_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );

    aCfg->Read( PAGE_LIST_WIDTH_KEY, &m_pageListWidth, 100 );
    aCfg->Read( PARAMLIST_WIDTH_KEY, &m_parameterGridWidth, 200 );

    // Set parameters to a reasonable value.
    if( m_pageListWidth > m_FrameSize.x / 3 )
        m_pageListWidth = m_FrameSize.x / 3;

    if( m_parameterGridWidth > m_FrameSize.x / 2 )
        m_parameterGridWidth = m_FrameSize.x / 2;
}


void FOOTPRINT_WIZARD_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::SaveSettings( aCfg );

    aCfg->Write( PAGE_LIST_WIDTH_KEY, m_pageList->GetSize().x );
    aCfg->Write( PARAMLIST_WIDTH_KEY, m_parameterGrid->GetSize().x );
}


void FOOTPRINT_WIZARD_FRAME::OnActivate( wxActivateEvent& event )
{
    EDA_DRAW_FRAME::OnActivate( event );

    // Ensure we do not have old selection:
    if( !event.GetActive() )
        return;

#if 0
    // Currently, we do not have a way to see if a Python wizard has changed,
    // therefore the lists of parameters and option has to be rebuilt
    // This code could be enabled when this way exists
    bool footprintWizardsChanged = false;

    if( footprintWizardsChanged )
    {
        // If we are here, the library list has changed, rebuild it
        ReCreatePageList();
        DisplayWizardInfos();
    }
#endif
}


bool FOOTPRINT_WIZARD_FRAME::GeneralControl( wxDC* aDC, const wxPoint& aPosition, EDA_KEY aHotKey )
{
    bool eventHandled = true;

    // Filter out the 'fake' mouse motion after a keyboard movement
    if( !aHotKey && m_movingCursorWithKeyboard )
    {
        m_movingCursorWithKeyboard = false;
        return false;
    }

    wxCommandEvent  cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    wxPoint pos = aPosition;
    wxPoint oldpos = GetCrossHairPosition();
    GeneralControlKeyMovement( aHotKey, &pos, true );

    switch( aHotKey )
    {
    case WXK_F1:
        cmd.SetId( ID_POPUP_ZOOM_IN );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case WXK_F2:
        cmd.SetId( ID_POPUP_ZOOM_OUT );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case WXK_F3:
        cmd.SetId( ID_ZOOM_REDRAW );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case WXK_F4:
        cmd.SetId( ID_POPUP_ZOOM_CENTER );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case WXK_HOME:
        cmd.SetId( ID_ZOOM_PAGE );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case ' ':
        GetScreen()->m_O_Curseur = GetCrossHairPosition();
        break;

    default:
        eventHandled = false;
    }

    SetCrossHairPosition( pos );
    RefreshCrossHair( oldpos, aPosition, aDC );

    UpdateStatusBar();    // Display new cursor coordinates

    return eventHandled;
}


void FOOTPRINT_WIZARD_FRAME::Show3D_Frame( wxCommandEvent& event )
{
    EDA_3D_VIEWER* draw3DFrame = Get3DViewerFrame();

    if( draw3DFrame )
    {
        // Raising the window does not show the window on Windows if iconized.
        // This should work on any platform.
        if( draw3DFrame->IsIconized() )
            draw3DFrame->Iconize( false );

        draw3DFrame->Raise();

        // Raising the window does not set the focus on Linux.  This should work on any platform.
        if( wxWindow::FindFocus() != draw3DFrame )
            draw3DFrame->SetFocus();

        return;
    }

    draw3DFrame = new EDA_3D_VIEWER( &Kiway(), this, wxEmptyString );
    Update3D_Frame( false );
    draw3DFrame->Raise();     // Needed with some Window Managers
    draw3DFrame->Show( true );
}


/**
 * Function Update3D_Frame
 * must be called after a footprint selection
 * Updates the 3D view and 3D frame title.
 */
void FOOTPRINT_WIZARD_FRAME::Update3D_Frame( bool aForceReloadFootprint )
{
    EDA_3D_VIEWER* draw3DFrame = Get3DViewerFrame();

    if( draw3DFrame == NULL )
        return;

    wxString frm3Dtitle;
    frm3Dtitle.Printf( _( "ModView: 3D Viewer [%s]" ), GetChars( m_wizardName ) );
    draw3DFrame->SetTitle( frm3Dtitle );

    if( aForceReloadFootprint )
    {
        draw3DFrame->ReloadRequest();

        // Force 3D screen refresh immediately
        if( GetBoard()->m_Modules )
            draw3DFrame->NewDisplay();
    }
}


void FOOTPRINT_WIZARD_FRAME::ReCreateHToolbar()
{
    wxString msg;

    if( !m_mainToolBar )
    {
        m_mainToolBar = new wxAuiToolBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                          wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

        // Set up toolbar
        m_mainToolBar->AddTool( ID_FOOTPRINT_WIZARD_SELECT_WIZARD, wxEmptyString,
                                KiBitmap( module_wizard_xpm ),
                                _( "Select the wizard script to load and run" ) );

        m_mainToolBar->AddSeparator();

        m_mainToolBar->AddTool( ID_FOOTPRINT_WIZARD_RESET_TO_DEFAULT, wxEmptyString,
                                KiBitmap( reload_xpm ),
                                _( "Reset the wizard parameters to default values ") );

        m_mainToolBar->AddSeparator();

        m_mainToolBar->AddTool( ID_FOOTPRINT_WIZARD_PREVIOUS, wxEmptyString,
                                KiBitmap( lib_previous_xpm ),
                                _( "Select previous parameters page" ) );

        m_mainToolBar->AddTool( ID_FOOTPRINT_WIZARD_NEXT, wxEmptyString,
                                KiBitmap( lib_next_xpm ),
                                _( "Select next parameters page" ) );

        m_mainToolBar->AddSeparator();
        m_mainToolBar->AddTool( ID_FOOTPRINT_WIZARD_SHOW_3D_VIEW, wxEmptyString,
                                KiBitmap( three_d_xpm ),
                                _( "Show footprint in 3D viewer" ) );

        m_mainToolBar->AddSeparator();
        msg = AddHotkeyName( _( "Zoom in" ), g_Module_Editor_Hokeys_Descr,
                             HK_ZOOM_IN, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString,
                                KiBitmap( zoom_in_xpm ), msg );

        msg = AddHotkeyName( _( "Zoom out" ), g_Module_Editor_Hokeys_Descr,
                             HK_ZOOM_OUT, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString,
                                KiBitmap( zoom_out_xpm ), msg );

        msg = AddHotkeyName( _( "Redraw view" ), g_Module_Editor_Hokeys_Descr,
                             HK_ZOOM_REDRAW, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                                KiBitmap( zoom_redraw_xpm ), msg );

        msg = AddHotkeyName( _( "Zoom auto" ), g_Module_Editor_Hokeys_Descr,
                             HK_ZOOM_AUTO, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString,
                                KiBitmap( zoom_fit_in_page_xpm ), msg );

        // The footprint wizard always can export the current footprint
        m_mainToolBar->AddSeparator();
        m_mainToolBar->AddTool( ID_FOOTPRINT_WIZARD_DONE,
                                wxEmptyString, KiBitmap( export_footprint_names_xpm ),
                                _( "Export the footprint to the editor" ) );

        // after adding the buttons to the toolbar, must call Realize() to
        // reflect the changes
        m_mainToolBar->Realize();
    }

    m_mainToolBar->Refresh();
}


void FOOTPRINT_WIZARD_FRAME::ReCreateVToolbar()
{
    // Currently, there is no vertical toolbar
}

#if defined(KICAD_SCRIPTING)
void FOOTPRINT_WIZARD_FRAME::PythonPluginsReload()
{
    // Reload the Python plugins
    // Because the board editor has also a plugin python menu,
    // call PCB_EDIT_FRAME::PythonPluginsReload() if the board editor
    // is running
    PCB_EDIT_FRAME* brd_frame =
        static_cast<PCB_EDIT_FRAME*>( Kiway().Player( FRAME_PCB, false ) );

    if( brd_frame )
        brd_frame->PythonPluginsReload();
    else
        PythonPluginsReloadBase();
}
#endif

// frame to display messages from footprint builder scripts
FOOTPRINT_WIZARD_MESSAGES::FOOTPRINT_WIZARD_MESSAGES( FOOTPRINT_WIZARD_FRAME* aParent, wxConfigBase* aCfg ) :
        wxMiniFrame( aParent, wxID_ANY, _( "Footprint Builder Messages" ),
                     wxDefaultPosition, wxDefaultSize,
                     wxCAPTION | wxRESIZE_BORDER | wxFRAME_FLOAT_ON_PARENT )
{
    m_canClose = false;
    wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( bSizer );

    m_messageWindow = new wxTextCtrl( this, wxID_ANY, wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize,
                                      wxTE_MULTILINE|wxTE_READONLY );
    bSizer->Add( m_messageWindow, 1, wxEXPAND, 0 );

    m_config = aCfg;

    LoadSettings();

    SetSize( m_position.x, m_position.y, m_size.x, m_size.y );

    m_messageWindow->SetMinSize( wxSize( 350, 250 ) );
    Layout();

    bSizer->SetSizeHints( this );
}


FOOTPRINT_WIZARD_MESSAGES::~FOOTPRINT_WIZARD_MESSAGES()
{
}


BEGIN_EVENT_TABLE( FOOTPRINT_WIZARD_MESSAGES, wxMiniFrame )
    EVT_CLOSE( FOOTPRINT_WIZARD_MESSAGES::OnCloseMsgWindow )
END_EVENT_TABLE()


void FOOTPRINT_WIZARD_MESSAGES::OnCloseMsgWindow( wxCloseEvent& aEvent )
{
    if( !m_canClose )
        aEvent.Veto();
    else
        aEvent.Skip();
}


void FOOTPRINT_WIZARD_MESSAGES::PrintMessage( const wxString& aMessage )
{
    m_messageWindow->SetValue( aMessage );
}


void FOOTPRINT_WIZARD_MESSAGES::ClearScreen()
{
    m_messageWindow->Clear();
}


#define MESSAGE_BOX_POSX_KEY wxT( "Fpwizard_Msg_PosX" )
#define MESSAGE_BOX_POSY_KEY wxT( "Fpwizard_Msg_PosY" )
#define MESSAGE_BOX_SIZEX_KEY wxT( "Fpwizard_Msg_SIZEX" )
#define MESSAGE_BOX_SIZEY_KEY wxT( "Fpwizard_Msg_SIZEY" )

void FOOTPRINT_WIZARD_MESSAGES::SaveSettings()
{
    if( !IsIconized() )
    {
        m_position = GetPosition();
        m_size = GetSize();
    }

    m_config->Write( MESSAGE_BOX_POSX_KEY, m_position.x );
    m_config->Write( MESSAGE_BOX_POSY_KEY, m_position.y );
    m_config->Write( MESSAGE_BOX_SIZEX_KEY, m_size.x );
    m_config->Write( MESSAGE_BOX_SIZEY_KEY, m_size.y );

    m_canClose = false;     // close event now allowed
}


void FOOTPRINT_WIZARD_MESSAGES::LoadSettings()
{
    m_config->Read( MESSAGE_BOX_POSX_KEY, &m_position.x, -1 );
    m_config->Read( MESSAGE_BOX_POSY_KEY, &m_position.y, -1 );
    m_config->Read( MESSAGE_BOX_SIZEX_KEY, &m_size.x, 350 );
    m_config->Read( MESSAGE_BOX_SIZEY_KEY, &m_size.y, 250 );
}
