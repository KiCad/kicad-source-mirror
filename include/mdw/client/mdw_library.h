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
 * @file mdw_library.h
 * Shared component libraries (CG_SHARED_LIBRARIES) written into a registered local copy.
 */

#ifndef MDW_CLIENT_LIBRARY_H
#define MDW_CLIENT_LIBRARY_H

#include <kicommon.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_part_catalog.h>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>


struct MDW_LOCAL_CHANGE;
class MDW_SOURCE;

enum class MDW_LIBRARY_TYPE
{
    UNKNOWN   = 0,
    SYMBOL    = 1,
    FOOTPRINT = 2,
    MODEL_3D  = 3
};

struct MDW_LIBRARY_INFO
{
    wxString         library_id;
    wxString         display_name;          ///< The registered nickname
    MDW_LIBRARY_TYPE type = MDW_LIBRARY_TYPE::UNKNOWN;
    wxString         relative_path;         ///< Under the local root
};

/// One release of a library member
struct MDW_MEMBER_RELEASE
{
    wxString            revision_id;
    wxString            label;
    wxString            content_digest;     ///< Empty when no placed part may match it
    MDW_LIFECYCLE_STATE state;
    bool                published = true;
};

/// A library member is an item; member_id addresses it in every per-item operation
struct MDW_LIBRARY_MEMBER
{
    wxString            member_id;
    wxString            library_id;
    wxString            name;               ///< The item name inside the library, as in a LIB_ID
    MDW_MEMBER_KIND     kind = MDW_MEMBER_KIND::UNKNOWN;
    wxString            path_hint;
    wxString            revision_id;        ///< Latest release at the snapshot; empty when never released
    wxString            revision_label;
    MDW_LIFECYCLE_STATE state;
    wxString            version;            ///< The content at the snapshot, compared only and never shown
    wxString            release_digest;     ///< The content_digest revision_id was released with

    /// Every release at the snapshot, oldest first
    std::vector<MDW_MEMBER_RELEASE> releases;
};

struct MDW_LIBRARY_LIST
{
    std::vector<MDW_LIBRARY_INFO>   libraries;
    MDW_SNAPSHOT                    snapshot;
    std::vector<MDW_LIBRARY_MEMBER> members; ///< Every member at snapshot; only Materialize lists them
};

struct MDW_MEMBER_CHANGE
{
    MDW_ITEM_REF    member;
    MDW_CHANGE_TYPE change = MDW_CHANGE_TYPE::UNKNOWN;
    wxString        old_revision_label;
    wxString        new_revision_label;
};

/**
 * A member changed locally and upstream that the backend could not merge. Ours is the member called
 * name inside relative_path, absent when either is missing. Download base and theirs as
 * single-member files, merge them with an extract of ours, and update again with an
 * MDW_CONFLICT_RESOLUTION.
 */
struct MDW_UPDATE_CONFLICT
{
    MDW_ITEM_REF      item;
    MDW_DOCUMENT_KIND kind = MDW_DOCUMENT_KIND::UNKNOWN;
    wxString          base_revision_id;   ///< Empty when both sides added the member
    wxString          theirs_revision_id; ///< Empty when upstream removed the member
    wxString          relative_path;      ///< The member's file under the local root
    wxString          name;               ///< The member's name inside that file
    wxString          ours_token;         ///< Opaque; echoed in the resolution
};

struct MDW_CONFLICT_RESOLUTION
{
    wxString item_id;
    wxString theirs_revision_id;

    /// A file holding only the merged member, empty to remove it; unset keeps the member as the local file has it
    std::optional<std::string> merged;

    wxString ours_token; ///< The conflict's, so a member saved since is not overwritten

    /// Set when merged is the member's whole library file, upgraded to hold it. It replaces that file only while
    /// the file still has this git blob id, the one it was built from
    wxString whole_file_of;
};

/// Any conflict means nothing changed locally
struct MDW_LIBRARY_UPDATE
{
    MDW_SNAPSHOT                     snapshot;
    std::vector<MDW_MEMBER_CHANGE>   changes;
    std::vector<MDW_UPDATE_CONFLICT> conflicts;
    std::vector<MDW_LIBRARY_INFO>    libraries; ///< What the copy holds at snapshot, never the latest
    std::vector<MDW_LIBRARY_MEMBER>  members;   ///< Every member at snapshot
};

/// One member's check-in. library_id and name identify it even when no item could be created.
struct MDW_MEMBER_RESULT
{
    wxString   library_id;
    wxString   name;
    wxString   item_id;
    MDW_CHANGE change;
    MDW_RESULT status;
};

/// A format-only file the backend left out of a check-in, with why
struct MDW_SKIPPED_FILE
{
    wxString path;
    wxString reason;
};

struct MDW_LIBRARY_CHECKIN
{
    std::vector<MDW_MEMBER_RESULT> results;
    std::vector<MDW_SKIPPED_FILE>  skipped_files;
    MDW_SNAPSHOT                   snapshot; ///< Empty token when nothing was committed or the backend mints none
    MDW_SNAPSHOT                   copy_snapshot; ///< local_root as the check-in left it, empty unless named exactly
};

class KICOMMON_API MDW_LIBRARY
{
public:
    explicit MDW_LIBRARY( std::shared_ptr<MDW_SOURCE> aSource );

    MDW_OP List( MDW_DONE<MDW_LIBRARY_LIST> aDone );

    /// aPin is a snapshot token; empty means the latest
    MDW_OP Materialize( const wxString& aLocalRoot, const wxString& aPin, MDW_DONE<MDW_LIBRARY_LIST> aDone );
    MDW_OP Update( const wxString& aLocalRoot, const std::vector<MDW_CONFLICT_RESOLUTION>& aResolutions,
                   MDW_DONE<MDW_LIBRARY_UPDATE> aDone );

    /// aSnapshot is a snapshot token; empty means the latest
    MDW_OP Members( const wxString& aLibraryId, const wxString& aSnapshot, const MDW_PAGE& aPage,
                    MDW_DONE<MDW_PAGED<MDW_LIBRARY_MEMBER>> aDone );

    MDW_OP LocalChanges( const wxString& aLocalRoot, const MDW_PAGE& aPage,
                         MDW_DONE<MDW_PAGED<MDW_LOCAL_CHANGE>> aDone );

    /**
     * The result succeeds when the batch was processed; each member carries its own status. aFormatUpgrade says
     * aWholeFiles only reformat their members, which then keep their revisions, and requires aItemIds to be empty.
     */
    MDW_OP CheckIn( const wxString& aLocalRoot, const std::vector<wxString>& aItemIds, const wxString& aComment,
                    const std::map<wxString, wxString>& aWholeFiles, bool aFormatUpgrade,
                    MDW_DONE<MDW_LIBRARY_CHECKIN> aDone );

private:
    std::shared_ptr<MDW_SOURCE> m_source;
};

#endif // MDW_CLIENT_LIBRARY_H
