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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef EDITOR_TABS_PANEL_H
#define EDITOR_TABS_PANEL_H

#include <functional>
#include <vector>

#include <wx/panel.h>
#include <wx/string.h>

#include <widgets/kicad_tab_art.h>

class EDA_DRAW_PANEL_GAL;
class wxAuiTabCtrl;
class wxAuiNotebookEvent;
class wxBoxSizer;
class wxMouseEvent;


/// GUI-free preview/pin/dirty/close state machine. One unpinned unedited preview tab is reused for
/// the next preview; editing or pinning promotes it to permanent.
class EDITOR_TABS_MODEL
{
public:
    struct ENTRY
    {
        wxString key;
        bool     pinned = false;
        bool     preview = false;
        bool     modified = false;
    };

    /**
     * Return the index to display the document at, reusing a preview slot or appending a new tab.
     */
    int OpenDocument( const wxString& aKey, bool aAsPreview );

    /**
     * Drop the document with @p aKey, freeing the preview slot if it held it.
     */
    void CloseDocument( const wxString& aKey );

    /**
     * Update the modified flag. Turning it on promotes a preview tab to permanent.
     */
    void MarkModified( const wxString& aKey, bool aModified );

    /**
     * Pin or unpin. A pinned tab is exempt from preview replacement.
     */
    void Pin( const wxString& aKey, bool aPinned );

    /**
     * Clear the preview flag so the tab becomes permanent and is no longer reused.
     */
    void Promote( const wxString& aKey );

    /**
     * True when the document has no unsaved edits and can be closed silently.
     */
    bool CanCloseWithoutPrompt( const wxString& aKey ) const;

    int FindIndex( const wxString& aKey ) const;

    /**
     * Index of the current reusable preview tab, or -1 if none.
     */
    int PreviewIndex() const;

    const std::vector<ENTRY>& Entries() const { return m_entries; }

private:
    std::vector<ENTRY> m_entries;
};


/// The tab strip plus the single shared GAL canvas. The canvas is fixed and never reparented after
/// setup. Each open document is one tab keyed by a hidden dummy window; the host owns the documents.
class EDITOR_TABS_PANEL : public wxPanel
{
public:
    EDITOR_TABS_PANEL( wxWindow* aParent, EDA_DRAW_PANEL_GAL* aSharedCanvas );
    ~EDITOR_TABS_PANEL() override;

    /// Host swaps the active document context to the tab at the given index.
    std::function<void( int )> onActivateTab;

    /// Host prompts as needed; return false to veto the close.
    std::function<bool( int )> onCloseTabRequested;

    /// Host reports the visual state (modified/preview/pinned) for the tab at the given index.
    std::function<TAB_VISUAL_STATE( int )> onQueryVisualState;

    /// Host mirrors the pinned flag into its own tab context, which is what gets persisted. Fired for
    /// every pin change so the menu, action, and restore paths all stay in sync through one channel.
    std::function<void( int, bool )> onPinChanged;

    /**
     * Hand the host-owned canvas back to its original parent.
     *
     * The host must call this before the base draw-frame destructor runs so the canvas is freed
     * exactly once. Idempotent.
     */
    void ReleaseSharedCanvas();

    /**
     * When set, a close selects the fallback page visually but does not fire onActivateTab, for
     * hosts that already installed the successor document inside onCloseTabRequested.
     */
    void SetSuppressActivateOnClose( bool aSuppress ) { m_suppressActivateOnClose = aSuppress; }

    int  AddTab( const wxString& aKey, const wxString& aLabel, bool aAsPreview );
    void CloseTab( int aIdx );
    void CloseOthers( int aKeepIdx );
    void CloseToRight( int aIdx );
    void CloseAll();
    void SetPinned( int aIdx, bool aPinned );

    /**
     * Mark the tab modified. An edit also promotes a preview tab to permanent.
     */
    void MarkModified( int aIdx, bool aModified );

    /**
     * Convert a preview tab into a permanent one, dropping its italic styling. No-op otherwise.
     */
    void PromoteTab( int aIdx );

    void SelectTab( int aIdx );
    void AdvanceTab( bool aForward );
    int  GetActiveTab() const;
    int  FindTab( const wxString& aKey ) const;
    void RefreshTabLabels();

    const EDITOR_TABS_MODEL& Model() const { return m_model; }

    /**
     * Read-only view of the Ctrl+Tab MRU order, front is most recent.
     */
    const std::vector<wxString>& MruForTest() const { return m_mru; }

private:
    void onPageChanged( wxAuiNotebookEvent& aEvent );
    void onPageChanging( wxAuiNotebookEvent& aEvent );
    void onPageClose( wxAuiNotebookEvent& aEvent );
    void onPageButton( wxAuiNotebookEvent& aEvent );
    void onTabRightDown( wxAuiNotebookEvent& aEvent );
    void onTabDClick( wxMouseEvent& aEvent );
    void onContextMenu( wxCommandEvent& aEvent );

    /**
     * Close the tab at @p aIdx, prompting the host exactly once.
     */
    void closeTabInternal( int aIdx );

    /**
     * Activate the tab at @p aIdx without re-entering the change handler.
     */
    void activateTab( int aIdx );

    /**
     * Bump @p aKey to the front of the MRU order.
     */
    void touchMru( const wxString& aKey );

    /**
     * Remove @p aKey from the MRU order.
     */
    void forgetMru( const wxString& aKey );

    /**
     * Map a notebook page window pointer to its current tab index, or -1.
     */
    int indexOfWindow( wxWindow* aWindow ) const;

    /**
     * Size the strip to the art provider's measured tab height and re-Layout, since a bare
     * wxAuiTabCtrl reports a near-zero best height and would stay invisible.
     */
    void updateTabStripHeight();

    TAB_VISUAL_STATE visualStateForIndex( int aIdx ) const;

    wxAuiTabCtrl*         m_tabs = nullptr;
    EDA_DRAW_PANEL_GAL*   m_sharedCanvas = nullptr;

    /// The canvas's parent before it was borrowed, reparented back on destruction so it is not freed
    /// as a child of this panel.
    wxWindow*            m_originalCanvasParent = nullptr;

    wxBoxSizer*           m_sizer = nullptr;
    EDITOR_TABS_MODEL     m_model;

    /// Guards activateTab() against re-entrancy from programmatic SetActivePage().
    bool m_activating = false;

    /// When set, closeTabInternal selects the fallback page without firing onActivateTab.
    bool m_suppressActivateOnClose = false;

    /// Hidden per-tab key windows; index-aligned with the model entries.
    std::vector<wxWindow*> m_pageWindows;

    /// Most-recently-used key order for Ctrl+Tab cycling; front is most recent.
    std::vector<wxString>  m_mru;

    int m_contextMenuIdx = -1;
};

#endif // EDITOR_TABS_PANEL_H
