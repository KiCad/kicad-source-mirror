/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "symbol_tree_pane.h"
#include <widgets/lib_tree.h>
#include <symbol_library_manager.h>
#include <symbol_edit_frame.h>
#include <symbol_lib_table.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>

SYMBOL_TREE_PANE::SYMBOL_TREE_PANE( SYMBOL_EDIT_FRAME* aParent, SYMBOL_LIBRARY_MANAGER* aLibMgr )
        : wxPanel( aParent ),
          m_symbolEditFrame( aParent ),
          m_tree( nullptr ),
          m_libMgr( aLibMgr )
{
    // Create widgets
    wxBoxSizer* boxSizer = new wxBoxSizer( wxVERTICAL );
    m_tree = new LIB_TREE( this, &SYMBOL_LIB_TABLE::GetGlobalLibTable(), m_libMgr->GetAdapter(),
                           LIB_TREE::SEARCH );
    boxSizer->Add( m_tree, 1, wxEXPAND, 5 );

    SetSizer( boxSizer );      // should remove the previous sizer according to wxWidgets docs
    Layout();
    boxSizer->Fit( this );

    m_libMgr->GetAdapter()->FinishTreeInitialization();

    // Event handlers
    Bind( SYMBOL_SELECTED, &SYMBOL_TREE_PANE::onComponentSelected, this );
    m_tree->Bind( wxEVT_UPDATE_UI, &SYMBOL_TREE_PANE::onUpdateUI, this );
}


SYMBOL_TREE_PANE::~SYMBOL_TREE_PANE()
{
    m_tree->Destroy();
}


void SYMBOL_TREE_PANE::onComponentSelected( wxCommandEvent& aEvent )
{
    m_symbolEditFrame->GetToolManager()->RunAction( EE_ACTIONS::editSymbol, true );

    // Make sure current-part highlighting doesn't get lost in selection highlighting
    m_tree->Unselect();

    // Turn off any previous current-part highlighting
    m_tree->RefreshLibTree();
}


void SYMBOL_TREE_PANE::onUpdateUI( wxUpdateUIEvent& aEvent )
{
    if( m_symbolEditFrame->GetCanvas()->HasFocus() )
    {
        // Don't allow a selected item in the tree when the canvas has focus: it's too easy
        // to confuse the selected-highlighting with the being-edited-on-canvas-highlighting.
        m_tree->Unselect();
    }
}
