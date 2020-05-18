/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney@ieee.org>
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

#include <wx/infobar.h>
#include <wx/wx.h>

class wxAuiManager;


/**
 * A modified version of the wxInfoBar class that allows us to:
 *     * Show the close button along with the other buttons
 *     * Remove all user-provided buttons at once
 *
 * This inherits from the generic infobar because the native infobar
 * on GTK doesn't include the icon on the left and it looks worse.
 */
class WX_INFOBAR : public wxInfoBarGeneric
{
public:
    /**
     * Construct an infobar that can exist inside an AUI managed frame.
     *
     * @param aParent is the parent
     * @param aMgr is the AUI manager that this infobar is added to
     * @param aWinId is the ID for this infobar object
     */
    WX_INFOBAR( wxWindow* aParent, wxAuiManager* aMgr = nullptr, wxWindowID aWinid = wxID_ANY );

    /**
     * Add the default close button to the infobar on the right side.
     *
     * @param aTooltip is the tooltip to give the close button
     */
    void AddCloseButton( const wxString& aTooltip = _( "Hide this message." ) );

    /**
     * Add an already created button to the infobar.
     * New buttons are added in the right-most position.
     *
     * @param aButton is the button to add
     */
    void AddButton( wxButton* aButton );

    /**
     * Add a button with the provided ID and text.
     * The new button is created on the right-most positon.
     *
     * @param aId is the ID to assign to the button
     * @param aLabel is the text for the button
     */
    void AddButton( wxWindowID aId, const wxString& aLabel = wxEmptyString ) override;

    /**
     * Remove all the buttons that have been added by the user.
     */
    void RemoveAllButtons();

    /**
     * Show the info bar with the provided message and icon.
     *
     * @param aMessage is the message to display
     * @param aFlags is the flag containing the icon to display on the left side of the infobar
     */
    void ShowMessage( const wxString& aMessage, int aFlags = wxICON_INFORMATION ) override;

    /**
     * Dismisses the infobar and updates the containing layout and AUI manager
     * (if one is provided).
     */
    void Dismiss() override;

protected:
    /**
     * Event handler for the close button.
     * This is bound to ID_CLOSE_INFOBAR on the infobar.
     */
    void OnCloseButton( wxCommandEvent& aEvt );

    /**
     * Update the AUI pane to show or hide this infobar.
     *
     * @param aShow is true to show the pane
     */
    void UpdateAuiLayout( bool aShow );

    wxAuiManager* m_auiManager;

    DECLARE_EVENT_TABLE()
};
