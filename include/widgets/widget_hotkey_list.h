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

/**
 * @file widget_hotkey_list.h
 */

#ifndef __widget_hotkey_list__
#define __widget_hotkey_list__

#include <utility>
#include <vector>

#include <wx/treelist.h>
#include <widgets/two_column_tree_list.h>

#include <hotkeys_basic.h>
#include <hotkey_store.h>


class WIDGET_HOTKEY_CLIENT_DATA;

class WIDGET_HOTKEY_LIST : public TWO_COLUMN_TREE_LIST
{
    HOTKEY_STORE&               m_hk_store;

    wxTreeListItem              m_context_menu_item;

    /**
     * Method GetHKClientData
     * Return the WIDGET_HOTKEY_CLIENT_DATA for the given item, or NULL if the
     * item is invalid.
     */
    WIDGET_HOTKEY_CLIENT_DATA* GetHKClientData( wxTreeListItem aItem );

    /**
     * Method GetSelHKClientData
     * Return the WIDGET_HOTKEY_CLIENT_DATA for the item being edited, or NULL if
     * none is selected.
     */
    WIDGET_HOTKEY_CLIENT_DATA* GetSelHKClientData();

    /**
     * Method UpdateFromClientData
     * Refresh the visible text on the widget from the rows' client data objects.
     */
    void UpdateFromClientData();

protected:

    /**
     * Method EditItem
     * Prompt the user for a new hotkey given a list item.
     */
    void EditItem( wxTreeListItem aItem );

    /**
     * Method ResetItem
     * Reset the item to the original from the dialog was created.
     */
    void ResetItem( wxTreeListItem aItem );

    /**
     * Method ResetItemToDefault
     * Reset the item to the default value.
     */
    void ResetItemToDefault( wxTreeListItem aItem );

    /**
     * Method OnActivated
     * Handle activation of a row.
     */
    void OnActivated( wxTreeListEvent& aEvent );

    /**
     * Method OnContextMenu
     * Handle right-click on a row.
     */
    void OnContextMenu( wxTreeListEvent& aEvent );

    /**
     * Method OnMenu
     * Handle activation of a context menu item.
     */
    void OnMenu( wxCommandEvent& aEvent );

    /**
     * Function OnSize
     * Handle resizing of the control. Overrides the buggy wxTreeListCtrl::OnSize.
     */
    void OnSize( wxSizeEvent& aEvent );

    /**
     * Method ResolveKeyConflicts
     * Check if we can set a hotkey, and prompt the user if there is a conflict between
     * keys. The key code should already have been checked that it's not for the same
     * entry as it's current in, or else this method will prompt for the self-change.
     *
     * The method will do conflict resolution depending on aSectionTag.
     * g_CommonSectionTag means the key code must only be checkd with the aSectionTag
     * section and g_CommonSectionTag section.
     *
     * @param aKey - key to check
     * @param aSectionTag - section tag into which the key is proposed to be installed
     *
     * @return true iff the user accepted the overwrite or no conflict existed
     */
    bool ResolveKeyConflicts( long aKey, const wxString& aSectionTag );

public:
    /**
     * Constructor WIDGET_HOTKEY_LIST
     * Create a WIDGET_HOTKEY_LIST.
     *
     * @param aParent - parent widget
     * @param aHotkeys - EDA_HOTKEY_CONFIG data - a hotkey store is constructed
     * from this.
     */
    WIDGET_HOTKEY_LIST( wxWindow* aParent, HOTKEY_STORE& aHotkeyStore );

    /**
     * Method InstallOnPanel
     * Install this WIDGET_HOTKEY_LIST onto an empty panel. This is useful
     * when combining with wxFormBuilder, as an empty panel can be left as a
     * placeholder in the layout.
     */
    void InstallOnPanel( wxPanel* aPanel );

    /**
     * Set hotkeys in the control to default or original values.
     * @param aResetToDefault if true,.reset to the defaults inherent to the
     * hotkeym, else reset to the value they had when the dialog was invoked.
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
};

#endif // __widget_hotkey_list__
