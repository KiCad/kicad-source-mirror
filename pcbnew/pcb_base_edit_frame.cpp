/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2020-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_base_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <pgm_base.h>
#include <board.h>
#include <board_design_settings.h>
#include <pcb_dimension.h>
#include <footprint_info_impl.h>
#include <project.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <widgets/appearance_controls.h>
#include <widgets/pcb_properties_panel.h>
#include <dialogs/eda_view_switcher.h>
#include <wildcards_and_files_ext.h>
#include <collectors.h>
#include <widgets/wx_aui_utils.h>


PCB_BASE_EDIT_FRAME::PCB_BASE_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent,
                                          FRAME_T aFrameType, const wxString& aTitle,
                                          const wxPoint& aPos, const wxSize& aSize, long aStyle,
                                          const wxString& aFrameName ) :
        PCB_BASE_FRAME( aKiway, aParent, aFrameType, aTitle, aPos, aSize, aStyle, aFrameName ),
        m_undoRedoBlocked( false ),
        m_selectionFilterPanel( nullptr ),
        m_appearancePanel( nullptr ),
        m_propertiesPanel( nullptr ),
        m_tabbedPanel( nullptr ),
        m_show_properties( false )
{
    m_darkMode = KIPLATFORM::UI::IsDarkTheme();

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
}


PCB_BASE_EDIT_FRAME::~PCB_BASE_EDIT_FRAME()
{
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
                    switcher.ShowModal();
                    s_presetSwitcherShown = false;

                    int idx = switcher.GetSelection();

                    if( idx >= 0 && idx < (int) mru.size() )
                        m_appearancePanel->ApplyLayerPreset( mru[idx] );

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
                    switcher.ShowModal();
                    s_viewportSwitcherShown = false;

                    int idx = switcher.GetSelection();

                    if( idx >= 0 && idx < (int) mru.size() )
                        m_appearancePanel->ApplyViewport( mru[idx] );

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
    bool new_board = ( aBoard != m_pcb );

    if( new_board )
    {
        if( m_toolManager )
            m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

        GetCanvas()->GetView()->Clear();
        GetCanvas()->GetView()->InitPreview();
    }

    PCB_BASE_FRAME::SetBoard( aBoard, aReporter );

    GetCanvas()->GetGAL()->SetGridOrigin( VECTOR2D( aBoard->GetDesignSettings().GetGridOrigin() ) );

    if( new_board )
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

        if( new_board )
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
    return Pgm().GetSettingsManager().GetColorSettings( GetPcbNewSettings()->m_ColorTheme );
}


wxString PCB_BASE_EDIT_FRAME::GetDesignRulesPath()
{
    if( !GetBoard() )
        return wxEmptyString;

    wxFileName fn = GetBoard()->GetFileName();
    fn.SetExt( DesignRulesFileExtension );
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
}


void PCB_BASE_EDIT_FRAME::UpdateProperties()
{
    if( !m_propertiesPanel || !m_propertiesPanel->IsShownOnScreen() )
        return;

    m_propertiesPanel->UpdateData();
}


void PCB_BASE_EDIT_FRAME::ToggleProperties()
{
    if( !m_propertiesPanel )
        return;

    PCBNEW_SETTINGS* settings = GetPcbNewSettings();

    m_show_properties = !m_show_properties;

    wxAuiPaneInfo& propertiesPaneInfo = m_auimgr.GetPane( "PropertiesManager" );
    propertiesPaneInfo.Show( m_show_properties );

    if( m_show_properties )
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
