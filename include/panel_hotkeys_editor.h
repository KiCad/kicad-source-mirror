/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <widgets/resettable_panel.h>
#include <widgets/widget_hotkey_list.h>

#include "wx/panel.h"


class wxPanel;
class wxSizer;
class TOOL_MANAGER;
class wxSearchCtrl;


class PANEL_HOTKEYS_EDITOR : public RESETTABLE_PANEL
{
public:
    PANEL_HOTKEYS_EDITOR( EDA_BASE_FRAME* aFrame, wxWindow* aWindow );
    ~PANEL_HOTKEYS_EDITOR();

    std::vector<TOOL_ACTION*>& ActionsList() { return m_actions; }

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ResetPanel() override;

    wxString GetResetTooltip() const override
    {
        return _( "Reset all hotkeys to the built-in KiCad defaults" );
    }

    wxSizer* GetBottomSizer() { return m_bottomSizer; }

private:
    /**
     * Install the button panel (global reset/default, import/export)
     *
     * @param aSizer the dialog to install on.
     */
    void installButtons( wxSizer* aSizer );

    /**
     * Handle a change in the hotkey filter text.
     *
     * @param aEvent is the search event, used to get the search query.
     */
    void OnFilterSearch( wxCommandEvent& aEvent );

    /**
     * Put up a dialog allowing the user to select a hotkeys file and then overlays those
     * hotkeys onto the current hotkey store.
     */
    void ImportHotKeys();

    /**
     * Dump all actions and their hotkeys to a text file for inclusion in documentation.
     *
     * The format is asciidoc-compatible table rows.
     * This function is hidden behind an advanced config flag and not intended for users.
     */
    void dumpHotkeys();

    wxSearchCtrl* m_filterSearch;

protected:
    EDA_BASE_FRAME*            m_frame;
    wxSizer*                   m_bottomSizer;
    std::vector<TOOL_ACTION*>  m_actions;
    HOTKEY_STORE               m_hotkeyStore;
    WIDGET_HOTKEY_LIST*        m_hotkeyListCtrl;
};


#endif  // PANEL_HOTKEYS_EDITOR_H
