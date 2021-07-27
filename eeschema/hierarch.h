/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef HIERARCH_H
#define HIERARCH_H

#include <dialog_shim.h>
#include <wx/imaglist.h>
#include <wx/object.h> // wxRTTI macros
#include <wx/treectrl.h>

// The window name of the hierarchy navigator, used to find it
#define HIERARCHY_NAVIG_DLG_WNAME "hierarchy_navig_dlg"

class SCH_EDIT_FRAME;
class SCH_SHEET_PATH;

class HIERARCHY_NAVIG_DLG;

/**
 * Handle hierarchy tree control.
 */
class HIERARCHY_TREE : public wxTreeCtrl
{
public:
    HIERARCHY_TREE( HIERARCHY_NAVIG_DLG* parent );

    // Closes the dialog on escape key
    void onChar( wxKeyEvent& event );

    int OnCompareItems( const wxTreeItemId& item1, const wxTreeItemId& item2 ) override;

private:
    // Need to use wxRTTI macros in order for OnCompareItems to work properly
    // See: https://docs.wxwidgets.org/3.1/classwx_tree_ctrl.html#ab90a465793c291ca7aa827a576b7d146
    wxDECLARE_ABSTRACT_CLASS( HIERARCHY_TREE );

    HIERARCHY_NAVIG_DLG* m_parent;
    wxImageList*         imageList;
};

class HIERARCHY_NAVIG_DLG : public DIALOG_SHIM
{
public:
    HIERARCHY_NAVIG_DLG( SCH_EDIT_FRAME* aParent );

    ~HIERARCHY_NAVIG_DLG();

    void OnCloseNav( wxCloseEvent& event );

    /**
     * Update the hierarchical tree of the schematic.
     */
    void UpdateHierarchyTree();

private:
    /**
     * Create the hierarchical tree of the schematic.
     *
     * @warning This routine is reentrant!
     *
     * @param[in] aList is the #SCH_SHEET_PATH list to explore.
     * @param aPreviousmenu is the wxTreeItemId used as parent to add sub items.
     */
    void buildHierarchyTree( SCH_SHEET_PATH* aList, wxTreeItemId* aPreviousmenu );

    /**
     * Open the selected sheet and display the corresponding screen when a tree item is
     * selected.
     */
    void onSelectSheetPath( wxTreeEvent& event );

    /**
     * @return String with page number in parenthesis
     */
    wxString getRootString();

    /**
     * @return String with page number in parenthesis
    */
    wxString formatPageString( const wxString& aName, const wxString& aPage );

    SCH_SHEET_PATH  m_currSheet;
    SCH_SHEET_PATH  m_list;
    SCH_EDIT_FRAME* m_SchFrameEditor;
    HIERARCHY_TREE* m_Tree;
    int             m_nbsheets;
};

#endif // HIERARCH_H
