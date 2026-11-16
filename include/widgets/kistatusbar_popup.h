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

#ifndef KISTATUSBAR_POPUP_H
#define KISTATUSBAR_POPUP_H

#include <typeinfo>

#include <wx/popupwin.h>

/**
 * A list popped up from a status bar icon.
 *
 * It opens above and to the left of the icon so it stays inside the frame, since Wayland never moves a popup back
 * on screen.  Escape or a click outside dismisses it, and a dismissed popup destroys itself.
 */
class KISTATUSBAR_POPUP : public wxPopupTransientWindow
{
public:
    explicit KISTATUSBAR_POPUP( wxWindow* aParent );

    /**
     * Show the popup with its bottom-right corner at \a aCorner, in screen coordinates.
     */
    void PopupAt( const wxPoint& aCorner );

    /// Dismiss the popup as a click outside would, which destroys it
    using wxPopupTransientWindow::DismissAndNotify;

    /**
     * Return true, once, for a click that arrives just after a popup of type \a aPopupType was dismissed.
     *
     * The click that dismisses a popup can still reach the icon under it, which must not open it again.
     */
    static bool IsDismissingClick( const std::type_info& aPopupType );

protected:
    void OnDismiss() override;

private:
    void onCharHook( wxKeyEvent& aEvent );

    static long long             s_lastDismissed;
    static const std::type_info* s_lastDismissedType;
};

#endif // KISTATUSBAR_POPUP_H
