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

#include <mdw/client/mdw_event_notice.h>
#include <mdw/mdw_claim_text.h>

#include <kiid.h>

#include <wx/intl.h>


std::optional<MDW_NOTICE> MdwEventNotice( const MDW_EVENT& aEvent, const wxString& aProjectSource )
{
    if( aEvent.from_this_client )
        return std::nullopt;

    const bool project = !aProjectSource.IsEmpty() && aEvent.source == aProjectSource;

    // A team library's check-ins would flood the bell, but losing a claim on one of its parts is news
    if( !project && aEvent.type != MDW_EVENT_TYPE::ITEM_UNLOCKED )
        return std::nullopt;

    wxString                  who = aEvent.client_identity.IsEmpty() ? _( "another user" ) : aEvent.client_identity;
    wxString                  name = aEvent.path_hint.IsEmpty() ? aEvent.item_id : aEvent.path_hint;
    std::optional<MDW_NOTICE> notice;

    switch( aEvent.type )
    {
    case MDW_EVENT_TYPE::ITEM_UNLOCKED:
        if( std::optional<wxString> released = MdwClaimNotice( aEvent ) )
            notice = MDW_NOTICE{ wxString(), _( "Editing claim released" ), *released };

        break;

    case MDW_EVENT_TYPE::REVISION_CHECKED_IN:
        notice = MDW_NOTICE{ wxString(), _( "New revision" ),
                             wxString::Format( _( "%s was checked in by %s." ), name, who ) };
        break;

    case MDW_EVENT_TYPE::LIFECYCLE_CATEGORY_CHANGED:
        notice = MDW_NOTICE{ wxString(), _( "Lifecycle changed" ),
                             wxString::Format( _( "The lifecycle state of %s changed." ), name ) };
        break;

    default: break;
    }

    if( notice )
    {
        // Keyed by source, so the same item in another workspace keeps its own notice
        notice->key = wxString::Format( wxS( "mdw-%s-%s" ), KIID::FromName( aEvent.source.utf8_string() ).AsString(),
                                        aEvent.item_id );
    }

    return notice;
}
