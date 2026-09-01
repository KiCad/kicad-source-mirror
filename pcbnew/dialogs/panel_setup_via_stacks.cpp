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

#include <algorithm>

#include <wx/textdlg.h>

#include <confirm.h>
#include <board.h>
#include <pcb_edit_frame.h>
#include <widgets/wx_grid.h>

#include <dialogs/panel_setup_via_stacks.h>
#include <dialogs/dialog_microvia_stack.h>
#include <generators/pcb_via_stack.h>


PANEL_SETUP_VIA_STACKS::PANEL_SETUP_VIA_STACKS( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_VIA_STACKS_BASE( aParentWindow ),
        m_frame( aFrame )
{
    m_grid->AppendCols( 6 );
    m_grid->SetColLabelValue( 0, _( "Name" ) );
    m_grid->SetColLabelValue( 1, _( "Type" ) );
    m_grid->SetColLabelValue( 2, _( "Layers" ) );
    m_grid->SetColLabelValue( 3, _( "Diameter / Hole" ) );
    m_grid->SetColLabelValue( 4, _( "Fill" ) );
    m_grid->SetColLabelValue( 5, _( "Pitch" ) );
    // Only the name is editable in place, the rest needs the dialog.
    m_grid->EnableEditing( true );

    m_grid->Bind( wxEVT_GRID_CELL_CHANGED, &PANEL_SETUP_VIA_STACKS::onCellChanged, this );
    m_addButton->Bind( wxEVT_BUTTON, &PANEL_SETUP_VIA_STACKS::onAdd, this );
    m_editButton->Bind( wxEVT_BUTTON, &PANEL_SETUP_VIA_STACKS::onEdit, this );
    m_removeButton->Bind( wxEVT_BUTTON, &PANEL_SETUP_VIA_STACKS::onRemove, this );
}


void PANEL_SETUP_VIA_STACKS::rebuildGrid()
{
    int cursor = m_grid->GetGridCursorRow();

    if( m_grid->GetNumberRows() > 0 )
        m_grid->DeleteRows( 0, m_grid->GetNumberRows() );

    m_grid->AppendRows( (int) m_presets.size() );

    for( int row = 0; row < (int) m_presets.size(); ++row )
    {
        const VIA_STACK_PRESET& preset = m_presets[row];

        m_grid->SetCellValue( row, 0, preset.m_Name );
        m_grid->SetCellValue( row, 1, preset.m_Staggered ? _( "Staggered" ) : _( "Stacked" ) );
        BOARD* board = m_frame->GetBoard();

        m_grid->SetCellValue( row, 2,
                              board->GetLayerName( preset.m_StartLayer ) + wxT( " -> " )
                                      + board->GetLayerName( preset.m_EndLayer ) );
        m_grid->SetCellValue( row, 3,
                              preset.m_UseNetclass
                                      ? _( "netclass" )
                                      : m_frame->MessageTextFromValue( preset.m_ViaSize, false ) + wxT( " / " )
                                                + m_frame->MessageTextFromValue( preset.m_ViaDrill ) );
        m_grid->SetCellValue( row, 4, preset.m_Filled ? _( "yes" ) : _( "no" ) );
        m_grid->SetCellValue(
                row, 5, preset.m_Staggered ? m_frame->MessageTextFromValue( preset.m_Pitch ) : wxString( wxS( "-" ) ) );

        for( int col = 1; col < 6; ++col )
            m_grid->SetReadOnly( row, col );
    }

    if( !m_presets.empty() && cursor >= 0 )
        m_grid->SetGridCursor( std::min( cursor, (int) m_presets.size() - 1 ), 0 );
}


bool PANEL_SETUP_VIA_STACKS::editPreset( VIA_STACK_PRESET& aPreset )
{
    PCB_VIA_STACK temp( m_frame->GetBoard(), F_Cu );
    temp.ApplyPreset( aPreset );

    DIALOG_MICROVIA_STACK dlg( m_frame, &temp );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    wxString name = aPreset.m_Name;
    aPreset = temp.ToPreset();
    aPreset.m_Name = name;

    return true;
}


void PANEL_SETUP_VIA_STACKS::onAdd( wxCommandEvent& aEvent )
{
    wxString name;

    while( true )
    {
        wxTextEntryDialog nameDlg( this, _( "Preset name:" ), _( "New Microvia Stack Preset" ), name );

        if( nameDlg.ShowModal() != wxID_OK )
            return;

        name = nameDlg.GetValue();
        name.Trim( true ).Trim( false );

        if( name.IsEmpty() )
            return;

        if( nameIsFree( name, -1 ) )
            break;

        DisplayErrorMessage( this, wxString::Format( _( "A preset named \"%s\" already exists." ), name ) );
    }

    VIA_STACK_PRESET preset;
    preset.m_Name = name;

    if( editPreset( preset ) )
    {
        m_presets.push_back( preset );
        rebuildGrid();
    }
}


// A preset is referenced by name, by the toolbar selector and by every placed stack, so two
// presets sharing one is ambiguous.
bool PANEL_SETUP_VIA_STACKS::nameIsFree( const wxString& aName, int aIgnoreRow ) const
{
    for( int row = 0; row < (int) m_presets.size(); ++row )
    {
        if( row != aIgnoreRow && m_presets[row].m_Name.CmpNoCase( aName ) == 0 )
            return false;
    }

    return true;
}


void PANEL_SETUP_VIA_STACKS::onCellChanged( wxGridEvent& aEvent )
{
    int row = aEvent.GetRow();

    if( aEvent.GetCol() != 0 || row < 0 || row >= (int) m_presets.size() )
        return;

    wxString name = m_grid->GetCellValue( row, 0 );
    name.Trim( true ).Trim( false );

    if( name.IsEmpty() || !nameIsFree( name, row ) )
    {
        if( !name.IsEmpty() )
            DisplayErrorMessage( this, wxString::Format( _( "A preset named \"%s\" already exists." ), name ) );

        m_grid->SetCellValue( row, 0, m_presets[row].m_Name );
        return;
    }

    if( m_presets[row].m_Name == m_activePreset )
        m_activePreset = name;

    m_presets[row].m_Name = name;
    m_grid->SetCellValue( row, 0, name );
}


void PANEL_SETUP_VIA_STACKS::onEdit( wxCommandEvent& aEvent )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    int row = m_grid->GetGridCursorRow();

    if( row < 0 || row >= (int) m_presets.size() )
        return;

    if( editPreset( m_presets[row] ) )
        rebuildGrid();
}


void PANEL_SETUP_VIA_STACKS::onRemove( wxCommandEvent& aEvent )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    int row = m_grid->GetGridCursorRow();

    if( row < 0 || row >= (int) m_presets.size() )
        return;

    m_presets.erase( m_presets.begin() + row );
    rebuildGrid();
}


void PANEL_SETUP_VIA_STACKS::loadFrom( BOARD_DESIGN_SETTINGS& aBds )
{
    m_presets = aBds.m_ViaStackPresets;

    int active = aBds.GetViaStackIndex();

    if( active >= 0 && active < (int) m_presets.size() )
        m_activePreset = m_presets[active].m_Name;
    else
        m_activePreset.Clear();

    rebuildGrid();
}


bool PANEL_SETUP_VIA_STACKS::TransferDataToWindow()
{
    loadFrom( m_frame->GetBoard()->GetDesignSettings() );
    return true;
}


void PANEL_SETUP_VIA_STACKS::ImportSettingsFrom( BOARD* aBoard )
{
    loadFrom( aBoard->GetDesignSettings() );
}


bool PANEL_SETUP_VIA_STACKS::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    BOARD_DESIGN_SETTINGS& bds = m_frame->GetBoard()->GetDesignSettings();

    bds.m_ViaStackPresets = m_presets;

    // Adding or removing a preset shifts the others, so follow the selection by name.
    int active = 0;

    for( int row = 0; row < (int) m_presets.size(); ++row )
    {
        if( m_presets[row].m_Name == m_activePreset )
        {
            active = row;
            break;
        }
    }

    bds.SetViaStackIndex( active );
    return true;
}
