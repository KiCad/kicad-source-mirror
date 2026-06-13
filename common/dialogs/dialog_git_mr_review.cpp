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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "dialog_git_mr_review.h"

#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <diff_merge/merge_dispatch.h>
#include <kiway.h>
#include <reporter.h>

#include <git2.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include <wx/time.h>
#include <wx/utils.h>


DIALOG_GIT_MR_REVIEW::DIALOG_GIT_MR_REVIEW( wxWindow* aParent, git_repository* aRepo,
                                            const std::vector<wxString>& aRefList ) :
        DIALOG_GIT_MR_REVIEW_BASE( aParent ),
        m_repo( aRepo )
{
    m_listFiles->AppendColumn( _( "Path" ),   wxLIST_FORMAT_LEFT, 400 );
    m_listFiles->AppendColumn( _( "Status" ), wxLIST_FORMAT_LEFT, 100 );
    m_listFiles->AppendColumn( _( "Old Path" ), wxLIST_FORMAT_LEFT, 200 );

    for( const wxString& ref : aRefList )
    {
        m_comboBase->Append( ref );
        m_comboHead->Append( ref );
    }

    if( !aRefList.empty() )
    {
        m_comboBase->SetSelection( 0 );
        m_comboHead->SetSelection( aRefList.size() > 1 ? 1 : 0 );
    }

    Layout();
}


void DIALOG_GIT_MR_REVIEW::OnClose( wxCloseEvent& aEvent )
{
    EndModal( wxID_CANCEL );
}


void DIALOG_GIT_MR_REVIEW::OnCompareClick( wxCommandEvent& aEvent )
{
    populateFileList();
}


void DIALOG_GIT_MR_REVIEW::OnFileActivated( wxListEvent& aEvent )
{
    openFileDiff( aEvent.GetIndex() );
}


void DIALOG_GIT_MR_REVIEW::populateFileList()
{
    m_listFiles->DeleteAllItems();
    m_changedFiles.clear();

    if( !m_repo )
    {
        wxMessageBox( _( "No git repository available." ), _( "Compare Branches" ),
                      wxOK | wxICON_ERROR, this );
        return;
    }

    wxString base = m_comboBase->GetValue();
    wxString head = m_comboHead->GetValue();

    if( base.IsEmpty() || head.IsEmpty() )
    {
        wxMessageBox( _( "Select base and head refs to compare." ),
                      _( "Compare Branches" ), wxOK | wxICON_INFORMATION, this );
        return;
    }

    // Validate each ref resolves and peels to a tree; CompareRefs needs a
    // tree on both sides.
    auto validateRef =
            [this]( const wxString& aRef ) -> bool
            {
                KIGIT::GitTreePtr treePtr( KIGIT::ResolveRefToTree( m_repo, aRef ) );

                if( !treePtr )
                {
                    wxMessageBox(
                            wxString::Format( _( "Could not resolve ref '%s' to a commit or tree." ),
                                              aRef ),
                            _( "Compare Branches" ), wxOK | wxICON_ERROR, this );
                    return false;
                }

                return true;
            };

    if( !validateRef( base ) || !validateRef( head ) )
        return;

    m_changedFiles = KIGIT::CompareRefs( m_repo, base, head );

    if( m_changedFiles.empty() )
    {
        m_listFiles->InsertItem( 0, _( "No changes between selected refs." ) );
        return;
    }

    long row = 0;

    for( const KIGIT::CHANGED_FILE& f : m_changedFiles )
    {
        long idx = m_listFiles->InsertItem( row++, f.path );
        m_listFiles->SetItem( idx, 1,
                              wxString::FromUTF8( KIGIT::FileChangeStatusToString( f.status ) ) );

        if( !f.oldPath.IsEmpty() )
            m_listFiles->SetItem( idx, 2, f.oldPath );
    }
}


namespace
{

/// Resolve a ref + path inside the repo to a blob written into aDestPath.
/// Returns true on success. Empty/null on the ref side (e.g. ADDED on base or
/// REMOVED on head) writes an empty file so the differ sees a valid empty
/// document.
bool writeRefBlobToFile( git_repository* aRepo, const wxString& aRef, const wxString& aPath,
                         const wxString& aDestPath )
{
    if( aRef.IsEmpty() || aPath.IsEmpty() )
        return false;

    KIGIT::GitTreePtr treePtr( KIGIT::ResolveRefToTree( aRepo, aRef ) );

    if( !treePtr )
        return false;

    git_tree_entry* entry = nullptr;

    if( git_tree_entry_bypath( &entry, treePtr.get(), aPath.ToUTF8().data() ) != 0 )
        return false;

    std::unique_ptr<git_tree_entry, decltype( &git_tree_entry_free )> entryPtr(
            entry, &git_tree_entry_free );

    if( git_tree_entry_type( entry ) != GIT_OBJECT_BLOB )
        return false;

    git_object* blobObj = nullptr;

    if( git_tree_entry_to_object( &blobObj, aRepo, entry ) != 0 )
        return false;

    KIGIT::GitObjectPtr blobObjPtr( blobObj );
    git_blob*           blob = reinterpret_cast<git_blob*>( blobObj );

    wxFile out( aDestPath, wxFile::write );

    if( !out.IsOpened() )
        return false;

    const void* raw  = git_blob_rawcontent( blob );
    git_off_t   size = git_blob_rawsize( blob );

    return out.Write( raw, static_cast<size_t>( size ) ) == static_cast<size_t>( size );
}

} // namespace


namespace
{

/// Detect "foo.pretty/bar.kicad_mod" change paths so we can dispatch them as
/// a single-footprint FP_LIB diff (the FP differ is library-level; we build
/// minimal one-entry .pretty dirs on each side).
bool isPrettyFootprintChange( const wxString& aPath )
{
    if( KICAD_DIFF::DocKindFromExtension( aPath ) != KICAD_DIFF::DOC_KIND::FOOTPRINT )
        return false;

    wxFileName fn( aPath );
    const wxArrayString& dirs = fn.GetDirs();

    return !dirs.IsEmpty() && dirs.Last().EndsWith( wxS( ".pretty" ) );
}


/// Recursively removes a directory on scope exit. Prevents leaking the
/// temporary diff workspace down any error path.
struct TempDirGuard
{
    wxString path;

    ~TempDirGuard()
    {
        if( !path.IsEmpty() )
            wxFileName::Rmdir( path, wxPATH_RMDIR_RECURSIVE );
    }
};

} // namespace


void DIALOG_GIT_MR_REVIEW::openFileDiff( long aListIndex )
{
    if( aListIndex < 0 || aListIndex >= static_cast<long>( m_changedFiles.size() ) )
        return;

    const KIGIT::CHANGED_FILE& file = m_changedFiles[aListIndex];

    KICAD_DIFF::DOC_KIND kind            = KICAD_DIFF::DocKindFromExtension( file.path );
    bool                 prettyFootprint = isPrettyFootprintChange( file.path );

    if( prettyFootprint )
        kind = KICAD_DIFF::DOC_KIND::FP_LIB;

    if( kind == KICAD_DIFF::DOC_KIND::UNKNOWN || kind == KICAD_DIFF::DOC_KIND::FOOTPRINT )
    {
        wxMessageBox( wxString::Format( _( "No diff handler for '%s'." ), file.path ),
                      _( "Compare File" ), wxOK | wxICON_INFORMATION, this );
        return;
    }

    const wxString base = m_comboBase->GetValue();
    const wxString head = m_comboHead->GetValue();

    // Non-recursive Mkdir keeps the creation atomic. wxPATH_MKDIR_FULL would
    // silently accept an attacker-precreated leaf, reintroducing the TOCTOU
    // hole the previous CreateTempFileName + remove + Mkdir pattern had.
    const wxString tempBase =
            wxStandardPaths::Get().GetTempDir() + wxFileName::GetPathSeparator()
            + wxS( "kicad_diff_" )
            + wxString::Format( wxS( "%lu_" ),
                                static_cast<unsigned long>( wxGetProcessId() ) );

    const unsigned millis = static_cast<unsigned>( wxGetLocalTimeMillis().GetLo() );
    wxString       tmpRoot;

    for( int attempt = 0; attempt < 16 && tmpRoot.IsEmpty(); ++attempt )
    {
        wxString candidate = tempBase + wxString::Format( wxS( "%d_%u" ), attempt, millis );

        if( wxFileName::Mkdir( candidate, 0700, 0 ) )
            tmpRoot = candidate;
    }

    if( tmpRoot.IsEmpty() )
    {
        wxMessageBox( _( "Could not create a temporary directory." ), _( "Compare File" ),
                      wxOK | wxICON_ERROR, this );
        return;
    }

    TempDirGuard tempGuard{ tmpRoot };

    const bool     hasBase  = file.status != KIGIT::FILE_CHANGE_STATUS::ADDED;
    const bool     hasHead  = file.status != KIGIT::FILE_CHANGE_STATUS::REMOVED;
    const wxString basePath = !file.oldPath.IsEmpty() ? file.oldPath : file.path;

    wxString pathA;
    wxString pathB;

    if( prettyFootprint )
    {
        // Build minimal .pretty directories on each side so FP_LIB_DIFFER can
        // operate. The footprint's name inside the library is the filename
        // without extension; both sides keep the same name.
        wxFileName srcFn( file.path );
        const wxString fpFile = srcFn.GetFullName();

        wxString dirA = tmpRoot + wxFileName::GetPathSeparator() + wxS( "base.pretty" );
        wxString dirB = tmpRoot + wxFileName::GetPathSeparator() + wxS( "head.pretty" );

        if( hasBase && !wxFileName::Mkdir( dirA, 0700, wxPATH_MKDIR_FULL ) )
        {
            return;
        }

        if( hasHead && !wxFileName::Mkdir( dirB, 0700, wxPATH_MKDIR_FULL ) )
        {
            return;
        }

        if( hasBase )
        {
            wxString blobPath = dirA + wxFileName::GetPathSeparator() + fpFile;

            if( !writeRefBlobToFile( m_repo, base, basePath, blobPath ) )
            {
                wxMessageBox( wxString::Format( _( "Could not extract %s from %s." ),
                                                basePath, base ),
                              _( "Compare File" ), wxOK | wxICON_ERROR, this );
                return;
            }

            pathA = dirA;
        }

        if( hasHead )
        {
            wxString blobPath = dirB + wxFileName::GetPathSeparator() + fpFile;

            if( !writeRefBlobToFile( m_repo, head, file.path, blobPath ) )
            {
                wxMessageBox( wxString::Format( _( "Could not extract %s from %s." ),
                                                file.path, head ),
                              _( "Compare File" ), wxOK | wxICON_ERROR, this );
                return;
            }

            pathB = dirB;
        }
    }
    else
    {
        wxFileName srcName( file.path );

        if( hasBase )
        {
            pathA = tmpRoot + wxFileName::GetPathSeparator() + wxS( "base_" )
                    + srcName.GetFullName();

            if( !writeRefBlobToFile( m_repo, base, basePath, pathA ) )
            {
                wxMessageBox( wxString::Format( _( "Could not extract %s from %s." ),
                                                basePath, base ),
                              _( "Compare File" ), wxOK | wxICON_ERROR, this );
                return;
            }
        }

        if( hasHead )
        {
            pathB = tmpRoot + wxFileName::GetPathSeparator() + wxS( "head_" )
                    + srcName.GetFullName();

            if( !writeRefBlobToFile( m_repo, head, file.path, pathB ) )
            {
                wxMessageBox( wxString::Format( _( "Could not extract %s from %s." ),
                                                file.path, head ),
                              _( "Compare File" ), wxOK | wxICON_ERROR, this );
                return;
            }
        }
    }

    // pathA empty for ADDED / pathB empty for REMOVED — the kiface synthesizes
    // an empty document for the missing side.
    const wxString labelA = wxString::Format( wxS( "%s:%s" ), base, basePath );
    const wxString labelB = wxString::Format( wxS( "%s:%s" ), head, file.path );

    WX_STRING_REPORTER reporter;
    KICAD_DIFF::DispatchOpenDiffDialog( Kiway(), kind, pathA, pathB, labelA, labelB, this,
                                        &reporter );

    if( reporter.HasMessage() )
        wxMessageBox( reporter.GetMessages(), _( "Compare File" ),
                      wxOK | wxICON_INFORMATION, this );
}
