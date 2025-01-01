/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Ian McInerney <Ian.S.McInerney@ieee.org>
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

#ifndef FILEHISTORY_H_
#define FILEHISTORY_H_

#include <tool/action_menu.h>
#include <tool/selection_conditions.h>
#include <wx/filehistory.h>
#include <wx/menu.h>


class APP_SETTINGS_BASE;

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
     * @param aClearId is the ID to use for the clear menu menu item
     * @param aClearText is the text to use for the menu item that clears the history.
     */
    FILE_HISTORY( size_t aMaxFiles, int aBaseFileId, int aClearId,
            wxString aClearText = _( "Clear Recent Files" ) );

    /**
     * Loads history from a JSON settings object
     * @param aSettings is the settings object for this application
     */
    void Load( const APP_SETTINGS_BASE& aSettings );

    /**
     * Loads history from a list of strings
     * @param aList is a list of filenames to load
     */
    void Load( const std::vector<wxString>& aList );

    /**
     * Saves history into a JSON settings object
     * @param aSettings is the settings object to save into
     */
    void Save( APP_SETTINGS_BASE& aSettings );

    /**
     * Saves history into a list of strings
     * @param aList is a pointer to a string vector to clear and fill with the file history
     */
    void Save( std::vector<wxString>* aList );

    // Hide warnings about these virtual functions
    using wxFileHistory::Load;
    using wxFileHistory::Save;

    /**
     * Adds a file to the history.
     *
     * This function overrides the default wxWidgets method to iterate through all
     * menus associated with the file history, remove the added menu items, lets wx
     * add the new files, and then re-adds the clear menu item.
     *
     * @param aFile is the filename of the file to add to the history.
     */
    void AddFileToHistory( const wxString& aFile ) override;

    /**
     * Add the files to all registered menus.
     */
    void AddFilesToMenu() override
    {
        // This is needed to ensure that the proper base class function is called
        wxFileHistory::AddFilesToMenu();
    }

    /**
     * Add the files to the specified menu
     *
     * @param aMenu is the menu to operate on.
     */
    void AddFilesToMenu( wxMenu* aMenu ) override;

    /**
     * Update the number of files that will be contained inside the file history.
     *
     * @param aMaxFiles is the new number of files for the history
     */
    void SetMaxFiles( size_t aMaxFiles );

    /**
     * Set the text displayed on the menu item that clears the entire menu.
     *
     * @param aClearText is the text to use for the menu item
     */
    void SetClearText( wxString aClearText )
    {
        m_clearText = aClearText;
    }

    /**
     * Update the text displayed on the menu item that clears the entire menu.
     * useful after language change.
     *
     * @param aMenu is the menu to operate on.
     * @param aClearText is the new text to use for the menu item
     */
    void UpdateClearText(  wxMenu* aMenu, wxString aClearText );

    /**
     * Clear all entries from the file history.
     */
    void ClearFileHistory();

    /**
     * Create a #SELECTION_CONDITION that can be used to enable a menu item when the
     * file history has items in it.
     *
     * @param aHistory is the file history to check for items
     * @return the selection condition function
     */
    static SELECTION_CONDITION FileHistoryNotEmpty( const FILE_HISTORY& aHistory );

protected:
    /**
     * Remove the clear menu item and the preceding separator from the given menu.
     *
     * @param aMenu is the menu to operate on
     */
    void doRemoveClearitem( wxMenu* aMenu );

    /**
     * Add the clear menu item and the preceding separator to the given menu.
     *
     * @param aMenu is the menu to operate on
     */
    void doAddClearItem( wxMenu* aMenu );

private:
    /**
     * Test if the file history is empty. This function is designed to be used with a
     * #SELECTION_CONDITION to enable/disable the file history menu.
     *
     * @param aSelection is unused
     * @param aHistory is the file history to test for items
     */
    static bool isHistoryNotEmpty( const SELECTION& aSelection, const FILE_HISTORY& aHistory );

    int      m_clearId;
    wxString m_clearText;
};

#endif
