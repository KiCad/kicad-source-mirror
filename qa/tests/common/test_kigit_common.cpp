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

#include <dialogs/git/git_credentials_prompt.h>
#include <dialogs/git/git_status_entries.h>
#include <dialogs/git/git_transfer.h>
#include <dialogs/git/git_ui_utils.h>
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <git/project_git_utils.h>
#include <git/git_branch_handler.h>
#include <git/git_push_handler.h>
#include <qa_utils/file_utils.h>
#include <qa_utils/git_test_fixture.h>

#include <git2.h>
#include <git2/sys/repository.h>

#include <wx/app.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/string.h>
#include <wx/stdpaths.h>

#include <chrono>
#include <algorithm>
#include <fstream>
#include <future>
#include <iterator>
#include <memory>
#include <set>
#include <stdexcept>
#include <thread>

#ifndef __WINDOWS__
#include <unistd.h>
#endif


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


// Initialize a git repository at the given path with one initial commit on "main"
git_repository* makeRepoWithCommit( const wxString& aRepoPath, const wxString& aFileName,
                                    const std::string& aFileContents )
{
    KIGIT::GitRepositoryPtr repo = KI_TEST::InitGitRepository( aRepoPath );
    KI_TEST::WriteGitFile( aRepoPath, aFileName, aFileContents );
    KI_TEST::CommitGitFile( repo.get(), aFileName, "initial" );
    return repo.release();
}


// Stage aFileName from the working tree and commit it on top of HEAD.
void commitWorkingFile( git_repository* aRepo, const wxString& aFileName, const char* aMessage )
{
    KI_TEST::CommitGitFile( aRepo, aFileName, aMessage );
}


std::string readWorkingFile( const wxFileName& aFile )
{
    std::ifstream in( aFile.GetFullPath().utf8_string() );
    return std::string( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
}


void writeFile( const wxString& aRepoPath, const wxString& aFileName, const char* aContent )
{
    KI_TEST::WriteGitFile( aRepoPath, aFileName, aContent );
}


// Query completions use the application's event queue, just as they do for a dialog.
bool waitForQuery( const std::function<bool()>& aFinished )
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );

    while( !aFinished() && std::chrono::steady_clock::now() < deadline )
    {
        wxTheApp->ProcessPendingEvents();
        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
    }

    return aFinished();
}


// Like wxWeakRef, every copy and destruction of this callback capture must stay on the UI
// thread. A shared pointee alone would miss copies made and destroyed while posting an event.
struct UI_CALLBACK_CAPTURE
{
    explicit UI_CALLBACK_CAPTURE( std::shared_ptr<std::atomic<bool>> aWrongThread ) :
            wrongThread( std::move( aWrongThread ) ),
            uiThread( std::this_thread::get_id() )
    {
    }

    UI_CALLBACK_CAPTURE( const UI_CALLBACK_CAPTURE& aOther ) :
            wrongThread( aOther.wrongThread ),
            uiThread( aOther.uiThread )
    {
        if( std::this_thread::get_id() != uiThread )
            wrongThread->store( true );
    }

    UI_CALLBACK_CAPTURE( UI_CALLBACK_CAPTURE&& ) = default;

    ~UI_CALLBACK_CAPTURE()
    {
        if( wrongThread && std::this_thread::get_id() != uiThread )
            wrongThread->store( true );
    }

    std::shared_ptr<std::atomic<bool>> wrongThread;
    std::thread::id                    uiThread;
};

} // namespace


BOOST_FIXTURE_TEST_SUITE( KiGitCommon, KI_TEST::GIT_TEST_FIXTURE )


// git_repository_path() returns a path ending in /.git/, which broke the popup-menu test for
// whether a project lives at the repository root.
BOOST_AUTO_TEST_CASE( GitRootDirectoryReturnsWorkdir )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "data\n" );
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
    tmpFn.AssignDir( TempPath() );
    tmpFn.MakeAbsolute();
    BOOST_CHECK_EQUAL( rootFn.GetFullPath().ToStdString(), tmpFn.GetFullPath().ToStdString() );
}


// The remote need not be called origin.
BOOST_AUTO_TEST_CASE( HasPushAndPullRemoteAcceptsNonOriginName )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "data\n" );
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
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "data\n" );
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
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "data\n" );
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
    // Local root commit with two files.  The bug previously listed both as AHEAD.
    git_repository*         repo = makeRepoWithCommit( TempPath(), "untouched.txt", "stable\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    // Add a second file to local HEAD so we have more than the bare initial tree.
    {
        wxFileName    extra( TempPath(), wxS( "second.txt" ) );
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
    // Initial commit becomes the shared base.  Two files: only one will be modified later.
    git_repository*         repo = makeRepoWithCommit( TempPath(), "untouched.txt", "stable\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    {
        wxFileName    extra( TempPath(), wxS( "touched.txt" ) );
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
        wxFileName    extra( TempPath(), wxS( "touched.txt" ) );
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
    git_repository*         repo = makeRepoWithCommit( TempPath(), "untouched.txt", "stable\n" );
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
        wxFileName    extra( TempPath(), wxS( "remote_only.txt" ) );
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


BOOST_AUTO_TEST_CASE( GetRecentCommitsHonorsAnAlreadyCancelledOperation )
{
    KIGIT::GitRepositoryPtr repo( makeRepoWithCommit( TempPath(), "file.txt", "one\n" ) );
    KIGIT_COMMON            common( repo.get() );
    auto                    lock = common.LockAction();

    BOOST_REQUIRE_EQUAL( common.GetRecentCommits( 10 ).size(), 1u );

    KIGIT_COMMON::CANCEL_TOKEN token = KIGIT_COMMON::NewCancelToken();
    token->store( true );

    {
        KIGIT_COMMON::OPERATION_GUARD operation( &common, token );
        BOOST_CHECK( common.GetRecentCommits( 10 ).empty() );
    }

    // Cancellation belongs to that operation. A later history read must still work.
    BOOST_CHECK_EQUAL( common.GetRecentCommits( 10 ).size(), 1u );
}


// GetRecentCommits returns commits newest-first with their summary, author and short hash,
// and honors the requested maximum.
BOOST_AUTO_TEST_CASE( GetRecentCommitsReturnsNewestFirst )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    // Add a second commit on top of the first.
    {
        wxFileName    fn( TempPath(), wxS( "file.txt" ) );
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


// Git does not check the address itself, so this is only here to catch a typo before every
// commit carries it.
BOOST_AUTO_TEST_CASE( IdentityEmailMustLookLikeAnAddress )
{
    wxString error;

    BOOST_CHECK( KIGIT::ValidateIdentity( wxT( "A Name" ), wxT( "a@b.c" ), error ) );
    BOOST_CHECK( KIGIT::ValidateIdentity( wxT( "A Name" ), wxT( "first.last+tag@example.co.uk" ), error ) );

    BOOST_CHECK( !KIGIT::ValidateIdentity( wxEmptyString, wxT( "a@b.c" ), error ) );
    BOOST_CHECK( !KIGIT::ValidateIdentity( wxT( "   " ), wxT( "a@b.c" ), error ) );

    for( const wxString& bad : { wxT( "a@" ), wxT( "@" ), wxT( "@@" ), wxT( "a@b" ), wxT( "@b.c" ), wxT( "a b@c.d" ),
                                 wxT( "a@b.c@d.e" ), wxT( "a@.c" ), wxT( "a@b." ) } )
    {
        BOOST_CHECK_MESSAGE( !KIGIT::ValidateIdentity( wxT( "A Name" ), bad, error ), bad );
    }
}

// A project can be open before it is ever put under version control, so a session with no
// repository is a normal state and not a programming error.
BOOST_AUTO_TEST_CASE( SessionWithoutARepositoryAnswersQuietly )
{
    KIGIT_COMMON common( nullptr );

    BOOST_CHECK( common.GetCurrentBranchName().IsEmpty() );
    BOOST_CHECK( common.GetProjectDirs().empty() );
    BOOST_CHECK( !common.HasPushAndPullRemote() );
    BOOST_CHECK( common.GetRemotename().IsEmpty() );

    BOOST_CHECK_NO_THROW( common.UpdateCurrentBranchInfo() );
}

// A branch name the user typed is checked before git sees it, so the refusal names the
// mistake instead of quoting libgit2.
BOOST_AUTO_TEST_CASE( BranchNameErrorNamesTheMistake )
{
    BOOST_CHECK( KIGIT::BranchNameError( wxS( "feature/new-thing" ) ).IsEmpty() );
    BOOST_CHECK( KIGIT::BranchNameError( wxS( "fix_25074" ) ).IsEmpty() );

    BOOST_CHECK( !KIGIT::BranchNameError( wxEmptyString ).IsEmpty() );
    BOOST_CHECK( KIGIT::BranchNameError( wxS( "two words" ) ).Contains( wxS( "spaces" ) ) );
    BOOST_CHECK( KIGIT::BranchNameError( wxS( "-lead" ) ).Contains( wxS( "dash" ) ) );
    BOOST_CHECK( KIGIT::BranchNameError( wxS( "a..b" ) ).Contains( wxS( "dots" ) ) );
    BOOST_CHECK( KIGIT::BranchNameError( wxS( "trailing/" ) ).Contains( wxS( "slash" ) ) );

    // Caught by git itself rather than by a named rule.
    BOOST_CHECK( !KIGIT::BranchNameError( wxS( "with~tilde" ) ).IsEmpty() );
    BOOST_CHECK( !KIGIT::BranchNameError( wxS( "with:colon" ) ).IsEmpty() );
    BOOST_CHECK( !KIGIT::BranchNameError( wxS( "ends.lock" ) ).IsEmpty() );
}


// The check belongs to branch creation, not to the one dialog that prompts for a name.
BOOST_AUTO_TEST_CASE( CreateBranchRefusesAnInvalidName )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    BOOST_CHECK( !common.CreateBranch( wxS( "two words" ) ) );
    BOOST_CHECK( !common.CreateBranch( wxS( "a..b" ) ) );
    BOOST_CHECK( common.CreateBranch( wxS( "feature/ok" ) ) );
}

// Git stores commit text as opaque bytes. A repository written on Windows can hold ISO 8859-1,
// and decoding it strictly as UTF-8 would leave the whole row blank.
BOOST_AUTO_TEST_CASE( CommitTextThatIsNotUtf8StillReads )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    git_reference* head = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_head( &head, repo ), GIT_OK );
    KIGIT::GitReferencePtr headPtr( head );

    git_commit* parent = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &parent, repo, git_reference_target( head ) ), GIT_OK );
    KIGIT::GitCommitPtr parentPtr( parent );

    git_tree* tree = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_tree( &tree, parent ), GIT_OK );
    KIGIT::GitTreePtr treePtr( tree );

    // "Jorg" and "Cafe fix" with the accented letters as single Latin-1 bytes.
    const char* latin1Author = "J\xf6rg";
    const char* latin1Summary = "Caf\xe9 fix";

    git_signature* sig = nullptr;
    BOOST_REQUIRE_EQUAL( git_signature_now( &sig, latin1Author, "qa@example.com" ), GIT_OK );
    KIGIT::GitSignaturePtr sigPtr( sig );

    const git_commit* parents[1] = { parent };
    git_oid           oid;
    BOOST_REQUIRE_EQUAL( git_commit_create( &oid, repo, "HEAD", sig, sig, nullptr, latin1Summary, tree, 1, parents ),
                         GIT_OK );

    KIGIT_COMMON common( repo );

    std::vector<KIGIT_COMMIT_INFO> commits = common.GetRecentCommits( 1 );
    BOOST_REQUIRE_EQUAL( commits.size(), 1u );
    BOOST_CHECK_EQUAL( commits[0].summary.ToStdString(), wxString::FromUTF8( "Caf\xc3\xa9 fix" ).ToStdString() );
    BOOST_CHECK_EQUAL( commits[0].author.ToStdString(), wxString::FromUTF8( "J\xc3\xb6rg" ).ToStdString() );

    KIGIT_COMMIT_DETAIL detail = common.GetCommitDetail( commits[0].hash );
    BOOST_REQUIRE( detail.found );
    BOOST_CHECK( !detail.message.IsEmpty() );
    BOOST_CHECK( !detail.author.IsEmpty() );
}

// The History page can narrow the log to one branch or to a search text. With no branch named
// the walk covers every branch, so a commit on a side branch shows up too.
BOOST_AUTO_TEST_CASE( GetRecentCommitsFiltersByBranchAndSearch )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
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
    writeFile( TempPath(), "side.txt", "side\n" );
    commitWorkingFile( repo, "side.txt", "side work" );

    BOOST_REQUIRE_EQUAL( git_repository_set_head( repo, "refs/heads/main" ), GIT_OK );
    writeFile( TempPath(), "file.txt", "v2\n" );
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
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    git_oid firstOid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &firstOid, repo, "HEAD" ), GIT_OK );

    git_object* first = nullptr;
    BOOST_REQUIRE_EQUAL( git_object_lookup( &first, repo, &firstOid, GIT_OBJECT_COMMIT ), GIT_OK );
    KIGIT::GitObjectPtr firstPtr( first );

    git_oid tagOid;
    BOOST_REQUIRE_EQUAL( git_tag_create_lightweight( &tagOid, repo, "v1.0", first, 0 ), GIT_OK );

    writeFile( TempPath(), "file.txt", "v2\n" );
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
    git_repository*             repo = nullptr;
    git_repository_init_options opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
    opts.flags = GIT_REPOSITORY_INIT_MKPATH;
    opts.initial_head = "main";

    BOOST_REQUIRE_EQUAL( git_repository_init_ext( &repo, TempPath().utf8_string().c_str(), &opts ), GIT_OK );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    BOOST_REQUIRE( common.IsHeadUnborn() );
    BOOST_CHECK( common.GetBranches().empty() );
    BOOST_CHECK( !common.HasLocalCommits() );
    BOOST_CHECK( !common.HasUpstreamCommits() );
    BOOST_CHECK( !common.GetUnmergedCommitCount( "main" ) );
    BOOST_CHECK( !common.GetBranchDetail( "main" ).divergence );

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
    BOOST_CHECK( common.GetBranches().empty() );
}


// The History page shows a commit's whole message in its detail pane, summary line and body.
BOOST_AUTO_TEST_CASE( GetCommitDetailCarriesTheWholeMessage )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    writeFile( TempPath(), "file.txt", "v2\n" );
    commitWorkingFile( repo, "file.txt", "Route the power rails\n\nBody line one.\nBody line two.\n" );

    KIGIT_COMMON        common( repo );
    KIGIT_COMMIT_DETAIL detail = common.GetCommitDetail( common.GetRecentCommits( 1 )[0].hash );

    BOOST_REQUIRE( detail.found );
    BOOST_CHECK_EQUAL( detail.message.ToStdString(), "Route the power rails\n\nBody line one.\nBody line two." );
}


BOOST_AUTO_TEST_CASE( GetCommitDetailCountsLinesPerFile )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "one\ntwo\nthree\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    // Change one line of the existing file and add a new file in the same commit.
    writeFile( TempPath(), "file.txt", "one\n2\nthree\n" );
    writeFile( TempPath(), "new.txt", "a\nb\n" );

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
    git_repository*         repo = makeRepoWithCommit( TempPath(), "readme.txt", "root\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    BOOST_REQUIRE( wxFileName::Mkdir( TempPath() + wxS( "/mine" ) ) );
    BOOST_REQUIRE( wxFileName::Mkdir( TempPath() + wxS( "/theirs" ) ) );
    writeFile( TempPath(), wxS( "mine/board.txt" ), "one\ntwo\n" );
    writeFile( TempPath(), wxS( "theirs/board.txt" ), "one\ntwo\n" );

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
    writeFile( TempPath(), wxS( "mine/board.txt" ), "one\ntwo\nthree\n" );
    writeFile( TempPath(), wxS( "theirs/board.txt" ), "one\ntwo\na\nb\nc\n" );
    writeFile( TempPath(), wxS( "mine/~board.lck" ), "locked\n" );

    KIGIT_COMMON common( repo );

    std::pair<int, int> mine = common.GetWorkingTreeLineStats( { wxS( "mine/board.txt" ) } );
    BOOST_CHECK_EQUAL( mine.first, 1 );
    BOOST_CHECK_EQUAL( mine.second, 0 );

    // A path that did not change contributes nothing, and an unlisted one is not counted.
    std::pair<int, int> none = common.GetWorkingTreeLineStats( { wxS( "readme.txt" ) } );
    BOOST_CHECK_EQUAL( none.first, 0 );
    BOOST_CHECK_EQUAL( none.second, 0 );
}


#ifndef __WINDOWS__
// A project can be opened through a symlink pointing straight at its folder inside the
// repository. Stepping the visible path up one component then lands outside the repository
// altogether, so the folder has to be worked out from the resolved paths instead.
BOOST_AUTO_TEST_CASE( SymlinkedProjectFolderStillFindsItsPrefix )
{
    wxString repoRoot = TempPath() + wxS( "/real/repo" );
    BOOST_REQUIRE( wxFileName::Mkdir( repoRoot + wxS( "/proj" ), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );
    writeFile( repoRoot, wxS( "proj/p.kicad_pro" ), "{}\n" );

    wxString deepLink = TempPath() + wxS( "/deep" );
    BOOST_REQUIRE_EQUAL( symlink( ( repoRoot + wxS( "/proj" ) ).utf8_string().c_str(), deepLink.utf8_string().c_str() ),
                         0 );

    wxString rootLink = TempPath() + wxS( "/top" );
    BOOST_REQUIRE_EQUAL( symlink( repoRoot.utf8_string().c_str(), rootLink.utf8_string().c_str() ), 0 );

    const wxString canonicalWorkDir = repoRoot + wxS( "/" );

    // Pointed at the project folder there is no visible path for the repository root, so the
    // resolved one is used.
    wxString deepProject = deepLink + wxS( "/" );
    wxString deepWorkDir = KIGIT::PROJECT_GIT_UTILS::ComputeSymlinkPreservingWorkDir( deepProject, canonicalWorkDir );
    BOOST_CHECK_EQUAL( deepWorkDir.ToStdString(), canonicalWorkDir.ToStdString() );
    std::optional<wxString> deepPrefix = KIGIT::PROJECT_GIT_UTILS::ProjectPrefix( deepWorkDir, deepProject );
    BOOST_REQUIRE( deepPrefix );
    BOOST_CHECK_EQUAL( deepPrefix->ToStdString(), "proj/" );
    BOOST_CHECK( KIGIT::PROJECT_GIT_UTILS::IsWithinProjectPath( repoRoot + wxS( "/proj/p.kicad_pro" ), deepProject ) );
    BOOST_CHECK( !KIGIT::PROJECT_GIT_UTILS::IsWithinProjectPath( repoRoot + wxS( "/elsewhere/x.txt" ), deepProject ) );

    // Pointed at the repository root the visible path still stands for it, so it is kept.
    wxString topProject = rootLink + wxS( "/proj/" );
    wxString topWorkDir = KIGIT::PROJECT_GIT_UTILS::ComputeSymlinkPreservingWorkDir( topProject, canonicalWorkDir );
    BOOST_CHECK_EQUAL( topWorkDir.ToStdString(), ( rootLink + wxS( "/" ) ).ToStdString() );
    std::optional<wxString> topPrefix = KIGIT::PROJECT_GIT_UTILS::ProjectPrefix( topWorkDir, topProject );
    BOOST_REQUIRE( topPrefix );
    BOOST_CHECK_EQUAL( topPrefix->ToStdString(), "proj/" );
}
#endif


// WriteBlobToFile materializes a file's committed content, used to feed the diff viewer.
BOOST_AUTO_TEST_CASE( WriteBlobToFileExtractsCommittedContent )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "hello world\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    wxString dest = TempPath() + wxFileName::GetPathSeparator() + wxS( "extracted.txt" );

    BOOST_CHECK( KIGIT::PROJECT_GIT_UTILS::WriteBlobToFile( repo, wxS( "HEAD" ), wxS( "file.txt" ), dest ) );

    wxFile   in( dest );
    wxString content;
    BOOST_REQUIRE( in.IsOpened() );
    in.ReadAll( &content );
    BOOST_CHECK_EQUAL( content.ToStdString(), "hello world\n" );

    // A path that is not in the tree returns false and leaves the destination alone.
    BOOST_CHECK( !KIGIT::PROJECT_GIT_UTILS::WriteBlobToFile( repo, wxS( "HEAD" ), wxS( "missing.txt" ), dest ) );
    BOOST_CHECK_EQUAL( readWorkingFile( wxFileName( dest ) ), "hello world\n" );
}


BOOST_AUTO_TEST_CASE( DiffWorkspaceKeepsExtractionsSeparateAndRemovesThem )
{
    KIGIT::GitRepositoryPtr repo( makeRepoWithCommit( TempPath(), "board.kicad_pcb", "board version\n" ) );
    writeFile( TempPath(), "schematics/root.kicad_sch", "root sheet\n" );
    commitWorkingFile( repo.get(), "schematics/root.kicad_sch", "add root sheet" );
    writeFile( TempPath(), "schematics/child.kicad_sch", "child sheet\n" );
    commitWorkingFile( repo.get(), "schematics/child.kicad_sch", "add child sheet" );

    wxString root;
    wxString otherRoot;
    wxString boardFile;
    wxString schematicFile;

    {
        GIT_DIFF_WORKSPACE workspace;
        BOOST_REQUIRE( workspace.IsOk() );
        root = workspace.Root();
        boardFile = workspace.Materialize( repo.get(), "HEAD", "board.kicad_pcb", KICAD_DIFF::DOC_KIND::PCB );
        BOOST_REQUIRE( !boardFile.IsEmpty() );
        BOOST_CHECK_EQUAL( readWorkingFile( wxFileName( boardFile ) ), "board version\n" );

        {
            GIT_DIFF_WORKSPACE other;
            BOOST_REQUIRE( other.IsOk() );
            otherRoot = other.Root();
            BOOST_CHECK( root != otherRoot );
            wxString otherBoard = other.Materialize( repo.get(), "HEAD", "board.kicad_pcb", KICAD_DIFF::DOC_KIND::PCB );
            BOOST_REQUIRE( !otherBoard.IsEmpty() );
            BOOST_CHECK( boardFile != otherBoard );
            BOOST_CHECK_EQUAL( readWorkingFile( wxFileName( otherBoard ) ), "board version\n" );
        }

        BOOST_CHECK( !wxFileName::DirExists( otherRoot ) );
        BOOST_CHECK_EQUAL( readWorkingFile( wxFileName( boardFile ) ), "board version\n" );
        BOOST_CHECK( workspace.Materialize( repo.get(), "missing-ref", "board.kicad_pcb", KICAD_DIFF::DOC_KIND::PCB )
                             .IsEmpty() );

        // A schematic extraction includes the child sheets beside its root sheet.
        schematicFile =
                workspace.Materialize( repo.get(), "HEAD", "schematics/root.kicad_sch", KICAD_DIFF::DOC_KIND::SCH );
        BOOST_REQUIRE( !schematicFile.IsEmpty() );
        BOOST_CHECK_EQUAL( readWorkingFile( wxFileName( schematicFile ) ), "root sheet\n" );
        wxFileName childFile( wxFileName( schematicFile ).GetPath(), wxS( "child.kicad_sch" ) );
        BOOST_CHECK_EQUAL( readWorkingFile( childFile ), "child sheet\n" );
    }

    BOOST_CHECK( !wxFileName::DirExists( root ) );
    BOOST_CHECK( !wxFileName::FileExists( boardFile ) );
    BOOST_CHECK( !wxFileName::FileExists( schematicFile ) );
}


// GetTags resolves each tag to the commit it points at.
BOOST_AUTO_TEST_CASE( GetTagsResolvesTagToCommit )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
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
    BOOST_CHECK_EQUAL( tags[0].hash, common.GetRecentCommits( 1 )[0].hash );
}


// GetStashes lists the repository's stashes.
BOOST_AUTO_TEST_CASE( GetStashesListsStash )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_CHECK( common.GetStashes().empty() );

    // Dirty the working tree so there is something to stash.
    {
        wxFileName    fn( TempPath(), wxS( "file.txt" ) );
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
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    auto dirtyAndStash = [&]( const char* aContent, const wxString& aMessage )
    {
        wxFileName fn( TempPath(), wxS( "file.txt" ) );

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
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    writeFile( TempPath(), "file.txt", "v2\n" );
    BOOST_REQUIRE( common.StashSave( wxT( "set aside for pull" ) ) );

    wxFileName fn( TempPath(), wxS( "file.txt" ) );
    BOOST_CHECK_EQUAL( readWorkingFile( fn ), "v1\n" );

    std::vector<KIGIT_STASH_INFO> stashes = common.GetStashes();
    BOOST_REQUIRE_EQUAL( stashes.size(), 1u );
    BOOST_CHECK( stashes[0].message.Contains( "set aside for pull" ) );

    BOOST_CHECK( common.StashPop( 0 ) );
    BOOST_CHECK_EQUAL( readWorkingFile( fn ), "v2\n" );
    BOOST_CHECK( common.GetStashes().empty() );
}


// CreateBranch and DeleteBranch add and remove a local branch.
BOOST_AUTO_TEST_CASE( CreateAndDeleteBranch )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    std::vector<KIGIT_BRANCH_INFO> branches = common.GetBranches();
    BOOST_REQUIRE_EQUAL( branches.size(), 1u );
    wxString original = branches[0].name;

    BOOST_CHECK( common.CreateBranch( "feature" ) );
    branches = common.GetBranches();
    BOOST_REQUIRE_EQUAL( branches.size(), 2u );
    BOOST_CHECK( std::any_of( branches.begin(), branches.end(),
                              []( const KIGIT_BRANCH_INFO& aBranch )
                              {
                                  return aBranch.name == wxS( "feature" ) && !aBranch.isRemote;
                              } ) );

    BOOST_CHECK( common.DeleteBranch( "feature" ) );
    branches = common.GetBranches();
    BOOST_REQUIRE_EQUAL( branches.size(), 1u );
    BOOST_CHECK_EQUAL( branches[0].name, original );
}


// CreateTag and DeleteTag add and remove a lightweight tag.
BOOST_AUTO_TEST_CASE( CreateAndDeleteTag )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
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
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    BOOST_CHECK( common.SetUserConfig( "New Name", "new@example.com" ) );

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


BOOST_AUTO_TEST_CASE( GitIdentityUsesRepositoryValuesBeforeCustomDefaults )
{
    KIGIT::GitRepositoryPtr repo =
            KI_TEST::InitGitRepository( TempPath(), "main", "Repository Author", "repository@example.com" );
    KIGIT_COMMON common( repo.get() );
    auto         lock = common.LockAction();

    git_config* config = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_config( &config, repo.get() ), GIT_OK );
    KIGIT::GitConfigPtr configPtr( config );

    // Keep the user's global Git identity out of this test.
    git_config* local = nullptr;
    BOOST_REQUIRE_EQUAL( git_config_open_level( &local, config, GIT_CONFIG_LEVEL_LOCAL ), GIT_OK );
    KIGIT::GitConfigPtr localPtr( local );
    BOOST_REQUIRE_EQUAL( git_repository_set_config( repo.get(), local ), GIT_OK );

    COMMON_SETTINGS::GIT preferences{};
    preferences.useDefaultAuthor = false;
    preferences.authorName = wxS( "Custom Author" );
    preferences.authorEmail = wxS( "custom@example.com" );

    KIGIT::GIT_IDENTITY identity = KIGIT::ReadGitIdentity( common );
    BOOST_CHECK( identity.configured );
    GitUserConfig resolved = KIGIT::ResolveGitIdentity( identity, preferences, wxS( "OS User" ) );
    BOOST_CHECK_EQUAL( resolved.authorName, wxS( "Repository Author" ) );
    BOOST_CHECK_EQUAL( resolved.authorEmail, wxS( "repository@example.com" ) );

    BOOST_REQUIRE_EQUAL( git_config_delete_entry( local, "user.email" ), GIT_OK );
    identity = KIGIT::ReadGitIdentity( common );
    BOOST_CHECK( !identity.configured );
    resolved = KIGIT::ResolveGitIdentity( identity, preferences, wxS( "OS User" ) );
    BOOST_CHECK_EQUAL( resolved.authorName, wxS( "Repository Author" ) );
    BOOST_CHECK_EQUAL( resolved.authorEmail, wxS( "custom@example.com" ) );

    BOOST_REQUIRE_EQUAL( git_config_delete_entry( local, "user.name" ), GIT_OK );
    identity = KIGIT::ReadGitIdentity( common );
    BOOST_CHECK( !identity.configured );
    resolved = KIGIT::ResolveGitIdentity( identity, preferences, wxS( "OS User" ) );
    BOOST_CHECK_EQUAL( resolved.authorName, wxS( "Custom Author" ) );
    BOOST_CHECK_EQUAL( resolved.authorEmail, wxS( "custom@example.com" ) );

    // Default mode must not revive custom values left in Preferences.
    preferences.useDefaultAuthor = true;
    resolved = KIGIT::ResolveGitIdentity( identity, preferences, wxS( "OS User" ) );
    BOOST_CHECK_EQUAL( resolved.authorName, wxS( "OS User" ) );
    BOOST_CHECK( resolved.authorEmail.IsEmpty() );

    preferences.useDefaultAuthor = false;
    preferences.authorName.clear();
    resolved = KIGIT::ResolveGitIdentity( identity, preferences, wxS( "OS User" ) );
    BOOST_CHECK_EQUAL( resolved.authorName, wxS( "OS User" ) );
    BOOST_CHECK_EQUAL( resolved.authorEmail, wxS( "custom@example.com" ) );
}


// StashDrop removes a stash from the list.
BOOST_AUTO_TEST_CASE( StashDropRemovesStash )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    writeFile( TempPath(), "file.txt", "v2\n" );
    BOOST_REQUIRE( common.StashSave( wxS( "work in progress" ) ) );

    BOOST_REQUIRE_EQUAL( common.GetStashes().size(), 1u );
    BOOST_CHECK( common.StashDrop( 0 ) );
    BOOST_CHECK( common.GetStashes().empty() );
}


// Apply leaves the stash in the list. Pop removes it.
BOOST_AUTO_TEST_CASE( StashApplyKeepsAndStashPopRemoves )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    writeFile( TempPath(), "file.txt", "v2\n" );
    BOOST_REQUIRE( common.StashSave( wxS( "wip" ) ) );
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
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );
    KIGIT_COMMON            common( repo );

    wxFileName fn( TempPath(), wxS( "file.txt" ) );

    writeFile( TempPath(), "file.txt", "stashed\n" );
    BOOST_REQUIRE( common.StashSave( wxS( "wip" ) ) );
    BOOST_REQUIRE_EQUAL( common.GetStashes().size(), 1u );

    // A different change to the same line, committed, so the stash no longer applies cleanly.
    writeFile( TempPath(), "file.txt", "committed\n" );
    commitWorkingFile( repo, wxS( "file.txt" ), "conflicting change" );

    BOOST_CHECK( !common.StashPop( 0 ) );
    BOOST_CHECK_EQUAL( common.GetStashes().size(), 1u );
    BOOST_CHECK_EQUAL( readWorkingFile( fn ), "committed\n" );

    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    KIGIT::GitIndexPtr indexPtr( index );
    BOOST_CHECK( !git_index_has_conflicts( index ) );
}


// Whether the project rebases when getting changes is read with git's own bool parser, so every
// spelling git accepts counts.
BOOST_AUTO_TEST_CASE( PullRebaseConfigIsReadAsGitReadsIt )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    // Unset means merge, as git does.
    BOOST_CHECK( !common.IsPullRebaseConfigured() );

    auto setRebase = [repo]( const char* aValue )
    {
        git_config* cfg = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_config( &cfg, repo ), GIT_OK );
        KIGIT::GitConfigPtr cfgPtr( cfg );
        BOOST_REQUIRE_EQUAL( git_config_set_string( cfg, "pull.rebase", aValue ), GIT_OK );
    };

    for( const char* yes : { "true", "yes", "on", "1" } )
    {
        setRebase( yes );
        BOOST_CHECK_MESSAGE( common.IsPullRebaseConfigured(), std::string( "pull.rebase=" ) + yes );
    }

    for( const char* no : { "false", "no", "off", "0" } )
    {
        setRebase( no );
        BOOST_CHECK_MESSAGE( !common.IsPullRebaseConfigured(), std::string( "pull.rebase=" ) + no );
    }
}


// A detached HEAD is on no branch, so it has no upstream. That missing upstream must not read
// as unpushed work, while a branch without one still does.
BOOST_AUTO_TEST_CASE( DetachedHeadHasNoLocalCommits )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_REQUIRE( common.CreateTag( "v1.0" ) );

    // On a branch with no upstream the commit really is unpublished.
    BOOST_REQUIRE( !common.IsHeadDetached() );
    BOOST_CHECK( common.HasLocalCommits() );
    BOOST_CHECK( !common.HasUpstreamCommits() );

    BOOST_REQUIRE( common.CheckoutTag( "v1.0" ) );
    BOOST_REQUIRE( common.IsHeadDetached() );
    BOOST_CHECK( !common.HasLocalCommits() );
    BOOST_CHECK( !common.HasUpstreamCommits() );

    std::optional<size_t> count = common.GetUnmergedCommitCount( "main" );
    BOOST_REQUIRE( count );
    BOOST_CHECK_EQUAL( *count, 0u );
}


// CheckoutTag puts the working tree at the tagged commit with HEAD detached.
BOOST_AUTO_TEST_CASE( CheckoutTagDetachesHead )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_REQUIRE( common.CreateTag( "v1.0" ) );

    git_oid tagged;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &tagged, repo, "HEAD" ), GIT_OK );

    // A later commit, so the checkout has to move both the tree and HEAD back.
    writeFile( TempPath(), "file.txt", "v2\n" );
    commitWorkingFile( repo, "file.txt", "second" );

    BOOST_CHECK( !common.IsHeadDetached() );
    BOOST_CHECK( common.CheckoutTag( "v1.0" ) );
    BOOST_CHECK( common.IsHeadDetached() );

    git_oid headNow;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headNow, repo, "HEAD" ), GIT_OK );
    BOOST_CHECK( git_oid_equal( &headNow, &tagged ) );
    BOOST_CHECK_EQUAL( readWorkingFile( wxFileName( TempPath(), "file.txt" ) ), "v1\n" );
}


BOOST_AUTO_TEST_CASE( UnmergedCommitCountIsWhatHeadDoesNotContain )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );
    KIGIT_COMMON            common( repo );

    BOOST_REQUIRE( common.CreateBranch( "feature" ) );
    std::optional<size_t> count = common.GetUnmergedCommitCount( "feature" );
    BOOST_REQUIRE( count );
    BOOST_CHECK_EQUAL( *count, 0u );

    BOOST_REQUIRE_EQUAL( git_repository_set_head( repo, "refs/heads/feature" ), GIT_OK );

    wxFileName fn( TempPath(), wxS( "file.txt" ) );
    {
        std::ofstream f( fn.GetFullPath().utf8_string() );
        f << "v2\n";
    }

    commitWorkingFile( repo, wxS( "file.txt" ), "only on feature" );
    BOOST_REQUIRE_EQUAL( git_repository_set_head( repo, "refs/heads/main" ), GIT_OK );

    count = common.GetUnmergedCommitCount( "feature" );
    BOOST_REQUIRE( count );
    BOOST_CHECK_EQUAL( *count, 1u );
    count = common.GetUnmergedCommitCount( "main" );
    BOOST_REQUIRE( count );
    BOOST_CHECK_EQUAL( *count, 0u );
    BOOST_CHECK( !common.GetUnmergedCommitCount( "no-such-branch" ) );
}


// Remotes round-trip without returning embedded user credentials.
BOOST_AUTO_TEST_CASE( RemoteRoundTripAndUserinfoStripped )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_CHECK( common.GetRemotes().empty() );

    BOOST_CHECK( common.AddRemote( "origin", "https://example.com/x.git" ) );

    std::vector<std::pair<wxString, wxString>> remotes = common.GetRemotes();
    BOOST_REQUIRE_EQUAL( remotes.size(), 1u );
    BOOST_CHECK_EQUAL( remotes[0].first.ToStdString(), "origin" );
    BOOST_CHECK_EQUAL( remotes[0].second.ToStdString(), "https://example.com/x.git" );

    BOOST_CHECK( common.SetRemoteUrl( "origin", "https://u:secret@example.com/y.git" ) );

    // The password must not surface through GetRemotes.
    BOOST_CHECK_EQUAL( common.GetRemotes()[0].second.ToStdString(), "https://example.com/y.git" );

    BOOST_CHECK( common.RemoveRemote( "origin" ) );
    BOOST_CHECK( common.GetRemotes().empty() );
}


// Branches created at the same HEAD share a timestamp. A std::map keyed on it would drop one.
BOOST_AUTO_TEST_CASE( GetBranchesKeepsBranchesSharingACommitTime )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_REQUIRE( common.CreateBranch( "alpha" ) );
    BOOST_REQUIRE( common.CreateBranch( "beta" ) );

    std::vector<KIGIT_BRANCH_INFO> branches = common.GetBranches();
    BOOST_REQUIRE_EQUAL( branches.size(), 3u );
    std::set<wxString> names;

    for( const KIGIT_BRANCH_INFO& branch : branches )
    {
        names.insert( branch.name );
        BOOST_CHECK( !branch.isRemote );
        BOOST_CHECK( branch.when == branches.front().when );
    }

    BOOST_CHECK_EQUAL( names.count( wxS( "main" ) ), 1u );
    BOOST_CHECK_EQUAL( names.count( wxS( "alpha" ) ), 1u );
    BOOST_CHECK_EQUAL( names.count( wxS( "beta" ) ), 1u );
}


// Branch listings carry the tip commit and identify the current branch.
BOOST_AUTO_TEST_CASE( GetBranchesCarriesTipAndCurrent )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_REQUIRE( common.CreateBranch( "older" ) );

    writeFile( TempPath(), "file.txt", "v2\n" );
    const char* message = "second\n\nA full commit message.\n";
    commitWorkingFile( repo, "file.txt", message );

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

    for( const KIGIT_BRANCH_INFO& branch : branches )
    {
        BOOST_CHECK( !branch.isRemote );
        BOOST_CHECK( branch.when.IsValid() );

        if( branch.name == wxS( "main" ) )
        {
            BOOST_CHECK( branch.isCurrent );
            BOOST_CHECK_EQUAL( branch.tipHash.ToStdString(), hashOf( "second" ) );
            BOOST_CHECK_EQUAL( branch.message.ToStdString(), message );
        }
        else
        {
            BOOST_CHECK_EQUAL( branch.name.ToStdString(), "older" );
            BOOST_CHECK( !branch.isCurrent );
            BOOST_CHECK_EQUAL( branch.tipHash.ToStdString(), hashOf( "initial" ) );
            BOOST_CHECK_EQUAL( branch.message.ToStdString(), "initial" );
        }
    }
}


BOOST_AUTO_TEST_CASE( GetBranchesDistinguishesRemoteNamesAndSkipsSymbolicAliases )
{
    KIGIT::GitRepositoryPtr repo( makeRepoWithCommit( TempPath(), "file.txt", "v1\n" ) );
    KIGIT_COMMON            common( repo.get() );
    BOOST_REQUIRE( common.CreateBranch( "origin/topic" ) );

    git_oid headOid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headOid, repo.get(), "HEAD" ), GIT_OK );
    git_reference* remote = nullptr;
    BOOST_REQUIRE_EQUAL( git_reference_create( &remote, repo.get(), "refs/remotes/origin/topic", &headOid, 0, nullptr ),
                         GIT_OK );
    KIGIT::GitReferencePtr remotePtr( remote );

    git_reference* alias = nullptr;
    BOOST_REQUIRE_EQUAL( git_reference_symbolic_create( &alias, repo.get(), "refs/remotes/origin/HEAD",
                                                        "refs/remotes/origin/topic", 0, nullptr ),
                         GIT_OK );
    KIGIT::GitReferencePtr aliasPtr( alias );

    std::vector<KIGIT_BRANCH_INFO> branches = common.GetBranches();
    BOOST_REQUIRE_EQUAL( branches.size(), 3u );
    unsigned int localCount = 0;
    unsigned int remoteCount = 0;

    for( const KIGIT_BRANCH_INFO& branch : branches )
    {
        if( branch.name == wxS( "origin/topic" ) )
        {
            if( branch.isRemote )
                ++remoteCount;
            else
                ++localCount;

            BOOST_CHECK( !branch.isCurrent );
        }
        else
        {
            BOOST_CHECK_EQUAL( branch.name.ToStdString(), "main" );
            BOOST_CHECK( branch.isCurrent );
            BOOST_CHECK( !branch.isRemote );
        }
    }

    BOOST_CHECK_EQUAL( localCount, 1u );
    BOOST_CHECK_EQUAL( remoteCount, 1u );

    BOOST_REQUIRE_EQUAL( git_repository_set_head_detached( repo.get(), &headOid ), GIT_OK );
    branches = common.GetBranches();
    BOOST_REQUIRE_EQUAL( branches.size(), 3u );
    BOOST_CHECK( std::none_of( branches.begin(), branches.end(),
                               []( const KIGIT_BRANCH_INFO& aBranch )
                               {
                                   return aBranch.isCurrent;
                               } ) );
}


BOOST_AUTO_TEST_CASE( SortBranchesForDisplayGroupsBranchesAndBreaksTimeTies )
{
    const wxDateTime older( 1, wxDateTime::Jan, 2020 );
    const wxDateTime newer( 2, wxDateTime::Jan, 2020 );

    auto branch = []( const wxString& aName, const wxDateTime& aWhen, bool aRemote = false, bool aCurrent = false )
    {
        KIGIT_BRANCH_INFO info;
        info.name = aName;
        info.when = aWhen;
        info.isRemote = aRemote;
        info.isCurrent = aCurrent;
        return info;
    };

    std::vector<KIGIT_BRANCH_INFO> branches = {
        branch( "origin/unknown", wxDateTime(), true ), branch( "b-tie", older ),
        branch( "origin/z-new", newer, true ),          branch( "a-undated", wxDateTime() ),
        branch( "origin/a-old", older, true ),          branch( "z-new", newer ),
        branch( "main", wxDateTime(), false, true ),    branch( "a-tie", older )
    };

    KIGIT::SortBranchesForDisplay( branches );

    const std::vector<wxString> expected = { "main",      "z-new",        "a-tie",        "b-tie",
                                             "a-undated", "origin/z-new", "origin/a-old", "origin/unknown" };

    BOOST_REQUIRE_EQUAL( branches.size(), expected.size() );

    for( size_t i = 0; i < expected.size(); ++i )
        BOOST_CHECK_EQUAL( branches[i].name.ToStdString(), expected[i].ToStdString() );
}


// The detail pane shows a branch's tip commit and how far it is from the branch it tracks.
BOOST_AUTO_TEST_CASE( GetBranchDetailReportsTipAndUpstream )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_REQUIRE( common.CreateBranch( "base" ) );

    writeFile( TempPath(), "file.txt", "v2\n" );
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
    BOOST_REQUIRE( detail.divergence );
    BOOST_CHECK_EQUAL( detail.divergence->ahead, 1u );
    BOOST_CHECK_EQUAL( detail.divergence->behind, 0u );

    KIGIT_BRANCH_DETAIL base = common.GetBranchDetail( wxS( "base" ) );
    BOOST_REQUIRE( base.found );
    BOOST_CHECK( base.upstream.IsEmpty() );
    BOOST_CHECK( !base.divergence );
    BOOST_CHECK_EQUAL( base.message.ToStdString(), "initial" );

    BOOST_CHECK( !common.GetBranchDetail( wxS( "nowhere" ) ).found );
}


BOOST_AUTO_TEST_CASE( BranchComparisonsUseTheSelectedBranchAndItsOwnUpstream )
{
    KIGIT::GitRepositoryPtr repo( makeRepoWithCommit( TempPath(), "file.txt", "initial\n" ) );
    KIGIT_COMMON            common( repo.get() );
    BOOST_REQUIRE( common.CreateBranch( "selected" ) );
    BOOST_REQUIRE( common.CreateBranch( "main-upstream" ) );

    auto commitOn = [&]( const char* aBranch, const char* aMessage )
    {
        wxString ref = wxS( "refs/heads/" ) + wxString::FromUTF8( aBranch );
        BOOST_REQUIRE_EQUAL( git_repository_set_head( repo.get(), ref.utf8_str() ), GIT_OK );
        writeFile( TempPath(), "file.txt", aMessage );
        commitWorkingFile( repo.get(), "file.txt", aMessage );
    };

    commitOn( "selected", "selected one" );
    BOOST_REQUIRE( common.CreateBranch( "selected-upstream" ) );
    commitOn( "selected", "selected two" );
    commitOn( "selected-upstream", "selected upstream one" );
    commitOn( "main-upstream", "main upstream one" );
    commitOn( "main-upstream", "main upstream two" );
    commitOn( "main-upstream", "main upstream three" );
    BOOST_REQUIRE_EQUAL( git_repository_set_head( repo.get(), "refs/heads/main" ), GIT_OK );

    auto setUpstream = [&]( const char* aBranch, const char* aUpstream )
    {
        git_reference* ref = nullptr;
        BOOST_REQUIRE_EQUAL( git_branch_lookup( &ref, repo.get(), aBranch, GIT_BRANCH_LOCAL ), GIT_OK );
        KIGIT::GitReferencePtr refPtr( ref );
        BOOST_REQUIRE_EQUAL( git_branch_set_upstream( ref, aUpstream ), GIT_OK );
    };

    setUpstream( "main", "main-upstream" );
    setUpstream( "selected", "selected-upstream" );

    // The selected branch and its upstream each have one new commit.
    KIGIT_BRANCH_DETAIL selected = common.GetBranchDetail( "selected" );
    BOOST_REQUIRE( selected.found );
    BOOST_CHECK_EQUAL( selected.upstream.ToStdString(), "selected-upstream" );
    BOOST_REQUIRE( selected.divergence );
    BOOST_CHECK_EQUAL( selected.divergence->ahead, 1u );
    BOOST_CHECK_EQUAL( selected.divergence->behind, 1u );

    // Both selected branch commits are missing from HEAD.
    std::optional<size_t> count = common.GetUnmergedCommitCount( "selected" );
    BOOST_REQUIRE( count );
    BOOST_CHECK_EQUAL( *count, 2u );

    KIGIT_BRANCH_DETAIL main = common.GetBranchDetail( "main" );
    BOOST_REQUIRE( main.divergence );
    BOOST_CHECK_EQUAL( main.divergence->ahead, 0u );
    BOOST_CHECK_EQUAL( main.divergence->behind, 3u );
    BOOST_CHECK( !common.HasLocalCommits() );
    BOOST_CHECK( common.HasUpstreamCommits() );

    commitOn( "main", "local main one" );
    main = common.GetBranchDetail( "main" );
    BOOST_REQUIRE( main.divergence );
    BOOST_CHECK_EQUAL( main.divergence->ahead, 1u );
    BOOST_CHECK_EQUAL( main.divergence->behind, 3u );
    BOOST_CHECK( common.HasLocalCommits() );
    BOOST_CHECK( common.HasUpstreamCommits() );
}


BOOST_AUTO_TEST_CASE( BranchComparisonsStayUnknownWhenReferencesCannotBeRead )
{
    KIGIT_COMMON unavailable( nullptr );
    BOOST_CHECK( !unavailable.HasLocalCommits() );
    BOOST_CHECK( !unavailable.HasUpstreamCommits() );
    BOOST_CHECK( !unavailable.GetUnmergedCommitCount( "main" ) );
    BOOST_CHECK( !unavailable.GetBranchDetail( "main" ).divergence );

    KIGIT::GitRepositoryPtr repo( makeRepoWithCommit( TempPath(), "file.txt", "initial\n" ) );
    KIGIT_COMMON            common( repo.get() );

    // The reference exists, but its commit is missing.
    writeFile( TempPath(), ".git/refs/heads/broken", "1111111111111111111111111111111111111111\n" );
    git_config* config = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_config( &config, repo.get() ), GIT_OK );
    KIGIT::GitConfigPtr configPtr( config );
    BOOST_REQUIRE_EQUAL( git_config_set_string( config, "branch.main.remote", "." ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_config_set_string( config, "branch.main.merge", "refs/heads/broken" ), GIT_OK );

    KIGIT_BRANCH_DETAIL detail = common.GetBranchDetail( "main" );
    BOOST_REQUIRE( detail.found );
    BOOST_CHECK_EQUAL( detail.upstream.ToStdString(), "broken" );
    BOOST_CHECK( !detail.divergence );
    BOOST_CHECK( !common.HasLocalCommits() );
    BOOST_CHECK( !common.HasUpstreamCommits() );
    BOOST_CHECK( !common.GetUnmergedCommitCount( "broken" ) );
}


// New Branch in the detail pane starts the branch where the selected one is, not at HEAD.
BOOST_AUTO_TEST_CASE( CreateBranchStartsAtTheGivenBranch )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_REQUIRE( common.CreateBranch( "base" ) );

    writeFile( TempPath(), "file.txt", "v2\n" );
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
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_CHECK( !common.GetLastFetchTime().IsValid() );

    wxFileName fetchHead( wxString::FromUTF8( git_repository_path( repo ) ), wxS( "FETCH_HEAD" ) );

    {
        std::ofstream f( fetchHead.GetFullPath().utf8_string() );
        f << "0000000000000000000000000000000000000000\t\tbranch 'main' of https://example.com/x.git\n";
    }

    // The time is the file's own, so set it to something that is not now.
    wxDateTime stamp( 1, wxDateTime::Jan, 2020, 12, 0, 0 );
    BOOST_REQUIRE( fetchHead.SetTimes( nullptr, &stamp, nullptr ) );

    wxDateTime when = common.GetLastFetchTime();
    BOOST_REQUIRE( when.IsValid() );
    BOOST_CHECK( when.IsEqualTo( stamp ) );
}


// The Changes page sums the lines the working tree adds and removes, a new file counting as
// all added.
BOOST_AUTO_TEST_CASE( GetWorkingTreeLineStatsCountsEditsAndNewFiles )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "one\ntwo\nthree\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    std::pair<int, int> clean = common.GetWorkingTreeLineStats( { wxS( "file.txt" ) } );
    BOOST_CHECK_EQUAL( clean.first, 0 );
    BOOST_CHECK_EQUAL( clean.second, 0 );

    writeFile( TempPath(), "file.txt", "one\n2\nthree\n" );
    writeFile( TempPath(), "new.txt", "a\nb\n" );

    std::pair<int, int> dirty = common.GetWorkingTreeLineStats( { wxS( "file.txt" ), wxS( "new.txt" ) } );
    BOOST_CHECK_EQUAL( dirty.first, 3 );
    BOOST_CHECK_EQUAL( dirty.second, 1 );
}


// The detail pane shows what a stash holds: the branch and commit it was taken on, its message
// and the files it changes, a new file included.
BOOST_AUTO_TEST_CASE( GetStashDetailNamesBranchCommitAndFiles )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );

    std::vector<KIGIT_COMMIT_INFO> commits = common.GetRecentCommits( 1 );
    BOOST_REQUIRE_EQUAL( commits.size(), 1u );

    writeFile( TempPath(), "file.txt", "v2\n" );
    writeFile( TempPath(), "new.txt", "n\n" );

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


// The History page can be narrowed to one file or folder. A commit stays listed when the tree
// entry at that path differs from its parent's, so a folder catches changes anywhere under it.
BOOST_AUTO_TEST_CASE( GetRecentCommitsFiltersByPath )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    wxFileName::Mkdir( TempPath() + wxS( "/sub" ) );
    writeFile( TempPath(), "sub/a.txt", "a\n" );
    commitWorkingFile( repo, "sub/a.txt", "add a" );
    writeFile( TempPath(), "file.txt", "v2\n" );
    commitWorkingFile( repo, "file.txt", "edit file" );
    writeFile( TempPath(), "sub/b.txt", "b\n" );
    commitWorkingFile( repo, "sub/b.txt", "add b" );

    KIGIT_COMMON         common( repo );
    KIGIT_HISTORY_FILTER filter;

    filter.path = wxS( "file.txt" );
    std::vector<KIGIT_COMMIT_INFO> commits = common.GetRecentCommits( 50, filter );
    BOOST_REQUIRE_EQUAL( commits.size(), 2 );
    BOOST_CHECK_EQUAL( commits[0].summary, wxS( "edit file" ) );
    BOOST_CHECK_EQUAL( commits[1].summary, wxS( "initial" ) );

    filter.path = wxS( "sub" );
    commits = common.GetRecentCommits( 50, filter );
    BOOST_REQUIRE_EQUAL( commits.size(), 2 );
    BOOST_CHECK_EQUAL( commits[0].summary, wxS( "add b" ) );
    BOOST_CHECK_EQUAL( commits[1].summary, wxS( "add a" ) );

    filter.path = wxS( "sub/a.txt" );
    BOOST_CHECK_EQUAL( common.GetRecentCommits( 50, filter ).size(), 1 );

    filter.path = wxS( "nowhere.txt" );
    BOOST_CHECK( common.GetRecentCommits( 50, filter ).empty() );

    // The search still applies on top of the path.
    filter.path = wxS( "sub" );
    filter.search = wxS( "add b" );
    BOOST_CHECK_EQUAL( common.GetRecentCommits( 50, filter ).size(), 1 );
}


// The Tags page shows a tag's message, author and date. An annotated tag has its own, a
// lightweight tag borrows the commit's.
// libgit2 runs no filter program, so a file git routes through LFS has to be recognised before
// an operation writes the placeholder text over it.
BOOST_AUTO_TEST_CASE( ProjectUsesGitLfsFindsAFilteredFile )
{
    using KIGIT::PROJECT_GIT_UTILS;


    git_repository*         repo = makeRepoWithCommit( TempPath(), "board.kicad_pcb", "(kicad_pcb)\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );
    KIGIT_COMMON            common( repo, TempPath() );

    BOOST_CHECK( !PROJECT_GIT_UTILS::ProjectUsesGitLfs( common ) );

    writeFile( TempPath(), ".gitattributes", "*.step filter=lfs diff=lfs merge=lfs -text\n" );
    commitWorkingFile( repo, ".gitattributes", "route step files through lfs" );
    writeFile( TempPath(), "case.step", "ISO-10303-21;\n" );
    commitWorkingFile( repo, "case.step", "add the enclosure" );

    BOOST_CHECK( PROJECT_GIT_UTILS::PathUsesGitLfs( repo, wxS( "case.step" ) ) );
    BOOST_CHECK( !PROJECT_GIT_UTILS::PathUsesGitLfs( repo, wxS( "board.kicad_pcb" ) ) );

    wxString example;
    BOOST_CHECK( PROJECT_GIT_UTILS::ProjectUsesGitLfs( common, &example ) );
    BOOST_CHECK_EQUAL( example.ToStdString(), "case.step" );

    // A project folder that holds none of the filtered files is left alone.
    const wxString elsewhere = TempPath() + wxS( "/elsewhere" );
    BOOST_REQUIRE( wxFileName::Mkdir( elsewhere ) );
    KIGIT_COMMON otherProject( repo, elsewhere );
    BOOST_CHECK( !PROJECT_GIT_UTILS::ProjectUsesGitLfs( otherProject ) );

    std::optional<wxString> prefix = PROJECT_GIT_UTILS::ProjectPrefix( wxS( "/repo" ), wxS( "/repo/proj" ) );
    BOOST_REQUIRE( prefix );
    BOOST_CHECK_EQUAL( prefix->ToStdString(), "proj/" );
    prefix = PROJECT_GIT_UTILS::ProjectPrefix( wxS( "/repo" ), wxS( "/repo" ) );
    BOOST_REQUIRE( prefix );
    BOOST_CHECK( prefix->IsEmpty() );
    BOOST_CHECK( !PROJECT_GIT_UTILS::ProjectPrefix( wxS( "/repo" ), wxS( "/other/proj" ) ) );
    BOOST_CHECK( !PROJECT_GIT_UTILS::ProjectPrefix( wxS( "/repo" ), wxS( "/repo2/proj" ) ) );
    BOOST_CHECK( !PROJECT_GIT_UTILS::ProjectPrefix( wxS( "/repo" ), wxEmptyString ) );

#ifdef __WINDOWS__
    // The working directory comes from libgit2 and the project path from KiCad, so the two
    // can disagree about separators and about case. Windows does not care about either.
    prefix = PROJECT_GIT_UTILS::ProjectPrefix( wxS( "C:\\repo" ), wxS( "C:/repo/proj" ) );
    BOOST_REQUIRE( prefix );
    BOOST_CHECK_EQUAL( prefix->ToStdString(), "proj/" );
    prefix = PROJECT_GIT_UTILS::ProjectPrefix( wxS( "c:\\Repo\\" ), wxS( "C:\\repo\\Proj" ) );
    BOOST_REQUIRE( prefix );
    BOOST_CHECK_EQUAL( prefix->ToStdString(), "Proj/" );
    BOOST_CHECK( !PROJECT_GIT_UTILS::ProjectPrefix( wxS( "C:\\repo" ), wxS( "D:\\repo\\proj" ) ) );
#endif
}


BOOST_AUTO_TEST_CASE( ProjectUsesGitLfsStaysWithinTheSessionProject )
{
    using KIGIT::PROJECT_GIT_UTILS;

    KIGIT::GitRepositoryPtr repo = KI_TEST::InitGitRepository( TempPath() );
    writeFile( TempPath(), ".gitattributes", "*.step filter=lfs diff=lfs merge=lfs -text\n" );
    commitWorkingFile( repo.get(), ".gitattributes", "use lfs" );
    writeFile( TempPath(), "projects/board/board.kicad_pcb", "(kicad_pcb)\n" );
    commitWorkingFile( repo.get(), "projects/board/board.kicad_pcb", "add project" );
    writeFile( TempPath(), "projects/board2/case.step", "sibling enclosure\n" );
    commitWorkingFile( repo.get(), "projects/board2/case.step", "add sibling enclosure" );

    KIGIT_COMMON common( repo.get(), TempPath() + wxS( "/projects/board" ) );
    wxString     example;
    BOOST_CHECK( !PROJECT_GIT_UTILS::ProjectUsesGitLfs( common, &example ) );
    BOOST_CHECK( PROJECT_GIT_UTILS::LfsRefusal( &common ).IsEmpty() );

    KIGIT_COMMON unknownProject( repo.get() );
    BOOST_CHECK( !PROJECT_GIT_UTILS::LfsRefusal( &unknownProject ).IsEmpty() );

    writeFile( TempPath(), "projects/board/models/case.step", "project enclosure\n" );
    commitWorkingFile( repo.get(), "projects/board/models/case.step", "add project enclosure" );

    BOOST_CHECK( PROJECT_GIT_UTILS::ProjectUsesGitLfs( common, &example ) );
    BOOST_CHECK_EQUAL( example.ToStdString(), "projects/board/models/case.step" );
    BOOST_CHECK( PROJECT_GIT_UTILS::LfsRefusal( &common ).Contains( example ) );
}


// An amend or a rebase leaves a commit's author time behind its committer time. The History
// page dates a commit by the committer time, so the Tags page has to agree.
BOOST_AUTO_TEST_CASE( GetTagDetailDatesALightweightTagByCommitterTime )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
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
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
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

    BOOST_CHECK( !common.GetTagDetail( wxS( "nowhere" ) ).found );
}


// The Tags page lists newest first. A tag on a tree has no commit and so no date, and goes last.
BOOST_AUTO_TEST_CASE( GetTagsListsNewestFirstAndKeepsTagsWithoutCommits )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_REQUIRE( common.CreateTag( "v1.0", "Release" ) );
    BOOST_REQUIRE( common.CreateTag( "beta-1" ) );

    // An old tag made long ago, and a tag on a tree.
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

    // The two tags made just now share a second, so their order between them is not fixed.
    std::vector<KIGIT_TAG_INFO> tags = common.GetTags();
    BOOST_REQUIRE_EQUAL( tags.size(), 4 );
    std::set<wxString> newest{ tags[0].name, tags[1].name };
    std::set<wxString> madeNow{ wxS( "v1.0" ), wxS( "beta-1" ) };
    BOOST_CHECK( newest == madeNow );
    BOOST_CHECK_EQUAL( tags[2].name.ToStdString(), "old" );
    BOOST_CHECK_EQUAL( tags[3].name.ToStdString(), "tree-tag" );
    BOOST_CHECK( tags[3].hash.IsEmpty() && !tags[3].when.IsValid() );
}


// The docked panel's Recent messages dropdown offers earlier messages to start from. Newest
// first, a message that was used twice listed once, and only what HEAD can reach.
BOOST_AUTO_TEST_CASE( GetRecentCommitMessagesListsDistinctMessagesNewestFirst )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    writeFile( TempPath(), "file.txt", "v2\n" );
    commitWorkingFile( repo, "file.txt", "Update BOM\n\nSecond pass.\n" );
    writeFile( TempPath(), "file.txt", "v3\n" );
    commitWorkingFile( repo, "file.txt", "Route power\n" );
    writeFile( TempPath(), "file.txt", "v4\n" );
    commitWorkingFile( repo, "file.txt", "Update BOM\n\nSecond pass.\n" );
    writeFile( TempPath(), "file.txt", "v5\n" );
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
    KI_TEST::SCOPED_TEMP_DIR empty( wxS( "kicad-qa-git-empty" ) );
    git_repository*          unborn = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_init( &unborn, empty.PathStr().utf8_string().c_str(), 0 ), GIT_OK );
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
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    writeFile( TempPath(), "file.txt", "v2\n" );

    KIGIT_COMMON common( repo );
    BOOST_REQUIRE( common.CreateBranch( "topic" ) );

    GIT_BRANCH_HANDLER handler( &common );
    BOOST_REQUIRE_MESSAGE( handler.SwitchToBranch( wxS( "topic" ) ) == BranchResult::Success,
                           handler.GetErrorString() );

    BOOST_CHECK_EQUAL( common.GetCurrentBranchName(), wxS( "topic" ) );
    BOOST_CHECK_EQUAL( readWorkingFile( wxFileName( TempPath(), "file.txt" ) ), "v2\n" );
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


BOOST_AUTO_TEST_CASE( ParseRemoteURLPreservesSSHAddress )
{
    struct TEST_CASE
    {
        const char* url;
        const char* username;
        const char* hostname;
    };

    const TEST_CASE cases[] = {
        { "ssh://runner@192.168.1.55:29022/srv/git/repos/l3-auth-30.git", "runner", "192.168.1.55" },
        { "ssh://192.168.1.55:29022/srv/git/repos/l3-auth-30.git", "", "192.168.1.55" },
        { "git+ssh://runner@example.com:2222/srv/repo", "runner", "example.com" },
        { "git@example.com:team/repo.git", "git", "example.com" },
        { "runner@example.com:team/repo", "runner", "example.com" },
        { "example.com:team/repo", "", "example.com" },
        { "ssh://runner@[2001:db8::1]:2222/srv/repo", "runner", "2001:db8::1" },
        { "runner@[2001:db8::1]:team/repo", "runner", "2001:db8::1" },
    };

    for( const TEST_CASE& test : cases )
    {
        BOOST_TEST_CONTEXT( test.url )
        {
            KIGIT_COMMON::REMOTE_URL parsed = KIGIT_COMMON::ParseRemoteURL( wxString::FromUTF8( test.url ) );

            BOOST_CHECK_EQUAL( parsed.url.ToStdString(), test.url );
            BOOST_CHECK_EQUAL( parsed.username.ToStdString(), test.username );
            BOOST_CHECK_EQUAL( parsed.hostname.ToStdString(), test.hostname );
            BOOST_CHECK( parsed.password.IsEmpty() );
            BOOST_CHECK( parsed.connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH );
        }
    }
}


BOOST_AUTO_TEST_CASE( ParseRemoteURLKeepsPortWhenExtractingCredentials )
{
    struct TEST_CASE
    {
        const char*                 input;
        const char*                 address;
        const char*                 username;
        const char*                 password;
        KIGIT_COMMON::GIT_CONN_TYPE connType;
    };

    const TEST_CASE cases[] = {
        { "https://alice:pa@ss@example.com:8443/team/repo@branch.git", "https://example.com:8443/team/repo@branch.git",
          "alice", "pa@ss", KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS },
        { "http://alice:secret@example.com:8080/team/repo", "http://example.com:8080/team/repo", "alice", "secret",
          KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS },
        { "https://example.com:8443/team/repo@branch.git", "https://example.com:8443/team/repo@branch.git", "", "",
          KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS },
        { "ssh://runner:pa@ss@example.com:29022/srv/repo", "ssh://runner@example.com:29022/srv/repo", "runner", "pa@ss",
          KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH },
    };

    for( const TEST_CASE& test : cases )
    {
        BOOST_TEST_CONTEXT( test.input )
        {
            KIGIT_COMMON::REMOTE_URL parsed = KIGIT_COMMON::ParseRemoteURL( wxString::FromUTF8( test.input ) );

            BOOST_CHECK_EQUAL( parsed.url.ToStdString(), test.address );
            BOOST_CHECK_EQUAL( parsed.username.ToStdString(), test.username );
            BOOST_CHECK_EQUAL( parsed.password.ToStdString(), test.password );
            BOOST_CHECK_EQUAL( parsed.hostname.ToStdString(), "example.com" );
            BOOST_CHECK( parsed.connType == test.connType );
        }
    }
}


BOOST_AUTO_TEST_CASE( ParseRemoteURLKeepsLocalPaths )
{
    const char* paths[] = {
        "/srv/git/repos/project.git",     "../repos/project.git",
        "./repos/project:backup.git",     "C:/repos/project.git",
        "C:\\repos\\project.git",         "C:project.git",
        "\\\\server\\repos\\project.git", "file:///srv/git/repos/project.git",
        "file:///C:/repos/project.git",
    };

    for( const char* path : paths )
    {
        BOOST_TEST_CONTEXT( path )
        {
            KIGIT_COMMON::REMOTE_URL parsed = KIGIT_COMMON::ParseRemoteURL( wxString::FromUTF8( path ) );

            BOOST_CHECK_EQUAL( parsed.url.ToStdString(), path );
            BOOST_CHECK( parsed.username.IsEmpty() );
            BOOST_CHECK( parsed.password.IsEmpty() );
            BOOST_CHECK( parsed.hostname.IsEmpty() );
            BOOST_CHECK( parsed.connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL );
        }
    }
}


BOOST_AUTO_TEST_CASE( SetRemotePreservesSSHAddressAndAcceptsLocalReplacement )
{
    KIGIT_COMMON   common( nullptr );
    const wxString sshURL = wxS( "ssh://runner@192.168.1.55:29022/srv/git/repos/l3-auth-30.git" );

    common.SetUsername( wxS( "previous-user" ) );
    common.SetRemote( sshURL );

    BOOST_CHECK_EQUAL( common.GetRemote(), sshURL );
    BOOST_CHECK_EQUAL( common.GetUsername(), wxS( "runner" ) );
    BOOST_CHECK( common.GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH );

    common.SetRemote( wxS( "/srv/git/repos/local.git" ) );

    BOOST_CHECK_EQUAL( common.GetRemote(), wxS( "/srv/git/repos/local.git" ) );
    BOOST_CHECK( common.GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL );
}


// Called after editing a remote, so a later push authenticates against the new server.
BOOST_AUTO_TEST_CASE( UpdateCurrentBranchInfoRefreshesChangedRemoteUrl )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
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
    // Only keys with both files on disk are offered
    KI_TEST::WriteGitFile( TempPath(), wxS( "keys/kicad" ), c_opensshPlain );
    KI_TEST::WriteGitFile( TempPath(), wxS( "keys/kicad.pub" ), "ssh-ed25519 AAAA qa\n" );

    const wxString key = wxFileName( TempPath() + wxS( "/keys/kicad" ) ).GetFullPath();
    KIGIT_COMMON   common( nullptr );

    common.SetSSHKey( key );
    common.SetSSHKey( wxEmptyString );
    common.ResetNextKey();

    BOOST_CHECK_EQUAL( common.GetNextPublicKey(), key );
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
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "v1\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    BOOST_CHECK( !common.HasUncommittedTrackedChanges() );

    // An untracked file alone does not count.
    {
        wxFileName    fn( TempPath(), wxS( "notes.txt" ) );
        std::ofstream f( fn.GetFullPath().utf8_string() );
        f << "scratch\n";
    }

    BOOST_CHECK( !common.HasUncommittedTrackedChanges() );

    // A modified tracked file does.
    {
        wxFileName    fn( TempPath(), wxS( "file.txt" ) );
        std::ofstream f( fn.GetFullPath().utf8_string() );
        f << "v2\n";
    }

    BOOST_CHECK( common.HasUncommittedTrackedChanges() );
}


BOOST_AUTO_TEST_CASE( WorktreeRefusalPrioritizesUnsavedDocumentAndLibraryEditors )
{
    GIT_WORKTREE_STATUS status;
    status.checked = true;
    status.hasTrackedChanges = true;
    status.lfsMessage = wxS( "Git LFS prevents this rewrite." );
    status.outsideProject = { wxS( "other/board.kicad_pcb" ) };

    const std::vector<wxString> editors = { wxS( "Schematic Editor" ), wxS( "Footprint Editor" ) };
    wxString                    message = status.RefusalMessage( editors );

    BOOST_CHECK( message.Contains( "Schematic Editor" ) );
    BOOST_CHECK( message.Contains( "Footprint Editor" ) );
    BOOST_CHECK( message.Contains( "unsaved changes" ) );
    BOOST_CHECK( message.Contains( "Save or discard" ) );
    BOOST_CHECK( !message.Contains( "Git LFS" ) );
    BOOST_CHECK( !message.Contains( "other/board.kicad_pcb" ) );

    wxString stashMessage = status.RefusalMessage( editors, true );
    BOOST_CHECK( stashMessage.Contains( "Schematic Editor" ) );
    BOOST_CHECK( stashMessage.Contains( "Footprint Editor" ) );
    BOOST_CHECK( stashMessage.Contains( "unsaved changes" ) );
    BOOST_CHECK( stashMessage.Contains( "Save or discard" ) );
    BOOST_CHECK( stashMessage.Contains( "before stashing" ) );

    // A failed disk check must still explain the unsaved work the user can resolve first.
    status.checked = false;
    BOOST_CHECK_EQUAL( status.RefusalMessage( editors ).ToStdString(), message.ToStdString() );
}


BOOST_AUTO_TEST_CASE( WorktreeStatusRefusesUnavailableAndFailedReads )
{
    KIGIT_COMMON       unavailable( nullptr );
    auto               unavailableLock = unavailable.LockAction();

    BOOST_CHECK( !GIT_WORKTREE_STATUS{}.RefusalMessage( {} ).IsEmpty() );

    GIT_WORKTREE_STATUS missing = ReadGitWorktreeStatus( unavailable, wxEmptyString );
    BOOST_CHECK( !missing.checked );
    BOOST_CHECK( missing.RefusalMessage( {} ).Contains( "Could not check" ) );
    BOOST_CHECK( !missing.RefusalMessage( {} ).Contains( "Stash Changes" ) );
    BOOST_CHECK( !missing.RefusalMessage( {}, true ).IsEmpty() );

    // A repository without a working tree makes libgit2's status read fail.
    git_repository* rawRepo = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_init( &rawRepo, TempPath().utf8_str(), 1 ), GIT_OK );
    KIGIT::GitRepositoryPtr repo( rawRepo );
    KIGIT_COMMON            common( repo.get() );
    auto                    lock = common.LockAction();

    GIT_WORKTREE_STATUS failed = ReadGitWorktreeStatus( common, TempPath() );
    BOOST_CHECK( !failed.checked );
    BOOST_CHECK( failed.RefusalMessage( {} ).Contains( "Could not check" ) );
}


BOOST_AUTO_TEST_CASE( WorktreeStatusAllowsCleanAndUntrackedFiles )
{
    KIGIT::GitRepositoryPtr repo( makeRepoWithCommit( TempPath(), "board.kicad_pcb", "original\n" ) );
    KIGIT_COMMON            common( repo.get(), TempPath() );
    auto                    lock = common.LockAction();

    GIT_WORKTREE_STATUS clean = ReadGitWorktreeStatus( common, TempPath() );
    BOOST_CHECK( clean.checked );
    BOOST_CHECK( !clean.hasTrackedChanges );
    BOOST_CHECK( clean.RefusalMessage( {} ).IsEmpty() );

    writeFile( TempPath(), "notes.txt", "untracked notes\n" );

    GIT_WORKTREE_STATUS untracked = ReadGitWorktreeStatus( common, TempPath() );
    BOOST_CHECK( untracked.checked );
    BOOST_CHECK( !untracked.hasTrackedChanges );
    BOOST_CHECK( untracked.outsideProject.empty() );
    BOOST_CHECK( untracked.RefusalMessage( {} ).IsEmpty() );
}


BOOST_AUTO_TEST_CASE( WorktreeStatusExplainsTrackedChangesInsideAndOutsideTheProject )
{
    KIGIT::GitRepositoryPtr repo( makeRepoWithCommit( TempPath(), "README.txt", "repository\n" ) );

    const wxString projectPath = TempPath() + wxS( "/project" );
    BOOST_REQUIRE( wxFileName::Mkdir( projectPath, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );
    BOOST_REQUIRE( wxFileName::Mkdir( TempPath() + wxS( "/project2" ), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );
    writeFile( TempPath(), "project/board.kicad_pcb", "project board\n" );
    commitWorkingFile( repo.get(), "project/board.kicad_pcb", "add project" );
    writeFile( TempPath(), "project2/board.kicad_pcb", "sibling board\n" );
    commitWorkingFile( repo.get(), "project2/board.kicad_pcb", "add sibling" );

    KIGIT_COMMON common( repo.get(), projectPath );
    auto         lock = common.LockAction();
    writeFile( TempPath(), "project/board.kicad_pcb", "edited project board\n" );

    GIT_WORKTREE_STATUS inside = ReadGitWorktreeStatus( common, projectPath );
    BOOST_CHECK( inside.checked );
    BOOST_CHECK( inside.hasTrackedChanges );
    BOOST_CHECK( inside.outsideProject.empty() );
    BOOST_CHECK( inside.RefusalMessage( {} ).Contains( "Stash Changes" ) );
    BOOST_CHECK( inside.RefusalMessage( {}, true ).IsEmpty() );

    writeFile( TempPath(), "project2/board.kicad_pcb", "edited sibling board\n" );
    writeFile( TempPath(), "project2/notes.txt", "untracked sibling notes\n" );

    GIT_WORKTREE_STATUS outside = ReadGitWorktreeStatus( common, projectPath );
    BOOST_CHECK( outside.checked );
    BOOST_CHECK( outside.hasTrackedChanges );
    BOOST_REQUIRE_EQUAL( outside.outsideProject.size(), 1 );
    BOOST_CHECK_EQUAL( outside.outsideProject.front().ToStdString(), "project2/board.kicad_pcb" );

    wxString message = outside.RefusalMessage( {} );
    BOOST_CHECK( message.Contains( "project2/board.kicad_pcb" ) );
    BOOST_CHECK( !message.Contains( "project/board.kicad_pcb" ) );
    BOOST_CHECK( !message.Contains( "notes.txt" ) );
    BOOST_CHECK( outside.RefusalMessage( {}, true ).IsEmpty() );
}


BOOST_AUTO_TEST_CASE( WorktreeStatusDistinguishesGitLfsFromUncommittedChanges )
{
    KIGIT::GitRepositoryPtr repo( makeRepoWithCommit( TempPath(), "board.kicad_pcb", "original\n" ) );

    writeFile( TempPath(), ".gitattributes", "*.step filter=lfs diff=lfs merge=lfs -text\n" );
    commitWorkingFile( repo.get(), ".gitattributes", "route step files through lfs" );
    writeFile( TempPath(), "case.step", "ISO-10303-21;\n" );
    commitWorkingFile( repo.get(), "case.step", "add the enclosure" );

    KIGIT_COMMON common( repo.get(), TempPath() );
    auto         lock = common.LockAction();

    GIT_WORKTREE_STATUS clean = ReadGitWorktreeStatus( common, TempPath() );
    BOOST_CHECK( clean.checked );
    BOOST_CHECK( !clean.hasTrackedChanges );
    BOOST_CHECK( clean.RefusalMessage( {} ).Contains( "Git LFS" ) );
    BOOST_CHECK( clean.RefusalMessage( {} ).Contains( "case.step" ) );

    writeFile( TempPath(), "board.kicad_pcb", "edited board\n" );

    GIT_WORKTREE_STATUS dirty = ReadGitWorktreeStatus( common, TempPath() );
    BOOST_CHECK( dirty.checked );
    BOOST_CHECK( dirty.hasTrackedChanges );
    BOOST_CHECK( dirty.RefusalMessage( {} ).Contains( "Git LFS" ) );
    BOOST_CHECK( !dirty.RefusalMessage( {} ).Contains( "Stash Changes" ) );
    BOOST_CHECK( dirty.RefusalMessage( {}, true ).Contains( "Git LFS" ) );
}


// Discarding a conflicted file puts back what HEAD holds and clears the conflict, so it settles
// the file rather than only throwing away an edit. The panel says so, and asks first.
BOOST_AUTO_TEST_CASE( DiscardingAConflictedPathSettlesIt )
{
    git_repository*         repo = makeRepoWithCommit( TempPath(), "file.txt", "one\ntwo\nthree\n" );
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


BOOST_AUTO_TEST_CASE( ForcePushRejectsAChangedConfirmedTarget )
{
    KIGIT::GitRepositoryPtr repo( makeRepoWithCommit( TempPath(), "file.txt", "one\n" ) );
    KIGIT_COMMON            common( repo.get() );

    git_oid head;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &head, repo.get(), "HEAD" ), GIT_OK );
    const wxString confirmedHead = wxString::FromUTF8( git_oid_tostr_s( &head ) );
    const wxString confirmedBranch = common.GetCurrentBranchName();

    auto forcePushError = [&common]( const wxString& aBranch, const wxString& aHeadOid )
    {
        GIT_PUSH_HANDLER handler( &common );
        handler.SetExpectedTarget( aBranch, aHeadOid );
        BOOST_CHECK_EQUAL( static_cast<int>( handler.PerformPush( true ) ), static_cast<int>( PushResult::Error ) );
        return handler.GetErrorString();
    };

    // A matching target gets past confirmation validation. With no remote configured it
    // stops at lookup, so none of these checks can start a network connection.
    const wxString noRemote = wxS( "There is no remote named 'origin'." );
    BOOST_CHECK( forcePushError( confirmedBranch, confirmedHead ).StartsWith( noRemote ) );

    const wxString targetChanged = wxS( "The branch or its last commit changed while the push was waiting." );

    // Switching branches must be rejected even when both branches name the same commit.
    BOOST_REQUIRE( common.CreateBranch( wxS( "other" ) ) );
    BOOST_REQUIRE_EQUAL( git_repository_set_head( repo.get(), "refs/heads/other" ), GIT_OK );
    BOOST_CHECK( forcePushError( confirmedBranch, confirmedHead ).Contains( targetChanged ) );

    // Advancing the confirmed branch must also be rejected, with the branch name unchanged.
    BOOST_REQUIRE_EQUAL( git_repository_set_head( repo.get(), "refs/heads/main" ), GIT_OK );
    writeFile( TempPath(), wxS( "file.txt" ), "two\n" );
    commitWorkingFile( repo.get(), wxS( "file.txt" ), "second" );
    BOOST_CHECK( forcePushError( confirmedBranch, confirmedHead ).Contains( targetChanged ) );

    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &head, repo.get(), "HEAD" ), GIT_OK );
    BOOST_CHECK(
            forcePushError( confirmedBranch, wxString::FromUTF8( git_oid_tostr_s( &head ) ) ).StartsWith( noRemote ) );
}


BOOST_AUTO_TEST_CASE( PushWaitsForQueryLockAndHonorsCancellation )
{
    KIGIT::GitRepositoryPtr repo( makeRepoWithCommit( TempPath(), "file.txt", "one\n" ) );
    KIGIT_COMMON            common( repo.get() );
    GIT_PUSH_HANDLER        handler( &common );
    handler.SetWaitForActionLock( true );

    KIGIT_COMMON::CANCEL_TOKEN token = KIGIT_COMMON::NewCancelToken();
    handler.SetCancelToken( token );

    auto               lock = common.LockAction();
    std::promise<void> entered;
    auto               ready = entered.get_future();
    auto               result = std::async( std::launch::async,
                                            [&handler, &entered]()
                                            {
                                  entered.set_value();
                                  return handler.PerformPush();
                                            } );

    const bool started = ready.wait_for( std::chrono::seconds( 5 ) ) == std::future_status::ready;
    const bool waited = result.wait_for( std::chrono::milliseconds( 50 ) ) == std::future_status::timeout;
    token->store( true );
    lock.unlock();

    BOOST_CHECK( started );
    BOOST_CHECK( waited );
    BOOST_REQUIRE( result.wait_for( std::chrono::seconds( 5 ) ) == std::future_status::ready );
    BOOST_CHECK_EQUAL( static_cast<int>( result.get() ), static_cast<int>( PushResult::Error ) );
    BOOST_CHECK( handler.IsThisOperationCancelled() );

    // No remote is configured. Reaching remote lookup would have added an error, so an empty
    // error list confirms cancellation stopped the operation before any network preparation.
    BOOST_CHECK( handler.GetErrorStrings().empty() );
    BOOST_CHECK_EQUAL( common.GetRecentCommits( 1 ).size(), 1u );
}


BOOST_AUTO_TEST_CASE( TransferCompletionCapturesStayOnUiThread )
{
    auto                wrongThread = std::make_shared<std::atomic<bool>>( false );
    UI_CALLBACK_CAPTURE capture( wrongThread );
    bool                completed = false;

    {
        KIGIT_TRANSFER transfer;
        BOOST_REQUIRE( transfer.Start(
                "completion capture",
                []( const KIGIT_COMMON::CANCEL_TOKEN& )
                {
                },
                [capture, &completed]()
                {
                    completed = true;
                } ) );

        BOOST_REQUIRE( waitForQuery(
                [&transfer]()
                {
                    return !transfer.IsBusy();
                } ) );
    }

    BOOST_CHECK( completed );
    BOOST_CHECK( !wrongThread->load() );

    auto               entered = std::make_shared<std::promise<void>>();
    std::promise<void> release;
    auto               ready = entered->get_future();
    auto               proceed = release.get_future().share();
    auto               transfer = std::make_unique<KIGIT_TRANSFER>();
    completed = false;

    BOOST_REQUIRE( transfer->Start(
            "destroyed completion owner",
            [entered, proceed]( const KIGIT_COMMON::CANCEL_TOKEN& )
            {
                entered->set_value();
                proceed.wait();
            },
            [capture, &completed]()
            {
                completed = true;
            } ) );

    const bool started = ready.wait_for( std::chrono::seconds( 5 ) ) == std::future_status::ready;
    transfer.reset();
    release.set_value();

    BOOST_CHECK( started );
    BOOST_REQUIRE_EQUAL( Backend()->OrphanRegistry().JoinAll( std::chrono::seconds( 5 ) ), 0u );
    wxTheApp->ProcessPendingEvents();
    BOOST_CHECK( !completed );
    BOOST_CHECK( !wrongThread->load() );
}


BOOST_AUTO_TEST_CASE( QuerySerializesAndKeepsOnlyTheLatestRequest )
{
    KIGIT::GitRepositoryPtr repo( makeRepoWithCommit( TempPath(), "file.txt", "one\n" ) );
    auto                    session = std::make_shared<KIGIT_COMMON>( repo.get() );
    KIGIT_QUERY             query;

    auto               entered = std::make_shared<std::promise<void>>();
    std::promise<void> release;
    auto               ready = entered->get_future();
    auto               proceed = release.get_future().share();
    auto               worked = std::make_shared<std::atomic<unsigned>>( 0 );
    auto               workerThread = std::make_shared<std::thread::id>();
    unsigned           completed = 0;

    query.Run(
            session,
            [worked, workerThread, proceed, entered]( KIGIT_COMMON& )
            {
                *workerThread = std::this_thread::get_id();
                worked->fetch_or( 1 );
                entered->set_value();
                proceed.wait();
            },
            [&completed]()
            {
                completed |= 1;
            } );

    BOOST_REQUIRE( ready.wait_for( std::chrono::seconds( 5 ) ) == std::future_status::ready );
    BOOST_CHECK( *workerThread != std::this_thread::get_id() );
    BOOST_CHECK( !session->TryLockAction().owns_lock() );

    query.Run(
            session,
            [worked]( KIGIT_COMMON& )
            {
                worked->fetch_or( 2 );
            },
            [&completed]()
            {
                completed |= 2;
            } );
    query.Run(
            session,
            [worked]( KIGIT_COMMON& )
            {
                worked->fetch_or( 4 );
            },
            [&completed]()
            {
                completed |= 4;
            } );
    release.set_value();

    BOOST_REQUIRE( waitForQuery(
            [&query]()
            {
                return !query.IsBusy();
            } ) );
    BOOST_CHECK_EQUAL( worked->load(), 5u );
    BOOST_CHECK_EQUAL( completed, 4u );
}


BOOST_AUTO_TEST_CASE( QueryCancelDropsPendingWorkAndQueuedCompletion )
{
    KIGIT::GitRepositoryPtr repo( makeRepoWithCommit( TempPath(), "file.txt", "one\n" ) );
    auto                    session = std::make_shared<KIGIT_COMMON>( repo.get() );
    KIGIT_QUERY             query;
    bool                    completed = false;
    auto                    worked = std::make_shared<std::atomic<unsigned>>( 0 );
    std::shared_ptr<int>    workerLifetime = std::make_shared<int>( 0 );
    std::weak_ptr<int>      workerWatch = workerLifetime;

    query.Run(
            session,
            [worked, workerLifetime]( KIGIT_COMMON& )
            {
                worked->fetch_add( 1 );
            },
            [&completed]()
            {
                completed = true;
            } );
    workerLifetime.reset();

    // The worker drops its capture after posting completion. Leave that event queued.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );

    while( !workerWatch.expired() && std::chrono::steady_clock::now() < deadline )
        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

    BOOST_REQUIRE( workerWatch.expired() );
    query.Run(
            session,
            [worked]( KIGIT_COMMON& )
            {
                worked->fetch_add( 1 );
            },
            [&completed]()
            {
                completed = true;
            } );
    query.Cancel();

    BOOST_REQUIRE( waitForQuery(
            [&query]()
            {
                return !query.IsBusy();
            } ) );
    BOOST_CHECK_EQUAL( worked->load(), 1u );
    BOOST_CHECK( !completed );
}


BOOST_AUTO_TEST_CASE( QueryDestructionDoesNotWaitAndPinsTheSession )
{
    KIGIT::GitRepositoryPtr     repo( makeRepoWithCommit( TempPath(), "file.txt", "one\n" ) );
    auto                        session = std::make_shared<KIGIT_COMMON>( repo.get() );
    std::weak_ptr<KIGIT_COMMON> watched = session;
    auto                        query = std::make_unique<KIGIT_QUERY>();

    auto               entered = std::make_shared<std::promise<void>>();
    std::promise<void> release;
    auto               ready = entered->get_future();
    auto               proceed = release.get_future().share();
    bool               completed = false;

    query->Run(
            session,
            [proceed, entered]( KIGIT_COMMON& )
            {
                entered->set_value();
                proceed.wait();
            },
            [&completed]()
            {
                completed = true;
            } );

    BOOST_REQUIRE( ready.wait_for( std::chrono::seconds( 5 ) ) == std::future_status::ready );
    session.reset();
    const auto start = std::chrono::steady_clock::now();
    query.reset();
    BOOST_CHECK( std::chrono::steady_clock::now() - start < std::chrono::seconds( 1 ) );
    BOOST_CHECK( !watched.expired() );
    release.set_value();

    BOOST_REQUIRE( waitForQuery(
            [&watched]()
            {
                return watched.expired();
            } ) );
    wxTheApp->ProcessPendingEvents();
    BOOST_CHECK( !completed );
}


BOOST_AUTO_TEST_CASE( QuerySkipsUnavailableSessionsAndCompletesOnUi )
{
    KIGIT::GitRepositoryPtr repo( makeRepoWithCommit( TempPath(), "file.txt", "one\n" ) );
    KIGIT_QUERY             query;
    auto                    worked = std::make_shared<std::atomic<bool>>( false );
    std::thread::id         completionThread;

    auto work = [worked]( KIGIT_COMMON& )
    {
        worked->store( true );
    };
    auto done = [&completionThread]()
    {
        completionThread = std::this_thread::get_id();
    };

    query.Run( nullptr, work, done );
    BOOST_REQUIRE( waitForQuery(
            [&query]()
            {
                return !query.IsBusy();
            } ) );
    BOOST_CHECK( completionThread == std::this_thread::get_id() );

    auto session = std::make_shared<KIGIT_COMMON>( nullptr );
    completionThread = {};
    query.Run( session, work, done );
    BOOST_REQUIRE( waitForQuery(
            [&query]()
            {
                return !query.IsBusy();
            } ) );
    BOOST_CHECK( completionThread == std::this_thread::get_id() );

    session->SetRepo( repo.get() );
    session->SetShuttingDown( true );
    completionThread = {};
    query.Run( session, work, done );
    BOOST_REQUIRE( waitForQuery(
            [&query]()
            {
                return !query.IsBusy();
            } ) );
    BOOST_CHECK( completionThread == std::this_thread::get_id() );
    BOOST_CHECK( !worked->load() );

    Backend()->OrphanRegistry().JoinAll( std::chrono::seconds( 1 ) );
    completionThread = {};
    query.Run( session, work, done );
    BOOST_CHECK( !query.IsBusy() );
    BOOST_CHECK( completionThread == std::this_thread::get_id() );
}


BOOST_AUTO_TEST_CASE( QueryExceptionStillCompletesAndCompletionCanDestroyOwner )
{
    KIGIT::GitRepositoryPtr repo( makeRepoWithCommit( TempPath(), "file.txt", "one\n" ) );
    auto                    session = std::make_shared<KIGIT_COMMON>( repo.get() );
    auto                    query = std::make_unique<KIGIT_QUERY>();
    wxLogNull               noLog;

    query->Run(
            session,
            []( KIGIT_COMMON& )
            {
                throw std::runtime_error( "query failed" );
            },
            [&query]()
            {
                query.reset();
            } );

    BOOST_REQUIRE( waitForQuery(
            [&query]()
            {
                return !query;
            } ) );
    BOOST_CHECK( session->TryLockAction().owns_lock() );
    BOOST_CHECK_EQUAL( session->GetRecentCommits( 1 ).size(), 1u );
}


BOOST_AUTO_TEST_CASE( QueryExceptionStillStartsThePendingRequest )
{
    KIGIT::GitRepositoryPtr repo( makeRepoWithCommit( TempPath(), "file.txt", "one\n" ) );
    auto                    session = std::make_shared<KIGIT_COMMON>( repo.get() );
    KIGIT_QUERY             query;
    wxLogNull               noLog;

    auto               entered = std::make_shared<std::promise<void>>();
    std::promise<void> release;
    auto               ready = entered->get_future();
    auto               proceed = release.get_future().share();
    auto               worked = std::make_shared<std::atomic<bool>>( false );
    unsigned           completed = 0;

    query.Run(
            session,
            [entered, proceed]( KIGIT_COMMON& )
            {
                entered->set_value();
                proceed.wait();
                throw 1;
            },
            [&completed]()
            {
                completed |= 1;
            } );

    BOOST_REQUIRE( ready.wait_for( std::chrono::seconds( 5 ) ) == std::future_status::ready );
    query.Run(
            session,
            [worked]( KIGIT_COMMON& )
            {
                worked->store( true );
            },
            [&completed]()
            {
                completed |= 2;
            } );
    release.set_value();

    BOOST_REQUIRE( waitForQuery(
            [&query]()
            {
                return !query.IsBusy();
            } ) );
    BOOST_CHECK( worked->load() );
    BOOST_CHECK_EQUAL( completed, 2u );
}


BOOST_AUTO_TEST_SUITE_END()
