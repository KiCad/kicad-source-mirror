/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <wx/listctrl.h>
#include <wx/dialog.h>
#include <wx/grid.h>

#include <hotkeys_basic.h>
#include <draw_frame.h>
#include <../common/dialogs/dialog_hotkeys_editor_base.h>

class HOTKEYS_EDITOR_DIALOG;

/**
 * Class HOTKEY_LIST_CTRL
 * is a class to contain the contents of a hotkey editor tab page.
 */
class HOTKEY_LIST_CTRL : public wxListCtrl
{
public:
    HOTKEY_LIST_CTRL( wxWindow* aParent, struct EDA_HOTKEY_CONFIG* aSection );
    ~HOTKEY_LIST_CTRL() {};

    /**
     * Function DeselectRow
     * Deselect the given row
     *
     * @param aRow is the row to deselect
     */
    void DeselectRow( int aRow );

    /**
     * Function GetHotkeys
     * Access to return the vector used for the list control data. This will contain the
     * "live" state of the user's configuration.
     *
     * @return Pointer to vector of hotkey settings
     */
    std::vector< EDA_HOTKEY* >& GetHotkeys() { return m_hotkeys; }

    /**
     * Function RestoreFrom
     * Restores list control hotkey keycodes to the keycodes present in the
     * given hotkey configuration array.
     *
     * @param aSection is a pointer to the hotkey configuration array
     */
    void RestoreFrom( struct EDA_HOTKEY_CONFIG* aSection );

private:
    int m_curEditingRow;
    wxString* m_sectionTag;
    std::vector< EDA_HOTKEY* > m_hotkeys;

    /**
     * Function recalculateColumns
     * Adjusts the width of grid columns in proportion of the max text length of both
     */
    void recalculateColumns();

protected:
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

    /**
     * Function OnListItemSelected
     * Item selection handler which is used to record what index is selected to alter
     * update with the key press
     *
     * @param aEvent is the button press event, unused
     */
    void OnListItemSelected( wxListEvent& aEvent );

    /**
     * Function OnSize
     * Sizing update handler to recompute the column widths dynamically and maximize them.
     * Due to wxWidget's broken autosizing support (it's completely inconsistent across
     * platforms), we just do it based on a scale of
     *
     * @param aEvent is the button press event, unused
     */
    void OnSize( wxSizeEvent& aEvent );
};

/**
 * Class HOTKEY_SECTION_PAGE
 * is a class to contain the contents of a hotkey editor tab page.
 */
class HOTKEY_SECTION_PAGE : public wxPanel
{
public:
private:
    EDA_HOTKEY_CONFIG*  m_hotkeySection;
    HOTKEY_LIST_CTRL *m_hotkeyList;
    HOTKEYS_EDITOR_DIALOG* m_dialog;

public:
    /** Constructor to create a setup page for one netlist format.
     * Used in Netlist format Dialog box creation
     * @param parent = wxNotebook * parent
     * @param title = title (name) of the notebook page
     * @param id_NetType = netlist type id
     */
    HOTKEY_SECTION_PAGE( HOTKEYS_EDITOR_DIALOG* aDialog, wxNotebook* aParent,
                         const wxString& aTitle,
                         EDA_HOTKEY_CONFIG* aSection );
    ~HOTKEY_SECTION_PAGE() {};

    /**
     * Function Restore
     * Resets the hotkeys back to their original unedited state
     */
    void Restore();

    /**
     * Function GetHotkeys
     * Accessor to retrieve hotkeys list from list control
     *
     * @return Pointer to vector used for list control data
     */
    std::vector< EDA_HOTKEY* >& GetHotkeys() { return m_hotkeyList->GetHotkeys(); }

    /**
     * Function GetHotkeySection
     * Accessor to retrieve hotkey configuration array assigned to a tab control page
     *
     * @return Pointer to hotkey configuration array
     */
    EDA_HOTKEY_CONFIG* GetHotkeySection() { return m_hotkeySection; }

    /**
     * Function GetDialog
     * Returns pointer to parent dialog window
     *
     * @return Pointer to parent dialog window
     */
    HOTKEYS_EDITOR_DIALOG* GetDialog() { return m_dialog; }
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

    std::vector<HOTKEY_SECTION_PAGE*> m_hotkeySectionPages;

public:
    HOTKEYS_EDITOR_DIALOG( EDA_BASE_FRAME* aParent, EDA_HOTKEY_CONFIG* aHotkeys );

    ~HOTKEYS_EDITOR_DIALOG() {};

    /**
     * Function CanSetKey
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
    bool CanSetKey( long aKey, const wxString* aSectionTag );

private:
    /**
     * Function OnOKClicked
     * Close the dialog and make save all changes to hotkeys
     *
     * @param aEvent is the button press event, unused
     */
    void OnOKClicked( wxCommandEvent& aEvent );

    /**
     * Function CancelClicked
     * Close the dialog and make no changes to hotkeys
     *
     * @param aEvent is the button press event, unused
     */
    void CancelClicked( wxCommandEvent& aEvent );

    /**
     * Function UndoClicked
     * Reinit the hotkeys to the initial state (removes all pending changes)
     *
     * @param aEvent is the button press event, unused
     */
    void UndoClicked( wxCommandEvent& aEvent );
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
