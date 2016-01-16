/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2016 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @file widget_hotkey_list
 */

#ifndef __widget_hotkey_list__
#define __widget_hotkey_list__

#include <utility>
#include <vector>

#include <wx/treelist.h>

#include <hotkeys_basic.h>

/// Width of the hotkey list
const int WIDGET_HOTKEY_LIST_HKCOLUMN_WIDTH = 100;

/// Extra margin to compensate for vertical scrollbar
const int WIDGET_HOTKEY_LIST_HMARGIN = 30;

/**
 * struct HOTKEY_SECTION
 * Associates a hotkey configuration with a name.
 */
struct HOTKEY_SECTION
{
    wxString            m_name;
    EDA_HOTKEY_CONFIG*  m_section;
};

typedef std::vector<HOTKEY_SECTION> HOTKEY_SECTIONS;
typedef std::vector<EDA_HOTKEY>     HOTKEY_LIST;

class WIDGET_HOTKEY_CLIENT_DATA;

class WIDGET_HOTKEY_LIST : public wxTreeListCtrl
{
    HOTKEY_SECTIONS             m_sections;
    std::vector<HOTKEY_LIST>    m_hotkeys;

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
     * Method LoadSection
     * Generates a HOTKEY_LIST from the given hotkey configuration array and pushes
     * it to m_hotkeys.
     */
    void LoadSection( EDA_HOTKEY_CONFIG* aSection );

    /**
     * Function OnChar
     * Handle keypress.
     * XXX REMOVE
     */
    void OnChar( wxKeyEvent& aEvent );

    /**
     * Function OnSize
     * Handle resizing of the control. Overrides the buggy wxTreeListCtrl::OnSize.
     */
    void OnSize( wxSizeEvent& aEvent );

    /**
     * Method CheckKeyConflicts
     * Check whether the given key conflicts with anything in this WIDGET_HOTKEY_LIST.
     *
     * @param aKey - key to check
     * @param aSectionTag - section tag into which the key is proposed to be installed
     * @param aConfKey - if not NULL, outparam getting the key this one conflicts with
     * @param aConfSect - if not NULL, outparam getting the section this one conflicts with
     */
    bool CheckKeyConflicts( long aKey, const wxString& aSectionTag,
            EDA_HOTKEY** aConfKey, EDA_HOTKEY_CONFIG** aConfSect );

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
     * @param aSections - list of the hotkey sections to display and their names.
     *  See WIDGET_HOTKEY_LIST::GenSections for a way to generate these easily
     *  from an EDA_HOTKEY_CONFIG*.
     */
    WIDGET_HOTKEY_LIST( wxWindow* aParent, const HOTKEY_SECTIONS& aSections );

    /**
     * Static method GenSections
     * Generate a list of sections and names from an EDA_HOTKEY_CONFIG*. Titles
     * will be looked up from translations.
     */
    static HOTKEY_SECTIONS GenSections( EDA_HOTKEY_CONFIG* aHotkeys );

    /**
     * Method TransferDataToControl
     * Load the hotkey data into the control. It is safe to call this multiple times,
     * for example to reset the control.
     * @return true iff the operation was successful
     */
    bool TransferDataToControl();

    /**
     * Method TransferDataFromControl
     * Save the hotkey data from the control.
     * @return true iff the operation was successful
     */
    bool TransferDataFromControl();
};

#endif // __widget_hotkey_list__
