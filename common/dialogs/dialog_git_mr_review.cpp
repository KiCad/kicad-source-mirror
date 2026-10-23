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
#include <git/project_git_utils.h>
#include <dialogs/git/git_status_entries.h>
#include <dialogs/git/git_ui_utils.h>
#include <diff_merge/merge_dispatch.h>
#include <kiway.h>
#include <reporter.h>

#include <git2.h>
#include <mutex>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include <wx/time.h>
#include <wx/utils.h>


DIALOG_GIT_MR_REVIEW::DIALOG_GIT_MR_REVIEW( wxWindow* aParent, KIGIT_COMMON* aCommon,
                                            const std::vector<wxString>& aRefList ) :
        DIALOG_GIT_MR_REVIEW_BASE( aParent ),
        m_common( aCommon ),
        m_repo( aCommon ? aCommon->GetRepo() : nullptr )
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

    SetupStandardButtons( { { wxID_CANCEL, _( "Close" ) } } );
    finishDialogSettings();
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

    // Hold the lock while resolving and comparing refs.
    std::unique_lock<std::mutex> lock = KIGIT::TryLockAction( *m_common, this );

    if( !lock.owns_lock() )
        return;

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

    lock.unlock();

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

    // The shared workspace owns a private scratch directory and cleans it up on scope exit. It
    // checks out the whole tree for a schematic so its sub-sheets resolve, instead of a lone blob.
    GIT_DIFF_WORKSPACE workspace;

    if( !workspace.IsOk() )
    {
        wxMessageBox( _( "Could not create a temporary directory." ), _( "Compare File" ),
                      wxOK | wxICON_ERROR, this );
        return;
    }

    // Extract both versions under the lock, then release it before opening the diff.
    std::unique_lock<std::mutex> lock = KIGIT::TryLockAction( *m_common, this );

    if( !lock.owns_lock() )
        return;

    const bool     hasBase  = file.status != KIGIT::FILE_CHANGE_STATUS::ADDED;
    const bool     hasHead  = file.status != KIGIT::FILE_CHANGE_STATUS::REMOVED;
    const wxString basePath = !file.oldPath.IsEmpty() ? file.oldPath : file.path;

    wxString pathA;
    wxString pathB;

    if( prettyFootprint )
    {
        // FP_LIB_DIFFER needs a .pretty directory on each side. The footprint's name in the
        // library is the filename without extension.
        const wxString sep = wxFileName::GetPathSeparator();
        const wxString fpFile = wxFileName( file.path ).GetFullName();

        wxString dirA = workspace.Root() + sep + wxS( "base.pretty" );
        wxString dirB = workspace.Root() + sep + wxS( "head.pretty" );

        if( hasBase && !wxFileName::Mkdir( dirA, 0700, wxPATH_MKDIR_FULL ) )
            return;

        if( hasHead && !wxFileName::Mkdir( dirB, 0700, wxPATH_MKDIR_FULL ) )
            return;

        if( hasBase )
        {
            if( !KIGIT::PROJECT_GIT_UTILS::WriteBlobToFile( m_repo, base, basePath, dirA + sep + fpFile ) )
            {
                wxMessageBox( wxString::Format( _( "Could not extract %s from %s." ), basePath, base ),
                              _( "Compare File" ), wxOK | wxICON_ERROR, this );
                return;
            }

            pathA = dirA;
        }

        if( hasHead )
        {
            if( !KIGIT::PROJECT_GIT_UTILS::WriteBlobToFile( m_repo, head, file.path, dirB + sep + fpFile ) )
            {
                wxMessageBox( wxString::Format( _( "Could not extract %s from %s." ), file.path, head ),
                              _( "Compare File" ), wxOK | wxICON_ERROR, this );
                return;
            }

            pathB = dirB;
        }
    }
    else
    {
        if( hasBase )
        {
            pathA = workspace.Materialize( m_repo, base, basePath, kind );

            if( pathA.IsEmpty() )
            {
                wxMessageBox( wxString::Format( _( "Could not extract %s from %s." ), basePath, base ),
                              _( "Compare File" ), wxOK | wxICON_ERROR, this );
                return;
            }
        }

        if( hasHead )
        {
            pathB = workspace.Materialize( m_repo, head, file.path, kind );

            if( pathB.IsEmpty() )
            {
                wxMessageBox( wxString::Format( _( "Could not extract %s from %s." ), file.path, head ),
                              _( "Compare File" ), wxOK | wxICON_ERROR, this );
                return;
            }
        }
    }

    lock.unlock();

    // pathA empty for ADDED, pathB empty for REMOVED. The kiface synthesizes an empty
    // document for the missing side.
    const wxString labelA = wxString::Format( wxS( "%s:%s" ), base, basePath );
    const wxString labelB = wxString::Format( wxS( "%s:%s" ), head, file.path );

    WX_STRING_REPORTER reporter;
    KICAD_DIFF::DispatchOpenDiffDialog( Kiway(), kind, pathA, pathB, labelA, labelB, this,
                                        &reporter );

    if( reporter.HasMessage() )
        wxMessageBox( reporter.GetMessages(), _( "Compare File" ),
                      wxOK | wxICON_INFORMATION, this );
}
