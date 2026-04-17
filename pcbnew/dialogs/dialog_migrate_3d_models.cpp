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
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <dialogs/dialog_migrate_3d_models.h>

#include <algorithm>
#include <unordered_set>

#include <wx/utils.h>
#include <wx/dir.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wx/filename.h>

#include <board.h>
#include <board_commit.h>
#include <footprint.h>
#include <pcb_edit_frame.h>
#include <pgm_base.h>
#include <project.h>
#include <project_pcb.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>

#include <3d_cache/3d_cache.h>
#include <3d_model_viewer/eda_3d_model_viewer.h>
#include <common_ogl/ogl_attr_list.h>
#include <filename_resolver.h>


namespace
{
/**
 * Model extensions we consider "current" (STEP-era) replacements.  Ordered
 * by user preference: STEP first, then compressed STEP, then IGES.
 */
const std::vector<wxString>& stepExtensions()
{
    static const std::vector<wxString> exts = {
        wxS( "step" ), wxS( "stp" ), wxS( "stpz" ),
        wxS( "step.gz" ), wxS( "stp.gz" ),
        wxS( "iges" ), wxS( "igs" )
    };
    return exts;
}


/**
 * Remove any recognised 3D extension (WRL or STEP family) from aName and
 * lowercase the result.  Used to build the comparable "stem" used by the
 * match scorer.
 */
wxString normalizeStem( const wxString& aName )
{
    wxString stem = aName;

    // Strip known extensions (case-insensitive), including doubled-up .gz
    // forms that don't round-trip through wxFileName::GetName.
    static const wxString stripList[] = {
        wxS( ".step.gz" ), wxS( ".stp.gz" ),
        wxS( ".wrl" ),  wxS( ".wrz" ),
        wxS( ".step" ), wxS( ".stp" ), wxS( ".stpz" ),
        wxS( ".iges" ), wxS( ".igs" )
    };

    for( const wxString& ext : stripList )
    {
        if( stem.length() > ext.length()
                && stem.Right( ext.length() ).Lower() == ext )
        {
            stem = stem.Left( stem.length() - ext.length() );
            break;
        }
    }

    stem.MakeLower();

    // Treat separator characters as interchangeable so that e.g.
    // "R_0603_1608Metric" and "R-0603-1608Metric" collide.
    stem.Replace( wxS( "-" ), wxS( "_" ) );
    stem.Replace( wxS( " " ), wxS( "_" ) );

    return stem;
}


/// Classic iterative Levenshtein edit distance, O(|a| * |b|) time, O(min) space.
int levenshtein( const wxString& a, const wxString& b )
{
    const size_t m = a.length();
    const size_t n = b.length();

    if( m == 0 )
        return static_cast<int>( n );

    if( n == 0 )
        return static_cast<int>( m );

    std::vector<int> prev( n + 1 );
    std::vector<int> curr( n + 1 );

    for( size_t j = 0; j <= n; ++j )
        prev[j] = static_cast<int>( j );

    for( size_t i = 1; i <= m; ++i )
    {
        curr[0] = static_cast<int>( i );

        for( size_t j = 1; j <= n; ++j )
        {
            int cost = ( a[i - 1] == b[j - 1] ) ? 0 : 1;
            curr[j] = std::min( { curr[j - 1] + 1, prev[j] + 1, prev[j - 1] + cost } );
        }

        std::swap( prev, curr );
    }

    return prev[n];
}


/// Extract the basename of the parent directory of aPath, e.g.
/// "/foo/Resistor_SMD.3dshapes/R_0603.wrl" -> "Resistor_SMD.3dshapes".
wxString parentDirName( const wxString& aPath )
{
    wxFileName fn( aPath );
    const wxArrayString& dirs = fn.GetDirs();
    return dirs.empty() ? wxString() : dirs.Last();
}


/// Collect plausible directories from the WRL filename even before resolving.
/// For "${KICAD9_3DMODEL_DIR}/Resistor_SMD.3dshapes/R_0603.wrl" this returns
/// "Resistor_SMD.3dshapes".
wxString parentDirFromFilename( const wxString& aFilename )
{
    return parentDirName( aFilename );
}

}  // anonymous namespace


DIALOG_MIGRATE_3D_MODELS::DIALOG_MIGRATE_3D_MODELS( PCB_EDIT_FRAME* aFrame ) :
        DIALOG_MIGRATE_3D_MODELS_BASE( aFrame ),
        m_frame( aFrame ),
        m_modelViewer( nullptr )
{
    // Single column spanning the full list-control width; wxLC_REPORT needs
    // at least one column to show anything, wxLC_NO_HEADER hides the header.
    m_missingList->AppendColumn( wxEmptyString, wxLIST_FORMAT_LEFT, 800 );
    m_candidatesList->AppendColumn( wxEmptyString, wxLIST_FORMAT_LEFT, 800 );

    // Keep the single column filling the visible width as the splitter sash
    // moves.  Bare wxListCtrl in report mode clips overflowing text cleanly.
    auto fitColumn = []( wxListCtrl* aList )
    {
        const int width = std::max( aList->GetClientSize().GetWidth() - 2, 20 );
        aList->SetColumnWidth( 0, width );
    };

    m_missingList->Bind( wxEVT_SIZE,
            [this, fitColumn]( wxSizeEvent& aEvt )
            {
                fitColumn( m_missingList );
                aEvt.Skip();
            } );

    m_candidatesList->Bind( wxEVT_SIZE,
            [this, fitColumn]( wxSizeEvent& aEvt )
            {
                fitColumn( m_candidatesList );
                aEvt.Skip();
            } );

    // Embed the 3D model preview canvas in the placeholder panel created by
    // the generated base class.  Same pattern as PANEL_FP_PROPERTIES_3D_MODEL.
    S3D_CACHE* cache = PROJECT_PCB::Get3DCacheManager( &m_frame->Prj() );
    m_modelViewer = new EDA_3D_MODEL_VIEWER( m_previewPanel,
            OGL_ATT_LIST::GetAttributesList( ANTIALIASING_MODE::AA_8X ), cache );
    m_previewPanel->GetSizer()->Add( m_modelViewer, 1, wxEXPAND, 0 );
    m_previewPanel->Layout();

    collectMissingModels();
    buildCatalog();
    rankAllCandidates();

    // Auto-select the top real (non-"keep existing") candidate for every
    // missing row so the user sees a sensible default without having to
    // click through each one.  Rows for which no real candidate exists get
    // bolded by populateMissingList() as a visual attention marker.
    for( size_t i = 0; i < m_missing.size(); ++i )
    {
        const std::vector<MATCH_CANDIDATE>& cands = m_candidatesPerMissing[i];

        if( !cands.empty() && !cands.front().m_absPath.IsEmpty() )
            m_selectedPerMissing[i] = 0;
    }

    populateMissingList();

    if( !m_missing.empty() )
    {
        m_missingList->SetItemState( 0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
                                     wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );
        populateCandidatesList( 0 );
    }

    finishDialogSettings();
    applyInitialSizeCaps();

    // Position the splitter sashes so the three columns start out at roughly
    // one third of the dialog width each.  Must happen after the dialog has
    // been laid out at its final initial size.
    const int clientWidth = m_mainSplitter->GetClientSize().GetWidth();

    if( clientWidth > 360 )
    {
        const int outerSash = clientWidth / 3;
        m_mainSplitter->SetSashPosition( outerSash );

        const int innerWidth = std::max( clientWidth - outerSash, 240 );
        m_innerSplitter->SetSashPosition( innerWidth / 2 );
    }
}


void DIALOG_MIGRATE_3D_MODELS::applyInitialSizeCaps()
{
    const wxSize screenSize = wxGetDisplaySize();
    const int hardCap = ( screenSize.GetWidth() * 9 ) / 10;

    // Soft cap (0.9 * parent width) only applies on the first time the
    // dialog is opened against a given user settings file.  We detect that
    // by comparing the current width to the hardcoded base-class default:
    // if finishDialogSettings() restored a saved value the width won't
    // match the default.
    constexpr int kBaseDefaultWidth = 900;
    int cap = hardCap;

    if( m_frame && GetSize().GetWidth() == kBaseDefaultWidth )
    {
        const int parentCap = ( m_frame->GetSize().GetWidth() * 9 ) / 10;
        cap = std::min( cap, parentCap );
    }

    if( GetSize().GetWidth() > cap )
    {
        wxSize sized = GetSize();
        sized.SetWidth( std::max( cap, GetMinSize().GetWidth() ) );
        SetSize( sized );
        Centre( wxBOTH );
    }
}


DIALOG_MIGRATE_3D_MODELS::~DIALOG_MIGRATE_3D_MODELS() = default;


bool DIALOG_MIGRATE_3D_MODELS::BoardHasUnresolvedWrlReferences( PCB_EDIT_FRAME* aFrame )
{
    if( !aFrame )
        return false;

    BOARD*             board    = aFrame->GetBoard();
    FILENAME_RESOLVER* resolver = PROJECT_PCB::Get3DFilenameResolver( &aFrame->Prj() );

    if( !board || !resolver )
        return false;

    const wxString projectPath = aFrame->Prj().GetProjectPath();

    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( const FP_3DMODEL& model : fp->Models() )
        {
            const wxString& fn = model.m_Filename;

            if( fn.IsEmpty() )
                continue;

            const wxString ext = fn.AfterLast( '.' ).Lower();

            if( ext != wxS( "wrl" ) && ext != wxS( "wrz" ) )
                continue;

            if( resolver->ResolvePath( fn, projectPath, {} ).IsEmpty() )
                return true;
        }
    }

    return false;
}


void DIALOG_MIGRATE_3D_MODELS::collectMissingModels()
{
    BOARD*             board    = m_frame->GetBoard();
    FILENAME_RESOLVER* resolver = PROJECT_PCB::Get3DFilenameResolver( &m_frame->Prj() );

    if( !board || !resolver )
        return;

    const wxString projectPath = m_frame->Prj().GetProjectPath();
    std::unordered_set<wxString> seen;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( const FP_3DMODEL& model : fp->Models() )
        {
            const wxString& fn = model.m_Filename;

            if( fn.IsEmpty() )
                continue;

            const wxString ext = fn.AfterLast( '.' ).Lower();

            if( ext != wxS( "wrl" ) && ext != wxS( "wrz" ) )
                continue;

            if( seen.count( fn ) )
                continue;

            // Mirror the resolver invocation used by the 3D viewer: empty
            // return means "not found on disk via any search path".
            wxString resolved = resolver->ResolvePath( fn, projectPath, {} );

            if( resolved.IsEmpty() )
            {
                seen.insert( fn );
                m_missing.push_back( fn );
            }
        }
    }

    std::sort( m_missing.begin(), m_missing.end() );
    m_selectedPerMissing.assign( m_missing.size(), -1 );
    m_candidatesPerMissing.resize( m_missing.size() );
}


void DIALOG_MIGRATE_3D_MODELS::buildCatalog()
{
    if( m_missing.empty() )
        return;

    wxBusyCursor busy;

    FILENAME_RESOLVER* resolver = PROJECT_PCB::Get3DFilenameResolver( &m_frame->Prj() );

    // Standard KiCad 3D search paths (resolved to absolute directories).  GetKicadPaths
    // returns variable names, not filesystem paths, so walk the resolver's search-path list
    // directly and consume m_Pathexp (the expanded absolute directory).
    if( resolver )
    {
        if( const std::list<SEARCH_PATH>* searchPaths = resolver->GetPaths() )
        {
            for( const SEARCH_PATH& sp : *searchPaths )
            {
                if( !sp.m_Pathexp.IsEmpty() )
                    scanDirectory( sp.m_Pathexp );
            }
        }
    }

    // Project-local 3D model directory, when present.
    wxFileName prj3D( m_frame->Prj().GetProjectPath(), wxEmptyString );
    prj3D.AppendDir( wxS( "3dshapes" ) );

    if( prj3D.DirExists() )
        scanDirectory( prj3D.GetPath() );

    // User-added extra directories, persisted across sessions.
    COMMON_SETTINGS* common = Pgm().GetCommonSettings();

    if( common )
    {
        for( const wxString& dir : common->m_Extra3DSearchDirs )
            scanDirectory( dir );
    }
}


void DIALOG_MIGRATE_3D_MODELS::scanDirectory( const wxString& aDir )
{
    if( aDir.IsEmpty() )
        return;

    wxFileName normFn( aDir, wxEmptyString );
    normFn.Normalize( wxPATH_NORM_ABSOLUTE | wxPATH_NORM_DOTS );
    const wxString key = normFn.GetPath().Lower();

    if( !m_scannedDirs.insert( key ).second )
        return;  // already scanned

    if( !wxDir::Exists( normFn.GetPath() ) )
        return;

    wxArrayString files;
    wxDir::GetAllFiles( normFn.GetPath(), &files, wxEmptyString, wxDIR_FILES | wxDIR_DIRS );

    // Collect duplicate-detection set by absolute-path key so re-scanning
    // the same tree (e.g. through a different env var alias) doesn't add
    // duplicate catalog entries.
    std::unordered_set<wxString> existing;

    for( const CATALOG_ENTRY& e : m_catalog )
        existing.insert( e.m_absPath.Lower() );

    for( const wxString& f : files )
    {
        const wxString lower = f.Lower();
        bool acceptable = false;

        for( const wxString& ext : stepExtensions() )
        {
            if( lower.length() > ext.length() + 1
                    && lower.Right( ext.length() + 1 ) == wxS( "." ) + ext )
            {
                acceptable = true;
                break;
            }
        }

        if( !acceptable )
            continue;

        if( existing.count( lower ) )
            continue;

        existing.insert( lower );

        CATALOG_ENTRY entry;
        entry.m_absPath = f;
        entry.m_stem    = normalizeStem( wxFileName( f ).GetFullName() );
        entry.m_parent  = parentDirName( f );
        m_catalog.push_back( entry );
    }
}


std::vector<DIALOG_MIGRATE_3D_MODELS::MATCH_CANDIDATE>
DIALOG_MIGRATE_3D_MODELS::rankCandidatesFor( const wxString& aWrlFilename ) const
{
    const wxString wrlStem   = normalizeStem( wxFileName( aWrlFilename ).GetFullName() );
    const wxString wrlParent = parentDirFromFilename( aWrlFilename );

    std::vector<MATCH_CANDIDATE> ranked;
    ranked.reserve( std::min<size_t>( m_catalog.size(), 64 ) );

    for( const CATALOG_ENTRY& entry : m_catalog )
    {
        int score = 0;

        if( entry.m_stem == wrlStem )
        {
            score = ( !wrlParent.IsEmpty() && entry.m_parent.CmpNoCase( wrlParent ) == 0 )
                            ? 1000
                            : 800;
        }
        else
        {
            const int dist     = levenshtein( entry.m_stem, wrlStem );
            const int maxDist  = static_cast<int>( std::max<size_t>( 2, wrlStem.length() / 4 ) );

            if( dist <= maxDist )
                score = 500 - dist * 10;
        }

        if( score > 0 )
        {
            MATCH_CANDIDATE cand;
            cand.m_absPath = entry.m_absPath;
            cand.m_display = wxFileName( entry.m_absPath ).GetFullName()
                             + wxS( "   \u2014   " )
                             + entry.m_parent;
            cand.m_score   = score;
            ranked.push_back( cand );
        }
    }

    std::sort( ranked.begin(), ranked.end(),
               []( const MATCH_CANDIDATE& a, const MATCH_CANDIDATE& b )
               {
                   if( a.m_score != b.m_score )
                       return a.m_score > b.m_score;

                   return a.m_display.CmpNoCase( b.m_display ) < 0;
               } );

    constexpr size_t kMaxCandidates = 15;

    if( ranked.size() > kMaxCandidates )
        ranked.resize( kMaxCandidates );

    // Always append "keep existing" at the bottom so the user has a way
    // to explicitly decline a replacement for this one row.
    MATCH_CANDIDATE keep;
    keep.m_absPath = wxEmptyString;
    keep.m_display = _( "(keep existing reference)" );
    keep.m_score   = 0;
    ranked.push_back( keep );

    return ranked;
}


void DIALOG_MIGRATE_3D_MODELS::rankAllCandidates()
{
    for( size_t i = 0; i < m_missing.size(); ++i )
        m_candidatesPerMissing[i] = rankCandidatesFor( m_missing[i] );
}


void DIALOG_MIGRATE_3D_MODELS::populateMissingList()
{
    m_missingList->DeleteAllItems();

    for( size_t i = 0; i < m_missing.size(); ++i )
    {
        const long row = m_missingList->InsertItem( static_cast<long>( i ), m_missing[i] );
        (void) row;
        updateMissingItemStyle( static_cast<int>( i ) );
    }
}


void DIALOG_MIGRATE_3D_MODELS::updateMissingItemStyle( int aMissingIndex )
{
    if( aMissingIndex < 0 || aMissingIndex >= static_cast<int>( m_missing.size() ) )
        return;

    wxFont font = m_missingList->GetFont();
    font.SetWeight( m_selectedPerMissing[aMissingIndex] < 0 ? wxFONTWEIGHT_BOLD
                                                             : wxFONTWEIGHT_NORMAL );
    m_missingList->SetItemFont( aMissingIndex, font );
}


void DIALOG_MIGRATE_3D_MODELS::populateCandidatesList( int aMissingIndex )
{
    m_candidatesList->DeleteAllItems();

    if( aMissingIndex < 0 || aMissingIndex >= static_cast<int>( m_missing.size() ) )
        return;

    const std::vector<MATCH_CANDIDATE>& cands = m_candidatesPerMissing[aMissingIndex];

    for( size_t i = 0; i < cands.size(); ++i )
        m_candidatesList->InsertItem( static_cast<long>( i ), cands[i].m_display );

    const int sel = m_selectedPerMissing[aMissingIndex];

    if( sel >= 0 && sel < static_cast<int>( cands.size() ) )
    {
        m_candidatesList->SetItemState( sel, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
                                        wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );
        showPreview( cands[sel].m_absPath );
    }
    else if( !cands.empty() )
    {
        // No committed choice yet (only the "keep existing" row is available).
        // Highlight the top row for visual continuity but leave the preview
        // empty — there's no real candidate to show.
        m_candidatesList->SetItemState( 0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
                                        wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );
        showPreview( cands.front().m_absPath );
    }
}


void DIALOG_MIGRATE_3D_MODELS::showPreview( const wxString& aAbsPath )
{
    if( !m_modelViewer )
        return;

    if( aAbsPath.IsEmpty() )
        m_modelViewer->Clear3DModel();
    else
        m_modelViewer->Set3DModel( aAbsPath );
}


static int getSelectedRow( wxListCtrl* aList )
{
    return static_cast<int>( aList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED ) );
}


void DIALOG_MIGRATE_3D_MODELS::OnMissingSelected( wxListEvent& aEvent )
{
    populateCandidatesList( static_cast<int>( aEvent.GetIndex() ) );
}


void DIALOG_MIGRATE_3D_MODELS::OnCandidateSelected( wxListEvent& aEvent )
{
    const int missingIdx = static_cast<int>( getSelectedRow( m_missingList ) );
    const int candIdx    = static_cast<int>( aEvent.GetIndex() );

    if( missingIdx < 0 || missingIdx >= static_cast<int>( m_missing.size() ) )
        return;

    const std::vector<MATCH_CANDIDATE>& cands = m_candidatesPerMissing[missingIdx];

    if( candIdx < 0 || candIdx >= static_cast<int>( cands.size() ) )
        return;

    const MATCH_CANDIDATE& c = cands[candIdx];

    // Record this row's choice.  The "keep existing" row has an empty path
    // and we store -1 to mean "no replacement".
    m_selectedPerMissing[missingIdx] = c.m_absPath.IsEmpty() ? -1 : candIdx;

    // Bold the missing row again when the user explicitly chose "keep
    // existing", so the visual cue tracks the actual selection state.
    updateMissingItemStyle( missingIdx );

    showPreview( c.m_absPath );
}


void DIALOG_MIGRATE_3D_MODELS::OnAddSearchDirectoryClick( wxCommandEvent& )
{
    wxDirDialog dlg( this, _( "Add 3D Model Search Directory" ) );

    if( dlg.ShowModal() != wxID_OK )
        return;

    const wxString chosen = dlg.GetPath();

    if( chosen.IsEmpty() )
        return;

    // Persist the directory so future sessions include it automatically.
    COMMON_SETTINGS* common = Pgm().GetCommonSettings();

    if( common )
    {
        auto& dirs = common->m_Extra3DSearchDirs;

        if( std::find( dirs.begin(), dirs.end(), chosen ) == dirs.end() )
            dirs.push_back( chosen );
    }

    // Scan just the new directory; catalog dedup ensures no duplicates.
    {
        wxBusyCursor busy;
        scanDirectory( chosen );
    }

    // Re-rank every row against the expanded catalog and refresh the view,
    // preserving the user's existing per-row selections by filename.
    std::vector<wxString> priorSelectionPaths( m_missing.size() );

    for( size_t i = 0; i < m_missing.size(); ++i )
    {
        const int sel = m_selectedPerMissing[i];

        if( sel >= 0 && sel < static_cast<int>( m_candidatesPerMissing[i].size() ) )
            priorSelectionPaths[i] = m_candidatesPerMissing[i][sel].m_absPath;
    }

    rankAllCandidates();

    for( size_t i = 0; i < m_missing.size(); ++i )
    {
        m_selectedPerMissing[i] = -1;
        const std::vector<MATCH_CANDIDATE>& cands = m_candidatesPerMissing[i];

        // Preserve the user's prior explicit pick when the same file still
        // appears in the catalog.
        if( !priorSelectionPaths[i].IsEmpty() )
        {
            for( size_t j = 0; j < cands.size(); ++j )
            {
                if( cands[j].m_absPath == priorSelectionPaths[i] )
                {
                    m_selectedPerMissing[i] = static_cast<int>( j );
                    break;
                }
            }
        }

        // For rows with no prior pick (or whose pick disappeared), auto-select
        // the newly-best candidate so adding a directory immediately un-bolds
        // rows that just acquired a plausible match.
        if( m_selectedPerMissing[i] < 0 && !cands.empty()
                && !cands.front().m_absPath.IsEmpty() )
        {
            m_selectedPerMissing[i] = 0;
        }

        updateMissingItemStyle( static_cast<int>( i ) );
    }

    const int activeRow = getSelectedRow( m_missingList );
    populateCandidatesList( activeRow );
}


void DIALOG_MIGRATE_3D_MODELS::OnOpenExternalFileClick( wxCommandEvent& )
{
    const int missingIdx = getSelectedRow( m_missingList );

    if( missingIdx < 0 || missingIdx >= static_cast<int>( m_missing.size() ) )
        return;

    wxString wildcard;

    for( const wxString& ext : stepExtensions() )
    {
        if( !wildcard.IsEmpty() )
            wildcard += wxS( ";" );

        wildcard += wxS( "*." ) + ext;
    }

    wxFileDialog dlg( this, _( "Select 3D Model File" ), wxEmptyString, wxEmptyString,
                      _( "3D Models" ) + wxS( " (" ) + wildcard + wxS( ")|" ) + wildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() != wxID_OK )
        return;

    const wxString chosenPath = dlg.GetPath();

    if( chosenPath.IsEmpty() )
        return;

    // Inject the chosen file as a top-ranked candidate for this row so the
    // user can see it previewed and it will be applied when they hit Replace.
    MATCH_CANDIDATE injected;
    injected.m_absPath = chosenPath;
    injected.m_display = wxFileName( chosenPath ).GetFullName() + wxS( "   \u2014   " )
                         + _( "user-selected" );
    injected.m_score   = 2000;

    std::vector<MATCH_CANDIDATE>& cands = m_candidatesPerMissing[missingIdx];

    // Avoid duplicating an already-present entry.
    auto existing = std::find_if( cands.begin(), cands.end(),
                                   [&]( const MATCH_CANDIDATE& c )
                                   { return c.m_absPath == chosenPath; } );

    if( existing == cands.end() )
    {
        cands.insert( cands.begin(), injected );
        m_selectedPerMissing[missingIdx] = 0;
    }
    else
    {
        m_selectedPerMissing[missingIdx] = static_cast<int>( existing - cands.begin() );
    }

    updateMissingItemStyle( missingIdx );
    populateCandidatesList( missingIdx );
}


void DIALOG_MIGRATE_3D_MODELS::OnReplaceClick( wxCommandEvent& )
{
    // Build the old->new filename map from the per-row selections.  We store
    // the new filename through FILENAME_RESOLVER::ShortenPath() so that
    // env-var aliases like ${KICAD10_3DMODEL_DIR} survive round-trip.
    FILENAME_RESOLVER* resolver = PROJECT_PCB::Get3DFilenameResolver( &m_frame->Prj() );
    std::map<wxString, wxString> replacements;

    for( size_t i = 0; i < m_missing.size(); ++i )
    {
        const int sel = m_selectedPerMissing[i];

        if( sel < 0 || sel >= static_cast<int>( m_candidatesPerMissing[i].size() ) )
            continue;

        const MATCH_CANDIDATE& c = m_candidatesPerMissing[i][sel];

        if( c.m_absPath.IsEmpty() )
            continue;  // "(keep existing)"

        wxString stored = c.m_absPath;

        if( resolver )
            stored = resolver->ShortenPath( c.m_absPath );

        replacements[m_missing[i]] = stored;
    }

    if( replacements.empty() )
    {
        // User clicked Replace without choosing anything; treat as Keep.
        if( m_doNotShowAgain->IsChecked() )
        {
            if( COMMON_SETTINGS* s = Pgm().GetCommonSettings() )
                s->m_DoNotShowAgain.migrate_wrl_prompt = true;
        }

        EndModal( wxID_CANCEL );
        return;
    }

    BOARD*       board  = m_frame->GetBoard();
    BOARD_COMMIT commit( m_frame );

    for( FOOTPRINT* fp : board->Footprints() )
    {
        bool changedThisFp = false;

        for( FP_3DMODEL& model : fp->Models() )
        {
            auto it = replacements.find( model.m_Filename );

            if( it == replacements.end() )
                continue;

            if( !changedThisFp )
            {
                commit.Modify( fp );
                changedThisFp = true;
            }

            model.m_Filename = it->second;
        }
    }

    commit.Push( _( "Migrate 3D model references" ) );

    // Respect the user's "do not show again" choice regardless of whether
    // they replaced any models on this particular board.
    if( m_doNotShowAgain->IsChecked() )
    {
        if( COMMON_SETTINGS* s = Pgm().GetCommonSettings() )
            s->m_DoNotShowAgain.migrate_wrl_prompt = true;
    }

    EndModal( wxID_OK );
}


void DIALOG_MIGRATE_3D_MODELS::OnKeepClick( wxCommandEvent& )
{
    if( m_doNotShowAgain->IsChecked() )
    {
        if( COMMON_SETTINGS* s = Pgm().GetCommonSettings() )
            s->m_DoNotShowAgain.migrate_wrl_prompt = true;
    }

    EndModal( wxID_CANCEL );
}
