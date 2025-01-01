/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef WX_MENUBAR_H_
#define WX_MENUBAR_H_

#include <wx/accel.h>
#include <wx/menu.h>
#include <tool/action_menu.h>


/**
 * Wrapper around a wxMenuBar object that prevents the accelerator table from being used.
 *
 * It appears that on MSW the accelerator table of a wxMenuBar will be searched before key events
 * are passed to other items. This means key events matching hotkey combinations are converted to
 * menu events and never get passed to text controls. To work around this, simply don't let the
 * menubar have an accelerator table.
 * See https://gitlab.com/kicad/code/kicad/-/issues/1941
 *
 * Note that on OSX, menus also steal the key events from text controls, but that is done by OSX
 * itself, so other workarounds are included inside ACTION_MENU::OnMenuEvent() and
 * TOOL_DISPATCHER::DispatchWxEvent() to redirect key presses to text-based controls when they
 * have focus.
 */
class WX_MENUBAR : public wxMenuBar
{
public:
    // Just take all constructors
    using wxMenuBar::wxMenuBar;

    void SetAcceleratorTable( const wxAcceleratorTable& aTable ) override
    {
        // Don't use the passed in accelerator table, create a new empty one
        wxMenuBar::SetAcceleratorTable( wxAcceleratorTable() );
    }

    wxString GetMenuLabelText( size_t aPos ) const override
    {
        if( ACTION_MENU* actionMenu = dynamic_cast<ACTION_MENU*>( GetMenu( aPos ) ) )
        {
            wxString title = actionMenu->GetTitle();

            // Clear accelerator key markings
            title.Replace( wxS( " & " ), wxS( " {amp} " ) );
            title.Replace( wxS( "&" ), wxEmptyString );
            title.Replace( wxS( "{amp}" ), wxS( "&" ) );

            return title;
        }

        return wxMenuBar::GetMenuLabelText( aPos );
    }
};

#endif // COMMON_WIDGETS_WX_BUSY_INDICATOR__H
