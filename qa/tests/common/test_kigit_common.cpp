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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <git/project_git_utils.h>
#include <git/git_branch_handler.h>
#include <qa_utils/scoped_git_backend.h>

#include <git2.h>

#include <wx/file.h>
#include <wx/filename.h>
#include <wx/string.h>
#include <wx/stdpaths.h>

#include <chrono>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <memory>
#include <cstdlib>


namespace
{

// Throwaway keys, generated for these tests and used nowhere else.

const char* c_opensshEncrypted = "-----BEGIN OPENSSH PRIVATE KEY-----\n"
                                 "b3BlbnNzaC1rZXktdjEAAAAACmFlczI1Ni1jdHIAAAAGYmNyeXB0AAAAGAAAABAxS2G8mY\n"
                                 "+85LKkVcFtJi1GAAAAGAAAAAEAAAAzAAAAC3NzaC1lZDI1NTE5AAAAIKB0oSTjiyvxC+qh\n"
                                 "cJ2zEnPDzZWArHcl90v2Se1SkmoGAAAAkKKrIYipMtR+JUEsuoYSmnfD0b/ng57Fq2uj25\n"
                                 "oHDRqrDyPKS0xqQJa9+IQvPeM+E3G1fAGvsOYTQXsLJzXuQDiLX3vceEuN9AmJkA9Naea4\n"
                                 "qAPy8CTlBx/ktdd4YgnYXBETi6Ab1f/h7LAVlLJTOc9Ta9HilKzO8Vl5p3EUS1PWkfGGqe\n"
                                 "QLo1/ryfJjC3ZZbA==\n"
                                 "-----END OPENSSH PRIVATE KEY-----\n";


const char* c_opensshPlain = "-----BEGIN OPENSSH PRIVATE KEY-----\n"
                             "b3BlbnNzaC1rZXktdjEAAAAABG5vbmUAAAAEbm9uZQAAAAAAAAABAAAAMwAAAAtzc2gtZW\n"
                             "QyNTUxOQAAACD4Xd9qfxjO+vpBXO94Otoa0kg/p5H8kMYhN6eFe7v8MgAAAIjYZhWO2GYV\n"
                             "jgAAAAtzc2gtZWQyNTUxOQAAACD4Xd9qfxjO+vpBXO94Otoa0kg/p5H8kMYhN6eFe7v8Mg\n"
                             "AAAEBNlyz61Kiufy1a3WO6i2I7vB+7W2iQc8R81H0qrBheSfhd32p/GM76+kFc73g62hrS\n"
                             "SD+nkfyQxiE3p4V7u/wyAAAAAXQBAgME\n"
                             "-----END OPENSSH PRIVATE KEY-----\n";


const char* c_opensshTruncated = "-----BEGIN OPENSSH PRIVATE KEY-----\n"
                                 "b3BlbnNzaC1r\n"
                                 "-----END OPENSSH PRIVATE KEY-----\n";


const char* c_pemEncrypted = "-----BEGIN EC PRIVATE KEY-----\n"
                             "Proc-Type: 4,ENCRYPTED\n"
                             "DEK-Info: AES-128-CBC,10297828DB5031A5C22276D18E077B61\n"
                             "\n"
                             "rHfBqMZtByvcsRb7zgNtkSjhFgnRY3jpyyagjM8jxNoEDRaSSHNSugON80psxytE\n"
                             "HBTGShcJbDXOC7EER7QZOTjM/d+3DB+FEL4EwnJxgnVfWN/5p9hP3CoJLgeItkQV\n"
                             "POqt3mQ1V8AnJibunyn13YrczHM39IFruBB9JvNKKG+i/7yf4Jy4jCB9nicH7TC+\n"
                             "hQCu/mOMde02ziwULJiWIU3EALSYo6JhDJxTRIqUr7TDGXEAPcxpQEvMDz+HAgzY\n"
                             "9zTU6LC/CSbHQNjHYSvgDY4zRDFWxeHgNeQVfqrlOPGGE8NC5HCeo5i1r+rSVPn+\n"
                             "kzqXcGpwN682lM/oJV4ix3her0jf59RnAPKYwyWXfhUwYFtzFwroOoquFi6+HHVT\n"
                             "LlzJacbVVj/GTBOdHBSsu8cfiVLymHhEEzQCyIYY05DeHFe8GiSwDDdvMCPppX38\n"
                             "/R3PnNaEYwOEUK4o50nsWEfPHQw8kZaUkiUcX7uIyV4=\n"
                             "-----END EC PRIVATE KEY-----\n";


const char* c_pemPlain = "-----BEGIN EC PRIVATE KEY-----\n"
                         "MIIBaAIBAQQgGuajOowPzHaTJKkj1W+RQS0p87K8bEkZbgbw5pmVArmggfowgfcC\n"
                         "AQEwLAYHKoZIzj0BAQIhAP////8AAAABAAAAAAAAAAAAAAAA////////////////\n"
                         "MFsEIP////8AAAABAAAAAAAAAAAAAAAA///////////////8BCBaxjXYqjqT57Pr\n"
                         "vVV2mIa8ZR0GsMxTsPY7zjw+J9JgSwMVAMSdNgiG5wSTamZ44ROdJreBn36QBEEE\n"
                         "axfR8uEsQkf4vOblY6RA8ncDfYEt6zOg9KE5RdiYwpZP40Li/hp/m47n60p8D54W\n"
                         "K84zV2sxXs7LtkBoN79R9QIhAP////8AAAAA//////////+85vqtpxeehPO5ysL8\n"
                         "YyVRAgEBoUQDQgAELhPMgsc2xVYFBrKzqXq8CBRS8t9BBCnznM96FcZFv1MvdbIK\n"
                         "yn1nix57Bwq0VoyooErjaZ3SpXl47JlJpIsm4A==\n"
                         "-----END EC PRIVATE KEY-----\n";


const char* c_pkcs8Encrypted = "-----BEGIN ENCRYPTED PRIVATE KEY-----\n"
                               "MIIBzzBJBgkqhkiG9w0BBQ0wPDAbBgkqhkiG9w0BBQwwDgQImaRUWsA/SFECAggA\n"
                               "MB0GCWCGSAFlAwQBAgQQiywEwYp7En3NzOemv0ek9QSCAYAIh56t3GB2r1S/B7Cy\n"
                               "UFS4qn1DvGDuhubEl6cw3n8Y1PnWj6ryq6O9Pw/DKdnKonzVN+PVaofXg26ZPLdR\n"
                               "q+J7fmcMPIEC0DqV/6T+oGqWp2iaiTZ11BBOoyBkd8ev59c/TxngOWlbJvvmGhEF\n"
                               "mOiCwXKoKjzU8kSVbmL+vFe1thZjrCaihL79JsZE6Ws5bd7yAn6grPBPI8sBexNi\n"
                               "fERG81T9Nu2mxCEd7zkNAoK0cZBnUwFvxDama18wMvXRYzEHETCUCWevoNFdcwo/\n"
                               "IkGgLciMxtSBBOXyWiVvAzTloSGHa5wafptTsgyTc1HKZBTR0ZTNvBX37aziggWE\n"
                               "u4fMHcHlAJwP4UXgCACtgcfktVhjohgNQdBm5RB95qEUdNAQDQSe4Ro27HfT0YyR\n"
                               "AHZYNXziRjZHsO6wqGarcxdZkH1Cy8Ca9FJr3uRWWr+8CWIjE9nDEkFbXUyQGOPq\n"
                               "sTAC7FpnQHVczHcaa45jXYaPmU/iNFnPkO2DzxWADe/1xIg=\n"
                               "-----END ENCRYPTED PRIVATE KEY-----\n";


// KIGIT_COMMON routes its repository edits through the installed backend, and libgit2 must be
// initialized, so every test installs one for its lifetime through the shared helper.
using GitInitGuard = SCOPED_GIT_BACKEND;


struct ScopedTempDir
{
    wxString path;

    ScopedTempDir()
    {
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        long long ticks = std::chrono::duration_cast<std::chrono::nanoseconds>( now ).count();

        wxString base = wxFileName::GetTempDir();
        wxFileName fn;
        fn.AssignDir( base );
        fn.AppendDir( wxString::Format( "kicad-qa-git-%lld-%p", ticks, this ) );
        wxFileName::Mkdir( fn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

#ifdef __WXMAC__
        // Store the realpath-resolved form.  libgit2 canonicalizes the repo workdir with
        // realpath(3), so on macOS -- where /var is a symlink to /private/var -- any path
        // derived from the repo differs textually from the one we built above unless we
        // resolve symlinks here too.  Comparisons against GetGitRootDirectory() rely on this.
        char* resolved = realpath( fn.GetPath().utf8_string().c_str(), nullptr );

        if( resolved )
        {
            path = wxString::FromUTF8( resolved );
            free( resolved );
        }
        else
        {
            path = fn.GetPath();
        }
#else
        path = fn.GetPath();
#endif
    }

    ~ScopedTempDir()
    {
        if( !path.IsEmpty() )
            wxFileName::Rmdir( path, wxPATH_RMDIR_RECURSIVE );
    }
};


// Initialize a git repository at the given path with one initial commit on "main"
git_repository* makeRepoWithCommit( const wxString& aRepoPath, const wxString& aFileName,
                                    const std::string& aFileContents )
{
    git_repository* repo = nullptr;
    git_repository_init_options init_opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
    init_opts.flags = GIT_REPOSITORY_INIT_MKPATH;
    init_opts.initial_head = "main";

    BOOST_REQUIRE_EQUAL( git_repository_init_ext( &repo, aRepoPath.utf8_string().c_str(),
                                                  &init_opts ),
                         GIT_OK );

    // Write a file
    wxFileName filePath( aRepoPath, aFileName );
    {
        std::ofstream f( filePath.GetFullPath().utf8_string() );
        f << aFileContents;
    }

    // Configure user
    git_config* cfg = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_config( &cfg, repo ), GIT_OK );
    KIGIT::GitConfigPtr cfgPtr( cfg );
    git_config_set_string( cfg, "user.name", "QA Test" );
    git_config_set_string( cfg, "user.email", "qa@example.com" );

    // Stage and commit
    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    KIGIT::GitIndexPtr indexPtr( index );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, aFileName.utf8_string().c_str() ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );

    git_oid tree_oid;
    BOOST_REQUIRE_EQUAL( git_index_write_tree( &tree_oid, index ), GIT_OK );

    git_tree* tree = nullptr;
    BOOST_REQUIRE_EQUAL( git_tree_lookup( &tree, repo, &tree_oid ), GIT_OK );
    KIGIT::GitTreePtr treePtr( tree );

    git_signature* sig = nullptr;
    BOOST_REQUIRE_EQUAL( git_signature_now( &sig, "QA Test", "qa@example.com" ), GIT_OK );
    KIGIT::GitSignaturePtr sigPtr( sig );

    git_oid commit_oid;
    BOOST_REQUIRE_EQUAL( git_commit_create( &commit_oid, repo, "HEAD", sig, sig, nullptr,
                                            "initial", tree, 0, nullptr ),
                         GIT_OK );

    return repo;
}


// Stage aFileName from the working tree and commit it on top of HEAD.
void commitWorkingFile( git_repository* aRepo, const wxString& aFileName, const char* aMessage )
{
    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, aRepo ), GIT_OK );
    KIGIT::GitIndexPtr indexPtr( index );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, aFileName.utf8_string().c_str() ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );

    git_oid treeOid;
    BOOST_REQUIRE_EQUAL( git_index_write_tree( &treeOid, index ), GIT_OK );

    git_tree* tree = nullptr;
    BOOST_REQUIRE_EQUAL( git_tree_lookup( &tree, aRepo, &treeOid ), GIT_OK );
    KIGIT::GitTreePtr treePtr( tree );

    git_oid headOid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headOid, aRepo, "HEAD" ), GIT_OK );

    git_commit* parent = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &parent, aRepo, &headOid ), GIT_OK );
    KIGIT::GitCommitPtr parentPtr( parent );

    git_signature* sig = nullptr;
    BOOST_REQUIRE_EQUAL( git_signature_now( &sig, "QA Test", "qa@example.com" ), GIT_OK );
    KIGIT::GitSignaturePtr sigPtr( sig );

    const git_commit* parents[1] = { parent };
    git_oid           commitOid;
    BOOST_REQUIRE_EQUAL( git_commit_create( &commitOid, aRepo, "HEAD", sig, sig, nullptr, aMessage, tree, 1, parents ),
                         GIT_OK );
}


std::string readWorkingFile( const wxFileName& aFile )
{
    std::ifstream in( aFile.GetFullPath().utf8_string() );
    return std::string( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
}


void writeFile( const wxString& aRepoPath, const wxString& aFileName, const char* aContent )
{
    std::ofstream f( ( aRepoPath + wxS( "/" ) + aFileName ).utf8_string() );
    f << aContent;
}

} // namespace


BOOST_AUTO_TEST_SUITE( KiGitCommon )


// git_repository_path() returns a path ending in /.git/, which broke the popup-menu test for
// whether a project lives at the repository root.
BOOST_AUTO_TEST_CASE( GitRootDirectoryReturnsWorkdir )
{
    GitInitGuard libgit;
    ScopedTempDir tmp;

    git_repository* repo = makeRepoWithCommit( tmp.path, "file.txt", "data\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    wxString root = common.GetGitRootDirectory();

    BOOST_CHECK( !root.IsEmpty() );
    BOOST_CHECK_MESSAGE( !root.Contains( wxS( "/.git" ) ) && !root.Contains( wxS( "\\.git" ) ),
                         "GetGitRootDirectory should return the working directory, got: "
                                 + root.ToStdString() );

    // Should be the repo path (with trailing separator).
    wxFileName rootFn;
    rootFn.AssignDir( root );
    rootFn.MakeAbsolute();
    wxFileName tmpFn;
    tmpFn.AssignDir( tmp.path );
    tmpFn.MakeAbsolute();
    BOOST_CHECK_EQUAL( rootFn.GetFullPath().ToStdString(), tmpFn.GetFullPath().ToStdString() );
}


// The remote need not be called origin.
BOOST_AUTO_TEST_CASE( HasPushAndPullRemoteAcceptsNonOriginName )
{
    GitInitGuard libgit;
    ScopedTempDir tmp;

    git_repository* repo = makeRepoWithCommit( tmp.path, "file.txt", "data\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    // No remotes configured.
    KIGIT_COMMON common( repo );
    BOOST_CHECK( !common.HasPushAndPullRemote() );

    // Add a remote with a non-default name.
    git_remote* remote = nullptr;
    BOOST_REQUIRE_EQUAL( git_remote_create( &remote, repo, "github",
                                            "git@github.com:example/repo.git" ),
                         GIT_OK );
    KIGIT::GitRemotePtr remotePtr( remote );

    BOOST_CHECK( common.HasPushAndPullRemote() );
}


BOOST_AUTO_TEST_CASE( HasPushAndPullRemoteFindsOrigin )
{
    GitInitGuard libgit;
    ScopedTempDir tmp;

    git_repository* repo = makeRepoWithCommit( tmp.path, "file.txt", "data\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    git_remote* remote = nullptr;
    BOOST_REQUIRE_EQUAL( git_remote_create( &remote, repo, "origin",
                                            "git@example.com:repo.git" ),
                         GIT_OK );
    KIGIT::GitRemotePtr remotePtr( remote );

    KIGIT_COMMON common( repo );
    BOOST_CHECK( common.HasPushAndPullRemote() );
}


// Walking unbounded history with no upstream OID dumped the whole root-commit tree into the
// modified set, falsely flagging every file as ahead of the remote.
BOOST_AUTO_TEST_CASE( GetDifferentFilesEmptyWithoutUpstream )
{
    GitInitGuard libgit;
    ScopedTempDir tmp;

    git_repository* repo = makeRepoWithCommit( tmp.path, "file.txt", "data\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    auto [local, remote] = common.GetDifferentFiles();

    BOOST_CHECK_MESSAGE( local.empty(),
                         "Expected no AHEAD files when no upstream is configured, got "
                                 + std::to_string( local.size() ) );
    BOOST_CHECK_MESSAGE( remote.empty(),
                         "Expected no BEHIND files when no upstream is configured, got "
                                 + std::to_string( remote.size() ) );
}


// With disjoint histories the walk ran past the upstream to the local root commit, and a root
// commit has no parent, so its whole tree landed in the AHEAD set.
BOOST_AUTO_TEST_CASE( GetDifferentFilesHandlesUnrelatedHistories )
{
    GitInitGuard libgit;
    ScopedTempDir tmp;

    // Local root commit with two files.  The bug previously listed both as AHEAD.
    git_repository* repo = makeRepoWithCommit( tmp.path, "untouched.txt", "stable\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    // Add a second file to local HEAD so we have more than the bare initial tree.
    {
        wxFileName extra( tmp.path, wxS( "second.txt" ) );
        std::ofstream f( extra.GetFullPath().utf8_string() );
        f << "more\n";
    }

    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    KIGIT::GitIndexPtr indexPtr( index );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, "second.txt" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );

    git_oid newTreeOid;
    BOOST_REQUIRE_EQUAL( git_index_write_tree( &newTreeOid, index ), GIT_OK );

    git_tree* newTree = nullptr;
    BOOST_REQUIRE_EQUAL( git_tree_lookup( &newTree, repo, &newTreeOid ), GIT_OK );
    KIGIT::GitTreePtr newTreePtr( newTree );

    git_reference* head = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_head( &head, repo ), GIT_OK );
    KIGIT::GitReferencePtr headPtr( head );

    git_commit* parentCommit = nullptr;
    BOOST_REQUIRE_EQUAL(
            git_commit_lookup( &parentCommit, repo, git_reference_target( head ) ), GIT_OK );
    KIGIT::GitCommitPtr parentCommitPtr( parentCommit );

    git_signature* sig = nullptr;
    BOOST_REQUIRE_EQUAL( git_signature_now( &sig, "QA Test", "qa@example.com" ), GIT_OK );
    KIGIT::GitSignaturePtr sigPtr( sig );

    const git_commit* parents[1] = { parentCommit };
    git_oid           secondCommitOid;
    BOOST_REQUIRE_EQUAL( git_commit_create( &secondCommitOid, repo, "HEAD", sig, sig, nullptr,
                                            "second", newTree, 1, parents ),
                         GIT_OK );

    // Build a disjoint upstream history with its own root commit and a different file.
    git_treebuilder* tb = nullptr;
    BOOST_REQUIRE_EQUAL( git_treebuilder_new( &tb, repo, nullptr ), GIT_OK );

    git_oid blobOid;
    const std::string upstreamData = "upstream\n";
    BOOST_REQUIRE_EQUAL( git_blob_create_from_buffer( &blobOid, repo, upstreamData.data(),
                                                      upstreamData.size() ),
                         GIT_OK );
    BOOST_REQUIRE_EQUAL( git_treebuilder_insert( nullptr, tb, "remote_only.txt", &blobOid,
                                                 GIT_FILEMODE_BLOB ),
                         GIT_OK );

    git_oid remoteTreeOid;
    BOOST_REQUIRE_EQUAL( git_treebuilder_write( &remoteTreeOid, tb ), GIT_OK );
    git_treebuilder_free( tb );

    git_tree* remoteTree = nullptr;
    BOOST_REQUIRE_EQUAL( git_tree_lookup( &remoteTree, repo, &remoteTreeOid ), GIT_OK );
    KIGIT::GitTreePtr remoteTreePtr( remoteTree );

    git_oid remoteCommitOid;
    BOOST_REQUIRE_EQUAL( git_commit_create( &remoteCommitOid, repo, nullptr, sig, sig, nullptr,
                                            "upstream root", remoteTree, 0, nullptr ),
                         GIT_OK );

    git_reference* remoteRef = nullptr;
    BOOST_REQUIRE_EQUAL( git_reference_create( &remoteRef, repo, "refs/remotes/origin/main",
                                               &remoteCommitOid, true, nullptr ),
                         GIT_OK );
    KIGIT::GitReferencePtr remoteRefPtr( remoteRef );

    git_config* cfg = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_config( &cfg, repo ), GIT_OK );
    KIGIT::GitConfigPtr cfgPtr( cfg );
    BOOST_REQUIRE_EQUAL( git_config_set_string( cfg, "branch.main.remote", "origin" ),
                         GIT_OK );
    BOOST_REQUIRE_EQUAL( git_config_set_string( cfg, "branch.main.merge", "refs/heads/main" ),
                         GIT_OK );

    KIGIT_COMMON common( repo );
    auto [local, remote] = common.GetDifferentFiles();

    // With unrelated histories there is no merge base, so the implementation cannot
    // attribute changes to either side.  Pre-fix, the revwalk reached both root commits
    // and dumped both trees in full; the new merge-base approach short-circuits and
    // reports nothing on either side in this ambiguous state.
    BOOST_CHECK_MESSAGE( local.empty(), "no files should be reported as AHEAD when the local and remote "
                                        "histories share no ancestor" );
    BOOST_CHECK_MESSAGE( remote.empty(), "no files should be reported as BEHIND when the local and remote "
                                         "histories share no ancestor" );
}


// Files present unchanged in both trees must not show up as ahead.
BOOST_AUTO_TEST_CASE( GetDifferentFilesReportsOnlyTouchedFilesWhenAhead )
{
    GitInitGuard libgit;
    ScopedTempDir tmp;

    // Initial commit becomes the shared base.  Two files: only one will be modified later.
    git_repository* repo = makeRepoWithCommit( tmp.path, "untouched.txt", "stable\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    {
        wxFileName extra( tmp.path, wxS( "touched.txt" ) );
        std::ofstream f( extra.GetFullPath().utf8_string() );
        f << "v1\n";
    }

    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    KIGIT::GitIndexPtr indexPtr( index );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, "touched.txt" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );

    git_oid           treeOid;
    BOOST_REQUIRE_EQUAL( git_index_write_tree( &treeOid, index ), GIT_OK );

    git_tree* tree = nullptr;
    BOOST_REQUIRE_EQUAL( git_tree_lookup( &tree, repo, &treeOid ), GIT_OK );
    KIGIT::GitTreePtr treePtr( tree );

    git_reference* head = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_head( &head, repo ), GIT_OK );
    KIGIT::GitReferencePtr headPtr( head );

    git_commit* parent = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &parent, repo, git_reference_target( head ) ),
                         GIT_OK );
    KIGIT::GitCommitPtr parentPtr( parent );

    git_signature* sig = nullptr;
    BOOST_REQUIRE_EQUAL( git_signature_now( &sig, "QA Test", "qa@example.com" ), GIT_OK );
    KIGIT::GitSignaturePtr sigPtr( sig );

    const git_commit* parents[1] = { parent };
    git_oid           base_oid;
    BOOST_REQUIRE_EQUAL( git_commit_create( &base_oid, repo, "HEAD", sig, sig, nullptr,
                                            "add touched", tree, 1, parents ),
                         GIT_OK );

    // Mark this commit as the upstream tip and configure a remote so libgit2 accepts the
    // branch.main.remote = origin pointer below.
    git_remote* origin = nullptr;
    BOOST_REQUIRE_EQUAL( git_remote_create( &origin, repo, "origin",
                                            "git@example.com:repo.git" ),
                         GIT_OK );
    KIGIT::GitRemotePtr originPtr( origin );

    git_reference* upstream_ref = nullptr;
    BOOST_REQUIRE_EQUAL( git_reference_create( &upstream_ref, repo, "refs/remotes/origin/main",
                                               &base_oid, true, nullptr ),
                         GIT_OK );
    KIGIT::GitReferencePtr upstreamRefPtr( upstream_ref );

    git_config* cfg = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_config( &cfg, repo ), GIT_OK );
    KIGIT::GitConfigPtr cfgPtr( cfg );
    BOOST_REQUIRE_EQUAL( git_config_set_string( cfg, "branch.main.remote", "origin" ),
                         GIT_OK );
    BOOST_REQUIRE_EQUAL( git_config_set_string( cfg, "branch.main.merge", "refs/heads/main" ),
                         GIT_OK );

    // Local-only commit modifying just touched.txt.
    {
        wxFileName extra( tmp.path, wxS( "touched.txt" ) );
        std::ofstream f( extra.GetFullPath().utf8_string() );
        f << "v2\n";
    }

    BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, "touched.txt" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );

    git_oid newTreeOid;
    BOOST_REQUIRE_EQUAL( git_index_write_tree( &newTreeOid, index ), GIT_OK );

    git_tree* newTree = nullptr;
    BOOST_REQUIRE_EQUAL( git_tree_lookup( &newTree, repo, &newTreeOid ), GIT_OK );
    KIGIT::GitTreePtr newTreePtr( newTree );

    git_commit* baseCommit = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &baseCommit, repo, &base_oid ), GIT_OK );
    KIGIT::GitCommitPtr baseCommitPtr( baseCommit );

    const git_commit* aheadParents[1] = { baseCommit };
    git_oid           aheadOid;
    BOOST_REQUIRE_EQUAL( git_commit_create( &aheadOid, repo, "HEAD", sig, sig, nullptr,
                                            "modify touched", newTree, 1, aheadParents ),
                         GIT_OK );

    KIGIT_COMMON common( repo );
    auto [local, remote] = common.GetDifferentFiles();

    BOOST_CHECK( remote.empty() );
    BOOST_CHECK_MESSAGE( local.find( wxS( "touched.txt" ) ) != local.end(),
                         "touched.txt should be reported as AHEAD" );
    BOOST_CHECK_MESSAGE( local.find( wxS( "untouched.txt" ) ) == local.end(),
                         "untouched.txt should NOT be reported as AHEAD - this was the "
                         "regression in issue 21576" );
}


BOOST_AUTO_TEST_CASE( PrivateKeyEncryptionOpenSSHFormat )
{
    BOOST_CHECK_MESSAGE( KIGIT::IsPrivateKeyEncrypted( c_opensshEncrypted ),
                         "A passphrase-protected OpenSSH key must be reported as encrypted" );

    BOOST_CHECK_MESSAGE( !KIGIT::IsPrivateKeyEncrypted( c_opensshPlain ),
                         "An OpenSSH key with cipher \"none\" must be reported as unencrypted" );

    BOOST_CHECK_MESSAGE( !KIGIT::IsPrivateKeyEncrypted( c_opensshTruncated ),
                         "An OpenSSH key with an unreadable body must not be reported as encrypted" );
}


BOOST_AUTO_TEST_CASE( PrivateKeyEncryptionClassicPEMFormat )
{
    BOOST_CHECK_MESSAGE( KIGIT::IsPrivateKeyEncrypted( c_pemEncrypted ),
                         "A PEM key with Proc-Type: 4,ENCRYPTED must be reported as encrypted" );

    BOOST_CHECK_MESSAGE( !KIGIT::IsPrivateKeyEncrypted( c_pemPlain ),
                         "A PEM key with no Proc-Type must be reported as unencrypted" );
}


BOOST_AUTO_TEST_CASE( PrivateKeyEncryptionPKCS8Format )
{
    BOOST_CHECK_MESSAGE( KIGIT::IsPrivateKeyEncrypted( c_pkcs8Encrypted ),
                         "A PKCS#8 key headed ENCRYPTED PRIVATE KEY must be reported as encrypted" );
}


BOOST_AUTO_TEST_CASE( PrivateKeyEncryptionRejectsNonKeys )
{
    BOOST_CHECK( !KIGIT::IsPrivateKeyEncrypted( wxEmptyString ) );

    BOOST_CHECK_MESSAGE( !KIGIT::IsPrivateKeyEncrypted( "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIKB0 t\n" ),
                         "A public key is not an encrypted private key" );
}


// A file changed only on the remote tip, with the local branch still at the shared base, must
// land in the BEHIND (remote) set and not in the AHEAD (local) set.
BOOST_AUTO_TEST_CASE( GetDifferentFilesReportsRemoteOnlyFilesWhenBehind )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "untouched.txt", "stable\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    git_reference* head = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_head( &head, repo ), GIT_OK );
    KIGIT::GitReferencePtr headPtr( head );

    git_oid baseOid = *git_reference_target( head );

    git_commit* baseCommit = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &baseCommit, repo, &baseOid ), GIT_OK );
    KIGIT::GitCommitPtr baseCommitPtr( baseCommit );

    // Build a tree that adds remote_only.txt on top of the base, then commit it without moving
    // HEAD so the local branch stays behind.
    {
        wxFileName    extra( tmp.path, wxS( "remote_only.txt" ) );
        std::ofstream f( extra.GetFullPath().utf8_string() );
        f << "from remote\n";
    }

    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    KIGIT::GitIndexPtr indexPtr( index );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, "remote_only.txt" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );

    git_oid treeOid;
    BOOST_REQUIRE_EQUAL( git_index_write_tree( &treeOid, index ), GIT_OK );

    git_tree* tree = nullptr;
    BOOST_REQUIRE_EQUAL( git_tree_lookup( &tree, repo, &treeOid ), GIT_OK );
    KIGIT::GitTreePtr treePtr( tree );

    git_signature* sig = nullptr;
    BOOST_REQUIRE_EQUAL( git_signature_now( &sig, "QA Test", "qa@example.com" ), GIT_OK );
    KIGIT::GitSignaturePtr sigPtr( sig );

    const git_commit* parents[1] = { baseCommit };
    git_oid           remoteOid;
    BOOST_REQUIRE_EQUAL(
            git_commit_create( &remoteOid, repo, nullptr, sig, sig, nullptr, "add remote_only", tree, 1, parents ),
            GIT_OK );

    git_remote* origin = nullptr;
    BOOST_REQUIRE_EQUAL( git_remote_create( &origin, repo, "origin", "git@example.com:repo.git" ), GIT_OK );
    KIGIT::GitRemotePtr originPtr( origin );

    git_reference* upstream_ref = nullptr;
    BOOST_REQUIRE_EQUAL(
            git_reference_create( &upstream_ref, repo, "refs/remotes/origin/main", &remoteOid, true, nullptr ),
            GIT_OK );
    KIGIT::GitReferencePtr upstreamRefPtr( upstream_ref );

    git_config* cfg = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_config( &cfg, repo ), GIT_OK );
    KIGIT::GitConfigPtr cfgPtr( cfg );
    BOOST_REQUIRE_EQUAL( git_config_set_string( cfg, "branch.main.remote", "origin" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_config_set_string( cfg, "branch.main.merge", "refs/heads/main" ), GIT_OK );

    KIGIT_COMMON common( repo );
    auto [local, remote] = common.GetDifferentFiles();

    BOOST_CHECK( local.empty() );
    BOOST_CHECK_MESSAGE( remote.find( wxS( "remote_only.txt" ) ) != remote.end(),
                         "remote_only.txt should be reported as BEHIND" );
}


// The commit panels and the advanced window register refresh listeners so a write from any
// one of them updates the others. A notify must fire every registered listener.
BOOST_AUTO_TEST_CASE( RefreshListenersFireOnNotify )
{
    KIGIT_COMMON common( nullptr );

    int a = 0;
    int b = 0;
    int ownerA = 0;
    int ownerB = 0;

    common.AddRefreshListener( &ownerA,
                               [&]()
                               {
                                   a++;
                               } );
    common.AddRefreshListener( &ownerB,
                               [&]()
                               {
                                   b++;
                               } );

    common.NotifyRefreshListeners();
    BOOST_CHECK_EQUAL( a, 1 );
    BOOST_CHECK_EQUAL( b, 1 );

    common.NotifyRefreshListeners();
    BOOST_CHECK_EQUAL( a, 2 );
    BOOST_CHECK_EQUAL( b, 2 );
}


// A removed listener, such as one whose panel was destroyed, must never fire again.
BOOST_AUTO_TEST_CASE( RemovedRefreshListenerDoesNotFire )
{
    KIGIT_COMMON common( nullptr );

    int a = 0;
    int b = 0;
    int ownerA = 0;
    int ownerB = 0;

    common.AddRefreshListener( &ownerA,
                               [&]()
                               {
                                   a++;
                               } );
    common.AddRefreshListener( &ownerB,
                               [&]()
                               {
                                   b++;
                               } );
    common.RemoveRefreshListener( &ownerA );

    common.NotifyRefreshListeners();
    BOOST_CHECK_EQUAL( a, 0 );
    BOOST_CHECK_EQUAL( b, 1 );
}


// A panel torn down while responding to a refresh unregisters from inside its own callback, and
// the remaining listeners must still run.
BOOST_AUTO_TEST_CASE( RefreshListenerCanUnregisterDuringNotify )
{
    KIGIT_COMMON common( nullptr );

    int a = 0;
    int b = 0;
    int ownerA = 0;
    int ownerB = 0;

    common.AddRefreshListener( &ownerA,
                               [&]()
                               {
                                   a++;
                                   common.RemoveRefreshListener( &ownerA );
                               } );
    common.AddRefreshListener( &ownerB,
                               [&]()
                               {
                                   b++;
                               } );

    common.NotifyRefreshListeners();
    BOOST_CHECK_EQUAL( a, 1 );
    BOOST_CHECK_EQUAL( b, 1 );

    common.NotifyRefreshListeners();
    BOOST_CHECK_EQUAL( a, 1 );
    BOOST_CHECK_EQUAL( b, 2 );
}


// A listener removed by an earlier listener in the same round must not run.
BOOST_AUTO_TEST_CASE( RefreshListenerRemovedByAnotherDoesNotFire )
{
    KIGIT_COMMON common( nullptr );

    int a = 0;
    int b = 0;
    int ownerA = 0;
    int ownerB = 0;

    common.AddRefreshListener( &ownerA,
                               [&]()
                               {
                                   a++;
                                   common.RemoveRefreshListener( &ownerB );
                               } );
    common.AddRefreshListener( &ownerB,
                               [&]()
                               {
                                   b++;
                               } );

    common.NotifyRefreshListeners();
    BOOST_CHECK_EQUAL( a, 1 );
    BOOST_CHECK_EQUAL( b, 0 );
}


// The History page reads the log through GetRecentCommits. With no repository it must return
// an empty list rather than crash.
BOOST_AUTO_TEST_CASE( GetRecentCommitsEmptyWithoutRepo )
{
    KIGIT_COMMON common( nullptr );
    BOOST_CHECK( common.GetRecentCommits( 10 ).empty() );
}


// GetRecentCommits returns commits newest-first with their summary, author and short hash,
// and honors the requested maximum.
BOOST_AUTO_TEST_CASE( GetRecentCommitsReturnsNewestFirst )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    // Add a second commit on top of the first.
    {
        wxFileName    fn( tmp.path, wxS( "file.txt" ) );
        std::ofstream f( fn.GetFullPath().utf8_string() );
        f << "v2\n";
    }

    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    KIGIT::GitIndexPtr indexPtr( index );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, "file.txt" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );

    git_oid treeOid;
    BOOST_REQUIRE_EQUAL( git_index_write_tree( &treeOid, index ), GIT_OK );

    git_tree* tree = nullptr;
    BOOST_REQUIRE_EQUAL( git_tree_lookup( &tree, repo, &treeOid ), GIT_OK );
    KIGIT::GitTreePtr treePtr( tree );

    git_reference* head = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_head( &head, repo ), GIT_OK );
    KIGIT::GitReferencePtr headPtr( head );

    git_commit* parent = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &parent, repo, git_reference_target( head ) ), GIT_OK );
    KIGIT::GitCommitPtr parentPtr( parent );

    git_signature* sig = nullptr;
    BOOST_REQUIRE_EQUAL( git_signature_now( &sig, "QA Test", "qa@example.com" ), GIT_OK );
    KIGIT::GitSignaturePtr sigPtr( sig );

    const git_commit* parents[1] = { parent };
    git_oid           secondOid;
    BOOST_REQUIRE_EQUAL( git_commit_create( &secondOid, repo, "HEAD", sig, sig, nullptr, "second", tree, 1, parents ),
                         GIT_OK );

    KIGIT_COMMON common( repo );

    std::vector<KIGIT_COMMIT_INFO> commits = common.GetRecentCommits( 10 );
    BOOST_REQUIRE_EQUAL( commits.size(), 2u );
    BOOST_CHECK_EQUAL( commits[0].summary.ToStdString(), "second" );
    BOOST_CHECK_EQUAL( commits[1].summary.ToStdString(), "initial" );
    BOOST_CHECK_EQUAL( commits[0].author.ToStdString(), "QA Test" );

    // The hash is abbreviated to 12 characters. A 7-char abbreviation was ambiguous in large
    // repositories, which made a later revparse of the hash fail.
    BOOST_CHECK_EQUAL( commits[0].hash.length(), 12u );

    // The maximum is honored.
    BOOST_CHECK_EQUAL( common.GetRecentCommits( 1 ).size(), 1u );
    BOOST_CHECK_EQUAL( common.GetRecentCommits( 0 ).size(), 0u );
}


// The History page can narrow the log to one branch or to a search text. With no branch named
// the walk covers every branch, so a commit on a side branch shows up too.
BOOST_AUTO_TEST_CASE( GetRecentCommitsFiltersByBranchAndSearch )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    // A side branch with one commit of its own, then back on main for one more.
    git_oid headOid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headOid, repo, "HEAD" ), GIT_OK );

    git_commit* headCommit = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &headCommit, repo, &headOid ), GIT_OK );
    KIGIT::GitCommitPtr headPtr( headCommit );

    git_reference* side = nullptr;
    BOOST_REQUIRE_EQUAL( git_branch_create( &side, repo, "side", headCommit, 0 ), GIT_OK );
    KIGIT::GitReferencePtr sidePtr( side );

    BOOST_REQUIRE_EQUAL( git_repository_set_head( repo, "refs/heads/side" ), GIT_OK );
    writeFile( tmp.path, "side.txt", "side\n" );
    commitWorkingFile( repo, "side.txt", "side work" );

    BOOST_REQUIRE_EQUAL( git_repository_set_head( repo, "refs/heads/main" ), GIT_OK );
    writeFile( tmp.path, "file.txt", "v2\n" );
    commitWorkingFile( repo, "file.txt", "main work" );

    KIGIT_COMMON common( repo );

    auto summaries = []( const std::vector<KIGIT_COMMIT_INFO>& aCommits )
    {
        std::vector<std::string> out;

        for( const KIGIT_COMMIT_INFO& c : aCommits )
            out.push_back( c.summary.ToStdString() );

        return out;
    };

    // Every branch: the side commit is there along with main's.
    std::vector<std::string> all = summaries( common.GetRecentCommits( 10 ) );
    BOOST_REQUIRE_EQUAL( all.size(), 3u );
    BOOST_CHECK( std::find( all.begin(), all.end(), "side work" ) != all.end() );
    BOOST_CHECK( std::find( all.begin(), all.end(), "main work" ) != all.end() );

    // One branch: only what it reaches.
    KIGIT_HISTORY_FILTER onMain;
    onMain.branch = wxS( "main" );
    std::vector<std::string> mainOnly = summaries( common.GetRecentCommits( 10, onMain ) );
    BOOST_REQUIRE_EQUAL( mainOnly.size(), 2u );
    BOOST_CHECK_EQUAL( mainOnly[0], "main work" );
    BOOST_CHECK_EQUAL( mainOnly[1], "initial" );

    // Search matches the summary, case insensitively, and the hash.
    KIGIT_HISTORY_FILTER search;
    search.search = wxS( "SIDE" );
    std::vector<KIGIT_COMMIT_INFO> found = common.GetRecentCommits( 10, search );
    BOOST_REQUIRE_EQUAL( found.size(), 1u );
    BOOST_CHECK_EQUAL( found[0].summary.ToStdString(), "side work" );

    search.search = found[0].hash.Left( 6 );
    BOOST_REQUIRE_EQUAL( common.GetRecentCommits( 10, search ).size(), 1u );

    search.search = wxS( "nothing like this" );
    BOOST_CHECK( common.GetRecentCommits( 10, search ).empty() );
}


// Each commit carries the names of the refs that point at it, so the list can show where HEAD,
// the branches and the tags are.
BOOST_AUTO_TEST_CASE( GetRecentCommitsNamesRefsOnEachCommit )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    git_oid firstOid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &firstOid, repo, "HEAD" ), GIT_OK );

    git_object* first = nullptr;
    BOOST_REQUIRE_EQUAL( git_object_lookup( &first, repo, &firstOid, GIT_OBJECT_COMMIT ), GIT_OK );
    KIGIT::GitObjectPtr firstPtr( first );

    git_oid tagOid;
    BOOST_REQUIRE_EQUAL( git_tag_create_lightweight( &tagOid, repo, "v1.0", first, 0 ), GIT_OK );

    writeFile( tmp.path, "file.txt", "v2\n" );
    commitWorkingFile( repo, "file.txt", "second" );

    KIGIT_COMMON common( repo );

    std::vector<KIGIT_COMMIT_INFO> commits = common.GetRecentCommits( 10 );
    BOOST_REQUIRE_EQUAL( commits.size(), 2u );

    // The newest commit is HEAD and main. The first one carries only the tag.
    BOOST_REQUIRE_EQUAL( commits[0].refs.size(), 2u );
    BOOST_CHECK_EQUAL( commits[0].refs[0].ToStdString(), "HEAD" );
    BOOST_CHECK_EQUAL( commits[0].refs[1].ToStdString(), "main" );

    BOOST_REQUIRE_EQUAL( commits[1].refs.size(), 1u );
    BOOST_CHECK_EQUAL( commits[1].refs[0].ToStdString(), "v1.0" );
}


// The detail pane shows one commit with its message, parent and per-file line counts.
// A project just put under version control has no commits. There is nothing to branch from, so
// naming a branch means pointing HEAD at it and letting the first commit create it.
BOOST_AUTO_TEST_CASE( StartUnbornBranchNamesTheBranchTheFirstCommitWillCreate )
{
    SCOPED_GIT_BACKEND libgit;
    ScopedTempDir      tmp;

    git_repository*             repo = nullptr;
    git_repository_init_options opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
    opts.flags = GIT_REPOSITORY_INIT_MKPATH;
    opts.initial_head = "main";

    BOOST_REQUIRE_EQUAL( git_repository_init_ext( &repo, tmp.path.utf8_string().c_str(), &opts ), GIT_OK );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    BOOST_REQUIRE( common.IsHeadUnborn() );

    // There is no commit to start a branch at, which is why the method below exists.
    BOOST_CHECK( !common.CreateBranch( wxS( "rev-a" ) ) );

    // A name git itself would refuse must be refused here too, or the first commit creates a
    // branch no git tool can name.
    BOOST_CHECK( !common.StartUnbornBranch( wxS( "-leading" ) ) );
    BOOST_CHECK( !common.StartUnbornBranch( wxS( "has space" ) ) );
    BOOST_CHECK( !common.StartUnbornBranch( wxS( "bad..name" ) ) );

    BOOST_REQUIRE( common.StartUnbornBranch( wxS( "rev-a" ) ) );

    git_reference* head = nullptr;
    BOOST_REQUIRE_EQUAL( git_reference_lookup( &head, repo, "HEAD" ), GIT_OK );
    KIGIT::GitReferencePtr headPtr( head );

    BOOST_CHECK_EQUAL( std::string( git_reference_symbolic_target( head ) ), "refs/heads/rev-a" );
    BOOST_CHECK( common.IsHeadUnborn() );
}


// The History page shows a commit's whole message in its detail pane, summary line and body.
BOOST_AUTO_TEST_CASE( GetCommitDetailCarriesTheWholeMessage )
{
    SCOPED_GIT_BACKEND libgit;
    ScopedTempDir      tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    writeFile( tmp.path, "file.txt", "v2\n" );
    commitWorkingFile( repo, "file.txt", "Route the power rails\n\nBody line one.\nBody line two.\n" );

    KIGIT_COMMON        common( repo );
    KIGIT_COMMIT_DETAIL detail = common.GetCommitDetail( common.GetRecentCommits( 1 )[0].hash );

    BOOST_REQUIRE( detail.found );
    BOOST_CHECK_EQUAL( detail.message.ToStdString(), "Route the power rails\n\nBody line one.\nBody line two." );
}


BOOST_AUTO_TEST_CASE( GetCommitDetailCountsLinesPerFile )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "one\ntwo\nthree\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    // Change one line of the existing file and add a new file in the same commit.
    writeFile( tmp.path, "file.txt", "one\n2\nthree\n" );
    writeFile( tmp.path, "new.txt", "a\nb\n" );

    {
        git_index* index = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
        KIGIT::GitIndexPtr indexPtr( index );
        BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, "new.txt" ), GIT_OK );
        BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );
    }

    commitWorkingFile( repo, "file.txt", "edit and add\n\nWith a body line." );

    KIGIT_COMMON common( repo );

    std::vector<KIGIT_COMMIT_INFO> commits = common.GetRecentCommits( 10 );
    BOOST_REQUIRE_EQUAL( commits.size(), 2u );

    KIGIT_COMMIT_DETAIL detail = common.GetCommitDetail( commits[0].hash );
    BOOST_REQUIRE( detail.found );
    BOOST_CHECK_EQUAL( detail.shortHash.ToStdString(), commits[0].hash.ToStdString() );
    BOOST_CHECK_EQUAL( detail.hash.length(), 40u );
    BOOST_CHECK_EQUAL( detail.message.ToStdString(), "edit and add\n\nWith a body line." );
    BOOST_CHECK_EQUAL( detail.author.ToStdString(), "QA Test" );
    BOOST_REQUIRE_EQUAL( detail.parents.size(), 1u );
    BOOST_CHECK_EQUAL( detail.parents[0].ToStdString(), commits[1].hash.ToStdString() );
    BOOST_CHECK_EQUAL( detail.refs.size(), 2u );

    BOOST_REQUIRE_EQUAL( detail.files.size(), 2u );
    BOOST_CHECK_EQUAL( detail.files[0].path.ToStdString(), "file.txt" );
    BOOST_CHECK( detail.files[0].status == KIGIT::FILE_CHANGE_STATUS::MODIFIED );
    BOOST_CHECK_EQUAL( detail.files[0].added, 1 );
    BOOST_CHECK_EQUAL( detail.files[0].removed, 1 );
    BOOST_CHECK_EQUAL( detail.files[1].path.ToStdString(), "new.txt" );
    BOOST_CHECK( detail.files[1].status == KIGIT::FILE_CHANGE_STATUS::ADDED );
    BOOST_CHECK_EQUAL( detail.files[1].added, 2 );
    BOOST_CHECK_EQUAL( detail.files[1].removed, 0 );
    BOOST_CHECK_EQUAL( detail.added, 3 );
    BOOST_CHECK_EQUAL( detail.removed, 1 );

    // A root commit has no parent, so everything it holds counts as added.
    KIGIT_COMMIT_DETAIL root = common.GetCommitDetail( commits[1].hash );
    BOOST_REQUIRE( root.found );
    BOOST_CHECK( root.parents.empty() );
    BOOST_REQUIRE_EQUAL( root.files.size(), 1u );
    BOOST_CHECK( root.files[0].status == KIGIT::FILE_CHANGE_STATUS::ADDED );
    BOOST_CHECK_EQUAL( root.files[0].added, 3 );

    BOOST_CHECK( !common.GetCommitDetail( wxS( "0000000000ab" ) ).found );
}


// The changes list leaves out other projects in the repository, and KiCad's own lock and
// backup files. The line counts sit beside that list, so they must cover the same files.
BOOST_AUTO_TEST_CASE( WorkingTreeLineStatsCountOnlyTheFilesGiven )
{
    SCOPED_GIT_BACKEND libgit;
    ScopedTempDir      tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "readme.txt", "root\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    BOOST_REQUIRE( wxFileName::Mkdir( tmp.path + wxS( "/mine" ) ) );
    BOOST_REQUIRE( wxFileName::Mkdir( tmp.path + wxS( "/theirs" ) ) );
    writeFile( tmp.path, wxS( "mine/board.txt" ), "one\ntwo\n" );
    writeFile( tmp.path, wxS( "theirs/board.txt" ), "one\ntwo\n" );

    {
        git_index* index = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
        KIGIT::GitIndexPtr indexPtr( index );
        BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, "mine/board.txt" ), GIT_OK );
        BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, "theirs/board.txt" ), GIT_OK );
        BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );
    }

    commitWorkingFile( repo, wxS( "readme.txt" ), "two projects\n" );

    // One line added in this project, three in the other, and an untracked lock file.
    writeFile( tmp.path, wxS( "mine/board.txt" ), "one\ntwo\nthree\n" );
    writeFile( tmp.path, wxS( "theirs/board.txt" ), "one\ntwo\na\nb\nc\n" );
    writeFile( tmp.path, wxS( "mine/~board.lck" ), "locked\n" );

    KIGIT_COMMON common( repo );

    std::pair<int, int> mine = common.GetWorkingTreeLineStats( { wxS( "mine/board.txt" ) } );
    BOOST_CHECK_EQUAL( mine.first, 1 );
    BOOST_CHECK_EQUAL( mine.second, 0 );

    // A path that did not change contributes nothing, and an unlisted one is not counted.
    std::pair<int, int> none = common.GetWorkingTreeLineStats( { wxS( "readme.txt" ) } );
    BOOST_CHECK_EQUAL( none.first, 0 );
    BOOST_CHECK_EQUAL( none.second, 0 );
}


// WriteBlobToFile materializes a file's committed content, used to feed the diff viewer.
BOOST_AUTO_TEST_CASE( WriteBlobToFileExtractsCommittedContent )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "hello world\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    wxString dest = tmp.path + wxFileName::GetPathSeparator() + wxS( "extracted.txt" );

    BOOST_CHECK( KIGIT::PROJECT_GIT_UTILS::WriteBlobToFile( repo, wxS( "HEAD" ), wxS( "file.txt" ), dest ) );

    wxFile   in( dest );
    wxString content;
    BOOST_REQUIRE( in.IsOpened() );
    in.ReadAll( &content );
    BOOST_CHECK_EQUAL( content.ToStdString(), "hello world\n" );

    // A path that is not in the tree returns false.
    BOOST_CHECK( !KIGIT::PROJECT_GIT_UTILS::WriteBlobToFile( repo, wxS( "HEAD" ), wxS( "missing.txt" ), dest ) );
}


// GetTags resolves each tag to the commit it points at.
BOOST_AUTO_TEST_CASE( GetTagsResolvesTagToCommit )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON emptyCommon( nullptr );
    BOOST_CHECK( emptyCommon.GetTags().empty() );

    KIGIT_COMMON common( repo );
    BOOST_CHECK( common.GetTags().empty() );

    git_object* target = nullptr;
    BOOST_REQUIRE_EQUAL( git_revparse_single( &target, repo, "HEAD" ), GIT_OK );
    KIGIT::GitObjectPtr targetPtr( target );

    git_oid tagOid;
    BOOST_REQUIRE_EQUAL( git_tag_create_lightweight( &tagOid, repo, "v1.0", target, 0 ), GIT_OK );

    std::vector<KIGIT_TAG_INFO> tags = common.GetTags();
    BOOST_REQUIRE_EQUAL( tags.size(), 1u );
    BOOST_CHECK_EQUAL( tags[0].name.ToStdString(), "v1.0" );
    BOOST_CHECK_EQUAL( tags[0].summary.ToStdString(), "initial" );
    BOOST_CHECK( !tags[0].hash.IsEmpty() );
}


// GetStashes lists the repository's stashes.
BOOST_AUTO_TEST_CASE( GetStashesListsStash )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_CHECK( common.GetStashes().empty() );

    // Dirty the working tree so there is something to stash.
    {
        wxFileName    fn( tmp.path, wxS( "file.txt" ) );
        std::ofstream f( fn.GetFullPath().utf8_string() );
        f << "v2\n";
    }

    git_signature* sig = nullptr;
    BOOST_REQUIRE_EQUAL( git_signature_now( &sig, "QA Test", "qa@example.com" ), GIT_OK );
    KIGIT::GitSignaturePtr sigPtr( sig );

    git_oid stashOid;
    BOOST_REQUIRE_EQUAL( git_stash_save( &stashOid, repo, sig, "work in progress", GIT_STASH_DEFAULT ), GIT_OK );

    std::vector<KIGIT_STASH_INFO> stashes = common.GetStashes();
    BOOST_REQUIRE_EQUAL( stashes.size(), 1u );
    BOOST_CHECK_EQUAL( stashes[0].index, 0u );
    BOOST_CHECK( stashes[0].message.Contains( "work in progress" ) );
}


// A new stash takes index 0 and pushes the rest down, so anything holding an index from an
// earlier listing would act on the wrong stash. The id stays with its own stash.
BOOST_AUTO_TEST_CASE( StashIdStaysWithItsStashWhenAnotherIsAdded )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    auto dirtyAndStash = [&]( const char* aContent, const wxString& aMessage )
    {
        wxFileName fn( tmp.path, wxS( "file.txt" ) );

        {
            std::ofstream f( fn.GetFullPath().utf8_string() );
            f << aContent;
        }

        BOOST_REQUIRE( common.StashSave( aMessage ) );
    };

    dirtyAndStash( "v2\n", wxS( "first" ) );

    std::vector<KIGIT_STASH_INFO> before = common.GetStashes();
    BOOST_REQUIRE_EQUAL( before.size(), 1u );

    wxString firstId = before[0].id;
    BOOST_REQUIRE( !firstId.IsEmpty() );

    dirtyAndStash( "v3\n", wxS( "second" ) );

    std::vector<KIGIT_STASH_INFO> after = common.GetStashes();
    BOOST_REQUIRE_EQUAL( after.size(), 2u );

    // Index 0 is now a different stash than it was.
    BOOST_CHECK( after[0].id != firstId );

    // The first stash is still there, one place further down, under the same id.
    BOOST_CHECK_EQUAL( after[1].id.ToStdString(), firstId.ToStdString() );
}


// StashSave puts the committed content back, and the stash restores the edits afterwards.
BOOST_AUTO_TEST_CASE( StashSaveRoundTrip )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    {
        wxFileName    fn( tmp.path, wxS( "file.txt" ) );
        std::ofstream f( fn.GetFullPath().utf8_string() );
        f << "v2\n";
    }

    BOOST_REQUIRE( common.StashSave( wxT( "set aside for pull" ) ) );

    wxFileName fn( tmp.path, wxS( "file.txt" ) );

    auto readAll = []( const wxString& aPath )
    {
        std::ifstream in( aPath.utf8_string(), std::ios::binary );
        return std::string( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
    };

    BOOST_CHECK_EQUAL( readAll( fn.GetFullPath() ), "v1\n" );

    std::vector<KIGIT_STASH_INFO> stashes = common.GetStashes();
    BOOST_REQUIRE_EQUAL( stashes.size(), 1u );
    BOOST_CHECK( stashes[0].message.Contains( "set aside for pull" ) );

    BOOST_CHECK( common.StashPop( 0 ) );
    BOOST_CHECK_EQUAL( readAll( fn.GetFullPath() ), "v2\n" );
    BOOST_CHECK( common.GetStashes().empty() );
}


// CreateBranch and DeleteBranch add and remove a local branch.
BOOST_AUTO_TEST_CASE( CreateAndDeleteBranch )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    BOOST_REQUIRE_EQUAL( common.GetBranchNames().size(), 1u );
    BOOST_CHECK( common.CreateBranch( "feature" ) );
    BOOST_CHECK_EQUAL( common.GetBranchNames().size(), 2u );
    BOOST_CHECK( common.DeleteBranch( "feature" ) );
    BOOST_CHECK_EQUAL( common.GetBranchNames().size(), 1u );
}


// CreateTag and DeleteTag add and remove a lightweight tag.
BOOST_AUTO_TEST_CASE( CreateAndDeleteTag )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    BOOST_CHECK( common.CreateTag( "v1.0" ) );

    std::vector<KIGIT_TAG_INFO> tags = common.GetTags();
    BOOST_REQUIRE_EQUAL( tags.size(), 1u );
    BOOST_CHECK_EQUAL( tags[0].name.ToStdString(), "v1.0" );

    BOOST_CHECK( common.DeleteTag( "v1.0" ) );
    BOOST_CHECK( common.GetTags().empty() );
}


// SetUserConfig writes user.name and user.email to the repository config.
BOOST_AUTO_TEST_CASE( SetUserConfigPersists )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    BOOST_CHECK( common.SetUserConfig( "New Name", "new@example.com" ) );

    // Read it back with libgit2 directly. GIT_CONFIG_HANDLER needs a backend the harness lacks.
    git_config* config = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_config( &config, repo ), GIT_OK );
    KIGIT::GitConfigPtr configPtr( config );

    git_config_entry* nameEntry = nullptr;
    BOOST_REQUIRE_EQUAL( git_config_get_entry( &nameEntry, config, "user.name" ), GIT_OK );
    BOOST_CHECK_EQUAL( wxString::FromUTF8( nameEntry->value ).ToStdString(), "New Name" );
    git_config_entry_free( nameEntry );

    git_config_entry* emailEntry = nullptr;
    BOOST_REQUIRE_EQUAL( git_config_get_entry( &emailEntry, config, "user.email" ), GIT_OK );
    BOOST_CHECK_EQUAL( wxString::FromUTF8( emailEntry->value ).ToStdString(), "new@example.com" );
    git_config_entry_free( emailEntry );
}


// StashDrop removes a stash from the list.
BOOST_AUTO_TEST_CASE( StashDropRemovesStash )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    {
        wxFileName    fn( tmp.path, wxS( "file.txt" ) );
        std::ofstream f( fn.GetFullPath().utf8_string() );
        f << "v2\n";
    }

    git_signature* sig = nullptr;
    BOOST_REQUIRE_EQUAL( git_signature_now( &sig, "QA Test", "qa@example.com" ), GIT_OK );
    KIGIT::GitSignaturePtr sigPtr( sig );

    git_oid stashOid;
    BOOST_REQUIRE_EQUAL( git_stash_save( &stashOid, repo, sig, "work in progress", GIT_STASH_DEFAULT ), GIT_OK );

    BOOST_REQUIRE_EQUAL( common.GetStashes().size(), 1u );
    BOOST_CHECK( common.StashDrop( 0 ) );
    BOOST_CHECK( common.GetStashes().empty() );
}


// Apply leaves the stash in the list. Pop removes it.
BOOST_AUTO_TEST_CASE( StashApplyKeepsAndStashPopRemoves )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    auto dirtyAndStash = [&]()
    {
        wxFileName    fn( tmp.path, wxS( "file.txt" ) );
        std::ofstream f( fn.GetFullPath().utf8_string() );
        f << "v2\n";
        f.close();

        git_signature* sig = nullptr;
        BOOST_REQUIRE_EQUAL( git_signature_now( &sig, "QA Test", "qa@example.com" ), GIT_OK );
        KIGIT::GitSignaturePtr sigPtr( sig );

        git_oid stashOid;
        BOOST_REQUIRE_EQUAL( git_stash_save( &stashOid, repo, sig, "wip", GIT_STASH_DEFAULT ), GIT_OK );
    };

    dirtyAndStash();
    BOOST_REQUIRE_EQUAL( common.GetStashes().size(), 1u );

    // Apply keeps the stash in the list.
    BOOST_CHECK( common.StashApply( 0 ) );
    BOOST_CHECK_EQUAL( common.GetStashes().size(), 1u );

    // Applying needs a clean tree, so put the applied change away before popping.
    wxString error;
    BOOST_REQUIRE( common.DiscardChanges( { wxS( "file.txt" ) }, error ) );

    // Pop removes it.
    BOOST_CHECK( common.StashPop( 0 ) );
    BOOST_CHECK( common.GetStashes().empty() );
}


// libgit2 reports a conflicting apply as success, so pop would drop the only copy of the work.
BOOST_AUTO_TEST_CASE( StashPopKeepsStashWhenItConflicts )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );
    KIGIT_COMMON            common( repo );

    wxFileName fn( tmp.path, wxS( "file.txt" ) );

    auto writeFile = [&fn]( const char* aContent )
    {
        std::ofstream f( fn.GetFullPath().utf8_string() );
        f << aContent;
    };

    writeFile( "stashed\n" );
    BOOST_REQUIRE( common.StashSave( wxS( "wip" ) ) );
    BOOST_REQUIRE_EQUAL( common.GetStashes().size(), 1u );

    // A different change to the same line, committed, so the stash no longer applies cleanly.
    writeFile( "committed\n" );
    commitWorkingFile( repo, wxS( "file.txt" ), "conflicting change" );

    BOOST_CHECK( !common.StashPop( 0 ) );
    BOOST_CHECK_EQUAL( common.GetStashes().size(), 1u );
    BOOST_CHECK_EQUAL( readWorkingFile( fn ), "committed\n" );

    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    KIGIT::GitIndexPtr indexPtr( index );
    BOOST_CHECK( !git_index_has_conflicts( index ) );
}


// CheckoutTag puts the working tree at the tagged commit with HEAD detached.
BOOST_AUTO_TEST_CASE( CheckoutTagDetachesHead )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_REQUIRE( common.CreateTag( "v1.0" ) );

    BOOST_CHECK( !common.IsHeadDetached() );
    BOOST_CHECK( common.CheckoutTag( "v1.0" ) );
    BOOST_CHECK_EQUAL( git_repository_head_detached( repo ), 1 );
    BOOST_CHECK( common.IsHeadDetached() );
}


// The count is what deleting the branch would lose: its commits that HEAD does not reach.
BOOST_AUTO_TEST_CASE( UnmergedCommitCountIsWhatDeletingWouldLose )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );
    KIGIT_COMMON            common( repo );

    BOOST_REQUIRE( common.CreateBranch( "feature" ) );
    BOOST_CHECK_EQUAL( common.GetUnmergedCommitCount( "feature" ), 0u );

    BOOST_REQUIRE_EQUAL( git_repository_set_head( repo, "refs/heads/feature" ), GIT_OK );

    wxFileName fn( tmp.path, wxS( "file.txt" ) );
    {
        std::ofstream f( fn.GetFullPath().utf8_string() );
        f << "v2\n";
    }

    commitWorkingFile( repo, wxS( "file.txt" ), "only on feature" );
    BOOST_REQUIRE_EQUAL( git_repository_set_head( repo, "refs/heads/main" ), GIT_OK );

    BOOST_CHECK_EQUAL( common.GetUnmergedCommitCount( "feature" ), 1u );
    BOOST_CHECK_EQUAL( common.GetUnmergedCommitCount( "main" ), 0u );
    BOOST_CHECK_EQUAL( common.GetUnmergedCommitCount( "no-such-branch" ), 0u );
}


// Remotes round-trip, and neither getter returns an embedded user:password.
BOOST_AUTO_TEST_CASE( RemoteRoundTripAndUserinfoStripped )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_CHECK( common.GetRemotes().empty() );

    BOOST_CHECK( common.AddRemote( "origin", "https://example.com/x.git" ) );

    std::vector<std::pair<wxString, wxString>> remotes = common.GetRemotes();
    BOOST_REQUIRE_EQUAL( remotes.size(), 1u );
    BOOST_CHECK_EQUAL( remotes[0].first.ToStdString(), "origin" );
    BOOST_CHECK_EQUAL( remotes[0].second.ToStdString(), "https://example.com/x.git" );

    BOOST_CHECK( common.SetRemoteUrl( "origin", "https://u:secret@example.com/y.git" ) );

    // The password must not surface through GetRemotes or GetRemoteUrl.
    BOOST_CHECK_EQUAL( common.GetRemotes()[0].second.ToStdString(), "https://example.com/y.git" );
    BOOST_CHECK_EQUAL( common.GetRemoteUrl().ToStdString(), "https://example.com/y.git" );

    BOOST_CHECK( common.RemoveRemote( "origin" ) );
    BOOST_CHECK( common.GetRemotes().empty() );
}


// Branches created at the same HEAD share a timestamp. A std::map keyed on it would drop one.
BOOST_AUTO_TEST_CASE( GetAllBranchesKeepsBranchesSharingACommitTime )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_REQUIRE( common.CreateBranch( "alpha" ) );
    BOOST_REQUIRE( common.CreateBranch( "beta" ) );

    std::vector<std::pair<wxString, bool>> all = common.GetAllBranches();

    bool haveAlpha = false;
    bool haveBeta = false;

    for( const auto& [name, isRemote] : all )
    {
        if( name == wxS( "alpha" ) )
            haveAlpha = true;
        else if( name == wxS( "beta" ) )
            haveBeta = true;
    }

    BOOST_CHECK_MESSAGE( haveAlpha && haveBeta,
                         "both same-commit-time branches must appear, got " + std::to_string( all.size() ) );
}


// The Branches page lists every branch with the commit it points at, the current one first.
BOOST_AUTO_TEST_CASE( GetBranchesCarriesTipAndCurrent )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_REQUIRE( common.CreateBranch( "older" ) );

    writeFile( tmp.path, "file.txt", "v2\n" );
    commitWorkingFile( repo, "file.txt", "second" );

    std::vector<KIGIT_COMMIT_INFO> commits = common.GetRecentCommits( 10 );
    BOOST_REQUIRE_EQUAL( commits.size(), 2u );

    auto hashOf = [&]( const char* aSummary )
    {
        for( const KIGIT_COMMIT_INFO& c : commits )
        {
            if( c.summary == aSummary )
                return c.hash.ToStdString();
        }

        return std::string();
    };

    std::vector<KIGIT_BRANCH_INFO> branches = common.GetBranches();
    BOOST_REQUIRE_EQUAL( branches.size(), 2u );

    BOOST_CHECK_EQUAL( branches[0].name.ToStdString(), "main" );
    BOOST_CHECK( branches[0].isCurrent );
    BOOST_CHECK( !branches[0].isRemote );
    BOOST_CHECK_EQUAL( branches[0].tipHash.ToStdString(), hashOf( "second" ) );
    BOOST_CHECK( branches[0].when.IsValid() );

    BOOST_CHECK_EQUAL( branches[1].name.ToStdString(), "older" );
    BOOST_CHECK( !branches[1].isCurrent );
    BOOST_CHECK_EQUAL( branches[1].tipHash.ToStdString(), hashOf( "initial" ) );
}


// The detail pane shows a branch's tip commit and how far it is from the branch it tracks.
BOOST_AUTO_TEST_CASE( GetBranchDetailReportsTipAndUpstream )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_REQUIRE( common.CreateBranch( "base" ) );

    writeFile( tmp.path, "file.txt", "v2\n" );
    commitWorkingFile( repo, "file.txt", "second\n\nWith a body." );

    // Track the local branch, which git allows, so no remote is needed to see ahead and behind.
    git_reference* main = nullptr;
    BOOST_REQUIRE_EQUAL( git_branch_lookup( &main, repo, "main", GIT_BRANCH_LOCAL ), GIT_OK );
    KIGIT::GitReferencePtr mainPtr( main );
    BOOST_REQUIRE_EQUAL( git_branch_set_upstream( main, "base" ), GIT_OK );

    KIGIT_BRANCH_DETAIL detail = common.GetBranchDetail( wxS( "main" ) );
    BOOST_REQUIRE( detail.found );
    BOOST_CHECK_EQUAL( detail.name.ToStdString(), "main" );
    BOOST_CHECK_EQUAL( detail.tipHash.ToStdString(), common.GetRecentCommits( 1 )[0].hash.ToStdString() );
    BOOST_CHECK_EQUAL( detail.author.ToStdString(), "QA Test" );
    BOOST_CHECK_EQUAL( detail.email.ToStdString(), "qa@example.com" );
    BOOST_CHECK_EQUAL( detail.message.ToStdString(), "second\n\nWith a body." );
    BOOST_CHECK_EQUAL( detail.upstream.ToStdString(), "base" );
    BOOST_CHECK_EQUAL( detail.ahead, 1u );
    BOOST_CHECK_EQUAL( detail.behind, 0u );

    KIGIT_BRANCH_DETAIL base = common.GetBranchDetail( wxS( "base" ) );
    BOOST_REQUIRE( base.found );
    BOOST_CHECK( base.upstream.IsEmpty() );
    BOOST_CHECK_EQUAL( base.message.ToStdString(), "initial" );

    BOOST_CHECK( !common.GetBranchDetail( wxS( "nowhere" ) ).found );
}


// New Branch in the detail pane starts the branch where the selected one is, not at HEAD.
BOOST_AUTO_TEST_CASE( CreateBranchStartsAtTheGivenBranch )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_REQUIRE( common.CreateBranch( "base" ) );

    writeFile( tmp.path, "file.txt", "v2\n" );
    commitWorkingFile( repo, "file.txt", "second" );

    BOOST_REQUIRE( common.CreateBranch( "feature", "base" ) );

    KIGIT_BRANCH_DETAIL feature = common.GetBranchDetail( wxS( "feature" ) );
    KIGIT_BRANCH_DETAIL base = common.GetBranchDetail( wxS( "base" ) );
    BOOST_REQUIRE( feature.found && base.found );
    BOOST_CHECK_EQUAL( feature.tipHash.ToStdString(), base.tipHash.ToStdString() );

    BOOST_CHECK( !common.CreateBranch( "orphan", "nowhere" ) );
}


// The Remotes page shows when the repository last fetched. Git leaves that in FETCH_HEAD, which
// a repository that never fetched does not have.
BOOST_AUTO_TEST_CASE( GetLastFetchTimeReadsFetchHead )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_CHECK( !common.GetLastFetchTime().IsValid() );

    wxFileName fetchHead( wxString::FromUTF8( git_repository_path( repo ) ), wxS( "FETCH_HEAD" ) );

    {
        std::ofstream f( fetchHead.GetFullPath().utf8_string() );
        f << "0000000000000000000000000000000000000000\t\tbranch 'main' of https://example.com/x.git\n";
    }

    wxDateTime when = common.GetLastFetchTime();
    BOOST_REQUIRE( when.IsValid() );
    BOOST_CHECK( ( wxDateTime::Now() - when ).GetMinutes() < 5 );
}


// The Changes page sums the lines the working tree adds and removes, a new file counting as
// all added.
BOOST_AUTO_TEST_CASE( GetWorkingTreeLineStatsCountsEditsAndNewFiles )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "one\ntwo\nthree\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    std::pair<int, int> clean = common.GetWorkingTreeLineStats( { wxS( "file.txt" ) } );
    BOOST_CHECK_EQUAL( clean.first, 0 );
    BOOST_CHECK_EQUAL( clean.second, 0 );

    writeFile( tmp.path, "file.txt", "one\n2\nthree\n" );
    writeFile( tmp.path, "new.txt", "a\nb\n" );

    std::pair<int, int> dirty = common.GetWorkingTreeLineStats( { wxS( "file.txt" ), wxS( "new.txt" ) } );
    BOOST_CHECK_EQUAL( dirty.first, 3 );
    BOOST_CHECK_EQUAL( dirty.second, 1 );
}


// The detail pane shows what a stash holds: the branch and commit it was taken on, its message
// and the files it changes, a new file included.
BOOST_AUTO_TEST_CASE( GetStashDetailNamesBranchCommitAndFiles )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    std::vector<KIGIT_COMMIT_INFO> commits = common.GetRecentCommits( 1 );
    BOOST_REQUIRE_EQUAL( commits.size(), 1u );

    writeFile( tmp.path, "file.txt", "v2\n" );
    writeFile( tmp.path, "new.txt", "n\n" );

    {
        git_index* index = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
        KIGIT::GitIndexPtr indexPtr( index );
        BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, "new.txt" ), GIT_OK );
        BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );
    }

    BOOST_REQUIRE( common.StashSave( wxS( "connector work" ) ) );

    std::vector<KIGIT_STASH_INFO> stashes = common.GetStashes();
    BOOST_REQUIRE_EQUAL( stashes.size(), 1u );

    KIGIT_STASH_DETAIL detail = common.GetStashDetail( stashes[0].id );
    BOOST_REQUIRE( detail.found );
    BOOST_CHECK_EQUAL( detail.message.ToStdString(), "connector work" );
    BOOST_CHECK_EQUAL( detail.branch.ToStdString(), "main" );
    BOOST_CHECK_EQUAL( detail.baseHash.ToStdString(), commits[0].hash.ToStdString() );
    BOOST_CHECK( detail.when.IsValid() );

    std::vector<std::string> files;

    for( const wxString& f : detail.files )
        files.push_back( f.ToStdString() );

    std::sort( files.begin(), files.end() );
    BOOST_REQUIRE_EQUAL( files.size(), 2u );
    BOOST_CHECK_EQUAL( files[0], "file.txt" );
    BOOST_CHECK_EQUAL( files[1], "new.txt" );

    BOOST_CHECK( !common.GetStashDetail( wxS( "0000000000000000000000000000000000000000" ) ).found );
}


// The Tags page shows a tag's message, author and date. An annotated tag has its own, a
// lightweight tag borrows the commit's.
// An amend or a rebase leaves a commit's author time behind its committer time. The History
// page dates a commit by the committer time, so the Tags page has to agree.
BOOST_AUTO_TEST_CASE( GetTagDetailDatesALightweightTagByCommitterTime )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    git_signature* authored = nullptr;
    BOOST_REQUIRE_EQUAL( git_signature_new( &authored, "QA Test", "qa@example.com", 1000, 0 ), GIT_OK );
    KIGIT::GitSignaturePtr authoredPtr( authored );

    git_signature* committed = nullptr;
    BOOST_REQUIRE_EQUAL( git_signature_new( &committed, "QA Test", "qa@example.com", 2000, 0 ), GIT_OK );
    KIGIT::GitSignaturePtr committedPtr( committed );

    git_oid headOid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headOid, repo, "HEAD" ), GIT_OK );

    git_commit* parent = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &parent, repo, &headOid ), GIT_OK );
    KIGIT::GitCommitPtr parentPtr( parent );

    git_tree* tree = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_tree( &tree, parent ), GIT_OK );
    KIGIT::GitTreePtr tree_ptr( tree );

    const git_commit* parents[1] = { parent };
    git_oid           commitOid;
    BOOST_REQUIRE_EQUAL(
            git_commit_create( &commitOid, repo, "HEAD", authored, committed, nullptr, "rebased", tree, 1, parents ),
            GIT_OK );

    git_object* target = nullptr;
    BOOST_REQUIRE_EQUAL( git_revparse_single( &target, repo, "HEAD" ), GIT_OK );
    KIGIT::GitObjectPtr targetPtr( target );

    git_oid tagOid;
    BOOST_REQUIRE_EQUAL( git_tag_create_lightweight( &tagOid, repo, "rebased", target, 0 ), GIT_OK );

    KIGIT_COMMON     common( repo );
    KIGIT_TAG_DETAIL detail = common.GetTagDetail( wxS( "rebased" ) );

    BOOST_REQUIRE( detail.found );
    BOOST_CHECK_EQUAL( detail.when.GetTicks(), 2000 );
    BOOST_CHECK_EQUAL( common.GetRecentCommits( 1 )[0].when.GetTicks(), detail.when.GetTicks() );
}


BOOST_AUTO_TEST_CASE( GetTagDetailReadsAnnotatedAndLightweightTags )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    wxString     head = common.GetRecentCommits( 1 )[0].hash;

    // A different tagger from the commit author, so the detail must come from the tag object.
    BOOST_REQUIRE( common.SetUserConfig( wxS( "Tag Maker" ), wxS( "tags@example.com" ) ) );
    BOOST_REQUIRE( common.CreateTag( "v1.0", "Release v1.0\n\n- Add mounting holes" ) );
    BOOST_REQUIRE( common.CreateTag( "beta-1" ) );

    git_object* tagObject = nullptr;
    BOOST_REQUIRE_EQUAL( git_revparse_single( &tagObject, repo, "refs/tags/v1.0" ), GIT_OK );
    KIGIT::GitObjectPtr tagObjectPtr( tagObject );
    BOOST_CHECK_EQUAL( git_object_type( tagObject ), GIT_OBJECT_TAG );

    KIGIT_TAG_DETAIL annotated = common.GetTagDetail( wxS( "v1.0" ) );
    BOOST_REQUIRE( annotated.found );
    BOOST_CHECK_EQUAL( annotated.name.ToStdString(), "v1.0" );
    BOOST_CHECK_EQUAL( annotated.hash.ToStdString(), head.ToStdString() );
    BOOST_CHECK_EQUAL( annotated.message.ToStdString(), "Release v1.0\n\n- Add mounting holes" );
    BOOST_CHECK_EQUAL( annotated.author.ToStdString(), "Tag Maker" );
    BOOST_CHECK_EQUAL( annotated.email.ToStdString(), "tags@example.com" );
    BOOST_CHECK( annotated.when.IsValid() );

    KIGIT_TAG_DETAIL lightweight = common.GetTagDetail( wxS( "beta-1" ) );
    BOOST_REQUIRE( lightweight.found );
    BOOST_CHECK_EQUAL( lightweight.name.ToStdString(), "beta-1" );
    BOOST_CHECK_EQUAL( lightweight.hash.ToStdString(), head.ToStdString() );
    BOOST_CHECK_EQUAL( lightweight.message.ToStdString(), "initial" );
    BOOST_CHECK_EQUAL( lightweight.author.ToStdString(), "QA Test" );
    BOOST_CHECK( lightweight.when.IsValid() );

    // An old tag made long ago, and a tag on a tree, which has no commit and so no date.
    git_object* commitObject = nullptr;
    BOOST_REQUIRE_EQUAL( git_revparse_single( &commitObject, repo, "HEAD" ), GIT_OK );
    KIGIT::GitObjectPtr commitObjectPtr( commitObject );

    git_signature* oldSig = nullptr;
    BOOST_REQUIRE_EQUAL( git_signature_new( &oldSig, "QA Test", "qa@example.com", 1000, 0 ), GIT_OK );
    KIGIT::GitSignaturePtr oldSigPtr( oldSig );

    git_oid oid;
    BOOST_REQUIRE_EQUAL( git_tag_create( &oid, repo, "old", commitObject, oldSig, "old tag", 0 ), GIT_OK );

    git_object* treeObject = nullptr;
    BOOST_REQUIRE_EQUAL( git_revparse_single( &treeObject, repo, "HEAD^{tree}" ), GIT_OK );
    KIGIT::GitObjectPtr treeObjectPtr( treeObject );
    BOOST_REQUIRE_EQUAL( git_tag_create_lightweight( &oid, repo, "tree-tag", treeObject, 0 ), GIT_OK );

    std::vector<KIGIT_TAG_INFO> tags = common.GetTags();
    BOOST_REQUIRE_EQUAL( tags.size(), 4 );
    BOOST_CHECK( tags[0].name == wxS( "v1.0" ) || tags[0].name == wxS( "beta-1" ) );
    BOOST_CHECK_EQUAL( tags[2].name.ToStdString(), "old" );
    BOOST_CHECK_EQUAL( tags[3].name.ToStdString(), "tree-tag" );
    BOOST_CHECK( tags[3].hash.IsEmpty() && !tags[3].when.IsValid() );

    BOOST_CHECK( !common.GetTagDetail( wxS( "nowhere" ) ).found );
}


// The docked panel's Recent messages dropdown offers earlier messages to start from. Newest
// first, a message that was used twice listed once, and only what HEAD can reach.
BOOST_AUTO_TEST_CASE( GetRecentCommitMessagesListsDistinctMessagesNewestFirst )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    writeFile( tmp.path, "file.txt", "v2\n" );
    commitWorkingFile( repo, "file.txt", "Update BOM\n\nSecond pass.\n" );
    writeFile( tmp.path, "file.txt", "v3\n" );
    commitWorkingFile( repo, "file.txt", "Route power\n" );
    writeFile( tmp.path, "file.txt", "v4\n" );
    commitWorkingFile( repo, "file.txt", "Update BOM\n\nSecond pass.\n" );
    writeFile( tmp.path, "file.txt", "v5\n" );
    commitWorkingFile( repo, "file.txt", "   \n\n" );

    KIGIT_COMMON          common( repo );
    std::vector<wxString> messages = common.GetRecentCommitMessages( 10 );

    // The blank message is not offered.
    BOOST_REQUIRE_EQUAL( messages.size(), 3 );
    BOOST_CHECK_EQUAL( messages[0], wxS( "Update BOM\n\nSecond pass." ) );
    BOOST_CHECK_EQUAL( messages[1], wxS( "Route power" ) );
    BOOST_CHECK_EQUAL( messages[2], wxS( "initial" ) );

    BOOST_CHECK_EQUAL( common.GetRecentCommitMessages( 1 ).size(), 1 );
    BOOST_CHECK( KIGIT_COMMON( nullptr ).GetRecentCommitMessages( 10 ).empty() );

    // A repository with no commit yet has nothing to offer either.
    ScopedTempDir   empty;
    git_repository* unborn = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_init( &unborn, empty.path.utf8_string().c_str(), 0 ), GIT_OK );
    KIGIT::GitRepositoryPtr unbornPtr( unborn );
    BOOST_CHECK( KIGIT_COMMON( unborn ).GetRecentCommitMessages( 10 ).empty() );
}


// The docked panel composes the message from a summary line, a description box and a sign off
// tick, and takes an amended commit's message apart again to refill those boxes.
BOOST_AUTO_TEST_CASE( ComposeAndSplitCommitMessage )
{
    wxString signOff = wxS( "QA Test <qa@example.com>" );

    BOOST_CHECK_EQUAL( KIGIT::ComposeCommitMessage( wxS( "  Fix the thing  " ), wxEmptyString, wxEmptyString ),
                       wxS( "Fix the thing" ) );
    BOOST_CHECK_EQUAL( KIGIT::ComposeCommitMessage( wxS( "Fix" ), wxS( "Why it broke.\n" ), wxEmptyString ),
                       wxS( "Fix\n\nWhy it broke." ) );
    BOOST_CHECK_EQUAL( KIGIT::ComposeCommitMessage( wxS( "Fix" ), wxEmptyString, signOff ),
                       wxS( "Fix\n\nSigned-off-by: QA Test <qa@example.com>" ) );
    BOOST_CHECK_EQUAL( KIGIT::ComposeCommitMessage( wxS( "Fix" ), wxS( "Why." ), signOff ),
                       wxS( "Fix\n\nWhy.\n\nSigned-off-by: QA Test <qa@example.com>" ) );

    // An amended message already carries its sign off, even with a trailer after it, and
    // someone else's sign off is not ours.
    wxString signedOff = wxS( "Why.\n\nSigned-off-by: QA Test <qa@example.com>" );
    wxString coauthor = wxS( "\nCo-authored-by: X <x@example.com>" );
    wxString other = wxS( "Signed-off-by: Other <o@example.com>" );

    BOOST_CHECK_EQUAL( KIGIT::ComposeCommitMessage( wxS( "Fix" ), signedOff, signOff ), wxS( "Fix\n\n" ) + signedOff );
    BOOST_CHECK_EQUAL( KIGIT::ComposeCommitMessage( wxS( "Fix" ), signedOff + wxS( "\n" ), signOff ),
                       wxS( "Fix\n\n" ) + signedOff );
    BOOST_CHECK_EQUAL( KIGIT::ComposeCommitMessage( wxS( "Fix" ), signedOff + coauthor, signOff ),
                       wxS( "Fix\n\n" ) + signedOff + coauthor );
    BOOST_CHECK_EQUAL( KIGIT::ComposeCommitMessage( wxS( "Fix" ), other, signOff ),
                       wxS( "Fix\n\n" ) + other + wxS( "\n\nSigned-off-by: QA Test <qa@example.com>" ) );

    wxString summary, description;

    KIGIT::SplitCommitMessage( wxS( "Fix\n\nWhy it broke.\nAnd how.\n" ), summary, description );
    BOOST_CHECK_EQUAL( summary, wxS( "Fix" ) );
    BOOST_CHECK_EQUAL( description, wxS( "Why it broke.\nAnd how." ) );

    KIGIT::SplitCommitMessage( wxS( "Just a line\n" ), summary, description );
    BOOST_CHECK_EQUAL( summary, wxS( "Just a line" ) );
    BOOST_CHECK( description.IsEmpty() );

    // A two line message with no blank line keeps its second line as the description.
    KIGIT::SplitCommitMessage( wxS( "First\nSecond" ), summary, description );
    BOOST_CHECK_EQUAL( summary, wxS( "First" ) );
    BOOST_CHECK_EQUAL( description, wxS( "Second" ) );

    // A message read back from a Windows checkout can carry carriage returns.
    KIGIT::SplitCommitMessage( wxS( "Fix\r\n\r\nWhy.\r\n" ), summary, description );
    BOOST_CHECK_EQUAL( summary, wxS( "Fix" ) );
    BOOST_CHECK_EQUAL( description, wxS( "Why." ) );
}


// Committing to a new branch creates it at HEAD and switches before the commit. The switch
// must leave the uncommitted work in place, since the two branches share the same tree.
BOOST_AUTO_TEST_CASE( SwitchToBranchAtHeadKeepsUncommittedChanges )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    writeFile( tmp.path, "file.txt", "v2\n" );

    KIGIT_COMMON common( repo );
    BOOST_REQUIRE( common.CreateBranch( "topic" ) );

    GIT_BRANCH_HANDLER handler( &common );
    BOOST_REQUIRE_MESSAGE( handler.SwitchToBranch( wxS( "topic" ) ) == BranchResult::Success,
                           handler.GetErrorString() );

    BOOST_CHECK_EQUAL( common.GetCurrentBranchName(), wxS( "topic" ) );
    BOOST_CHECK_EQUAL( readWorkingFile( wxFileName( tmp.path, "file.txt" ) ), "v2\n" );
    BOOST_CHECK( common.HasUncommittedTrackedChanges() );
}


// The stored connection string must not carry the secret. A scp-style git@host is left intact.
BOOST_AUTO_TEST_CASE( SetRemoteStripsUrlUserinfo )
{
    KIGIT_COMMON common( nullptr );

    common.SetRemote( wxS( "https://user:secret@gitlab.com/x/y.git" ) );
    BOOST_CHECK_EQUAL( common.GetRemote().ToStdString(), "https://gitlab.com/x/y.git" );

    common.SetRemote( wxS( "git@gitlab.com:x/y.git" ) );
    BOOST_CHECK_EQUAL( common.GetRemote().ToStdString(), "git@gitlab.com:x/y.git" );
}


// A password may contain '@'. Splitting on the first one would leave part of the secret.
BOOST_AUTO_TEST_CASE( SetRemoteStripsUserinfoContainingAtSign )
{
    KIGIT_COMMON common( nullptr );

    common.SetRemote( wxS( "https://alice:pa@ss@gitlab.com/x/y.git" ) );
    BOOST_CHECK_EQUAL( common.GetRemote().ToStdString(), "https://gitlab.com/x/y.git" );

    // The credentials must also be split at the right place.
    BOOST_CHECK_EQUAL( common.GetUsername().ToStdString(), "alice" );
    BOOST_CHECK_EQUAL( common.GetPassword().ToStdString(), "pa@ss" );

    // An '@' that belongs to the path is not userinfo.
    common.SetRemote( wxS( "https://gitlab.com/x/y@z.git" ) );
    BOOST_CHECK_EQUAL( common.GetRemote().ToStdString(), "https://gitlab.com/x/y@z.git" );
}


// Called after editing a remote, so a later push authenticates against the new server.
BOOST_AUTO_TEST_CASE( UpdateCurrentBranchInfoRefreshesChangedRemoteUrl )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_REQUIRE( common.AddRemote( "origin", "https://example.com/old.git" ) );

    git_config* cfg = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_config( &cfg, repo ), GIT_OK );
    KIGIT::GitConfigPtr cfgPtr( cfg );
    BOOST_REQUIRE_EQUAL( git_config_set_string( cfg, "branch.main.remote", "origin" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_config_set_string( cfg, "branch.main.merge", "refs/heads/main" ), GIT_OK );

    common.UpdateCurrentBranchInfo();
    BOOST_CHECK_EQUAL( common.GetRemote().ToStdString(), "https://example.com/old.git" );

    BOOST_REQUIRE( common.SetRemoteUrl( "origin", "https://example.com/new.git" ) );
    common.UpdateCurrentBranchInfo();
    BOOST_CHECK_EQUAL( common.GetRemote().ToStdString(), "https://example.com/new.git" );
}


// An empty result means the end of the list, so an empty entry would hide every key behind it.
BOOST_AUTO_TEST_CASE( SetSSHKeyIgnoresEmptyKey )
{
    KIGIT_COMMON common( nullptr );

    common.SetSSHKey( wxS( "/home/qa/.ssh/id_ed25519" ) );
    common.SetSSHKey( wxEmptyString );
    common.ResetNextKey();

    BOOST_CHECK_EQUAL( common.GetNextPublicKey().ToStdString(), "/home/qa/.ssh/id_ed25519" );
}


// libgit2 reports forward slashes on every platform while the project path is native. On Windows
// the raw prefix test rejected every file and the commit panel stayed empty. A backslash is an
// ordinary file name character elsewhere, so only Windows folds the separators.
BOOST_AUTO_TEST_CASE( IsWithinProjectPathHandlesMixedSeparators )
{
    using KIGIT::PROJECT_GIT_UTILS;

#ifdef __WINDOWS__
    BOOST_CHECK(
            PROJECT_GIT_UTILS::IsWithinProjectPath( wxS( "C:/repo/proj/board.kicad_pcb" ), wxS( "C:\\repo\\proj" ) ) );
    BOOST_CHECK( PROJECT_GIT_UTILS::IsWithinProjectPath( wxS( "C:/repo/proj/sub/sheet.kicad_sch" ),
                                                         wxS( "C:\\repo\\proj\\" ) ) );
    BOOST_CHECK( !PROJECT_GIT_UTILS::IsWithinProjectPath( wxS( "C:/repo/other/board.kicad_pcb" ),
                                                          wxS( "C:\\repo\\proj" ) ) );
#endif

    // A sibling directory sharing the prefix as plain text is not inside the project.
    BOOST_CHECK( !PROJECT_GIT_UTILS::IsWithinProjectPath( wxS( "/repo/project2/file" ), wxS( "/repo/project" ) ) );
    BOOST_CHECK( PROJECT_GIT_UTILS::IsWithinProjectPath( wxS( "/repo/project/file" ), wxS( "/repo/project" ) ) );
}


// The pull gate refuses over uncommitted tracked changes, but untracked files must not block
// getting changes, since a merge leaves them alone.
BOOST_AUTO_TEST_CASE( HasUncommittedTrackedChangesIgnoresUntracked )
{
    GitInitGuard  libgit;
    ScopedTempDir tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_CHECK( !common.HasUncommittedTrackedChanges() );

    // An untracked file alone does not count.
    {
        wxFileName    fn( tmp.path, wxS( "notes.txt" ) );
        std::ofstream f( fn.GetFullPath().utf8_string() );
        f << "scratch\n";
    }

    BOOST_CHECK( !common.HasUncommittedTrackedChanges() );

    // A modified tracked file does.
    {
        wxFileName    fn( tmp.path, wxS( "file.txt" ) );
        std::ofstream f( fn.GetFullPath().utf8_string() );
        f << "v2\n";
    }

    BOOST_CHECK( common.HasUncommittedTrackedChanges() );
}


// Discarding a conflicted file puts back what HEAD holds and clears the conflict, so it settles
// the file rather than only throwing away an edit. The panel says so, and asks first.
BOOST_AUTO_TEST_CASE( DiscardingAConflictedPathSettlesIt )
{
    SCOPED_GIT_BACKEND libgit;
    ScopedTempDir      tmp;

    git_repository*         repo = makeRepoWithCommit( tmp.path, "file.txt", "one\ntwo\nthree\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_CHECK( !common.IsPathConflicted( wxS( "file.txt" ) ) );
    BOOST_CHECK( !common.IsPathConflicted( wxS( "nothing.txt" ) ) );

    git_oid blobOid;
    BOOST_REQUIRE_EQUAL( git_blob_create_from_buffer( &blobOid, repo, "other\n", 6 ), GIT_OK );

    {
        git_index* index = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
        KIGIT::GitIndexPtr indexPtr( index );

        git_index_entry stage{};
        stage.path = "file.txt";
        stage.mode = GIT_FILEMODE_BLOB;
        git_oid_cpy( &stage.id, &blobOid );

        BOOST_REQUIRE_EQUAL( git_index_conflict_add( index, &stage, &stage, &stage ), GIT_OK );
        BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );
    }

    BOOST_CHECK( common.IsPathConflicted( wxS( "file.txt" ) ) );

    wxString error;
    BOOST_REQUIRE_MESSAGE( common.DiscardChanges( { wxS( "file.txt" ) }, error ), error.ToStdString() );

    BOOST_CHECK_MESSAGE( !common.IsPathConflicted( wxS( "file.txt" ) ),
                         "discarding left the file still needing resolution" );
}


BOOST_AUTO_TEST_SUITE_END()
