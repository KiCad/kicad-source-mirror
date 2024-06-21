/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program;
 * if not, you may find one at http://www.gnu.org/licenses/
 */

#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <eda_draw_frame.h>
#include <widgets/lib_tree.h>
#include <project.h>
#include "wx/generic/textdlgg.h"
#include "library_editor_control.h"


LIBRARY_EDITOR_CONTROL::LIBRARY_EDITOR_CONTROL() :
    TOOL_INTERACTIVE( "common.LibraryEditorControl" ),
    m_frame( nullptr )
{
}


void LIBRARY_EDITOR_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<EDA_DRAW_FRAME>();
}


void LIBRARY_EDITOR_CONTROL::AddContextMenuItems( CONDITIONAL_MENU* aMenu )
{
    auto pinnedLibSelectedCondition =
            [this]( const SELECTION& aSel )
            {
                LIB_TREE*      libTree = m_frame->GetLibTree();
                LIB_TREE_NODE* current = libTree ? libTree->GetCurrentTreeNode() : nullptr;
                return current && current->m_Type == LIB_TREE_NODE::TYPE::LIBRARY && current->m_Pinned;
            };
    auto unpinnedLibSelectedCondition =
            [this](const SELECTION& aSel )
            {
                LIB_TREE*      libTree = m_frame->GetLibTree();
                LIB_TREE_NODE* current = libTree ? libTree->GetCurrentTreeNode() : nullptr;
                return current && current->m_Type == LIB_TREE_NODE::TYPE::LIBRARY && !current->m_Pinned;
            };

    aMenu->AddItem( ACTIONS::pinLibrary,        unpinnedLibSelectedCondition, 1 );
    aMenu->AddItem( ACTIONS::unpinLibrary,      pinnedLibSelectedCondition, 1 );
    aMenu->AddSeparator( 1 );

    aMenu->AddSeparator( 400 );
    aMenu->AddItem( ACTIONS::hideLibraryTree,   SELECTION_CONDITIONS::ShowAlways, 400 );
}


void LIBRARY_EDITOR_CONTROL::regenerateLibraryTree()
{
    LIB_TREE* libTree = m_frame->GetLibTree();
    LIB_ID    target = m_frame->GetTargetLibId();

    libTree->Regenerate( true );

    if( target.IsValid() )
        libTree->CenterLibId( target );
}


int LIBRARY_EDITOR_CONTROL::PinLibrary( const TOOL_EVENT& aEvent )
{
    LIB_TREE*      libTree = m_frame->GetLibTree();
    LIB_TREE_NODE* current = libTree ? libTree->GetCurrentTreeNode() : nullptr;

    if( current && !current->m_Pinned )
    {
        m_frame->Prj().PinLibrary( current->m_LibId.GetLibNickname(), false );
        current->m_Pinned = true;
        regenerateLibraryTree();
    }

    return 0;
}


int LIBRARY_EDITOR_CONTROL::UnpinLibrary( const TOOL_EVENT& aEvent )
{
    LIB_TREE*      libTree = m_frame->GetLibTree();
    LIB_TREE_NODE* current = libTree ? libTree->GetCurrentTreeNode() : nullptr;

    if( current && current->m_Pinned )
    {
        m_frame->Prj().UnpinLibrary( current->m_LibId.GetLibNickname(), false );
        current->m_Pinned = false;
        regenerateLibraryTree();
    }

    return 0;
}


int LIBRARY_EDITOR_CONTROL::ToggleLibraryTree( const TOOL_EVENT& aEvent )
{
    m_frame->ToggleLibraryTree();
    return 0;
}


int LIBRARY_EDITOR_CONTROL::LibraryTreeSearch( const TOOL_EVENT& aEvent )
{
    if (!m_frame->IsLibraryTreeShown() )
        m_frame->ToggleLibraryTree();

    m_frame->FocusLibraryTreeInput();
    return 0;
}


class RENAME_DIALOG : public wxTextEntryDialog
{
public:
    RENAME_DIALOG( wxWindow* aParent, const wxString& aTitle, const wxString& aName,
                   std::function<bool( const wxString& newName )> aValidator ) :
            wxTextEntryDialog( aParent, _( "New name:" ), aTitle, aName ),
            m_validator( std::move( aValidator ) )
    { }

protected:
    bool TransferDataFromWindow() override
    {
        return m_validator( m_textctrl->GetValue().Trim( true ).Trim( false ) );
    }

private:
    std::function<bool( const wxString& aNewName )> m_validator;
};


bool LIBRARY_EDITOR_CONTROL::RenameLibrary( const wxString& aTitle, const wxString& aName,
                                            std::function<bool( const wxString& aNewName )> aValidator )
{
    RENAME_DIALOG dlg( m_frame, aTitle, aName, std::move( aValidator ) );

    return dlg.ShowModal() == wxID_OK;
}


void LIBRARY_EDITOR_CONTROL::setTransitions()
{
    Go( &LIBRARY_EDITOR_CONTROL::PinLibrary,           ACTIONS::pinLibrary.MakeEvent() );
    Go( &LIBRARY_EDITOR_CONTROL::UnpinLibrary,         ACTIONS::unpinLibrary.MakeEvent() );
    Go( &LIBRARY_EDITOR_CONTROL::ToggleLibraryTree,    ACTIONS::showLibraryTree.MakeEvent() );
    Go( &LIBRARY_EDITOR_CONTROL::ToggleLibraryTree,    ACTIONS::hideLibraryTree.MakeEvent() );
    Go( &LIBRARY_EDITOR_CONTROL::LibraryTreeSearch,    ACTIONS::libraryTreeSearch.MakeEvent() );
}
