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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "panel_footprint_filters.h"

#include <algorithm>

#include <wx/clipbrd.h>
#include <wx/menu.h>
#include <wx/wupdlock.h>

#include <bitmaps.h>
#include <dialogs/dialog_text_entry.h>
#include <footprint_library_query.h>
#include <kiway.h>
#include <kiway_mail.h>
#include <lib_id.h>
#include <string_utils.h>
#include <widgets/listbox_tricks.h>
#include <widgets/std_bitmap_button.h>


/**
 * Footprint filters are saved as a space-separated list, so a
 * space inside a filter would be read as a separator.
 * Replace spaces with underscores.
 */
static wxString SanitizeFpFilter( const wxString& aFilter )
{
    wxString filter = aFilter;
    filter.Replace( wxT( " " ), wxT( "_" ) );
    return filter;
}


/**
 * Supplies the footprints matching a symbol's footprint filter patterns.
 *
 * Queries are made one pattern at a time through the pcbnew kiface and the result for each
 * pattern is cached, so editing one filter re-queries only that filter (and backspacing
 * doesn't trigger a new query for patterns that are already cached).
 */
class FP_FILTER_MATCHER
{
public:
    explicit FP_FILTER_MATCHER( KIWAY& aKiway ) :
            m_kiway( aKiway )
    {
    }

    /**
     * Return the case-insensitively sorted, de-duplicated union of the footprints matching
     * any of the given patterns.
     */
    FOOTPRINT_MATCH_RESULT GetMatches( const std::vector<wxString>& aPatterns )
    {
        FOOTPRINT_MATCH_RESULT result;
        result.m_Success = true;

        for( const wxString& pattern : aPatterns )
        {
            FOOTPRINT_MATCH_RESULT patternResult = fetchMatches( pattern );

            result.m_Success = result.m_Success && patternResult.m_Success;
            result.m_IsLimited = result.m_IsLimited || patternResult.m_IsLimited;

            result.m_MatchingNames.insert( result.m_MatchingNames.end(), patternResult.m_MatchingNames.begin(),
                                           patternResult.m_MatchingNames.end() );
        }

        std::sort( result.m_MatchingNames.begin(), result.m_MatchingNames.end(),
                   []( const wxString& aLeft, const wxString& aRight )
                   {
                       return StrNumCmp( aLeft, aRight, true ) < 0;
                   } );

        auto sameName = []( const wxString& aLeft, const wxString& aRight )
        {
            return aLeft.IsSameAs( aRight, false );
        };

        result.m_MatchingNames.erase(
                std::unique( result.m_MatchingNames.begin(), result.m_MatchingNames.end(), sameName ),
                result.m_MatchingNames.end() );

        return result;
    }

    /**
     * Drop the cached results for the patterns that are not in aPatterns.
     *
     * The filter list is the only place a pattern can persist, so dropping the rest keeps
     * the cache bounded.  A pattern from a live-preview edit never reaches the list, so it
     * is dropped here once the edit is done.
     */
    void PruneCache( const std::vector<wxString>& aPatterns )
    {
        std::set<wxString> wanted;

        for( const wxString& pattern : aPatterns )
            wanted.insert( pattern.Lower() );

        std::erase_if( m_cache,
                       [&wanted]( const auto& aEntry )
                       {
                           return !wanted.contains( aEntry.first );
                       } );
    }

private:
    /**
     * Return the matches for a single pattern, querying the libraries when it is not cached
     * yet.  Matching is case-insensitive, so patterns differing only in case share an entry.
     */
    FOOTPRINT_MATCH_RESULT fetchMatches( const wxString& aPattern )
    {
        wxString key = aPattern.Lower();
        auto     it = m_cache.find( key );

        if( it != m_cache.end() )
            return it->second;

        FOOTPRINT_MATCH_QUERY query;
        query.m_Patterns.push_back( aPattern );
        query.m_MaxResults = MAX_MATCH_RESULTS;

        FOOTPRINT_MATCH_RESULT result = QueryMatchingFootprints( m_kiway, query );

        // A failed query is not cached: the libraries may be reachable next time.
        if( result.m_Success )
            m_cache.emplace( key, result );

        return result;
    }

    // Symbol filters normally select at most a few hundred footprints, which is all the
    // matching-footprints list needs to show.
    static constexpr int MAX_MATCH_RESULTS = 500;

    KIWAY&                                     m_kiway;
    std::map<wxString, FOOTPRINT_MATCH_RESULT> m_cache;
};


PANEL_FOOTPRINT_FILTERS::PANEL_FOOTPRINT_FILTERS( wxWindow* aParent, KIWAY& aKiway ) :
        PANEL_FOOTPRINT_FILTERS_BASE( aParent ),
        m_kiway( aKiway ),
        m_fpFilterTricks( std::make_unique<LISTBOX_TRICKS>( *this, *m_FootprintFilterListBox ) ),
        m_fpMatcher( std::make_unique<FP_FILTER_MATCHER>( aKiway ) ),
        m_matchLoadTimer( this ),
        m_filterPreviewDebounce(
                [this]()
                {
                    updateMatchingFootprints();
                },
                WX_DEBOUNCED_ACTION::TEXT_INPUT_DEBOUNCE_MS )
{
    // Configure button logos
    m_addFilterButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_editFilterButton->SetBitmap( KiBitmapBundle( BITMAPS::small_edit ) );
    m_deleteFilterButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    // Forward the delete button to the tricks
    m_deleteFilterButton->Bind( wxEVT_BUTTON,
                                [&]( wxCommandEvent& aEvent )
                                {
                                    wxCommandEvent cmdEvent( EDA_EVT_LISTBOX_DELETE );
                                    m_fpFilterTricks->ProcessEvent( cmdEvent );
                                } );

    // When the filter tricks modifies something, update ourselves
    m_FootprintFilterListBox->Bind( EDA_EVT_LISTBOX_CHANGED,
                                    [&]( wxCommandEvent& aEvent )
                                    {
                                        // Filters pasted or duplicated into the list need the
                                        // same space handling as ones typed into the dialogs.
                                        for( unsigned ii = 0; ii < m_FootprintFilterListBox->GetCount(); ++ii )
                                        {
                                            wxString filter = m_FootprintFilterListBox->GetString( ii );
                                            m_FootprintFilterListBox->SetString( ii, SanitizeFpFilter( filter ) );
                                        }

                                        onModify();
                                        updateMatchingFootprints();
                                    } );

    // The selected event is sent whenever the selection changes, including when it is
    // cleared; the handler always reads the current state.
    m_FootprintFilterListBox->Bind( wxEVT_COMMAND_LISTBOX_SELECTED,
                                    [this]( wxCommandEvent& aEvent )
                                    {
                                        updateMatchingFootprints();
                                    } );

    // Loading the footprint libraries can take a while, so only query them once the page
    // is actually selected.
    aParent->Bind( wxEVT_NOTEBOOK_PAGE_CHANGED, &PANEL_FOOTPRINT_FILTERS::onNotebookPageChanged, this );

    // wxLC_REPORT needs a column and wxLC_NO_HEADER hides the header.  Keep the single
    // column filling the visible width.
    m_matchingFootprints->AppendColumn( wxEmptyString, wxLIST_FORMAT_LEFT, 200 );

    m_matchingFootprints->Bind( wxEVT_SIZE,
                                [this]( wxSizeEvent& aEvent )
                                {
                                    int width = m_matchingFootprints->GetClientSize().GetWidth() - 2;
                                    m_matchingFootprints->SetColumnWidth( 0, std::max( width, 20 ) );
                                    aEvent.Skip();
                                } );

    // Double-clicking a match opens that footprint in the footprint editor.
    m_matchingFootprints->Bind( wxEVT_LIST_ITEM_ACTIVATED, &PANEL_FOOTPRINT_FILTERS::onMatchingFootprintDoubleClick,
                                this );

    m_matchingFootprints->Bind( wxEVT_LIST_ITEM_RIGHT_CLICK, &PANEL_FOOTPRINT_FILTERS::onMatchingFootprintContextMenu,
                                this );

    m_matchingFootprints->Bind( wxEVT_KEY_DOWN, &PANEL_FOOTPRINT_FILTERS::onMatchingFootprintKeyDown, this );

    // The libraries the matches come from are loaded on first use.  Rather than blocking the
    // dialog, the matches panel shows a loading notice and is greyed out until they arrive.
    Bind( wxEVT_TIMER, &PANEL_FOOTPRINT_FILTERS::onFpFilterMatchLoadTimer, this, m_matchLoadTimer.GetId() );

    // Most likely the footprint libraries are already loaded/ing (the parent dialog has already triggered the load)
    // but this is basically free in that case.
    StartFootprintLibrariesLoad( m_kiway );
}


PANEL_FOOTPRINT_FILTERS::~PANEL_FOOTPRINT_FILTERS()
{
    m_matchLoadTimer.Stop();
    Unbind( wxEVT_TIMER, &PANEL_FOOTPRINT_FILTERS::onFpFilterMatchLoadTimer, this, m_matchLoadTimer.GetId() );

    GetParent()->Unbind( wxEVT_NOTEBOOK_PAGE_CHANGED, &PANEL_FOOTPRINT_FILTERS::onNotebookPageChanged, this );
}


void PANEL_FOOTPRINT_FILTERS::SetFilters( const wxArrayString& aFilters )
{
    m_FootprintFilterListBox->Set( aFilters );
}


wxArrayString PANEL_FOOTPRINT_FILTERS::GetFilters() const
{
    return m_FootprintFilterListBox->GetStrings();
}


void PANEL_FOOTPRINT_FILTERS::SetFootprintFieldAccessors( FOOTPRINT_ASSIGN_QUERY aCanAssign,
                                                          FOOTPRINT_ASSIGNER     aAssign )
{
    m_canAssignFootprint = std::move( aCanAssign );
    m_assignFootprint = std::move( aAssign );
}


void PANEL_FOOTPRINT_FILTERS::onModify()
{
    if( DIALOG_SHIM* dlg = dynamic_cast<DIALOG_SHIM*>( wxGetTopLevelParent( this ) ) )
        dlg->OnModify();
}


void PANEL_FOOTPRINT_FILTERS::onNotebookPageChanged( wxBookCtrlEvent& aEvent )
{
    wxNotebook* notebook = dynamic_cast<wxNotebook*>( aEvent.GetEventObject() );

    if( notebook && notebook == GetParent() && aEvent.GetSelection() >= 0
        && notebook->GetPage( aEvent.GetSelection() ) == this )
    {
        updateMatchingFootprints();
    }

    aEvent.Skip();
}


void PANEL_FOOTPRINT_FILTERS::OnFpFilterDClick( wxMouseEvent& aEvent )
{
    int            idx = m_FootprintFilterListBox->HitTest( aEvent.GetPosition() );
    wxCommandEvent dummy;

    if( idx >= 0 )
        OnEditFootprintFilter( dummy );
    else
        OnAddFootprintFilter( dummy );
}


void PANEL_FOOTPRINT_FILTERS::updateMatchingFootprints()
{
    std::vector<wxString> filters;

    for( unsigned ii = 0; ii < m_FootprintFilterListBox->GetCount(); ++ii )
        filters.push_back( m_FootprintFilterListBox->GetString( ii ) );

    std::vector<wxString> patterns;

    if( !m_fpFilterPreview.IsEmpty() )
    {
        // A filter is being typed: preview it rather than the filters in the list.  Its
        // patterns have nowhere in the list to live yet, so leave the cache alone until
        // the edit ends.
        patterns.push_back( m_fpFilterPreview );
    }
    else
    {
        // Filters that were edited away or deleted are gone for good; the other filters
        // keep their cached results.
        m_fpMatcher->PruneCache( filters );

        wxArrayInt selections;
        m_FootprintFilterListBox->GetSelections( selections );

        // With nothing selected, show the union of every filter; otherwise just the
        // selected filters.
        if( selections.IsEmpty() )
        {
            patterns = filters;
        }
        else
        {
            for( int ii : selections )
                patterns.push_back( filters[ii] );
        }
    }

    showFootprintMatches( patterns );
}


void PANEL_FOOTPRINT_FILTERS::showFootprintMatches( const std::vector<wxString>& aPatterns )
{
    // If the footprint libraries are not ready yet, keep the loading indication live.
    if( !aPatterns.empty() && !matchLibrariesReady() )
        return;

    FOOTPRINT_MATCH_RESULT result = m_fpMatcher->GetMatches( aPatterns );

    wxWindowUpdateLocker lock( m_matchingFootprints );
    m_matchingFootprints->DeleteAllItems();

    for( const wxString& footprint : result.m_MatchingNames )
        m_matchingFootprints->InsertItem( m_matchingFootprints->GetItemCount(), footprint );

    wxString labelText;
    if( !result.m_Success )
    {
        labelText = _( "Failed to query matching footprints." );
    }
    else
    {
        wxString countFormat =
                result.m_IsLimited ? _( "Matching footprints (%zu+):" ) : _( "Matching footprints (%zu):" );
        labelText = wxString::Format( countFormat, result.m_MatchingNames.size() );
    }

    m_staticTextFootprints1->SetLabel( labelText );

    Layout();
}


bool PANEL_FOOTPRINT_FILTERS::matchLibrariesReady()
{
    if( m_matchLibrariesReady )
        return true;

    float progress = GetFootprintLibrariesLoadProgress( m_kiway );

    if( progress >= 1.0f )
    {
        m_matchLibrariesReady = true;
        showFpFilterMatchLoadIndication( false );
        return true;
    }

    showFpFilterMatchLoadIndication( true, progress );
    // Go and poll in #onMatchLoadTimer
    m_matchLoadTimer.StartOnce( 250 );
    return false;
}


void PANEL_FOOTPRINT_FILTERS::showFpFilterMatchLoadIndication( bool aShow, float aProgress )
{
    m_matchingFootprints->Enable( !aShow );

    if( aShow )
    {
        // The result count is not known yet
        m_staticTextFootprints1->SetLabel( _( "Matching footprints:" ) );

        // The load reports less than 1.0 until it has finished, so this never reads 100%.
        wxString notice = wxString::Format( _( "Loading footprint libraries (%d%%)..." ),
                                            static_cast<int>( aProgress * 100.0f ) );

        if( m_matchingFootprints->GetItemCount() == 0 )
            m_matchingFootprints->InsertItem( 0, notice );
        else
            m_matchingFootprints->SetItemText( 0, notice );
    }
    else
    {
        m_matchingFootprints->DeleteAllItems();
    }
}


void PANEL_FOOTPRINT_FILTERS::onFpFilterMatchLoadTimer( wxTimerEvent& aEvent )
{
    if( !matchLibrariesReady() )
        return;

    // The filters may have changed while the libraries were loading; update the matches.
    updateMatchingFootprints();
}


void PANEL_FOOTPRINT_FILTERS::onFilterPreviewText( wxCommandEvent& aEvent )
{
    m_fpFilterPreview = SanitizeFpFilter( aEvent.GetString() );
    m_filterPreviewDebounce.Restart();
}


wxString PANEL_FOOTPRINT_FILTERS::matchingFootprintName( long aItem ) const
{
    // Events can have, wxNOT_FOUND as the index.
    if( aItem < 0 || aItem >= m_matchingFootprints->GetItemCount() )
        return wxEmptyString;

    LIB_ID fpId;

    // The list holds "library:footprint" names.
    if( fpId.Parse( m_matchingFootprints->GetItemText( aItem ) ) != -1 || !fpId.IsValid() )
        return wxEmptyString;

    return fpId.Format();
}


void PANEL_FOOTPRINT_FILTERS::openFootprintInEditor( const wxString& aFootprintName )
{
    m_kiway.Player( FRAME_FOOTPRINT_EDITOR, true );

    std::string packet = aFootprintName.utf8_string();
    m_kiway.ExpressMail( FRAME_FOOTPRINT_EDITOR, MAIL_FP_EDIT_LIBID, packet, this );
}


void PANEL_FOOTPRINT_FILTERS::copyFootprintName( const wxString& aFootprintName )
{
    if( wxTheClipboard->Open() )
    {
        wxTheClipboard->SetData( new wxTextDataObject( aFootprintName ) );
        wxTheClipboard->Close();
    }
}


void PANEL_FOOTPRINT_FILTERS::onMatchingFootprintDoubleClick( wxListEvent& aEvent )
{
    wxString fpName = matchingFootprintName( aEvent.GetIndex() );

    if( !fpName.IsEmpty() )
        openFootprintInEditor( fpName );
}


void PANEL_FOOTPRINT_FILTERS::onMatchingFootprintContextMenu( wxListEvent& aEvent )
{
    wxString fpName = matchingFootprintName( aEvent.GetIndex() );

    if( fpName.IsEmpty() )
        return;

    enum
    {
        OPEN_IN_FOOTPRINT_EDITOR = 1,
        ASSIGN_TO_FOOTPRINT_FIELD,
        COPY_FOOTPRINT_NAME
    };

    wxMenu menu;

    menu.Append( OPEN_IN_FOOTPRINT_EDITOR, _( "Open in Footprint Editor" ) );

    // The host dialog's Footprint field cannot always be written (it is read-only when the
    // symbol has no footprint).
    if( m_canAssignFootprint && m_canAssignFootprint() )
        menu.Append( ASSIGN_TO_FOOTPRINT_FIELD, _( "Assign to Footprint Field" ) );

    menu.Append( COPY_FOOTPRINT_NAME, _( "Copy Footprint Name" ) + "\tCtrl+C" );

    switch( GetPopupMenuSelectionFromUser( menu ) )
    {
    case OPEN_IN_FOOTPRINT_EDITOR:
        openFootprintInEditor( fpName );
        break;
    case ASSIGN_TO_FOOTPRINT_FIELD:
        if( m_assignFootprint )
            m_assignFootprint( fpName );

        break;
    case COPY_FOOTPRINT_NAME:
        copyFootprintName( fpName );
        break;
    default:
        break;
    }
}


void PANEL_FOOTPRINT_FILTERS::onMatchingFootprintKeyDown( wxKeyEvent& aEvent )
{
    if( aEvent.ControlDown() && aEvent.GetKeyCode() == 'C' )
    {
        long     item = m_matchingFootprints->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
        wxString fpName = matchingFootprintName( item );

        if( !fpName.IsEmpty() )
        {
            copyFootprintName( fpName );
            return;
        }
    }

    aEvent.Skip();
}


void PANEL_FOOTPRINT_FILTERS::OnAddFootprintFilter( wxCommandEvent& event )
{
    wxString             filterLine;
    WX_TEXT_ENTRY_DIALOG dlg( this, _( "Filter:" ), _( "Add Footprint Filter" ), filterLine );

    // Let the matches list preview what is being typed.
    dlg.Bind( wxEVT_TEXT, &PANEL_FOOTPRINT_FILTERS::onFilterPreviewText, this );

    int modalResult = dlg.ShowModal();

    // The preview is over, so a pending debounce must not fire after it.
    m_filterPreviewDebounce.Cancel();
    m_fpFilterPreview.clear();

    if( modalResult == wxID_CANCEL || dlg.GetValue().IsEmpty() )
    {
        updateMatchingFootprints(); // Discard the live preview
        return;
    }

    filterLine = SanitizeFpFilter( dlg.GetValue() );

    // duplicate filters do no harm, so don't be a nanny.
    m_FootprintFilterListBox->Append( filterLine );
    m_FootprintFilterListBox->SetSelection( (int) m_FootprintFilterListBox->GetCount() - 1 );

    onModify();
    updateMatchingFootprints();
}


void PANEL_FOOTPRINT_FILTERS::OnEditFootprintFilter( wxCommandEvent& event )
{
    wxArrayInt selections;
    const int  n = m_FootprintFilterListBox->GetSelections( selections );

    if( n > 0 )
    {
        // Just edit the first one
        int      idx = selections[0];
        wxString filter = m_FootprintFilterListBox->GetString( idx );

        WX_TEXT_ENTRY_DIALOG dlg( this, _( "Filter:" ), _( "Edit Footprint Filter" ), filter );

        // Let the matches list preview what is being typed.
        dlg.Bind( wxEVT_TEXT, &PANEL_FOOTPRINT_FILTERS::onFilterPreviewText, this );

        int modalResult = dlg.ShowModal();

        // The preview is over, so a pending debounce must not fire after it.
        m_filterPreviewDebounce.Cancel();
        m_fpFilterPreview.clear();

        if( modalResult == wxID_OK && !dlg.GetValue().IsEmpty() )
        {
            m_FootprintFilterListBox->SetString( (unsigned) idx, SanitizeFpFilter( dlg.GetValue() ) );
            onModify();
        }

        updateMatchingFootprints(); // Refresh, also discarding a cancelled preview
    }
}
