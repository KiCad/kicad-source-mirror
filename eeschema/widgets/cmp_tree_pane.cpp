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

#include "cmp_tree_pane.h"

#include <component_tree.h>
#include <eeschema_id.h>
#include <lib_manager.h>
#include <libeditframe.h>
#include <symbol_lib_table.h>


CMP_TREE_PANE::CMP_TREE_PANE( LIB_EDIT_FRAME* aParent, LIB_MANAGER* aLibMgr )
        : wxPanel( aParent ),
          m_libEditFrame( aParent ), m_tree( nullptr ), m_libMgr( aLibMgr )
{
    // Create widgets
    wxBoxSizer* boxSizer = new wxBoxSizer( wxVERTICAL );
    m_tree = new COMPONENT_TREE( this, &SYMBOL_LIB_TABLE::GetGlobalLibTable(),
            m_libMgr->GetAdapter(), COMPONENT_TREE::SEARCH );
    boxSizer->Add( m_tree, 1, wxEXPAND | wxALL, 5 );

    SetSizer( boxSizer );      // should remove the previous sizer according to wxWidgets docs
    Layout();
    boxSizer->Fit( this );

    // Setup right click-context menus
    std::unique_ptr<wxMenu> menuLibrary = std::make_unique<wxMenu>();
    menuLibrary->Append( ID_LIBEDIT_NEW_LIBRARY, _( "&New Library..." ) );
    menuLibrary->Append( ID_LIBEDIT_ADD_LIBRARY, _( "&Add Library..." ) );
    menuLibrary->Append( ID_LIBEDIT_SAVE_LIBRARY, _( "&Save Library" ) );
    menuLibrary->Append( ID_LIBEDIT_SAVE_LIBRARY_AS, _( "Save Library As..." ) );
    menuLibrary->Append( ID_LIBEDIT_REVERT_LIBRARY, _( "&Revert Library" ) );
    menuLibrary->AppendSeparator();
    menuLibrary->Append( ID_LIBEDIT_NEW_PART, _( "New &Part..." ) );
    menuLibrary->Append( ID_LIBEDIT_IMPORT_PART, _( "Import Part..." ) );
    menuLibrary->Append( ID_LIBEDIT_PASTE_PART, _( "Paste Part" ) );

    std::unique_ptr<wxMenu> menuPart = std::make_unique<wxMenu>();
    menuPart->Append( ID_LIBEDIT_EDIT_PART, _( "Edit Part" ) );
    menuPart->Append( ID_LIBEDIT_REMOVE_PART, _( "Remove Part" ) );
    menuPart->Append( ID_LIBEDIT_EXPORT_PART, _( "Export Part..." ) );
    menuPart->Append( ID_LIBEDIT_SAVE_PART, _( "&Save Part" ) );
    menuPart->Append( ID_LIBEDIT_REVERT_PART, _( "&Revert Part" ) );
    menuPart->AppendSeparator();
    menuPart->Append( ID_LIBEDIT_CUT_PART, _( "Cut Part" ) );
    menuPart->Append( ID_LIBEDIT_COPY_PART, _( "Copy Part" ) );
    menuPart->Append( ID_LIBEDIT_DUPLICATE_PART, _( "Duplicate Part" ) );
    menuPart->AppendSeparator();

    // Menu displayed when nothing is selected
    std::unique_ptr<wxMenu> menuNoSelection = std::make_unique<wxMenu>();
    menuNoSelection->Append( ID_LIBEDIT_NEW_LIBRARY, _( "&New Library..." ) );
    menuNoSelection->Append( ID_LIBEDIT_ADD_LIBRARY, _( "&Add Library..." ) );

    m_tree->SetMenu( CMP_TREE_NODE::LIBID, std::move( menuPart ) );
    m_tree->SetMenu( CMP_TREE_NODE::LIB, std::move( menuLibrary ) );
    m_tree->SetMenu( CMP_TREE_NODE::INVALID, std::move( menuNoSelection ) );

    // Event handlers
    Bind( COMPONENT_SELECTED, &CMP_TREE_PANE::onComponentSelected, this );
}


CMP_TREE_PANE::~CMP_TREE_PANE()
{
    m_tree->Destroy();
}


void CMP_TREE_PANE::Regenerate()
{
    if( m_tree )
        m_tree->Regenerate();
}


void CMP_TREE_PANE::onComponentSelected( wxCommandEvent& aEvent )
{
    // Repost the event
    wxCommandEvent evt( ID_LIBEDIT_EDIT_PART );
    // I cannot figure out why the two methods below do not work..
    //wxPostEvent( libEditFrame, evt );
    //wxQueueEvent( m_libEditFrame, new wxCommandEvent( ID_LIBEDIT_EDIT_PART ) );
    m_libEditFrame->OnEditPart( evt );
}
