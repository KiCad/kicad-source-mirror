/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <widgets/mdw_library_notice.h>

#include <dialogs/dialog_library_source_setup.h>
#include <eda_base_frame.h>
#include <kiway_holder.h>
#include <mdw/client/mdw_client.h>
#include <mdw/client/mdw_library_sources.h>
#include <project.h>
#include <project/project_file.h>
#include <string_utils.h>
#include <widgets/wx_infobar.h>

#include <wx/hyperlink.h>
#include <wx/weakref.h>
#include <wx/intl.h>

#include <algorithm>


void ShowTeamLibraryNotice( EDA_BASE_FRAME* aFrame, PROJECT& aProject )
{
    WX_INFOBAR* infobar = aFrame ? aFrame->GetInfoBar() : nullptr;

    if( !infobar )
        return;

    const std::vector<MDW_LIBRARY_REQUIREMENT>& needs = aProject.GetProjectFile().m_WorkspaceLibrarySources;
    const bool                                  enabled = MDW_LIBRARY_SOURCES::TeamLibrariesEnabled();
    MDW_LIBRARY_STATUS                          status = MDW_SETUP::LibraryStatus( needs, enabled );

    if( status.notice == MDW_LIBRARY_NOTICE::NONE )
    {
        if( infobar->GetMessageType() == WX_INFOBAR::MESSAGE_TYPE::TEAM_LIBRARIES )
            infobar->Dismiss();

        return;
    }

    // Another message, such as an older-format or read-only warning, goes first and this one follows once it is
    // dismissed or cleared
    if( infobar->IsShown() && infobar->GetMessageType() != WX_INFOBAR::MESSAGE_TYPE::TEAM_LIBRARIES )
    {
        infobar->CallAfterDismiss(
                [frame = wxWeakRef<EDA_BASE_FRAME>( aFrame )]()
                {
                    if( frame && !frame->IsBeingDeleted() )
                        ShowTeamLibraryNotice( frame, frame->Prj() );
                } );

        return;
    }

    wxString                             names;
    std::vector<MDW_LIBRARY_REQUIREMENT> needed;

    for( const MDW_REQUIREMENT_STATE& state : status.states )
    {
        needed.push_back( state.requirement );

        AccumulateDescription( names, state.requirement.name.IsEmpty() ? state.requirement.locator
                                                                       : state.requirement.name );
    }

    wxString message;

    infobar->RemoveAllButtons();

    switch( status.notice )
    {
    case MDW_LIBRARY_NOTICE::DISABLED:
        message = _( "This project uses team libraries. Turn on Design Workspace integration to use them." );

        infobar->AddLink( _( "Preferences..." ),
                          [aFrame]( wxHyperlinkEvent& )
                          {
                              aFrame->ShowPreferences( _( "Design Workspace" ), wxEmptyString );
                              ShowTeamLibraryNotice( aFrame, aFrame->Prj() );
                          } );
        break;

    case MDW_LIBRARY_NOTICE::NOT_SET_UP:
        message =
                wxString::Format( _( "Team libraries this project uses are not set up on this computer: %s." ), names );

        // The project is looked up again on click, since the frame may have opened another since
        infobar->AddLink( _( "Set Up..." ),
                          [aFrame, requirements = needed]( wxHyperlinkEvent& )
                          {
                              SetUpLibrarySource( aFrame, aFrame->Kiway(), &aFrame->Prj(), requirements );
                              ShowTeamLibraryNotice( aFrame, aFrame->Prj() );
                          } );
        break;

    case MDW_LIBRARY_NOTICE::NOT_FETCHED:
        message = wxString::Format( _( "Team libraries this project uses have not been fetched: %s." ), names );

        infobar->AddLink( _( "Retry" ),
                          [aFrame]( wxHyperlinkEvent& )
                          {
                              UpdateLibrarySources( aFrame, aFrame->Kiway(), &aFrame->Prj(),
                                                    ProjectLibrarySources( aFrame->Prj() ) );
                              ShowTeamLibraryNotice( aFrame, aFrame->Prj() );
                          } );
        break;

    case MDW_LIBRARY_NOTICE::NONE: break;
    }

    infobar->AddCloseButton();
    infobar->ShowMessage( message, wxICON_INFORMATION, WX_INFOBAR::MESSAGE_TYPE::TEAM_LIBRARIES );
}


namespace
{

void showLibraryUpdates( EDA_BASE_FRAME* aFrame )
{
    WX_INFOBAR* infobar = aFrame->GetInfoBar();

    if( !infobar )
        return;

    // Another message goes first, as the other team-library notices let it
    if( infobar->IsShown() && infobar->GetMessageType() != WX_INFOBAR::MESSAGE_TYPE::TEAM_LIBRARIES )
    {
        infobar->CallAfterDismiss(
                [frame = wxWeakRef<EDA_BASE_FRAME>( aFrame )]()
                {
                    if( frame && !frame->IsBeingDeleted() )
                        showLibraryUpdates( frame );
                } );

        return;
    }

    infobar->RemoveAllButtons();

    infobar->AddLink( _( "Get Library Updates" ),
                      [aFrame]( wxHyperlinkEvent& )
                      {
                          UpdateLibrarySources( aFrame, aFrame->Kiway(), &aFrame->Prj(),
                                                ProjectLibrarySources( aFrame->Prj() ) );
                          ShowTeamLibraryNotice( aFrame, aFrame->Prj() );
                      } );

    infobar->AddCloseButton();
    infobar->ShowMessage( _( "Library updates available" ), wxICON_INFORMATION,
                          WX_INFOBAR::MESSAGE_TYPE::TEAM_LIBRARIES );
}

} // namespace


MDW_SUBSCRIPTION WatchTeamLibraryUpdates( EDA_BASE_FRAME* aFrame )
{
    return MDW_CLIENT::Get().Subscribe(
            aFrame,
            [aFrame]( const MDW_EVENT& aEvent )
            {
                // The client announces its own syncs too, which leave nothing to get
                if( aEvent.type != MDW_EVENT_TYPE::LIBRARY_CHANGED || aEvent.from_this_client )
                    return;

                const std::vector<MDW_LIBRARY_SOURCE_CONFIG> sources = ProjectLibrarySources( aFrame->Prj() );

                if( std::any_of( sources.begin(), sources.end(),
                                 [&]( const MDW_LIBRARY_SOURCE_CONFIG& aSource )
                                 {
                                     return aSource.locator == aEvent.source;
                                 } ) )
                {
                    showLibraryUpdates( aFrame );
                }
            } );
}
