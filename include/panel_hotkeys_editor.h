/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2018 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef PANEL_HOTKEYS_EDITOR_H
#define PANEL_HOTKEYS_EDITOR_H

#include <hotkeys_basic.h>
#include <hotkey_store.h>

#include "../common/dialogs/panel_hotkeys_editor_base.h"
#include <widgets/widget_hotkey_list.h>


class PANEL_HOTKEYS_EDITOR : public PANEL_HOTKEYS_EDITOR_BASE
{
protected:
    EDA_BASE_FRAME*           m_frame;
    struct EDA_HOTKEY_CONFIG* m_hotkeys;
    struct EDA_HOTKEY_CONFIG* m_showHotkeys;
    wxString                  m_nickname;

    HOTKEY_STORE              m_hotkeyStore;
    WIDGET_HOTKEY_LIST*       m_hotkeyListCtrl;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

public:
    PANEL_HOTKEYS_EDITOR( EDA_BASE_FRAME* aFrame, wxWindow* aWindow,
                          EDA_HOTKEY_CONFIG* aHotkeys, EDA_HOTKEY_CONFIG* aShowHotkeys,
                          const wxString& aNickname );

    ~PANEL_HOTKEYS_EDITOR() {};

private:

    /**
     * Function ResetClicked
     * Reinit the hotkeys to the initial state (removes all pending changes)
     *
     * @param aEvent is the button press event, unused
     */
    void ResetClicked( wxCommandEvent& aEvent ) override;

    /**
     * Function DefaultsClicked
     * Set the hotkeys to the default values (values after installation)
     *
     * @param aEvent is the button press event, unused
     */
    void DefaultsClicked( wxCommandEvent& aEvent ) override;

    void OnExport( wxCommandEvent& aEvent ) override;
    void OnImport( wxCommandEvent& aEvent ) override;
};


#endif  // PANEL_HOTKEYS_EDITOR_H
