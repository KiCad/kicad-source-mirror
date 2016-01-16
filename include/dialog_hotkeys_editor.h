/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2016 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @file dialog_hotkeys_editor.h
 */

#ifndef __dialog_hotkeys_editor__
#define __dialog_hotkeys_editor__

#include <hotkeys_basic.h>
#include <../common/dialogs/dialog_hotkeys_editor_base.h>
#include <widgets/widget_hotkey_list.h>

/**
 * Class HOTKEYS_EDITOR_DIALOG
 * is the child class of HOTKEYS_EDITOR_DIALOG_BASE. This is the class
 * used to create a hotkey editor.
 */
class HOTKEYS_EDITOR_DIALOG : public HOTKEYS_EDITOR_DIALOG_BASE
{
protected:
    struct EDA_HOTKEY_CONFIG* m_hotkeys;

    WIDGET_HOTKEY_LIST* m_hotkeyListCtrl;

    bool TransferDataToWindow();
    bool TransferDataFromWindow();

    virtual EDA_BASE_FRAME* GetParent()
    {
        return static_cast<EDA_BASE_FRAME*>( HOTKEYS_EDITOR_DIALOG_BASE::GetParent() );
    }

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
