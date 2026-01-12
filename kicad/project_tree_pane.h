/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/**
 * @file project_tree_pane.h
 */


#ifndef TREEPRJ_FRAME_H
#define TREEPRJ_FRAME_H

#include <map>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <future>
#include <wx/datetime.h>
#include <wx/fswatcher.h>
#include <wx/laywin.h>
#include <wx/timer.h>
#include <wx/treebase.h>

#include <git/kicad_git_common.h>
#include "tree_file_type.h"


class KICAD_MANAGER_FRAME;
class PROJECT_TREE_ITEM;
class PROJECT_TREE;

/** PROJECT_TREE_PANE
 * Window to display the tree files
 */
class PROJECT_TREE_PANE : public wxSashLayoutWindow
{
    friend class PROJECT_TREE_ITEM;

public:
    PROJECT_TREE_PANE( KICAD_MANAGER_FRAME* parent );
    ~PROJECT_TREE_PANE();

    /**
     * Create or modify the tree showing project file names
     */
    void ReCreateTreePrj();

    /**
     * Reinit the watched paths
     * Should be called after opening a new project to
     * rebuild the list of watched paths.
     * Should be called *after* the main loop event handler is started
     */
    void FileWatcherReset();

    /**
     * Delete all @ref m_TreeProject entries
     */
    void EmptyTreePrj();

protected:
    static wxString GetFileExt( TREE_FILE_TYPE type );

    /**
     * Function GetSelectedData
     * return the item data from item currently selected (highlighted)
     * Note this is not necessary the "clicked" item,
     * because when expanding, collapsing an item this item is not selected
     */
    std::vector<PROJECT_TREE_ITEM*> GetSelectedData();

    /**
     * Function GetItemIdData
     * return the item data corresponding to a wxTreeItemId identifier
     * @param  aId = the wxTreeItemId identifier.
     * @return a PROJECT_TREE_ITEM pointer corresponding to item id aId
     */
    PROJECT_TREE_ITEM* GetItemIdData( wxTreeItemId aId );

private:
    /**
     * Called on a double click on an item
     */
    void onSelect( wxTreeEvent& Event );

    /**
     * Called on a click on the + or - button of an item with children
     */
    void onExpand( wxTreeEvent& Event );

    /**
     * Called on a right click on an item
     */
    void onRight( wxTreeEvent& Event );

    /**
     * Function onOpenSelectedFileWithTextEditor
     * Call the text editor to open the selected file in the tree project
     */
    void onOpenSelectedFileWithTextEditor( wxCommandEvent& event );

    /**
     * Function onDeleteFile
     * Delete the selected file or directory in the tree project
     */
    void onDeleteFile( wxCommandEvent& event );

    /**
     * Function onRenameFile
     * Rename the selected file or directory in the tree project
     */
    void onRenameFile( wxCommandEvent& event );

    /**
     * Function onOpenDirectory
     * Handles the right-click menu for opening a directory in the current system file browser
     */
    void onOpenDirectory( wxCommandEvent& event );

    /**
     * Function onCreateNewDirectory
     * Creates a new subdirectory inside the current kicad project directory the user is
     * prompted to enter a directory name
     */
    void onCreateNewDirectory( wxCommandEvent& event );

    /**
     * Switch to a other project selected from the tree project (by selecting an other .pro
     * file inside the current project folder)
     */
    void onSwitchToSelectedProject( wxCommandEvent& event );

    /**
     * Idle event handler, used process the selected items at a point in time
     * when all other events have been consumed
     */
    void onIdle( wxIdleEvent& aEvent );

    /**
     * We don't have uniform borders so we have to draw them ourselves.
     */
    void onPaint( wxPaintEvent& aEvent );

    /**
     * Initialize a new git repository in the current project directory
    */
    void onGitInitializeProject( wxCommandEvent& event );

    /**
     * Commit the current project saved changes to the git repository
    */
    void onGitCommit( wxCommandEvent& event );

    /**
     * Pull the latest changes from the git repository
    */
    void onGitPullProject( wxCommandEvent& event );

    /**
     * Push the current project changes to the git repository
    */
    void onGitPushProject( wxCommandEvent& event );

    /**
     * Switch to a different branch in the git repository
    */
    void onGitSwitchBranch( wxCommandEvent& event );

    /**
     * Compare the current project to a different branch in the git repository
    */
    void onGitCompare( wxCommandEvent& event );

    /**
     * Remove the git repository from the current project directory
    */
    void onGitRemoveVCS( wxCommandEvent& event );

    /**
     * Add a file to the git index
    */
    void onGitAddToIndex( wxCommandEvent& event );

    /**
     * Remove a file from the git index
    */
    void onGitRemoveFromIndex( wxCommandEvent& event );

    /**
     * Sync the current project with the git repository
    */
    void onGitSyncProject( wxCommandEvent& event );

    /**
     * Fetch the latest changes from the git repository
    */
    void onGitFetch( wxCommandEvent& event );

    /**
     * Resolve conflicts in the git repository
    */
    void onGitResolveConflict( wxCommandEvent& event );

    /**
     * Revert the local repository to the last commit
    */
    void onGitRevertLocal( wxCommandEvent& event );

    /**
     * Run a selected jobs file
     */
    void onRunSelectedJobsFile( wxCommandEvent& event );

    /**
     * Updates the icons shown in the tree project to reflect the current git status
    */
    void updateGitStatusIcons();

    /**
     * This is a threaded call that will change the map of git status icons for use in
     *  the main thread
     */
    void updateGitStatusIconMap();

    /**
     * Updates the map of the wxtreeitemid to the name of each file for use in the thread
     */
    void updateTreeCache();

    /**
     * Returns true if the current project has any uncommitted changes
    */
    bool hasChangedFiles();

    /**
     * Returns true if the current project has local commits that have not been pushed to the
     * remote repository
    */
    bool hasLocalCommits();

    /**
     * Shutdown the file watcher.  Used when closing to prevent post-free access into the project
     * tree.  (Using the destructor doesn't work as wxWidgets defers destruction in some cases.)
     */
    void shutdownFileWatcher();

    /**
     * Function addItemToProjectTree
     * @brief  Add the file or directory aName to the project tree
     * @param aName = the filename or the directory name to add in tree
     * @param aParent = the wxTreeItemId item where to add sub tree items
     * @param aRecurse = true to add file or subdir names to the current tree item
     *                   false to stop file add.
     * @return the Id for the new tree item
     */
    wxTreeItemId addItemToProjectTree( const wxString& aName, const wxTreeItemId& aParent,
                                       std::vector<wxString>* aProjectNames, bool aRecurse );

    /**
     * Function findSubdirTreeItem
     * searches for the item in tree project which is the node of the subdirectory aSubDir
     * @param aSubDir = the directory to find in tree
     * @return the opaque reference to the tree item; if not found, return an invalid tree item
     *         so that wxTreeItemId::IsOk() can be used to test the returned value
     */
    wxTreeItemId findSubdirTreeItem( const wxString& aSubDir );

    /**
     * called when a file or directory is modified/created/deleted
     * The tree project is modified when a file or directory is created/deleted/renamed to
     * reflect the file change
     */
    void onFileSystemEvent( wxFileSystemWatcherEvent& event );

    void onThemeChanged( wxSysColourChangedEvent &aEvent );

    /**
     * Returns true if the file has already been added to the repository or
     * false if it has not been added yet.
    */
    bool canFileBeAddedToVCS( const wxString& aFilePath );

    void onGitSyncTimer( wxTimerEvent& event );

    void onGitStatusTimer( wxTimerEvent& event );

    void gitStatusTimerHandler();

public:
    KICAD_MANAGER_FRAME*    m_Parent;
    PROJECT_TREE*           m_TreeProject;

private:
    bool                    m_isRenaming;       // Are we in the process of renaming a file
    wxTreeItemId            m_root;
    std::vector<wxString>   m_filters;
    wxFileSystemWatcher*    m_watcher;          // file system watcher
    PROJECT_TREE_ITEM*      m_selectedItem;
    bool                    m_watcherNeedReset; // true if FileWatcherReset() must be called
                                                // (during an idle time for instance) after
                                                // the main loop event handler is started
    int                     m_gitLastError;
    wxString                m_gitCurrentBranchName;
    wxTimer                 m_gitSyncTimer;
    wxTimer                 m_gitStatusTimer;
    std::future<void>       m_gitSyncTask;
    std::future<void>       m_gitStatusIconTask;

    std::mutex                                       m_gitTreeCacheMutex;
    std::unordered_map<wxString, wxTreeItemId>       m_gitTreeCache;
    std::mutex                                       m_gitStatusMutex;
    std::map<wxTreeItemId, KIGIT_COMMON::GIT_STATUS> m_gitStatusIcons;
    bool                                             m_gitIconsInitialized;

    DECLARE_EVENT_TABLE()
};

#endif    // TREEPRJ_FRAME_H
