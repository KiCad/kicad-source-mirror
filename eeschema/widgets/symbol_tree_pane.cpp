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
#include <eeschema_id.h>
#include <lib_manager.h>
#include <lib_edit_frame.h>
#include <symbol_lib_table.h>
#include <menus_helpers.h>
#include <tool/action_menu.h>
#include <tool/actions.h>
#include <tools/ee_actions.h>

SYMBOL_TREE_PANE::SYMBOL_TREE_PANE( LIB_EDIT_FRAME* aParent, LIB_MANAGER* aLibMgr )
        : wxPanel( aParent ),
          m_libEditFrame( aParent ),
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

    // Setup right click-context menus
    std::unique_ptr<ACTION_MENU> menuLibrary = std::make_unique<ACTION_MENU>();

    menuLibrary->Add( ACTIONS::newLibrary );
    menuLibrary->Add( ACTIONS::addLibrary );
    menuLibrary->Add( ACTIONS::save );
    menuLibrary->Add( ACTIONS::saveAs );
    menuLibrary->Add( ACTIONS::revert );

    menuLibrary->AppendSeparator();
    menuLibrary->Add( EE_ACTIONS::newSymbol );
    menuLibrary->Add( EE_ACTIONS::importSymbol );
    menuLibrary->Add( _( "Paste Symbol" ), ID_LIBEDIT_PASTE_PART, paste_xpm );

    std::unique_ptr<ACTION_MENU> menuPart = std::make_unique<ACTION_MENU>();
    menuPart->Add( _( "Edit Symbol" ), ID_LIBEDIT_EDIT_PART, edit_xpm );

    menuPart->AppendSeparator();
    menuPart->Add( ACTIONS::save );
    menuPart->Add( ACTIONS::saveCopyAs );
    menuPart->Add( _( "Duplicate" ), ID_LIBEDIT_DUPLICATE_PART, duplicate_xpm );
    menuPart->Add( _( "Delete" ), ID_LIBEDIT_REMOVE_PART, delete_xpm );
    menuPart->Add( ACTIONS::revert );

    menuPart->AppendSeparator();
    menuPart->Add( _( "Cut" ), ID_LIBEDIT_CUT_PART, cut_xpm );
    menuPart->Add( _( "Copy" ), ID_LIBEDIT_COPY_PART, copy_xpm );

    menuPart->AppendSeparator();
    menuPart->Add( _( "Export Symbol..." ), ID_LIBEDIT_EXPORT_PART, export_part_xpm );

    // Menu displayed when nothing is selected
    std::unique_ptr<ACTION_MENU> menuNoSelection = std::make_unique<ACTION_MENU>();
    menuLibrary->Add( ACTIONS::newLibrary );
    menuLibrary->Add( ACTIONS::addLibrary );

    m_tree->SetMenu( LIB_TREE_NODE::LIBID, std::move( menuPart ) );
    m_tree->SetMenu( LIB_TREE_NODE::LIB, std::move( menuLibrary ) );
    m_tree->SetMenu( LIB_TREE_NODE::INVALID, std::move( menuNoSelection ) );

    // Event handlers
    Bind( COMPONENT_SELECTED, &SYMBOL_TREE_PANE::onComponentSelected, this );
}


SYMBOL_TREE_PANE::~SYMBOL_TREE_PANE()
{
    m_tree->Destroy();
}


void SYMBOL_TREE_PANE::Regenerate()
{
    if( m_tree )
        m_tree->Regenerate( true );
}


void SYMBOL_TREE_PANE::onComponentSelected( wxCommandEvent& aEvent )
{
    wxCommandEvent evt( ID_LIBEDIT_EDIT_PART );
    m_libEditFrame->OnEditPart( evt );

    // Make sure current-part highlighting doesn't get lost in seleciton highlighting
    m_tree->Unselect();
}
