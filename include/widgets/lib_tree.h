/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
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

#ifndef LIB_TREE_H
#define LIB_TREE_H

#include <wx/panel.h>
#include <wx/timer.h>
#include <lib_tree_model_adapter.h>
#include <widgets/html_window.h>
#include <widgets/wx_dataviewctrl.h>

class wxTextCtrl;
class wxHtmlLinkEvent;
class wxSearchCtrl;
class wxTimer;
class wxTimerEvent;
class wxPopupWindow;
class STD_BITMAP_BUTTON;
class ACTION_MENU;
class LIB_ID;

/**
 * Widget displaying a tree of symbols with optional search text control and description panel.
 */
class LIB_TREE : public wxPanel
{
public:
    ///< Flags to select extra widgets and options
    enum FLAGS
    {
        FLAGS_NONE  = 0x00,
        SEARCH      = 0x01,
        FILTERS     = 0x02,
        DETAILS     = 0x04,
        ALL_WIDGETS = 0x0F,
        MULTISELECT = 0x10
    };

    /**
     * Construct a symbol tree.
     *
     * @param aParent parent window containing this tree widget.
     * @param aRecentSearchesKey a key into a global map storing recent searches (usually "power",
     *                           "symbols", or "footprints", but could be further differentiated).
     * @param aLibTable table containing libraries and items to display.
     * @param aAdapter a LIB_TREE_MODEL_ADAPTER instance to use.
     * @param aFlags selection of sub-widgets to include and other options.
     * @param aDetails if not null, a custom HTML_WINDOW to hold symbol details. If null this
     *                 will be created inside the LIB_TREE.
     */
    LIB_TREE( wxWindow* aParent, const wxString& aRecentSearchesKey,
              wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>& aAdapter, int aFlags = ALL_WIDGETS,
              HTML_WINDOW* aDetails = nullptr );

    ~LIB_TREE() override;

    /**
     * For multi-unit symbols, if the user selects the symbol itself
     * rather than picking an individual unit, 0 will be returned in aUnit.
     * Beware that this is an invalid unit number - this should be replaced
     * with whatever default is desired (usually 1).
     *
     * @param aUnit if not NULL, the selected unit is filled in here.
     * @return the library id of the symbol that has been selected.
     */
    LIB_ID GetSelectedLibId( int* aUnit = nullptr ) const;

    int GetSelectionCount() const
    {
        return m_tree_ctrl->GetSelectedItemsCount();
    }

    /**
     * Retrieve a list of selections for trees that allow multi-selection.
     *
     * @see GetSelectedLibId for details on how aUnit will be filled.
     * @param aSelection will be filled with a list of selected LIB_IDs.
     * @param aUnit is an optional pointer to a list to fill with unit numbers.
     * @return the number of selected items.
     */
    int GetSelectedLibIds( std::vector<LIB_ID>& aSelection,
                           std::vector<int>* aUnit = nullptr ) const;

    /**
     * Retrieve the tree node for the first selected item.
     *
     * @return the tree node for the first selected item.
     */
    LIB_TREE_NODE* GetCurrentTreeNode() const;

    /**
     * Retrieve a list of pointers to selected tree nodes for trees that allow multi-selection.
     * 
     * @param aSelection will be filled with a list of pointers of selected tree nodes.
     * @return the number of selected items.
     */
    int GetSelectedTreeNodes( std::vector<LIB_TREE_NODE*>& aSelection ) const;

    /**
     * Select an item in the tree widget.
     */
    void SelectLibId( const LIB_ID& aLibId );

    /**
     * Ensure that an item is visible (preferably centered).
     */
    void CenterLibId( const LIB_ID& aLibId );

    /**
     * Unselect currently selected item in wxDataViewCtrl.
     */
    void Unselect();

    /**
     * Expand and item i the tree widget.
     */
    void ExpandLibId( const LIB_ID& aLibId );

    void ExpandAll();
    void CollapseAll();

    /**
     * Save/restore search string.
     */
    void SetSearchString( const wxString& aSearchString );
    wxString GetSearchString() const;

    /**
     * Save/restore the sorting mode.
     */
    void SetSortMode( LIB_TREE_MODEL_ADAPTER::SORT_MODE aMode ) { m_adapter->SetSortMode( aMode ); }
    LIB_TREE_MODEL_ADAPTER::SORT_MODE GetSortMode() const { return m_adapter->GetSortMode(); }

    /**
     * Regenerate the tree.
     */
    void Regenerate( bool aKeepState );

    /**
     * Refresh the tree (mainly to update highlighting and asterisking)
     */
    void RefreshLibTree();

    wxWindow* GetFocusTarget();

    wxSizer* GetFiltersSizer() { return m_filtersSizer; }

    /**
     * Focus the search widget if it exists.
     */
    void FocusSearchFieldIfExists();

    void ShowChangedLanguage();

    void BlockPreview( bool aBlock )
    {
        m_previewDisabled = aBlock;
    }

    void ShutdownPreviews();

protected:
    /**
     * Expand or collapse a node, switching it to the opposite state.
     */
    void toggleExpand( const wxDataViewItem& aTreeId );

    /**
     * If a wxDataViewitem is valid, select it and post a selection event.
     */
    void selectIfValid( const wxDataViewItem& aTreeId );

    void centerIfValid( const wxDataViewItem& aTreeId );

    void expandIfValid( const wxDataViewItem& aTreeId );

    /**
     * Post a wxEVT_DATAVIEW_SELECTION_CHANGED to notify the selection handler
     * that a new part has been preselected.
     */
    void postPreselectEvent();

    /**
     * Post #SYMBOL_SELECTED event to notify the selection handler that a part has been selected.
     */
    void postSelectEvent();

    /**
     * Structure storing state of the symbol tree widget.
     */
    struct STATE
    {
        /// List of expanded nodes.
        std::vector<wxDataViewItem> expanded;

        /// Current selection, might be not valid if nothing was selected.
        LIB_ID selection;

        VECTOR2I scrollpos;
    };

    /**
     * Return the symbol tree widget state.
     */
    STATE getState() const;

    /**
     * Restore the symbol tree widget state from an object.
     */
    void setState( const STATE& aState );

    void updateRecentSearchMenu();

    void showPreview( wxDataViewItem aItem );
    void hidePreview();

    void onQueryText( wxCommandEvent& aEvent );
    void onQueryCharHook( wxKeyEvent& aEvent );
    void onQueryMouseMoved( wxMouseEvent& aEvent );

    void onTreeSelect( wxDataViewEvent& aEvent );
    void onTreeActivate( wxDataViewEvent& aEvent );
    void onTreeCharHook( wxKeyEvent& aEvent );

    void onIdle( wxIdleEvent& aEvent );
    void onHoverTimer( wxTimerEvent& aEvent );

    void onDetailsLink( wxHtmlLinkEvent& aEvent );
    void onPreselect( wxCommandEvent& aEvent );
    void onItemContextMenu( wxDataViewEvent& aEvent );
    void onHeaderContextMenu( wxDataViewEvent& aEvent );

    void onDebounceTimer( wxTimerEvent& aEvent );

protected:
    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> m_adapter;

    wxSearchCtrl*      m_query_ctrl;
    BITMAP_BUTTON*     m_sort_ctrl;
    WX_DATAVIEWCTRL*   m_tree_ctrl;
    HTML_WINDOW*       m_details_ctrl;
    wxTimer*           m_debounceTimer;
    bool               m_inTimerEvent;

    wxString           m_recentSearchesKey;

    wxBoxSizer*        m_filtersSizer;

    bool               m_skipNextRightClick;

    wxPoint            m_hoverPos;
    wxDataViewItem     m_hoverItem;
    wxRect             m_hoverItemRect;
    wxTimer            m_hoverTimer;
    wxDataViewItem     m_previewItem;
    wxRect             m_previewItemRect;
    wxPopupWindow*     m_previewWindow;
    bool               m_previewDisabled;
};

/// Custom event sent when an item is selected in the list.
wxDECLARE_EVENT( EVT_LIBITEM_SELECTED, wxCommandEvent );

/// Custom event sent when an item is chosen (double-clicked).
wxDECLARE_EVENT( EVT_LIBITEM_CHOSEN, wxCommandEvent );

#endif /* LIB_TREE_H */
