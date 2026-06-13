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

#include "dialog_kicad_merge_3way.h"

#include <diff_merge/auto_resolution.h>
#include <diff_merge/diff_scene.h>
#include <trace_helpers.h>
#include <widgets/widget_diff_canvas.h>

#include <wx/ffile.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/utils.h>


namespace
{

/// Color of the conflict bbox highlight overlaid on the canvas. Matches
/// the diff theme's CONFLICT color so the user gets the same magenta
/// across the two dialogs.
const KIGFX::COLOR4D& highlightColor()
{
    static const KICAD_DIFF::DIFF_COLOR_THEME theme;
    return theme.conflict;
}


/// Tri-state outcome of looking at KICAD_MERGETOOL_AUTO. Differentiates
/// "interactive — env unset" from "headless — env set but bad" so the
/// dialog can fail loudly in scripted/CI runs rather than silently falling
/// back to an interactive UI that has no human at the keyboard.
enum class AUTO_MODE
{
    NOT_SET,        // KICAD_MERGETOOL_AUTO env var absent — use the dialog
    LOADED,         // env set, file valid, aOut populated
    LOAD_FAILED     // env set but file missing / unparseable / malformed
};


/// Read the KICAD_MERGETOOL_AUTO file and parse it. All JSON / validation
/// logic lives in `KICAD_DIFF::ParseAutoResolutionJson` — this is the
/// thin env-var + file-I/O + trace-log shim around it.
AUTO_MODE loadAutoResolutionFile( std::map<wxString, KICAD_DIFF::ITEM_RES>& aOut )
{
    wxString path;

    if( !wxGetEnv( wxT( "KICAD_MERGETOOL_AUTO" ), &path ) || path.IsEmpty() )
        return AUTO_MODE::NOT_SET;

    wxFFile  file( path, wxT( "rb" ) );
    wxString contents;

    if( !file.IsOpened() || !file.ReadAll( &contents, wxConvUTF8 ) )
    {
        wxLogTrace( traceDiffMerge,
                    wxT( "KICAD_MERGETOOL_AUTO points at unreadable path '%s'" ), path );
        return AUTO_MODE::LOAD_FAILED;
    }

    KICAD_DIFF::AUTO_RESOLUTION_PARSE_RESULT parsed =
            KICAD_DIFF::ParseAutoResolutionJson(
                    std::string( contents.utf8_str().data(), contents.utf8_str().length() ) );

    if( parsed.status != KICAD_DIFF::AUTO_RESOLUTION_STATUS::OK )
    {
        wxLogTrace( traceDiffMerge,
                    wxT( "KICAD_MERGETOOL_AUTO parse failed (status=%d, context=%s)" ),
                    static_cast<int>( parsed.status ), parsed.errorContext );
        return AUTO_MODE::LOAD_FAILED;
    }

    aOut = std::move( parsed.resolutions );
    return AUTO_MODE::LOADED;
}

} // namespace


DIALOG_KICAD_MERGE_3WAY::DIALOG_KICAD_MERGE_3WAY( wxWindow* aParent,
                                                  const KICAD_DIFF::MERGE_PLAN& aPlan,
                                                  CONFLICT_CONTEXT aContext ) :
        DIALOG_KICAD_MERGE_3WAY_BASE( aParent ),
        m_plan( aPlan ),
        m_context( std::move( aContext ) )
{
    // Splice a GAL-backed conflict viewer into the resolution panel's sizer
    // when one is present. The original .fbp emits a text-only layout; we
    // augment it programmatically here rather than re-authoring the .fbp.
    if( wxSizer* resolutionSizer = m_panelResolution->GetSizer() )
    {
        m_canvas = new WIDGET_DIFF_CANVAS( m_panelResolution );
        m_canvas->SetMinSize( wxSize( -1, 200 ) );

        // Insert just above the text detail (which sits at index 1 after
        // the label). Keep the existing layout otherwise intact.
        const size_t insertAt = std::min<size_t>( 1, resolutionSizer->GetItemCount() );
        resolutionSizer->Insert( insertAt, m_canvas,
                                 wxSizerFlags( 1 ).Expand().Border( wxALL, 4 ) );
        m_panelResolution->Layout();
    }

    buildList();

    if( m_conflictActionIndex.empty() )
    {
        m_labelDetail->SetLabel( _( "No conflicts to resolve." ) );
        m_sdbSizerApply->Enable( true );
    }
    else
    {
        m_labelDetail->SetLabel( wxString::Format( _( "%zu conflict(s) require resolution." ),
                                                   m_conflictActionIndex.size() ) );
    }

    Layout();

    // Headless / scripted mode: if KICAD_MERGETOOL_AUTO is set, apply the
    // pre-recorded resolutions and close the dialog immediately. The
    // EndModal is deferred via CallAfter so it runs after ShowModal has
    // started its event loop. Used by CI test runners that exercise the
    // full git mergetool → kicad-cli → kicad --mergetool → applier flow
    // without a human at the keyboard.
    //
    // Bad-env paths (missing file, parse error, partial coverage, unknown
    // resolution kind) explicitly EndModal( wxID_CANCEL ) so the caller's
    // exit code is non-zero. Silently falling back to the interactive
    // dialog would hang a CI run forever.
    std::map<wxString, KICAD_DIFF::ITEM_RES> autoResolutions;

    switch( loadAutoResolutionFile( autoResolutions ) )
    {
    case AUTO_MODE::NOT_SET:
        break;

    case AUTO_MODE::LOAD_FAILED:
        wxLogTrace( traceDiffMerge,
                    wxT( "KICAD_MERGETOOL_AUTO load failed; closing dialog as cancelled" ) );
        CallAfter( [this]() { EndModal( wxID_CANCEL ); } );
        break;

    case AUTO_MODE::LOADED:
    {
        KICAD_DIFF::APPLY_AUTO_RESOLUTIONS_RESULT applied =
                KICAD_DIFF::ApplyAutoResolutions( m_plan, m_conflictActionIndex,
                                                  autoResolutions );

        switch( applied.status )
        {
        case KICAD_DIFF::APPLY_AUTO_RESOLUTIONS_STATUS::PARTIAL:
            wxLogTrace( traceDiffMerge,
                        wxT( "KICAD_MERGETOOL_AUTO missing resolution for %s" ),
                        applied.firstMissingId.AsString() );
            CallAfter( [this]() { EndModal( wxID_CANCEL ); } );
            break;

        case KICAD_DIFF::APPLY_AUTO_RESOLUTIONS_STATUS::COMPLETE:
        case KICAD_DIFF::APPLY_AUTO_RESOLUTIONS_STATUS::NO_CONFLICTS:
            CallAfter( [this]() { EndModal( wxID_APPLY ); } );
            wxLogTrace( traceDiffMerge,
                        wxT( "KICAD_MERGETOOL_AUTO applied %zu auto-resolutions" ),
                        applied.appliedCount );
            break;
        }

        break;
    }
    }
}


void DIALOG_KICAD_MERGE_3WAY::OnClose( wxCloseEvent& aEvent )
{
    EndModal( wxID_CANCEL );
}


void DIALOG_KICAD_MERGE_3WAY::OnCancel( wxCommandEvent& aEvent )
{
    EndModal( wxID_CANCEL );
}


void DIALOG_KICAD_MERGE_3WAY::OnApply( wxCommandEvent& aEvent )
{
    // Only mark resolved if every conflict has a concrete TAKE_* choice.
    // KEEP / MERGE_PROPS at this point mean the user hasn't actually picked
    // a side; without this guard the dialog would silently accept the
    // engine's default resolution and lose user intent.
    const std::vector<KIID_PATH> stillUnresolved =
            KICAD_DIFF::CollectUnresolvedConflicts( m_plan, m_conflictActionIndex );

    if( !stillUnresolved.empty() )
    {
        wxMessageBox( wxString::Format( _( "%zu conflict(s) still need a resolution. "
                                           "Pick Ours, Theirs, or Ancestor for each one." ),
                                        stillUnresolved.size() ),
                      _( "Resolve Merge Conflicts" ),
                      wxOK | wxICON_INFORMATION, this );
        return;
    }

    m_plan.unresolved.clear();
    EndModal( wxID_APPLY );
}


void DIALOG_KICAD_MERGE_3WAY::OnConflictSelected( wxCommandEvent& aEvent )
{
    showConflict( m_listConflicts->GetSelection() );
}


void DIALOG_KICAD_MERGE_3WAY::OnResolutionChanged( wxCommandEvent& aEvent )
{
    if( m_currentConflict < 0
        || m_currentConflict >= static_cast<int>( m_conflictActionIndex.size() ) )
    {
        return;
    }

    std::size_t              actionIdx = m_conflictActionIndex[m_currentConflict];
    KICAD_DIFF::ITEM_RESOLUTION& action = m_plan.actions[actionIdx];

    if( m_radioOurs->GetValue() )          action.kind = KICAD_DIFF::ITEM_RES::TAKE_OURS;
    else if( m_radioTheirs->GetValue() )   action.kind = KICAD_DIFF::ITEM_RES::TAKE_THEIRS;
    else if( m_radioAncestor->GetValue() ) action.kind = KICAD_DIFF::ITEM_RES::TAKE_ANCESTOR;

    // Side selection also drives which geometry the canvas displays so the
    // user can preview each candidate's context.
    rebuildCanvas();
}


DIALOG_KICAD_MERGE_3WAY::SIDE DIALOG_KICAD_MERGE_3WAY::activeSide() const
{
    if( m_radioTheirs->GetValue() )   return SIDE::THEIRS;
    if( m_radioAncestor->GetValue() ) return SIDE::ANCESTOR;
    return SIDE::OURS;
}


void DIALOG_KICAD_MERGE_3WAY::rebuildCanvas()
{
    if( !m_canvas )
        return;

    KICAD_DIFF::DIFF_SCENE scene;
    const std::map<KIID_PATH, BOX2I>* primaryBBoxes = nullptr;

    switch( activeSide() )
    {
    case SIDE::OURS:
        scene.referenceGeometry = m_context.oursGeometry;
        primaryBBoxes           = &m_context.oursBBoxes;
        break;
    case SIDE::THEIRS:
        scene.referenceGeometry = m_context.theirsGeometry;
        primaryBBoxes           = &m_context.theirsBBoxes;
        break;
    case SIDE::ANCESTOR:
        scene.referenceGeometry = m_context.ancestorGeometry;
        primaryBBoxes           = &m_context.ancestorBBoxes;
        break;
    }

    static const std::map<KIID_PATH, BOX2I> emptyBBoxMap;

    BOX2I conflictBBox;

    if( m_currentConflict >= 0
        && m_currentConflict < static_cast<int>( m_conflictActionIndex.size() ) )
    {
        const KICAD_DIFF::ITEM_RESOLUTION& action =
                m_plan.actions[m_conflictActionIndex[m_currentConflict]];

        const std::optional<BOX2I> resolved =
                KICAD_DIFF::ResolveConflictBBox(
                        action.id,
                        primaryBBoxes ? *primaryBBoxes : emptyBBoxMap,
                        m_context.oursBBoxes, m_context.theirsBBoxes,
                        m_context.ancestorBBoxes );

        if( resolved.has_value() )
        {
            conflictBBox = *resolved;

            KICAD_DIFF::SCENE_SHAPE shape;
            shape.bbox     = conflictBBox;
            shape.color    = highlightColor();
            shape.changeId = action.id;
            scene.conflictShapes.push_back( shape );
        }
    }

    // documentBBox covers the side's geometry so the user can pan/zoom out
    // to see context; the explicit ZoomToBBox below focuses the initial view
    // on the conflict so it isn't lost in the wide-area auto-fit.
    KICAD_DIFF::ExpandBBoxToGeometry( scene );

    const std::optional<KIID_PATH> highlight =
            m_currentConflict >= 0
                    ? std::optional<KIID_PATH>(
                            m_plan.actions[m_conflictActionIndex[m_currentConflict]].id )
                    : std::nullopt;

    m_canvas->SetScene( std::move( scene ) );
    m_canvas->HighlightChange( highlight );

    if( conflictBBox.GetWidth() > 0 || conflictBBox.GetHeight() > 0 )
    {
        // Pad the focus a bit so the highlight isn't flush with the edges
        // and the user sees some surrounding context.
        BOX2I focus = conflictBBox;
        focus.Inflate( std::max( conflictBBox.GetWidth(), conflictBBox.GetHeight() ) );
        m_canvas->ZoomToBBox( focus );
    }
}


void DIALOG_KICAD_MERGE_3WAY::buildList()
{
    m_listConflicts->Clear();
    m_conflictActionIndex.clear();

    for( const KICAD_DIFF::CONFLICT_LIST_ENTRY& entry
                : KICAD_DIFF::BuildConflictList( m_plan ) )
    {
        m_listConflicts->Append( entry.label );
        m_conflictActionIndex.push_back( entry.actionIndex );
    }
}


void DIALOG_KICAD_MERGE_3WAY::showConflict( int aIndex )
{
    m_currentConflict = aIndex;

    if( aIndex < 0 || aIndex >= static_cast<int>( m_conflictActionIndex.size() ) )
    {
        m_labelDetail->SetLabel( _( "Select a conflict on the left to see details." ) );
        m_textDetail->SetValue( wxEmptyString );
        return;
    }

    std::size_t actionIdx = m_conflictActionIndex[aIndex];
    const KICAD_DIFF::ITEM_RESOLUTION& action = m_plan.actions[actionIdx];

    m_labelDetail->SetLabel( wxString::Format( _( "Conflict on %s" ), action.id.AsString() ) );

    m_textDetail->SetValue( KICAD_DIFF::BuildConflictDetailText( action ) );

    // Sync the radio buttons to the current resolution.
    m_radioOurs->SetValue(     action.kind == KICAD_DIFF::ITEM_RES::TAKE_OURS );
    m_radioTheirs->SetValue(   action.kind == KICAD_DIFF::ITEM_RES::TAKE_THEIRS );
    m_radioAncestor->SetValue( action.kind == KICAD_DIFF::ITEM_RES::TAKE_ANCESTOR );

    rebuildCanvas();
}
