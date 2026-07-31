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

/**
 * @file mdw_proto_convert.h
 * @brief The one translation between kiapi.workspace protobufs and the
 *        framework's wxString value structs.
 *
 * Keeping both directions of a type together stops one side gaining a field the other never learns about.
 */

#ifndef MDW_PROTO_CONVERT_H
#define MDW_PROTO_CONVERT_H

#include <mdw/client/mdw_auth.h>
#include <mdw/client/mdw_baseline.h>
#include <mdw/client/mdw_capabilities.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_events.h>
#include <mdw/client/mdw_item.h>
#include <mdw/client/mdw_library.h>
#include <mdw/client/mdw_lock.h>
#include <mdw/client/mdw_navigation.h>
#include <mdw/client/mdw_revision.h>
#include <mdw/client/mdw_search.h>
#include <mdw/client/mdw_types.h>

#include <api/mdw/backend_base.pb.h>
#include <api/mdw/backend_revisions.pb.h>
#include <api/mdw/backend_shared_libs.pb.h>
#include <api/workspace/workspace_types.pb.h>

#include <cstdint>
#include <string>

#include <wx/datetime.h>
#include <wx/string.h>


// Framework enums cross the wire only through these switches, so the proto alone carries the numbering

/// An unset or unknown category may forbid editing, so it reads as the most guarded we know
inline MDW_LIFECYCLE_CATEGORY MdwFromProto( kiapi::workspace::LifecycleCategory aCategory )
{
    switch( aCategory )
    {
    case kiapi::workspace::LC_EDITABLE:       return MDW_LIFECYCLE_CATEGORY::EDITABLE;
    case kiapi::workspace::LC_PENDING_REVIEW: return MDW_LIFECYCLE_CATEGORY::PENDING_REVIEW;
    case kiapi::workspace::LC_RELEASED:       return MDW_LIFECYCLE_CATEGORY::RELEASED;
    case kiapi::workspace::LC_RESTRICTED:
    case kiapi::workspace::LC_UNKNOWN:
    default:                                  return MDW_LIFECYCLE_CATEGORY::RESTRICTED;
    }
}


inline kiapi::workspace::LifecycleCategory MdwToProto( MDW_LIFECYCLE_CATEGORY aCategory )
{
    switch( aCategory )
    {
    case MDW_LIFECYCLE_CATEGORY::EDITABLE:       return kiapi::workspace::LC_EDITABLE;
    case MDW_LIFECYCLE_CATEGORY::PENDING_REVIEW: return kiapi::workspace::LC_PENDING_REVIEW;
    case MDW_LIFECYCLE_CATEGORY::RESTRICTED:     return kiapi::workspace::LC_RESTRICTED;
    case MDW_LIFECYCLE_CATEGORY::RELEASED:       return kiapi::workspace::LC_RELEASED;
    }

    return kiapi::workspace::LC_RESTRICTED;
}


/// An unknown change outcome must not read as applied
inline MDW_CHANGE_OUTCOME MdwFromProto( kiapi::workspace::ChangeOutcome aOutcome )
{
    switch( aOutcome )
    {
    case kiapi::workspace::CO_UNKNOWN: return MDW_CHANGE_OUTCOME::UNKNOWN;
    case kiapi::workspace::CO_APPLIED: return MDW_CHANGE_OUTCOME::APPLIED;
    case kiapi::workspace::CO_PENDING:
    default:                           return MDW_CHANGE_OUTCOME::PENDING;
    }
}


/// An absent code is no success, and one from a newer protocol reads as INTERNAL keeping its message
inline MDW_ERROR_CODE MdwFromProto( kiapi::workspace::WorkspaceErrorCode aCode )
{
    switch( aCode )
    {
    case kiapi::workspace::WE_OK:                       return MDW_ERROR_CODE::SUCCESS;
    case kiapi::workspace::WE_NOT_SUPPORTED:            return MDW_ERROR_CODE::NOT_SUPPORTED;
    case kiapi::workspace::WE_NOT_AUTHENTICATED:        return MDW_ERROR_CODE::NOT_AUTHENTICATED;
    case kiapi::workspace::WE_AUTH_FAILED:              return MDW_ERROR_CODE::AUTH_FAILED;
    case kiapi::workspace::WE_PERMISSION_DENIED:        return MDW_ERROR_CODE::PERMISSION_DENIED;
    case kiapi::workspace::WE_NETWORK:                  return MDW_ERROR_CODE::NETWORK;
    case kiapi::workspace::WE_TIMEOUT:                  return MDW_ERROR_CODE::TIMEOUT;
    case kiapi::workspace::WE_LOCK_CONFLICT:            return MDW_ERROR_CODE::LOCK_CONFLICT;
    case kiapi::workspace::WE_NOT_LOCK_OWNER:           return MDW_ERROR_CODE::NOT_LOCK_OWNER;
    case kiapi::workspace::WE_ITEM_NOT_FOUND:           return MDW_ERROR_CODE::ITEM_NOT_FOUND;
    case kiapi::workspace::WE_REVISION_NOT_FOUND:       return MDW_ERROR_CODE::REVISION_NOT_FOUND;
    case kiapi::workspace::WE_VERSION_CONFLICT:         return MDW_ERROR_CODE::VERSION_CONFLICT;
    case kiapi::workspace::WE_INVALID_STATE_TRANSITION: return MDW_ERROR_CODE::INVALID_STATE_TRANSITION;
    case kiapi::workspace::WE_NO_CHANGES:               return MDW_ERROR_CODE::NO_CHANGES;
    case kiapi::workspace::WE_CANCELLED:                return MDW_ERROR_CODE::CANCELLED;
    case kiapi::workspace::WE_INVALID_PARAMETER:        return MDW_ERROR_CODE::INVALID_PARAMETER;
    case kiapi::workspace::WE_BACKEND_LOST:             return MDW_ERROR_CODE::BACKEND_LOST;
    case kiapi::workspace::WE_PROTOCOL:                 return MDW_ERROR_CODE::PROTOCOL;
    case kiapi::workspace::WE_RATE_LIMITED:             return MDW_ERROR_CODE::RATE_LIMITED;
    case kiapi::workspace::WE_BUSY:                     return MDW_ERROR_CODE::BUSY;
    case kiapi::workspace::WE_UNTRUSTED_HOST:           return MDW_ERROR_CODE::UNTRUSTED_HOST;
    case kiapi::workspace::WE_COPY_BEHIND:              return MDW_ERROR_CODE::COPY_BEHIND;
    case kiapi::workspace::WE_INTERNAL:
    case kiapi::workspace::WE_UNKNOWN:
    default:                                            return MDW_ERROR_CODE::INTERNAL;
    }
}


/// An unset model reads as an upload, which assumes nothing of the disk
inline MDW_CHECKIN_MODEL MdwFromProto( kiapi::workspace::CheckinModel aModel )
{
    switch( aModel )
    {
    case kiapi::workspace::CM_WORKING_COPY: return MDW_CHECKIN_MODEL::WORKING_COPY;
    case kiapi::workspace::CM_UPLOAD:
    case kiapi::workspace::CM_UNKNOWN:
    default:                                return MDW_CHECKIN_MODEL::UPLOAD;
    }
}


/// A group from a newer protocol reads as UNKNOWN, which no caller can use
inline MDW_CAPABILITY_GROUP MdwFromProto( kiapi::workspace::CapabilityGroup aGroup )
{
    switch( aGroup )
    {
    case kiapi::workspace::CG_LOCKING:          return MDW_CAPABILITY_GROUP::LOCKING;
    case kiapi::workspace::CG_REVISIONS:        return MDW_CAPABILITY_GROUP::REVISIONS;
    case kiapi::workspace::CG_LIFECYCLE:        return MDW_CAPABILITY_GROUP::LIFECYCLE;
    case kiapi::workspace::CG_EVENTS:           return MDW_CAPABILITY_GROUP::EVENTS;
    case kiapi::workspace::CG_SEARCH:           return MDW_CAPABILITY_GROUP::SEARCH;
    case kiapi::workspace::CG_SHARED_LIBRARIES: return MDW_CAPABILITY_GROUP::SHARED_LIBRARIES;
    case kiapi::workspace::CG_BOM:              return MDW_CAPABILITY_GROUP::BOM;
    case kiapi::workspace::CG_ECO:              return MDW_CAPABILITY_GROUP::ECO;
    case kiapi::workspace::CG_REVIEW:           return MDW_CAPABILITY_GROUP::REVIEW;
    case kiapi::workspace::CG_PARTS:            return MDW_CAPABILITY_GROUP::PARTS;
    case kiapi::workspace::CG_PERMISSIONS:      return MDW_CAPABILITY_GROUP::PERMISSIONS;
    case kiapi::workspace::CG_UNKNOWN:
    default:                                    return MDW_CAPABILITY_GROUP::UNKNOWN;
    }
}


inline MDW_DOCUMENT_KIND MdwFromProto( kiapi::workspace::DocumentKind aKind )
{
    switch( aKind )
    {
    case kiapi::workspace::DK_PROJECT:        return MDW_DOCUMENT_KIND::PROJECT;
    case kiapi::workspace::DK_SCHEMATIC:      return MDW_DOCUMENT_KIND::SCHEMATIC;
    case kiapi::workspace::DK_BOARD:          return MDW_DOCUMENT_KIND::BOARD;
    case kiapi::workspace::DK_SYMBOL_LIBRARY: return MDW_DOCUMENT_KIND::SYMBOL_LIBRARY;
    case kiapi::workspace::DK_FOOTPRINT:      return MDW_DOCUMENT_KIND::FOOTPRINT;
    case kiapi::workspace::DK_OTHER:          return MDW_DOCUMENT_KIND::OTHER;
    case kiapi::workspace::DK_UNKNOWN:
    default:                                  return MDW_DOCUMENT_KIND::UNKNOWN;
    }
}


inline MDW_REVISION_KIND MdwFromProto( kiapi::workspace::RevisionKind aKind )
{
    switch( aKind )
    {
    case kiapi::workspace::RK_CHECKIN:          return MDW_REVISION_KIND::CHECKIN;
    case kiapi::workspace::RK_RELEASE:          return MDW_REVISION_KIND::RELEASE;
    case kiapi::workspace::RK_BASELINE:         return MDW_REVISION_KIND::BASELINE;
    case kiapi::workspace::RK_WORKING:          return MDW_REVISION_KIND::WORKING;
    case kiapi::workspace::RK_RELEASE_PENDING:  return MDW_REVISION_KIND::RELEASE_PENDING;
    case kiapi::workspace::RK_RELEASE_REJECTED: return MDW_REVISION_KIND::RELEASE_REJECTED;
    case kiapi::workspace::RK_UNKNOWN:
    default:                                    return MDW_REVISION_KIND::UNKNOWN;
    }
}


inline MDW_PENDING_STATUS MdwFromProto( kiapi::workspace::PendingChangeStatus aStatus )
{
    switch( aStatus )
    {
    case kiapi::workspace::PCS_PENDING:   return MDW_PENDING_STATUS::PENDING;
    case kiapi::workspace::PCS_APPLIED:   return MDW_PENDING_STATUS::APPLIED;
    case kiapi::workspace::PCS_REJECTED:  return MDW_PENDING_STATUS::REJECTED;
    case kiapi::workspace::PCS_WITHDRAWN: return MDW_PENDING_STATUS::WITHDRAWN;
    case kiapi::workspace::PCS_UNKNOWN:
    default:                              return MDW_PENDING_STATUS::UNKNOWN;
    }
}


inline MDW_CHANGE_TYPE MdwFromProto( kiapi::mdw::ChangeType aChange )
{
    switch( aChange )
    {
    case kiapi::mdw::CT_ADDED:    return MDW_CHANGE_TYPE::ADDED;
    case kiapi::mdw::CT_REMOVED:  return MDW_CHANGE_TYPE::REMOVED;
    case kiapi::mdw::CT_MODIFIED: return MDW_CHANGE_TYPE::MODIFIED;
    case kiapi::mdw::CT_UNKNOWN:
    default:                      return MDW_CHANGE_TYPE::UNKNOWN;
    }
}


inline MDW_EVENT_TYPE MdwFromProto( kiapi::workspace::EventType aType )
{
    switch( aType )
    {
    case kiapi::workspace::ET_WORKSPACE_BOUND:            return MDW_EVENT_TYPE::WORKSPACE_BOUND;
    case kiapi::workspace::ET_WORKSPACE_UNBOUND:          return MDW_EVENT_TYPE::WORKSPACE_UNBOUND;
    case kiapi::workspace::ET_WORKSPACE_RECONNECTED:      return MDW_EVENT_TYPE::WORKSPACE_RECONNECTED;
    case kiapi::workspace::ET_ITEM_LOCKED:                return MDW_EVENT_TYPE::ITEM_LOCKED;
    case kiapi::workspace::ET_ITEM_UNLOCKED:              return MDW_EVENT_TYPE::ITEM_UNLOCKED;
    case kiapi::workspace::ET_REVISION_CHECKED_IN:        return MDW_EVENT_TYPE::REVISION_CHECKED_IN;
    case kiapi::workspace::ET_REVISION_CHECKED_OUT:       return MDW_EVENT_TYPE::REVISION_CHECKED_OUT;
    case kiapi::workspace::ET_LIFECYCLE_CATEGORY_CHANGED: return MDW_EVENT_TYPE::LIFECYCLE_CATEGORY_CHANGED;
    case kiapi::workspace::ET_BACKEND_EVENT:              return MDW_EVENT_TYPE::BACKEND_EVENT;
    case kiapi::workspace::ET_REVISION_RELEASED:          return MDW_EVENT_TYPE::REVISION_RELEASED;
    case kiapi::workspace::ET_PENDING_RESOLVED:           return MDW_EVENT_TYPE::PENDING_RESOLVED;
    case kiapi::workspace::ET_CAPABILITIES_CHANGED:       return MDW_EVENT_TYPE::CAPABILITIES_CHANGED;
    case kiapi::workspace::ET_LIBRARY_CHANGED:            return MDW_EVENT_TYPE::LIBRARY_CHANGED;
    case kiapi::workspace::ET_PROGRESS:                   return MDW_EVENT_TYPE::PROGRESS;
    case kiapi::workspace::ET_LOCKS_CHANGED:              return MDW_EVENT_TYPE::LOCKS_CHANGED;
    case kiapi::workspace::ET_UNKNOWN:
    default:                                              return MDW_EVENT_TYPE::UNKNOWN;
    }
}


inline MDW_MEMBER_KIND MdwFromProto( kiapi::workspace::MemberKind aKind )
{
    switch( aKind )
    {
    case kiapi::workspace::MK_SYMBOL:    return MDW_MEMBER_KIND::SYMBOL;
    case kiapi::workspace::MK_FOOTPRINT: return MDW_MEMBER_KIND::FOOTPRINT;
    case kiapi::workspace::MK_UNKNOWN:
    default:                             return MDW_MEMBER_KIND::UNKNOWN;
    }
}


inline MDW_LIBRARY_TYPE MdwFromProto( kiapi::mdw::SharedLibType aType )
{
    switch( aType )
    {
    case kiapi::mdw::SLT_SYMBOL:    return MDW_LIBRARY_TYPE::SYMBOL;
    case kiapi::mdw::SLT_FOOTPRINT: return MDW_LIBRARY_TYPE::FOOTPRINT;
    case kiapi::mdw::SLT_MODEL_3D:  return MDW_LIBRARY_TYPE::MODEL_3D;
    case kiapi::mdw::SLT_UNKNOWN:
    default:                        return MDW_LIBRARY_TYPE::UNKNOWN;
    }
}


/// A severity this build does not know blocks, since only an exception may release such a part
inline MDW_SEVERITY MdwFromProto( kiapi::mdw::Severity aSeverity )
{
    switch( aSeverity )
    {
    case kiapi::mdw::SEV_NONE:    return MDW_SEVERITY::NONE;
    case kiapi::mdw::SEV_WARNING: return MDW_SEVERITY::WARNING;
    case kiapi::mdw::SEV_ERROR:
    case kiapi::mdw::SEV_UNKNOWN:
    default:                      return MDW_SEVERITY::BLOCKING;
    }
}


/// An unset or unknown hold reads as LOCKED, the one hold no release step clears
inline MDW_BASELINE_HOLD MdwFromProto( kiapi::mdw::BaselineHold aHold )
{
    switch( aHold )
    {
    case kiapi::mdw::HOLD_UNCOMMITTED: return MDW_BASELINE_HOLD::UNCOMMITTED;
    case kiapi::mdw::HOLD_DETACHED:    return MDW_BASELINE_HOLD::DETACHED;
    case kiapi::mdw::HOLD_NO_REMOTE:   return MDW_BASELINE_HOLD::NO_REMOTE;
    case kiapi::mdw::HOLD_NONE:        return MDW_BASELINE_HOLD::NONE;
    case kiapi::mdw::HOLD_LOCKED:
    case kiapi::mdw::HOLD_UNKNOWN:
    default:                           return MDW_BASELINE_HOLD::LOCKED;
    }
}


inline void MdwSanitize( kiapi::workspace::CustomState& aState )
{
    aState.set_category( MdwToProto( MdwFromProto( aState.category() ) ) );
}


/// A state from a newer protocol offers the action and lets its call decide
inline void MdwSanitize( kiapi::workspace::Permission& aPermission )
{
    if( !kiapi::workspace::PermissionState_IsValid( aPermission.state() ) )
        aPermission.set_state( kiapi::workspace::PS_UNKNOWN );
}


inline void MdwSanitize( kiapi::workspace::LifecycleTransition& aTransition )
{
    aTransition.set_target_category( MdwToProto( MdwFromProto( aTransition.target_category() ) ) );
    MdwSanitize( *aTransition.mutable_permission() );
}


inline std::string MdwToUtf8( const wxString& aValue )
{
    return aValue.utf8_string();
}


inline wxString MdwFromUtf8( const std::string& aValue )
{
    return wxString::FromUTF8( aValue );
}


/// Zero is the protocol's absent timestamp. wxDateTime carries its own invalid
/// state and has no epoch-zero value the two could be confused for.
inline wxDateTime MdwFromUnixTime( std::int64_t aSeconds )
{
    return aSeconds != 0 ? wxDateTime( static_cast<time_t>( aSeconds ) ) : wxDateTime();
}


inline std::int64_t MdwToUnixTime( const wxDateTime& aTime )
{
    return aTime.IsValid() ? static_cast<std::int64_t>( aTime.GetTicks() ) : 0;
}


inline MDW_VERSION MdwToStruct( const kiapi::workspace::Version& aProto )
{
    MDW_VERSION version;
    version.major = aProto.major();
    version.minor = aProto.minor();
    version.patch = aProto.patch();
    version.full = MdwFromUtf8( aProto.full() );

    return version;
}


inline MDW_BACKEND_INFO MdwToStruct( const kiapi::workspace::BackendInfo& aProto )
{
    MDW_BACKEND_INFO info;
    info.backend_id = MdwFromUtf8( aProto.backend_id() );
    info.display_name = MdwFromUtf8( aProto.display_name() );
    info.description = MdwFromUtf8( aProto.description() );
    info.vendor = MdwFromUtf8( aProto.vendor() );
    info.version = MdwToStruct( aProto.version() );
    info.checkin_model = MdwFromProto( aProto.checkin_model() );

    return info;
}


inline MDW_CAPABILITIES MdwToStruct( const kiapi::workspace::Capabilities& aProto )
{
    MDW_CAPABILITIES caps;

    for( int group : aProto.groups() )
    {
        MDW_CAPABILITY_GROUP known = MdwFromProto( static_cast<kiapi::workspace::CapabilityGroup>( group ) );

        if( known != MDW_CAPABILITY_GROUP::UNKNOWN )
            caps.groups.insert( known );
    }

    for( kiapi::workspace::CustomState state : aProto.custom_states() )
    {
        MdwSanitize( state );
        caps.custom_states.push_back( std::move( state ) );
    }

    caps.adopts_whole_files = aProto.adopts_whole_files();
    return caps;
}


inline void MdwToProto( kiapi::workspace::SearchFilter* aOut, const MDW_SEARCH_FILTER& aFilter )
{
    aOut->set_field_id( MdwToUtf8( aFilter.field_id ) );
    aOut->set_value( MdwToUtf8( aFilter.value ) );
}


inline MDW_LIFECYCLE_STATE MdwToStruct( const kiapi::workspace::LifecycleState& aProto )
{
    MDW_LIFECYCLE_STATE state;
    state.category = MdwFromProto( aProto.category() );
    state.state_id = MdwFromUtf8( aProto.state_id() );
    state.not_for_new_designs = aProto.not_for_new_designs();

    return state;
}


inline void MdwToProto( kiapi::workspace::LifecycleState* aOut, const MDW_LIFECYCLE_STATE& aState )
{
    aOut->set_category( MdwToProto( aState.category ) );
    aOut->set_state_id( MdwToUtf8( aState.state_id ) );
    aOut->set_not_for_new_designs( aState.not_for_new_designs );
}


inline MDW_PENDING_CHANGE MdwToStruct( const kiapi::workspace::PendingChange& aProto )
{
    MDW_PENDING_CHANGE pending;
    pending.change_id = MdwFromUtf8( aProto.change_id() );
    pending.eco_id = MdwFromUtf8( aProto.eco_id() );
    pending.display_id = MdwFromUtf8( aProto.display_id() );
    pending.reason = MdwFromUtf8( aProto.reason() );
    pending.url = MdwFromUtf8( aProto.url() );

    return pending;
}


inline MDW_LOCK_INFO MdwToStruct( const kiapi::workspace::LockInfo& aProto )
{
    MDW_LOCK_INFO info;
    info.is_locked = aProto.is_locked();
    info.owner_id = MdwFromUtf8( aProto.owner_id() );
    info.owner_display_name = MdwFromUtf8( aProto.owner_display_name() );
    info.lock_timestamp = MdwFromUnixTime( aProto.lock_timestamp_unix() );
    info.locked_by_me = aProto.locked_by_me();
    info.advisory = aProto.advisory();

    return info;
}


inline void MdwToProto( kiapi::workspace::LockInfo* aOut, const MDW_LOCK_INFO& aLock )
{
    aOut->set_is_locked( aLock.is_locked );
    aOut->set_owner_id( MdwToUtf8( aLock.owner_id ) );
    aOut->set_owner_display_name( MdwToUtf8( aLock.owner_display_name ) );
    aOut->set_lock_timestamp_unix( MdwToUnixTime( aLock.lock_timestamp ) );
    aOut->set_locked_by_me( aLock.locked_by_me );
    aOut->set_advisory( aLock.advisory );
}


inline MDW_ITEM_INFO MdwToStruct( const kiapi::workspace::ItemInfo& aProto )
{
    MDW_ITEM_INFO info;
    info.item_id = MdwFromUtf8( aProto.item_id() );
    info.display_name = MdwFromUtf8( aProto.display_name() );
    info.path_hint = MdwFromUtf8( aProto.path_hint() );
    info.relative_path = MdwFromUtf8( aProto.relative_path() );
    info.folder_id = MdwFromUtf8( aProto.folder_id() );
    info.is_folder = aProto.is_folder();
    info.kind = MdwFromProto( aProto.kind() );
    info.modified = MdwFromUnixTime( aProto.modified_unix() );
    info.revision_id = MdwFromUtf8( aProto.revision_id() );
    info.revision_label = MdwFromUtf8( aProto.revision_label() );
    info.revision_kind = MdwFromProto( aProto.revision_kind() );
    info.state = MdwToStruct( aProto.state() );
    info.lock = MdwToStruct( aProto.lock() );

    return info;
}


inline MDW_FOLDER MdwToStruct( const kiapi::workspace::Folder& aProto )
{
    MDW_FOLDER folder;
    folder.folder_id = MdwFromUtf8( aProto.folder_id() );
    folder.display_name = MdwFromUtf8( aProto.display_name() );
    folder.parent_folder_id = MdwFromUtf8( aProto.parent_folder_id() );
    folder.has_children = aProto.has_children();

    return folder;
}


inline MDW_REVISION MdwToStruct( const kiapi::workspace::Revision& aProto )
{
    MDW_REVISION revision;
    revision.revision_id = MdwFromUtf8( aProto.revision_id() );
    revision.label = MdwFromUtf8( aProto.label() );
    revision.ordinal = aProto.ordinal();
    revision.kind = MdwFromProto( aProto.kind() );
    revision.author = MdwFromUtf8( aProto.author() );
    revision.comment = MdwFromUtf8( aProto.comment() );
    revision.timestamp = MdwFromUnixTime( aProto.timestamp_unix() );
    revision.content_digest = MdwFromUtf8( aProto.content_digest() );
    revision.member_digest = MdwFromUtf8( aProto.member_digest() );
    revision.state = MdwToStruct( aProto.state() );

    for( const std::string& parent : aProto.parent_revision_ids() )
        revision.parent_revision_ids.push_back( MdwFromUtf8( parent ) );

    return revision;
}


inline MDW_ITEM_REF MdwToStruct( const kiapi::workspace::ItemRef& aProto )
{
    MDW_ITEM_REF ref;
    ref.item_id = MdwFromUtf8( aProto.item_id() );
    ref.path_hint = MdwFromUtf8( aProto.path_hint() );
    ref.source = MdwFromUtf8( aProto.source() );

    return ref;
}


/// A reference we keep always names its source, aLocator when the reply left it empty
inline MDW_ITEM_REF MdwToStruct( const kiapi::workspace::ItemRef& aProto, const wxString& aLocator )
{
    MDW_ITEM_REF ref = MdwToStruct( aProto );

    if( ref.source.IsEmpty() )
        ref.source = aLocator;

    return ref;
}


inline void MdwToProto( kiapi::workspace::ItemRef* aOut, const MDW_ITEM_REF& aRef )
{
    aOut->set_item_id( MdwToUtf8( aRef.item_id ) );
    aOut->set_path_hint( MdwToUtf8( aRef.path_hint ) );
    aOut->set_source( MdwToUtf8( aRef.source ) );
}


inline MDW_HOST_KEY MdwToStruct( const kiapi::workspace::HostKey& aProto )
{
    MDW_HOST_KEY key;
    key.host = MdwFromUtf8( aProto.host() );
    key.port = aProto.port();
    key.key_type = MdwFromUtf8( aProto.key_type() );
    key.sha256_fingerprint = MdwFromUtf8( aProto.sha256_fingerprint() );

    return key;
}


inline void MdwToProto( kiapi::workspace::HostKey* aOut, const MDW_HOST_KEY& aKey )
{
    aOut->set_host( MdwToUtf8( aKey.host ) );
    aOut->set_port( aKey.port );
    aOut->set_key_type( MdwToUtf8( aKey.key_type ) );
    aOut->set_sha256_fingerprint( MdwToUtf8( aKey.sha256_fingerprint ) );
}


inline MDW_AUTH_CHALLENGE MdwToStruct( const kiapi::workspace::AuthChallenge& aProto )
{
    MDW_AUTH_CHALLENGE challenge;
    challenge.realm = MdwFromUtf8( aProto.realm() );
    challenge.username_hint = MdwFromUtf8( aProto.username_hint() );

    for( const kiapi::workspace::AuthField& field : aProto.fields() )
    {
        challenge.fields.push_back( { MdwFromUtf8( field.field_id() ), MdwFromUtf8( field.display_name() ),
                                      field.secret(), field.required() } );
    }

    return challenge;
}


inline MDW_ACCOUNT MdwToStruct( const kiapi::workspace::Account& aProto )
{
    return { MdwFromUtf8( aProto.user_id() ), MdwFromUtf8( aProto.display_name() ), MdwFromUtf8( aProto.email() ) };
}


/// The whole envelope error, so a refusal keeps the backend's reason and any retry or auth detail
inline MDW_RESULT MdwToResult( const kiapi::workspace::WorkspaceError& aError )
{
    if( aError.code() == kiapi::workspace::WE_OK )
        return MDW_RESULT::Ok();

    MDW_RESULT result = MDW_RESULT::Fail( MdwFromProto( aError.code() ), MdwFromUtf8( aError.message() ) );
    result.detail = MdwFromUtf8( aError.detail() );
    result.retry_after_ms = aError.retry_after_ms();

    if( aError.has_auth_challenge() )
        result.auth_challenge = MdwToStruct( aError.auth_challenge() );

    if( aError.has_host_key() )
        result.host_key = MdwToStruct( aError.host_key() );

    return result;
}

#endif // MDW_PROTO_CONVERT_H
