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

#include <widgets/kistatusbar_popup.h>

#include <wx/time.h>


long long KISTATUSBAR_POPUP::s_lastDismissed = 0;


KISTATUSBAR_POPUP::KISTATUSBAR_POPUP( wxWindow* aParent ) :
        wxPopupTransientWindow( aParent, wxBORDER_SIMPLE | wxPU_CONTAINS_CONTROLS )
{
    SetSize( wxSize( 300, 150 ) );

    Bind( wxEVT_CHAR_HOOK, &KISTATUSBAR_POPUP::onCharHook, this );
}


void KISTATUSBAR_POPUP::PopupAt( const wxPoint& aCorner )
{
    Move( aCorner - GetSize() );
    Popup();
}


bool KISTATUSBAR_POPUP::IsDismissingClick()
{
    if( wxGetLocalTimeMillis().GetValue() - s_lastDismissed >= 300 )
        return false;

    s_lastDismissed = 0;
    return true;
}


void KISTATUSBAR_POPUP::OnDismiss()
{
    s_lastDismissed = wxGetLocalTimeMillis().GetValue();

    // MSW dismisses again on the deactivation that hiding on Escape causes
    if( !wxPendingDelete.Member( this ) )
        Destroy();
}


void KISTATUSBAR_POPUP::onCharHook( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_ESCAPE )
        DismissAndNotify();
    else
        aEvent.Skip();
}
