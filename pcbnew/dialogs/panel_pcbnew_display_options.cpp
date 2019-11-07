/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <fctsys.h>
#include <pcbnew.h>
#include <pcb_edit_frame.h>
#include <pcb_display_options.h>
#include <config_map.h>
#include <pcbnew_id.h>
#include <panel_pcbnew_display_options.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_view.h>
#include <pcb_painter.h>
#include <widgets/paged_dialog.h>
#include <widgets/gal_options_panel.h>


static const UTIL::CFG_MAP<PCB_DISPLAY_OPTIONS::TRACE_CLEARANCE_DISPLAY_MODE_T> traceClearanceSelectMap =
{
    { PCB_DISPLAY_OPTIONS::SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS,            2 },     // Default
    { PCB_DISPLAY_OPTIONS::DO_NOT_SHOW_CLEARANCE,                              0 },
    { PCB_DISPLAY_OPTIONS::SHOW_CLEARANCE_NEW_TRACKS,                          1 },
    { PCB_DISPLAY_OPTIONS::SHOW_CLEARANCE_NEW_AND_EDITED_TRACKS_AND_VIA_AREAS, 3 },
    { PCB_DISPLAY_OPTIONS::SHOW_CLEARANCE_ALWAYS,                              4 },
};


PANEL_PCBNEW_DISPLAY_OPTIONS::PANEL_PCBNEW_DISPLAY_OPTIONS( PCB_EDIT_FRAME* aFrame,
                                                            PAGED_DIALOG* aParent ) :
    PANEL_PCBNEW_DISPLAY_OPTIONS_BASE( aParent->GetTreebook() ),
    m_frame( aFrame )
{
    KIGFX::GAL_DISPLAY_OPTIONS& galOptions = m_frame->GetGalDisplayOptions();
    m_galOptsPanel = new GAL_OPTIONS_PANEL( this, galOptions );

    m_galOptionsSizer->Add( m_galOptsPanel, 1, wxEXPAND, 0 );
}


bool PANEL_PCBNEW_DISPLAY_OPTIONS::TransferDataToWindow()
{
    auto& displ_opts = m_frame->GetDisplayOptions();

    m_OptDisplayTracksClearance->SetSelection( UTIL::GetConfigForVal(
            traceClearanceSelectMap, displ_opts.m_ShowTrackClearanceMode ) );

    m_OptDisplayPadClearence->SetValue( displ_opts.m_DisplayPadIsol );
    m_OptDisplayPadNumber->SetValue( displ_opts.m_DisplayPadNum );
    m_OptDisplayPadNoConn->SetValue( m_frame->IsElementVisible( LAYER_NO_CONNECTS ) );
    m_ShowNetNamesOption->SetSelection( displ_opts.m_DisplayNetNamesMode );

    m_galOptsPanel->TransferDataToWindow();

    return true;
}


/*
 * Update variables with new options
 */
bool PANEL_PCBNEW_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    PCB_DISPLAY_OPTIONS displ_opts = m_frame->GetDisplayOptions();

    displ_opts.m_ShowTrackClearanceMode = UTIL::GetValFromConfig(
            traceClearanceSelectMap, m_OptDisplayTracksClearance->GetSelection() );

    displ_opts.m_DisplayPadIsol = m_OptDisplayPadClearence->GetValue();
    displ_opts.m_DisplayPadNum = m_OptDisplayPadNumber->GetValue();

    m_frame->SetElementVisibility( LAYER_NO_CONNECTS, m_OptDisplayPadNoConn->GetValue() );

    displ_opts.m_DisplayNetNamesMode = m_ShowNetNamesOption->GetSelection();

    m_galOptsPanel->TransferDataFromWindow();

    // Apply changes to the GAL
    KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();
    KIGFX::PCB_PAINTER* painter = static_cast<KIGFX::PCB_PAINTER*>( view->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings = painter->GetSettings();

    m_frame->SetDisplayOptions( displ_opts );
    settings->LoadDisplayOptions( displ_opts, m_frame->ShowPageLimits() );
    m_frame->SetElementVisibility( LAYER_RATSNEST, displ_opts.m_ShowGlobalRatsnest );

    view->RecacheAllItems();
    view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );

    return true;
}


