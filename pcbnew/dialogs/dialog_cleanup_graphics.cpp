/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_cleanup_graphics.h>
#include <board_commit.h>
#include <footprint.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <graphics_cleaner.h>
#include <pcb_base_frame.h>


DIALOG_CLEANUP_GRAPHICS::DIALOG_CLEANUP_GRAPHICS( PCB_BASE_FRAME* aParent,
                                                  bool aIsFootprintEditor ) :
        DIALOG_CLEANUP_GRAPHICS_BASE( aParent ),
        m_parentFrame( aParent ),
        m_isFootprintEditor( aIsFootprintEditor )
{
    m_changesTreeModel = new RC_TREE_MODEL( m_parentFrame, m_changesDataView );
    m_changesDataView->AssociateModel( m_changesTreeModel );

    m_changesTreeModel->SetSeverities( RPT_SEVERITY_ACTION );

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizerOK->SetLabel( aIsFootprintEditor ? _( "Update Footprint" ) : _( "Update PCB" ) );

    m_sdbSizerOK->SetDefault();
    GetSizer()->SetSizeHints(this);
    Centre();
}


DIALOG_CLEANUP_GRAPHICS::~DIALOG_CLEANUP_GRAPHICS()
{
    m_changesTreeModel->DecRef();
}


void DIALOG_CLEANUP_GRAPHICS::OnCheckBox( wxCommandEvent& anEvent )
{
    doCleanup( true );
}


bool DIALOG_CLEANUP_GRAPHICS::TransferDataToWindow()
{
    doCleanup( true );

    return true;
}


bool DIALOG_CLEANUP_GRAPHICS::TransferDataFromWindow()
{
    doCleanup( false );

    return true;
}


void DIALOG_CLEANUP_GRAPHICS::doCleanup( bool aDryRun )
{
    wxBusyCursor busy;

    BOARD_COMMIT     commit( m_parentFrame );
    BOARD*           board = m_parentFrame->GetBoard();
    FOOTPRINT*       fp = m_isFootprintEditor ? board->GetFirstFootprint() : nullptr;
    GRAPHICS_CLEANER cleaner( fp ? fp->GraphicalItems() : board->Drawings(), fp, commit );

    if( !aDryRun )
    {
        // Clear current selection list to avoid selection of deleted items
        m_parentFrame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

        // ... and to keep the treeModel from trying to refresh a deleted item
        m_changesTreeModel->SetProvider( nullptr );
    }

    m_items.clear();

    // Old model has to be refreshed, GAL normally does not keep updating it
    m_parentFrame->Compile_Ratsnest( false );

    cleaner.CleanupBoard( aDryRun, &m_items, m_createRectanglesOpt->GetValue(),
                                             m_deleteRedundantOpt->GetValue() );

    if( aDryRun )
    {
        RC_ITEMS_PROVIDER* provider = new VECTOR_CLEANUP_ITEMS_PROVIDER( &m_items );
        m_changesTreeModel->SetProvider( provider );
    }
    else if( !commit.Empty() )
    {
        // Clear undo and redo lists to avoid inconsistencies between lists
        commit.Push( _( "Graphics cleanup" ) );
        m_parentFrame->GetCanvas()->Refresh( true );
    }
}


void DIALOG_CLEANUP_GRAPHICS::OnSelectItem( wxDataViewEvent& aEvent )
{
    const KIID&   itemID = RC_TREE_MODEL::ToUUID( aEvent.GetItem() );
    BOARD_ITEM*   item = m_parentFrame->GetBoard()->GetItem( itemID );
    WINDOW_THAWER thawer( m_parentFrame );

    m_parentFrame->FocusOnItem( item );
    m_parentFrame->GetCanvas()->Refresh();

    aEvent.Skip();
}


void DIALOG_CLEANUP_GRAPHICS::OnLeftDClickItem( wxMouseEvent& event )
{
    event.Skip();

    if( m_changesDataView->GetCurrentItem().IsOk() )
    {
        if( !IsModal() )
            Show( false );
    }
}


