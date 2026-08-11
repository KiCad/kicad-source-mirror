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
#include "kicad_git_compat.h"
#include "kicad_git_memory.h"
#include "project_git_utils.h"
#include "kigit_lock_retry.h"

#include <git2.h>


namespace
{

bool parseOid( git_oid& aOid, const std::string& aText )
{
    return aText.size() == GIT_OID_HEXSZ && git_oid_fromstr( &aOid, aText.c_str() ) == GIT_OK;
}

} // namespace


std::optional<std::string> LIBGIT_BACKEND::ReadReference( git_repository* aRepo, const std::string& aName )
{
    git_oid oid;

    if( !aRepo || git_reference_name_to_id( &oid, aRepo, aName.c_str() ) != GIT_OK )
        return std::nullopt;

    return std::string( git_oid_tostr_s( &oid ) );
}


std::string LIBGIT_BACKEND::HeadTarget( git_repository* aRepo )
{
    git_reference* head = nullptr;

    if( !aRepo || git_reference_lookup( &head, aRepo, "HEAD" ) != GIT_OK )
        return std::string();

    KIGIT::GitReferencePtr headPtr( head );
    const char*            target = git_reference_symbolic_target( head );

    return target ? target : "HEAD";
}


int LIBGIT_BACKEND::SetReference( git_repository* aRepo, const std::string& aName, const std::string& aOid,
                                  const std::optional<std::string>& aExpected, const std::string& aLogMessage )
{
    git_oid oid;
    git_oid expected = {};

    if( !aRepo || !parseOid( oid, aOid ) || ( aExpected && !aExpected->empty() && !parseOid( expected, *aExpected ) ) )
        return GIT_EINVALIDSPEC;

    git_reference* ref = nullptr;
    int            rc = 0;

    // A zero expected id is checked under the ref lock, where force=0 alone checks before taking it
    if( aExpected )
        rc = git_reference_create_matching( &ref, aRepo, aName.c_str(), &oid, 1, &expected, aLogMessage.c_str() );
    else
        rc = git_reference_create( &ref, aRepo, aName.c_str(), &oid, 1, aLogMessage.c_str() );

    git_reference_free( ref );

    return rc == GIT_EEXISTS ? GIT_EMODIFIED : rc;
}


int LIBGIT_BACKEND::DeleteReference( git_repository* aRepo, const std::string& aName,
                                     const std::optional<std::string>& aExpected )
{
    if( !aRepo )
        return GIT_EINVALIDSPEC;

    git_reference* ref = nullptr;

    if( int rc = git_reference_lookup( &ref, aRepo, aName.c_str() ) )
        return rc == GIT_ENOTFOUND ? GIT_OK : rc;

    KIGIT::GitReferencePtr refPtr( ref );
    const git_oid*         target = git_reference_target( ref );

    if( aExpected && ( !target || *aExpected != git_oid_tostr_s( target ) ) )
        return GIT_EMODIFIED;

    // Deleting through the reference read fails if it moved since
    return git_reference_delete( ref );
}


std::vector<KIGIT_REFERENCE> LIBGIT_BACKEND::ListReferences( git_repository* aRepo, const std::string& aGlob )
{
    std::vector<KIGIT_REFERENCE> refs;
    git_reference_iterator*      it = nullptr;

    if( !aRepo || git_reference_iterator_glob_new( &it, aRepo, aGlob.c_str() ) != GIT_OK )
        return refs;

    KIGIT::GitReferenceIteratorPtr itPtr( it );
    git_reference*                 ref = nullptr;

    while( git_reference_next( &ref, it ) == GIT_OK )
    {
        KIGIT::GitReferencePtr refPtr( ref );
        git_oid                oid;

        if( git_reference_name_to_id( &oid, aRepo, git_reference_name( ref ) ) == GIT_OK )
            refs.push_back( { git_reference_name( ref ), git_oid_tostr_s( &oid ) } );
    }

    return refs;
}


std::optional<std::string> LIBGIT_BACKEND::CreateRecordCommit( git_repository* aRepo, const std::string& aMessage,
                                                               const std::string& aAuthorName,
                                                               const std::string& aAuthorEmail )
{
    git_treebuilder* builder = nullptr;
    git_oid          treeOid;
    git_oid          commitOid;
    git_tree*        tree = nullptr;
    git_signature*   signature = nullptr;

    if( !aRepo || git_treebuilder_new( &builder, aRepo, nullptr ) != GIT_OK )
        return std::nullopt;

    int rc = git_treebuilder_write( &treeOid, builder );
    git_treebuilder_free( builder );

    if( rc != GIT_OK || git_tree_lookup( &tree, aRepo, &treeOid ) != GIT_OK )
        return std::nullopt;

    KIGIT::GitTreePtr treePtr( tree );

    if( git_signature_now( &signature, aAuthorName.c_str(), aAuthorEmail.c_str() ) != GIT_OK )
        return std::nullopt;

    KIGIT::GitSignaturePtr signaturePtr( signature );

    if( git_commit_create( &commitOid, aRepo, nullptr, signature, signature, nullptr, aMessage.c_str(), tree, 0,
                           nullptr )
        != GIT_OK )
    {
        return std::nullopt;
    }

    return std::string( git_oid_tostr_s( &commitOid ) );
}


int LIBGIT_BACKEND::CreateAnnotatedTag( git_repository* aRepo, const std::string& aName, const std::string& aTarget,
                                        const std::string& aMessage, const std::string& aTaggerName,
                                        const std::string& aTaggerEmail, std::string& aTagOid )
{
    git_oid     targetOid;
    git_object* target = nullptr;
    int         valid = 0;

    if( !aRepo || !parseOid( targetOid, aTarget ) || git_tag_name_is_valid( &valid, aName.c_str() ) || !valid )
        return GIT_EINVALIDSPEC;

    if( int rc = git_object_lookup( &target, aRepo, &targetOid, GIT_OBJECT_COMMIT ) )
        return rc;

    KIGIT::GitObjectPtr targetPtr( target );
    git_signature*      tagger = nullptr;

    if( int rc = git_signature_now( &tagger, aTaggerName.c_str(), aTaggerEmail.c_str() ) )
        return rc;

    KIGIT::GitSignaturePtr taggerPtr( tagger );
    git_oid                tagOid;

    if( int rc = git_tag_annotation_create( &tagOid, aRepo, aName.c_str(), target, tagger, aMessage.c_str() ) )
        return rc;

    const std::string oid = git_oid_tostr_s( &tagOid );

    // force=0 looks for the tag before taking the ref lock, so only a swap from nothing never replaces one
    if( int rc = SetReference( aRepo, "refs/tags/" + aName, oid, std::string(), "tag: " + aName ) )
        return rc == GIT_EMODIFIED ? GIT_EEXISTS : rc;

    aTagOid = oid;
    return GIT_OK;
}


std::optional<KIGIT_TAG_RECORD> LIBGIT_BACKEND::ReadAnnotatedTag( git_repository* aRepo, const std::string& aRefName )
{
    git_reference* ref = nullptr;

    if( !aRepo || git_reference_lookup( &ref, aRepo, aRefName.c_str() ) != GIT_OK )
        return std::nullopt;

    KIGIT::GitReferencePtr refPtr( ref );
    const git_oid*         target = git_reference_target( ref );
    git_tag*               tag = nullptr;

    if( !target || git_tag_lookup( &tag, aRepo, target ) != GIT_OK )
        return std::nullopt;

    std::unique_ptr<git_tag, decltype( &git_tag_free )> tagPtr( tag, &git_tag_free );
    KIGIT_TAG_RECORD                                    record;

    record.oid = git_oid_tostr_s( target );
    record.target = git_oid_tostr_s( git_tag_target_id( tag ) );
    record.message = git_tag_message( tag ) ? git_tag_message( tag ) : "";

    if( const git_signature* tagger = git_tag_tagger( tag ) )
    {
        record.taggerName = tagger->name ? tagger->name : "";
        record.taggerEmail = tagger->email ? tagger->email : "";
        record.time = static_cast<std::int64_t>( tagger->when.time );
    }

    return record;
}


git_repository* LIBGIT_BACKEND::OpenRepositoryAt( const std::string& aPath )
{
    git_repository* repo = nullptr;

    if( git_repository_open_ext( &repo, aPath.c_str(), GIT_REPOSITORY_OPEN_NO_SEARCH, nullptr ) != GIT_OK )
        return nullptr;

    KIGIT::PROJECT_GIT_UTILS::AttachLongPaths( repo );
    return repo;
}


int LIBGIT_BACKEND::FindRepository( const std::string& aPath, git_repository*& aRepo )
{
    aRepo = nullptr;

    // A mount inside a working tree is still that tree
    int rc = git_repository_open_ext( &aRepo, aPath.c_str(), GIT_REPOSITORY_OPEN_CROSS_FS, nullptr );

    if( rc == GIT_OK )
        KIGIT::PROJECT_GIT_UTILS::AttachLongPaths( aRepo );

    return rc;
}


/// Make the index hold commit aOid's tree, staging nothing
static int readIntoIndex( git_repository* aRepo, const git_oid& aOid )
{
    git_commit* commit = nullptr;
    git_tree*   tree = nullptr;
    git_index*  index = nullptr;

    if( int rc = git_commit_lookup( &commit, aRepo, &aOid ) )
        return rc;

    KIGIT::GitCommitPtr commitPtr( commit );

    if( int rc = git_commit_tree( &tree, commit ) )
        return rc;

    KIGIT::GitTreePtr treePtr( tree );

    if( int rc = git_repository_index( &index, aRepo ) )
        return rc;

    KIGIT::GitIndexPtr indexPtr( index );

    if( int rc = git_index_read_tree( index, tree ) )
        return rc;

    return git_index_write( index );
}


int LIBGIT_BACKEND::CheckoutCommit( git_repository* aRepo, const std::string& aCommit, const std::string& aBranch )
{
    git_oid     oid;
    git_commit* commit = nullptr;

    if( !aRepo || !parseOid( oid, aCommit ) )
        return GIT_EINVALIDSPEC;

    if( int rc = git_commit_lookup( &commit, aRepo, &oid ) )
        return rc;

    KIGIT::GitCommitPtr  commitPtr( commit );
    git_checkout_options options;

    git_checkout_options_init( &options, GIT_CHECKOUT_OPTIONS_VERSION );
    // An ignored file is still someone's, and a status check never lists it
    options.checkout_strategy = GIT_CHECKOUT_SAFE | GIT_CHECKOUT_DONT_OVERWRITE_IGNORED;

    std::string blocking;

    options.notify_flags = GIT_CHECKOUT_NOTIFY_CONFLICT;
    options.notify_payload = &blocking;
    options.notify_cb = []( git_checkout_notify_t, const char* aPath, const git_diff_file*, const git_diff_file*,
                            const git_diff_file*, void*        aPayload ) -> int
    {
        std::string& first = *static_cast<std::string*>( aPayload );

        if( first.empty() && aPath )
            first = aPath;

        return 0;
    };

    if( int rc = git_checkout_tree( aRepo, reinterpret_cast<git_object*>( commit ), &options ) )
    {
        // libgit2 only counts the conflicts, which leaves the user no file to move
        if( rc == GIT_ECONFLICT && !blocking.empty() )
            git_error_set_str( GIT_ERROR_CHECKOUT, ( "'" + blocking + "' is in the way" ).c_str() );

        return rc;
    }

    auto moveHead = [&]()
    {
        if( aBranch.empty() )
            return git_repository_set_head_detached( aRepo, &oid );

        const std::string branchRef = "refs/heads/" + aBranch;
        git_reference*    ref = nullptr;

        if( int rc = git_reference_create( &ref, aRepo, branchRef.c_str(), &oid, 1, "checkout" ) )
            return rc;

        git_reference_free( ref );
        return git_repository_set_head( aRepo, branchRef.c_str() );
    };

    const int   rc = moveHead();
    git_object* head = nullptr;
    git_tree*   baseline = nullptr;

    if( rc == GIT_OK || git_revparse_single( &head, aRepo, "HEAD^{commit}" ) != GIT_OK )
        return rc;

    KIGIT::GitObjectPtr headPtr( head );

    // A tree left at aCommit under an older HEAD reads as local edits, which the next check-in would commit
    if( git_commit_tree( &baseline, commit ) == GIT_OK )
    {
        KIGIT::GitTreePtr baselinePtr( baseline );

        options.baseline = baseline;
        options.notify_cb = nullptr;
        git_checkout_tree( aRepo, head, &options );
    }

    // An index left at aCommit under an older HEAD reads as staged work, which every later update refuses
    readIntoIndex( aRepo, *git_object_id( head ) );
    return rc;
}


int LIBGIT_BACKEND::ResetIndex( git_repository* aRepo, const std::string& aCommit )
{
    git_oid oid;

    if( !aRepo || !parseOid( oid, aCommit ) )
        return GIT_EINVALIDSPEC;

    return readIntoIndex( aRepo, oid );
}


int LIBGIT_BACKEND::CheckoutFromIndex( git_repository* aRepo, const std::vector<std::string>& aPaths )
{
    if( !aRepo )
        return GIT_EINVALIDSPEC;

    if( aPaths.empty() )
        return GIT_OK;

    git_index* index = nullptr;

    if( int rc = git_repository_index( &index, aRepo ) )
        return rc;

    KIGIT::GitIndexPtr indexPtr( index );

    // Forced, so an unwritten change in this handle's copy never stands in for what is staged
    if( int rc = git_index_read( index, 1 ) )
        return rc;

    std::vector<const char*> paths;

    for( const std::string& path : aPaths )
    {
        // Like git checkout, a path with nothing staged is refused rather than deleted
        if( !git_index_get_bypath( index, path.c_str(), 0 ) )
        {
            git_error_set_str( GIT_ERROR_INDEX, ( path + " is not staged" ).c_str() );
            return GIT_ENOTFOUND;
        }

        paths.push_back( path.c_str() );
    }

    git_checkout_options options;
    git_checkout_options_init( &options, GIT_CHECKOUT_OPTIONS_VERSION );
    options.checkout_strategy =
            GIT_CHECKOUT_FORCE | GIT_CHECKOUT_DISABLE_PATHSPEC_MATCH | GIT_CHECKOUT_DONT_UPDATE_INDEX;
    options.baseline_index = index;
    options.paths.strings = const_cast<char**>( paths.data() );
    options.paths.count = paths.size();

    return git_checkout_index( aRepo, index, &options );
}


int LIBGIT_BACKEND::CommitFiles( git_repository* aRepo, const std::string& aBranch, const std::string& aParent,
                                 const std::map<std::string, std::optional<std::string>>& aFiles,
                                 const std::string& aMessage, const std::string& aAuthorName,
                                 const std::string& aAuthorEmail, std::string& aCommit )
{
    git_oid     parentOid;
    git_commit* parent = nullptr;
    git_tree*   parentTree = nullptr;

    if( !aRepo || aBranch.empty() || !parseOid( parentOid, aParent ) )
        return GIT_EINVALIDSPEC;

    if( int rc = git_commit_lookup( &parent, aRepo, &parentOid ) )
        return rc;

    KIGIT::GitCommitPtr parentPtr( parent );

    if( int rc = git_commit_tree( &parentTree, parent ) )
        return rc;

    KIGIT::GitTreePtr            parentTreePtr( parentTree );
    std::vector<git_tree_update> updates;
    std::vector<git_oid>         blobs( aFiles.size() );
    size_t                       slot = 0;

    for( const auto& [path, text] : aFiles )
    {
        git_tree_update update = {};
        update.path = path.c_str();
        update.action = text ? GIT_TREE_UPDATE_UPSERT : GIT_TREE_UPDATE_REMOVE;

        if( text )
        {
            if( int rc = git_blob_create_from_buffer( &blobs[slot], aRepo, text->data(), text->size() ) )
                return rc;

            update.id = blobs[slot];
            update.filemode = GIT_FILEMODE_BLOB;
        }

        updates.push_back( update );
        ++slot;
    }

    git_oid        treeOid;
    git_tree*      tree = nullptr;
    git_signature* signature = nullptr;
    git_oid        commitOid;
    git_reference* ref = nullptr;

    if( int rc = git_tree_create_updated( &treeOid, aRepo, parentTree, updates.size(), updates.data() ) )
        return rc;

    if( int rc = git_tree_lookup( &tree, aRepo, &treeOid ) )
        return rc;

    KIGIT::GitTreePtr treePtr( tree );

    if( int rc = git_signature_now( &signature, aAuthorName.c_str(), aAuthorEmail.c_str() ) )
        return rc;

    KIGIT::GitSignaturePtr signaturePtr( signature );
    const git_commit*      parents[] = { parent };
    const std::string      branchRef = "refs/heads/" + aBranch;

    if( int rc = git_commit_create( &commitOid, aRepo, nullptr, signature, signature, nullptr, aMessage.c_str(), tree,
                                    1, parents ) )
    {
        return rc;
    }

    if( int rc = git_reference_create_matching( &ref, aRepo, branchRef.c_str(), &commitOid, 1, &parentOid,
                                                aMessage.c_str() ) )
    {
        return rc == GIT_EEXISTS ? GIT_EMODIFIED : rc;
    }

    git_reference_free( ref );
    aCommit = git_oid_tostr_s( &commitOid );

    std::vector<std::string> paths;

    for( const auto& [path, text] : aFiles )
        paths.push_back( path );

    // The commit stands without this, so an index that is locked or unreadable is left alone
    MoveStaging( aRepo, aParent, aCommit, paths );

    return GIT_OK;
}


int LIBGIT_BACKEND::MoveStaging( git_repository* aRepo, const std::string& aFrom, const std::string& aTo,
                                 const std::vector<std::string>& aPaths )
{
    git_oid     fromOid;
    git_oid     toOid;
    git_commit* from = nullptr;
    git_commit* to = nullptr;
    git_tree*   fromTree = nullptr;
    git_tree*   toTree = nullptr;

    if( !aRepo || !parseOid( fromOid, aFrom ) || !parseOid( toOid, aTo ) )
        return GIT_EINVALIDSPEC;

    if( int rc = git_commit_lookup( &from, aRepo, &fromOid ) )
        return rc;

    KIGIT::GitCommitPtr fromPtr( from );

    if( int rc = git_commit_lookup( &to, aRepo, &toOid ) )
        return rc;

    KIGIT::GitCommitPtr toPtr( to );

    if( int rc = git_commit_tree( &fromTree, from ) )
        return rc;

    KIGIT::GitTreePtr fromTreePtr( fromTree );

    if( int rc = git_commit_tree( &toTree, to ) )
        return rc;

    KIGIT::GitTreePtr toTreePtr( toTree );

    auto move = [&]( git_index* aIndex )
    {
        for( const std::string& path : aPaths )
        {
            const git_index_entry* entry = git_index_get_bypath( aIndex, path.c_str(), 0 );
            git_tree_entry*        old = nullptr;
            bool                   current = !entry;

            // A path moves on only while it is staged as aFrom holds it, mode included
            if( git_tree_entry_bypath( &old, fromTree, path.c_str() ) == GIT_OK )
            {
                KIGIT::GitTreeEntryPtr oldPtr( old );
                current = entry && entry->mode == static_cast<uint32_t>( git_tree_entry_filemode( old ) )
                          && git_oid_equal( &entry->id, git_tree_entry_id( old ) );
            }

            if( !current )
                continue;

            git_tree_entry* target = nullptr;
            const int       found = git_tree_entry_bypath( &target, toTree, path.c_str() );

            if( found == GIT_ENOTFOUND )
            {
                git_index_remove_bypath( aIndex, path.c_str() );
                continue;
            }

            if( found )
                return found;

            KIGIT::GitTreeEntryPtr targetPtr( target );
            git_index_entry        added = {};
            added.mode = git_tree_entry_filemode( target );
            added.id = *git_tree_entry_id( target );
            added.path = path.c_str();

            if( int rc = git_index_add( aIndex, &added ) )
                return rc;
        }

        return 0;
    };

    return KIGIT::EditIndexLocked( aRepo, move );
}
