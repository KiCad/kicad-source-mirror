/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_base_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <board.h>
#include <board_design_settings.h>
#include <pcb_dimension.h>
#include <footprint_info_impl.h>
#include <project.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <widgets/appearance_controls.h>
#include <dialogs/dialog_grid_settings.h>
#include <dialogs/eda_view_switcher.h>
#include <wildcards_and_files_ext.h>
#include <collectors.h>


PCB_BASE_EDIT_FRAME::PCB_BASE_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent,
                                          FRAME_T aFrameType, const wxString& aTitle,
                                          const wxPoint& aPos, const wxSize& aSize, long aStyle,
                                          const wxString& aFrameName ) :
        PCB_BASE_FRAME( aKiway, aParent, aFrameType, aTitle, aPos, aSize, aStyle, aFrameName ),
                        m_rotationAngle( 900 ), m_undoRedoBlocked( false ),
        m_selectionFilterPanel( nullptr ),
        m_appearancePanel( nullptr )
{
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
        GFootprintList.WriteCacheToFile( Prj().GetProjectPath() + "fp-info-cache" );
    }

    // Close the project if we are standalone, so it gets cleaned up properly
    if( mgr->IsProjectOpen() && Kiface().IsSingle() )
        mgr->UnloadProject( &Prj(), false );
}


bool PCB_BASE_EDIT_FRAME::TryBefore( wxEvent& aEvent )
{
    static bool s_switcherShown = false;

    if( !s_switcherShown && wxGetKeyState( WXK_RAW_CONTROL ) && wxGetKeyState( WXK_TAB ) )
    {
        if( m_appearancePanel && this->IsActive() )
        {
            const wxArrayString& mru = m_appearancePanel->GetLayerPresetsMRU();
            EDA_VIEW_SWITCHER    switcher( this, mru );

            s_switcherShown = true;
            switcher.ShowModal();
            s_switcherShown = false;

            int idx = switcher.GetSelection();

            if( idx >= 0 && idx < (int) mru.size() )
                m_appearancePanel->ApplyLayerPreset( mru[idx] );

            return true;
        }
    }

    return PCB_BASE_FRAME::TryBefore( aEvent );
}


void PCB_BASE_EDIT_FRAME::SetRotationAngle( int aRotationAngle )
{
    wxCHECK2_MSG( aRotationAngle > 0 && aRotationAngle <= 900, aRotationAngle = 900,
                  wxT( "Invalid rotation angle, defaulting to 90." ) );

    m_rotationAngle = aRotationAngle;
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
        EDA_UNITS    units = GetUserUnits();
        KIGFX::VIEW* view  = GetCanvas()->GetView();

        INSPECTOR_FUNC inspector =
                [units, view]( EDA_ITEM* aItem, void* aTestData )
                {
                    PCB_DIMENSION_BASE* dimension = static_cast<PCB_DIMENSION_BASE*>( aItem );

                    if( dimension->GetUnitsMode() == DIM_UNITS_MODE::AUTOMATIC )
                    {
                        dimension->SetUnits( units );
                        dimension->Update();
                        view->Update( dimension );
                    }

                    return SEARCH_RESULT::CONTINUE;
                };

        board->Visit( inspector, nullptr, GENERAL_COLLECTOR::Dimensions );
    }

    ReCreateAuxiliaryToolbar();
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


COLOR_SETTINGS* PCB_BASE_EDIT_FRAME::GetColorSettings() const
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
    EDA_DRAW_FRAME::handleActivateEvent( aEvent );

    // The text in the collapsible pane headers need to be updated
    if( m_appearancePanel )
        m_appearancePanel->RefreshCollapsiblePanes();
}


void PCB_BASE_EDIT_FRAME::OnLayerAlphaChanged()
{
    if( m_appearancePanel )
        m_appearancePanel->OnLayerAlphaChanged();
}
