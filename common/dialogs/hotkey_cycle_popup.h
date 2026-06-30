/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef  HOTKEY_CYCLE_POPUP_H
#define  HOTKEY_CYCLE_POPUP_H

#include <eda_view_switcher_base.h>

class EDA_DRAW_FRAME;
class wxTimer;

/**
 * Similar to EDA_VIEW_SWITCHER, this dialog is a popup that shows feedback when using a hotkey to
 * cycle through a set of options.  This variant is designed for use with single-stroke hotkeys
 * (rather than chorded hotkeys like Ctrl+Tab) as feedback rather than as an interactive selector.
 */
class HOTKEY_CYCLE_POPUP : public EDA_VIEW_SWITCHER_BASE
{
public:
    HOTKEY_CYCLE_POPUP( EDA_DRAW_FRAME* aParent );

    ~HOTKEY_CYCLE_POPUP();

    void Popup( const wxString& aTitle, const wxArrayString& aItems, int aSelection );

    bool Show( bool aShow ) override;

protected:
    bool TryBefore( wxEvent& aEvent ) override;

private:
    wxTimer* m_showTimer;
    EDA_DRAW_FRAME* m_drawFrame;
};

#endif    // HOTKEY_CYCLE_POPUP_H
