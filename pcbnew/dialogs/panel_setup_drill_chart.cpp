/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "panel_setup_drill_chart.h"

#include <board.h>
#include <board_design_settings.h>
#include <pcb_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <widgets/paged_dialog.h>


namespace
{

/// Checklist row order. Explicit rather than positional so reordering either list cannot
/// silently rebind a checkbox to a different grouping key
const DRILL_GROUP_KEY g_groupKeyRows[] = {
    DRILL_GROUP_KEY::SIZE,          DRILL_GROUP_KEY::SLOT,
    DRILL_GROUP_KEY::PLATING,       DRILL_GROUP_KEY::SPAN,
    DRILL_GROUP_KEY::OPERATION,     DRILL_GROUP_KEY::HOLE_FUNCTION,
    DRILL_GROUP_KEY::PROTECTION,
    DRILL_GROUP_KEY::POST_MACHINING
};

} // namespace


PANEL_SETUP_DRILL_CHART::PANEL_SETUP_DRILL_CHART( wxWindow* aParentWindow,
                                                  PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_DRILL_CHART_BASE( aParentWindow ),
        m_frame( aFrame ),
        m_symbolSize( aFrame, m_symbolSizeLabel, m_symbolSizeCtrl, m_symbolSizeUnits ),
        m_symbolWidth( aFrame, m_symbolWidthLabel, m_symbolWidthCtrl, m_symbolWidthUnits )
{
}


void PANEL_SETUP_DRILL_CHART::loadSettings( const BOARD_DESIGN_SETTINGS& aSettings )
{
    const DRILL_SYMBOL_PROFILE& profile = aSettings.GetDrillSymbolProfile();

    for( size_t row = 0; row < std::size( g_groupKeyRows ); ++row )
        m_groupByList->Check( row, profile.IsGroupedBy( g_groupKeyRows[row] ) );

    m_markPolicyCtrl->SetSelection( static_cast<int>( profile.GetMarkPolicy() ) );
    m_symbolSize.SetValue( profile.GetSymbolSize() );
    m_symbolWidth.SetValue( profile.GetSymbolWidth() );
    m_freezeAssignments->SetValue( profile.GetFreezeAssignments() );

}


bool PANEL_SETUP_DRILL_CHART::TransferDataToWindow()
{
    loadSettings( m_frame->GetBoard()->GetDesignSettings() );

    return true;
}


bool PANEL_SETUP_DRILL_CHART::TransferDataFromWindow()
{
    BOARD_DESIGN_SETTINGS& bds = m_frame->GetBoard()->GetDesignSettings();
    DRILL_SYMBOL_PROFILE&  profile = bds.GetDrillSymbolProfile();

    for( size_t row = 0; row < std::size( g_groupKeyRows ); ++row )
        profile.SetGroupedBy( g_groupKeyRows[row], m_groupByList->IsChecked( row ) );

    profile.SetMarkPolicy( static_cast<DRILL_MARK_POLICY>( m_markPolicyCtrl->GetSelection() ) );
    profile.SetSymbolSize( m_symbolSize.GetIntValue() );
    profile.SetSymbolWidth( m_symbolWidth.GetIntValue() );
    profile.SetFreezeAssignments( m_freezeAssignments->GetValue() );


    // Grouping drives both the chart rows and the symbols, so every cached answer keyed on
    // the profile has to be given a reason to notice

    m_frame->GetBoard()->BumpDrillModelGeneration();

    return true;
}


void PANEL_SETUP_DRILL_CHART::ImportSettingsFrom( BOARD* aBoard )
{
    loadSettings( aBoard->GetDesignSettings() );
}



void PANEL_SETUP_DRILL_CHART::onEditGroups( wxCommandEvent& aEvent )
{
    PAGED_DIALOG* dialog = PAGED_DIALOG::GetDialog( this );

    if( !dialog )
        return;

    // Through the OK button, not EndModal, so every other page still gets its
    // TransferDataFromWindow
    wxCommandEvent okEvent( wxEVT_BUTTON, wxID_OK );
    dialog->GetEventHandler()->ProcessEvent( okEvent );

    // A page that refused to validate leaves the dialog open, and the user is still editing
    if( dialog->IsShown() )
        return;

    m_frame->GetToolManager()->PostAction( PCB_ACTIONS::showDrillGroups );
}
