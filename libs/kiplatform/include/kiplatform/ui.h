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

#include <wx/cursor.h>

class wxChoice;
class wxNonOwnedWindow;
class wxWindow;

namespace KIPLATFORM
{
    namespace UI
    {
        /**
         * Determine if the desktop interface is currently using a dark theme or a light theme.
         *
         * @return true if a dark theme is being used.
         */
        bool IsDarkTheme();

        wxColour GetDialogBGColour();

        /**
         * Pass the current focus to the window. On OSX this will forcefully give the focus to
         * the desired window, while on MSW and GTK it will simply call the wxWidgets SetFocus()
         * function.
         *
         * @param aWindow is the window to pass focus to
         */
        void ForceFocus( wxWindow* aWindow );

        /**
         * Check to see if the given window is the currently active window (e.g. the window
         * in the foreground the user is interacting with).
         *
         * @param aWindow is the window to check
         */
        bool IsWindowActive( wxWindow* aWindow );

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

        /*
         * An ugly hack to fix an issue on OSX: cmd+c closes the dialog instead of copying the
         * text if a button with wxID_CANCEL is used in a wxStdDialogButtonSizer created by
         * wxFormBuilder: the label is &Cancel, and this accelerator key has priority over the
         * standard copy accelerator.
         * Note: problem also exists in other languages; for instance cmd+a closes dialogs in
         * German because the button is &Abbrechen.
         */
        void FixupCancelButtonCmdKeyCollision( wxWindow* aWindow );

        /**
         * Checks if we designated a stock cursor for this OS as "OK" or else we may need to load a custom one
         *
         * @param aCursor is wxStockCursor we want to see if its acceptable
         */
        bool IsStockCursorOk( wxStockCursor aCursor );

        /**
         * Configure a wxChoice control to support a lot of entries by disabling functionality that makes
         * adding new items become very expensive.
         *
         * @param aChoice is the choice box to modify
         */
        void LargeChoiceBoxHack( wxChoice* aChoice );

        /**
         * Configure a wxChoice control to ellipsize the shown text in the button with the ellipses
         * placed at the end of the string.
         *
         * @param aChoice is the choice box to ellipsize
         */
        void EllipsizeChoiceBox( wxChoice* aChoice );

        /**
         * Tries to determine the pixel scaling factor currently in use for the window.  Under wx3.0, GTK
         * fails to properly detect the scale factor.
         * @param aWindow pointer to the window to check
         * @return Pixel scale factor in use, defaulting to the wxWidgets method
         */
        double GetPixelScaleFactor( const wxWindow* aWindow );

        /**
         * Tries to determine the content scaling factor currently in use for the window.
         * The content scaling factor is typically settable by the user and may differ from the
         * pixel scaling factor.
         */
        double GetContentScaleFactor( const wxWindow* aWindow );

        /**
         * Tries to determine the size of the viewport of a scrollable widget (wxDataViewCtrl, wxGrid)
         * that won't be obscured by scrollbars.
         * @param aWindow pointer to the scrollable widget to check
         * @return Viewport size that won't be obscured by scrollbars
         */
        wxSize GetUnobscuredSize( const wxWindow* aWindow );

        /**
         * Used to set overlay/non-overlay scrolling mode in a window.
         * Implemented only on GTK.
         */
        void SetOverlayScrolling( const wxWindow* aWindow, bool overlay );

        /**
         * If the user has disabled icons system-wide, we check that here
         */
        bool AllowIconsInMenus();

        /**
         * Move the mouse cursor to a specific position relative to the window
         * @param aWindow Window in which to position to mouse cursor
         * @param aX destination x position
         * @param aY destination y position
         */
        void WarpPointer( wxWindow* aWindow, int aX, int aY );

        /**
         * Configures the IME mode of a given control handle
         */
        void ImmControl( wxWindow* aWindow, bool aEnable );
    }
}

#endif // KIPLATFORM_UI_H_
