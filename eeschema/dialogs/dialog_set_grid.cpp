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
#include <sch_base_frame.h>
#include <class_draw_panel_gal.h>
#include <sch_view.h>
#include <gal/graphics_abstraction_layer.h>


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
    const GRIDS& gridSizes = m_frame->GetScreen()->GetGrids();

    for( size_t i = 0; i < gridSizes.size(); i++ )
    {
        m_choiceGridSize->Append( wxString::Format( "%0.1f",
                static_cast<float>( Iu2Mils( gridSizes[i].m_Size.x ) ) ) );

        if( gridSizes[i].m_CmdId == m_frame->GetScreen()->GetGridCmdId() )
            m_choiceGridSize->SetSelection( (int) i );
    }

    return true;
}


bool DIALOG_SET_GRID::TransferDataFromWindow()
{
    const GRIDS& gridSizes = m_frame->GetScreen()->GetGrids();
    wxRealPoint gridSize = gridSizes[ (size_t) m_choiceGridSize->GetSelection() ].m_Size;
    m_frame->SetLastGridSizeId( m_frame->GetScreen()->SetGrid( gridSize ) );

    m_frame->GetCanvas()->GetView()->GetGAL()->SetGridSize( VECTOR2D( gridSize ) );
    m_frame->GetCanvas()->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );

    return true;
}


void SCH_BASE_FRAME::OnGridSettings( wxCommandEvent& aEvent )
{
    DIALOG_SET_GRID dlg( this );

    dlg.ShowModal();

    UpdateStatusBar();
    GetCanvas()->Refresh();
}
