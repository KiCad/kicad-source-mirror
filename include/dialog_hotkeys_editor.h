/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2016 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @file dialog_hotkeys_editor.h
 */

#ifndef __dialog_hotkeys_editor__
#define __dialog_hotkeys_editor__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/choice.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/treelist.h>
#include <wx/dialog.h>
#include <wx/grid.h>

#include <vector>
#include <utility>

#include <hotkeys_basic.h>
#include <draw_frame.h>
#include <../common/dialogs/dialog_hotkeys_editor_base.h>

typedef std::pair<wxString, struct EDA_HOTKEY_CONFIG*> HOTKEYS_SECTION;
typedef std::vector<HOTKEYS_SECTION> HOTKEYS_SECTIONS;

typedef std::vector<EDA_HOTKEY> HOTKEY_LIST;

class HOTKEYS_EDITOR_DIALOG;
class DIALOG_HOTKEY_CLIENT_DATA;

/**
 * Class HOTKEY_LIST_CTRL
 * is a class to contain the contents of a hotkey editor tab page.
 */
class HOTKEY_LIST_CTRL : public wxTreeListCtrl
{
public:
    static HOTKEYS_SECTIONS Sections( EDA_HOTKEY_CONFIG* aHotkeys );

    HOTKEY_LIST_CTRL( wxWindow* aParent, const HOTKEYS_SECTIONS& aSections );
    ~HOTKEY_LIST_CTRL() {};

    /**
     * Function DeselectRow
     * Deselect the given row
     *
     * @param aRow is the row to deselect
     */
    void DeselectRow( int aRow );

    /**
     * Function TransferDataToControl
     * Load the hotkey data into the control.
     * @return true iff the operation was successful
     */
    bool TransferDataToControl();

    /**
     * Function TransferDataFromControl
     * Save the hotkey data from the control.
     * @return true iff the operation was successful
     */
    bool TransferDataFromControl();

    /**
     * Function ResolveKeyConflicts
     * Check if we can set a hotkey, this will prompt the user if there
     * is a conflict between keys. The key code should have already been
     * checked that it's not for the same entry as its currently in or else
     * it'll prompt the change on itself.
     * The function will do conflict detection depending on aSectionTag.
     * g_CommonSectionTag means the key code must be checked with all sections.
     * While other tags means the key code only must be checked with the aSectionTag
     * section and g_CommonSectionTag section.
     *
     * @param aKey is the key code that wants to be set
     * @param aSectionTag is the section tag that the key code came from
     *
     * @return True if the user accepted the overwrite or no conflict existed
     */
    bool ResolveKeyConflicts( long aKey, const wxString& aSectionTag );


    /**
     * Function CheckKeyConflicts
     * Check whether the given key conflicts with anything in this HOTKEY_LIST_CTRL.
     *
     * @param aKey - key to check
     * @param aSectionTag - section tag of the key
     * @param aConfKey - if not NULL, outparam holding the key this one conflicts with
     * @param aConfSect - if not NULL, outparam holding the section this one conflicts with
     */
    bool CheckKeyConflicts( long aKey, const wxString& aSectionTag,
            EDA_HOTKEY** aConfKey, EDA_HOTKEY_CONFIG** aConfSect );

    /**
     * Function UpdateFromClientData
     * Update all visible items from the data stored in their client data objects.
     */
    void UpdateFromClientData();

private:
    HOTKEYS_SECTIONS m_sections;
    std::vector< HOTKEY_LIST > m_hotkeys;
    std::vector< wxTreeListItem > m_items;

    /**
     * Function GetSelHKClientData
     * Return the DIALOG_HOTKEY_CLIENT_DATA for the item being edited, or NULL if none is selected.
     */
    DIALOG_HOTKEY_CLIENT_DATA* GetSelHKClientData();

    /**
     * Function GetHKClientData
     * Return the DIALOG_HOTKEY_CLIENT_DATA for the given item, or NULL if invalid.
     */
    DIALOG_HOTKEY_CLIENT_DATA* GetHKClientData( wxTreeListItem aItem );

protected:
    /**
     * Function LoadSection
     * Generates a HOTKEY_LIST from the given hotkey configuration array and
     * pushes it to m_hotkeys.
     *
     * @param aSection is a pointer to the hotkey configuration array
     */
    void LoadSection( struct EDA_HOTKEY_CONFIG* aSection );

    /**
     * Function OnGetItemText
     * Returns the requested row, column data to the list control.
     *
     * @param aRow is the row of the data which matches our hotkeys vector as a index
     * @param aColumn is the column of the data which is either Command(0) or KeyCode(1)
     *
     * @return String containing the text for the specified row, column combination
     */
    wxString OnGetItemText( long aRow, long aColumn ) const;

    /**
     * Function OnChar
     * Decoded key press handler which is used to set key codes in the list control
     *
     * @param aEvent is the key press event, the keycode is retrieved from it
     */
    void OnChar( wxKeyEvent& aEvent );
};


/**
 * Class HOTKEYS_EDITOR_DIALOG
 * is the child class of HOTKEYS_EDITOR_DIALOG_BASE. This is the class
 * used to create a hotkey editor.
 */
class HOTKEYS_EDITOR_DIALOG : public HOTKEYS_EDITOR_DIALOG_BASE
{
protected:
    EDA_BASE_FRAME* m_parent;
    struct EDA_HOTKEY_CONFIG* m_hotkeys;

    HOTKEY_LIST_CTRL* m_hotkeyListCtrl;

    bool TransferDataToWindow();
    bool TransferDataFromWindow();

public:
    HOTKEYS_EDITOR_DIALOG( EDA_BASE_FRAME* aParent, EDA_HOTKEY_CONFIG* aHotkeys );

    ~HOTKEYS_EDITOR_DIALOG() {};

private:

    /**
     * Function ResetClicked
     * Reinit the hotkeys to the initial state (removes all pending changes)
     *
     * @param aEvent is the button press event, unused
     */
    void ResetClicked( wxCommandEvent& aEvent );
};

/**
 * Function InstallHotkeyFrame
 * Create a hotkey editor dialog window with the provided hotkey configuration array
 *
 * @param aParent is the parent window
 * @param aHotkeys is the hotkey configuration array
 */
void InstallHotkeyFrame( EDA_BASE_FRAME* aParent, EDA_HOTKEY_CONFIG* aHotkeys );

#endif
