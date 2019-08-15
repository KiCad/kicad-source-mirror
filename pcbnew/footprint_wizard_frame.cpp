/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2015 Miguel Angel Ajo Pelayo <miguelangel@nbee.es>
 * Copyright (C) 2012-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiface_i.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_edit_frame.h>
#include <3d_viewer/eda_3d_viewer.h>
#include <msgpanel.h>
#include <macros.h>
#include <bitmaps.h>
#include <grid_tricks.h>
#include <eda_dockart.h>
#include <class_board.h>
#include <class_module.h>
#include <footprint_edit_frame.h>
#include <pcbnew_id.h>
#include "footprint_wizard_frame.h"
#include <footprint_info.h>
#include <wx/tokenzr.h>
#include <wx/numformatter.h>
#include <wildcards_and_files_ext.h>
#include <base_units.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/action_toolbar.h>
#include <tool/common_tools.h>
#include "tools/selection_tool.h"
#include "tools/pcbnew_control.h"
#include "tools/pcb_actions.h"


BEGIN_EVENT_TABLE( FOOTPRINT_WIZARD_FRAME, EDA_DRAW_FRAME )

    // Window events
    EVT_CLOSE( FOOTPRINT_WIZARD_FRAME::OnCloseWindow )
    EVT_SIZE( FOOTPRINT_WIZARD_FRAME::OnSize )
    EVT_ACTIVATE( FOOTPRINT_WIZARD_FRAME::OnActivate )

     // Toolbar events
    EVT_TOOL( ID_FOOTPRINT_WIZARD_SELECT_WIZARD, FOOTPRINT_WIZARD_FRAME::SelectCurrentWizard )
    EVT_TOOL( ID_FOOTPRINT_WIZARD_RESET_TO_DEFAULT, FOOTPRINT_WIZARD_FRAME::DefaultParameters )
    EVT_TOOL( ID_FOOTPRINT_WIZARD_NEXT, FOOTPRINT_WIZARD_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_FOOTPRINT_WIZARD_PREVIOUS, FOOTPRINT_WIZARD_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_FOOTPRINT_WIZARD_DONE, FOOTPRINT_WIZARD_FRAME::ExportSelectedFootprint )

    // listbox events
    EVT_LISTBOX( ID_FOOTPRINT_WIZARD_PAGE_LIST, FOOTPRINT_WIZARD_FRAME::ClickOnPageList )
    EVT_GRID_CMD_CELL_CHANGED( ID_FOOTPRINT_WIZARD_PARAMETER_LIST,
                               FOOTPRINT_WIZARD_FRAME::ParametersUpdated )
END_EVENT_TABLE()


// Note: our FOOTPRINT_WIZARD_FRAME is always modal.

FOOTPRINT_WIZARD_FRAME::FOOTPRINT_WIZARD_FRAME( KIWAY* aKiway, wxWindow* aParent,
                                                FRAME_T aFrameType ) :
        PCB_BASE_FRAME( aKiway, aParent, aFrameType, _( "Footprint Wizard" ),
                        wxDefaultPosition, wxDefaultSize,
                        aParent ? KICAD_DEFAULT_DRAWFRAME_STYLE | wxFRAME_FLOAT_ON_PARENT
                                : KICAD_DEFAULT_DRAWFRAME_STYLE | wxSTAY_ON_TOP,
                        FOOTPRINT_WIZARD_FRAME_NAME ),
        m_wizardListShown( false )
{
    wxASSERT( aFrameType == FRAME_PCB_FOOTPRINT_WIZARD );

    // This frame is always show modal:
    SetModal( true );

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( module_wizard_xpm) );
    SetIcon( icon );

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

    // the footprint wizard frame has no config menu. so use some settings
    // from the caller, or force some options:
    PCB_BASE_FRAME* caller = dynamic_cast<PCB_BASE_FRAME*>( aParent );

    if( caller )
        SetUserUnits( caller->GetUserUnits() );

    auto disp_opts = (PCB_DISPLAY_OPTIONS*)GetDisplayOptions();
    // In viewer, the default net clearance is not known (it depends on the actual board).
    // So we do not show the default clearance, by setting it to 0
    // The footprint or pad specific clearance will be shown
    GetBoard()->GetDesignSettings().GetDefault()->SetClearance(0);

    disp_opts->m_DisplayPadIsol = true;
    disp_opts->m_DisplayPadNum = true;
    GetBoard()->SetElementVisibility( LAYER_NO_CONNECTS, false );

    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    // Create GAL canvas
#ifdef __WXMAC__
    // Cairo renderer doesn't handle Retina displays
    EDA_DRAW_PANEL_GAL::GAL_TYPE backend = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
#else
    EDA_DRAW_PANEL_GAL::GAL_TYPE backend = EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
#endif
    PCB_DRAW_PANEL_GAL* gal_drawPanel = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_FrameSize,
                                                            GetGalDisplayOptions(), backend );
    SetCanvas( gal_drawPanel );

    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetBoard(), gal_drawPanel->GetView(),
                                   gal_drawPanel->GetViewControls(), this );
    m_actions = new PCB_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager, m_actions );
    gal_drawPanel->SetEventDispatcher( m_toolDispatcher );

    m_toolManager->RegisterTool( new PCBNEW_CONTROL );
    m_toolManager->RegisterTool( new SELECTION_TOOL );  // for std context menus (zoom & grid)
    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->InitTools();

    // Run the control tool, it is supposed to be always active
    m_toolManager->InvokeTool( "pcbnew.InteractiveSelection" );

    // Create the toolbars
    ReCreateHToolbar();
    ReCreateVToolbar();

    // Create the parameters panel
    m_parametersPanel = new wxPanel( this, wxID_ANY );

    m_pageList = new wxListBox( m_parametersPanel, ID_FOOTPRINT_WIZARD_PAGE_LIST,
                                wxDefaultPosition, wxDefaultSize, 0, NULL,
                                wxLB_HSCROLL | wxNO_BORDER );

    auto divider = new wxStaticLine( m_parametersPanel, wxID_ANY,
                                     wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );

    m_parameterGrid = new WX_GRID( m_parametersPanel, ID_FOOTPRINT_WIZARD_PARAMETER_LIST );
    initParameterGrid();
    m_parameterGrid->PushEventHandler( new GRID_TRICKS( m_parameterGrid ) );

    ReCreatePageList();

    wxBoxSizer* parametersSizer = new wxBoxSizer( wxHORIZONTAL );
    parametersSizer->Add( m_pageList, 0, wxEXPAND, 5 );
    parametersSizer->Add( divider, 0, wxEXPAND, 5 );
    parametersSizer->Add( m_parameterGrid, 1, wxEXPAND, 5 );
    m_parametersPanel->SetSizer( parametersSizer );
    m_parametersPanel->Layout();

    // Create the build message box
    m_buildMessageBox = new wxTextCtrl( this, wxID_ANY, wxEmptyString,
                                        wxDefaultPosition, wxDefaultSize,
                                        wxTE_MULTILINE | wxTE_READONLY | wxNO_BORDER );

    DisplayWizardInfos();

    m_auimgr.SetManagedWindow( this );
    m_auimgr.SetArtProvider( new EDA_DOCKART() );

    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6)
                      .BestSize( -1, m_MsgFrameHeight ) );

    m_auimgr.AddPane( m_parametersPanel, EDA_PANE().Palette().Name( "Params" ).Left().Position(0)
                      .Caption( _( "Parameters" ) ).MinSize( 360, 180 ) );
    m_auimgr.AddPane( m_buildMessageBox, EDA_PANE().Palette().Name( "Output" ).Left().Position(1)
                      .CaptionVisible( false ).MinSize( 360, -1 ) );

    m_auimgr.AddPane( GetCanvas(), wxAuiPaneInfo().Name( "DrawFrame" ).CentrePane() );

    auto& galOpts = GetGalDisplayOptions();
    galOpts.m_fullscreenCursor = true;
    galOpts.m_forceDisplayCursor = true;
    galOpts.m_axesEnabled = true;

    GetCanvas()->GetView()->SetScale( GetZoomLevelCoeff() / GetScreen()->GetZoom() );
    ActivateGalCanvas();
    updateView();

    SetActiveLayer( F_Cu );
    GetToolManager()->RunAction( ACTIONS::zoomFitScreen, false );

    // Do not Run a dialog here: on some Window Managers, it creates issues.
    // Reason: the FOOTPRINT_WIZARD_FRAME is run as modal;
    // It means the call to FOOTPRINT_WIZARD_FRAME::ShowModal will change the
    // Event Loop Manager, and stop the one created by the dialog.
    // It does not happen on all W.M., perhaps due to the way the order events are called
    // See the call in onActivate instead
}


FOOTPRINT_WIZARD_FRAME::~FOOTPRINT_WIZARD_FRAME()
{
    // Delete the GRID_TRICKS.
    m_parameterGrid->PopEventHandler( true );

    GetCanvas()->StopDrawing();
    // Be sure any event cannot be fired after frame deletion:
    GetCanvas()->SetEvtHandlerEnabled( false );

    // Be sure a active tool (if exists) is desactivated:
    if( m_toolManager )
        m_toolManager->DeactivateTool();

    EDA_3D_VIEWER* draw3DFrame = Get3DViewerFrame();

    if( draw3DFrame )
        draw3DFrame->Destroy();

    // Now this frame can be deleted
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


void FOOTPRINT_WIZARD_FRAME::updateView()
{
    GetCanvas()->UseColorScheme( &Settings().Colors() );
    GetCanvas()->DisplayBoard( GetBoard() );
    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
    m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
    UpdateMsgPanel();
}


void FOOTPRINT_WIZARD_FRAME::UpdateMsgPanel()
{
    BOARD_ITEM* footprint = GetBoard()->GetFirstModule();

    if( footprint )
    {
        MSG_PANEL_ITEMS items;

        footprint->GetMsgPanelInfo( m_userUnits, items );
        SetMsgPanel( items );
    }
    else
        ClearMsgPanel();
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
    GetCanvas()->Refresh();
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
    wxArrayString designatorsList = footprintWizard->GetParameterDesignators( m_parameterGridPage );
    wxArrayString namesList       = footprintWizard->GetParameterNames( m_parameterGridPage );
    wxArrayString valuesList      = footprintWizard->GetParameterValues( m_parameterGridPage );
    wxArrayString typesList       = footprintWizard->GetParameterTypes( m_parameterGridPage );
    wxArrayString hintsList       = footprintWizard->GetParameterHints( m_parameterGridPage );

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
    if( m_pageList->GetSelection() >= 0 )
    {
        ReCreateParameterList();
        GetCanvas()->Refresh();
        DisplayWizardInfos();
    }
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
    // Ensure we do not have old selection:
    if( !event.GetActive() )
        return;

    if( !m_wizardListShown )
    {
        m_wizardListShown = true;
        wxPostEvent( this, wxCommandEvent( wxEVT_TOOL, ID_FOOTPRINT_WIZARD_SELECT_WIZARD ) );
    }
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


/**
 * Function Update3DView
 * must be called after a footprint selection
 * Updates the 3D view and 3D frame title.
 */
void FOOTPRINT_WIZARD_FRAME::Update3DView( bool aForceReload, const wxString* aTitle )
{
    wxString frm3Dtitle;
    frm3Dtitle.Printf( _( "ModView: 3D Viewer [%s]" ), GetChars( m_wizardName ) );
    PCB_BASE_FRAME::Update3DView( aForceReload, &frm3Dtitle );
}


void FOOTPRINT_WIZARD_FRAME::ReCreateHToolbar()
{
    wxString msg;

    if( !m_mainToolBar )
    {
        m_mainToolBar = new ACTION_TOOLBAR( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
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
        m_mainToolBar->Add( ACTIONS::show3DViewer );

        m_mainToolBar->AddSeparator();
        m_mainToolBar->Add( ACTIONS::zoomRedraw );
        m_mainToolBar->Add( ACTIONS::zoomInCenter );
        m_mainToolBar->Add( ACTIONS::zoomOutCenter );
        m_mainToolBar->Add( ACTIONS::zoomFitScreen );

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


