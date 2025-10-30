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

/**
 * @file dialog_hotkey_list.h
 * Hotkey list dialog (as opposed to editor)
 */

#ifndef DIALOG_HOTKEYS_LIST_H
#define DIALOG_HOTKEYS_LIST_H


#include <dialog_shim.h>

class PANEL_HOTKEYS_EDITOR;


/**
 * A dialog that presents the user with a list of hotkeys and allows editing their bindings.
 */
class DIALOG_LIST_HOTKEYS: public DIALOG_SHIM
{
public:

    /**
     * Construct a hotkey list dialog on the given frame
     *
     * @param aParent the parent frame
     */
    DIALOG_LIST_HOTKEYS( EDA_BASE_FRAME* aParent );

protected:

    /**
     * Called on dialog initialisation - inits the dialog's own widgets
     */
    bool TransferDataToWindow() override;

    /**
     * Called on dialog close to save the hotkey changes
     */
    bool TransferDataFromWindow() override;

private:
    PANEL_HOTKEYS_EDITOR* m_hk_list;
};

#endif // DIALOG_HOTKEYS_LIST_H