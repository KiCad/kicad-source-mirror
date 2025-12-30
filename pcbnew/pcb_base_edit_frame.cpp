/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <kiface_base.h>
#include <kiplatform/ui.h>
#include <background_jobs_monitor.h>
#include <pcb_base_edit_frame.h>
#include <3d_viewer/eda_3d_viewer_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <widgets/panel_selection_filter.h>
#include <pgm_base.h>
#include <board.h>
#include <board_design_settings.h>
#include <gal/graphics_abstraction_layer.h>
#include <pcb_dimension.h>
#include <pcb_layer_box_selector.h>
#include <footprint.h>
#include <footprint_info_impl.h>
#include <layer_pairs.h>
#include <project.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <widgets/appearance_controls.h>
#include <widgets/pcb_properties_panel.h>
#include <widgets/vertex_editor_pane.h>
#include <dialogs/eda_view_switcher.h>
#include <wildcards_and_files_ext.h>

#include <widgets/kistatusbar.h>
#include <widgets/wx_aui_utils.h>
#include <id.h>


PCB_BASE_EDIT_FRAME::PCB_BASE_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent,
                                          FRAME_T aFrameType, const wxString& aTitle,
                                          const wxPoint& aPos, const wxSize& aSize, long aStyle,
                                          const wxString& aFrameName ) :
        PCB_BASE_FRAME( aKiway, aParent, aFrameType, aTitle, aPos, aSize, aStyle, aFrameName ),
        m_undoRedoBlocked( false ),
        m_selectionFilterPanel( nullptr ),
        m_appearancePanel( nullptr ),
        m_vertexEditorPane( nullptr ),
        m_tabbedPanel( nullptr )
{
    m_SelLayerBox = nullptr;
    m_darkMode = KIPLATFORM::UI::IsDarkTheme();

    // Do not register the idle event handler if we are running in headless mode.
    if( !wxApp::GetGUIInstance() )
        return;

    Bind( wxEVT_IDLE,
          [this]( wxIdleEvent& aEvent )
          {
              // Handle cursor adjustments.  While we can get motion and key events through
              // wxWidgets, we can't get modifier-key-up events.
              if( m_toolManager )
              {
                  PCB_SELECTION_TOOL* selTool = m_toolManager->GetTool<PCB_SELECTION_TOOL>();

                  if( selTool )
                      selTool->OnIdle( aEvent );
              }

              if( m_darkMode != KIPLATFORM::UI::IsDarkTheme() )
              {
                  onDarkModeToggle();
                  m_darkMode = KIPLATFORM::UI::IsDarkTheme();
              }
          } );

    Pgm().GetBackgroundJobMonitor().RegisterStatusBar( static_cast<KISTATUSBAR*>( GetStatusBar() ) );
}


PCB_BASE_EDIT_FRAME::~PCB_BASE_EDIT_FRAME()
{
    Pgm().GetBackgroundJobMonitor().UnregisterStatusBar( static_cast<KISTATUSBAR*>( GetStatusBar() ) );
    CloseVertexEditor();
    GetCanvas()->GetView()->Clear();
}


void PCB_BASE_EDIT_FRAME::doCloseWindow()
{
    SETTINGS_MANAGER* mgr = GetSettingsManager();
    wxFileName projectName( Prj().GetProjectFullName() );

    if( mgr->IsProjectOpen() && wxFileName::IsDirWritable( projectName.GetPath() )
            && projectName.Exists() )
    {
        GFootprintList.WriteCacheToFile( Prj().GetProjectPath() + wxT( "fp-info-cache" ) );
    }

    // Close the project if we are standalone, so it gets cleaned up properly
    if( mgr->IsProjectOpen() && Kiface().IsSingle() )
        mgr->UnloadProject( &Prj(), false );
}


bool PCB_BASE_EDIT_FRAME::TryBefore( wxEvent& aEvent )
{
    static bool s_presetSwitcherShown = false;
    static bool s_viewportSwitcherShown = false;

    // wxWidgets generates no key events for the tab key when the ctrl key is held down.  One
    // way around this is to look at all events and inspect the keyboard state of the tab key.
    // However, this runs into issues on some linux VMs where querying the keyboard state is
    // very slow.  Fortunately we only use ctrl-tab on Mac, so we implement this lovely hack:
#ifdef __WXMAC__
    if( wxGetKeyState( WXK_TAB ) )
#else
    if( ( aEvent.GetEventType() == wxEVT_CHAR || aEvent.GetEventType() == wxEVT_CHAR_HOOK )
            && static_cast<wxKeyEvent&>( aEvent ).GetKeyCode() == WXK_TAB )
#endif
    {
        if( !s_presetSwitcherShown && wxGetKeyState( PRESET_SWITCH_KEY ) )
        {
            if( m_appearancePanel && this->IsActive() )
            {
                const wxArrayString& mru = m_appearancePanel->GetLayerPresetsMRU();

                if( mru.size() > 0 )
                {
                    EDA_VIEW_SWITCHER switcher( this, mru, PRESET_SWITCH_KEY );

                    s_presetSwitcherShown = true;
                    const int switcherDialogRet = switcher.ShowModal();
                    s_presetSwitcherShown = false;

                    if( switcherDialogRet == wxID_OK )
                    {
                        int idx = switcher.GetSelection();

                        if( idx >= 0 && idx < (int) mru.size() )
                            m_appearancePanel->ApplyLayerPreset( mru[idx] );
                    }

                    return true;
                }
            }
        }
        else if( !s_viewportSwitcherShown && wxGetKeyState( VIEWPORT_SWITCH_KEY ) )
        {
            if( m_appearancePanel && this->IsActive() )
            {
                const wxArrayString& mru = m_appearancePanel->GetViewportsMRU();

                if( mru.size() > 0 )
                {
                    EDA_VIEW_SWITCHER switcher( this, mru, VIEWPORT_SWITCH_KEY );

                    s_viewportSwitcherShown = true;
                    const int switcherDialogRet = switcher.ShowModal();
                    s_viewportSwitcherShown = false;

                    if( switcherDialogRet == wxID_OK )
                    {
                        int idx = switcher.GetSelection();

                        if( idx >= 0 && idx < (int) mru.size() )
                            m_appearancePanel->ApplyViewport( mru[idx] );
                    }

                    return true;
                }
            }
        }
    }

    return PCB_BASE_FRAME::TryBefore( aEvent );
}


EDA_ANGLE PCB_BASE_EDIT_FRAME::GetRotationAngle() const
{
    // Return a default angle (90 degrees) used for rotate operations.
    return ANGLE_90;
}


void PCB_BASE_EDIT_FRAME::ActivateGalCanvas()
{
    PCB_BASE_FRAME::ActivateGalCanvas();

    GetCanvas()->SyncLayersVisibility( m_pcb );
}


void PCB_BASE_EDIT_FRAME::SetBoard( BOARD* aBoard, PROGRESS_REPORTER* aReporter )
{
    bool is_new_board = ( aBoard != m_pcb );

    if( is_new_board )
    {
        if( m_toolManager )
            m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

        wxCommandEvent e( EDA_EVT_BOARD_CHANGING );
        ProcessEventLocally( e );

        GetCanvas()->GetView()->Clear();
        GetCanvas()->GetView()->InitPreview();
    }

    PCB_BASE_FRAME::SetBoard( aBoard, aReporter );

    GetCanvas()->GetGAL()->SetGridOrigin( VECTOR2D( aBoard->GetDesignSettings().GetGridOrigin() ) );

    if( is_new_board )
    {
        BOARD_DESIGN_SETTINGS& bds = aBoard->GetDesignSettings();
        bds.m_DRCEngine            = std::make_shared<DRC_ENGINE>( aBoard, &bds );
    }

    // update the tool manager with the new board and its view.
    if( m_toolManager )
    {
        GetCanvas()->DisplayBoard( aBoard, aReporter );

        GetCanvas()->UpdateColors();
        m_toolManager->SetEnvironment( aBoard, GetCanvas()->GetView(),
                                       GetCanvas()->GetViewControls(), config(), this );

        if( is_new_board )
            m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
    }
}


void PCB_BASE_EDIT_FRAME::unitsChangeRefresh()
{
    PCB_BASE_FRAME::unitsChangeRefresh();

    if( BOARD* board = GetBoard() )
    {
        board->UpdateUserUnits( board, GetCanvas()->GetView() );
        m_toolManager->PostEvent( EVENTS::SelectedItemsModified );
    }

    ReCreateAuxiliaryToolbar();
    UpdateProperties();
}


void PCB_BASE_EDIT_FRAME::SetGridVisibility( bool aVisible )
{
    PCB_BASE_FRAME::SetGridVisibility( aVisible );

    // Update the grid checkbox in the layer widget
    if( m_appearancePanel )
        m_appearancePanel->SetObjectVisible( LAYER_GRID, aVisible );
}


void PCB_BASE_EDIT_FRAME::SetObjectVisible( GAL_LAYER_ID aLayer, bool aVisible )
{
    if( m_appearancePanel )
        m_appearancePanel->SetObjectVisible( aLayer, aVisible );
}


COLOR_SETTINGS* PCB_BASE_EDIT_FRAME::GetColorSettings( bool aForceRefresh ) const
{
    PCBNEW_SETTINGS* cfg = GetPcbNewSettings();
    return ::GetColorSettings( cfg ? cfg->m_ColorTheme : DEFAULT_THEME );
}


wxString PCB_BASE_EDIT_FRAME::GetDesignRulesPath()
{
    if( !GetBoard() )
        return wxEmptyString;

    wxFileName fn = GetBoard()->GetFileName();
    fn.SetExt( FILEEXT::DesignRulesFileExtension );
    return Prj().AbsolutePath( fn.GetFullName() );
}


void PCB_BASE_EDIT_FRAME::handleActivateEvent( wxActivateEvent& aEvent )
{
    PCB_BASE_FRAME::handleActivateEvent( aEvent );

    // The text in the collapsible pane headers need to be updated
    if( m_appearancePanel )
        m_appearancePanel->RefreshCollapsiblePanes();
}


void PCB_BASE_EDIT_FRAME::onDarkModeToggle()
{
    m_appearancePanel->OnDarkModeToggle();

    EDA_3D_VIEWER_FRAME* viewer = Get3DViewerFrame();

    if( viewer )
        viewer->OnDarkModeToggle();
}


void PCB_BASE_EDIT_FRAME::ToggleProperties()
{
    if( !m_propertiesPanel )
        return;

    bool show = !m_propertiesPanel->IsShownOnScreen();

    wxAuiPaneInfo& propertiesPaneInfo = m_auimgr.GetPane( PropertiesPaneName() );
    propertiesPaneInfo.Show( show );

    PCBNEW_SETTINGS* settings = GetPcbNewSettings();

    if( show )
    {
        SetAuiPaneSize( m_auimgr, propertiesPaneInfo,
                        settings->m_AuiPanels.properties_panel_width, -1 );
    }
    else
    {
        settings->m_AuiPanels.properties_panel_width = m_propertiesPanel->GetSize().x;
        m_auimgr.Update();
    }
}


void PCB_BASE_EDIT_FRAME::GetContextualTextVars( BOARD_ITEM* aSourceItem, const wxString& aCrossRef,
                                                 wxArrayString*  aTokens )
{
    BOARD* board = aSourceItem->GetBoard();

    if( !aCrossRef.IsEmpty() )
    {
        for( FOOTPRINT* candidate : board->Footprints() )
        {
            if( candidate->GetReference() == aCrossRef )
            {
                candidate->GetContextualTextVars( aTokens );
                break;
            }
        }
    }
    else
    {
        board->GetContextualTextVars( aTokens );

        if( FOOTPRINT* footprint = aSourceItem->GetParentFootprint() )
            footprint->GetContextualTextVars( aTokens );
    }
}


void PCB_BASE_EDIT_FRAME::configureToolbars()
{
    // Load the toolbar configuration and base controls
    PCB_BASE_FRAME::configureToolbars();

    // Layer selector
    auto layerSelectorFactory =
            [this]( ACTION_TOOLBAR* aToolbar )
            {
                if( !m_SelLayerBox )
                {
                    m_SelLayerBox = new PCB_LAYER_BOX_SELECTOR( aToolbar, ID_ON_LAYER_SELECT );
                    m_SelLayerBox->SetBoardFrame( this );
                }

                m_SelLayerBox->SetToolTip( _( "+/- to switch" ) );
                m_SelLayerBox->Resync();

                aToolbar->Add( m_SelLayerBox );

                // UI update handler for the control
                aToolbar->Bind( wxEVT_UPDATE_UI,
                                [this]( wxUpdateUIEvent& aEvent )
                                    {
                                        if( m_SelLayerBox->GetCount()
                                            && ( m_SelLayerBox->GetLayerSelection() != GetActiveLayer() ) )
                                        {
                                            m_SelLayerBox->SetLayerSelection( GetActiveLayer() );
                                        }
                                    },
                                m_SelLayerBox->GetId() );

                // Event handler to respond to the user interacting with the control
                aToolbar->Bind( wxEVT_COMBOBOX,
                                [this]( wxCommandEvent& aEvent )
                                    {
                                        SetActiveLayer( ToLAYER_ID( m_SelLayerBox->GetLayerSelection() ) );

                                        if( GetDisplayOptions().m_ContrastModeDisplay != HIGH_CONTRAST_MODE::NORMAL )
                                            GetCanvas()->Refresh();
                                    },
                                m_SelLayerBox->GetId() );
            };

    RegisterCustomToolbarControlFactory( ACTION_TOOLBAR_CONTROLS::layerSelector, layerSelectorFactory );
}


void PCB_BASE_EDIT_FRAME::ClearToolbarControl( int aId )
{
    PCB_BASE_FRAME::ClearToolbarControl( aId );

    switch( aId )
    {
    case ID_ON_LAYER_SELECT: m_SelLayerBox = nullptr; break;
    }
}


void PCB_BASE_EDIT_FRAME::HighlightSelectionFilter( const PCB_SELECTION_FILTER_OPTIONS& aOptions )
{
    PCB_SELECTION_FILTER_EVENT evt( aOptions );
    wxPostEvent( this, evt );
}

wxString PCB_BASE_EDIT_FRAME::VertexEditorPaneName()
{
    return wxS( "VertexEditor" );
}

void PCB_BASE_EDIT_FRAME::OpenVertexEditor( BOARD_ITEM* aItem )
{
    if( !m_vertexEditorPane )
    {
        m_vertexEditorPane = new PCB_VERTEX_EDITOR_PANE( this );

        wxAuiPaneInfo paneInfo = EDA_PANE().Name( VertexEditorPaneName() )
                                       .Float()
                                       .Caption( _( "Edit Vertices" ) )
                                       .PaneBorder( true )
                                       .CloseButton( true )
                                       .DestroyOnClose( true )
                                       .Resizable( true )
                                       .MinSize( FromDIP( wxSize( 260, 200 ) ) )
                                       .BestSize( FromDIP( wxSize( 260, 320 ) ) )
                                       .FloatingSize( FromDIP( wxSize( 320, 360 ) ) );

        m_auimgr.AddPane( m_vertexEditorPane, paneInfo );
        m_auimgr.Update();
    }

    m_vertexEditorPane->SetItem( aItem );
    m_vertexEditorPane->SetFocus();
}

void PCB_BASE_EDIT_FRAME::CloseVertexEditor()
{
    if( m_vertexEditorPane )
        m_vertexEditorPane->ClearItem();
}

void PCB_BASE_EDIT_FRAME::UpdateVertexEditorSelection( BOARD_ITEM* aItem )
{
    if( m_vertexEditorPane )
        m_vertexEditorPane->OnSelectionChanged( aItem );
}

void PCB_BASE_EDIT_FRAME::OnVertexEditorPaneClosed( PCB_VERTEX_EDITOR_PANE* aPane )
{
    if( m_vertexEditorPane == aPane )
        m_vertexEditorPane = nullptr;
}
