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

#include <widgets/widget_hotkey_list.h>

#include "wx/panel.h"


class wxPanel;
class wxSizer;
class TOOL_MANAGER;


class PANEL_HOTKEYS_EDITOR : public wxPanel
{
protected:
    EDA_BASE_FRAME*            m_frame;
    bool                       m_readOnly;
    
    std::vector<TOOL_MANAGER*> m_toolManagers;
    HOTKEY_STORE               m_hotkeyStore;
    WIDGET_HOTKEY_LIST*        m_hotkeyListCtrl;

public:
    PANEL_HOTKEYS_EDITOR( EDA_BASE_FRAME* aFrame, wxWindow* aWindow, bool aReadOnly );

    void AddHotKeys( TOOL_MANAGER* aToolMgr );
    
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:

    /**
     * Install the button panel (global reset/default, import/export)
     * @param aSizer the dialog to install on
     */
    void installButtons( wxSizer* aSizer );

    /**
     * Function OnFilterSearch
     * Handle a change in the hoteky filter text
     *
     * @param aEvent: the search event, used to get the search query
     */
    void OnFilterSearch( wxCommandEvent& aEvent );

    /**
     * Function ImportHotKeys
     * Puts up a dialog allowing the user to select a hotkeys file and then overlays those
     * hotkeys onto the current hotkey store.
     */
    void ImportHotKeys();

};


#endif  // PANEL_HOTKEYS_EDITOR_H
