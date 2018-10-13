/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2015 Miguel Angel Ajo Pelayo <miguelangel@nbee.es>
 * Copyright (C) 2012-2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_edit_frame.h>
#include <pcbnew.h>
#include <3d_viewer/eda_3d_viewer.h>
#include <msgpanel.h>
#include <macros.h>
#include <bitmaps.h>
#include <grid_tricks.h>

#include <class_board.h>
#include <class_module.h>
#include <footprint_edit_frame.h>

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

    m_showAxis      = true;     // true to draw axis.

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( module_wizard_xpm) );
    SetIcon( icon );

    m_hotkeysDescrList = g_Module_Viewer_Hotkeys_Descr;
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
    auto disp_opts = (PCB_DISPLAY_OPTIONS*)GetDisplayOptions();
    disp_opts->m_DisplayPadIsol = false;
    disp_opts->m_DisplayPadNum = true;
    GetBoard()->SetElementVisibility( LAYER_NO_CONNECTS, false );

    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    ReCreateHToolbar();
    ReCreateVToolbar();
    SetActiveLayer( F_Cu );

    // Create the parameters panel
    m_parametersPanel = new wxPanel( this, wxID_ANY );

    m_pageList = new wxListBox( m_parametersPanel, ID_FOOTPRINT_WIZARD_PAGE_LIST,
                                wxDefaultPosition, wxDefaultSize,
                                0, NULL, wxLB_HSCROLL );

    m_parameterGrid = new wxGrid( m_parametersPanel, ID_FOOTPRINT_WIZARD_PARAMETER_LIST );
    initParameterGrid();
    m_parameterGrid->PushEventHandler( new GRID_TRICKS( m_parameterGrid ) );

    ReCreatePageList();

    wxBoxSizer* parametersSizer = new wxBoxSizer( wxHORIZONTAL );
    parametersSizer->Add( m_pageList, 0, wxEXPAND|wxALL, 5 );
    parametersSizer->Add( m_parameterGrid, 1, wxEXPAND|wxALL, 5 );
    m_parametersPanel->SetSizer( parametersSizer );
    m_parametersPanel->Layout();

    // Create the build message box
    m_buildMessageBox = new wxTextCtrl( this, wxID_ANY, wxEmptyString,
                                        wxDefaultPosition, wxDefaultSize,
                                        wxTE_MULTILINE|wxTE_READONLY );

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

    // Manage the parameters panel
    EDA_PANEINFO parametersPaneInfo;
    parametersPaneInfo.InfoToolbarPane().Name( wxT( "m_parametersPanel" ) ).Left().Position( 0 );
    m_auimgr.AddPane( m_parametersPanel, wxAuiPaneInfo( parametersPaneInfo ) );

    // Manage the build message box
    EDA_PANEINFO buildMessageBoxInfo;
    buildMessageBoxInfo.InfoToolbarPane().Name( wxT( "m_buildMessageBox" ) ).Left().Position( 1 );
    m_auimgr.AddPane( m_buildMessageBox, wxAuiPaneInfo( buildMessageBoxInfo ) );

    // Manage the draw panel
    m_auimgr.AddPane( m_canvas,
                      wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    // Manage the message panel
    m_auimgr.AddPane( m_messagePanel,
                      wxAuiPaneInfo( mesg ).Name( wxT( "MsgPanel" ) ).Bottom().Layer(1) );

    // Give a min size to the parameters
    m_auimgr.GetPane( m_parametersPanel ).MinSize( wxSize( 360, 180 ) );

    // DO  NOT use m_auimgr.LoadPerspective( m_auiPerspective ) here.
    // It creates issues when items managed by m_auimgr are modified over the time or
    // Kicad versions (some are not displayed)

    // Ensure the draw panel is shown, regardless the perspective config:
    m_auimgr.GetPane( wxT( "DrawFrame" ) ).Show( true );

    // Now Drawpanel is sized, we can use BestZoom to show the component (if any)
#ifdef USE_WX_GRAPHICS_CONTEXT
    GetScreen()->SetScalingFactor( BestZoom() );
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
    // Delete the GRID_TRICKS.
    m_parameterGrid->PopEventHandler( true );

    EDA_3D_VIEWER* draw3DFrame = Get3DViewerFrame();

    if( draw3DFrame )
        draw3DFrame->Destroy();
}


void FOOTPRINT_WIZARD_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
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
    m_parameterGridPage = -1;

    // Prepare the grid where parameters are displayed

    m_parameterGrid->CreateGrid( 0, 3 );

    m_parameterGrid->SetColLabelValue( WIZ_COL_NAME, _( "Parameter" ) );
    m_parameterGrid->SetColLabelValue( WIZ_COL_VALUE, _( "Value" ) );
    m_parameterGrid->SetColLabelValue( WIZ_COL_UNITS, _( "Units" ) );

    m_parameterGrid->SetColLabelSize( 22 );
    m_parameterGrid->SetColLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );
    m_parameterGrid->AutoSizeColumns();

    m_parameterGrid->AutoSizeRows();
    m_parameterGrid->SetRowLabelSize( 0 );

    m_parameterGrid->DisableDragGridSize();
    m_parameterGrid->DisableDragColSize();

    m_parameterGrid->Connect( wxEVT_SIZE,
                              wxSizeEventHandler( FOOTPRINT_WIZARD_FRAME::OnGridSize ),
                              NULL, this );
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

    m_parameterGrid->ClearGrid();
    m_parameterGridPage = m_pageList->GetSelection();

    if( m_parameterGridPage < 0 )   // Should not happen
        return;

    // Get the list of names, values, types, hints and designators
    wxArrayString designatorsList    = footprintWizard->GetParameterDesignators( m_parameterGridPage );
    wxArrayString namesList          = footprintWizard->GetParameterNames( m_parameterGridPage );
    wxArrayString valuesList         = footprintWizard->GetParameterValues( m_parameterGridPage );
    wxArrayString typesList          = footprintWizard->GetParameterTypes( m_parameterGridPage );
    wxArrayString hintsList          = footprintWizard->GetParameterHints( m_parameterGridPage );

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

        // Boolean parameters are displayed using a checkbox
        if( units == WIZARD_PARAM_UNITS_BOOL )
        {
            // Set to ReadOnly as we delegate interactivity to GRID_TRICKS
            m_parameterGrid->SetReadOnly( i, WIZ_COL_VALUE );
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

        // Set the 'Value'
        m_parameterGrid->SetCellValue( i, WIZ_COL_VALUE, value );
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


#define AUI_PERSPECTIVE_KEY  wxT( "Fpwizard_auiPerspective" )


void FOOTPRINT_WIZARD_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );

    aCfg->Read( AUI_PERSPECTIVE_KEY, &m_auiPerspective );
}


void FOOTPRINT_WIZARD_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::SaveSettings( aCfg );

    aCfg->Write( AUI_PERSPECTIVE_KEY, m_auimgr.SavePerspective() );
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
    bool keyHandled = GeneralControlKeyMovement( aHotKey, &pos, true );

    switch( aHotKey )
    {
    case WXK_F1:
        cmd.SetId( ID_KEY_ZOOM_IN );
        GetEventHandler()->ProcessEvent( cmd );
        keyHandled = true;
        break;

    case WXK_F2:
        cmd.SetId( ID_KEY_ZOOM_OUT );
        GetEventHandler()->ProcessEvent( cmd );
        keyHandled = true;
        break;

    case WXK_F3:
        cmd.SetId( ID_ZOOM_REDRAW );
        GetEventHandler()->ProcessEvent( cmd );
        keyHandled = true;
        break;

    case WXK_F4:
        cmd.SetId( ID_POPUP_ZOOM_CENTER );
        GetEventHandler()->ProcessEvent( cmd );
        keyHandled = true;
        break;

    case WXK_HOME:
        cmd.SetId( ID_ZOOM_PAGE );
        GetEventHandler()->ProcessEvent( cmd );
        keyHandled = true;
        break;

    case ' ':
        GetScreen()->m_O_Curseur = GetCrossHairPosition();
        keyHandled = true;
        break;

    default:
        break;
    }

    SetCrossHairPosition( pos );
    RefreshCrossHair( oldpos, aPosition, aDC );

    UpdateStatusBar();    // Display new cursor coordinates

    return keyHandled;
}


void FOOTPRINT_WIZARD_FRAME::Show3D_Frame( wxCommandEvent& event )
{
    EDA_3D_VIEWER* draw3DFrame = Get3DViewerFrame();

    // We can probably remove this for 6.0, but just to be safe we'll stick to
    // one 3DFrame at a time for 5.0
    if( draw3DFrame )
        draw3DFrame->Close( true );

    draw3DFrame = new EDA_3D_VIEWER( &Kiway(), this, _( "3D Viewer" ) );
    Update3D_Frame( false );

#ifdef  __WXMAC__
    // A stronger version of Raise() which promotes the window to its parent's level.
    draw3DFrame->ReparentQuasiModal();
#else
    draw3DFrame->Raise();     // Needed with some Window Managers
#endif

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
        // Force 3D screen refresh immediately
        draw3DFrame->NewDisplay( true );
    }
}


void FOOTPRINT_WIZARD_FRAME::ReCreateHToolbar()
{
    wxString msg;

    if( !m_mainToolBar )
    {
        m_mainToolBar = new wxAuiToolBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                          KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

        // Set up toolbar
        m_mainToolBar->AddTool( ID_FOOTPRINT_WIZARD_SELECT_WIZARD, wxEmptyString,
                                KiBitmap( module_wizard_xpm ),
                                _( "Select wizard script to run" ) );

        m_mainToolBar->AddSeparator();

        m_mainToolBar->AddTool( ID_FOOTPRINT_WIZARD_RESET_TO_DEFAULT, wxEmptyString,
                                KiBitmap( reload_xpm ),
                                _( "Reset wizard parameters to default") );

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
        msg = AddHotkeyName( _( "Zoom in" ), g_Module_Editor_Hotkeys_Descr,
                             HK_ZOOM_IN, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString,
                                KiBitmap( zoom_in_xpm ), msg );

        msg = AddHotkeyName( _( "Zoom out" ), g_Module_Editor_Hotkeys_Descr,
                             HK_ZOOM_OUT, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString,
                                KiBitmap( zoom_out_xpm ), msg );

        msg = AddHotkeyName( _( "Redraw view" ), g_Module_Editor_Hotkeys_Descr,
                             HK_ZOOM_REDRAW, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                                KiBitmap( zoom_redraw_xpm ), msg );

        msg = AddHotkeyName( _( "Zoom auto" ), g_Module_Editor_Hotkeys_Descr,
                             HK_ZOOM_AUTO, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString,
                                KiBitmap( zoom_fit_in_page_xpm ), msg );

        // The footprint wizard always can export the current footprint
        m_mainToolBar->AddSeparator();
        m_mainToolBar->AddTool( ID_FOOTPRINT_WIZARD_DONE,
                                wxEmptyString, KiBitmap( export_footprint_names_xpm ),
                                _( "Export footprint to editor" ) );

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


