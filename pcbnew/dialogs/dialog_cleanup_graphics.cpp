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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <dialog_cleanup_graphics.h>
#include <board_commit.h>
#include <footprint.h>
#include <pad.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <graphics_cleaner.h>
#include <pcb_base_frame.h>


static int s_defaultTolerance = pcbIUScale.mmToIU( 2 );

DIALOG_CLEANUP_GRAPHICS::DIALOG_CLEANUP_GRAPHICS( PCB_BASE_FRAME* aParent,
                                                  bool aIsFootprintEditor ) :
        DIALOG_CLEANUP_GRAPHICS_BASE( aParent ),
        m_parentFrame( aParent ),
        m_isFootprintEditor( aIsFootprintEditor ),
        m_tolerance( aParent, m_toleranceLabel, m_toleranceCtrl, m_toleranceUnits )
{
    m_changesTreeModel = new RC_TREE_MODEL( m_parentFrame, m_changesDataView );
    m_changesDataView->AssociateModel( m_changesTreeModel );

    if( aIsFootprintEditor )
    {
        SetupStandardButtons( { { wxID_OK, _( "Update Footprint" ) } } );
        m_nettieHint->SetFont( KIUI::GetInfoFont( aParent ).Italic() );

        m_fixBoardOutlines->Show( false );
        m_toleranceSizer->Show( false );
    }
    else
    {
        SetupStandardButtons( { { wxID_OK, _( "Update PCB" ) } } );
        m_mergePadsOpt->Show( false );
        m_nettieHint->Show( false );
    }

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
    m_tolerance.SetValue( s_defaultTolerance );

    doCleanup( true );

    return true;
}


bool DIALOG_CLEANUP_GRAPHICS::TransferDataFromWindow()
{
    s_defaultTolerance = m_tolerance.GetValue();

    doCleanup( false );

    return true;
}


void DIALOG_CLEANUP_GRAPHICS::doCleanup( bool aDryRun )
{
    wxBusyCursor busy;

    BOARD_COMMIT     commit( m_parentFrame );
    BOARD*           board = m_parentFrame->GetBoard();
    FOOTPRINT*       fp = m_isFootprintEditor ? board->GetFirstFootprint() : nullptr;
    GRAPHICS_CLEANER cleaner( fp ? fp->GraphicalItems() : board->Drawings(), fp, commit,
                              m_parentFrame->GetToolManager() );

    if( !aDryRun )
    {
        // Clear current selection list to avoid selection of deleted items
        m_parentFrame->GetToolManager()->RunAction( ACTIONS::selectionClear );

        // ... and to keep the treeModel from trying to refresh a deleted item
        m_changesTreeModel->Update( nullptr, RPT_SEVERITY_ACTION );
    }

    m_items.clear();

    // Old model has to be refreshed, GAL normally does not keep updating it
    m_parentFrame->Compile_Ratsnest( false );

    cleaner.CleanupBoard( aDryRun, &m_items, m_createRectanglesOpt->GetValue(),
                          m_deleteRedundantOpt->GetValue(), m_mergePadsOpt->GetValue(),
                          m_fixBoardOutlines->GetValue(), m_tolerance.GetIntValue() );

    if( aDryRun )
    {
        m_changesTreeModel->Update( std::make_shared<VECTOR_CLEANUP_ITEMS_PROVIDER>( &m_items ),
                                    RPT_SEVERITY_ACTION );
    }
    else if( !commit.Empty() )
    {
        // Clear undo and redo lists to avoid inconsistencies between lists
        commit.Push( _( "Cleanup Graphics" ) );
        m_parentFrame->GetCanvas()->Refresh( true );
    }
}


void DIALOG_CLEANUP_GRAPHICS::OnSelectItem( wxDataViewEvent& aEvent )
{
    const KIID& itemID = RC_TREE_MODEL::ToUUID( aEvent.GetItem() );

    if( BOARD_ITEM* item = m_parentFrame->GetBoard()->ResolveItem( itemID, true ) )
    {
        WINDOW_THAWER thawer( m_parentFrame );

        if( !item->GetLayerSet().test( m_parentFrame->GetActiveLayer() ) )
            m_parentFrame->SetActiveLayer( item->GetLayerSet().UIOrder().front() );

        m_parentFrame->FocusOnItem( item );
        m_parentFrame->GetCanvas()->Refresh();
    }

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


