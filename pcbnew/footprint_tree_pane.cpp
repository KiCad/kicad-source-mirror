/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "footprint_tree_pane.h"
#include "fp_tree_synchronizing_adapter.h"
#include <widgets/lib_tree.h>
#include <footprint_edit_frame.h>
#include <fp_lib_table.h>

FOOTPRINT_TREE_PANE::FOOTPRINT_TREE_PANE( FOOTPRINT_EDIT_FRAME* aParent )
        : wxPanel( aParent ),
          m_frame( aParent ),
          m_tree( nullptr )
{
    // Create widgets
    wxBoxSizer* boxSizer = new wxBoxSizer( wxVERTICAL );
    m_tree = new LIB_TREE( this, wxT( "footprints" ),
                           m_frame->GetLibTreeAdapter(), LIB_TREE::SEARCH );
    boxSizer->Add( m_tree, 1, wxEXPAND, 5 );

    SetSizer( boxSizer );      // should remove the previous sizer according to wxWidgets docs
    Layout();
    boxSizer->Fit( this );

    m_frame->GetLibTreeAdapter()->FinishTreeInitialization();

    // Event handlers
    Bind( EVT_LIBITEM_CHOSEN, &FOOTPRINT_TREE_PANE::onComponentSelected, this );
    m_tree->Bind( wxEVT_UPDATE_UI, &FOOTPRINT_TREE_PANE::onUpdateUI, this );
    m_frame->Bind( wxEVT_MENU_OPEN, &FOOTPRINT_TREE_PANE::onMenuOpen, this );
    m_frame->Bind( wxEVT_MENU_CLOSE, &FOOTPRINT_TREE_PANE::onMenuClose, this );
}


void FOOTPRINT_TREE_PANE::FocusSearchFieldIfExists()
{
    if( m_tree )
    {
        m_tree->FocusSearchFieldIfExists();
    }
}


FOOTPRINT_TREE_PANE::~FOOTPRINT_TREE_PANE()
{
    m_frame->Unbind( wxEVT_MENU_OPEN, &FOOTPRINT_TREE_PANE::onMenuOpen, this );
    m_frame->Unbind( wxEVT_MENU_CLOSE, &FOOTPRINT_TREE_PANE::onMenuClose, this );
    m_tree->Unbind( wxEVT_UPDATE_UI, &FOOTPRINT_TREE_PANE::onUpdateUI, this );
    Unbind( EVT_LIBITEM_CHOSEN, &FOOTPRINT_TREE_PANE::onComponentSelected, this );
    m_tree->Destroy();
}


void FOOTPRINT_TREE_PANE::onMenuOpen( wxMenuEvent& aEvent )
{
    m_tree->BlockPreview( true );
    aEvent.Skip();
}


void FOOTPRINT_TREE_PANE::onMenuClose( wxMenuEvent& aEvent )
{
    m_tree->BlockPreview( false );
    aEvent.Skip();
}


void FOOTPRINT_TREE_PANE::onComponentSelected( wxCommandEvent& aEvent )
{
    m_frame->LoadFootprintFromLibrary( GetLibTree()->GetSelectedLibId() );
    // Make sure current-part highlighting doesn't get lost in seleciton highlighting
    m_tree->Unselect();
}


void FOOTPRINT_TREE_PANE::onUpdateUI( wxUpdateUIEvent& aEvent )
{
    if( m_frame->GetCanvas()->HasFocus() )
    {
        // Don't allow a selected item in the tree when the canvas has focus: it's too easy
        // to confuse the selected-highlighting with the being-edited-on-canvas-highlighting.
        m_tree->Unselect();
    }
}
