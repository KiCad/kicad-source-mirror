/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Ian McInerney <Ian.S.McInerney@ieee.org>
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef FILEHISTORY_H_
#define FILEHISTORY_H_

#include <tool/action_menu.h>
#include <tool/selection_conditions.h>
#include <wx/filehistory.h>
#include <wx/menu.h>


/**
 * This class implements a file history object to store a list of files, that can then
 * be added to a menu.
 *
 * This class extends the wxWidgets wxFileHistory class to include KiCad specific items.
 */
class FILE_HISTORY : public wxFileHistory
{
public:
    /**
     * Create a file history object to store a list of files and add them to a menu.
     *
     * @param aMaxFiles is the number of files to store in the history
     * @param aBaseFileId is the ID to use for the first file menu item
     */
    FILE_HISTORY( size_t aMaxFiles, int aBaseFileId );

    /**
     * Adds a file to the history.
     *
     * This function overrides the default wxWidgets method to iterate through all
     * menus associated with the file history, and if they are of the FILE_HISTORY_MENU
     * type, call their RefreshMenu() function to update the menu display.
     *
     * @param aFile is the filename of the file to add to the history.
     */
    void AddFileToHistory( const wxString &aFile ) override;

    /**
     * Update the number of files that will be contained inside the file history.
     *
     * @param aMaxFiles is the new number of files for the history
     */
    void SetMaxFiles( size_t aMaxFiles );

    /**
     * Create a SELECTION_CONDITION that can be used to enable a menu item when the
     * file history has items in it.
     *
     * @param aHistory is the file history to check for items
     * @return the selection condition function
     */
    static SELECTION_CONDITION FileHistoryNotEmpty( const FILE_HISTORY& aHistory );

private:
    static bool isHistoryNotEmpty( const SELECTION& aSelection, const FILE_HISTORY& aHistory );
};

/**
 * This class implements a menu container for a file history. It adds in the ability to clear
 * the file history through a menu item.
 */
class FILE_HISTORY_MENU : public ACTION_MENU
{
public:
    /**
     * Create the file history menu.
     *
     * @param aHistory is the file history to use in the menu
     * @param aClearText is the text to use for the menu item that clears the history.
     */
    FILE_HISTORY_MENU( FILE_HISTORY& aHistory, wxString aClearText = _( "Clear Recent Files" ) );

    ~FILE_HISTORY_MENU();

    /**
     * Refresh the menu. This removes all entries from the menu and readds them, to ensure that the
     * clear menu item is at the bottom of the menu.
     */
    void RefreshMenu();

private:
    //! @copydoc ACTION_MENU::create()
    ACTION_MENU* create() const override;

    /**
     * Construct the menu by adding the file history and menu items.
     */
    void buildMenu();

    /**
     * Event handler for when the clear menu item is activated.
     *
     * @param aEvent the menu event
     */
    void onClearEntries( wxMenuEvent& aEvent );

    FILE_HISTORY& m_fileHistory;
    wxString      m_clearText;
};

#endif
