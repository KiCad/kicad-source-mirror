/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <lib_symbol_library_manager.h>
#include <symbol_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>

SYMBOL_TREE_PANE::SYMBOL_TREE_PANE( SYMBOL_EDIT_FRAME* aParent,
                                    LIB_SYMBOL_LIBRARY_MANAGER* aLibMgr )
        : wxPanel( aParent ),
          m_symbolEditFrame( aParent ),
          m_tree( nullptr ),
          m_libMgr( aLibMgr )
{
    // Create widgets
    wxBoxSizer* boxSizer = new wxBoxSizer( wxVERTICAL );
    m_tree = new LIB_TREE( this, wxT( "symbols" ),
                           m_libMgr->GetAdapter(), LIB_TREE::SEARCH | LIB_TREE::MULTISELECT );
    boxSizer->Add( m_tree, 1, wxEXPAND, 5 );

    SetSizer( boxSizer );      // should remove the previous sizer according to wxWidgets docs
    Layout();
    boxSizer->Fit( this );

    m_libMgr->GetAdapter()->FinishTreeInitialization();

    // Event handlers
    Bind( EVT_LIBITEM_CHOSEN, &SYMBOL_TREE_PANE::onSymbolSelected, this );
    m_tree->Bind( wxEVT_UPDATE_UI, &SYMBOL_TREE_PANE::onUpdateUI, this );
    m_symbolEditFrame->Bind( wxEVT_MENU_OPEN, &SYMBOL_TREE_PANE::onMenuOpen, this );
    m_symbolEditFrame->Bind( wxEVT_MENU_CLOSE, &SYMBOL_TREE_PANE::onMenuClose, this );
}


SYMBOL_TREE_PANE::~SYMBOL_TREE_PANE()
{
    m_symbolEditFrame->Unbind( wxEVT_MENU_OPEN, &SYMBOL_TREE_PANE::onMenuOpen, this );
    m_symbolEditFrame->Unbind( wxEVT_MENU_CLOSE, &SYMBOL_TREE_PANE::onMenuClose, this );
    m_tree->Unbind( wxEVT_UPDATE_UI, &SYMBOL_TREE_PANE::onUpdateUI, this );
    Unbind( EVT_LIBITEM_CHOSEN, &SYMBOL_TREE_PANE::onSymbolSelected, this );
    m_tree->Destroy();
}


void SYMBOL_TREE_PANE::onMenuOpen( wxMenuEvent& aEvent )
{
    m_tree->BlockPreview( true );
    aEvent.Skip();
}


void SYMBOL_TREE_PANE::onMenuClose( wxMenuEvent& aEvent )
{
    m_tree->BlockPreview( false );
    aEvent.Skip();
}


void SYMBOL_TREE_PANE::onSymbolSelected( wxCommandEvent& aEvent )
{
    m_symbolEditFrame->GetToolManager()->RunAction( SCH_ACTIONS::editSymbol );

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
        if( m_symbolEditFrame->GetCurSymbol() != nullptr )
        {
            m_tree->Unselect();
        }
    }
}
