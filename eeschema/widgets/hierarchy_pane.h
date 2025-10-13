/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2022 Mike Williams <mike at mikebwilliams.com>
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


#ifndef HIERARCHY_PANE_H
#define HIERARCHY_PANE_H

#include <wx/imaglist.h>
#include <wx/object.h> // wxRTTI macros
#include <wx/treectrl.h>
#include <set>
#include <vector>
#include "widgets/wx_panel.h"


class SCH_COMMIT;
class SCH_EDIT_FRAME;
class SCH_SHEET_PATH;

class HIERARCHY_PANE;


/**
 * Navigation hierarchy tree control.
 *
 * wxTreeCtrl must be subclassed to implement the OnCompareItems method
 * to sort according to page numbers.
 */
class HIERARCHY_TREE : public wxTreeCtrl
{
public:
    HIERARCHY_TREE( HIERARCHY_PANE* parent ) :
            wxTreeCtrl( (wxWindow*) parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                        wxTR_HAS_BUTTONS | wxTR_EDIT_LABELS | wxTR_HIDE_ROOT, wxDefaultValidator,
                        wxT( "HierachyTreeCtrl" ) )
    {
    }

    int OnCompareItems( const wxTreeItemId& item1, const wxTreeItemId& item2 ) override;

private:
    // Need to use wxRTTI macros in order for OnCompareItems to work properly
    // See: https://docs.wxwidgets.org/3.1/classwx_tree_ctrl.html#ab90a465793c291ca7aa827a576b7d146
    wxDECLARE_ABSTRACT_CLASS( HIERARCHY_TREE );
};


class HIERARCHY_PANE : public WX_PANEL
{
public:
    enum ContextMenuAction
    {
        EDIT_PAGE_NUMBER,
        EXPAND_ALL,
        COLLAPSE_ALL,
        RENAME,
        NEW_TOP_LEVEL_SHEET,
        DELETE_TOP_LEVEL_SHEET
    };

    HIERARCHY_PANE( SCH_EDIT_FRAME* aParent );

    ~HIERARCHY_PANE();

    /**
     * Update the hierarchical tree of the schematic.
     */
    void UpdateHierarchyTree( bool aClear = false );

    /**
     * Updates the tree's selection to match current page
     */
    void UpdateHierarchySelection();

    /**
     * Update the labels of the hierarchical tree of the schematic.
     * Must be called only for an up to date tree, to update displayed labels after
     * a sheet name or a sheet number change.
     */
    void UpdateLabelsHierarchyTree();

    /**
     * Returns a list of sheet paths for nodes that are currently collapsed.
     */
    std::vector<wxString> GetCollapsedPaths() const;

private:
    /**
     * Create the hierarchical tree of the schematic.
     *
     * @warning This routine is reentrant!
     *
     * @param[in] aList is the #SCH_SHEET_PATH list to explore.
     * @param aPreviousmenu is the wxTreeItemId used as parent to add sub items.
     */
    void buildHierarchyTree( SCH_SHEET_PATH* aList, const wxTreeItemId& aParent );

    /**
     * Open the selected sheet and display the corresponding screen when a tree item is
     * selected.
     */
    void onSelectSheetPath( wxTreeEvent& aEvent );

    void onTreeItemRightClick( wxTreeEvent& aEvent );
    void onRightClick( wxTreeItemId aItem );
    void onContextMenu( wxContextMenuEvent& aEvent );
    void onCharHook( wxKeyEvent& aKeyStroke );
    void onTreeRightClick( wxTreeEvent& event );
    void onTreeEditFinished( wxTreeEvent& event );

    /**
     * @return String with page number in parenthesis
     */
    wxString getRootString();

    /**
     * @return String with page number in parenthesis
    */
    wxString formatPageString( const wxString& aName, const wxString& aPage );

    /**
     * When renaming the sheets in tree it is helpful to highlight
     * the identical sheets which got renamed by renaming the current sheet.
     */
    void setIdenticalSheetsHighlighted( const SCH_SHEET_PATH& path, bool highLighted = true );

    /**
     * Rename all sheets in a hierarchial desing which has the same source renamed sheet
     */
    void renameIdenticalSheets( const SCH_SHEET_PATH& renamedSheet, const wxString newName,
                                SCH_COMMIT* commit );

private:
    SCH_SHEET_PATH  m_list;
    SCH_EDIT_FRAME* m_frame;
    HIERARCHY_TREE* m_tree;

    bool            m_events_bound;
    std::set<wxString> m_collapsedPaths;
};

#endif // HIERARCHY_PANE_H
