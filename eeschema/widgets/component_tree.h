/* -*- c++ -*-
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2014-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef COMPONENT_TREE_H
#define COMPONENT_TREE_H

#include <wx/panel.h>
#include <cmp_tree_model_adapter.h>

class wxDataViewCtrl;
class wxTextCtrl;
class wxHtmlWindow;
class wxHtmlLinkEvent;
class LIB_ID;
class SYMBOL_LIB_TABLE;


/**
 * Widget displaying a tree of components with optional search text control and description panel.
 */
class COMPONENT_TREE : public wxPanel
{
public:
    ///> Flags to select extra widgets
    enum WIDGETS { NONE = 0x00, SEARCH = 0x01, DETAILS = 0x02, ALL = 0xFF };

    COMPONENT_TREE( wxWindow* aParent, SYMBOL_LIB_TABLE* aSymLibTable,
                    CMP_TREE_MODEL_ADAPTER_BASE::PTR& aAdapter, WIDGETS aWidgets = ALL );

    /**
     * For multi-unit components, if the user selects the component itself
     * rather than picking an individual unit, 0 will be returned in aUnit.
     * Beware that this is an invalid unit number - this should be replaced
     * with whatever default is desired (usually 1).
     *
     * @param aUnit if not NULL, the selected unit is filled in here.
     * @return the library id of the symbol that has been selected.
     */
    LIB_ID GetSelectedLibId( int* aUnit = nullptr ) const;

    /**
     * Select a part in the tree widget.
     *
     * @param aLibId is the identifier of part to be selected.
     */
    void SelectLibId( const LIB_ID& aLibId );

    /**
     * Associates a right click context menu for a specific node type.
     * @param aType is the node type to have a menu associated.
     * @param aMenu is the associated menu.
     */
    void SetMenu( CMP_TREE_NODE::TYPE aType, std::unique_ptr<wxMenu> aMenu )
    {
        m_menus[aType] = std::move( aMenu );
    }

    /**
     * Returns the status of right-click context menu.
     * @return True in case a right-click context menu is active.
     */
    bool IsMenuActive() const
    {
        return m_menuActive;
    }

    /**
     * Regenerates the tree.
     */
    void Regenerate();

protected:
    /**
     * If a wxDataViewitem is valid, select it and post a selection event.
     */
    void selectIfValid( const wxDataViewItem& aTreeId );

    /**
     * Post a wxEVT_DATAVIEW_SELECTION_CHANGED to notify the selection handler
     * that a new part has been preselected.
     */
    void postPreselectEvent();

    /**
     * Post COMPONENT_SELECTED event to notify the selection handler that a part has been selected.
     */
    void postSelectEvent();

    /**
     * Structure storing state of the component tree widget.
     */
    struct STATE
    {
        ///> List of expanded nodes
        std::vector<wxDataViewItem> expanded;

        ///> Current selection, might be not valid if nothing was selected
        LIB_ID selection;
    };

    /**
     * Returns the component tree widget state.
     */
    STATE getState() const;

    /**
     * Restores the component tree widget state from an object.
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

    SYMBOL_LIB_TABLE* m_sym_lib_table;
    CMP_TREE_MODEL_ADAPTER_BASE::PTR m_adapter;

    wxTextCtrl*     m_query_ctrl;
    wxDataViewCtrl* m_tree_ctrl;
    wxHtmlWindow*   m_details_ctrl;

    ///> Right click context menus for each tree level
    std::vector<std::unique_ptr<wxMenu>> m_menus;

    ///> Flag indicating whether a right-click context menu is active
    bool m_menuActive;

    ///> Flag indicating whether the results are filtered using the search query
    bool m_filtering;

    ///> State of the widget before any filters applied
    STATE m_unfilteredState;
};

///> Custom event sent when a new component is preselected
wxDECLARE_EVENT( COMPONENT_PRESELECTED, wxCommandEvent );

///> Custom event sent when a component is selected
wxDECLARE_EVENT( COMPONENT_SELECTED, wxCommandEvent );

#endif /* COMPONENT_TREE_H */
