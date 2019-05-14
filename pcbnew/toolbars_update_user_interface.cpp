/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2012-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <pcb_edit_frame.h>
#include <dialog_helpers.h>
#include <class_board.h>
#include <pcbnew.h>
#include <pcbnew_id.h>
#include <tool/actions.h>
#include <drc.h>
#include <pcb_layer_box_selector.h>


void PCB_EDIT_FRAME::OnUpdateLayerPair( wxUpdateUIEvent& aEvent )
{
    PrepareLayerIndicator();
}


void PCB_EDIT_FRAME::OnUpdateSelectTrackWidth( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetId() == ID_AUX_TOOLBAR_PCB_TRACK_WIDTH )
    {
        if( m_SelTrackWidthBox->GetSelection() != (int) GetDesignSettings().GetTrackWidthIndex() )
            m_SelTrackWidthBox->SetSelection( GetDesignSettings().GetTrackWidthIndex() );
    }
}


void PCB_EDIT_FRAME::OnUpdateSelectViaSize( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetId() == ID_AUX_TOOLBAR_PCB_VIA_SIZE )
    {
        if( m_SelViaSizeBox->GetSelection() != (int) GetDesignSettings().GetViaSizeIndex() )
            m_SelViaSizeBox->SetSelection( GetDesignSettings().GetViaSizeIndex() );
    }
}


void PCB_EDIT_FRAME::OnUpdateLayerSelectBox( wxUpdateUIEvent& aEvent )
{
    m_SelLayerBox->SetLayerSelection( GetActiveLayer() );
}


#if defined( KICAD_SCRIPTING_WXPYTHON )

// Used only when the DKICAD_SCRIPTING_WXPYTHON option is on
void PCB_EDIT_FRAME::OnUpdateScriptingConsoleState( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetEventObject() != m_mainToolBar )
        return;

    wxMiniFrame* pythonPanelFrame = (wxMiniFrame *) findPythonConsole();
    bool pythonPanelShown = pythonPanelFrame ? pythonPanelFrame->IsShown() : false;
    aEvent.Check( pythonPanelShown );
}

#endif


void PCB_EDIT_FRAME::OnUpdateDrcEnable( wxUpdateUIEvent& aEvent )
{
    bool state = !Settings().m_legacyDrcOn;
    aEvent.Check( state );
    m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_DRC_OFF,
                                        Settings().m_legacyDrcOn ?
                                        _( "Disable design rule checking while routing/editing tracks using Legacy Toolset.\nUse Route > Interactive Router Settings... for Modern Toolset." ) :
                                        _( "Enable design rule checking while routing/editing tracks using Legacy Toolset.\nUse Route > Interactive Router Settings... for Modern Toolset." ) );
}

void PCB_EDIT_FRAME::OnUpdateHighContrastDisplayMode( wxUpdateUIEvent& aEvent )
{
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)GetDisplayOptions();
    aEvent.Check( displ_opts->m_ContrastModeDisplay );
    m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE,
                                        displ_opts->m_ContrastModeDisplay ?
                                        _( "Normal contrast display mode" ) :
                                        _( "High contrast display mode" ) );
}

bool PCB_EDIT_FRAME::LayerManagerShown()
{
    return m_auimgr.GetPane( "LayersManager" ).IsShown();
}

bool PCB_EDIT_FRAME::MicrowaveToolbarShown()
{
    return m_auimgr.GetPane( "MicrowaveToolbar" ).IsShown();
}


void PCB_EDIT_FRAME::OnUpdateSave( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( GetScreen()->IsModify() );
}


void PCB_EDIT_FRAME::OnUpdateVerticalToolbar( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetEventObject() == m_drawToolBar || aEvent.GetEventObject() == m_mainToolBar )
        aEvent.Check( GetToolId() == aEvent.GetId() );
}

void PCB_EDIT_FRAME::OnUpdateMuWaveToolbar( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetEventObject() == m_microWaveToolBar )
        aEvent.Check( GetToolId() == aEvent.GetId() );
}


void PCB_EDIT_FRAME::SyncMenusAndToolbars()
{
    PCB_DISPLAY_OPTIONS* opts = (PCB_DISPLAY_OPTIONS*) GetDisplayOptions();
    int                  zoneMode = opts->m_DisplayZonesMode;

    m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_ZONES, zoneMode == 0 );
    m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_ZONES_DISABLE, zoneMode == 1 );
    m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_ZONES_OUTLINES_ONLY, zoneMode == 2 );
    m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_TRACKS_SKETCH, !opts->m_DisplayPcbTrackFill );
    m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_VIAS_SKETCH, !opts->m_DisplayViaFill );
    m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH, !opts->m_DisplayPadFill );

    m_optionsToolBar->Toggle( ACTIONS::toggleGrid, IsGridVisible() );
    m_optionsToolBar->Toggle( ACTIONS::metricUnits, GetUserUnits() != INCHES );
    m_optionsToolBar->Toggle( ACTIONS::imperialUnits, GetUserUnits() == INCHES );

    m_optionsToolBar->Refresh();
}
