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
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef PAGED_DIALOG_H
#define PAGED_DIALOG_H

#include <dialog_shim.h>
#include <widgets/wx_treebook.h>

#include <optional>
#include <set>

#include <wx/weakref.h>


class WX_INFOBAR;
class WX_TREEBOOK;
class wxSearchCtrl;

class PAGED_DIALOG : public DIALOG_SHIM
{
public:
    PAGED_DIALOG( wxWindow* aParent, const wxString& aTitle, bool aShowReset, bool aShowOpenFolder,
                  const wxString& aAuxiliaryAction = wxEmptyString,
                  const wxSize&   aInitialSize = wxDefaultSize );
    ~PAGED_DIALOG() override;

    WX_TREEBOOK* GetTreebook() { return m_treebook; }

    void SetInitialPage( const wxString& aPage, const wxString& aParentPage = wxEmptyString );

    /**
     * Select the page with the given title.
     *
     * @return the page, resolved if it was created lazily, or nullptr where no page carries
     *         that title or the page is not the one now showing.
     */
    wxWindow* ShowPage( const wxString& aTitle );

    void SetModified() { m_modified = true; }

    void SetError( const wxString& aMessage, const wxString& aPageName, int aCtrlId, int aRow = -1,
                   int aCol = -1 );

    void SetError( const wxString& aMessage, wxWindow* aPage, wxWindow* aCtrl, int aRow = -1,
                   int aCol = -1 );

    void UpdateResetButton( int aPage );

    WX_INFOBAR* GetInfoBar() { return m_infoBar; }

    static PAGED_DIALOG* GetDialog( wxWindow* aWindow );

    /// Keep aWindow and its children out of the search index, for text the tree already shows
    /// or that names something the page does not own. Does nothing outside a PAGED_DIALOG.
    static void ExcludeFromSearchIndex( wxWindow* aWindow );

    /// Keep the tab titles of aBook out of the index, where the user named them. The controls
    /// on each tab are still indexed.
    static void ExcludeBookTitlesFromSearchIndex( wxBookCtrlBase* aBook );

    /// Refresh the cached labels of the page holding aWindow, after a label is renamed or a
    /// control is added, removed or first shown. Without it the page matches its old wording.
    static void InvalidateSearchIndex( wxWindow* aWindow );

    /**
     * Keep the labels of aPages for the next dialog of the same aScope, so reopening does not
     * read them again. A different aIdentity, which names the project they were read from,
     * replaces what was kept. Call it once the tree is complete, as the pages are resolved to
     * tree paths here.
     */
    void ReuseSearchLabels( const wxString& aScope, const wxString& aIdentity, const std::set<size_t>& aPages );

    /**
     * Destroy each page the search builds once its labels and values are read, instead of keeping
     * every page of the dialog built. Only for dialogs whose pages hold no pointers to each other.
     */
    void ReleasePagesAfterIndexing() { m_releaseIndexedPages = true; }

protected:
    void finishInitialization();

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    /// Selects the page holding aCtrl, so the step lands on a page that is read when it is left
    bool showHistoryControl( wxWindow* aCtrl ) override;

    virtual void onAuxiliaryAction( wxCommandEvent& aEvent ) { aEvent.Skip(); }
    virtual void onResetButton( wxCommandEvent& aEvent );
    virtual void onOpenPrefsDir( wxCommandEvent& aEvent );
    virtual void onPageChanged( wxBookCtrlEvent& aEvent );
    virtual void onPageChanging( wxBookCtrlEvent& aEvent );
    virtual void onCharHook( wxKeyEvent& aEvent );
    void         onTreeLeftDown( wxMouseEvent& aEvent );
    void         onTreeItemActivated( wxTreeEvent& aEvent );

    void onSearchText( wxCommandEvent& aEvent );
    void onSearchEnter( wxCommandEvent& aEvent );
    void onSearchCharHook( wxKeyEvent& aEvent );

    /**
     * Append the current values on page aPage, which can change after its labels were read.
     * The base reads the controls of a built page and never builds one, so a dialog that reuses
     * labels for an unbuilt page must supply that page's values from its own data.
     */
    virtual void AppendSearchTerms( size_t aPage, std::vector<wxString>& aTerms ) const;

    /// Called before AppendSearchTerms() is asked about each page for one query
    virtual void BeginSearchPass() {}

    /**
     * Read current control values and choices without committing edits.
     * Set aIncludeValues to false when a page supplies its values from a separate edit buffer.
     */
    static void AppendControlSearchTerms( wxWindow* aWindow, std::vector<wxString>& aTerms,
                                          bool aIncludeValues = true );

    /**
     * Replace the first aShown among aTerms from aFirst on with aCurrent, or append aCurrent when
     * aShown is not there. A page not shown uses it for a value changed since the page last read it.
     */
    static void ReplaceSearchTerm( std::vector<wxString>& aTerms, size_t aFirst, const wxString& aShown,
                                   const wxString& aCurrent );

    /// Terms for a page listing the project's text variables, which it reads rather than owns.
    void AppendTextVarSearchTerms( std::vector<wxString>& aTerms ) const;

    WX_TREEBOOK* m_treebook;
    wxButton*    m_auxiliaryButton;
    wxButton*    m_resetButton;
    wxButton*    m_openPrefsDirButton;
    WX_INFOBAR*  m_infoBar;

private:
    void excludeFromSearchIndex( wxWindow* aWindow );
    void excludeBookTitlesFromSearchIndex( wxBookCtrlBase* aBook );
    void invalidateSearchIndex( wxWindow* aWindow );

    enum class SEARCH_ACTION
    {
        REFRESH,
        SELECT_PAGE,
        FOCUS_TREE
    };

    /// Tree path used to identify collapsed sections.
    wxString pagePath( size_t aPage ) const;

    std::set<wxString> collapsedNodes() const;
    void               saveTreeExpansion() const;
    void               loadTreeExpansion();
    void               restoreTreeExpansion( const std::set<wxString>& aCollapsed );

    bool isExcludedFromSearch( const wxWindow* aWindow ) const;

    /// Tree path of a page, lowercased, prefixed by its parent so "schematic colors" finds Colors
    wxString              pageSearchTitle( size_t aPage ) const;
    void                  onSearchIdle( wxIdleEvent& aEvent );
    void                  collectSearchText( wxWindow* aWindow, std::vector<wxString>& aTexts ) const;
    std::vector<wxString> buildPageSearchIndex( size_t aPage );
    void                  indexPage( size_t aPage );
    void                  refreshSearch();
    void                  refadeSearchMisses();
    void                  updateSearch( SEARCH_ACTION aAction );

    /**
     * Bold the pages matching the search box and grey out the rest.
     * @return the page to show: the current page if it matches, else the first matching page with controls,
     *         or wxNOT_FOUND when nothing matches.
     */
    int applySearch();

    wxString    m_title;

    wxSearchCtrl*                      m_searchCtrl;
    std::vector<std::vector<wxString>> m_searchIndex; ///< Lowercased page title and labels, per page
    std::vector<wxWeakRef<wxWindow>>   m_searchExcludedControls;
    std::vector<wxWeakRef<wxWindow>>   m_searchExcludedBookTitles;
    wxString                           m_searchReuseScope;
    std::set<wxString>                 m_reusableSearchPaths;
    std::vector<bool>                  m_searchLabelsPending; ///< Pages still waiting for their labels
    std::vector<std::vector<wxString>> m_releasedPageValues;  ///< Lowercased values of released pages
    bool                               m_releaseIndexedPages = false;
    bool                               m_indexingPage = false;
    bool                               m_indexingSearch = false;
    SEARCH_ACTION                      m_pendingSearchAction = SEARCH_ACTION::REFRESH;
    bool                               m_selectingSearchMatch = false;

    /// Sections collapsed before the query, restored when it is cleared
    std::optional<std::set<wxString>> m_collapsedBeforeSearch;

    wxBoxSizer* m_buttonsSizer;

    std::vector<bool> m_macHack;

    wxTreeItemId      m_clickExpandedItem;
};


#endif //PAGED_DIALOG_H
