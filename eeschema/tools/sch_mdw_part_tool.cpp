/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <tools/sch_mdw_part_tool.h>

#include <confirm.h>
#include <eda_list_dialog.h>
#include <libraries/symbol_library_adapter.h>
#include <mdw/client/mdw_source.h>
#include <mdw_part_field.h>
#include <project_sch.h>
#include <sch_actions.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <sch_selection_tool.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <string_utils.h>
#include <tool/tool_manager.h>
#include <widgets/mdw_editing.h>
#include <widgets/mdw_member_index.h>

#include <map>

#include <wx/intl.h>


SCH_MDW_PART_TOOL::SCH_MDW_PART_TOOL() :
        SCH_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.MdwPartTool" )
{
}


bool SCH_MDW_PART_TOOL::Init()
{
    SCH_TOOL_BASE::Init();

    // Hidden, never disabled, when the symbol's source has no parts
    auto partSource = [this]( const SELECTION& aSelection )
    {
        if( aSelection.Size() != 1 || aSelection.Front()->Type() != SCH_SYMBOL_T )
            return false;

        return partBinding( *static_cast<const SCH_SYMBOL*>( aSelection.Front() ) ).has_value();
    };

    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();
    selToolMenu.AddItem( SCH_ACTIONS::mdwAssignPart, SCH_CONDITIONS::Idle && partSource, 200 );

    return true;
}


void SCH_MDW_PART_TOOL::Reset( RESET_REASON aReason )
{
    SCH_TOOL_BASE::Reset( aReason );

    // An answer for the schematic that was open before would link a symbol of this one
    if( aReason == MODEL_RELOAD || aReason == SUPERMODEL_RELOAD )
        m_resolve = MDW_OP();
}


std::optional<MDW_TEAM_LIBRARY> SCH_MDW_PART_TOOL::partBinding( const SCH_SYMBOL& aSymbol )
{
    const LIB_ID&                     libId = aSymbol.GetLibId();
    std::optional<LIBRARY_TABLE_ROW*> row =
            PROJECT_SCH::SymbolLibAdapter( &m_frame->Prj() )->GetRow( libId.GetLibNickname() );

    if( !row )
        return std::nullopt;

    std::optional<MDW_TEAM_LIBRARY> binding = MdwTeamLibrary( MDW_LIBRARY_TYPE::SYMBOL, **row );

    if( !binding || !binding->source->Has( MDW_CAPABILITY_GROUP::PARTS ) )
        return std::nullopt;

    return binding;
}


bool SCH_MDW_PART_TOOL::CanAssignPart( const SCH_SYMBOL& aSymbol )
{
    return partBinding( aSymbol ).has_value();
}


int SCH_MDW_PART_TOOL::AssignPart( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_SYMBOL_T } );

    if( selection.Size() != 1 )
        return 0;

    SCH_SYMBOL*                     symbol = static_cast<SCH_SYMBOL*>( selection.Front() );
    std::optional<MDW_TEAM_LIBRARY> binding = partBinding( *symbol );

    if( !binding )
        return 0;

    wxString                        name = symbol->GetLibId().GetUniStringLibItemName();
    std::optional<MDW_MEMBER_ENTRY> entry = MDW_MEMBER_INDEX::Get().Find( *binding, name );

    if( !entry )
    {
        DisplayInfoMessage( m_frame, wxString::Format( _( "%s is not in its workspace library yet, or the library "
                                                          "is still being read. Try again shortly." ),
                                                       UnescapeString( name ) ) );
        return 0;
    }

    KIID   id = symbol->m_Uuid;
    LIB_ID libId = symbol->GetLibId();

    m_resolve = binding->source->Parts().Resolve(
            { MdwMemberRef( *binding, entry->member ) },
            [this, id, libId]( const MDW_RESULT& aResult, const std::vector<MDW_PART_CANDIDATES>& aParts )
            {
                if( !aResult.IsOk() )
                {
                    MdwReportFailure( m_frame, _( "Could not read the workspace parts that use this symbol." ),
                                      aResult );
                    return;
                }

                choosePart( id, libId, aParts.empty() ? std::vector<MDW_PART_INFO>() : aParts.front().candidates );
            } );

    return 0;
}


void SCH_MDW_PART_TOOL::choosePart( const KIID& aSymbol, const LIB_ID& aLibId,
                                    const std::vector<MDW_PART_INFO>& aCandidates )
{
    // The candidates answer for aLibId; a symbol relinked or removed since must not take one
    auto find = [&]( SCH_SHEET_PATH& aPath ) -> SCH_SYMBOL*
    {
        SCH_SYMBOL* found = dynamic_cast<SCH_SYMBOL*>( m_frame->Schematic().ResolveItem( aSymbol, &aPath, true ) );
        return found && found->GetLibId() == aLibId ? found : nullptr;
    };

    SCH_SHEET_PATH path;
    SCH_SYMBOL*    symbol = find( path );

    if( !symbol )
        return;

    if( aCandidates.empty() )
    {
        wxString name = UnescapeString( symbol->GetLibId().GetUniStringLibItemName() );
        DisplayInfoMessage( m_frame, wxString::Format( _( "No workspace part uses %s." ), name ) );
        return;
    }

    std::optional<MDW_PART_IDENTITY>         current = MdwReadPartField( *symbol );
    std::map<wxString, const MDW_PART_INFO*> byKey;
    std::vector<wxArrayString>               rows;
    wxString                                 preselect;

    for( const MDW_PART_INFO& part : aCandidates )
    {
        wxString key = part.part_number.IsEmpty() ? part.part.item_id : part.part_number;

        // Two parts can share a number across sources, and the list returns only the text
        if( byKey.count( key ) )
            key = wxString::Format( wxS( "%s (%s)" ), key, part.part.item_id );

        byKey[key] = &part;
        rows.push_back( { key, part.revision_label, part.name, part.description } );

        if( current && current->part.source == part.part.source && current->part.item_id == part.part.item_id )
            preselect = key;
    }

    EDA_LIST_DIALOG dlg( m_frame, _( "Assign Part" ), { _( "Part" ), _( "Revision" ), _( "Name" ), _( "Description" ) },
                         rows, preselect, false );
    dlg.SetOKLabel( _( "Assign" ) );

    if( dlg.ShowModal() != wxID_OK )
        return;

    auto chosen = byKey.find( dlg.GetTextSelection( 0 ) );

    if( chosen == byKey.end() )
        return;

    const MDW_PART_INFO& part = *chosen->second;
    symbol = find( path );

    if( !symbol )
        return;

    SCH_COMMIT commit( m_toolMgr );

    commit.Modify( symbol, path.LastScreen() );
    MdwWritePartField( *symbol, MDW_PART_IDENTITY{ part.part, part.revision_id } );

    // Every unit is one physical part, so the others must carry the same identity
    symbol->SyncOtherUnits( path, commit, nullptr );
    commit.Push( _( "Assign Part" ) );
}


void SCH_MDW_PART_TOOL::setTransitions()
{
    Go( &SCH_MDW_PART_TOOL::AssignPart, SCH_ACTIONS::mdwAssignPart.MakeEvent() );
}
