/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <tool/actions.h>
#include <tool/action_menu.h>
#include <widgets/lib_tree.h>
#include <pcbnew_id.h>
#include <footprint_edit_frame.h>
#include <fp_lib_table.h>
#include <menus_helpers.h>


FOOTPRINT_TREE_PANE::FOOTPRINT_TREE_PANE( FOOTPRINT_EDIT_FRAME* aParent )
        : wxPanel( aParent ),
          m_frame( aParent ),
          m_tree( nullptr )
{
    // Create widgets
    wxBoxSizer* boxSizer = new wxBoxSizer( wxVERTICAL );
    m_tree = new LIB_TREE( this, &GFootprintTable, m_frame->GetLibTreeAdapter(), LIB_TREE::SEARCH );
    boxSizer->Add( m_tree, 1, wxEXPAND, 5 );

    SetSizer( boxSizer );      // should remove the previous sizer according to wxWidgets docs
    Layout();
    boxSizer->Fit( this );

    // Setup right click-context menus
    std::unique_ptr<ACTION_MENU> menuLibrary = std::make_unique<ACTION_MENU>();
    menuLibrary->Add( ACTIONS::newLibrary );
    menuLibrary->Add( ACTIONS::addLibrary );
    menuLibrary->Add( _( "Save" ), ID_MODEDIT_SAVE, save_xpm );
    menuLibrary->Add( _( "Save a Copy As..." ), ID_MODEDIT_SAVE_AS, save_as_xpm );

    menuLibrary->AppendSeparator();
    menuLibrary->Add( _( "New Footprint..." ), ID_MODEDIT_NEW_MODULE, new_footprint_xpm );
#ifdef KICAD_SCRIPTING
    menuLibrary->Add( _( "Create Footprint from Wizard..." ), ID_MODEDIT_NEW_MODULE_FROM_WIZARD, module_wizard_xpm );
#endif
    menuLibrary->Add( _( "Import Footprint..." ), ID_MODEDIT_IMPORT_PART, import_module_xpm );
    menuLibrary->Add( _( "Paste Footprint" ), ID_MODEDIT_PASTE_PART, paste_xpm );

    std::unique_ptr<ACTION_MENU> menuPart = std::make_unique<ACTION_MENU>();
    menuPart->Add( _( "Edit Footprint" ), ID_MODEDIT_EDIT_MODULE, edit_xpm );

    menuPart->AppendSeparator();
    menuPart->Add( _( "Save" ), ID_MODEDIT_SAVE, save_xpm );
    menuPart->Add( _( "Save a Copy As..." ), ID_MODEDIT_SAVE_AS, save_as_xpm );
    menuPart->Add( _( "Delete" ), ID_MODEDIT_DELETE_PART, delete_xpm );
    menuPart->Add( ACTIONS::revert );

    menuPart->AppendSeparator();
    menuPart->Add( _( "Cut" ), ID_MODEDIT_CUT_PART, cut_xpm );
    menuPart->Add( _( "Copy" ), ID_MODEDIT_COPY_PART, copy_xpm );

    menuPart->AppendSeparator();
    menuPart->Add( _( "Export Footprint..." ), ID_MODEDIT_EXPORT_PART, export_module_xpm );

    // Menu displayed when nothing is selected
    std::unique_ptr<ACTION_MENU> menuNoSelection = std::make_unique<ACTION_MENU>();
    menuNoSelection->Add( ACTIONS::newLibrary );
    menuNoSelection->Add( ACTIONS::addLibrary );

    m_tree->SetMenu( LIB_TREE_NODE::LIBID, std::move( menuPart ) );
    m_tree->SetMenu( LIB_TREE_NODE::LIB, std::move( menuLibrary ) );
    m_tree->SetMenu( LIB_TREE_NODE::INVALID, std::move( menuNoSelection ) );

    // Event handlers
    Bind( COMPONENT_SELECTED, &FOOTPRINT_TREE_PANE::onComponentSelected, this );
    m_tree->Bind( wxEVT_UPDATE_UI, &FOOTPRINT_TREE_PANE::onUpdateUI, this );
}


FOOTPRINT_TREE_PANE::~FOOTPRINT_TREE_PANE()
{
    m_tree->Destroy();
}


void FOOTPRINT_TREE_PANE::Regenerate()
{
    if( m_tree )
        m_tree->Regenerate( true );
}


void FOOTPRINT_TREE_PANE::onComponentSelected( wxCommandEvent& aEvent )
{
    wxCommandEvent evt( wxEVT_COMMAND_TOOL_CLICKED, ID_MODEDIT_EDIT_MODULE );
    m_frame->Process_Special_Functions( evt );

    // Make sure current-part highlighting doesn't get lost in seleciton highlighting
    m_tree->Unselect();
}


void FOOTPRINT_TREE_PANE::onUpdateUI( wxUpdateUIEvent& aEvent )
{
    if( m_frame->GetGalCanvas()->HasFocus() )
    {
        // Don't allow a selected item in the tree when the canvas has focus: it's too easy
        // to confuse the selected-highlighting with the being-edited-on-canvas-highlighting.
        m_tree->Unselect();
    }
}
