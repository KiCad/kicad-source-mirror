/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2016-2017 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#ifndef __widget_hotkey_list__
#define __widget_hotkey_list__

#include <unordered_map>
#include <utility>
#include <vector>

#include <wx/treelist.h>
#include <wx/dataview.h>

#include <hotkeys_basic.h>
#include <hotkey_store.h>


class WIDGET_HOTKEY_CLIENT_DATA;

class WIDGET_HOTKEY_LIST : public wxTreeListCtrl
{
public:
    /**
     * Constructor WIDGET_HOTKEY_LIST
     * Create a WIDGET_HOTKEY_LIST.
     *
     * @param aParent - parent widget
     * @param aHotkeys - EDA_HOTKEY_CONFIG data - a hotkey store is constructed
     * from this.
     */
    WIDGET_HOTKEY_LIST( wxWindow* aParent, HOTKEY_STORE& aHotkeyStore, bool aReadOnly );

    /**
     * Method ApplyFilterString
     * Apply a filter string to the hotkey list, selecting which hotkeys
     * to show.
     *
     * @param aFilterStr the string to filter by
     */
    void ApplyFilterString( const wxString& aFilterStr );

    /**
     * Set hotkeys in the control to default or original values.
     * @param aResetToDefault if true, reset to the defaults inherent to the hotkeys, else
     *                        reset to the value they had when the dialog was invoked.
     */
    void ResetAllHotkeys( bool aResetToDefault );

    /**
     * Method TransferDataToControl
     * Load the hotkey data from the store into the control.
     * @return true iff the operation was successful
     */
    bool TransferDataToControl();

    /**
     * Method TransferDataFromControl
     * Save the hotkey data from the control.
     * @return true iff the operation was successful
     */
    bool TransferDataFromControl();

    /**
     * Static method MapKeypressToKeycode
     * Map a keypress event to the correct key code for use as a hotkey.
     */
    static long MapKeypressToKeycode( const wxKeyEvent& aEvent );

protected:
    /**
     * Method editItem
     * Prompt the user for a new hotkey given a list item.
     */
    void editItem( wxTreeListItem aItem );

    /**
     * Method resetItem
     * Reset the item to either the default, the value when the dialog was opened, or none.
     */
    void resetItem( wxTreeListItem aItem, int aResetId );

    /**
     * Method onActivated
     * Handle activation of a row.
     */
    void onActivated( wxTreeListEvent& aEvent );

    /**
     * Method onContextMenu
     * Handle right-click on a row.
     */
    void onContextMenu( wxTreeListEvent& aEvent );

    /**
     * Method onMenu
     * Handle activation of a context menu item.
     */
    void onMenu( wxCommandEvent& aEvent );

    /**
     * Method resolveKeyConflicts
     * Check if we can set a hotkey, and prompt the user if there is a conflict between keys.
     * The key code should already have been checked that it's not for the same entry as it's
     * current in, or else this method will prompt for the self-change.
     *
     * The method will do conflict resolution depending on aSectionTag.
     * g_CommonSectionTag means the key code must only be checked with the aSectionTag section
     * and g_CommonSectionTag section.
     *
     * @param aKey - key to check
     * @param aActionName - name of the action into which the key is proposed to be installed
     *
     * @return true iff the user accepted the overwrite or no conflict existed
     */
    bool resolveKeyConflicts( TOOL_ACTION* aAction, long aKey );

private:
    /**
     * Method getHKClientData
     * Return the WIDGET_HOTKEY_CLIENT_DATA for the given item, or NULL if the item is invalid.
     */
    WIDGET_HOTKEY_CLIENT_DATA* getHKClientData( wxTreeListItem aItem );

    /**
     * Get the WIDGET_HOTKEY_CLIENT_DATA form an item and assert if it isn't found. This is for
     * use when the data not being present indicates an error.
     */
    WIDGET_HOTKEY_CLIENT_DATA* getExpectedHkClientData( wxTreeListItem aItem );

    /**
     * Method updateFromClientData
     * Refresh the visible text on the widget from the rows' client data objects.
     */
    void updateFromClientData();

    /**
     * Method updateShownItems
     *
     * Update the items shown in the widget based on a given filter string.
     *
     * @param aFilterStr the string to filter with. Empty means no filter.
     */
    void updateShownItems( const wxString& aFilterStr );

    /**
     * Attempt to change the given hotkey to the given key code.
     *
     * If the hotkey conflicts, the user is prompted to change anyway (and in doing so, unset
     * the conflicting key), or cancel the attempt.
     *
     * @param aHotkey the change-able hotkey to try to change
     * @param aKey the key code to change it to
     */
    void changeHotkey( HOTKEY& aHotkey, long aKey );

    /**
     * Recalculates column widths after model has changed
     */
    void updateColumnWidths();

private:
    HOTKEY_STORE&  m_hk_store;
    bool           m_readOnly;

    std::unordered_map<long, wxString> m_reservedHotkeys;

    wxTreeListItem m_context_menu_item;
};

#endif // __widget_hotkey_list__
