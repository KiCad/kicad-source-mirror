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


#include <dialogs/git/workspace_detail.h>

#include <mdw/client/mdw_lifecycle.h>
#include <mdw/mdw_claim_text.h>
#include <widgets/mdw_member_index.h>

#include <wx/intl.h>


wxString WorkspaceStateCell( const MDW_CAPABILITIES& aCaps, const MDW_LIFECYCLE_STATE& aState,
                             const wxString& aRevisionLabel )
{
    if( !aCaps.Has( MDW_CAPABILITY_GROUP::LIFECYCLE ) )
        return wxString();

    wxString name = MDW_MEMBER_INDEX::StateName( aCaps, aState );

    // Editable is the unremarkable default, so only a state the backend names is worth a cell
    if( aState.category == MDW_LIFECYCLE_CATEGORY::EDITABLE && name == MdwCategoryName( aState.category ) )
        return wxString();

    if( aState.category == MDW_LIFECYCLE_CATEGORY::RELEASED && !aRevisionLabel.IsEmpty() )
        return name + wxS( " \u00B7 " ) + aRevisionLabel;

    return name;
}


std::vector<WORKSPACE_DETAIL_ROW> WorkspaceDetailRows( const MDW_CAPABILITIES& aCaps, const MDW_ITEM_INFO& aItem,
                                                       const MDW_LOCK_INFO& aLock, std::optional<bool> aModified,
                                                       const wxString& aFolderPath, const wxDateTime& aNow )
{
    std::vector<WORKSPACE_DETAIL_ROW> rows;

    if( aCaps.Has( MDW_CAPABILITY_GROUP::LIFECYCLE ) )
        rows.push_back( { _( "Lifecycle" ), MDW_MEMBER_INDEX::StateName( aCaps, aItem.state ) } );

    if( aCaps.Has( MDW_CAPABILITY_GROUP::LOCKING ) )
    {
        wxString claim = MdwClaimLabel( aLock, aNow );
        rows.push_back( { _( "Editing" ), claim.IsEmpty() ? _( "nobody" ) : claim } );
    }

    if( aModified.has_value() )
        rows.push_back( { _( "Content" ), *aModified ? _( "Modified" ) : _( "Current" ) } );

    // The backend id is a hash, so a short prefix identifies a revision that has no label
    wxString version = aItem.revision_label.IsEmpty() ? aItem.revision_id.Left( 7 ) : aItem.revision_label;
    rows.push_back( { _( "Version" ), version } );

    rows.push_back( { _( "Folder" ), aFolderPath.IsEmpty() ? _( "(top)" ) : aFolderPath } );

    if( aItem.modified.IsValid() )
        rows.push_back( { _( "Last change" ), aItem.modified.Format( wxS( "%x %H:%M" ) ) } );

    return rows;
}
