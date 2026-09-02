/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <tool/conditional_menu.h>
#include <tool/tool_menu.h>
#include <dialogs/dialog_mdw_library_checkin.h>
#include <eda_draw_frame.h>
#include <widgets/lib_tree.h>
#include <widgets/mdw_part_actions.h>
#include <widgets/mdw_part_controller.h>
#include <project.h>
#include <kiway.h>
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
    auto checkPinnedStatus = [this]( bool aPin )
    {
        LIB_TREE* libTree = m_frame->GetLibTree();

        if( !libTree )
            return false;

        std::vector<LIB_TREE_NODE*> selection;
        libTree->GetSelectedTreeNodes( selection );

        bool anyLibrary = false;

        // A selection of parts alone holds no library to pin or unpin
        for( const LIB_TREE_NODE* lib : selection )
        {
            if( !lib || lib->m_Type != LIB_TREE_NODE::TYPE::LIBRARY )
                continue;

            if( lib->m_Pinned != aPin )
                return false;

            anyLibrary = true;
        }

        return anyLibrary;
    };

    auto pinnedLibSelectedCondition = [checkPinnedStatus]( const SELECTION& aSel )
    {
        return checkPinnedStatus( true );
    };

    auto unpinnedLibSelectedCondition = [checkPinnedStatus]( const SELECTION& aSel )
    {
        return checkPinnedStatus( false );
    };

    aMenu->AddItem( ACTIONS::pinLibrary,        unpinnedLibSelectedCondition, 1 );
    aMenu->AddItem( ACTIONS::unpinLibrary,      pinnedLibSelectedCondition, 1 );
    aMenu->AddSeparator( 1 );

    aMenu->AddSeparator( 400 );
    aMenu->AddItem( ACTIONS::hideLibraryTree,   SELECTION_CONDITIONS::ShowAlways, 400 );
}


void LIBRARY_EDITOR_CONTROL::AddTeamLibraryMenuItems( TOOL_INTERACTIVE* aTool, TOOL_MENU& aToolMenu,
                                                      CONDITIONAL_MENU&          aMenu,
                                                      const SELECTION_CONDITION& aLibSelected )
{
    auto teamLibraryCondition =
            [this]( const SELECTION& aSel )
            {
                MDW_PART_EDITOR* editor = partEditor();

                return editor
                       && !MdwTeamLibraryMarker( editor->PartTableType(), selectedLibraryName() ).IsEmpty();
            };

    aMenu.AddItem( ACTIONS::checkInTeamLibrary, aLibSelected && teamLibraryCondition, 10 );
    aMenu.AddItem( ACTIONS::updateTeamLibrary, aLibSelected && teamLibraryCondition, 10 );

    MdwAddLifecycleMenu( aTool, aToolMenu, aMenu,
                         [this]()
                         {
                             return partEditor();
                         },
                         [this]()
                         {
                             return selectedLibIds();
                         } );
}


MDW_PART_EDITOR* LIBRARY_EDITOR_CONTROL::partEditor() const
{
    return dynamic_cast<MDW_PART_EDITOR*>( m_frame );
}


wxString LIBRARY_EDITOR_CONTROL::selectedLibraryName() const
{
    return m_frame->GetLibTree()->GetSelectedLibId().GetLibNickname();
}


std::vector<LIB_ID> LIBRARY_EDITOR_CONTROL::selectedLibIds() const
{
    std::vector<LIB_ID> selected;

    m_frame->GetLibTree()->GetSelectedLibIds( selected );
    return selected;
}


int LIBRARY_EDITOR_CONTROL::CheckInTeamLibrary( const TOOL_EVENT& aEvent )
{
    if( MDW_PART_EDITOR* editor = partEditor() )
        editor->CheckInLibrary( selectedLibraryName() );

    return 0;
}


int LIBRARY_EDITOR_CONTROL::UpdateTeamLibrary( const TOOL_EVENT& aEvent )
{
    if( MDW_PART_EDITOR* editor = partEditor() )
        MdwUpdatePartLibrary( *editor, selectedLibraryName() );

    return 0;
}


int LIBRARY_EDITOR_CONTROL::TeamPartAction( const TOOL_EVENT& aEvent )
{
    if( MDW_PART_EDITOR* editor = partEditor() )
        MdwRunPartToolAction( *editor, aEvent, selectedLibIds() );

    return 0;
}


void LIBRARY_EDITOR_CONTROL::regenerateLibraryTree()
{
    LIB_TREE* libTree = m_frame->GetLibTree();
    LIB_ID    target = m_frame->GetTargetLibId();

    libTree->Regenerate( true );

    if( target.IsValid() )
        libTree->CenterLibId( target );
}

void LIBRARY_EDITOR_CONTROL::changeSelectedPinStatus( const bool aPin )
{
    LIB_TREE* libTree = m_frame->GetLibTree();
    if( libTree )
    {
        std::vector<LIB_TREE_NODE*> selection;
        libTree->GetSelectedTreeNodes( selection );

        for( LIB_TREE_NODE* lib : selection )
        {
            if( lib && lib->m_Type == LIB_TREE_NODE::TYPE::LIBRARY && lib->m_Pinned != aPin )
            {
                const KIWAY::FACE_T kifaceType = KIWAY::KifaceType( m_frame->GetFrameType() );

                if( kifaceType == KIWAY::FACE_SCH || kifaceType == KIWAY::FACE_PCB )
                {
                    if( aPin )
                        m_frame->Prj().PinLibrary( lib->m_LibId.GetLibNickname(),
                                                   kifaceType == KIWAY::FACE_SCH
                                                           ? PROJECT::LIB_TYPE_T::SYMBOL_LIB
                                                           : PROJECT::LIB_TYPE_T::FOOTPRINT_LIB );
                    else
                        m_frame->Prj().UnpinLibrary( lib->m_LibId.GetLibNickname(),
                                                     kifaceType == KIWAY::FACE_SCH
                                                             ? PROJECT::LIB_TYPE_T::SYMBOL_LIB
                                                             : PROJECT::LIB_TYPE_T::FOOTPRINT_LIB );

                    lib->m_Pinned = aPin;
                }
                else
                {
                    wxFAIL_MSG( wxT( "Unsupported frame type for library pinning." ) );
                }
            }
        }

        regenerateLibraryTree();
    }
}

int LIBRARY_EDITOR_CONTROL::PinLibrary( const TOOL_EVENT& aEvent )
{
    changeSelectedPinStatus( true );

    return 0;
}


int LIBRARY_EDITOR_CONTROL::UnpinLibrary( const TOOL_EVENT& aEvent )
{
    changeSelectedPinStatus( false );

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
    Go( &LIBRARY_EDITOR_CONTROL::CheckInTeamLibrary,   ACTIONS::checkInTeamLibrary.MakeEvent() );
    Go( &LIBRARY_EDITOR_CONTROL::UpdateTeamLibrary,    ACTIONS::updateTeamLibrary.MakeEvent() );

    for( const auto& [action, id] : MdwPartToolActions() )
        Go( &LIBRARY_EDITOR_CONTROL::TeamPartAction, action->MakeEvent() );
}
