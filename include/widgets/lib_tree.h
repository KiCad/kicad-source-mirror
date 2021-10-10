/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2014-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <lib_tree_model_adapter.h>
#include <html_window.h>

class wxDataViewCtrl;
class wxTextCtrl;
class wxHtmlLinkEvent;
class wxSearchCtrl;
class wxTimer;
class wxTimerEvent;
class ACTION_MENU;
class LIB_ID;
class LIB_TABLE;

/**
 * Widget displaying a tree of symbols with optional search text control and description panel.
 */
class LIB_TREE : public wxPanel
{
public:
    ///< Flags to select extra widgets
    enum WIDGETS { NONE = 0x00, SEARCH = 0x01, DETAILS = 0x02, ALL = 0xFF };

    /**
     * Construct a symbol tree.
     *
     * @param aParent parent window containing this tree widget
     * @param aLibTable table containing libraries and items to display
     * @param aAdapter a LIB_TREE_MODEL_ADAPTER instance to use
     * @param aWidgets selection of sub-widgets to include
     * @param aDetails if not null, a custom HTML_WINDOW to hold symbol details. If null this
     *                 will be created inside the LIB_TREE.
     */
    LIB_TREE( wxWindow* aParent, LIB_TABLE* aLibTable,
              wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>& aAdapter, WIDGETS aWidgets = ALL,
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

    LIB_TREE_NODE* GetCurrentTreeNode() const;

    /**
     * Select an item in the tree widget.
     */
    void SelectLibId( const LIB_ID& aLibId );

    /**
     * Ensure that an item is visible (preferably centered).
     */
    void CenterLibId( const LIB_ID& aLibId );

    /**
     * Unselect currently selected item in wxDataViewCtrl
     */
    void Unselect();

    /**
     * Expand and item i the tree widget.
     */
    void ExpandLibId( const LIB_ID& aLibId );

    /**
     * Regenerate the tree.
     */
    void Regenerate( bool aKeepState );

    /**
     * Refreshes the tree (mainly to update highlighting and asterisking)
     */
    void RefreshLibTree();

    wxWindow* GetFocusTarget();

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
     * Post SYMBOL_SELECTED event to notify the selection handler that a part has been selected.
     */
    void postSelectEvent();

    /**
     * Structure storing state of the symbol tree widget.
     */
    struct STATE
    {
        ///< List of expanded nodes
        std::vector<wxDataViewItem> expanded;
        std::vector<wxString>       pinned;

        ///< Current selection, might be not valid if nothing was selected
        LIB_ID selection;
    };

    /**
     * Return the symbol tree widget state.
     */
    STATE getState() const;

    /**
     * Restore the symbol tree widget state from an object.
     */
    void setState( const STATE& aState );

    void onQueryText( wxCommandEvent& aEvent );
    void onQueryEnter( wxCommandEvent& aEvent );
    void onQueryCharHook( wxKeyEvent& aEvent );

    void onTreeSelect( wxDataViewEvent& aEvent );
    void onTreeActivate( wxDataViewEvent& aEvent );

    void onDetailsLink( wxHtmlLinkEvent& aEvent );
    void onPreselect( wxCommandEvent& aEvent );
    void onContextMenu( wxDataViewEvent& aEvent );

    void onDebounceTimer( wxTimerEvent& aEvent );

protected:
    LIB_TABLE*       m_lib_table;

    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> m_adapter;

    wxSearchCtrl*    m_query_ctrl;
    wxDataViewCtrl*  m_tree_ctrl;
    HTML_WINDOW*     m_details_ctrl;
    wxTimer*         m_debounceTimer;

    LIB_ID           m_last_libid;
};

///< Custom event sent when a new symbol is preselected
wxDECLARE_EVENT( SYMBOL_PRESELECTED, wxCommandEvent );

///< Custom event sent when a symbol is selected
wxDECLARE_EVENT( SYMBOL_SELECTED, wxCommandEvent );

#endif /* LIB_TREE_H */
