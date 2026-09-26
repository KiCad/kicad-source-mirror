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

#include "panel_drill_chart_options.h"

#include <iterator>
#include <set>

#include <board.h>
#include <board_design_settings.h>
#include <board_tables/generated_table_refresh.h>
#include <confirm.h>
#include <drill/drill_chart_template.h>
#include <drill/drill_symbol_profile.h>
#include <pcb_drill_chart.h>

#include <wx/filedlg.h>
#include <wx/filename.h>


namespace
{

/// Checklist row order. Explicit rather than positional so reordering either list cannot
/// silently rebind a checkbox to a different grouping key
const DRILL_GROUP_KEY g_groupKeyRows[] = { DRILL_GROUP_KEY::SIZE_GRP,   DRILL_GROUP_KEY::SLOT,
                                           DRILL_GROUP_KEY::PLATING,    DRILL_GROUP_KEY::SPAN,
                                           DRILL_GROUP_KEY::OPERATION,  DRILL_GROUP_KEY::HOLE_FUNCTION,
                                           DRILL_GROUP_KEY::PROTECTION, DRILL_GROUP_KEY::POST_MACHINING };

} // namespace


PANEL_DRILL_CHART_OPTIONS::PANEL_DRILL_CHART_OPTIONS( wxWindow* aParent ) :
        PANEL_DRILL_CHART_OPTIONS_BASE( aParent )
{
}


void PANEL_DRILL_CHART_OPTIONS::TransferToWindow( const PCB_GENERATED_TABLE& aTable )
{
    const PCB_DRILL_CHART&    chart = static_cast<const PCB_DRILL_CHART&>( aTable );
    const DRILL_CHART_FILTER& filter = chart.Filter();

    m_filterPlated->SetValue( filter.m_Plated );
    m_filterNonPlated->SetValue( filter.m_NonPlated );
    m_filterVias->SetValue( filter.m_Vias );
    m_filterSlots->SetValue( filter.m_Slots );
    m_filterBackdrills->SetValue( filter.m_Backdrills );
    m_filterCastellated->SetValue( filter.m_Castellated );

    m_showTotals->SetValue( chart.GetShowTotals() );

    if( const BOARD* board = chart.GetBoard() )
    {
        const DRILL_SYMBOL_PROFILE& profile = board->GetDesignSettings().GetDrillSymbolProfile();

        for( size_t row = 0; row < std::size( g_groupKeyRows ); ++row )
            m_groupByList->Check( row, profile.IsGroupedBy( g_groupKeyRows[row] ) );
    }
}


bool PANEL_DRILL_CHART_OPTIONS::TransferFromWindow( PCB_GENERATED_TABLE& aTable, GENERATED_TABLE_REFRESH& aRefresh )
{
    PCB_DRILL_CHART&    chart = static_cast<PCB_DRILL_CHART&>( aTable );
    DRILL_CHART_FILTER& filter = chart.Filter();

    filter.m_Plated = m_filterPlated->GetValue();
    filter.m_NonPlated = m_filterNonPlated->GetValue();
    filter.m_Vias = m_filterVias->GetValue();
    filter.m_Slots = m_filterSlots->GetValue();
    filter.m_Backdrills = m_filterBackdrills->GetValue();
    filter.m_Castellated = m_filterCastellated->GetValue();

    chart.SetShowTotals( m_showTotals->GetValue() );

    DRILL_SYMBOL_PROFILE&           profile = aRefresh.Pending<DRILL_PROFILE_PENDING>().m_Profile;
    const std::set<DRILL_GROUP_KEY> wasGroupedBy = profile.GroupKeys();

    for( size_t row = 0; row < std::size( g_groupKeyRows ); ++row )
        profile.SetGroupedBy( g_groupKeyRows[row], m_groupByList->IsChecked( row ) );

    if( profile.GroupKeys() != wasGroupedBy && chart.GetBoard() )
    {
        // Grouping is board-wide, so every other chart and map has to be rebuilt against it.
        // Only a cache key, so an OK that later fails costs one needless rebuild and nothing else
        chart.GetBoard()->BumpDrillModelGeneration();
    }

    return true;
}


void PANEL_DRILL_CHART_OPTIONS::onImportTemplate( wxCommandEvent& aEvent )
{
    wxFileDialog dlg( this, _( "Import Drill Chart Template" ), wxEmptyString, wxEmptyString,
                      _( "Drill chart templates" ) + wxT( " (*.json)|*.json" ), wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() != wxID_OK )
        return;

    DRILL_CHART_TEMPLATE tmpl;
    wxString             error;

    if( !tmpl.LoadFromFile( dlg.GetPath(), &error ) )
    {
        DisplayError( this, error );
        return;
    }

    GENERATED_TABLE_FORMAT format;
    format.m_Columns = tmpl.Columns();
    format.m_Units = tmpl.GetUnits();
    format.m_Precision = tmpl.GetPrecision();

    m_SetFormat( format );
    m_showTotals->SetValue( tmpl.GetShowTotals() );
}


void PANEL_DRILL_CHART_OPTIONS::onExportTemplate( wxCommandEvent& aEvent )
{
    std::optional<GENERATED_TABLE_FORMAT> format = m_GetFormat();

    if( !format )
        return;

    wxFileDialog dlg( this, _( "Export Drill Chart Template" ), wxEmptyString, wxT( "drill_chart" ),
                      _( "Drill chart templates" ) + wxT( " (*.json)|*.json" ), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() != wxID_OK )
        return;

    DRILL_CHART_TEMPLATE tmpl;

    // The file names the template, so whoever opens the json can tell which one it is
    tmpl.SetName( wxFileName( dlg.GetPath() ).GetName() );
    tmpl.SetUnits( format->m_Units );
    tmpl.SetPrecision( format->m_Precision );
    tmpl.SetShowTotals( m_showTotals->GetValue() );
    tmpl.Columns() = format->m_Columns;

    wxString error;

    if( !tmpl.SaveToFile( dlg.GetPath(), &error ) )
        DisplayError( this, error );
}
