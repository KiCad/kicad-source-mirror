/**
 * @file tree_project_frame.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2009 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 1992-2009 KiCad Developers, see change_log.txt for contributors.
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

#ifndef TREEPRJ_FRAME_H
#define TREEPRJ_FRAME_H

class TREEPROJECT_ITEM;

/** class TREE_PROJECT_FRAME
 * Window to display the tree files
 */
class TREE_PROJECT_FRAME : public wxSashLayoutWindow
{
private:

    std::vector<wxMenu*>  m_ContextMenus;
    std::vector<wxString> m_Filters;

    wxMenu*  m_PopupMenu;
    wxCursor m_DragCursor;
    wxCursor m_Default;

protected:
    wxMenu*          GetContextMenu( int type );

    /**
     * Function GetSelectedData
     * return the item data from item currently selected (highlighted)
     * Note this is not necessary the "clicked" item,
     * because when expanding, collapsing an item this item is not selected
     */
    TREEPROJECT_ITEM* GetSelectedData();
    /**
     * Function GetItemIdData
     * return the item data corresponding to a wxTreeItemId identifier
     * @param  aId = the wxTreeItemId identifier.
     * @return a TREEPROJECT_ITEM pointer correspondinfg to item id aId
     */
    TREEPROJECT_ITEM* GetItemIdData(wxTreeItemId aId);

public:
    KICAD_MANAGER_FRAME* m_Parent;
    TREEPROJECTFILES*   m_TreeProject;

    wxTreeItemId      m_root;

public:
    static wxString              GetFileExt( TreeFileType type );
    static wxString              GetFileWildcard( TreeFileType type );

    TREE_PROJECT_FRAME( KICAD_MANAGER_FRAME* parent );
    ~TREE_PROJECT_FRAME();
    void                         OnSelect( wxTreeEvent& Event );
    void                         OnExpand( wxTreeEvent& Event );
    void                         OnRenameAsk( wxTreeEvent& Event );
    void                         OnRename( wxTreeEvent& Event );
    void                         OnDragStart( wxTreeEvent& event );
    void                         OnDragEnd( wxTreeEvent& event );
    void                         OnRight( wxTreeEvent& Event );
    void                         ReCreateTreePrj();

    void                         OnTxtEdit( wxCommandEvent& event );

    void                         OnDeleteFile( wxCommandEvent& event );
    void                         OnRenameFile( wxCommandEvent& event );

    /**
     * Function OnCreateNewDirectory
     * Creates a new subdirectory inside the current kicad project directory
     * the user is prompted to enter a directory name
     */
    void                         OnCreateNewDirectory( wxCommandEvent& event );

    void                         ClearFilters();

    const std::vector<wxString>& GetFilters();
    void                         RemoveFilter( const wxString& filter );

   /**
     * Function AddFileToTree
     * @brief  Add the file or directory aName to the project tree
     * @param aName = the filename or the directory name to add in tree
     * @param aRoot = the wxTreeItemId item where to add sub tree items
     * @param aRecurse = true to filenames or sub dir names to the current tree item
     *                   false to stop file add.
     * @return true if the file (or directory) is added.
     */
     bool                  AddFileToTree( const wxString& aName,
                                   wxTreeItemId& aRoot, bool aRecurse = true);

    DECLARE_EVENT_TABLE()
};

#endif  // TREEPRJ_FRAME_H
