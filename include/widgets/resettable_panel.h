/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef RESETTABLE_PANEL_H_
#define RESETTABLE_PANEL_H_

#include <wx/panel.h>
#include <id.h>

class PAGED_DIALOG;

#define wxRESETTABLE 0x00008000
#define ID_RESET_PANEL ID_PREFERENCES_RESET_PANEL

/**
 * A wxPanel that is designed to be reset in a standard manner.
 */
class RESETTABLE_PANEL : public wxPanel
{
public:
    RESETTABLE_PANEL( wxWindow* aParent, wxWindowID aId = wxID_ANY,
                      const wxPoint& aPos = wxDefaultPosition,
                      const wxSize& aSize = wxSize( -1,-1 ),
                      long aStyle = wxTAB_TRAVERSAL,
                      const wxString& aName = wxEmptyString )
        : wxPanel( aParent, aId, aPos, aSize, aStyle | wxRESETTABLE, aName )
    {
        Bind( wxEVT_COMMAND_BUTTON_CLICKED,
                [&]( wxCommandEvent& aCmd )
                {
                    if( aCmd.GetId() == ID_RESET_PANEL )
                        ResetPanel();
                    else
                        aCmd.Skip();
                } );
    }

    /**
     * Reset the contents of this panel.
     */
    virtual void ResetPanel() = 0;

    /**
     * Get the tooltip the reset button should display when showing this panel.
     *
     * @return the tooltip
     */
    virtual wxString GetResetTooltip() const
    {
        return _( "Reset all settings on this page to their default" );
    }

    /**
     * Overridden to supply the reset button tooltip when queried with { -INT_MAX, INT_MAX }.
     */
    wxString GetHelpTextAtPoint( const wxPoint& aPt, wxHelpEvent::Origin aOrigin ) const override
    {
        if( aPt == wxPoint( -INT_MAX, INT_MAX ) )
            return GetResetTooltip();
        else
            return wxPanel::GetHelpTextAtPoint( aPt, aOrigin );
    }
};

#endif
