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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "libgit_backend.h"
#include "kicad_git_memory.h"

#include <git2.h>
#include <git2/sys/repository.h>

#include <algorithm>
#include <queue>
#include <set>


namespace
{

bool cancelled( const KIGIT_CANCEL& aCancelled )
{
    return aCancelled && aCancelled();
}


// A hex id, whole or abbreviated, so revision syntax cannot reach a ref
KIGIT::GitCommitPtr lookupCommit( git_repository* aRepo, const std::string& aOid )
{
    git_oid     oid;
    git_object* object = nullptr;
    git_object* peeled = nullptr;

    if( !aRepo || aOid.size() < GIT_OID_MINPREFIXLEN || aOid.size() > GIT_OID_HEXSZ
        || aOid.find_first_not_of( "0123456789abcdef" ) != std::string::npos
        || git_oid_fromstrn( &oid, aOid.c_str(), aOid.size() ) != GIT_OK
        || git_object_lookup_prefix( &object, aRepo, &oid, aOid.size(), GIT_OBJECT_ANY ) != GIT_OK )
    {
        return KIGIT::GitCommitPtr( nullptr );
    }

    KIGIT::GitObjectPtr objectPtr( object );

    if( git_object_peel( &peeled, object, GIT_OBJECT_COMMIT ) != GIT_OK )
        return KIGIT::GitCommitPtr( nullptr );

    return KIGIT::GitCommitPtr( reinterpret_cast<git_commit*>( peeled ) );
}


KIGIT_COMMIT_RECORD recordOf( git_commit* aCommit )
{
    KIGIT_COMMIT_RECORD record;
    record.oid = git_oid_tostr_s( git_commit_id( aCommit ) );
    record.time = static_cast<std::int64_t>( git_commit_time( aCommit ) );

    for( unsigned int i = 0; i < git_commit_parentcount( aCommit ); ++i )
        record.parents.push_back( git_oid_tostr_s( git_commit_parent_id( aCommit, i ) ) );

    if( const git_signature* author = git_commit_author( aCommit ) )
    {
        record.authorName = author->name ? author->name : "";
        record.authorEmail = author->email ? author->email : "";
    }

    if( const char* message = git_commit_message( aCommit ) )
        record.message = message;

    return record;
}


KIGIT_PATH_COMMIT pathCommitOf( git_commit* aCommit )
{
    KIGIT_PATH_COMMIT version;
    version.oid = git_oid_tostr_s( git_commit_id( aCommit ) );
    version.time = static_cast<std::int64_t>( git_commit_time( aCommit ) );

    for( unsigned int i = 0; i < git_commit_parentcount( aCommit ); ++i )
        version.parents.push_back( git_oid_tostr_s( git_commit_parent_id( aCommit, i ) ) );

    if( const git_signature* author = git_commit_author( aCommit ) )
        version.author = author->name ? author->name : "";

    if( const char* summary = git_commit_summary( aCommit ) )
        version.summary = summary;

    return version;
}


/// One path in one commit, file or folder, both fields zero when the commit lacks it
struct PATH_BLOB
{
    git_oid       oid = {};
    std::uint32_t mode = 0;

    bool operator==( const PATH_BLOB& aOther ) const
    {
        return mode == aOther.mode && git_oid_equal( &oid, &aOther.oid );
    }
};


PATH_BLOB blobIn( git_commit* aCommit, const std::string& aPath )
{
    PATH_BLOB blob;
    git_tree* tree = nullptr;

    if( git_commit_tree( &tree, aCommit ) != GIT_OK )
        return blob;

    KIGIT::GitTreePtr treePtr( tree );
    git_tree_entry*   entry = nullptr;

    if( aPath.empty() )
    {
        git_oid_cpy( &blob.oid, git_tree_id( tree ) );
        blob.mode = GIT_FILEMODE_TREE;
        return blob;
    }

    if( git_tree_entry_bypath( &entry, tree, aPath.c_str() ) != GIT_OK )
        return blob;

    KIGIT::GitTreeEntryPtr entryPtr( entry );

    git_oid_cpy( &blob.oid, git_tree_entry_id( entry ) );
    blob.mode = git_tree_entry_filemode( entry );
    return blob;
}


const git_tree_entry* entryOf( const git_tree* aTree, const std::string& aName, git_object_t aType )
{
    const git_tree_entry* entry = aTree ? git_tree_entry_byname( aTree, aName.c_str() ) : nullptr;

    return entry && ( aType == GIT_OBJECT_ANY || git_tree_entry_type( entry ) == aType ) ? entry : nullptr;
}


// Mark the paths whose entry and mode match in aOurs and aTheirs, the trees at path component aDepth
// Only subtrees whose ids differ are opened, so most commits cost a few lookups
void markSame( git_repository* aRepo, const git_tree* aOurs, const git_tree* aTheirs,
               const std::vector<std::vector<std::string>>& aParts, std::vector<size_t>& aIndices, size_t aDepth,
               std::vector<char>& aSame )
{
    if( ( !aOurs && !aTheirs )
        || ( aOurs && aTheirs && git_oid_equal( git_tree_id( aOurs ), git_tree_id( aTheirs ) ) ) )
    {
        for( size_t index : aIndices )
            aSame[index] = true;

        return;
    }

    // A path naming this whole tree changed with it
    auto whole = std::partition( aIndices.begin(), aIndices.end(),
                                 [&]( size_t aIndex )
                                 {
                                     return aParts[aIndex].size() > aDepth;
                                 } );

    for( auto it = whole; it != aIndices.end(); ++it )
        aSame[*it] = false;

    aIndices.erase( whole, aIndices.end() );

    std::sort( aIndices.begin(), aIndices.end(),
               [&]( size_t aLeft, size_t aRight )
               {
                   return aParts[aLeft][aDepth] < aParts[aRight][aDepth];
               } );

    for( size_t first = 0; first < aIndices.size(); )
    {
        const std::string& name = aParts[aIndices[first]][aDepth];
        size_t             last = first + 1;

        while( last < aIndices.size() && aParts[aIndices[last]][aDepth] == name )
            ++last;

        std::vector<size_t> deeper;

        for( size_t i = first; i < last; ++i )
        {
            size_t index = aIndices[i];

            if( aParts[index].size() > aDepth + 1 )
            {
                deeper.push_back( index );
                continue;
            }

            const git_tree_entry* ours = entryOf( aOurs, name, GIT_OBJECT_ANY );
            const git_tree_entry* theirs = entryOf( aTheirs, name, GIT_OBJECT_ANY );

            aSame[index] = ( !ours && !theirs )
                           || ( ours && theirs && git_tree_entry_filemode( ours ) == git_tree_entry_filemode( theirs )
                                && git_oid_equal( git_tree_entry_id( ours ), git_tree_entry_id( theirs ) ) );
        }

        if( !deeper.empty() )
        {
            const git_tree_entry* ours = entryOf( aOurs, name, GIT_OBJECT_TREE );
            const git_tree_entry* theirs = entryOf( aTheirs, name, GIT_OBJECT_TREE );

            if( ours && theirs && git_oid_equal( git_tree_entry_id( ours ), git_tree_entry_id( theirs ) ) )
            {
                for( size_t index : deeper )
                    aSame[index] = true;
            }
            else
            {
                git_tree* oursTree = nullptr;
                git_tree* theirsTree = nullptr;

                // An unreadable subtree holds nothing, as git_tree_entry_bypath fails in blobIn
                if( ours )
                    git_tree_lookup( &oursTree, aRepo, git_tree_entry_id( ours ) );

                if( theirs )
                    git_tree_lookup( &theirsTree, aRepo, git_tree_entry_id( theirs ) );

                KIGIT::GitTreePtr oursPtr( oursTree );
                KIGIT::GitTreePtr theirsPtr( theirsTree );

                markSame( aRepo, oursTree, theirsTree, aParts, deeper, aDepth + 1, aSame );
            }
        }

        first = last;
    }
}

} // namespace


std::optional<std::vector<std::string>> LIBGIT_BACKEND::IndexedPaths( git_repository* aRepo )
{
    git_index* index = nullptr;

    if( !aRepo || git_repository_index( &index, aRepo ) != GIT_OK )
        return std::nullopt;

    KIGIT::GitIndexPtr indexPtr( index );

    if( git_index_read( index, 0 ) != GIT_OK )
        return std::nullopt;

    std::vector<std::string> files;
    size_t                   count = git_index_entrycount( index );

    for( size_t i = 0; i < count; ++i )
    {
        const git_index_entry* entry = git_index_get_byindex( index, i );

        if( entry && GIT_INDEX_ENTRY_STAGE( entry ) == 0 )
            files.emplace_back( entry->path );
    }

    return files;
}


std::vector<KIGIT_PATH_PLACE> LIBGIT_BACKEND::LocatePaths( git_repository*                 aRepo,
                                                           const std::vector<std::string>& aPaths )
{
    std::vector<KIGIT_PATH_PLACE> places( aPaths.size() );
    git_object*                   head = nullptr;
    git_index*                    index = nullptr;

    if( !aRepo )
        return places;

    if( git_revparse_single( &head, aRepo, "HEAD^{tree}" ) != GIT_OK )
        head = nullptr;

    if( git_repository_index( &index, aRepo ) != GIT_OK )
        index = nullptr;

    KIGIT::GitObjectPtr headPtr( head );
    KIGIT::GitIndexPtr  indexPtr( index );
    const bool          indexRead = index && git_index_read( index, 0 ) == GIT_OK;

    for( size_t ii = 0; ii < aPaths.size(); ++ii )
    {
        const char*     path = aPaths[ii].c_str();
        git_tree_entry* entry = nullptr;

        if( head && git_tree_entry_bypath( &entry, reinterpret_cast<git_tree*>( head ), path ) == GIT_OK )
        {
            KIGIT::GitTreeEntryPtr entryPtr( entry );
            places[ii].inHead = git_tree_entry_type( entry ) == GIT_OBJECT_BLOB;
        }

        places[ii].inIndex = indexRead && git_index_get_bypath( index, path, 0 );
    }

    return places;
}


std::map<std::string, KIGIT_PATH_COMMIT> LIBGIT_BACKEND::LastChanges( git_repository*                 aRepo,
                                                                      const std::vector<std::string>& aPaths,
                                                                      const KIGIT_CANCEL&             aCancelled )
{
    std::map<std::string, KIGIT_PATH_COMMIT> result;
    git_oid                                  head;

    if( !aRepo || git_reference_name_to_id( &head, aRepo, "HEAD" ) != GIT_OK )
        return result;

    std::vector<std::string>              wanted;
    std::vector<std::vector<std::string>> parts;
    std::set<std::string>                 unique;

    for( const std::string& path : aPaths )
    {
        if( !unique.insert( path ).second )
            continue;

        wanted.push_back( path );
        std::vector<std::string>& split = parts.emplace_back();

        // The empty path is the whole tree, which has no components
        for( size_t start = 0, slash = 0; !path.empty() && slash != std::string::npos; start = slash + 1 )
        {
            slash = path.find( '/', start );
            split.push_back( path.substr( start, slash == std::string::npos ? slash : slash - start ) );
        }
    }

    // One walk carries every path; each follows its own unchanged parent exactly as PathHistory does alone
    struct NODE
    {
        std::set<size_t> waiting;
        std::set<size_t> done;
    };

    struct OID_LESS
    {
        bool operator()( const git_oid& aLhs, const git_oid& aRhs ) const { return git_oid_cmp( &aLhs, &aRhs ) < 0; }
    };

    // Newest first, and by id among equal times so the walk does not depend on the order commits were found
    auto older = []( git_commit* aLhs, git_commit* aRhs )
    {
        git_time_t lhs = git_commit_time( aLhs );
        git_time_t rhs = git_commit_time( aRhs );

        return lhs < rhs || ( lhs == rhs && git_oid_cmp( git_commit_id( aLhs ), git_commit_id( aRhs ) ) < 0 );
    };

    std::map<git_oid, NODE, OID_LESS>                                             nodes;
    std::priority_queue<git_commit*, std::vector<git_commit*>, decltype( older )> queue( older );
    std::vector<KIGIT::GitCommitPtr>                                              owned;
    std::vector<char>                                                             resolved( wanted.size(), false );
    std::vector<char>                                                             same( wanted.size(), false );
    size_t                                                                        open = wanted.size();

    auto send = [&]( const git_oid* aOid, const std::vector<size_t>& aIndices )
    {
        NODE& node = nodes[*aOid];
        bool  idle = node.waiting.empty();

        for( size_t index : aIndices )
        {
            if( !node.done.count( index ) )
                node.waiting.insert( index );
        }

        git_commit* commit = nullptr;

        if( !idle || node.waiting.empty() || git_commit_lookup( &commit, aRepo, aOid ) != GIT_OK )
            return;

        owned.emplace_back( commit );
        queue.push( commit );
    };

    std::vector<size_t> all( wanted.size() );

    for( size_t i = 0; i < all.size(); ++i )
        all[i] = i;

    send( &head, all );

    while( !queue.empty() && open > 0 && !cancelled( aCancelled ) )
    {
        git_commit* commit = queue.top();
        queue.pop();

        NODE&               node = nodes[*git_commit_id( commit )];
        std::vector<size_t> unassigned;

        for( size_t index : node.waiting )
        {
            node.done.insert( index );

            if( !resolved[index] )
            {
                unassigned.push_back( index );
                same[index] = false;
            }
        }

        node.waiting.clear();

        if( unassigned.empty() )
            continue;

        git_tree* tree = nullptr;

        // An unreadable tree holds nothing, as blobIn reads it, so the answer matches PathHistory's
        git_commit_tree( &tree, commit );
        KIGIT::GitTreePtr treePtr( tree );
        unsigned int      parents = git_commit_parentcount( commit );

        for( unsigned int i = 0; i < parents && !unassigned.empty(); ++i )
        {
            git_commit* parent = nullptr;
            git_tree*   parentTree = nullptr;

            if( git_commit_parent( &parent, commit, i ) != GIT_OK )
                continue;

            KIGIT::GitCommitPtr parentPtr( parent );

            git_commit_tree( &parentTree, parent );
            KIGIT::GitTreePtr   parentTreePtr( parentTree );
            std::vector<size_t> asked = unassigned;

            markSame( aRepo, tree, parentTree, parts, asked, 0, same );

            std::vector<size_t> toParent;
            std::vector<size_t> rest;

            for( size_t index : unassigned )
                ( same[index] ? toParent : rest ).push_back( index );

            if( !toParent.empty() )
                send( git_commit_parent_id( commit, i ), toParent );

            unassigned = std::move( rest );
        }

        if( unassigned.empty() )
            continue;

        // A root commit only counts for the paths it introduced
        if( parents == 0 )
        {
            std::vector<size_t> asked = unassigned;
            markSame( aRepo, tree, nullptr, parts, asked, 0, same );
        }

        for( size_t index : unassigned )
        {
            if( parents == 0 && same[index] )
                continue;

            resolved[index] = true;
            --open;
            result.emplace( wanted[index], pathCommitOf( commit ) );
        }
    }

    return result;
}


std::vector<KIGIT_PATH_COMMIT> LIBGIT_BACKEND::PathHistory( git_repository* aRepo, const std::string& aPath,
                                                            const KIGIT_CANCEL& aCancelled )
{
    std::vector<KIGIT_PATH_COMMIT> versions;
    git_oid                        head;

    if( !aRepo || git_reference_name_to_id( &head, aRepo, "HEAD" ) != GIT_OK )
        return versions;

    auto newestFirst = []( git_commit* aLhs, git_commit* aRhs )
    {
        return git_commit_time( aLhs ) < git_commit_time( aRhs );
    };

    std::vector<KIGIT::GitCommitPtr>                                                    owned;
    std::priority_queue<git_commit*, std::vector<git_commit*>, decltype( newestFirst )> queue( newestFirst );
    std::set<std::string>                                                               seen;

    auto push = [&]( const git_oid* aOid )
    {
        if( !seen.insert( git_oid_tostr_s( aOid ) ).second )
            return;

        git_commit* commit = nullptr;

        if( git_commit_lookup( &commit, aRepo, aOid ) != GIT_OK )
            return;

        owned.emplace_back( commit );
        queue.push( commit );
    };

    push( &head );

    // As `git log -- <path>` walks by default, a commit matching a parent follows only that parent, so a side
    // branch whose change a merge discarded is never reported
    while( !queue.empty() && !cancelled( aCancelled ) )
    {
        git_commit* commit = queue.top();
        queue.pop();

        PATH_BLOB    own = blobIn( commit, aPath );
        unsigned int parents = git_commit_parentcount( commit );
        int          sameAs = -1;

        for( unsigned int i = 0; i < parents && sameAs < 0; ++i )
        {
            git_commit* parent = nullptr;

            if( git_commit_parent( &parent, commit, i ) != GIT_OK )
                continue;

            KIGIT::GitCommitPtr parentPtr( parent );

            // A mode change alone is a change, as git log reports it
            if( blobIn( parent, aPath ) == own )
                sameAs = static_cast<int>( i );
        }

        if( sameAs >= 0 )
        {
            push( git_commit_parent_id( commit, static_cast<unsigned int>( sameAs ) ) );
            continue;
        }

        // A root commit only counts when it introduced the path
        if( parents == 0 && git_oid_is_zero( &own.oid ) )
            continue;

        versions.push_back( pathCommitOf( commit ) );

        for( unsigned int i = 0; i < parents; ++i )
            push( git_commit_parent_id( commit, i ) );
    }

    return versions;
}


std::optional<KIGIT_TREE_ENTRY> LIBGIT_BACKEND::TreeEntryAt( git_repository* aRepo, const std::string& aCommit,
                                                             const std::string& aPath )
{
    KIGIT::GitCommitPtr commit = lookupCommit( aRepo, aCommit );
    git_tree*           tree = nullptr;

    if( !commit || git_commit_tree( &tree, commit.get() ) != GIT_OK )
        return std::nullopt;

    KIGIT::GitTreePtr treePtr( tree );
    git_tree_entry*   entry = nullptr;

    if( aPath.empty() )
        return KIGIT_TREE_ENTRY{ std::string(), git_oid_tostr_s( git_tree_id( tree ) ), GIT_FILEMODE_TREE, true };

    if( git_tree_entry_bypath( &entry, tree, aPath.c_str() ) != GIT_OK )
        return std::nullopt;

    KIGIT::GitTreeEntryPtr entryPtr( entry );
    KIGIT_TREE_ENTRY       result;

    result.path = aPath;
    result.oid = git_oid_tostr_s( git_tree_entry_id( entry ) );
    result.mode = git_tree_entry_filemode( entry );
    result.isTree = git_tree_entry_type( entry ) == GIT_OBJECT_TREE;

    return result;
}


std::optional<std::string> LIBGIT_BACKEND::ReadBlob( git_repository* aRepo, const std::string& aOid )
{
    git_oid   oid;
    git_blob* blob = nullptr;

    if( !aRepo || git_oid_fromstrn( &oid, aOid.c_str(), aOid.size() ) != GIT_OK
        || git_blob_lookup_prefix( &blob, aRepo, &oid, aOid.size() ) != GIT_OK )
    {
        return std::nullopt;
    }

    KIGIT::GitBlobPtr blobPtr( blob );

    return std::string( static_cast<const char*>( git_blob_rawcontent( blob ) ),
                        static_cast<size_t>( git_blob_rawsize( blob ) ) );
}


int LIBGIT_BACKEND::WalkTree( git_repository* aRepo, const std::string& aCommit, const std::string& aFolder,
                              const std::function<KIGIT_WALK( const KIGIT_TREE_ENTRY& )>& aVisit )
{
    KIGIT::GitCommitPtr commit = lookupCommit( aRepo, aCommit );
    git_tree*           tree = nullptr;

    if( !commit )
        return GIT_ENOTFOUND;

    if( int rc = git_commit_tree( &tree, commit.get() ) )
        return rc;

    KIGIT::GitTreePtr treePtr( tree );

    if( !aFolder.empty() )
    {
        git_tree_entry* entry = nullptr;
        git_tree*       folder = nullptr;

        if( int rc = git_tree_entry_bypath( &entry, tree, aFolder.c_str() ) )
            return rc;

        KIGIT::GitTreeEntryPtr entryPtr( entry );

        if( git_tree_entry_type( entry ) != GIT_OBJECT_TREE )
            return GIT_ENOTFOUND;

        if( int rc = git_tree_lookup( &folder, aRepo, git_tree_entry_id( entry ) ) )
            return rc;

        treePtr.reset( folder );
    }

    struct WALK
    {
        std::string                                                 prefix;
        const std::function<KIGIT_WALK( const KIGIT_TREE_ENTRY& )>& visit;
    } walk{ aFolder.empty() ? std::string() : aFolder + "/", aVisit };

    auto visit = []( const char* aRoot, const git_tree_entry* aEntry, void* aPayload ) -> int
    {
        WALK*            state = static_cast<WALK*>( aPayload );
        KIGIT_TREE_ENTRY entry;

        entry.path = state->prefix + aRoot + git_tree_entry_name( aEntry );
        entry.oid = git_oid_tostr_s( git_tree_entry_id( aEntry ) );
        entry.mode = git_tree_entry_filemode( aEntry );
        entry.isTree = git_tree_entry_type( aEntry ) == GIT_OBJECT_TREE;

        switch( state->visit( entry ) )
        {
        case KIGIT_WALK::SKIP: return 1;
        case KIGIT_WALK::STOP: return GIT_EUSER;
        default: return 0;
        }
    };

    return git_tree_walk( treePtr.get(), GIT_TREEWALK_PRE, visit, &walk );
}


std::optional<std::string> LIBGIT_BACKEND::FilterText( git_repository* aRepo, const std::string& aPath,
                                                       const std::string& aText, bool aToRepository )
{
    git_filter_list* filters = nullptr;

    if( !aRepo
        || git_filter_list_load( &filters, aRepo, nullptr, aPath.c_str(),
                                 aToRepository ? GIT_FILTER_TO_ODB : GIT_FILTER_TO_WORKTREE, GIT_FILTER_DEFAULT )
                   < 0 )
    {
        return std::nullopt;
    }

    if( !filters )
        return aText;

    KIGIT::GitFilterListPtr    filtersPtr( filters );
    git_buf                    buf = GIT_BUF_INIT;
    std::optional<std::string> result;

    if( git_filter_list_apply_to_buffer( &buf, filters, aText.data(), aText.size() ) == GIT_OK )
        result.emplace( buf.ptr, buf.size );

    git_buf_dispose( &buf );
    return result;
}


std::string LIBGIT_BACKEND::BlobId( const std::string& aContent )
{
    git_oid oid;

    if( git_odb_hash( &oid, aContent.data(), aContent.size(), GIT_OBJECT_BLOB ) != GIT_OK )
        return std::string();

    return git_oid_tostr_s( &oid );
}


std::optional<bool> LIBGIT_BACKEND::IsDescendant( git_repository* aRepo, const std::string& aCommit,
                                                  const std::string& aAncestor )
{
    KIGIT::GitCommitPtr commit = lookupCommit( aRepo, aCommit );
    KIGIT::GitCommitPtr ancestor = lookupCommit( aRepo, aAncestor );

    if( !commit || !ancestor )
        return std::nullopt;

    int rc = git_graph_descendant_of( aRepo, git_commit_id( commit.get() ), git_commit_id( ancestor.get() ) );

    if( rc < 0 )
        return std::nullopt;

    return rc == 1;
}


std::optional<KIGIT_COMMIT_RECORD> LIBGIT_BACKEND::ReadCommit( git_repository* aRepo, const std::string& aOid )
{
    KIGIT::GitCommitPtr commit = lookupCommit( aRepo, aOid );

    if( !commit )
        return std::nullopt;

    return recordOf( commit.get() );
}


int LIBGIT_BACKEND::WalkCommits( git_repository* aRepo, const std::string& aRef, const std::string& aHide,
                                 const std::function<bool( const KIGIT_COMMIT_RECORD& )>& aVisit )
{
    git_revwalk* walk = nullptr;

    if( !aRepo )
        return GIT_EINVALIDSPEC;

    if( int rc = git_revwalk_new( &walk, aRepo ) )
        return rc;

    KIGIT::GitRevWalkPtr walkPtr( walk );

    git_revwalk_sorting( walk, GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME );

    if( int rc = git_revwalk_push_ref( walk, aRef.c_str() ) )
        return rc;

    if( !aHide.empty() )
    {
        KIGIT::GitCommitPtr hide = lookupCommit( aRepo, aHide );

        if( !hide )
            return GIT_ENOTFOUND;

        if( int rc = git_revwalk_hide( walk, git_commit_id( hide.get() ) ) )
            return rc;
    }

    git_oid oid;
    int     rc = 0;

    while( ( rc = git_revwalk_next( &oid, walk ) ) == GIT_OK )
    {
        git_commit* commit = nullptr;

        if( int lookup = git_commit_lookup( &commit, aRepo, &oid ) )
            return lookup;

        KIGIT::GitCommitPtr commitPtr( commit );

        if( !aVisit( recordOf( commit ) ) )
            return GIT_EUSER;
    }

    return rc == GIT_ITEROVER ? GIT_OK : rc;
}


std::map<std::string, std::optional<std::string>>
LIBGIT_BACKEND::FilterTextsAt( git_repository* aRepo, const std::string& aCommit,
                               const std::map<std::string, std::string>& aTexts, bool aToRepository )
{
    std::map<std::string, std::optional<std::string>> result;
    KIGIT::GitCommitPtr                               commit = lookupCommit( aRepo, aCommit );
    git_tree*                                         tree = nullptr;
    git_repository*                                   view = nullptr;
    git_index*                                        index = nullptr;

    for( const auto& [path, text] : aTexts )
        result[path] = std::nullopt;

    if( !commit || git_commit_tree( &tree, commit.get() ) != GIT_OK )
        return result;

    KIGIT::GitTreePtr treePtr( tree );

    // A bare view has no working tree, so attributes come only from the index it is given
    if( git_repository_open_bare( &view, git_repository_path( aRepo ) ) != GIT_OK )
        return result;

    KIGIT::GitRepositoryPtr viewPtr( view );

    if( git_index_new( &index ) != GIT_OK )
        return result;

    KIGIT::GitIndexPtr indexPtr( index );

    if( git_index_read_tree( index, tree ) != GIT_OK || git_repository_set_index( view, index ) != GIT_OK )
        return result;

    for( const auto& [path, text] : aTexts )
        result[path] = FilterText( view, path, text, aToRepository );

    return result;
}


std::optional<std::string> LIBGIT_BACKEND::MergeBase( git_repository* aRepo, const std::string& aOne,
                                                      const std::string& aTwo )
{
    KIGIT::GitCommitPtr one = lookupCommit( aRepo, aOne );
    KIGIT::GitCommitPtr two = lookupCommit( aRepo, aTwo );
    git_oid             base;

    if( !one || !two || git_merge_base( &base, aRepo, git_commit_id( one.get() ), git_commit_id( two.get() ) ) )
        return std::nullopt;

    return std::string( git_oid_tostr_s( &base ) );
}


std::optional<std::vector<std::string>> LIBGIT_BACKEND::ChangedPaths( git_repository* aRepo, const std::string& aFrom,
                                                                      const std::string& aTo )
{
    KIGIT::GitCommitPtr from = lookupCommit( aRepo, aFrom );
    git_tree*           fromTree = nullptr;
    git_diff*           diff = nullptr;

    if( !from || git_commit_tree( &fromTree, from.get() ) != GIT_OK )
        return std::nullopt;

    KIGIT::GitTreePtr fromTreePtr( fromTree );

    if( aTo.empty() )
    {
        git_index* index = nullptr;

        if( git_repository_index( &index, aRepo ) != GIT_OK )
            return std::nullopt;

        KIGIT::GitIndexPtr indexPtr( index );

        // Passing the index skips libgit2's own reload, so staging by another writer would be missed
        if( git_index_read( index, 0 ) != GIT_OK
            || git_diff_tree_to_index( &diff, aRepo, fromTree, index, nullptr ) != GIT_OK )
            return std::nullopt;
    }
    else
    {
        KIGIT::GitCommitPtr to = lookupCommit( aRepo, aTo );
        git_tree*           toTree = nullptr;

        if( !to || git_commit_tree( &toTree, to.get() ) != GIT_OK )
            return std::nullopt;

        KIGIT::GitTreePtr toTreePtr( toTree );

        if( git_diff_tree_to_tree( &diff, aRepo, fromTree, toTree, nullptr ) != GIT_OK )
            return std::nullopt;
    }

    KIGIT::GitDiffPtr     diffPtr( diff );
    std::set<std::string> paths;

    for( size_t ii = 0; ii < git_diff_num_deltas( diff ); ++ii )
    {
        const git_diff_delta* delta = git_diff_get_delta( diff, ii );

        paths.insert( delta->old_file.path );
        paths.insert( delta->new_file.path );
    }

    return std::vector<std::string>( paths.begin(), paths.end() );
}
