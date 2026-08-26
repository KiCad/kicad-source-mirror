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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <widgets/mdw_part_actions.h>

#include <dialogs/dialog_mdw_library_checkin.h>
#include <dialogs/git/dialog_mdw_part_history.h>
#include <tool/actions.h>
#include <tool/conditional_menu.h>
#include <tool/selection_conditions.h>
#include <tool/tool_event.h>
#include <tool/tool_interactive.h>
#include <tool/tool_menu.h>
#include <widgets/mdw_editing.h>
#include <widgets/mdw_part_controller.h>
#include <widgets/wx_infobar.h>

#include <eda_base_frame.h>
#include <i18n_utility.h>
#include <string_utils.h>

#include <wx/control.h>
#include <wx/intl.h>

#include <algorithm>
#include <iterator>
#include <set>


namespace
{

/// The parts aSelected names, leaving out the libraries
std::vector<LIB_ID> partsOf( const std::vector<LIB_ID>& aSelected )
{
    std::vector<LIB_ID> parts;

    std::ranges::copy_if( aSelected, std::back_inserter( parts ),
                          []( const LIB_ID& aId )
                          {
                              return !aId.GetLibItemName().empty();
                          } );

    return parts;
}


void releaseParts( MDW_PART_EDITOR& aEditor, const std::vector<LIB_ID>& aParts )
{
    aEditor.RefreshPart();

    std::vector<MDW_RELEASE_ITEM> items;

    for( const LIB_ID& id : aParts )
    {
        std::optional<MDW_PART_TARGET> target = aEditor.ResolvePart( id );

        if( !target )
            continue;

        MDW_RELEASE_ITEM item;
        item.id = id;
        item.target = std::move( *target );
        item.name = UnescapeString( id.GetUniStringLibItemName() );

        if( aEditor.DescribeForRelease( item, aParts ) )
            items.push_back( std::move( item ) );
    }

    aEditor.TeamPart().ReleaseParts( aEditor.PartFrame(), std::move( items ) );
}


/// Shows aId for the parts of aSelected that aEditor offers it for
SELECTION_CONDITION offers( const std::function<MDW_PART_EDITOR*()>&    aEditor,
                            const std::function<std::vector<LIB_ID>()>& aSelected, MDW_PART_ACTION::ID aId )
{
    return [aEditor, aSelected, aId]( const SELECTION& )
    {
        MDW_PART_EDITOR*          editor = aEditor();
        const std::vector<LIB_ID> parts = partsOf( aSelected() );

        // Only a release or check-in takes several parts
        if( !editor || parts.empty()
            || ( parts.size() > 1 && aId != MDW_PART_ACTION::ID::RELEASE && aId != MDW_PART_ACTION::ID::CHECK_IN ) )
        {
            return false;
        }

        return MdwOfferedPartAction( *editor, parts, aId ).has_value();
    };
}


/// The library tree's Lifecycle submenu, whose claim release names the holder as the part infobar does
class LIFECYCLE_MENU : public CONDITIONAL_MENU
{
public:
    LIFECYCLE_MENU( TOOL_INTERACTIVE* aTool, std::function<MDW_PART_EDITOR*()> aEditor,
                    std::function<std::vector<LIB_ID>()> aSelected ) :
            CONDITIONAL_MENU( aTool ),
            m_editor( std::move( aEditor ) ),
            m_selected( std::move( aSelected ) )
    {
        SetUntranslatedTitle( _HKI( "Lifecycle" ) );

        for( const auto& [action, id] : MdwPartToolActions() )
            AddItem( *action, offers( m_editor, m_selected, id ) );
    }

protected:
    ACTION_MENU* create() const override { return new LIFECYCLE_MENU( m_tool, m_editor, m_selected ); }

    // The action's own label can name no one, since only the selected part knows who holds its claim
    void update() override
    {
        wxMenuItem*      item = FindChildItem( ACTIONS::teamPartReleaseClaim.GetUIId() );
        MDW_PART_EDITOR* editor = m_editor();

        if( !item || !editor )
            return;

        std::optional<MDW_PART_ACTION> release =
                MdwOfferedPartAction( *editor, partsOf( m_selected() ), MDW_PART_ACTION::ID::RELEASE_CLAIM );

        if( release )
            item->SetItemLabel( wxControl::EscapeMnemonics( MDW_PART_CONTROLLER::Label( *release ) ) );
    }

private:
    std::function<MDW_PART_EDITOR*()>    m_editor;
    std::function<std::vector<LIB_ID>()> m_selected;
};


void checkIn( MDW_PART_EDITOR& aEditor, const std::vector<LIB_ID>& aParts )
{
    std::set<wxString> libraries;

    // A library no team source serves has nothing to check in, so it is never asked to save for one
    for( const LIB_ID& id : aParts )
    {
        if( aEditor.ResolvePart( id ) )
            libraries.insert( id.GetUniStringLibNickname() );
    }

    for( const wxString& library : libraries )
        aEditor.CheckInLibrary( library );
}

} // namespace


const std::vector<std::pair<const TOOL_ACTION*, MDW_PART_ACTION::ID>>& MdwPartToolActions()
{
    using ID = MDW_PART_ACTION::ID;

    static const std::vector<std::pair<const TOOL_ACTION*, ID>> actions = {
        { &ACTIONS::teamPartRevise, ID::REVISE },
        { &ACTIONS::teamPartCancelRevision, ID::CANCEL_REVISION },
        { &ACTIONS::teamPartReleaseClaim, ID::RELEASE_CLAIM },
        { &ACTIONS::teamPartRelease, ID::RELEASE },
        { &ACTIONS::teamPartTransition, ID::TRANSITION },
        { &ACTIONS::teamPartCheckIn, ID::CHECK_IN },
        { &ACTIONS::teamPartHistory, ID::HISTORY },
    };

    return actions;
}


std::optional<MDW_PART_ACTION::ID> MdwPartActionOf( const TOOL_EVENT& aEvent )
{
    for( const auto& [action, id] : MdwPartToolActions() )
    {
        if( aEvent.IsAction( action ) )
            return id;
    }

    return std::nullopt;
}


std::optional<MDW_PART_ACTION> MdwOfferedPartAction( MDW_PART_EDITOR& aEditor, const std::vector<LIB_ID>& aParts,
                                                     MDW_PART_ACTION::ID aId )
{
    std::vector<MDW_PART_TARGET> targets;

    for( const LIB_ID& id : aParts )
    {
        if( std::optional<MDW_PART_TARGET> target = aEditor.ResolvePart( id ) )
            targets.push_back( std::move( *target ) );
    }

    return aEditor.TeamPart().Offered( targets, aId );
}


void MdwRunPartAction( MDW_PART_EDITOR& aEditor, MDW_PART_ACTION::ID aId, const std::vector<LIB_ID>& aParts,
                       const wxString& aState )
{
    using ID = MDW_PART_ACTION::ID;

    if( aParts.empty() )
        return;

    if( aId == ID::RELEASE )
    {
        releaseParts( aEditor, aParts );
        return;
    }

    if( aId == ID::CHECK_IN )
    {
        checkIn( aEditor, aParts );
        return;
    }

    const LIB_ID&                  id = aParts.front();
    std::optional<MDW_PART_ACTION> offered = MdwOfferedPartAction( aEditor, { id }, aId );

    if( !offered )
        return;

    if( !offered->Allowed() )
    {
        MdwReportFailure( aEditor.PartFrame(), MDW_PART_CONTROLLER::Refusal( *offered ) );
        return;
    }

    // The part's own actions work on what the editor shows
    if( !aEditor.OpenPart( id ) )
        return;

    MDW_PART_CONTROLLER& part = aEditor.TeamPart();

    switch( aId )
    {
    case ID::REVISE: part.Revise(); break;
    case ID::CANCEL_REVISION: part.CancelRevision(); break;
    case ID::RELEASE_CLAIM: part.ReleaseClaim(); break;
    case ID::TRANSITION: part.Transition( aState ); break;

    case ID::HISTORY:
        if( part.Target() )
            ShowMdwPartHistory( aEditor.PartFrame(), *part.Target(), UnescapeString( id.GetUniStringLibItemName() ) );

        break;

    default: break;
    }
}


void MdwUpdatePartLibrary( MDW_PART_EDITOR& aEditor, const wxString& aNickname )
{
    EDA_BASE_FRAME* frame = aEditor.PartFrame();

    UpdateTeamLibrarySource( frame, frame->Kiway(), &frame->Prj(), aEditor.PartTableType(), aNickname );
}


void MdwCheckInPartLibrary( MDW_PART_EDITOR& aEditor, const wxString& aNickname, bool aModified,
                            const std::function<bool()>& aSave )
{
    EDA_BASE_FRAME* frame = aEditor.PartFrame();

    SaveAndCheckInTeamLibrary( frame, frame->Kiway(), aEditor.PartTableType(), aNickname, aModified, aSave );
}


bool MdwOpenPartOnce( const std::function<bool()>& aIsOpen, const std::function<void()>& aLoad )
{
    if( !aIsOpen() )
        aLoad();

    return aIsOpen();
}


void MdwRunPartToolAction( MDW_PART_EDITOR& aEditor, const TOOL_EVENT& aEvent, const std::vector<LIB_ID>& aSelected )
{
    // Only Change State carries a parameter, the state to move to
    const wxString state = aEvent.ParameterIf<wxString>().value_or( wxString() );

    if( std::optional<MDW_PART_ACTION::ID> id = MdwPartActionOf( aEvent ) )
        MdwRunPartAction( aEditor, *id, partsOf( aSelected ), state );
}


void MdwAddLifecycleMenu( TOOL_INTERACTIVE* aTool, TOOL_MENU& aToolMenu, CONDITIONAL_MENU& aContextMenu,
                          std::function<MDW_PART_EDITOR*()> aEditor, std::function<std::vector<LIB_ID>()> aSelected )
{
    auto lifecycleMenu = std::make_shared<LIFECYCLE_MENU>( aTool, aEditor, aSelected );
    aToolMenu.RegisterSubMenu( lifecycleMenu );

    SELECTION_CONDITION anyOffered = SELECTION_CONDITIONS::ShowNever;

    for( const auto& [action, id] : MdwPartToolActions() )
        anyOffered = anyOffered || offers( aEditor, aSelected, id );

    aContextMenu.AddMenu( lifecycleMenu.get(), anyOffered, 10 );
}


void MdwAddPartLinks( WX_INFOBAR& aInfoBar, wxArrayString& aMessages, MDW_PART_EDITOR& aEditor,
                      const std::function<void( const MDW_PART_ACTION& )>& aRun )
{
    const MDW_PART_CONTROLLER& part = aEditor.TeamPart();
    const wxString             message = part.Message();

    if( message.IsEmpty() )
        return;

    aMessages.push_back( message );

    // Check-in and release share a backend's write permission, so one warning would show twice
    auto note = [&]( const wxString& aLine )
    {
        if( !aLine.IsEmpty() && aMessages.Index( aLine ) == wxNOT_FOUND )
            aMessages.push_back( aLine );
    };

    auto link = [&]( const MDW_PART_ACTION& aAction )
    {
        aInfoBar.AddLink( MDW_PART_CONTROLLER::Label( aAction ),
                          [aRun, aAction]( wxHyperlinkEvent& )
                          {
                              aRun( aAction );
                          } );
    };

    std::vector<MDW_PART_ACTION> seldom;

    for( const MDW_PART_ACTION& action : part.Actions() )
    {
        if( !action.Allowed() )
        {
            note( MDW_PART_CONTROLLER::Refusal( action ) );
            continue;
        }

        note( action.reason );

        if( action.id == MDW_PART_ACTION::ID::TRANSITION || action.id == MDW_PART_ACTION::ID::HISTORY )
            seldom.push_back( action );
        else
            link( action );
    }

    if( part.OffersGetLatest() )
    {
        aInfoBar.AddLink( _( "Get Latest..." ),
                          [&aEditor]( wxHyperlinkEvent& )
                          {
                              aEditor.TeamPart().GetLatest();
                          } );
    }

    // State changes and history are seldom wanted, so several of them share one menu
    if( seldom.size() == 1 )
    {
        link( seldom.front() );
    }
    else if( !seldom.empty() )
    {
        std::vector<WX_INFOBAR::MENU_LINK_ITEM> items;

        for( const MDW_PART_ACTION& action : seldom )
        {
            items.emplace_back( MDW_PART_CONTROLLER::Label( action ),
                                [aRun, action]()
                                {
                                    aRun( action );
                                } );
        }

        aInfoBar.AddMenuLink( _( "More" ), items );
    }
}
