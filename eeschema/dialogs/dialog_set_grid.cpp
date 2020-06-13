/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_set_grid_base.h>
#include <common.h>
#include <tool/tool_manager.h>
#include <settings/app_settings.h>
#include <sch_base_frame.h>
#include <tool/grid_menu.h>

class DIALOG_SET_GRID : public DIALOG_SET_GRID_BASE
{
    SCH_BASE_FRAME* m_frame;

public:
    DIALOG_SET_GRID( SCH_BASE_FRAME* aParent );

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;
};


DIALOG_SET_GRID::DIALOG_SET_GRID( SCH_BASE_FRAME* aParent ):
    DIALOG_SET_GRID_BASE( aParent ),
    m_frame( aParent )
{
    m_sdbSizerOK->SetDefault();

    FinishDialogSettings();
}


bool DIALOG_SET_GRID::TransferDataToWindow()
{
    int           idx = m_frame->config()->m_Window.grid.last_size_idx;
    wxArrayString grids;

    GRID_MENU::BuildChoiceList( &grids, m_frame->config(), GetUserUnits() != EDA_UNITS::INCHES );

    for( const wxString& grid : grids )
        m_choiceGridSize->Append( grid );

    if( idx >= 0 && idx < int( m_choiceGridSize->GetCount() ) )
        m_choiceGridSize->SetSelection( idx );

    return true;
}


bool DIALOG_SET_GRID::TransferDataFromWindow()
{
    int idx = m_choiceGridSize->GetSelection();

    m_frame->GetToolManager()->RunAction( "common.Control.gridPreset", true, idx );

    return true;
}


void SCH_BASE_FRAME::OnGridSettings( wxCommandEvent& aEvent )
{
    DIALOG_SET_GRID dlg( this );

    dlg.ShowModal();

    UpdateStatusBar();
    GetCanvas()->Refresh();
}
