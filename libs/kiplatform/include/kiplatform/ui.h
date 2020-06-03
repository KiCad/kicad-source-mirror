/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <Ian.S.McInerney at ieee.org>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KIPLATFORM_UI_H_
#define KIPLATFORM_UI_H_

class wxNonOwnedWindow;
class wxWindow;

namespace KIPLATFORM
{
    namespace UI
    {
        /**
         * Pass the current focus to the window. On OSX this will forcefully give the focus to
         * the desired window, while on MSW and GTK it will simply call the wxWidgets SetFocus()
         * function.
         *
         * @param aWindow is the window to pass focus to
         */
        void ForceFocus( wxWindow* aWindow );

        /**
         * Move a window's parent to be the top-level window and force the window to be on top.
         *
         * This only has an affect for OSX, it is a NOP for GTK and MSW.
         *
         * Apple in its infinite wisdom will raise a disabled window before even passing
         * us the event, so we have no way to stop it.  Instead, we must set an order on
         * the windows so that the quasi-modal will be pushed in front of the disabled
         * window when it is raised.
         *
         * @param aWindow is the window to reparent
         */
        void ReparentQuasiModal( wxNonOwnedWindow* aWindow );
    }
}

#endif // KIPLATFORM_UI_H_
