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
 * WITHOUT ANY WARRANTY: without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef QA_GIT_TEST_FIXTURE_H
#define QA_GIT_TEST_FIXTURE_H

#include <boost/test/unit_test.hpp>
#include <git/kicad_git_memory.h>
#include <qa_utils/file_utils.h>
#include <qa_utils/scoped_git_backend.h>

#include <wx/file.h>
#include <wx/filename.h>

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace KI_TEST
{

/// Point every config level above the repository at one directory, restoring the previous paths afterwards.
class SCOPED_GIT_CONFIG_SEARCH_PATH
{
public:
    explicit SCOPED_GIT_CONFIG_SEARCH_PATH( const std::string& aPath )
    {
        for( git_config_level_t level :
             { GIT_CONFIG_LEVEL_PROGRAMDATA, GIT_CONFIG_LEVEL_SYSTEM, GIT_CONFIG_LEVEL_XDG, GIT_CONFIG_LEVEL_GLOBAL } )
        {
            git_buf previous = GIT_BUF_INIT;
            BOOST_REQUIRE_EQUAL( git_libgit2_opts( GIT_OPT_GET_SEARCH_PATH, level, &previous ), GIT_OK );
            m_saved.emplace_back( level, previous.ptr ? previous.ptr : "" );
            git_buf_dispose( &previous );
        }

        bool overridden = true;

        for( const auto& [level, searchPath] : m_saved )
            overridden &= git_libgit2_opts( GIT_OPT_SET_SEARCH_PATH, level, aPath.c_str() ) == GIT_OK;

        // A throwing constructor skips the destructor, so undo a partial override here
        if( !overridden )
            restore();

        BOOST_REQUIRE( overridden );
    }

    ~SCOPED_GIT_CONFIG_SEARCH_PATH() { restore(); }

    SCOPED_GIT_CONFIG_SEARCH_PATH( const SCOPED_GIT_CONFIG_SEARCH_PATH& ) = delete;
    SCOPED_GIT_CONFIG_SEARCH_PATH& operator=( const SCOPED_GIT_CONFIG_SEARCH_PATH& ) = delete;

private:
    void restore()
    {
        for( const auto& [level, searchPath] : m_saved )
            git_libgit2_opts( GIT_OPT_SET_SEARCH_PATH, level, searchPath.c_str() );
    }

    std::vector<std::pair<git_config_level_t, std::string>> m_saved;
};


/// Give each test a backend and a temporary directory. Tests own their repository handles.
/// Only repository config is read, so results do not depend on the user's global or system Git config.
class GIT_TEST_FIXTURE
{
public:
    GIT_TEST_FIXTURE() :
            m_tempDir( "kicad-qa-git" ),
            m_configDir( "kicad-qa-gitconfig" ),
            m_configSearchPath( toWxString( m_configDir.Path() ).utf8_string() )
    {
        // Resolve temporary-directory symlinks to match libgit2's workdir path.
        m_tempPath = toWxString( std::filesystem::canonical( m_tempDir.Path() ) );
    }

    const wxString& TempPath() const { return m_tempPath; }
    LIBGIT_BACKEND* Backend() const { return m_backend.Backend(); }

private:
    static wxString toWxString( const std::filesystem::path& aPath )
    {
#ifdef __WINDOWS__
        return aPath.wstring();
#else
        return wxString::FromUTF8( aPath.native() );
#endif
    }

    // Declared after the backend so the search paths are set after libgit2 starts and restored before it stops
    SCOPED_GIT_BACKEND            m_backend;
    SCOPED_TEMP_DIR               m_tempDir;
    SCOPED_TEMP_DIR               m_configDir;
    SCOPED_GIT_CONFIG_SEARCH_PATH m_configSearchPath;
    wxString                      m_tempPath;
};


/// Pin the branch and author so repository setup does not depend on the user's Git config.
inline KIGIT::GitRepositoryPtr InitGitRepository( const wxString& aPath, const char* aBranch = "main",
                                                  const char* aAuthorName = "QA Test",
                                                  const char* aAuthorEmail = "qa@example.com" )
{
    git_repository_init_options options = GIT_REPOSITORY_INIT_OPTIONS_INIT;
    options.flags = GIT_REPOSITORY_INIT_MKPATH;
    options.initial_head = aBranch;

    git_repository*         repo = nullptr;
    int                     result = git_repository_init_ext( &repo, aPath.utf8_str(), &options );
    KIGIT::GitRepositoryPtr repoPtr( repo );
    BOOST_REQUIRE_EQUAL( result, GIT_OK );

    git_config* config = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_config( &config, repo ), GIT_OK );
    KIGIT::GitConfigPtr configPtr( config );
    BOOST_REQUIRE_EQUAL( git_config_set_string( config, "user.name", aAuthorName ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_config_set_string( config, "user.email", aAuthorEmail ), GIT_OK );

    return repoPtr;
}


/// Write test data without staging it. Create parent directories when needed.
inline void WriteGitFile( const wxString& aRepoPath, const wxString& aRelativePath, const std::string& aContents )
{
    wxFileName path( aRepoPath + wxFileName::GetPathSeparator() + aRelativePath );

    if( !wxFileName::DirExists( path.GetPath() ) )
        BOOST_REQUIRE( wxFileName::Mkdir( path.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    wxFile file( path.GetFullPath(), wxFile::write );
    BOOST_REQUIRE( file.IsOpened() );
    BOOST_REQUIRE_EQUAL( file.Write( aContents.data(), aContents.size() ), aContents.size() );
    BOOST_REQUIRE( file.Close() );
}


/// Commit the current index, using HEAD as the parent when it exists.
inline git_oid CommitGitIndex( git_repository* aRepo, const wxString& aMessage )
{
    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, aRepo ), GIT_OK );
    KIGIT::GitIndexPtr indexPtr( index );

    git_oid treeOid;
    BOOST_REQUIRE_EQUAL( git_index_write_tree( &treeOid, index ), GIT_OK );

    git_tree* tree = nullptr;
    BOOST_REQUIRE_EQUAL( git_tree_lookup( &tree, aRepo, &treeOid ), GIT_OK );
    KIGIT::GitTreePtr treePtr( tree );

    git_signature* signature = nullptr;
    BOOST_REQUIRE_EQUAL( git_signature_default( &signature, aRepo ), GIT_OK );
    KIGIT::GitSignaturePtr signaturePtr( signature );

    git_oid             headOid;
    int                 result = git_reference_name_to_id( &headOid, aRepo, "HEAD" );
    KIGIT::GitCommitPtr parent;

    if( result == GIT_OK )
    {
        git_commit* commit = nullptr;
        BOOST_REQUIRE_EQUAL( git_commit_lookup( &commit, aRepo, &headOid ), GIT_OK );
        parent.reset( commit );
    }
    else
    {
        BOOST_REQUIRE( result == GIT_ENOTFOUND || result == GIT_EUNBORNBRANCH );
    }

    const git_commit* parents[] = { parent.get() };
    git_oid           commitOid;
    BOOST_REQUIRE_EQUAL( git_commit_create( &commitOid, aRepo, "HEAD", signature, signature, nullptr,
                                            aMessage.utf8_str(), tree, parent ? 1 : 0, parent ? parents : nullptr ),
                         GIT_OK );
    return commitOid;
}


/// Stage one file and commit the index. Other staged changes are kept.
inline git_oid CommitGitFile( git_repository* aRepo, const wxString& aPath, const wxString& aMessage )
{
    {
        git_index* index = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_index( &index, aRepo ), GIT_OK );
        KIGIT::GitIndexPtr indexPtr( index );
        BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, aPath.utf8_str() ), GIT_OK );
        BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );
    }

    return CommitGitIndex( aRepo, aMessage );
}


/// Stage all changes, including deletions, and commit them.
inline git_oid CommitAllGitFiles( git_repository* aRepo, const wxString& aMessage )
{
    {
        git_index* index = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_index( &index, aRepo ), GIT_OK );
        KIGIT::GitIndexPtr indexPtr( index );
        BOOST_REQUIRE_EQUAL( git_index_add_all( index, nullptr, GIT_INDEX_ADD_DEFAULT, nullptr, nullptr ), GIT_OK );
        BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );
    }

    return CommitGitIndex( aRepo, aMessage );
}

} // namespace KI_TEST

#endif // QA_GIT_TEST_FIXTURE_H
