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

#include <mdw/client/mdw_library.h>

#include "mdw_call.h"
#include "mdw_op_state.h"

#include <mdw/client/mdw_source.h>
#include <mdw/mdw_proto_convert.h>

#include <api/mdw/backend_shared_libs.pb.h>

#include <utility>


namespace mdw = kiapi::mdw;


namespace
{

MDW_SNAPSHOT toSnapshot( const mdw::Snapshot& aProto )
{
    MDW_SNAPSHOT snapshot{ MdwFromUtf8( aProto.token() ), MdwFromUtf8( aProto.label() ), {} };

    for( const std::string& warning : aProto.warnings() )
        snapshot.warnings.push_back( MdwFromUtf8( warning ) );

    return snapshot;
}


std::vector<MDW_LIBRARY_INFO> toLibraries( const google::protobuf::RepeatedPtrField<mdw::SharedLibrary>& aProto )
{
    std::vector<MDW_LIBRARY_INFO> libraries;

    for( const mdw::SharedLibrary& proto : aProto )
    {
        MDW_LIBRARY_INFO& library = libraries.emplace_back();
        library.library_id = MdwFromUtf8( proto.library_id() );
        library.display_name = MdwFromUtf8( proto.display_name() );
        library.type = MdwFromProto( proto.type() );
        library.relative_path = MdwFromUtf8( proto.relative_path() );
    }

    return libraries;
}


std::vector<MDW_LIBRARY_MEMBER> toMembers( const google::protobuf::RepeatedPtrField<mdw::LibraryMember>& aProto )
{
    std::vector<MDW_LIBRARY_MEMBER> members;

    for( const mdw::LibraryMember& proto : aProto )
    {
        MDW_LIBRARY_MEMBER& member = members.emplace_back();
        member.member_id = MdwFromUtf8( proto.member_id() );
        member.library_id = MdwFromUtf8( proto.library_id() );
        member.name = MdwFromUtf8( proto.name() );
        member.kind = MdwFromProto( proto.kind() );
        member.path_hint = MdwFromUtf8( proto.path_hint() );
        member.revision_id = MdwFromUtf8( proto.revision_id() );
        member.revision_label = MdwFromUtf8( proto.revision_label() );
        member.state = MdwToStruct( proto.state() );
        member.version = MdwFromUtf8( proto.version() );
        member.release_digest = MdwFromUtf8( proto.release_digest() );

        for( const mdw::MemberRelease& release : proto.releases() )
        {
            member.releases.push_back( { MdwFromUtf8( release.revision_id() ), MdwFromUtf8( release.label() ),
                                         MdwFromUtf8( release.content_digest() ), MdwToStruct( release.state() ),
                                         release.published() } );
        }
    }

    return members;
}


} // namespace


MDW_LIBRARY::MDW_LIBRARY( std::shared_ptr<MDW_SOURCE> aSource ) :
        m_source( std::move( aSource ) )
{
}


MDW_OP MDW_LIBRARY::List( MDW_DONE<MDW_LIBRARY_LIST> aDone )
{
    return MdwCall<mdw::ListSharedLibrariesRequest, mdw::ListSharedLibrariesReply, MDW_LIBRARY_LIST>(
            m_source, MDW_CAPABILITY_GROUP::SHARED_LIBRARIES, mdw::ListSharedLibrariesRequest(),
            []( const mdw::ListSharedLibrariesReply& aReply )
            {
                return MDW_LIBRARY_LIST{ toLibraries( aReply.libraries() ), toSnapshot( aReply.latest() ), {} };
            },
            std::move( aDone ) );
}


MDW_OP MDW_LIBRARY::Materialize( const wxString& aLocalRoot, const wxString& aPin, MDW_DONE<MDW_LIBRARY_LIST> aDone )
{
    mdw::MaterializeSharedLibrariesRequest request;
    request.set_local_root( MdwToUtf8( aLocalRoot ) );
    request.set_pin( MdwToUtf8( aPin ) );

    MDW_TRACKED_OP op = m_source ? MdwBeginOperation( *m_source ) : MDW_TRACKED_OP();
    request.set_idempotency_id( MdwToUtf8( op.id ) );

    return MdwCall<mdw::MaterializeSharedLibrariesRequest, mdw::MaterializeSharedLibrariesReply, MDW_LIBRARY_LIST>(
            op, m_source, MDW_CAPABILITY_GROUP::SHARED_LIBRARIES, request,
            [pin = aPin]( const mdw::MaterializeSharedLibrariesReply& aReply, MDW_LIBRARY_LIST& aList )
            {
                aList = { toLibraries( aReply.libraries() ), toSnapshot( aReply.snapshot() ),
                          toMembers( aReply.members() ) };

                // A copy that cannot say what it holds cannot be pinned or updated later
                return !aList.snapshot.token.IsEmpty() && ( pin.IsEmpty() || aList.snapshot.token == pin );
            },
            std::move( aDone ),
            [op]( const MDW_RESULT& aResult, const MDW_LIBRARY_LIST& )
            {
                if( !aResult.IsOk() )
                    op.Abandon( aResult );
            },
            MDW_TRANSFER_TIMEOUT_MS );
}


MDW_OP MDW_LIBRARY::Update( const wxString& aLocalRoot, const std::vector<MDW_CONFLICT_RESOLUTION>& aResolutions,
                            MDW_DONE<MDW_LIBRARY_UPDATE> aDone )
{
    mdw::UpdateSharedLibrariesRequest request;
    request.set_local_root( MdwToUtf8( aLocalRoot ) );

    for( const MDW_CONFLICT_RESOLUTION& resolution : aResolutions )
    {
        mdw::ConflictResolution* proto = request.add_resolutions();
        proto->set_item_id( MdwToUtf8( resolution.item_id ) );
        proto->set_theirs_revision_id( MdwToUtf8( resolution.theirs_revision_id ) );

        if( resolution.merged )
            proto->set_merged( *resolution.merged );

        proto->set_ours_token( MdwToUtf8( resolution.ours_token ) );
        proto->set_whole_file_of( MdwToUtf8( resolution.whole_file_of ) );
    }

    MDW_TRACKED_OP op = m_source ? MdwBeginOperation( *m_source ) : MDW_TRACKED_OP();
    request.set_idempotency_id( MdwToUtf8( op.id ) );

    wxString locator = m_source ? m_source->Info().locator : wxString();

    return MdwCall<mdw::UpdateSharedLibrariesRequest, mdw::UpdateSharedLibrariesReply, MDW_LIBRARY_UPDATE>(
            op, m_source, MDW_CAPABILITY_GROUP::SHARED_LIBRARIES, request,
            [locator]( const mdw::UpdateSharedLibrariesReply& aReply, MDW_LIBRARY_UPDATE& aUpdate )
            {
                aUpdate.snapshot = toSnapshot( aReply.snapshot() );
                aUpdate.libraries = toLibraries( aReply.libraries() );
                aUpdate.members = toMembers( aReply.members() );

                for( const mdw::MemberChange& proto : aReply.changes() )
                {
                    MDW_MEMBER_CHANGE& change = aUpdate.changes.emplace_back();
                    change.member = MdwToStruct( proto.member(), locator );
                    change.change = MdwFromProto( proto.change() );
                    change.old_revision_label = MdwFromUtf8( proto.old_revision_label() );
                    change.new_revision_label = MdwFromUtf8( proto.new_revision_label() );
                }

                for( const mdw::UpdateConflict& proto : aReply.conflicts() )
                {
                    // Both empty leaves nothing to merge against
                    if( proto.base_revision_id().empty() && proto.theirs_revision_id().empty() )
                        return false;

                    MDW_UPDATE_CONFLICT& conflict = aUpdate.conflicts.emplace_back();
                    conflict.item = MdwToStruct( proto.item(), locator );
                    conflict.kind = MdwFromProto( proto.kind() );
                    conflict.base_revision_id = MdwFromUtf8( proto.base_revision_id() );
                    conflict.theirs_revision_id = MdwFromUtf8( proto.theirs_revision_id() );
                    conflict.relative_path = MdwFromUtf8( proto.relative_path() );
                    conflict.name = MdwFromUtf8( proto.name() );
                    conflict.ours_token = MdwFromUtf8( proto.ours_token() );
                }

                return !aUpdate.conflicts.empty() || !aUpdate.snapshot.token.IsEmpty();
            },
            std::move( aDone ),
            [op]( const MDW_RESULT& aResult, const MDW_LIBRARY_UPDATE& )
            {
                if( !aResult.IsOk() )
                    op.Abandon( aResult );
            },
            MDW_TRANSFER_TIMEOUT_MS );
}


MDW_OP MDW_LIBRARY::Members( const wxString& aLibraryId, const wxString& aSnapshot, const MDW_PAGE& aPage,
                             MDW_DONE<MDW_PAGED<MDW_LIBRARY_MEMBER>> aDone )
{
    mdw::ListMembersRequest request;
    request.set_library_id( MdwToUtf8( aLibraryId ) );
    request.set_snapshot( MdwToUtf8( aSnapshot ) );
    MdwToProto( request.mutable_page(), aPage );

    return MdwCall<mdw::ListMembersRequest, mdw::ListMembersReply, MDW_PAGED<MDW_LIBRARY_MEMBER>>(
            m_source, MDW_CAPABILITY_GROUP::SHARED_LIBRARIES, request,
            []( const mdw::ListMembersReply& aReply )
            {
                MDW_PAGED<MDW_LIBRARY_MEMBER> page;
                page.rows = toMembers( aReply.members() );
                page.next_page_token = MdwFromUtf8( aReply.next_page_token() );
                return page;
            },
            std::move( aDone ) );
}


MDW_OP MDW_LIBRARY::LocalChanges( const wxString& aLocalRoot, const MDW_PAGE& aPage,
                                  MDW_DONE<MDW_PAGED<MDW_LOCAL_CHANGE>> aDone )
{
    if( !m_source )
        return MdwNotSupported<MDW_PAGED<MDW_LOCAL_CHANGE>>( std::move( aDone ) );

    return m_source->LocalChanges( aLocalRoot, aPage, std::move( aDone ) );
}


MDW_OP MDW_LIBRARY::CheckIn( const wxString& aLocalRoot, const std::vector<wxString>& aItemIds,
                             const wxString& aComment, const std::map<wxString, wxString>& aWholeFiles,
                             bool aFormatUpgrade, MDW_DONE<MDW_LIBRARY_CHECKIN> aDone )
{
    mdw::CheckInSharedLibraryRequest request;
    request.set_local_root( MdwToUtf8( aLocalRoot ) );
    request.set_comment( MdwToUtf8( aComment ) );
    request.set_format_upgrade( aFormatUpgrade );

    for( const wxString& itemId : aItemIds )
        request.add_item_ids( MdwToUtf8( itemId ) );

    for( const auto& [file, digest] : aWholeFiles )
        ( *request.mutable_whole_files() )[MdwToUtf8( file )] = MdwToUtf8( digest );

    MDW_TRACKED_OP op = m_source ? MdwBeginOperation( *m_source ) : MDW_TRACKED_OP();
    request.set_idempotency_id( MdwToUtf8( op.id ) );

    return MdwCall<mdw::CheckInSharedLibraryRequest, mdw::CheckInSharedLibraryReply, MDW_LIBRARY_CHECKIN>(
            op, m_source, MDW_CAPABILITY_GROUP::SHARED_LIBRARIES, request,
            []( const mdw::CheckInSharedLibraryReply& aReply, MDW_LIBRARY_CHECKIN& aCheckIn )
            {
                if( aReply.has_snapshot() )
                    aCheckIn.snapshot = toSnapshot( aReply.snapshot() );

                if( aReply.has_copy_snapshot() )
                    aCheckIn.copy_snapshot = toSnapshot( aReply.copy_snapshot() );

                for( const mdw::WholeFileSkip& skipped : aReply.skipped_files() )
                {
                    aCheckIn.skipped_files.push_back(
                            { MdwFromUtf8( skipped.path() ), MdwFromUtf8( skipped.reason() ) } );
                }

                for( const mdw::MemberResult& proto : aReply.results() )
                {
                    MDW_MEMBER_RESULT& result = aCheckIn.results.emplace_back();
                    result.library_id = MdwFromUtf8( proto.library_id() );
                    result.name = MdwFromUtf8( proto.name() );
                    result.item_id = MdwFromUtf8( proto.item_id() );

                    if( proto.has_error() && proto.error().code() != kiapi::workspace::WE_OK )
                    {
                        result.status = MdwToResult( proto.error() );
                        continue;
                    }

                    result.status = MDW_RESULT::Ok();
                    result.change.outcome = MdwFromProto( proto.outcome() );
                    result.change.revision = MdwToStruct( proto.revision() );
                    result.change.pending = MdwToStruct( proto.pending() );

                    // A member must say what it made, or which change will decide it
                    bool applied = result.change.outcome == MDW_CHANGE_OUTCOME::APPLIED && !result.item_id.IsEmpty()
                                   && !result.change.revision.revision_id.IsEmpty();
                    bool pending = result.change.outcome == MDW_CHANGE_OUTCOME::PENDING
                                   && !result.change.pending.change_id.IsEmpty();

                    if( !applied && !pending )
                        return false;
                }

                return true;
            },
            std::move( aDone ),
            [op]( const MDW_RESULT& aResult, const MDW_LIBRARY_CHECKIN& aCheckIn )
            {
                if( !aResult.IsOk() )
                {
                    op.Abandon( aResult );
                    return;
                }

                bool applied = false;

                for( const MDW_MEMBER_RESULT& result : aCheckIn.results )
                {
                    if( result.status.IsOk() && result.change.outcome == MDW_CHANGE_OUTCOME::APPLIED )
                    {
                        op.Emit( MDW_EVENT_TYPE::REVISION_CHECKED_IN, result.item_id );
                        applied = true;
                    }
                }

                // Nothing applied, so no echo is coming
                if( !applied )
                    op.Abandon( aResult );
            } );
}
